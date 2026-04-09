/*
** Standard library header.
** Copyright (C) 2005-2026 Mike Pall. See Copyright Notice in luajit.h
*/

#ifndef _LUALIB_H
#define _LUALIB_H

#include "lua.h"

#define LUA_FILEHANDLE	"FILE*"

#define LUA_COLIBNAME	"coroutine"
#define LUA_MATHLIBNAME	"math"
#define LUA_STRLIBNAME	"string"
#define LUA_TABLIBNAME	"table"
#define LUA_IOLIBNAME	"io"
#define LUA_OSLIBNAME	"os"
#define LUA_LOADLIBNAME	"package"
#define LUA_DBLIBNAME	"debug"
#define LUA_BITLIBNAME	"bit"
#define LUA_JITLIBNAME	"jit"
#define LUA_FFILIBNAME	"ffi"

LUALIB_API int lj2open_base(lua_State *L);
LUALIB_API int lj2open_math(lua_State *L);
LUALIB_API int lj2open_string(lua_State *L);
LUALIB_API int lj2open_table(lua_State *L);
LUALIB_API int lj2open_io(lua_State *L);
LUALIB_API int lj2open_os(lua_State *L);
LUALIB_API int lj2open_package(lua_State *L);
LUALIB_API int lj2open_debug(lua_State *L);
LUALIB_API int lj2open_bit(lua_State *L);
LUALIB_API int lj2open_jit(lua_State *L);
LUALIB_API int lj2open_ffi(lua_State *L);
LUALIB_API int lj2open_string_buffer(lua_State *L);

LUALIB_API void lj2L_openlibs(lua_State *L);

#ifndef lua_assert
#define lua_assert(x)	((void)0)
#endif

#endif
