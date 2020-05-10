#include <stddef.h>
#include <lua.h>
#include <lauxlib.h>
#include "ipc.h"
#include "memfile.h"


static int str_open( lua_State* L ) {
  size_t len = 0;
  char const* str = luaL_checklstring( L, 1, &len );
  memfile_new( L, (void*)str, len, MEMFILE_R, 1, 0, 0 );
  return 1;
}


IPC_API int luaopen_ipc_strfile( lua_State* L ) {
  luaL_Reg const functions[] = {
    { "open", str_open },
    { NULL, NULL }
  };
  luaL_newlib( L, functions );
  return 1;
}

