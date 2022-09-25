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

int newbone(lua_State *L, scene_t *scene, mesh_t *mesh, bone_t *bone)
    {
    ud_t *ud;
    TRACE_CREATE(bone, "bone");
    ud = newuserdata(L, (void*)bone, BONE_MT);
    ud->scene = scene;
    ud->mesh = mesh;
    return 1;   
    }

int freebone(lua_State *L, bone_t *bone)
    {
    TRACE_DELETE(bone, "bone");
    freeuserdata(L, bone);
    return 0;
    }

static int Name(lua_State *L)
    {
    bone_t *bone = checkbone(L, 1);
    if(bone->mName.length == 0)
        return 0;
    lua_pushstring(L, bone->mName.data);
    return 1;
    }


static int NumWeights(lua_State *L)
    {
    bone_t *bone = checkbone(L, 1);
    lua_pushnumber(L, bone->mNumWeights);
    return 1;
    }

static int Weights(lua_State *L)
/* returns a (possibly sparse) table t 
 * where t[i] is the weight for the vertex i 
 * (i= 1-based index in the mesh's vertices list)
 */
    {
    unsigned int i;
    bone_t *bone = checkbone(L, 1);
    lua_newtable(L);
    if(bone->mWeights == NULL || bone->mNumWeights == 0) return 0;
    for(i = 0; i < bone->mNumWeights; i++)
        {
#define w bone->mWeights[i]
        lua_pushnumber(L, w.mWeight);
        lua_rawseti(L, -2, w.mVertexId+1);
#undef w
        }
    return 1;
    }


static int OrderedWeights(lua_State *L)
/* returns a table t of elements { vertex=i, weight=val }
 */
    {
    unsigned int i;
    bone_t *bone = checkbone(L, 1);
    lua_newtable(L);
    if(bone->mWeights == NULL || bone->mNumWeights == 0) return 0;
    for(i = 0; i < bone->mNumWeights; i++)
        {
#define w bone->mWeights[i]
        lua_newtable(L);
        lua_pushinteger(L, w.mVertexId+1);
        lua_setfield(L, -2, "vertex");
        lua_pushnumber(L, w.mWeight);
        lua_setfield(L, -2, "weight");
        lua_rawseti(L, -2, i+1);
#undef w
        }
    return 1;
    }

static int OffsetMatrix(lua_State *L)
    {
    bone_t *bone = checkbone(L, 1);
    return pushmatrix4(L, &(bone->mOffsetMatrix), 0);
    }


/*------------------------------------------------------------------------------*
 | Registration                                                                 |
 *------------------------------------------------------------------------------*/

static const struct luaL_Reg Methods[] = 
    {
        { "name", Name },
        { "num_weights", NumWeights },
        { "weights", Weights },
        { "ordered_weights", OrderedWeights },
        { "offset_matrix", OffsetMatrix },
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


void moonassimp_open_bone(lua_State *L)
    {
    udata_define(L, BONE_MT, Methods, MetaMethods);
    luaL_setfuncs(L, Functions, 0);
    }

