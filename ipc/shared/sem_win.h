#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <limits.h>
#define WIN32_LEAN_AND_MEAN
#include <windows.h>


typedef struct {
  HANDLE sem;
} ipc_sem_handle;


static void ipc_sem_error( char* buf, size_t len, int code ) {
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


static int ipc_sem_make_name_( char const* s, char** name ) {
  size_t len = strlen( s );
  if( len == 0 || len > MAX_PATH || strcspn( s, "/\\" ) != len )
    return IPC_ERR( ERROR_INVALID_NAME );
  /* FIXME: should we try a "Global\\" name first? */
#define P "Local\\"
  *name = malloc( len + sizeof( P ) );
  if( *name == NULL )
    return IPC_ERR( ERROR_OUTOFMEMORY ); /* or ERROR_NOT_ENOUGH_MEMORY?! */
  memcpy( *name, P, sizeof( P )-1 );
  memcpy( *name+sizeof( P )-1, s, len+1 );
#undef P
  return 0;
}


static int ipc_sem_create( ipc_sem_handle* h, char const* name,
                           unsigned value ) {
  char* rname = NULL;
  int rv = ipc_sem_make_name_( name, &rname );
  if( rv != 0 )
    return IPC_ERR( rv );
  if( value > LONG_MAX ) {
    free( rname );
    return IPC_ERR( ERROR_INVALID_PARAMETER );
  }
  h->sem = CreateSemaphoreA( NULL,
                             (LONG)value,
                             LONG_MAX,
                             rname );
  if( h->sem == NULL ) {
    int saved_errno = GetLastError();
    free( rname );
    return IPC_ERR( saved_errno );
  } else if( GetLastError() == ERROR_ALREADY_EXISTS ) {
    CloseHandle( h->sem );
    free( rname );
    return IPC_ERR( ERROR_ALREADY_EXISTS );
  }
  /* Windows automatically handles the lifetime of the semaphore
   * object, so we don't have to keep track of the name! */
  free( rname );
  return 0;
}


static int ipc_sem_open( ipc_sem_handle* h, char const* name ) {
  char* rname = NULL;
  int rv = ipc_sem_make_name_( name, &rname );
  if( rv != 0 )
    return IPC_ERR( rv );
  h->sem = OpenSemaphoreA( SEMAPHORE_ALL_ACCESS,
                           FALSE,
                           rname );
  if( h->sem == NULL ) {
    int saved_errno = GetLastError();
    free( rname );
    return IPC_ERR( saved_errno );
  }
  /* Windows automatically handles the lifetime of the semaphore
   * object, so we don't have to keep track of the name! */
  free( rname );
  return 0;
}


static int ipc_sem_inc( ipc_sem_handle* h ) {
  if( !ReleaseSemaphore( h->sem, 1, NULL ) )
    return IPC_ERR( GetLastError() );
  return 0;
}


static int ipc_sem_dec( ipc_sem_handle* h, int* could_dec, unsigned milliseconds ) {
  if( could_dec == NULL )
    milliseconds = INFINITE;
  switch( WaitForSingleObject( h->sem, milliseconds ) ) {
    case WAIT_OBJECT_0:
      if( could_dec != NULL )
        *could_dec = 1;
      break;
    case WAIT_TIMEOUT:
      if( could_dec != NULL )
        *could_dec = 0;
      else
        return IPC_ERR( ERROR_SEM_TIMEOUT );
      break;
    default:
      return IPC_ERR( GetLastError() );
  }
  return 0;
}


static int ipc_sem_close( ipc_sem_handle* h ) {
  if( !CloseHandle( h->sem ) )
    return IPC_ERR( GetLastError() );
  return 0;
}


static int ipc_sem_remove( ipc_sem_handle* h ) {
  /* Windows automatically removes a semaphore object when all
   * processes referencing it are destroyed (or when all handles are
   * closed?). */
  return IPC_ERR( ipc_sem_close( h ) );
}

