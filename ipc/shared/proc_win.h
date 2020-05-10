#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#define WIN32_LEAN_AND_MEAN
#include <windows.h>


/* Windows doesn't really use signals, and it doesn't have a mechanism
 * to gracefully shutdown an unrelated process, so we just kill the
 * child process dead for all signals. */
#define IPC_PROC_SIGTERM 0
#define IPC_PROC_SIGKILL 0

#define IPC_PROC_BUFSIZE 1024


typedef struct {
  HANDLE child;
  /* process info structure? */
  int exitstatus;
  char const* exitkind;
  char const* inbuf;
  size_t inlen; /* length of the string `inbuf` */
  char outbuf[ IPC_PROC_BUFSIZE ];
  char errbuf[ IPC_PROC_BUFSIZE ];
  OVERLAPPED inov;
  OVERLAPPED outov;
  OVERLAPPED errov;
} ipc_proc_handle;


static void ipc_proc_error( char* buf, size_t len, int code ) {
  if( len > 0 ) {
    if( 0 == FormatMessageA( FORMAT_MESSAGE_FROM_SYSTEM |
                             FORMAT_MESSAGE_IGNORE_INSERTS,
                             NULL,
                             code,
                             0,
                             buf,
                             len,
                             NULL ) ) {
      strncpy( buf, "unknown error", len-1 );
      buf[ len-1 ] = '\0';
    } else { /* Windows puts an extra newline in there! */
      size_t n = strlen( buf );
      while( n > 0 && isspace( (unsigned char)buf[ --n ] ) )
        buf[ n ] = '\0';
    }
  }
}


static int ipc_proc_waitprocess_( ipc_proc_handle* h, DWORD timeout ) {
  DWORD exitcode = 0;
  if( h->exitkind != NULL )
    return 0;
  switch( WaitForSingleObject( h->child, timeout ) ) {
    case WAIT_OBJECT_0:
      if( !GetExitCodeProcess( h->child, &exitcode ) )
        return IPC_ERR( GetLastError() );
      h->exitstatus = exitcode;
      h->exitkind = "exit";
      return 0;
    case WAIT_TIMEOUT:
      return 0;
    default:
      return IPC_ERR( GetLastError() );
  }
  return 0;
}


static int ipc_proc_kill( ipc_proc_handle* h, int signum ) {
  (void)signum;
  ipc_proc_waitprocess_( h, 0 );
  if( h->exitkind != 0 ) {
    if( !TerminateProcess( h->child, 3 ) )
      return IPC_ERR( GetLastError() );
  }
  return 0;
}


static int ipc_proc_wait( ipc_proc_handle* h, int* status,
                          char const** what ) {
  int rv = ipc_proc_waitprocess_( h, INFINITE );
  if( rv != 0 )
    return IPC_ERR( rv );
  *status = h->exitstatus;
  *what = h->exitkind;
  return 0;
}


static int ipc_proc_trywrite_( ipc_proc_handle* h, int* done ) {
  /* TODO */
  return 0;
}


static int ipc_proc_stdinready( ipc_proc_handle* h, int* done ) {
  /* TODO */
  return 0;
}


static int ipc_proc_closestdin( ipc_proc_handle* h ) {
  /* TODO */
  return 0;
}


static int ipc_proc_write( ipc_proc_handle* h, char const* s,
                           size_t len, int* completed ) {
  /* TODO */
  return 0;
}


static int ipc_proc_waitio( ipc_proc_handle* h, int* child_complete,
                            int* stdin_complete, int* stdout_complete,
                            int* stderr_complete ) {
  /* TODO */
  return 0;
}


static int ipc_proc_stdoutready( ipc_proc_handle* h, char const** data,
                                 size_t* len ) {
  /* TODO */
  return 0;
}

static int ipc_proc_stderrready( ipc_proc_handle* h, char const** data,
                                 size_t* len ) {
  /* TODO */
  return 0;
}


static int ipc_proc_spawn( ipc_proc_handle* h, char const* cmdline,
                           FILE* cstdin, int pstdin,
                           FILE* cstdout, int pstdout,
                           FILE* cstderr, int pstderr ) {
  /* TODO */
  return 0;
}


static int ipc_proc_prepare( void ) {
  /* no global preparation necessary on Windows */
  return 0;
}

