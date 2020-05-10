#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <io.h>


typedef ULONGLONG ipc_flock_off_t;


static void ipc_flock_error( char* buf, size_t len, int code ) {
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


static int ipc_flock_lock( FILE* f, int is_wlock, int* could_lock,
                           ipc_flock_off_t start,
                           ipc_flock_off_t len ) {
  HANDLE fh = (HANDLE)_get_osfhandle( _fileno( f ) );
  DWORD flags = is_wlock ? LOCKFILE_EXCLUSIVE_LOCK : 0;
  DWORD lenlo = (DWORD)len, lenhi = (DWORD)(len >> 32);
  OVERLAPPED ov;
  if( fh == (HANDLE)INVALID_HANDLE_VALUE )
    return IPC_ERR( ERROR_INVALID_HANDLE );
  if( could_lock != NULL )
    flags |= LOCKFILE_FAIL_IMMEDIATELY;
  ov.Offset = (DWORD)start;
  ov.OffsetHigh = (DWORD)(start >> 32);
  ov.hEvent = NULL;
  if( len == 0 )
    lenhi = lenlo = (DWORD)-1;
  if( !LockFileEx( fh, flags, 0, lenlo, lenhi, &ov ) ) {
    int code = GetLastError();
    if( could_lock != NULL &&
        (code == ERROR_LOCK_VIOLATION || code == ERROR_IO_PENDING) ) {
      *could_lock = 0;
      return 0;
    }
    return IPC_ERR( code );
  }
  if( could_lock != NULL )
    *could_lock = 1;
  return 0;
}


static int ipc_flock_unlock( FILE* f, ipc_flock_off_t start,
                             ipc_flock_off_t len ) {
  HANDLE fh = (HANDLE)_get_osfhandle( _fileno( f ) );
  DWORD lenlo = (DWORD)len, lenhi = (DWORD)(len >> 32);
  DWORD offlo = (DWORD)start, offhi = (DWORD)(start >> 32);
  if( fh == (HANDLE)INVALID_HANDLE_VALUE )
    return IPC_ERR( ERROR_INVALID_HANDLE );
  if( len == 0 )
    lenhi = lenlo = (DWORD)-1;
  if( !UnlockFile( fh, offlo, offhi, lenlo, lenhi ) )
    return IPC_ERR( GetLastError() );
  return 0;
}

