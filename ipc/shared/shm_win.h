#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#define WIN32_LEAN_AND_MEAN
#include <windows.h>


typedef struct {
  HANDLE h; /* creator handle */
  void* addr;
  size_t len;
} ipc_shm_handle;

static void* ipc_shm_addr( ipc_shm_handle* h ) {
  return h->addr;
}

static size_t ipc_shm_size( ipc_shm_handle* h ) {
  return h->len;
}


static void ipc_shm_error( char* buf, size_t len, int code ) {
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


static int ipc_shm_make_name_( char const* s, char** name ) {
  size_t len = strlen( s );
  if( len == 0 || strcspn( s, "/\\" ) != len )
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


static int ipc_shm_create( ipc_shm_handle* h, char const* name,
                           size_t req ) {
  char* rname = NULL;
  int rv = ipc_shm_make_name_( name, &rname );
  if( rv != 0 )
    return IPC_ERR( rv );
  if( req == 0 ) {
    free( rname );
    return IPC_ERR( ERROR_INVALID_PARAMETER );
  }
  h->h = CreateFileMappingA( INVALID_HANDLE_VALUE,
                             NULL,
                             PAGE_READWRITE,
                             (DWORD)((req >> 16) >> 16),
                             (DWORD)req,
                             rname );
  if( h->h == NULL ) {
    int saved_errno = GetLastError();
    free( rname );
    return IPC_ERR( saved_errno );
  } else if( GetLastError() == ERROR_ALREADY_EXISTS ) {
    /* It seems we have no way of actually removing the shared memory
     * object -- should we fail with EEXIST anyway?! */
    CloseHandle( h->h );
    free( rname );
    return IPC_ERR( ERROR_ALREADY_EXISTS );
  }
  /* Windows automatically handles the lifetime of the shared memory
   * object, so we don't need to keep track of the name! */
  free( rname );
  h->addr = MapViewOfFile( h->h,
                           FILE_MAP_ALL_ACCESS,
                           0,
                           0,
                           0 );
  if( h->addr == NULL ) {
    int saved_errno = GetLastError();
    CloseHandle( h->h );
    return IPC_ERR( saved_errno );
  }
  h->len = req;
  return 0;
}


static int ipc_shm_attach( ipc_shm_handle* h, char const* name ) {
  HANDLE hmap;
  MEMORY_BASIC_INFORMATION meminfo;
  char* rname = NULL;
  int rv = ipc_shm_make_name_( name, &rname );
  if( rv != 0 )
    return IPC_ERR( rv );
  hmap = OpenFileMappingA( FILE_MAP_ALL_ACCESS,
                           FALSE,
                           rname );
  if( hmap == NULL ) {
    int saved_errno = GetLastError();
    free( rname );
    return IPC_ERR( saved_errno );
  }
  free( rname );
  h->addr = MapViewOfFile( hmap,
                           FILE_MAP_ALL_ACCESS,
                           0,
                           0,
                           0 );
  if( h->addr == NULL ) {
    int saved_errno = GetLastError();
    CloseHandle( hmap );
    return IPC_ERR( saved_errno );
  }
  CloseHandle( hmap );
  if( VirtualQuery( h->addr, &meminfo, sizeof( meminfo ) ) == 0 ) {
    int saved_errno = GetLastError();
    UnmapViewOfFile( h->addr );
    return IPC_ERR( saved_errno );
  }
  h->len = meminfo.RegionSize;
  return 0;
}


static int ipc_shm_detach( ipc_shm_handle* h ) {
  if( !UnmapViewOfFile( h->addr ) )
    return IPC_ERR( GetLastError() );
  return 0;
}


static int ipc_shm_remove( ipc_shm_handle* h ) {
  if( !UnmapViewOfFile( h->addr ) ) {
    int saved_errno = GetLastError();
    CloseHandle( h->h );
    return IPC_ERR( saved_errno );
  }
  CloseHandle( h->h );
  return 0;
}

