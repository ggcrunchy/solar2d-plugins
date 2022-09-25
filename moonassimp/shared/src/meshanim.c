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

int newmeshanim(lua_State *L, scene_t *scene, animation_t *animation, meshanim_t *meshanim)
    {
    ud_t *ud;
//  TRACE_CREATE(meshanim, "meshanim");
    ud = newuserdata(L, (void*)meshanim, MESHANIM_MT);
    ud->scene = scene;
    ud->animation = animation;
    return 1;   
    }

int freemeshanim(lua_State *L, meshanim_t *meshanim)
    {
//  TRACE_DELETE(meshanim, "meshanim");
    freeuserdata(L, meshanim);
    return 0;
    }


static int Name(lua_State *L)
    {
    meshanim_t *meshanim = checkmeshanim(L, 1);
    if(meshanim->mName.length == 0)
        return 0;
    lua_pushstring(L, meshanim->mName.data);
    return 1;
    }

static int NumKeys(lua_State *L)
    {
    meshanim_t *meshanim = checkmeshanim(L, 1);
    lua_pushinteger(L, meshanim->mNumKeys);
    return 1;
    }


static int Keys(lua_State *L)
/* { key1, key2, ... }
 *
 * key1 = { time = 123.456, value = index }
 */
    {
    unsigned int i;
    meshanim_t *meshanim = checkmeshanim(L, 1);
    lua_newtable(L);
    for(i = 0; i < meshanim->mNumKeys; i++)
        {
#define key meshanim->mKeys[i]
        lua_newtable(L);
        lua_pushnumber(L, key.mTime);
        lua_setfield(L, -2, "time");
        pushindex(L, key.mValue);
        lua_setfield(L, -2, "value");
        lua_rawseti(L, -2, i+1);
#undef key
        }
    return 1;
    }

/*------------------------------------------------------------------------------*
 | Registration                                                                 |
 *------------------------------------------------------------------------------*/

static const struct luaL_Reg Methods[] = 
    {
        { "mesh_name", Name },
        { "num_keys", NumKeys },
        { "keys", Keys },
        { NULL, NULL } /* sentinel */
    };


static const struct luaL_Reg MetaMethods[] = 
    {
        { NULL, NULL } /* sentinel */
    };

static const struct luaL_Reg Functions[] = 
    {
        { NULL, NULL } /* sentinel */
    };


void moonassimp_open_meshanim(lua_State *L)
    {
    udata_define(L, MESHANIM_MT, Methods, MetaMethods);
    luaL_setfuncs(L, Functions, 0);
    }

