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

int newnode(lua_State *L, scene_t *scene, node_t *node)
/* recursively creates the userdata for the node and its children */
    {
    ud_t *ud;
    unsigned int i;
    TRACE_CREATE(node, "node");
    for(i = 0; i < node->mNumChildren; i++)
        {
        newnode(L, scene, node->mChildren[i]);
        lua_pop(L, 1);
        }
    ud = newuserdata(L, (void*)node, NODE_MT);
    ud->scene = scene;
    return 1;   
    }

int freenode(lua_State *L, node_t *node)
/* recursively deletes the userdata for the node and its children */
    {
    unsigned int i;
    for(i = 0; i < node->mNumChildren; i++)
        freenode(L, node->mChildren[i]);
    TRACE_DELETE(node, "node");
    freeuserdata(L, node);
    return 0;
    }

static int Name(lua_State *L)
    {
    node_t *node = checknode(L, 1);
    if(node->mName.length == 0)
        return 0;
    lua_pushstring(L, node->mName.data);
    return 1;
    }


static int Parent(lua_State *L)
    {
    node_t *node = checknode(L, 1);
    if(node->mParent == NULL)
        return 0; /* root */
    pushnode(L, node->mParent);
    return 1;
    }

static int NumChildren(lua_State *L)
    {
    node_t *node = checknode(L, 1);
    lua_pushinteger(L, node->mNumChildren);
    return 1;
    }

static int Children(lua_State *L)
    {
    unsigned int i;
    node_t *node = checknode(L, 1);
    lua_newtable(L);
    for(i = 0; i < node->mNumChildren; i++)
        {
        pushnode(L, node->mChildren[i]);
        lua_rawseti(L, -2, i+1);
        }
    return 1;
    }

static int Child(lua_State *L)
    { 
    node_t *node = checknode(L, 1);
    unsigned int i = checkindex(L, 2);
    if(i >= node->mNumChildren)
        return luaL_argerror(L, 2, "out of range");
    return pushnode(L, node->mChildren[i]);
    }


static int NumMeshes(lua_State *L)
    {
    node_t *node = checknode(L, 1);
    lua_pushinteger(L, node->mNumMeshes);
    return 1;
    }

static int Meshes(lua_State *L)
    {
#define scene ud->scene
    unsigned int i, index;
    node_t *node = checknode(L, 1);
    ud_t *ud = userdata(node);
    lua_newtable(L);
    for(i = 0; i < node->mNumMeshes; i++)
        {
        index = node->mMeshes[i];
        if(index >= scene->mNumMeshes)
            return unexpected(L);   
        pushmesh(L, scene->mMeshes[index]);
        lua_rawseti(L, -2, i+1);
        }
    return 1;
#undef scene
    }

static int Mesh(lua_State *L)
    {
#define scene ud->scene
    unsigned int index;
    node_t *node = checknode(L, 1);
    unsigned int i = checkindex(L, 2);
    ud_t *ud = userdata(node);
    if(i >= node->mNumMeshes)
        return luaL_argerror(L, 2, "out of range");
    index = node->mMeshes[i];
    if(index >= scene->mNumMeshes)
        return unexpected(L);   
    return pushmesh(L, scene->mMeshes[index]);
#undef scene
    }

static int Transformation(lua_State *L)
    {
    node_t *node = checknode(L, 1);
    return pushmatrix4(L, &(node->mTransformation), 0);
    }


/*------------------------------------------------------------------------------*
 | Registration                                                                 |
 *------------------------------------------------------------------------------*/


static const struct luaL_Reg Methods[] = 
    {
        { "name", Name },
        { "num_children", NumChildren },
        { "num_meshes", NumMeshes },
        { "parent", Parent },
        { "child", Child },
        { "children", Children },
        { "mesh", Mesh },
        { "meshes", Meshes },
        { "transformation", Transformation },
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


void moonassimp_open_node(lua_State *L)
    {
    udata_define(L, NODE_MT, Methods, MetaMethods);
    luaL_setfuncs(L, Functions, 0);
    }

