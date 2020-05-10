#ifndef _POSIX_C_SOURCE
#  define _POSIX_C_SOURCE 200112L
#endif
#include <stddef.h>
#include <stdlib.h>
#include <lua.h>
#include <lauxlib.h>
#include "ipc.h"


/* check for POSIX */
#if defined( unix ) || defined( __unix ) || defined( __unix__ ) || \
    (defined( __APPLE__ ) && defined( __MACH__ )) || \
    HAVE_UNISTD_H
#  include <unistd.h>
#  if defined( _POSIX_VERSION ) && _POSIX_VERSION >= 200112L
#    define HAVE_PROC
#    include "proc_posix.h"
#  endif
#endif


/* check for Windows */
#if !defined( HAVE_PROC ) && \
    defined( _WIN32 ) && !defined( __CYGWIN__ )
#  define HAVE_PROC
#  include "proc_win.h"
#endif


#ifdef HAVE_PROC

#define NAME "ipc.proc"


/* needed for its address: */
static char EOF_id = 0;


typedef struct {
  ipc_proc_handle h; /* platform specific data */
  /* extra management info: */
  char is_valid;
  char may_write;
} l_proc_handle;


static int pusherror( lua_State* L, int code ) {
  char buf[ IPC_MAXERRMSG ];
  ipc_proc_error( buf, sizeof( buf ), code );
  lua_pushnil( L );
  lua_pushstring( L, buf );
  lua_pushinteger( L, code );
  return 3;
}


static int l_proc_gc( lua_State* L ) {
  l_proc_handle* h = lua_touserdata( L, 1 );
  if( h->is_valid ) {
    int status = 0;
    char const* what = NULL;
    ipc_proc_kill( &h->h, IPC_PROC_SIGKILL );
    ipc_proc_wait( &h->h, &status, &what );
  }
  return 0;
}


static int l_proc_kill( lua_State* L ) {
  l_proc_handle* h = luaL_checkudata( L, 1, NAME );
  char const* const signames[] = {
    "term", "kill", NULL
  };
  int const sigs[] = {
    IPC_PROC_SIGTERM, IPC_PROC_SIGKILL
  };
  int sig = sigs[ luaL_checkoption( L, 2, NULL, signames ) ];
  int rv = 0;
  if( !h->is_valid )
    luaL_error( L, "attempt to use invalid process object" );
  rv = ipc_proc_kill( &h->h, sig );
  if( rv != 0 )
    return pusherror( L, rv );
  lua_pushboolean( L, 1 );
  return 1;
}


static void compactenv( lua_State* L, int idx ) {
  int n = 0;
  lua_getuservalue( L, idx );
  if( lua_type( L, -1 ) != LUA_TTABLE )
    luaL_error( L, "does not have uservalue/environment table" );
  n = (int)lua_rawlen( L, -1 );
  if( n > 1 ) {
    int i = 1;
    luaL_checkstack( L, n, "not enough stack space" );
    for( i = 1; i <= n; ++i )
      lua_rawgeti( L, -i, i );
    lua_concat( L, n );
    lua_rawseti( L, -2, 1 );
    for( i = 2; i <= n; ++i ) {
      lua_pushnil( L );
      lua_rawseti( L, -2, i );
    }
  }
  lua_pop( L, 1 );
}


static int addtoenv( lua_State* L, int idx1, int idx2 ) {
  int n = 0;
  idx2 = lua_absindex( L, idx2 );
  lua_getuservalue( L, idx1 );
  if( lua_type( L, -1 ) != LUA_TTABLE )
    luaL_error( L, "does not have uservalue/environment table" );
  n = (int)lua_rawlen( L, -1 )+1;
  lua_pushvalue( L, idx2 );
  lua_rawseti( L, -2, n );
  lua_pop( L, 1 );
  return n;
}


static int startoutput( lua_State* L, l_proc_handle* h, int idx ) {
  char const* str = NULL;
  size_t len = 0;
  int completed = 0;
  int rv = 0;
  /* concatenate all pending strings in the uservalue sequence */
  compactenv( L, idx );
  lua_getuservalue( L, idx ); /* env */
  lua_rawgeti( L, -1, 1 ); /* env, str */
  if( lua_type( L, -1 ) == LUA_TSTRING ) {
    /* don't let the string be gc'd */
    lua_pushvalue( L, -1 ); /* env, str, str */
    lua_setfield( L, -3, "stdinbuf" ); /* env, str */
    str = lua_tolstring( L, -1, &len );
    /* enqueue the output request to the child's stdin */
    rv = ipc_proc_write( &h->h, str, len, &completed );
    if( rv != 0 )
      return rv;
    /* clear uservalue sequence */
    lua_pushnil( L ); /* env, str, nil */
    lua_rawseti( L, -3, 1 ); /* env, str */
    if( completed ) { /* output completed already?! */
      /* output string can be gc'd now! */
      lua_pushnil( L ); /* env, str, nil */
      lua_setfield( L, -3, "stdinbuf" ); /* env, str */
    }
  }
  /* check whether we can/should close the pipe now */
  lua_getfield( L, -2, "stdinbuf" ); /* env, str, stdinbuf */
  lua_getfield( L, -3, "iseof" ); /* env, str, stdinbuf, iseof */
  if( lua_type( L, -2 ) == LUA_TNIL && lua_type( L, -1 ) != LUA_TNIL )
    rv = ipc_proc_closestdin( &h->h );
  lua_pop( L, 4 );
  return rv;
}


LUA_KFUNCTION( l_proc_waitk ) {
  l_proc_handle* h = lua_touserdata( L, 1 );
  int child_complete = 0;
  int stdin_complete = 0;
  int stdout_complete = 0;
  int stderr_complete = 0;
  int rv = 0;
  char const* data = NULL;
  size_t len = 0;
  (void)status;
  lua_settop( L, 5 ); /* handle, child_complete, ..., stderr_complete */
  /* lua_settop */
  switch( ctx ) {
    case 0:
      do {
        /* call blocking ipc function to check for readable input or
         * writable output (also checks for dead child) */
        rv = ipc_proc_waitio( &h->h, &child_complete, &stdin_complete,
                              &stdout_complete, &stderr_complete );
        if( rv != 0 )
          return pusherror( L, rv );
        /* this function may yield, so we have to save all temporary
         * data in the Lua state. */
        lua_pushboolean( L, child_complete );
        lua_replace( L, 2 );
        lua_pushboolean( L, stdin_complete );
        lua_replace( L, 3 );
        lua_pushboolean( L, stdout_complete );
        lua_replace( L, 4 );
        lua_pushboolean( L, stderr_complete );
        lua_replace( L, 5 );
        /* call handler function from uservalue */
        if( lua_toboolean( L, 4 ) ) { /* stdout_complete */
          ipc_getuservaluefield( L, 1, "callback" );
          lua_pushliteral( L, "stdout" );
          rv = ipc_proc_stdoutready( &h->h, &data, &len );
          if( rv != 0 ) {
            lua_callk( L, pusherror( L, rv )+1, 0, 1, l_proc_waitk );
          } else if( len > 0 ) {
            lua_pushlstring( L, data, len );
            lua_callk( L, 2, 0, 1, l_proc_waitk );
          } else
            lua_pop( L, 2 );
        }
    case 1:
        if( lua_toboolean( L, 5 ) ) { /* stderr_complete */
          ipc_getuservaluefield( L, 1, "callback" );
          lua_pushliteral( L, "stderr" );
          rv = ipc_proc_stderrready( &h->h, &data, &len );
          if( rv != 0 ) {
            lua_callk( L, pusherror( L, rv )+1, 0, 2, l_proc_waitk );
          } else if( len > 0 ) {
            lua_pushlstring( L, data, len );
            lua_callk( L, 2, 0, 2, l_proc_waitk );
          } else
            lua_pop( L, 2 );
        }
    case 2:
        if( lua_toboolean( L, 2 ) ) { /* child_complete */
          int stat = 0;
          char const* what = NULL;
          rv = ipc_proc_wait( &h->h, &stat, &what );
          h->is_valid = 0;
          if( rv != 0 )
            return pusherror( L, rv );
          else {
            if( stat == 0 && *what == 'e' )
              lua_pushboolean( L, 1 );
            else
              lua_pushnil( L );
            lua_pushstring( L, what );
            lua_pushinteger( L, stat );
            return 3;
          }
        }
        if( lua_toboolean( L, 3 ) ) { /* stdin_complete */
          rv = startoutput( L, h, 1 );
          if( rv != 0 )
            return pusherror( L, rv );
        }
      } while( 1 );
  }
  /* should never happen: */
  return luaL_error( L, "invalid ctx in process wait function" );
}

static int l_proc_wait( lua_State* L ) {
  /* we do parameter checking here, so that we can avoid it in the
   * continuation function above. */
  l_proc_handle* h = luaL_checkudata( L, 1, NAME );
  if( !h->is_valid )
    luaL_error( L, "attempt to use invalid process object" );
  return l_proc_waitk( L, 0, 0 );
}


static int l_proc_write( lua_State* L ) {
  l_proc_handle* h = luaL_checkudata( L, 1, NAME );
  int rv = 0, ready = 0;
  int got_eof = 0;
  int i = 0;
  int top = lua_gettop( L );
  if( !h->is_valid )
    luaL_error( L, "attempt to use invalid process object" );
  if( !h->may_write )
    luaL_error( L, "process stdin is not connected to a pipe" );
  for( i = 2; i <= top; ++i ) {
    if( lua_islightuserdata( L, i ) &&
        lua_touserdata( L, i ) == &EOF_id )
      got_eof = i;
    else
      luaL_checkstring( L, i );
    if( ipc_getuservaluefield( L, 1, "iseof" ) != LUA_TNIL )
      luaL_error( L, "pipe to stdin is already closed" );
    lua_pop( L, 1 );
    if( got_eof ) {
      lua_pushvalue( L, got_eof );
      ipc_setuservaluefield( L, 1, "iseof" );
    } else {
      /* append input to table in uservalue */
      addtoenv( L, 1, i );
    }
  }
  /* check whether we are waiting already */
  rv = ipc_proc_stdinready( &h->h, &ready );
  if( rv != 0 )
    return pusherror( L, rv );
  /* if not, start asynchronous output to child process */
  if( ready ) {
    rv = startoutput( L, h, 1 );
    if( rv != 0 )
      return pusherror( L, rv );
  }
  lua_pushboolean( L, 1 );
  return 1;
}


static int l_proc_spawn( lua_State* L ) {
  FILE* cstdin = NULL;
  FILE* cstdout = NULL;
  FILE* cstderr = NULL;
  int pipe_stdin = 0, pipe_stdout = 0, pipe_stderr = 0;
  int rv = 0;
  l_proc_handle* h = NULL;
  /* check and extract arguments */
  char const* cmdline = luaL_checkstring( L, 1 );
  luaL_checktype( L, 2, LUA_TTABLE );
  lua_settop( L, 2 );
  lua_getfield( L, 2, "stdin" );
  cstdin = ipc_testfile( L, -1 );
  if( cstdin == NULL )
    pipe_stdin = lua_toboolean( L, -1 );
  lua_pop( L, 1 );
  lua_getfield( L, 2, "stdout" );
  cstdout = ipc_testfile( L, -1 );
  if( cstdout == NULL )
    pipe_stdout = lua_toboolean( L, -1 );
  lua_pop( L, 1 );
  lua_getfield( L, 2, "stderr" );
  cstderr = ipc_testfile( L, -1 );
  if( cstderr == NULL )
    pipe_stderr = lua_toboolean( L, -1 );
  lua_pop( L, 1 );
  lua_getfield( L, 2, "callback" );
  if( lua_type( L, -1 ) != LUA_TFUNCTION &&
      (pipe_stdout || pipe_stderr) )
    luaL_error( L, "'callback' field required" );
  /* construct handle */
  h = lua_newuserdata( L, sizeof( *h ) );
  h->is_valid = 0;
  h->may_write = pipe_stdin;
  lua_newtable( L ); /* uservalue table */
  lua_pushvalue( L, 3 );
  lua_setfield( L, -2, "callback" );
  lua_setuservalue( L, -2 );
  luaL_getmetatable( L, NAME );
  lua_setmetatable( L, -2 );
  /* setup pipes and create child process */
  rv = ipc_proc_spawn( &h->h, cmdline, cstdin, pipe_stdin,
                       cstdout, pipe_stdout, cstderr, pipe_stderr );
  if( rv != 0 )
    return pusherror( L, rv );
  h->is_valid = 1;
  return 1;
}


IPC_API int luaopen_ipc_proc( lua_State* L ) {
  luaL_Reg const methods[] = {
    { "kill", l_proc_kill },
    { "wait", l_proc_wait },
    { "write", l_proc_write },
    { NULL, NULL }
  };
  luaL_Reg const functions[] = {
    { "spawn", l_proc_spawn },
    { NULL, NULL }, /* reserve space for proc.EOF */
    { NULL, NULL }
  };
  int rv = ipc_proc_prepare();
  if( rv != 0 ) {
    char buf[ IPC_MAXERRMSG ];
    ipc_proc_error( buf, sizeof( buf ), rv );
    luaL_error( L, "ipc.proc initialization failed: %s", buf );
  }
  if( !luaL_newmetatable( L, NAME ) )
    luaL_error( L, "redefinition of metatable '%s'", NAME );
  lua_pushcfunction( L, l_proc_gc );
  lua_setfield( L, -2, "__gc" );
  lua_pushboolean( L, 0 );
  lua_setfield( L, -2, "__metatable" );
  luaL_newlib( L, methods );
  lua_setfield( L, -2, "__index" );
  lua_pop( L, 1 );
  luaL_newlib( L, functions );
  lua_pushlightuserdata( L, (void*)&EOF_id );
  lua_setfield( L, -2, "EOF" );
  return 1;
}

#else /* no implementation for this platform available: */
IPC_API int luaopen_ipc_proc( lua_State* L ) {
  IPC_NOTIMPLEMENTED( L );
  return 0;
}
#endif

