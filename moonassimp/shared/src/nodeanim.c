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

int newnodeanim(lua_State *L, scene_t *scene, animation_t *animation, nodeanim_t *nodeanim)
    {
    ud_t *ud;
//  TRACE_CREATE(nodeanim, "nodeanim");
    ud = newuserdata(L, (void*)nodeanim, NODEANIM_MT);
    ud->scene = scene;
    ud->animation = animation;
    return 1;   
    }

int freenodeanim(lua_State *L, nodeanim_t *nodeanim)
    {
//  TRACE_DELETE(nodeanim, "nodeanim");
    freeuserdata(L, nodeanim);
    return 0;
    }


static int NodeName(lua_State *L)
    {
    nodeanim_t *nodeanim = checknodeanim(L, 1);
    if(nodeanim->mNodeName.length == 0)
        return 0;
    lua_pushstring(L, nodeanim->mNodeName.data);
    return 1;
    }


static int PreState(lua_State *L)
    {
    nodeanim_t *nodeanim = checknodeanim(L, 1);
    return pushanimbehaviour(L, nodeanim->mPreState);
    }

static int PostState(lua_State *L)
    {
    nodeanim_t *nodeanim = checknodeanim(L, 1);
    return pushanimbehaviour(L, nodeanim->mPostState);
    }

static int NumPositionKeys(lua_State *L)
    {
    nodeanim_t *nodeanim = checknodeanim(L, 1);
    lua_pushinteger(L, nodeanim->mNumPositionKeys);
    return 1;
    }

static int NumRotationKeys(lua_State *L)
    {
    nodeanim_t *nodeanim = checknodeanim(L, 1);
    lua_pushinteger(L, nodeanim->mNumRotationKeys);
    return 1;
    }

static int NumScalingKeys(lua_State *L)
    {
    nodeanim_t *nodeanim = checknodeanim(L, 1);
    lua_pushinteger(L, nodeanim->mNumScalingKeys);
    return 1;
    }


static int PositionKeys(lua_State *L)
/* { key1, key2, ... }
 *
 * key1 = { time = 123.456, value = { x, y, z } }
 */
    {
    unsigned int i;
    nodeanim_t *nodeanim = checknodeanim(L, 1);
    lua_newtable(L);
    for(i = 0; i < nodeanim->mNumPositionKeys; i++)
        {
#define key nodeanim->mPositionKeys[i]
        lua_newtable(L);
        lua_pushnumber(L, key.mTime);
        lua_setfield(L, -2, "time");
        pushvector3(L, &(key.mValue), 1);
        lua_setfield(L, -2, "value");
        lua_rawseti(L, -2, i+1);
#undef key
        }
    return 1;
    }

static int RotationKeys(lua_State *L)
/* { key1, key2, ... }
 *
 * key1 = { time = 123.456, value = { w, x, y, z } }
 */
    {
    unsigned int i;
    nodeanim_t *nodeanim = checknodeanim(L, 1);
    lua_newtable(L);
    for(i = 0; i < nodeanim->mNumRotationKeys; i++)
        {
#define key nodeanim->mRotationKeys[i]
        lua_newtable(L);
        lua_pushnumber(L, key.mTime);
        lua_setfield(L, -2, "time");
        pushquaternion(L, &(key.mValue), 1);
        lua_setfield(L, -2, "value");
        lua_rawseti(L, -2, i+1);
#undef key
        }
    return 1;
    }


static int ScalingKeys(lua_State *L)
/* { key1, key2, ... }
 *
 * key1 = { time = 123.456, value = { x, y, z } }
 */
    {
    unsigned int i;
    nodeanim_t *nodeanim = checknodeanim(L, 1);
    lua_newtable(L);
    for(i = 0; i < nodeanim->mNumScalingKeys; i++)
        {
#define key nodeanim->mScalingKeys[i]
        lua_newtable(L);
        lua_pushnumber(L, key.mTime);
        lua_setfield(L, -2, "time");
        pushvector3(L, &(key.mValue), 1);
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
        { "node_name", NodeName },
        { "num_position_keys", NumPositionKeys },
        { "num_rotation_keys", NumRotationKeys },
        { "num_scaling_keys", NumScalingKeys },
        { "position_keys", PositionKeys },
        { "rotation_keys", RotationKeys },
        { "scaling_keys", ScalingKeys },
        { "pre_state", PreState },
        { "post_state", PostState },
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


void moonassimp_open_nodeanim(lua_State *L)
    {
    udata_define(L, NODEANIM_MT, Methods, MetaMethods);
    luaL_setfuncs(L, Functions, 0);
    }

