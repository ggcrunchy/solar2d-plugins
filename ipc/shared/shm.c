#ifndef _POSIX_C_SOURCE
#  define _POSIX_C_SOURCE 200112L
#endif
#ifndef _FILE_OFFSET_BITS
#  define _LARGEFILE_SOURCE 1
#  define _FILE_OFFSET_BITS 64
#endif
#include <stddef.h>
#include <lua.h>
#include <lauxlib.h>
#include "memfile.h"


/* check for POSIX */
#if defined( unix ) || defined( __unix ) || defined( __unix__ ) || \
    (defined( __APPLE__ ) && defined( __MACH__ )) || \
    HAVE_UNISTD_H
#  include <unistd.h>
#  if defined( _POSIX_VERSION ) && _POSIX_VERSION >= 200112L && \
      ((defined( _POSIX_MAPPED_FILES ) && _POSIX_MAPPED_FILES > 0 && \
        defined( _POSIX_SHARED_MEMORY_OBJECTS ) && \
        _POSIX_SHARED_MEMORY_OBJECTS > 0) || \
       (defined( __APPLE__ ) && defined( __MACH__ )))
#    define HAVE_SHM
#    include "shm_posix.h"
#  endif
#endif


/* check for Windows */
#if !defined( HAVE_SHM ) && \
    defined( _WIN32 ) && !defined( __CYGWIN__ )
#  define HAVE_SHM
#  include "shm_win.h"
#endif


#ifdef HAVE_SHM

#define NAME "ipc.shm"

typedef struct {
  ipc_shm_handle h; /* platform specific data */
  /* extra management info: */
  char is_owner;
  char is_valid;
} l_shm_handle;


static int pusherror( lua_State* L, int code ) {
  char buf[ IPC_MAXERRMSG ];
  ipc_shm_error( buf, sizeof( buf ), code );
  lua_pushnil( L );
  lua_pushstring( L, buf );
  lua_pushinteger( L, code );
  return 3;
}


static int l_shm_close( lua_State* L ) {
  l_shm_handle* h = luaL_checkudata( L, 1, NAME );
  int rv = 0;
  if( !h->is_valid )
    luaL_error( L, "attempt to use invalid shm object" );
  if( h->is_owner )
    rv = ipc_shm_remove( &h->h );
  else
    rv = ipc_shm_detach( &h->h );
  if( rv != 0 )
    return pusherror( L, rv );
  h->is_valid = 0;
  lua_pushboolean( L, 1 );
  return 1;
}


static int l_shm_gc( lua_State* L ) {
  l_shm_handle* h = lua_touserdata( L, 1 );
  if( h->is_valid ) {
    if( h->is_owner )
      ipc_shm_remove( &h->h );
    else
      ipc_shm_detach( &h->h );
  }
  return 0;
}



static int l_shm_create( lua_State* L ) {
  char const* name = luaL_checkstring( L, 1 );
  size_t req = luaL_checkinteger( L, 2 );
  l_shm_handle* h = lua_newuserdata( L, sizeof( *h ) );
  int rv = 0;
  h->is_owner = 1;
  h->is_valid = 0;
  luaL_getmetatable( L, NAME );
  lua_setmetatable( L, -2 );
  rv = ipc_shm_create( &h->h, name, req );
  if( rv != 0 )
    return pusherror( L, rv );
  h->is_valid = 1;
  lua_pushcfunction( L, l_shm_close );
  memfile_new( L, ipc_shm_addr( &h->h ), ipc_shm_size( &h->h ),
               MEMFILE_RW, -2, -1, 0 );
  return 1;
}


static int l_shm_attach( lua_State* L ) {
  char const* name = luaL_checkstring( L, 1 );
  l_shm_handle* h = lua_newuserdata( L, sizeof( *h ) );
  int rv = 0;
  h->is_owner = 0;
  h->is_valid = 0;
  luaL_getmetatable( L, NAME );
  lua_setmetatable( L, -2 );
  rv = ipc_shm_attach( &h->h, name );
  if( rv != 0 )
    return pusherror( L, rv );
  h->is_valid = 1;
  lua_pushcfunction( L, l_shm_close );
  memfile_new( L, ipc_shm_addr( &h->h ), ipc_shm_size( &h->h ),
               MEMFILE_RW, -2, -1, 0 );
  return 1;
}


IPC_API int luaopen_ipc_shm( lua_State* L ) {
  luaL_Reg const functions[] = {
    { "create", l_shm_create },
    { "attach", l_shm_attach },
    { NULL, NULL }
  };
  if( !luaL_newmetatable( L, NAME ) )
    luaL_error( L, "redefinition of metatable '%s'", NAME );
  lua_pushcfunction( L, l_shm_gc );
  lua_setfield( L, -2, "__gc" );
  lua_pop( L, 1 );
  luaL_newlib( L, functions );
  return 1;
}

#else /* no implementation for this platform available: */
IPC_API int luaopen_ipc_shm( lua_State* L ) {
  IPC_NOTIMPLEMENTED( L );
  return 0;
}
#endif

