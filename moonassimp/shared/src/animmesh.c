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

int newanimmesh(lua_State *L, scene_t *scene, mesh_t *mesh, animmesh_t *animmesh)
    {
    ud_t *ud;
//  TRACE_CREATE(animmesh, "animmesh");
    ud = newuserdata(L, (void*)animmesh, ANIMMESH_MT);
    ud->scene = scene;
    ud->mesh = mesh;
    return 1;   
    }

int freeanimmesh(lua_State *L, animmesh_t *animmesh)
    {
//  TRACE_DELETE(animmesh, "animmesh");
    freeuserdata(L, animmesh);
    return 0;
    }


#define F(what)                                     \
static int Has##what(lua_State *L)                  \
    {                                               \
    animmesh_t *animmesh = checkanimmesh(L, 1);     \
    lua_pushboolean(L, animmesh->m##what != NULL);  \
    return 1;                                       \
    }
F(Vertices)
F(Normals)
F(Tangents)

#undef F

static int Mesh(lua_State *L)
    {
    animmesh_t *animmesh = checkanimmesh(L, 1);
    ud_t *ud = userdata(animmesh);
    mesh_t *mesh = ud->mesh;
    return pushmesh(L, mesh);
    }

static int HasColors(lua_State *L)
    {
    animmesh_t *animmesh = checkanimmesh(L, 1);
    unsigned int i = checkindex(L, 2);
    if( i >= AI_MAX_NUMBER_OF_COLOR_SETS )
        lua_pushboolean(L, 0);
    else
        lua_pushboolean(L, animmesh->mColors[i] != NULL);
    return 1;
    }


static int HasTextureCoords(lua_State *L)
    {
    animmesh_t *animmesh = checkanimmesh(L, 1);
    unsigned int i = checkindex(L, 2);
    if( i >= AI_MAX_NUMBER_OF_TEXTURECOORDS )
        lua_pushboolean(L, 0);
    else
        lua_pushboolean(L, animmesh->mTextureCoords[i] != NULL);
    return 1;
    }


#if 0
static int NumVertices(lua_State *L)
/* From Assimp source code:
 * "The number of vertices in the aiAnimMesh, and thus the length of all the member arrays.
 * This has always the same value as the mNumVertices property in the corresponding aiMesh. 
 * It is duplicated here merely to make the length of the member arrays accessible even if 
 * the aiMesh is not known, e.g. from language bindings." */

    {
    animmesh_t *animmesh = checkanimmesh(L, 1);
    lua_pushinteger(L, animmesh->mNumVertices);
    return 1;
    }
#endif

#define F(func, what)                                       \
static int func(lua_State *L)                               \
    {                                                       \
    animmesh_t *animmesh = checkanimmesh(L, 1);             \
    unsigned int i = checkindex(L, 2);                      \
    if( animmesh->m##what == NULL || i >= animmesh->mNumVertices )  \
        return luaL_argerror(L, 2, "out of range");         \
    return pushvector3(L, &(animmesh->m##what[i]), 0);      \
    }

F(Position, Vertices)
F(Normal, Normals)
F(Tangent, Tangents)
F(Bitangent, Bitangents)

#undef F

static int Color(lua_State *L)
    {
    animmesh_t *animmesh = checkanimmesh(L, 1);
    unsigned int chan = checkindex(L, 2);
    unsigned int i = checkindex(L, 3);
    if( chan >= AI_MAX_NUMBER_OF_COLOR_SETS || animmesh->mColors[chan] == NULL)
        return luaL_argerror(L, 2, "out of range");
    if( i >= animmesh->mNumVertices)
        return luaL_argerror(L, 3, "out of range");
    return pushcolor4(L, &(animmesh->mColors[chan][i]), 0);
    }


static int TextureCoords(lua_State *L)
    {
    vector3_t *coords;
    vector2_t vec2;
    unsigned int ncomp;
    animmesh_t *animmesh = checkanimmesh(L, 1);
    ud_t *ud = userdata(animmesh);
    mesh_t *mesh = ud->mesh;
    unsigned int chan = checkindex(L, 2);
    unsigned int i = checkindex(L, 3);
    if( chan >= AI_MAX_NUMBER_OF_TEXTURECOORDS || animmesh->mTextureCoords[chan] == NULL)
        return luaL_argerror(L, 2, "out of range");
    if( i >= animmesh->mNumVertices)
        return luaL_argerror(L, 3, "out of range");
    ncomp = mesh->mNumUVComponents[chan]; /* no. of components */
    coords =  &(animmesh->mTextureCoords[chan][i]);
    if(ncomp == 3)
        return pushvector3(L, coords, 0);
    if(ncomp == 2)
        {
        vec2.x = coords->x;
        vec2.y = coords->y;
        return pushvector2(L, &vec2, 0);
        }
    if(ncomp == 1)
        {
        lua_pushnumber(L, coords->x);
        return 1;
        }
    return unexpected(L); /* 4 components are currently not supported */
    }




/*------------------------------------------------------------------------------*
 | Registration                                                                 |
 *------------------------------------------------------------------------------*/

static const struct luaL_Reg Methods[] = 
    {
        { "mesh", Mesh },
        { "has_positions", HasVertices },
        { "has_normals", HasNormals },
        { "has_tangents", HasTangents },
        { "has_colors", HasColors },
        { "has_texture_coords", HasTextureCoords },
//      { "num_vertices", NumVertices },
        { "position", Position },
        { "normal", Normal },
        { "tangent", Tangent },
        { "bitangent", Bitangent },
        { "color", Color },
        { "texture_coords", TextureCoords },
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


void moonassimp_open_animmesh(lua_State *L)
    {
    udata_define(L, ANIMMESH_MT, Methods, MetaMethods);
    luaL_setfuncs(L, Functions, 0);
    }

