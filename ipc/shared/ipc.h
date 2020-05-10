#ifndef IPC_H_
#define IPC_H_

#include <stddef.h>
#include <stdio.h>
#include <lua.h>
#include <lauxlib.h>
#include "CoronaLua.h"


#ifndef IPC_LOCAL
/* emulate CLANG feature checking on other compilers */
#  ifndef __has_attribute
#    define __has_attribute( _x ) 0
#  endif
#  if !defined( _WIN32 ) && !defined( __CYGWIN__ ) && \
      ((defined( __GNUC__ ) && __GNUC__ >= 4 ) || \
       __has_attribute( __visibility__ ))
#    define IPC_LOCAL __attribute__((__visibility__("hidden")))
#  else
#    define IPC_LOCAL
#  endif
#endif

#define IPC_API

#ifndef IPC_API
#  ifdef _WIN32
#    define IPC_API __declspec(dllexport)
#  else
#    define IPC_API extern
#  endif
#endif


/* maximum expected length of error messages */
#define IPC_MAXERRMSG 200


#ifndef NDEBUG
#  if (defined( __STDC_VERSION__ ) && __STDC_VERSION__  >= 199901L) || \
      defined( __GNUC__ ) || defined( __clang__ )
#    define IPC_ERR( code ) (ipc_err( __FILE__, __LINE__, __func__, (int)(code) ))
#  elif defined( _MSC_VER ) && _MSC_VER >= 1100L
#    define IPC_ERR( code ) (ipc_err( __FILE__, __LINE__, __FUNCTION__, (int)(code) ))
#  else
#    define IPC_ERR( code ) (ipc_err( __FILE__, __LINE__, NULL, (int)(code) ))
#  endif
#else
#  define IPC_ERR( code ) ((int)(code))
#endif


#define IPC_NOTIMPLEMENTED( L ) \
  luaL_error( L, "module '%s' not implemented on this platform", \
              lua_tostring( L, 1 ) )


#define IPC_EINTR( _rv, _call ) \
  do { \
    _rv = _call; \
  } while( _rv < 0 && errno == EINTR )


#define IPC_OPTBIGINT( _t, _l, _i, _d ) \
  ((sizeof( _t ) > sizeof( lua_Integer ) && \
    sizeof( lua_Number ) > sizeof( lua_Integer )) \
    ? (_t)luaL_optnumber( _l, _i, _d ) \
    : (_t)luaL_optinteger( _l, _i, _d ))


IPC_LOCAL FILE* ipc_checkfile( lua_State* L, int idx );
IPC_LOCAL FILE* ipc_testfile( lua_State* L, int idx );
IPC_LOCAL int ipc_getuservaluefield( lua_State* L, int idx,
                                     char const* name );
IPC_LOCAL void ipc_setuservaluefield( lua_State* L, int idx,
                                      char const* name );
IPC_LOCAL int ipc_err( char const* file, int line, char const* func,
                       int code );


/* compatibility functions for older Lua versions */
#if LUA_VERSION_NUM == 501

typedef int lua_KContext;

IPC_LOCAL int ipc_absindex( lua_State* L, int idx );
#define lua_absindex( L, i ) ipc_absindex( L, i )

IPC_LOCAL void* ipc_testudata( lua_State* L, int idx,
                               char const* name );
#define luaL_testudata( L, i, n ) ipc_testudata( L, i, n )

#define lua_rawlen( L, i ) lua_objlen( L, i )

#define lua_setuservalue( L, i ) lua_setfenv( L, i )
#define lua_getuservalue( L, i ) lua_getfenv( L, i )

#define luaL_newlib( L, r ) \
  (lua_newtable( L ), luaL_register( L, NULL, r ))

#define lua_callk( L, na, nr, ctx, cont ) \
  ((void)ctx, (void)cont, lua_call( L, na, nr ))

#define lua_pcallk( L, na, nr, err, ctx, cont ) \
  ((void)ctx, (void)cont, lua_pcall( L, na, nr, err ))

#elif LUA_VERSION_NUM == 502

typedef int lua_KContext;

#define LUA_KFUNCTION( _name ) \
  static int (_name)( lua_State* L, int status, lua_KContext ctx ); \
  static int (_name ## _52)( lua_State* L ) { \
    lua_KContext ctx; \
    int status = lua_getctx( L, &ctx ); \
    return (_name)( L, status, ctx ); \
  } \
  static int (_name)( lua_State* L, int status, lua_KContext ctx )

#define lua_callk( L, na, nr, ctx, cont ) \
  lua_callk( L, na, nr, ctx, cont ## _52 )

#define lua_pcallk( L, na, nr, err, ctx, cont ) \
  lua_pcallk( L, na, nr, err, ctx, cont ## _52 )

#ifdef lua_call
#  undef lua_call
#  define lua_call( L, na, nr ) \
  (lua_callk)( L, na, nr, 0, NULL )
#endif

#ifdef lua_pcall
#  undef lua_pcall
#  define lua_pcall( L, na, nr, err ) \
  (lua_pcallk( L, na, nr, err, 0, NULL )
#endif

#endif /* LUA_VERSION_NUM */


#ifndef LUA_KFUNCTION

/* definition for everything except Lua 5.2 */
#define LUA_KFUNCTION( _name ) \
  static int (_name)( lua_State* L, int status, lua_KContext ctx )

#endif

IPC_API int luaopen_ipc_filelock (lua_State * L);
IPC_API int luaopen_ipc_mmap (lua_State * L);
IPC_API int luaopen_ipc_proc (lua_State * L);
IPC_API int luaopen_ipc_sem (lua_State * L);
IPC_API int luaopen_ipc_shm (lua_State * L);
IPC_API int luaopen_ipc_strfile (lua_State * L);

#endif /* IPC_H_ */

