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

int newface(lua_State *L, scene_t *scene, mesh_t *mesh, face_t *face)
    {
    ud_t *ud;
//  TRACE_CREATE(face, "face");
    ud = newuserdata(L, (void*)face, FACE_MT);
    ud->scene = scene;
    ud->mesh = mesh;
    return 1;   
    }

int freeface(lua_State *L, face_t *face)
    {
//  TRACE_DELETE(face, "face");
    freeuserdata(L, face);
    return 0;
    }

static int NumIndices(lua_State *L)
    {
    face_t * face = checkface(L, 1);
    lua_pushinteger(L, face->mNumIndices);
    return 1;
    }

static int Indices(lua_State *L)
    {
    unsigned int i;
    face_t * face = checkface(L, 1);
    if(face->mIndices == NULL || face->mNumIndices == 0)
        return 0;
    luaL_checkstack(L, face->mNumIndices, NULL);
    for(i=0; i < face->mNumIndices; i++)
        pushindex(L, face->mIndices[i]);
    return face->mNumIndices;
    }

static int ZeroBasedIndices(lua_State *L)
    {
    unsigned int i;
    face_t * face = checkface(L, 1);
    if(face->mIndices == NULL || face->mNumIndices == 0)
        return 0;
    luaL_checkstack(L, face->mNumIndices, NULL);
    for(i=0; i < face->mNumIndices; i++)
        lua_pushinteger(L, face->mIndices[i]);
    return face->mNumIndices;
    }

int pushfaceindices(lua_State *L, face_t *face, int zero_based)
/* pushes a table with the face indices */
    {
    unsigned int i;
    lua_newtable(L);
    if(face->mIndices == NULL || face->mNumIndices == 0)
        return 1;
    if(zero_based)
        {
        for(i=0; i < face->mNumIndices; i++)
            {
            lua_pushinteger(L, face->mIndices[i]);
            lua_rawseti(L, -2, i+1);
            }
        }
    else
        {
        for(i=0; i < face->mNumIndices; i++)
            {
            lua_pushinteger(L, face->mIndices[i] + 1);
            lua_rawseti(L, -2, i+1);
            }
        }
    return 1;
    }


/*------------------------------------------------------------------------------*
 | Registration                                                                 |
 *------------------------------------------------------------------------------*/

static const struct luaL_Reg Methods[] = 
    {
        { "num_indices", NumIndices },
        { "indices", Indices },
        { "zero_based_indices", ZeroBasedIndices },
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


void moonassimp_open_face(lua_State *L)
    {
    udata_define(L, FACE_MT, Methods, MetaMethods);
    luaL_setfuncs(L, Functions, 0);
    }


