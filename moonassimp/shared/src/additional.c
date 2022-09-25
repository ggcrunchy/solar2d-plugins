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

/*------------------------------------------------------------------------------*
 | Additional non-Assimp utilities                                              |
 *------------------------------------------------------------------------------*/

static int Textureflags(lua_State *L)
    {
    if(lua_type(L, 1) == LUA_TNUMBER)
        return pushtextureflags(L, luaL_checkinteger(L, 1), 0);
    lua_pushinteger(L, checktextureflags(L, 1));
    return 1;
    }

static int Primtypes(lua_State *L)
    {
    if(lua_type(L, 1) == LUA_TNUMBER)
        return pushprimitivetype(L, luaL_checkinteger(L, 1), 0);
    lua_pushinteger(L, checkprimitivetype(L, 1));
    return 1;
    }

static int Postprocessflags(lua_State *L)
    {
    if(lua_type(L, 1) == LUA_TNUMBER)
        return pushpostprocessflags(L, luaL_checkinteger(L, 1), 0);
    lua_pushinteger(L, checkpostprocessflags(L, 1));
    return 1;
    }


static int Sceneflags(lua_State *L)
    {
    if(lua_type(L, 1) == LUA_TNUMBER)
        return pushsceneflags(L, luaL_checkinteger(L, 1), 0);
    lua_pushinteger(L, checksceneflags(L, 1));
    return 1;
    }


/*------------------------------------------------------------------------------*
 | Registration                                                                 |
 *------------------------------------------------------------------------------*/

static const struct luaL_Reg Functions[] = 
    {
        { "textureflags", Textureflags },
        { "primtypes", Primtypes },
        { "postprocessflags", Postprocessflags },
        { "sceneflags", Sceneflags },
        { NULL, NULL } /* sentinel */
    };


void moonassimp_open_additional(lua_State *L)
    {
    luaL_setfuncs(L, Functions, 0);
    }

