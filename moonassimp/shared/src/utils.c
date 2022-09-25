/* The MIT License (MIT)
 *
 * Copyright (c) 2016 Stefano Trettel
 *
 * Software repository: MoonAssimp, https://github.com/stetre/moonassimp
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include "internal.h"

int trace_enabled = 0;
int trace(const char *fmt, ...)
    {
    va_list args;
    if(!trace_enabled)
        return 0;
    if(fmt)
        {
        va_start(args,fmt);
        vprintf(fmt,args);
        va_end(args);
        }
    return 0;
    }

int noprintf(const char *fmt, ...) 
    { (void)fmt; return 0; }

int notavailable(lua_State *L, ...) 
    { NOT_AVAILABLE; }

int nilerrmsg(lua_State *L)
    {
    lua_pushnil(L); 
    lua_pushstring(L, aiGetErrorString()); 
    return 2;
    }


/*------------------------------------------------------------------------------*
 | Custom luaL_checkxxx() style functions                                       |
 *------------------------------------------------------------------------------*/

int checkoption_hint(lua_State *L, int arg, const char *def, const char *const lst[])
/* Variant of luaL_checkoption(), with an added hint in the error message */
    {
    const char *hint = NULL;
    const char *name = (def) ? luaL_optstring(L, arg, def) : luaL_checkstring(L, arg);
    int i;
    for (i=0; lst[i]; i++)
        if (strcmp(lst[i], name) == 0)  return i;

    if(lua_checkstack(L, i*2))
        {
        for(i=0; lst[i]; i++)
            {
            lua_pushfstring(L, "'%s'", lst[i]);
            lua_pushstring(L, "|");
            }
        i = i*2;
        if(i>0)
            {
            lua_pop(L, 1); /* the last separator */
            lua_concat(L, i-1);
            hint = lua_tostring(L, -1); 
            }
        }
    if(hint)
        return luaL_argerror(L, arg, lua_pushfstring(L, 
                    "invalid option '%s', valid options are: %s", name, hint));
    return luaL_argerror(L, arg, lua_pushfstring(L, "invalid option '%s'", name));
    }


int checkboolean(lua_State *L, int arg)
    {
    if(!lua_isboolean(L, arg))
        return (int)luaL_argerror(L, arg, "boolean expected");
    return lua_toboolean(L, arg) ? AI_TRUE : AI_FALSE;
    }

int optboolean(lua_State *L, int arg, int d)
    {
    if(!lua_isboolean(L, arg))
        return d;
    return lua_toboolean(L, arg);
    }


/* 1-based index to 0-based ------------------------------------------*/

int checkindex(lua_State *L, int arg)
    {
    int val = luaL_checkinteger(L, arg);
    if(val < 1)
        return luaL_argerror(L, arg, "positive integer expected");
    return val - 1;
    }

int optindex(lua_State *L, int arg, int optval /* 0-based */)
    {
    int val = luaL_optinteger(L, arg, optval + 1);
    if(val < 1)
        return luaL_argerror(L, arg, "positive integer expected");
    return val - 1;
    }

void pushindex(lua_State *L, int val)
    { lua_pushinteger((L), (val) + 1); }

/* vectors, colors, quaternions etc -------------------------------------*/

int pushvector2(lua_State *L, vector2_t *vec, int astable)
    {
    if(astable)
        {
        lua_newtable(L);
        lua_pushnumber(L, vec->x); lua_seti(L, -2, 1);
        lua_pushnumber(L, vec->y); lua_seti(L, -2, 2);
        return 1;
        }
    lua_pushnumber(L, vec->x);  
    lua_pushnumber(L, vec->y);  
    return 2;
    }

int pushvector3(lua_State *L, vector3_t *vec, int astable)
    {
    if(astable)
        {
        lua_newtable(L);
        lua_pushnumber(L, vec->x); lua_seti(L, -2, 1);
        lua_pushnumber(L, vec->y); lua_seti(L, -2, 2);
        lua_pushnumber(L, vec->z); lua_seti(L, -2, 3);
        return 1;
        }
    lua_pushnumber(L, vec->x);  
    lua_pushnumber(L, vec->y);  
    lua_pushnumber(L, vec->z);  
    return 3;
    }


int pushcolor3(lua_State *L, color3_t *col, int astable)
    {
    if(astable)
        {
        lua_newtable(L);
        lua_pushnumber(L, col->r); lua_seti(L, -2, 1);
        lua_pushnumber(L, col->g); lua_seti(L, -2, 2);
        lua_pushnumber(L, col->b); lua_seti(L, -2, 3);
        return 1;
        }
    lua_pushnumber(L, col->r);  
    lua_pushnumber(L, col->g);  
    lua_pushnumber(L, col->b);  
    return 3;
    }

int pushcolor4(lua_State *L, color4_t *col, int astable)
    {
    if(astable)
        {
        lua_newtable(L);
        lua_pushnumber(L, col->r); lua_seti(L, -2, 1);
        lua_pushnumber(L, col->g); lua_seti(L, -2, 2);
        lua_pushnumber(L, col->b); lua_seti(L, -2, 3);
        lua_pushnumber(L, col->a); lua_seti(L, -2, 4);
        return 1;
        }
    lua_pushnumber(L, col->r);  
    lua_pushnumber(L, col->g);  
    lua_pushnumber(L, col->b);  
    lua_pushnumber(L, col->a);  
    return 4;
    }


int pushquaternion(lua_State *L, quaternion_t *quat, int astable)
    {
    if(astable)
        {
        lua_newtable(L);
        lua_pushnumber(L, quat->w); lua_seti(L, -2, 1);
        lua_pushnumber(L, quat->x); lua_seti(L, -2, 2);
        lua_pushnumber(L, quat->y); lua_seti(L, -2, 3);
        lua_pushnumber(L, quat->z); lua_seti(L, -2, 4);
        return 1;
        }
    lua_pushnumber(L, quat->w); 
    lua_pushnumber(L, quat->x); 
    lua_pushnumber(L, quat->y); 
    lua_pushnumber(L, quat->z); 
    return 4;
    }

int pushtexel(lua_State *L, texel_t *vec, int astable)
    {
    if(astable)
        {
        lua_newtable(L);
        lua_pushinteger(L, vec->r); lua_seti(L, -2, 1);
        lua_pushinteger(L, vec->g); lua_seti(L, -2, 2);
        lua_pushinteger(L, vec->b); lua_seti(L, -2, 3);
        lua_pushinteger(L, vec->a); lua_seti(L, -2, 4);
        return 1;
        }
    lua_pushinteger(L, vec->r); 
    lua_pushinteger(L, vec->g); 
    lua_pushinteger(L, vec->b); 
    lua_pushinteger(L, vec->a); 
    return 4;
    }

int pushtexelf(lua_State *L, texel_t *vec, int astable)
    {
    if(astable)
        {
        lua_newtable(L);
        lua_pushnumber(L, vec->r/255.f); lua_seti(L, -2, 1);
        lua_pushnumber(L, vec->g/255.f); lua_seti(L, -2, 2);
        lua_pushnumber(L, vec->b/255.f); lua_seti(L, -2, 3);
        lua_pushnumber(L, vec->a/255.f); lua_seti(L, -2, 4);
        return 1;
        }
    lua_pushnumber(L, vec->r/255.f);    
    lua_pushnumber(L, vec->g/255.f);    
    lua_pushnumber(L, vec->b/255.f);    
    lua_pushnumber(L, vec->a/255.f);    
    return 4;
    }



/* pushmatrix how
 *
 * how=0 : flat     -> a1, a2, ...
 * how=1 : vectors  -> { a1, a2, a3 }, { b1, .. }, { c1, .. }
 * how=2 : table    -> {{ a1, a2, a3 }, { b1, .. }, { c1, .. }}
 */

int pushmatrix3(lua_State *L, matrix3_t *mat, int how)
    {
    if(how==0)
        {
        luaL_checkstack(L, 9, NULL);
        lua_pushnumber(L, mat->a1); 
        lua_pushnumber(L, mat->a2); 
        lua_pushnumber(L, mat->a3); 
        lua_pushnumber(L, mat->b1); 
        lua_pushnumber(L, mat->b2); 
        lua_pushnumber(L, mat->b3); 
        lua_pushnumber(L, mat->c1); 
        lua_pushnumber(L, mat->c2); 
        lua_pushnumber(L, mat->c3); 
        return 9;
        }
    if(how==2) lua_newtable(L);

    lua_newtable(L);
    lua_pushnumber(L, mat->a1); lua_seti(L, -2, 1);
    lua_pushnumber(L, mat->a2); lua_seti(L, -2, 2);
    lua_pushnumber(L, mat->a3); lua_seti(L, -2, 3);
    if(how==2) lua_seti(L, -2, 1);
    lua_newtable(L);
    lua_pushnumber(L, mat->b1); lua_seti(L, -2, 1);
    lua_pushnumber(L, mat->b2); lua_seti(L, -2, 2);
    lua_pushnumber(L, mat->b3); lua_seti(L, -2, 3);
    if(how==2) lua_seti(L, -2, 2);
    lua_newtable(L);
    lua_pushnumber(L, mat->c1); lua_seti(L, -2, 1);
    lua_pushnumber(L, mat->c2); lua_seti(L, -2, 2);
    lua_pushnumber(L, mat->c3); lua_seti(L, -2, 3);
    if(how==2) lua_seti(L, -2, 3);

    return (how==2) ? 1 : 3;
    }


int pushmatrix4(lua_State *L, matrix4_t *mat, int how)
    {
    if(how==0)
        {
        luaL_checkstack(L, 16, NULL);
        lua_pushnumber(L, mat->a1); 
        lua_pushnumber(L, mat->a2); 
        lua_pushnumber(L, mat->a3); 
        lua_pushnumber(L, mat->a4); 
        lua_pushnumber(L, mat->b1); 
        lua_pushnumber(L, mat->b2); 
        lua_pushnumber(L, mat->b3); 
        lua_pushnumber(L, mat->b4); 
        lua_pushnumber(L, mat->c1); 
        lua_pushnumber(L, mat->c2); 
        lua_pushnumber(L, mat->c3); 
        lua_pushnumber(L, mat->c4); 
        lua_pushnumber(L, mat->d1); 
        lua_pushnumber(L, mat->d2); 
        lua_pushnumber(L, mat->d3); 
        lua_pushnumber(L, mat->d4); 
        return 16;
        }
    if(how==2) lua_newtable(L);

    lua_newtable(L);
    lua_pushnumber(L, mat->a1); lua_seti(L, -2, 1);
    lua_pushnumber(L, mat->a2); lua_seti(L, -2, 2);
    lua_pushnumber(L, mat->a3); lua_seti(L, -2, 3);
    lua_pushnumber(L, mat->a4); lua_seti(L, -2, 4);
    if(how==2) lua_seti(L, -2, 1);
    lua_newtable(L);
    lua_pushnumber(L, mat->b1); lua_seti(L, -2, 1);
    lua_pushnumber(L, mat->b2); lua_seti(L, -2, 2);
    lua_pushnumber(L, mat->b3); lua_seti(L, -2, 3);
    lua_pushnumber(L, mat->b4); lua_seti(L, -2, 4);
    if(how==2) lua_seti(L, -2, 2);
    lua_newtable(L);
    lua_pushnumber(L, mat->c1); lua_seti(L, -2, 1);
    lua_pushnumber(L, mat->c2); lua_seti(L, -2, 2);
    lua_pushnumber(L, mat->c3); lua_seti(L, -2, 3);
    lua_pushnumber(L, mat->c4); lua_seti(L, -2, 4);
    if(how==2) lua_seti(L, -2, 3);
    lua_newtable(L);
    lua_pushnumber(L, mat->d1); lua_seti(L, -2, 1);
    lua_pushnumber(L, mat->d2); lua_seti(L, -2, 2);
    lua_pushnumber(L, mat->d3); lua_seti(L, -2, 3);
    lua_pushnumber(L, mat->d4); lua_seti(L, -2, 4);
    if(how==2) lua_seti(L, -2, 4);

    return (how==2) ? 1 : 4;
    }


/*------------------------------------------------------------------------------*
 | userdata handling                                                            |
 *------------------------------------------------------------------------------*/

ud_t *newuserdata(lua_State *L, void *ptr, const char *mt)
    {
    ud_t *ud;
    ud = (ud_t*)udata_new(L, sizeof(ud_t), ptr, mt);
    memset(ud, 0, sizeof(ud_t));
    ud->obj = ptr ? ptr : (void*)ud;
    MarkValid(ud);
    return ud;
    }

int freeuserdata(lua_State *L, void *ptr)
    {
    ud_t *ud = userdata(ptr);
    if(!ud || !IsValid(ud))
        return 0; /* already deleted */
    CancelValid(ud);
    udata_free(L, ptr);
    return 1;
    }

void* testxxx(lua_State *L, int arg, const char *mt)
    {
    ud_t *ud = (ud_t*)udata_test(L, arg, mt);
    if(ud && IsValid(ud))
        return ud->obj;
    return NULL;
    }

void* checkxxx(lua_State *L, int arg, const char *mt)
    {
    void *p = testxxx(L, arg, mt);
    if(p) return p;
    lua_pushfstring(L, "not a %s", mt);
    luaL_argerror(L, arg, lua_tostring(L, -1));
    return NULL;
    }

int pushxxx(lua_State *L, void *p)
    { return udata_push(L, p); }

