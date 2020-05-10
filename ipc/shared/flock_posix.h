#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>


typedef off_t ipc_flock_off_t;


static void ipc_flock_error( char* buf, size_t len, int code ) {
  if( len > 0 && strerror_r( code, buf, len ) != (int)0 ) {
    strncpy( buf, "unknown error", len-1 );
    buf[ len-1 ] = '\0';
  }
}


static int ipc_flock_lock( FILE* f, int is_wlock, int* could_lock,
                           ipc_flock_off_t start,
                           ipc_flock_off_t len ) {
  int rv = 0;
  int fd = fileno( f );
  int op = could_lock != NULL ? F_SETLK : F_SETLKW;
  struct flock fl;
  fl.l_type = is_wlock ? F_WRLCK : F_RDLCK;
  fl.l_whence = SEEK_SET;
  fl.l_start = start;
  fl.l_len = len;
  IPC_EINTR( rv, fcntl( fd, op, &fl ) );
  if( rv < 0 ) {
    if( could_lock != NULL &&
        (errno == EACCES || errno == EAGAIN) ) {
      *could_lock = 0;
      return 0;
    }
    return IPC_ERR( errno );
  }
  if( could_lock != NULL )
    *could_lock = 1;
  return 0;
}


static int ipc_flock_unlock( FILE* f, ipc_flock_off_t start,
                             ipc_flock_off_t len ) {
  struct flock fl;
  fl.l_type = F_UNLCK;
  fl.l_whence = SEEK_SET;
  fl.l_start = start;
  fl.l_len = len;
  if( fcntl( fileno( f ), F_SETLK, &fl ) < 0 )
    return IPC_ERR( errno );
  return 0;
}

