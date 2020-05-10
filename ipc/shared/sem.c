#ifndef _POSIX_C_SOURCE
#  define _POSIX_C_SOURCE 200112L
#endif
#include <stddef.h>
#include <lua.h>
#include <lauxlib.h>
#include "ipc.h"


/* check for POSIX */
#if defined( unix ) || defined( __unix ) || defined( __unix__ ) || \
    (defined( __APPLE__ ) && defined( __MACH__ )) || \
    HAVE_UNISTD_H
#  include <unistd.h>
#  if defined( _POSIX_VERSION ) && _POSIX_VERSION >= 200112L && \
   defined( _POSIX_SEMAPHORES ) && _POSIX_SEMAPHORES > 0 && \
   defined( _POSIX_TIMEOUTS ) && _POSIX_TIMEOUTS > 0
#    define HAVE_SEM
#    include "sem_posix.h"
#  endif
#endif


/* OSX needs its own code, because it doesn't implement
 * `sem_timedwait()`! When it does, we'll gladly use the generic
 * POSIX code above. Until then ... */
#if !defined( HAVE_SEM ) && \
    defined( __APPLE__ ) && defined( __MACH__ )
#  define HAVE_SEM
#  include "sem_osx.h"
#endif


/* check for Windows */
#if !defined( HAVE_SEM ) && \
    defined( _WIN32 ) && !defined( __CYGWIN__ )
#  define HAVE_SEM
#  include "sem_win.h"
#endif


#ifdef HAVE_SEM

#define NAME "ipc.sem"

typedef struct {
  ipc_sem_handle h; /* platform specific data */
  /* extra management info: */
  char is_owner;
  char is_valid;
} l_sem_handle;


static int pusherror( lua_State* L, int code ) {
  char buf[ IPC_MAXERRMSG ];
  ipc_sem_error( buf, sizeof( buf ), code );
  lua_pushnil( L );
  lua_pushstring( L, buf );
  lua_pushinteger( L, code );
  return 3;
}


static int l_sem_close( lua_State* L ) {
  l_sem_handle* h = luaL_checkudata( L, 1, NAME );
  int rv = 0;
  if( !h->is_valid )
    luaL_error( L, "attempt to use invalid semaphore object" );
  if( h->is_owner )
    rv = ipc_sem_remove( &h->h );
  else
    rv = ipc_sem_close( &h->h );
  if( rv != 0 )
    return pusherror( L, rv );
  h->is_valid = 0;
  lua_pushboolean( L, 1 );
  return 1;
}


static int l_sem_gc( lua_State* L ) {
  l_sem_handle* h = lua_touserdata( L, 1 );
  if( h->is_valid ) {
    if( h->is_owner )
      ipc_sem_remove( &h->h );
    else
      ipc_sem_close( &h->h );
  }
  return 0;
}


static int l_sem_open( lua_State* L ) {
  char const* name = luaL_checkstring( L, 1 );
  unsigned value = (unsigned)luaL_optinteger( L, 2, 0 );
  l_sem_handle* h = lua_newuserdata( L, sizeof( *h ) );
  int rv = 0;
  h->is_owner = 0;
  h->is_valid = 0;
  luaL_getmetatable( L, NAME );
  lua_setmetatable( L, -2 );
  if( value != 0 )
    rv = ipc_sem_create( &h->h, name, value );
  else
    rv = ipc_sem_open( &h->h, name );
  if( rv != 0 )
    return pusherror( L, rv );
  if( value != 0 )
    h->is_owner = 1;
  h->is_valid = 1;
  return 1;
}


static int l_sem_dec( lua_State* L ) {
  l_sem_handle* h = luaL_checkudata( L, 1, NAME );
  lua_Integer msec = (lua_Integer)(luaL_optnumber( L, 2, -1 )*1000);
  int could_rec = 1;
  int rv = ipc_sem_dec( &h->h, msec >= 0 ? &could_rec : NULL,
                        (unsigned)msec );
  if( rv != 0 )
    return pusherror( L, rv );
  lua_pushboolean( L, could_rec );
  return 1;
}


static int l_sem_inc( lua_State* L ) {
  l_sem_handle* h = luaL_checkudata( L, 1, NAME );
  int rv = ipc_sem_inc( &h->h );
  if( rv != 0 )
    return pusherror( L, rv );
  lua_pushboolean( L, 1 );
  return 1;
}


IPC_API int luaopen_ipc_sem( lua_State* L ) {
  luaL_Reg const methods[] = {
    { "inc", l_sem_inc },
    { "dec", l_sem_dec },
    { "close", l_sem_close },
    { NULL, NULL }
  };
  luaL_Reg const functions[] = {
    { "open", l_sem_open },
    { NULL, NULL }
  };
  if( !luaL_newmetatable( L, NAME ) )
    luaL_error( L, "redefinition of metatable '%s'", NAME );
  lua_pushcfunction( L, l_sem_gc );
  lua_setfield( L, -2, "__gc" );
  lua_pushboolean( L, 0 );
  lua_setfield( L, -2, "__metatable" );
  luaL_newlib( L, methods );
  lua_setfield( L, -2, "__index" );
  lua_pop( L, 1 );
  luaL_newlib( L, functions );
  return 1;
}

#else /* no implementation for this platform available: */
IPC_API int luaopen_ipc_sem( lua_State* L ) {
  IPC_NOTIMPLEMENTED( L );
  return 0;
}
#endif

