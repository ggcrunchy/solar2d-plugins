/*
** $Id: lauxlib.h,v 1.88.1.1 2007/12/27 13:02:25 roberto Exp $
** Auxiliary functions for building Lua libraries
** See Copyright Notice in lua.h
*/


#ifndef lauxlib_h
#define lauxlib_h


#include <stddef.h>
#include <stdio.h>

#include "lua.h"


/* extra error code for `luaL_load' */
#define LUA_ERRFILE     (LUA_ERRERR+1)

typedef struct luaL_Reg {
  const char *name;
  lua_CFunction func;
} luaL_Reg;

LUALIB_API void (lj2L_openlib) (lua_State *L, const char *libname,
                                const luaL_Reg *l, int nup);
LUALIB_API void (lj2L_register) (lua_State *L, const char *libname,
                                const luaL_Reg *l);
LUALIB_API int (lj2L_getmetafield) (lua_State *L, int obj, const char *e);
LUALIB_API int (lj2L_callmeta) (lua_State *L, int obj, const char *e);
LUALIB_API int (lj2L_typerror) (lua_State *L, int narg, const char *tname);
LUALIB_API int (lj2L_argerror) (lua_State *L, int numarg, const char *extramsg);
LUALIB_API const char *(lj2L_checklstring) (lua_State *L, int numArg,
                                                          size_t *l);
LUALIB_API const char *(lj2L_optlstring) (lua_State *L, int numArg,
                                          const char *def, size_t *l);
LUALIB_API lua_Number (lj2L_checknumber) (lua_State *L, int numArg);
LUALIB_API lua_Number (lj2L_optnumber) (lua_State *L, int nArg, lua_Number def);

LUALIB_API lua_Integer (lj2L_checkinteger) (lua_State *L, int numArg);
LUALIB_API lua_Integer (lj2L_optinteger) (lua_State *L, int nArg,
                                          lua_Integer def);

LUALIB_API void (lj2L_checkstack) (lua_State *L, int sz, const char *msg);
LUALIB_API void (lj2L_checktype) (lua_State *L, int narg, int t);
LUALIB_API void (lj2L_checkany) (lua_State *L, int narg);

LUALIB_API int   (lj2L_newmetatable) (lua_State *L, const char *tname);
LUALIB_API void *(lj2L_checkudata) (lua_State *L, int ud, const char *tname);

LUALIB_API void (lj2L_where) (lua_State *L, int lvl);
LUALIB_API int (lj2L_error) (lua_State *L, const char *fmt, ...);

LUALIB_API int (lj2L_checkoption) (lua_State *L, int narg, const char *def,
                                   const char *const lst[]);

/* pre-defined references */
#define LUA_NOREF       (-2)
#define LUA_REFNIL      (-1)

LUALIB_API int (lj2L_ref) (lua_State *L, int t);
LUALIB_API void (lj2L_unref) (lua_State *L, int t, int ref);

LUALIB_API int (lj2L_loadfile) (lua_State *L, const char *filename);
LUALIB_API int (lj2L_loadbuffer) (lua_State *L, const char *buff, size_t sz,
                                  const char *name);
LUALIB_API int (lj2L_loadstring) (lua_State *L, const char *s);

LUALIB_API lua_State *(lj2L_newstate) (void);


LUALIB_API const char *(lj2L_gsub) (lua_State *L, const char *s, const char *p,
                                                  const char *r);

LUALIB_API const char *(lj2L_findtable) (lua_State *L, int idx,
                                         const char *fname, int szhint);

/* From Lua 5.2. */
LUALIB_API int lj2L_fileresult(lua_State *L, int stat, const char *fname);
LUALIB_API int lj2L_execresult(lua_State *L, int stat);
LUALIB_API int (lj2L_loadfilex) (lua_State *L, const char *filename,
				 const char *mode);
LUALIB_API int (lj2L_loadbufferx) (lua_State *L, const char *buff, size_t sz,
				   const char *name, const char *mode);
LUALIB_API void lj2L_traceback (lua_State *L, lua_State *L1, const char *msg,
				int level);
LUALIB_API void (lj2L_setfuncs) (lua_State *L, const luaL_Reg *l, int nup);
LUALIB_API void (lj2L_pushmodule) (lua_State *L, const char *modname,
				   int sizehint);
LUALIB_API void *(lj2L_testudata) (lua_State *L, int ud, const char *tname);
LUALIB_API void (lj2L_setmetatable) (lua_State *L, const char *tname);


/*
** ===============================================================
** some useful macros
** ===============================================================
*/

#define luaL_argcheck(L, cond,numarg,extramsg)	\
		((void)((cond) || lj2L_argerror(L, (numarg), (extramsg))))
#define luaL_checkstring(L,n)	(lj2L_checklstring(L, (n), NULL))
#define luaL_optstring(L,n,d)	(lj2L_optlstring(L, (n), (d), NULL))
#define luaL_checkint(L,n)	((int)lj2L_checkinteger(L, (n)))
#define luaL_optint(L,n,d)	((int)lj2L_optinteger(L, (n), (d)))
#define luaL_checklong(L,n)	((long)lj2L_checkinteger(L, (n)))
#define luaL_optlong(L,n,d)	((long)lj2L_optinteger(L, (n), (d)))

#define luaL_typename(L,i)	lj2_typename(L, lj2_type(L,(i)))

#define luaL_dofile(L, fn) \
	(lj2L_loadfile(L, fn) || lj2_pcall(L, 0, LUA_MULTRET, 0))

#define luaL_dostring(L, s) \
	(lj2L_loadstring(L, s) || lj2_pcall(L, 0, LUA_MULTRET, 0))

#define luaL_getmetatable(L,n)	(lj2_getfield(L, LUA_REGISTRYINDEX, (n)))

#define luaL_opt(L,f,n,d)	(lua_isnoneornil(L,(n)) ? (d) : f(L,(n)))

/* From Lua 5.2. */
#define luaL_newlibtable(L, l) \
	lj2_createtable(L, 0, sizeof(l)/sizeof((l)[0]) - 1)
#define luaL_newlib(L, l)	(luaL_newlibtable(L, l), lj2L_setfuncs(L, l, 0))

/*
** {======================================================
** Generic Buffer manipulation
** =======================================================
*/



typedef struct luaL_Buffer {
  char *p;			/* current position in buffer */
  int lvl;  /* number of strings in the stack (level) */
  lua_State *L;
  char buffer[LUAL_BUFFERSIZE];
} luaL_Buffer;

#define luaL_addchar(B,c) \
  ((void)((B)->p < ((B)->buffer+LUAL_BUFFERSIZE) || lj2L_prepbuffer(B)), \
   (*(B)->p++ = (char)(c)))

/* compatibility only */
#define luaL_putchar(B,c)	luaL_addchar(B,c)

#define luaL_addsize(B,n)	((B)->p += (n))

LUALIB_API void (lj2L_buffinit) (lua_State *L, luaL_Buffer *B);
LUALIB_API char *(lj2L_prepbuffer) (luaL_Buffer *B);
LUALIB_API void (lj2L_addlstring) (luaL_Buffer *B, const char *s, size_t l);
LUALIB_API void (lj2L_addstring) (luaL_Buffer *B, const char *s);
LUALIB_API void (lj2L_addvalue) (luaL_Buffer *B);
LUALIB_API void (lj2L_pushresult) (luaL_Buffer *B);


/* }====================================================== */

#endif
