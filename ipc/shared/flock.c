#ifndef _POSIX_C_SOURCE
#  define _POSIX_C_SOURCE 200112L
#endif
#ifndef _FILE_OFFSET_BITS
#  define _LARGEFILE_SOURCE 1
#  define _FILE_OFFSET_BITS 64
#endif
#include <stddef.h>
#include <stdio.h>
#include <lua.h>
#include <lauxlib.h>
#include "ipc.h"


/* check for POSIX */
#if defined( unix ) || defined( __unix ) || defined( __unix__ ) || \
    (defined( __APPLE__ ) && defined( __MACH__ )) || \
    HAVE_UNISTD_H
#  include <unistd.h>
#  if defined( _POSIX_VERSION ) && _POSIX_VERSION >= 200112L
#    define HAVE_FLOCK
#    include "flock_posix.h"
#  endif
#endif


/* check for Windows */
#if !defined( HAVE_FLOCK ) && \
    defined( _WIN32 ) && !defined( __CYGWIN__ )
#  define HAVE_FLOCK
#  include "flock_win.h"
#endif


#ifdef HAVE_FLOCK

static int pusherror( lua_State* L, int code ) {
  char buf[ IPC_MAXERRMSG ];
  ipc_flock_error( buf, sizeof( buf ), code );
  lua_pushnil( L );
  lua_pushstring( L, buf );
  lua_pushinteger( L, code );
  return 3;
}


static void invalidate_input_buffer( FILE* f ) {
  /* Linux (and apparently many other implementations) discard
   * unread characters from the input buffer if fflush is called on
   * an input file, but this is not guaranteed by ISO C. */
  fflush( f );
  /* This should also invalidate the input buffer unless the
   * implementation checks for that specific case. */
  fseek( f, 0, SEEK_CUR );
  /* If both methods don't work, we are out of luck. But using
   * low-level file locking with buffered IO is a bad idea
   * anyway! */
}


static int l_flock_lock( lua_State* L ) {
  FILE* f = ipc_checkfile( L, 1 );
  static char const* const mnames[] = { "r", "w", "rw", NULL };
  static int const modes[] = { 0, 1, 1 };
  int is_wlock = modes[ luaL_checkoption( L, 2, "rw", mnames ) ];
  ipc_flock_off_t start = IPC_OPTBIGINT( ipc_flock_off_t, L, 3, 0 );
  ipc_flock_off_t len = IPC_OPTBIGINT( ipc_flock_off_t, L, 4, 0 );
  int rv = ipc_flock_lock( f, is_wlock, NULL, start, len );
  if( rv != 0 )
    return pusherror( L, rv );
  /* try to flush input buffer */
  invalidate_input_buffer( f );
  lua_pushboolean( L, 1 );
  return 1;
}


static int l_flock_trylock( lua_State* L ) {
  FILE* f = ipc_checkfile( L, 1 );
  static char const* const mnames[] = { "r", "w", "rw", NULL };
  static int const modes[] = { 0, 1, 1 };
  int is_wlock = modes[ luaL_checkoption( L, 2, "rw", mnames ) ];
  ipc_flock_off_t start = IPC_OPTBIGINT( ipc_flock_off_t, L, 3, 0 );
  ipc_flock_off_t len = IPC_OPTBIGINT( ipc_flock_off_t, L, 4, 0 );
  int could_lock = 0;
  int rv = ipc_flock_lock( f, is_wlock, &could_lock, start, len );
  if( rv != 0 )
    return pusherror( L, rv );
  if( could_lock ) /* try to flush input buffer */
    invalidate_input_buffer( f );
  lua_pushboolean( L, could_lock );
  return 1;
}


static int l_flock_unlock( lua_State* L ) {
  FILE* f = ipc_checkfile( L, 1 );
  ipc_flock_off_t start = (ipc_flock_off_t)luaL_optinteger( L, 2, 0 );
  ipc_flock_off_t len = (ipc_flock_off_t)luaL_optinteger( L, 3, 0 );
  int rv = 0;
  fflush( f ); /* flush output buffer */
  rv = ipc_flock_unlock( f, start, len );
  if( rv != 0 )
    return pusherror( L, rv );
  lua_pushboolean( L, 1 );
  return 1;
}


IPC_API int luaopen_ipc_filelock( lua_State* L ) {
  luaL_Reg const functions[] = {
    { "lock", l_flock_lock },
    { "trylock", l_flock_trylock },
    { "unlock", l_flock_unlock },
    { NULL, NULL }
  };
  luaL_newlib( L, functions );
  return 1;
}

#else /* no implementation for this platform available: */
IPC_API int luaopen_ipc_filelock( lua_State* L ) {
  IPC_NOTIMPLEMENTED( L );
  return 0;
}
#endif

