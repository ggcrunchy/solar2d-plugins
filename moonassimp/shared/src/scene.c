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

int newscene(lua_State *L, scene_t *scene)
/* recursively creates the userdata for the scene,
 * the root node and its children, and all other nested objects */
    {
    unsigned int i;
    TRACE_CREATE(scene, "scene");
    if(scene->mRootNode)
        {
        newnode(L, scene, scene->mRootNode);
        lua_pop(L, 1);
        }
#define New(what, newfunc) do {                     \
    for(i=0; i< scene->mNum##what; i++)             \
        { newfunc(L, scene, scene->m##what[i]); lua_pop(L, 1);  } } while(0)
    New(Meshes, newmesh);
    New(Materials, newmaterial);
    New(Lights, newlight);
    New(Textures, newtexture);
    New(Cameras, newcamera);
    New(Animations, newanimation);
#undef New
    newuserdata(L, (void*)scene, SCENE_MT);
    return 1;   
    }

static int Delete(lua_State *L)
    {
    unsigned int i;
    scene_t *scene = testscene(L, 1);
    if(!scene) return 0; /* already deleted */
    /* first, release all userdata */
#define Free(what, freefunc) do {                   \
    if(scene->mNum##what > 0)                       \
        {                                           \
        for(i=0; i < scene->mNum##what; i++)        \
            freefunc(L, scene->m##what[i]);         \
        }                                           \
} while(0)
    Free(Meshes, freemesh);
    Free(Materials, freematerial);
    Free(Lights, freelight);
    Free(Textures, freetexture);
    Free(Cameras, freecamera);
    Free(Animations, freeanimation);
#undef Free
    if(scene->mRootNode)
        freenode(L, scene->mRootNode);
    TRACE_DELETE(scene, "scene");
    freeuserdata(L, scene);
    /* finally, release the scene */
    aiReleaseImport(scene);
    return 0;
    }

#define F(what)                                 \
static int Has##what(lua_State *L)              \
    {                                           \
    scene_t *scene = checkscene(L, 1);          \
    lua_pushboolean(L, scene->m##what != NULL && scene->mNum##what > 0); \
    return 1;                                   \
    }                                           \
static int Num##what(lua_State *L)              \
    {                                           \
    scene_t *scene = checkscene(L, 1);          \
    lua_pushinteger(L, scene->mNum##what);      \
    return 1;                                   \
    }

F(Meshes)
F(Materials)
F(Lights)
F(Textures)
F(Cameras)
F(Animations)

#undef F

static int RootNode(lua_State *L)
    {
    scene_t *scene = checkscene(L, 1);
    if(scene->mRootNode == NULL)
        return 0;
    pushnode(L, scene->mRootNode);
    return 1;
    }

static int Flags(lua_State *L)
    {
    scene_t *scene = checkscene(L, 1);
    return pushsceneflags(L, scene->mFlags, 1);
    }

static int Mesh(lua_State *L)
    {
    scene_t *scene = checkscene(L, 1);
    unsigned int i = checkindex(L, 2); /* 1-based */
    if(i > scene->mNumMeshes)
        return luaL_argerror(L, 2, "out of range");
    pushmesh(L, scene->mMeshes[i]);
    return 1;
    }

static int Meshes(lua_State *L)
    {
    unsigned int i;
    scene_t *scene = checkscene(L, 1);
    lua_newtable(L);
    for(i = 0; i < scene->mNumMeshes; i++)
        {
        pushmesh(L, scene->mMeshes[i]);
        lua_rawseti(L, -2, i+1);
        }
    return 1;
    }

#define F(what, pushfunc)                           \
static int what(lua_State *L)                       \
    {                                               \
    scene_t *scene = checkscene(L, 1);              \
    unsigned int i = checkindex(L, 2);              \
    if(i > scene->mNum##what##s)                    \
        return luaL_argerror(L, 2, "out of range"); \
    pushfunc(L, scene->m##what##s[i]);              \
    return 1;                                       \
    }                                               \
                                                    \
static int what##s(lua_State *L)                    \
    {                                               \
    unsigned int i;                                 \
    scene_t *scene = checkscene(L, 1);              \
    lua_newtable(L);                                \
    for(i = 0; i < scene->mNum##what##s; i++)       \
        {                                           \
        pushfunc(L, scene->m##what##s[i]);          \
        lua_rawseti(L, -2, i+1);                    \
        }                                           \
    return 1;                                       \
    }

F(Material, pushmaterial)
F(Light, pushlight)
F(Texture, pushtexture)
F(Camera, pushcamera)
F(Animation, pushanimation)

#undef F



/*------------------------------------------------------------------------------*
 | Registration                                                                 |
 *------------------------------------------------------------------------------*/

static const struct luaL_Reg Methods[] = 
    {
        { "flags", Flags },
        { "has_animations", HasAnimations },
        { "has_cameras", HasCameras },
        { "has_lights", HasLights },
        { "has_materials", HasMaterials },
        { "has_meshes", HasMeshes },
        { "has_textures", HasTextures },
        { "num_animations", NumAnimations },
        { "num_cameras", NumCameras },
        { "num_lights", NumLights },
        { "num_materials", NumMaterials },
        { "num_meshes", NumMeshes },
        { "num_textures", NumTextures },
        { "animation", Animation },
        { "animations", Animations },
        { "camera", Camera },
        { "cameras", Cameras },
        { "light", Light },
        { "lights", Lights },
        { "mesh", Mesh },
        { "meshes", Meshes },
        { "root_node", RootNode },
        { "material", Material },
        { "materials", Materials },
        { "texture", Texture },
        { "textures", Textures },
        { NULL, NULL } /* sentinel */
    };


static const struct luaL_Reg MetaMethods[] = 
    {
        { "__gc",  Delete },
        { NULL, NULL } /* sentinel */
    };

static const struct luaL_Reg Functions[] = 
    {
        { "release_import",  Delete }, //@@DOC
        { NULL, NULL } /* sentinel */
    };


void moonassimp_open_scene(lua_State *L)
    {
    udata_define(L, SCENE_MT, Methods, MetaMethods);
    luaL_setfuncs(L, Functions, 0);
    }


