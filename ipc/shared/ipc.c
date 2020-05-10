#include <stddef.h>
#include <string.h>
#include <stdio.h>
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>
#include "ipc.h"


IPC_LOCAL FILE* ipc_checkfile( lua_State* L, int idx ) {
#if LUA_VERSION_NUM == 501 /* Lua 5.1 / LuaJIT */
  FILE** fp = luaL_checkudata( L, idx, LUA_FILEHANDLE );
  if( *fp == NULL )
    luaL_error( L, "attempt to use closed file" );
  return *fp;
#elif LUA_VERSION_NUM >= 502 && LUA_VERSION_NUM <= 503
  luaL_Stream* s = luaL_checkudata( L, idx, LUA_FILEHANDLE );
  if( s->closef == 0 || s->f == NULL )
    luaL_error( L, "attempt to use closed file" );
  return s->f;
#else
#error unsupported Lua version
#endif
}


IPC_LOCAL FILE* ipc_testfile( lua_State* L, int idx ) {
#if LUA_VERSION_NUM == 501 /* Lua 5.1 / LuaJIT */
  FILE** fp = luaL_testudata( L, idx, LUA_FILEHANDLE );
  if( fp == NULL )
    return NULL;
  else if( *fp == NULL )
    luaL_error( L, "attempt to use closed filed" );
  return *fp;
#elif LUA_VERSION_NUM >= 502 && LUA_VERSION_NUM <= 503
  luaL_Stream* s = luaL_testudata( L, idx, LUA_FILEHANDLE );
  if( s == NULL )
    return NULL;
  else if( s->closef == 0 || s->f == NULL )
    luaL_error( L, "attempt to use closed filed" );
  return s->f;
#else
#error unsupported Lua version
#endif
}


IPC_LOCAL int ipc_getuservaluefield( lua_State* L, int idx,
                                     char const* name ) {
  luaL_checkstack( L, 2, "not enough stack space" );
  lua_getuservalue( L, idx );
  if( lua_type( L, -1 ) == LUA_TTABLE )
    lua_getfield( L, -1, name );
  else
    lua_pushnil( L );
  lua_replace( L, -2 );
  return lua_type( L, -1 );
}


IPC_LOCAL void ipc_setuservaluefield( lua_State* L, int idx,
                                      char const* name ) {
  luaL_checkstack( L, 2, "not enough stack space" );
  lua_getuservalue( L, idx );
  if( lua_type( L, -1 ) != LUA_TTABLE )
    luaL_error( L, "attempt to set field of non-table uservalue" );
  lua_pushvalue( L, -2 );
  lua_setfield( L, -2, name );
  lua_pop( L, 2 );
}


IPC_LOCAL int ipc_err( char const* file, int line, char const* func,
                       int code ) {
  if( code != 0 ) {
    if( func != NULL )
      fprintf( stderr, "[%s:%d] error return (%d) in function '%s'\n",
               file, line, code, func );
    else
      fprintf( stderr, "[%s:%d]: error return (%d)\n",
               file, line, code );
    fflush( stderr );
  }
  return code;
}


/* implementation of compatibility functions */
#if LUA_VERSION_NUM == 501
IPC_LOCAL int ipc_absindex( lua_State* L, int idx ) {
  if( idx < 0 && idx > LUA_REGISTRYINDEX )
    idx += lua_gettop( L )+1;
  return idx;
}


IPC_LOCAL void* ipc_testudata( lua_State* L, int idx,
                               char const* name ) {
  void* p = lua_touserdata( L, idx );
  if( p == NULL || !lua_getmetatable( L, idx ) )
    return NULL;
  else {
    int res = 0;
    luaL_getmetatable( L, name );
    res = lua_rawequal( L, -1, -2 );
    lua_pop( L, 2 );
    if( !res )
      p = NULL;
  }
  return p;
}
#endif /* LUA_VERSION_NUM == 501 */


//#ifdef _WIN32
/* LuaRocks with MSVC can't really handle multiple modules in a single
 * DLL, so we have to export the luaopen_ functions ourself, and let
 * LuaRocks think that the ipc.dll contains the ipc module: */
/*IPC_API*/CORONA_EXPORT int luaopen_plugin_ipc( lua_State* L ) {
	luaL_Reg funcs[] = {
		{ "filelock", luaopen_ipc_filelock },
		{ "mmap", luaopen_ipc_mmap },
		{ "proc", luaopen_ipc_proc },
		{ "sem", luaopen_ipc_sem },
		{ "shm", luaopen_ipc_shm },
		{ "strfile", luaopen_ipc_strfile },
		{ 0, 0 }
	};
	int i;

	lua_createtable(L, 6, 0);	// ipc

	for (i = 0; funcs[i].func; ++i)
	{
		lua_pushcfunction(L, funcs[i].func);// ipc, func
		lua_call(L, 0, 1);	// ipc, mod
		lua_setfield(L, -2, funcs[i].name);	// ipc = { ..., name = mod }
	}
//  (void)L;
  return 1;//0;
}
//#endif

