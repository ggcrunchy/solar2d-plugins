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
#include "ipc.h"
#include "memfile.h"


/* check for POSIX */
#if defined( unix ) || defined( __unix ) || defined( __unix__ ) || \
    (defined( __APPLE__ ) && defined( __MACH__ )) || \
    HAVE_UNISTD_H
#  include <unistd.h>
#  if defined( _POSIX_VERSION ) && _POSIX_VERSION >= 200112L && \
      defined( _POSIX_MAPPED_FILES ) && _POSIX_MAPPED_FILES > 0
#    define HAVE_MMAP
#    include "mmap_posix.h"
#  endif
#endif


/* check for Windows */
#if !defined( HAVE_MMAP ) && \
    defined( _WIN32 ) && !defined( __CYGWIN__ )
#  define HAVE_MMAP
#  include "mmap_win.h"
#endif


#ifdef HAVE_MMAP

#define NAME "ipc.mmap"

typedef struct {
  ipc_mmap_handle h; /* platform specific data */
  /* extra management info: */
  char is_valid;
} l_mmap_handle;


static int pusherror( lua_State* L, int code ) {
  char buf[ IPC_MAXERRMSG ];
  ipc_mmap_error( buf, sizeof( buf ), code );
  lua_pushnil( L );
  lua_pushstring( L, buf );
  lua_pushinteger( L, code );
  return 3;
}


static int l_mmap_close( lua_State* L ) {
  l_mmap_handle* h = luaL_checkudata( L, 1, NAME );
  int rv = 0;
  if( !h->is_valid )
    luaL_error( L, "attempt to use invalid mmap object" );
  rv = ipc_mmap_close( &h->h );
  if( rv != 0 )
    return pusherror( L, rv );
  h->is_valid = 0;
  lua_pushboolean( L, 1 );
  return 1;
}


static int l_mmap_gc( lua_State* L ) {
  l_mmap_handle* h = lua_touserdata( L, 1 );
  if( h->is_valid )
    ipc_mmap_close( &h->h );
  return 0;
}


#ifdef IPC_MMAP_HAVE_FLUSH
static int l_mmap_flush( lua_State* L ) {
  l_mmap_handle* h = luaL_checkudata( L, 1, NAME );
  size_t pos = luaL_checkinteger( L, 2 );
  int rv = 0;
  if( !h->is_valid )
    luaL_error( L, "attempt to use invalid mmap object" );
  rv = ipc_mmap_flush( &h->h, pos );
  if( rv != 0 )
    return pusherror( L, rv );
  lua_pushboolean( L, 1 );
  return 1;
}
#endif


static int l_mmap_open( lua_State* L ) {
  static char const* const modenames[] = {
    "r", "w", "rw", NULL
  };
  static int const modes[] = {
    MEMFILE_R, MEMFILE_W, MEMFILE_RW
  };
  char const* name = luaL_checkstring( L, 1 );
  int mode = modes[ luaL_checkoption( L, 2, "r", modenames ) ];
  ipc_mmap_off_t offset = IPC_OPTBIGINT( ipc_mmap_off_t, L, 3, 0 );
  size_t size = luaL_optinteger( L, 4, 0 );
  l_mmap_handle* h = lua_newuserdata( L, sizeof( *h ) );
  int rv = 0;
  h->is_valid = 0;
  luaL_getmetatable( L, NAME );
  lua_setmetatable( L, -2 );
  rv = ipc_mmap_open( &h->h, name, mode, offset, size );
  if( rv != 0 )
    return pusherror( L, rv );
  h->is_valid = 1;
  lua_pushcfunction( L, l_mmap_close );
#ifdef IPC_MMAP_HAVE_FLUSH
  lua_pushcfunction( L, l_mmap_flush );
  memfile_new( L, ipc_mmap_addr( &h->h ), ipc_mmap_size( &h->h ),
               mode, -3, -2, -1 );
#else
  memfile_new( L, ipc_mmap_addr( &h->h ), ipc_mmap_size( &h->h ),
               mode, -2, -1, 0 );
#endif
  return 1;
}


IPC_API int luaopen_ipc_mmap( lua_State* L ) {
  luaL_Reg const functions[] = {
    { "open", l_mmap_open },
    { NULL, NULL }, /* reserve space */
    { NULL, NULL }
  };
  if( !luaL_newmetatable( L, NAME ) )
    luaL_error( L, "redefinition of metatable '%s'", NAME );
  lua_pushcfunction( L, l_mmap_gc );
  lua_setfield( L, -2, "__gc" );
  lua_pop( L, 1 );
  luaL_newlib( L, functions );
  lua_pushinteger( L, (lua_Integer)ipc_mmap_pagesize() );
  lua_setfield( L, -2, "pagesize" );
  return 1;
}

#else /* no implementation for this platform available: */
IPC_API int luaopen_ipc_mmap( lua_State* L ) {
  IPC_NOTIMPLEMENTED( L );
  return 0;
}
#endif

