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

int newanimation(lua_State *L, scene_t *scene, animation_t *animation)
    {
    unsigned int i;
    ud_t *ud;
    TRACE_CREATE(animation, "animation");
    if(animation->mChannels != NULL && animation->mNumChannels > 0)
        {
        for(i = 0; i < animation->mNumChannels; i++)
            { newnodeanim(L, scene, animation, animation->mChannels[i]); lua_pop(L, 1); }
        TRACE_CREATE_N(i, "nodeanims");
        }
    if(animation->mMeshChannels != NULL && animation->mNumMeshChannels > 0)
        {
        for(i = 0; i < animation->mNumMeshChannels; i++)
            { newmeshanim(L, scene, animation, animation->mMeshChannels[i]); lua_pop(L, 1); }
        TRACE_CREATE_N(i, "meshanims");
        }
    ud = newuserdata(L, (void*)animation, ANIMATION_MT);
    ud->scene = scene;
    return 1;   
    }

int freeanimation(lua_State *L, animation_t *animation)
    {
    unsigned int i;
    if(animation->mChannels != NULL && animation->mNumChannels > 0)
        {
        for(i = 0; i < animation->mNumChannels; i++)
            freenodeanim(L, animation->mChannels[i]);
        TRACE_DELETE_N(i, "nodeanims");
        }
    if(animation->mMeshChannels != NULL && animation->mNumMeshChannels > 0)
        {
        for(i = 0; i < animation->mNumMeshChannels; i++)
            freemeshanim(L, animation->mMeshChannels[i]);
        TRACE_DELETE_N(i, "meshanims");
        }
    TRACE_DELETE(animation, "animation");

    freeuserdata(L, animation);
    return 0;
    }

static int Name(lua_State *L)
    {
    animation_t *animation = checkanimation(L, 1);
    if(animation->mName.length == 0)
        return 0;
    lua_pushstring(L, animation->mName.data);
    return 1;
    }


static int Duration(lua_State *L)
    {
    animation_t *animation = checkanimation(L, 1);
    lua_pushnumber(L, animation->mDuration);
    return 1;
    }

static int TicksPerSecond(lua_State *L)
    {
    animation_t *animation = checkanimation(L, 1);
    lua_pushnumber(L, animation->mTicksPerSecond);
    return 1;
    }

static int NumChannels(lua_State *L)
    {
    animation_t *animation = checkanimation(L, 1);
    lua_pushinteger(L, animation->mNumChannels);
    return 1;
    }

static int NumMeshChannels(lua_State *L)
    {
    animation_t *animation = checkanimation(L, 1);
    lua_pushinteger(L, animation->mNumMeshChannels);
    return 1;
    }


static int Channel(lua_State *L)
/* nodeanim = animation:node_anim(node_name) */
    {
    unsigned int i;
    animation_t *animation = checkanimation(L, 1);
    const char *name = luaL_checkstring(L, 2);
    if(animation->mChannels == NULL || animation->mNumChannels == 0) return 0;
    for(i = 0; i < animation->mNumChannels; i++)
        {
#define nodename animation->mChannels[i]->mNodeName
        if(strncmp(name, nodename.data, nodename.length) == 0) /* found */
            {
            pushnodeanim(L, animation->mChannels[i]);
            return 1;
            }
#undef nodename
        }
    return 0; /* not found */
    }


static int Channels(lua_State *L)
    {
    unsigned int i;
    animation_t *animation = checkanimation(L, 1);
    lua_newtable(L);
    if(animation->mChannels == NULL || animation->mNumChannels == 0) return 1;
    for(i = 0; i < animation->mNumChannels; i++)
        {
        pushnodeanim(L, animation->mChannels[i]);
        lua_rawseti(L, -2, i+1);
        }
    return 1;
    }


static int MeshChannels(lua_State *L)
    {
    unsigned int i;
    animation_t *animation = checkanimation(L, 1);
    const char *name = luaL_optstring(L, 2, NULL);
    lua_newtable(L);
    if(animation->mMeshChannels == NULL || animation->mNumMeshChannels == 0) return 1;
    for(i = 0; i < animation->mNumMeshChannels; i++)
        {
#define maname animation->mMeshChannels[i]->mName
        if(!name || (strncmp(name, maname.data, maname.length) == 0))
            {
            pushmeshanim(L, animation->mMeshChannels[i]);
            lua_rawseti(L, -2, i+1);
            }
#undef maname
        }
    return 1;
    }


/*------------------------------------------------------------------------------*
 | Registration                                                                 |
 *------------------------------------------------------------------------------*/

static const struct luaL_Reg Methods[] = 
    {
        { "name", Name },
        { "duration", Duration },
        { "ticks_per_second", TicksPerSecond },
        { "num_node_anims", NumChannels },
        { "node_anim", Channel },
        { "node_anims", Channels },
        { "num_mesh_anims", NumMeshChannels },
        { "mesh_anims", MeshChannels },
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


void moonassimp_open_animation(lua_State *L)
    {
    udata_define(L, ANIMATION_MT, Methods, MetaMethods);
    luaL_setfuncs(L, Functions, 0);
    }

