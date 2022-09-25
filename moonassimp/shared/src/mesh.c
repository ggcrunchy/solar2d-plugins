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

int newmesh(lua_State *L, scene_t *scene, mesh_t *mesh)
    {
    unsigned int i;
    ud_t *ud;
    TRACE_CREATE(mesh, "mesh");
    if(mesh->mFaces != NULL && mesh->mNumFaces > 0)
        {
        for(i = 0; i < mesh->mNumFaces; i++)
            { newface(L, scene, mesh, &(mesh->mFaces[i])); lua_pop(L, 1); }
        TRACE_CREATE_N(i, "faces");
        }
    if(mesh->mBones != NULL && mesh->mNumBones > 0)
        {
        for(i = 0; i < mesh->mNumBones; i++)
            { newbone(L, scene, mesh, mesh->mBones[i]); lua_pop(L, 1); }
        TRACE_CREATE_N(i, "bones");
        }
    if(mesh->mAnimMeshes != NULL && mesh->mNumAnimMeshes > 0)
        {
        for(i = 0; i < mesh->mNumAnimMeshes; i++)
            { newanimmesh(L, scene, mesh, mesh->mAnimMeshes[i]); lua_pop(L, 1); }
        TRACE_CREATE_N(i, "animmeshes");
        }
    ud = newuserdata(L, (void*)mesh, MESH_MT);
    ud->scene = scene;
    return 1;   
    }

int freemesh(lua_State *L, mesh_t *mesh)
    {
    unsigned int i;
    if(mesh->mFaces != NULL && mesh->mNumFaces > 0)
        {
        for(i = 0; i < mesh->mNumFaces; i++)
            freeface(L, &(mesh->mFaces[i]));
        TRACE_DELETE_N(i, "faces");
        }
    if(mesh->mBones != NULL && mesh->mNumBones > 0)
        {
        for(i = 0; i < mesh->mNumBones; i++)
            freebone(L, mesh->mBones[i]);
        TRACE_DELETE_N(i, "bones");
        }
    if(mesh->mAnimMeshes != NULL && mesh->mNumAnimMeshes > 0)
        {
        for(i = 0; i < mesh->mNumAnimMeshes; i++)
            freeanimmesh(L, mesh->mAnimMeshes[i]);
        TRACE_DELETE_N(i, "animmeshes");
        }
    TRACE_DELETE(mesh, "mesh");
    freeuserdata(L, mesh);
    return 0;
    }

static int Name(lua_State *L)
    {
    mesh_t *mesh = checkmesh(L, 1);
    if(mesh->mName.length == 0)
        return 0;
    lua_pushstring(L, mesh->mName.data);
    return 1;
    }

static int PrimitiveTypes(lua_State *L)
    {
    mesh_t *mesh = checkmesh(L, 1);
    return pushprimitivetype(L, mesh->mPrimitiveTypes, 1);
    }


#define F(what)                                 \
static int Has##what(lua_State *L)              \
    {                                           \
    mesh_t *mesh = checkmesh(L, 1);             \
    lua_pushboolean(L, mesh->m##what != NULL && mesh->mNum##what > 0);  \
    return 1;                                   \
    }                                           \
static int Num##what(lua_State *L)              \
    {                                           \
    mesh_t *mesh = checkmesh(L, 1);             \
    lua_pushinteger(L, mesh->mNum##what);       \
    return 1;                                   \
    }

F(Vertices)
F(Faces)
F(Bones)
F(AnimMeshes)

#undef F

static int HasNormals(lua_State *L)
    {
    mesh_t *mesh = checkmesh(L, 1);
    lua_pushboolean(L, mesh->mNormals != NULL && mesh->mNumVertices > 0);
    return 1;
    }

static int HasTangents(lua_State *L) /* and bitangents */
    {
    mesh_t *mesh = checkmesh(L, 1);
    lua_pushboolean(L, mesh->mTangents != NULL && mesh->mBitangents != NULL &&
                        mesh->mNumVertices > 0);
    return 1;
    }

static int Material(lua_State *L)
    {
#define scene ud->scene
    mesh_t *mesh = checkmesh(L, 1);
    ud_t *ud = userdata(mesh);
    if(mesh->mMaterialIndex >= scene->mNumMaterials)
        return unexpected(L);   
    pushmaterial(L, scene->mMaterials[mesh->mMaterialIndex]);
    return 1;
#undef scene
    }


#define F(func, what)                                       \
static int func(lua_State *L)                               \
    {                                                       \
    mesh_t *mesh = checkmesh(L, 1);                         \
    unsigned int i = checkindex(L, 2);                      \
    if( mesh->m##what == NULL || i >= mesh->mNumVertices )  \
        return luaL_argerror(L, 2, "out of range");         \
    return pushvector3(L, &(mesh->m##what[i]), 0);          \
    }

F(Position, Vertices)
F(Normal, Normals)
F(Tangent, Tangents)
F(Bitangent, Bitangents)

#undef F


#define F(func, what)                                       \
static int func(lua_State *L)                               \
    {                                                       \
    mesh_t *mesh = checkmesh(L, 1);                         \
    unsigned int i;                                         \
    if(mesh->m##what == NULL)                               \
        return 0;                                           \
    lua_newtable(L);                                        \
    for(i=0; i < mesh->mNumVertices; i++)                   \
        {                                                   \
        pushvector3(L, &(mesh->m##what[i]), 1);             \
        lua_rawseti(L, -2, i+1);                            \
        }                                                   \
    return 1;                                               \
    }

F(AllPositions, Vertices)
F(AllNormals, Normals)
F(AllTangents, Tangents)
F(AllBitangents, Bitangents)

#undef F


static int HasColors(lua_State *L)
    {
    mesh_t *mesh = checkmesh(L, 1);
    unsigned int i = checkindex(L, 2);
    if( i >= AI_MAX_NUMBER_OF_COLOR_SETS)
        lua_pushboolean(L, 0);
    else
        lua_pushboolean(L, mesh->mColors[i] != NULL && mesh->mNumVertices > 0);
    return 1;
    }

static int NumColorChannels(lua_State *L)
    {
    unsigned int n = 0;
    mesh_t *mesh = checkmesh(L, 1);
    while (n < AI_MAX_NUMBER_OF_COLOR_SETS && mesh->mColors[n]) ++n;
    lua_pushinteger(L, n);
    return 1;
    }

static int Color(lua_State *L)
    {
    mesh_t *mesh = checkmesh(L, 1);
    unsigned int chan = checkindex(L, 2);
    unsigned int i = checkindex(L, 3);
    if( chan >= AI_MAX_NUMBER_OF_COLOR_SETS || mesh->mColors[chan] == NULL)
        return luaL_argerror(L, 2, "out of range");
    if( i >= mesh->mNumVertices)
        return luaL_argerror(L, 3, "out of range");
    return pushcolor4(L, &(mesh->mColors[chan][i]), 0);
    }

static int AllColors(lua_State *L)
    {
    mesh_t *mesh = checkmesh(L, 1);
    unsigned int chan;
    unsigned int i;
    lua_newtable(L);
    for(chan = 0; chan < AI_MAX_NUMBER_OF_COLOR_SETS; chan++)
        {
        if(mesh->mColors[chan] == NULL) break;
        lua_newtable(L);
        for(i=0; i < mesh->mNumVertices; i++)
            {
            pushcolor4(L, &(mesh->mColors[chan][i]), 1);
            lua_rawseti(L, -2, i+1);
            }
        lua_rawseti(L, -2, chan+1);
        }
    return 1;
    }


static int HasTextureCoords(lua_State *L)
    {
    mesh_t *mesh = checkmesh(L, 1);
    unsigned int i = checkindex(L, 2);
    if( i >= AI_MAX_NUMBER_OF_TEXTURECOORDS)
        lua_pushboolean(L, 0);
    else
        lua_pushboolean(L, mesh->mTextureCoords[i] != NULL && mesh->mNumVertices > 0);
    return 1;
    }

static int NumTextureCoordsChannels(lua_State *L)
    {
    unsigned int n = 0;
    mesh_t *mesh = checkmesh(L, 1);
    while (n < AI_MAX_NUMBER_OF_TEXTURECOORDS && mesh->mTextureCoords[n]) ++n;
    lua_pushinteger(L, n);
    return 1;
    }

static int NumTextureCoordsComponents(lua_State *L)
    {
    mesh_t *mesh = checkmesh(L, 1);
    unsigned int chan = checkindex(L, 2);
    if( chan >= AI_MAX_NUMBER_OF_TEXTURECOORDS || mesh->mTextureCoords[chan] == NULL)
        return luaL_argerror(L, 2, "out of range");
    lua_pushinteger(L, mesh->mNumUVComponents[chan]);
    return 1;
    }


static int TextureCoords(lua_State *L)
    {
    vector3_t *coords;
    vector2_t vec2;
    unsigned int ncomp;
    mesh_t *mesh = checkmesh(L, 1);
    unsigned int chan = checkindex(L, 2);
    unsigned int i = checkindex(L, 3);
    if( chan >= AI_MAX_NUMBER_OF_TEXTURECOORDS || mesh->mTextureCoords[chan] == NULL)
        return luaL_argerror(L, 2, "out of range");
    if( i >= mesh->mNumVertices)
        return luaL_argerror(L, 3, "out of range");
    ncomp = mesh->mNumUVComponents[chan]; /* no. of components */
    coords =  &(mesh->mTextureCoords[chan][i]);
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


static int AllTextureCoords(lua_State *L)
    {
    vector3_t *coords;
    vector2_t vec2;
    unsigned int ncomp;
    mesh_t *mesh = checkmesh(L, 1);
    unsigned int chan;
    unsigned int i;

    lua_newtable(L);
    for(chan = 0; chan < AI_MAX_NUMBER_OF_TEXTURECOORDS; chan++)
        {
        if(mesh->mTextureCoords[chan] == NULL) break;
        ncomp = mesh->mNumUVComponents[chan]; /* no. of components */
        lua_newtable(L);
        for(i=0; i<mesh->mNumVertices; i++)
            {
            coords =  &(mesh->mTextureCoords[chan][i]);
            switch(ncomp)
                {
                case 3: pushvector3(L, coords, 1); break;
                case 2: {
                        vec2.x = coords->x;
                        vec2.y = coords->y;
                        pushvector2(L, &vec2, 1);
                        break;
                        }
                case 1: {
                        lua_newtable(L);
                        lua_pushnumber(L, coords->x);
                        lua_rawseti(L, -2, 1);
                        break;
                        }
                default:
                    return unexpected(L); /* 4 components are currently not supported */
                }
            lua_rawseti(L, -2, i+1);
            }
        lua_rawseti(L, -2, chan+1);
        }
    return 1;
    }


static int Face(lua_State *L)
    {
    mesh_t *mesh = checkmesh(L, 1);
    unsigned int i = checkindex(L, 2);
    if(mesh->mFaces == NULL || mesh->mNumFaces == 0 || i >= mesh->mNumFaces)
        return luaL_argerror(L, 2, "out of range");
    return pushface(L, &(mesh->mFaces[i]));
    }


static int AllIndices(lua_State *L)
    {
    mesh_t *mesh = checkmesh(L, 1);
    int zero_based = optboolean(L, 2, 0);
    unsigned int i;

    lua_newtable(L);
    if(mesh->mFaces == NULL || mesh->mNumFaces == 0)
        return 1;
    for(i = 0; i < mesh->mNumFaces; i++)
        {
        pushfaceindices(L, &(mesh->mFaces[i]), zero_based);
        lua_rawseti(L, -2, i+1);
        }
    return 1;
    }

static int AnimMesh(lua_State *L)
    {
    mesh_t *mesh = checkmesh(L, 1);
    unsigned int i = checkindex(L, 2);
    if(mesh->mAnimMeshes == NULL || mesh->mNumAnimMeshes == 0 || i >= mesh->mNumAnimMeshes)
        return luaL_argerror(L, 2, "out of range");
    return pushanimmesh(L, mesh->mAnimMeshes[i]);
    }

static int Bone(lua_State *L)
    {
    mesh_t *mesh = checkmesh(L, 1);
    unsigned int i = checkindex(L, 2);
    if(mesh->mBones == NULL || mesh->mNumBones == 0 || i >= mesh->mNumBones)
        return luaL_argerror(L, 2, "out of range");
    return pushbone(L, mesh->mBones[i]);
    }

static int Faces(lua_State *L)
    {
    unsigned int i;
    mesh_t *mesh = checkmesh(L, 1);
    lua_newtable(L);
    if(mesh->mFaces == NULL || mesh->mNumFaces == 0) return 1;
    for(i = 0; i < mesh->mNumFaces; i++)
        {
        pushface(L, &(mesh->mFaces[i]));
        lua_rawseti(L, -2, i+1);
        }
    return 1;
    }

static int Bones(lua_State *L)
    {
    unsigned int i;
    mesh_t *mesh = checkmesh(L, 1);
    lua_newtable(L);
    if(mesh->mBones == NULL || mesh->mNumBones == 0) return 1;
    for(i = 0; i < mesh->mNumBones; i++)
        {
        pushbone(L, mesh->mBones[i]);
        lua_rawseti(L, -2, i+1);
        }
    return 1;
    }

static int AnimMeshes(lua_State *L)
    {
    unsigned int i;
    mesh_t *mesh = checkmesh(L, 1);
    lua_newtable(L);
    if(mesh->mAnimMeshes == NULL || mesh->mNumAnimMeshes == 0) return 1;
    for(i = 0; i < mesh->mNumAnimMeshes; i++)
        {
        pushanimmesh(L, mesh->mAnimMeshes[i]);
        lua_rawseti(L, -2, i+1);
        }
    return 1;
    }


/*------------------------------------------------------------------------------*
 | Registration                                                                 |
 *------------------------------------------------------------------------------*/

static const struct luaL_Reg Methods[] = 
    {
        { "name", Name },
        { "primitive_types", PrimitiveTypes },
        { "has_positions", HasVertices },
        { "has_normals", HasNormals },
        { "num_vertices", NumVertices },
        { "has_tangents", HasTangents },
        { "has_colors", HasColors },
        { "has_texture_coords", HasTextureCoords },
        { "has_faces", HasFaces },
        { "num_faces", NumFaces },
        { "has_bones", HasBones },
        { "num_bones", NumBones },
        { "has_anim_meshes", HasAnimMeshes },
        { "num_anim_meshes", NumAnimMeshes },
        { "num_texture_coords_channels", NumTextureCoordsChannels }, /* aka. uv_hannels */
        { "num_texture_coords_components", NumTextureCoordsComponents },
        { "num_color_channels", NumColorChannels },
        { "position", Position },
        { "normal", Normal },
        { "tangent", Tangent  },
        { "bitangent", Bitangent },
        { "all_positions", AllPositions },
        { "all_normals", AllNormals },
        { "all_tangents", AllTangents  },
        { "all_bitangents", AllBitangents },
        { "color", Color },
        { "all_colors", AllColors },
        { "texture_coords", TextureCoords },
        { "all_texture_coords", AllTextureCoords },
        { "material", Material },
        { "face", Face },
        { "all_indices", AllIndices },
        { "bone", Bone },
        { "anim_mesh", AnimMesh },
        { "faces", Faces },
        { "bones", Bones },
        { "anim_meshes", AnimMeshes },
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


void moonassimp_open_mesh(lua_State *L)
    {
    udata_define(L, MESH_MT, Methods, MetaMethods);
    luaL_setfuncs(L, Functions, 0);
    }

