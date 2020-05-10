#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <sys/select.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <fcntl.h>


#define IPC_PROC_SIGTERM SIGTERM
#define IPC_PROC_SIGKILL SIGKILL

#define IPC_PROC_BUFSIZE 1024


typedef struct {
  pid_t child;
  int fds[ 3 ];
  int exitstatus;
  char const* exitkind; /* "exit" or "signal" */
  char const* inbuf;
  size_t inlen; /* length of the string `inbuf` */
  char outbuf[ IPC_PROC_BUFSIZE ];
  char errbuf[ IPC_PROC_BUFSIZE ];
} ipc_proc_handle;


static void ipc_proc_error( char* buf, size_t len, int code ) {
  if( len > 0 && strerror_r( code, buf, len ) != (int)0 ) {
    strncpy( buf, "unknown error", len-1 );
    buf[ len-1 ] = '\0';
  }
}


static void ipc_proc_inithandle_( ipc_proc_handle* h ) {
  h->child = 0;
  h->fds[ 0 ] = h->fds[ 1 ] = h->fds[ 2 ] = -1;
  h->exitstatus = 0;
  h->exitkind = NULL;
  h->inbuf = NULL;
  h->inlen = 0;
}


static int ipc_proc_waitpid_( ipc_proc_handle* h, int flags ) {
  int rv = 0;
  if( h->exitkind != NULL )
    return 0;
  IPC_EINTR( rv, waitpid( h->child, &h->exitstatus, flags ) );
  switch( rv ) {
    case -1:
      return IPC_ERR( errno );
    case 0:
      return 0;
    default:
      if( WIFEXITED( h->exitstatus ) ) {
        h->exitkind = "exit";
        h->exitstatus = WEXITSTATUS( h->exitstatus );
      } else if( WIFSIGNALED( h->exitstatus ) ) {
        h->exitkind = "signal";
        h->exitstatus = WTERMSIG( h->exitstatus );
      } else
        h->exitkind = "exit";
  }
  return 0;
}


static int ipc_proc_kill( ipc_proc_handle* h, int signum ) {
  ipc_proc_waitpid_( h, WNOHANG ); /* non-blocking */
  if( h->exitkind == NULL ) {
    if( kill( h->child, signum ) < 0 )
      return IPC_ERR( errno );
  }
  return 0;
}


static int ipc_proc_wait( ipc_proc_handle* h, int* status,
                          char const** what ) {
  int rv = ipc_proc_waitpid_( h, 0 ); /* blocking */
  if( rv < 0 )
    return IPC_ERR( rv );
  *status = h->exitstatus;
  *what = h->exitkind;
  return 0;
}


static int ipc_proc_trywrite_( ipc_proc_handle* h, int* done ) {
  ssize_t nwritten = 0;
  IPC_EINTR( nwritten, write( h->fds[ 0 ], h->inbuf, h->inlen ) );
  if( nwritten < 0 ) {
    *done = 0;
    if( errno == EAGAIN || errno == EWOULDBLOCK )
      return 0;
    else {
      int saved_errno = errno;
      close( h->fds[ 0 ] );
      h->fds[ 0 ] = -1;
      if( saved_errno == EPIPE )
        return 0;
      else
        return IPC_ERR( saved_errno );
    }
  } else {
    h->inbuf += nwritten;
    h->inlen -= nwritten;
    if( h->inlen > 0 )
      *done = 0;
    else {
      h->inbuf = NULL;
      *done = 1;
    }
  }
  return 0;
}


static int ipc_proc_stdinready( ipc_proc_handle* h, int* ready ) {
  int rv = 0;
  if( h->fds[ 0 ] < 0 ) {
    *ready = 0;
    return 0;
  }
  if( h->inbuf == NULL ) {
    *ready = 1;
    return 0;
  }
  rv = ipc_proc_trywrite_( h, ready );
  if( rv != 0 )
    return IPC_ERR( rv );
  return 0;
}


static int ipc_proc_closestdin( ipc_proc_handle* h ) {
  if( h->fds[ 0 ] < 0 )
    return IPC_ERR( EBADF );
  if( close( h->fds[ 0 ] ) < 0 )
    return IPC_ERR( errno );
  h->fds[ 0 ] = -1;
  return 0;
}


static int ipc_proc_write( ipc_proc_handle* h, char const* s,
                           size_t len, int* completed ) {
  int rv = 0;
  if( h->fds[ 0 ] < 0 )
    return IPC_ERR( EBADF );
  if( h->inbuf != NULL )
    return IPC_ERR( EBUSY );
  h->inbuf = s;
  h->inlen = len;
  rv = ipc_proc_trywrite_( h, completed );
  if( rv != 0 )
    return IPC_ERR( rv );
  return 0;
}


static int ipc_proc_waitio( ipc_proc_handle* h, int* child_done,
                            int* stdin_done, int* stdout_done,
                            int* stderr_done ) {
  int nfds = -1;
  int rv = 0;
  fd_set readset, writeset;
  fd_set* readsetp = NULL;
  fd_set* writesetp = NULL;
  *child_done = *stdin_done = *stdout_done = *stderr_done = 0;
  FD_ZERO( &readset );
  FD_ZERO( &writeset );
#define ADD2SET( fdidx, set, extra ) \
  do { \
    if( h->fds[ fdidx ] >= 0 && (extra) ) { \
      set ## p = &set; \
      nfds = nfds > h->fds[ fdidx ] ? nfds : h->fds[ fdidx ]; \
      FD_SET( h->fds[ fdidx ], set ## p ); \
    } \
  } while( 0 )
  ADD2SET( 0, writeset, h->inbuf != NULL );
  ADD2SET( 1, readset, 1 );
  ADD2SET( 2, readset, 1 );
#undef ADD2SET
  if( nfds >= 0 ) {
    /* when the child dies, the select should return too */
    IPC_EINTR( rv, select( nfds+1, readsetp, writesetp, NULL, NULL ) );
    if( rv < 0 )
      return IPC_ERR( errno );
    /* check for dead child */
    rv = ipc_proc_waitpid_( h, WNOHANG );
    if( rv != 0 )
      return IPC_ERR( rv );
    if( h->exitkind != NULL )
      *child_done = 1;
  } else {
    /* no I/O configured, so we just wait for the child to die! */
    rv = ipc_proc_waitpid_( h, 0 );
    if( rv != 0 )
      return IPC_ERR( rv );
    *child_done = 1;
  }
  if( writesetp != NULL && FD_ISSET( h->fds[ 0 ], writesetp ) )
    *stdin_done = 1;
  if( readsetp != NULL && h->fds[ 1 ] >= 0 &&
      FD_ISSET( h->fds[ 1 ], readsetp ) )
    *stdout_done = 1;
  if( readsetp != NULL && h->fds[ 2 ] >= 0 &&
      FD_ISSET( h->fds[ 2 ], readsetp ) )
    *stderr_done = 1;
  return 0;
}


static int ipc_proc_get_( ipc_proc_handle* h, int fdindex, char* buf,
                          char const** data, size_t* len ) {
  ssize_t n = 0;
  if( h->fds[ fdindex ] < 0 )
    return IPC_ERR( EBADF );
  *data = buf;
  *len = 0;
  IPC_EINTR( n, read( h->fds[ fdindex ], buf, IPC_PROC_BUFSIZE ) );
  if( n < 0 ) {
    if( errno != EAGAIN && errno != EWOULDBLOCK ) {
      int saved_errno = errno;
      close( h->fds[ fdindex ] );
      h->fds[ fdindex ] = -1;
      return IPC_ERR( saved_errno );
    }
  } else if( n == 0 ) { /* EOF */
    close( h->fds[ fdindex ] );
    h->fds[ fdindex ] = -1;
    *len = 0;
  } else
    *len = n;
  return 0;
}

static int ipc_proc_stdoutready( ipc_proc_handle* h, char const** data,
                                 size_t* len ) {
  return ipc_proc_get_( h, 1, h->outbuf, data, len );
}

static int ipc_proc_stderrready( ipc_proc_handle* h, char const** data,
                                 size_t* len ) {
  return ipc_proc_get_( h, 2, h->errbuf, data, len );
}


static int ipc_proc_spawn( ipc_proc_handle* h, char const* cmdline,
                           FILE* cstdin, int pstdin,
                           FILE* cstdout, int pstdout,
                           FILE* cstderr, int pstderr ) {
  int fds[] = { -1, -1, -1, -1, -1, -1 };
  int stdin_fd = -1, stdout_fd = -1, stderr_fd = -1;
  int rv = 0;
/* macro to check that the given file descriptor is valid for select
 * to use. cleans up and returns error code if not. */
#define CHECK_FD( fd ) \
  do { \
    int d = (fd); \
    if( d < 0 || d >= FD_SETSIZE ) { \
      size_t i = 0; \
      for( i = 0; i < sizeof( fds )/sizeof( *fds ); ++i ) \
        if( fds[ i ] >= 0 ) \
          close( fds[ i ] ); \
      return IPC_ERR( EMFILE ); \
    } \
  } while( 0 )
/* macro to check that a value is >= 0. cleans up and returns error
 * code if not. */
#define CHECK_R( v ) \
  do { \
    if( (v) < 0 ) { \
      int saved_errno = errno; \
      size_t i = 0; \
      for( i = 0; i < sizeof( fds )/sizeof( *fds ); ++i ) \
        if( fds[ i ] >= 0 ) \
          close( fds[ i ] ); \
      return IPC_ERR( saved_errno ); \
    } \
  } while( 0 )
  ipc_proc_inithandle_( h );
  /* create pipes if necessary */
  if( cstdin != NULL ) {
    CHECK_R( stdin_fd = fileno( cstdin ) );
  } else if( pstdin ) {
    CHECK_R( pipe( fds ) );
    IPC_EINTR( rv, fcntl( fds[ 1 ], F_SETFL, O_NONBLOCK ) );
    CHECK_R( rv );
    CHECK_FD( h->fds[ 0 ] = fds[ 1 ] );
    stdin_fd = fds[ 0 ];
  }
  if( cstdout != NULL ) {
    CHECK_R( stdout_fd = fileno( cstdout ) );
  } else if( pstdout ) {
    CHECK_R( pipe( fds+2 ) );
    IPC_EINTR( rv, fcntl( fds[ 2 ], F_SETFL, O_NONBLOCK ) );
    CHECK_R( rv );
    CHECK_FD( h->fds[ 1 ] = fds[ 2 ] );
    stdout_fd = fds[ 3 ];
  }
  if( cstderr != NULL ) {
    CHECK_R( stderr_fd = fileno( cstderr ) );
  } else if( pstderr ) {
    CHECK_R( pipe( fds+4 ) );
    IPC_EINTR( rv, fcntl( fds[ 4 ], F_SETFL, O_NONBLOCK ) );
    CHECK_R( rv );
    CHECK_FD( h->fds[ 2 ] = fds[ 4 ] );
    stderr_fd = fds[ 5 ];
  }
  /* create child process */
  CHECK_R( h->child = fork() );
  if( h->child > 0 ) { /* parent */
    if( fds[ 0 ] >= 0 )
      close( fds[ 0 ] );
    if( fds[ 3 ] >= 0 )
      close( fds[ 3 ] );
    if( fds[ 5 ] >= 0 )
      close( fds[ 5 ] );
    return 0;
  }
#undef CHECK_FD
#undef CHECK_R
  /* here we are the child process */
  if( fds[ 1 ] >= 0 )
    close( fds[ 1 ] );
  if( fds[ 2 ] >= 0 )
    close( fds[ 2 ] );
  if( fds[ 4 ] >= 0 )
    close( fds[ 4 ] );
  /* setup stdin, stdout, and stderr for the child process */
  if( stdin_fd >= 0 ) {
    IPC_EINTR( rv, dup2( stdin_fd, STDIN_FILENO ) );
    close( stdin_fd );
    if( rv < 0 )
      _exit( 127 );
  }
  if( stdout_fd >= 0 ) {
    IPC_EINTR( rv, dup2( stdout_fd, STDOUT_FILENO ) );
    close( stdout_fd );
    if( rv < 0 )
      _exit( 127 );
  }
  if( stderr_fd >= 0 ) {
    IPC_EINTR( rv, dup2( stderr_fd, STDERR_FILENO ) );
    close( stderr_fd );
    if( rv < 0 )
      _exit( 127 );
  }
  signal( SIGPIPE, SIG_DFL );
  /* FIXME should we prevent file descriptors from leaking?! */
  /* start the new process */
  execl( "/bin/sh", "/bin/sh", "-c", cmdline, (char const*)NULL );
  _exit( 127 );
  return 0;
}


static int ipc_proc_prepare( void ) {
  if( signal( SIGPIPE, SIG_IGN ) == SIG_ERR )
    return IPC_ERR( errno );
  return 0;
}

