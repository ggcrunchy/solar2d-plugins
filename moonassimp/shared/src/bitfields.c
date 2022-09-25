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


/*----------------------------------------------------------------------*
 | aiTextureFlags                                                       |
 *----------------------------------------------------------------------*/

unsigned int checktextureflags(lua_State *L, int arg)
/* Accepts an integer or a list of strings starting from index=arg */
    {
    unsigned int flags = 0;
    const char *s;
    
    if(lua_type(L, arg) == LUA_TNUMBER)
        return (unsigned int)luaL_checkinteger(L, arg);

    while(lua_isstring(L, arg))
        {
        s = lua_tostring(L, arg++);
#define CASE(CODE,str) if((strcmp(s, str)==0)) do { flags |= CODE; goto done; } while(0)
    CASE(aiTextureFlags_Invert, "invert");
    CASE(aiTextureFlags_UseAlpha, "use alpha");
    CASE(aiTextureFlags_IgnoreAlpha, "ignore alpha");
#undef CASE
        return (unsigned int)luaL_argerror(L, --arg, badvalue(L,s));
        done: ;
        }
    return flags;
    }

int pushtextureflags(lua_State *L, unsigned int flags, int pushcode)
/* Pushes an integer followed by a list of strings */
    {
    int n = 0;
    if(pushcode)
        { lua_pushinteger(L, flags); return 1; }

#define CASE(CODE,str) do { if( flags & CODE) { lua_pushstring(L, str); n++; } } while(0)
    CASE(aiTextureFlags_Invert, "invert");
    CASE(aiTextureFlags_UseAlpha, "use alpha");
    CASE(aiTextureFlags_IgnoreAlpha, "ignore alpha");
#undef CASE
    if(flags==0) { lua_pushstring(L, "none"); n++; }
    return n;
    }


/*----------------------------------------------------------------------*
 | aiPrimitiveType                                                      |
 *----------------------------------------------------------------------*/

unsigned int checkprimitivetype(lua_State *L, int arg)
/* Accepts an integer or a list of strings starting from index=arg */
    {
    unsigned int flags = 0;
    const char *s;
    
    if(lua_type(L, arg) == LUA_TNUMBER)
        return (unsigned int)luaL_checkinteger(L, arg);

    while(lua_isstring(L, arg))
        {
        s = lua_tostring(L, arg++);
#define CASE(CODE,str) if((strcmp(s, str)==0)) do { flags |= CODE; goto done; } while(0)
    CASE(aiPrimitiveType_POINT, "point");
    CASE(aiPrimitiveType_LINE, "line");
    CASE(aiPrimitiveType_TRIANGLE, "triangle");
    CASE(aiPrimitiveType_POLYGON, "polygon");
#undef CASE
        return (unsigned int)luaL_argerror(L, --arg, badvalue(L,s));
        done: ;
        }
    return flags;
    }

int pushprimitivetype(lua_State *L, unsigned int flags, int pushcode)
/* Pushes an integer followed by a list of strings */
    {
    int n = 0;
    if(pushcode)
        { lua_pushinteger(L, flags); return 1; }

#define CASE(CODE,str) do { if( flags & CODE) { lua_pushstring(L, str); n++; } } while(0)
    CASE(aiPrimitiveType_POINT, "point");
    CASE(aiPrimitiveType_LINE, "line");
    CASE(aiPrimitiveType_TRIANGLE, "triangle");
    CASE(aiPrimitiveType_POLYGON, "polygon");
#undef CASE
    if(flags==0) { lua_pushstring(L, "none"); n++; }
    return n;
    }


/*----------------------------------------------------------------------*
 | Scene Flags                                                          |
 *----------------------------------------------------------------------*/

unsigned int checksceneflags(lua_State *L, int arg)
/* Accepts an integer or a list of strings starting from index=arg */
    {
    unsigned int flags = 0;
    const char *s;
    
    if(lua_type(L, arg) == LUA_TNUMBER)
        return (unsigned int)luaL_checkinteger(L, arg);

    while(lua_isstring(L, arg))
        {
        s = lua_tostring(L, arg++);
#define CASE(CODE,str) if((strcmp(s, str)==0)) do { flags |= CODE; goto done; } while(0)
        CASE(0, "none");
        CASE(AI_SCENE_FLAGS_INCOMPLETE, "incomplete");
        CASE(AI_SCENE_FLAGS_VALIDATED, "validated");
        CASE(AI_SCENE_FLAGS_VALIDATION_WARNING, "validation warning");
        CASE(AI_SCENE_FLAGS_NON_VERBOSE_FORMAT, "non verbose format");
        CASE(AI_SCENE_FLAGS_TERRAIN, "terrain");
#undef CASE
        return (unsigned int)luaL_argerror(L, --arg, badvalue(L,s));
        done: ;
        }
    return flags;
    }

int pushsceneflags(lua_State *L, unsigned int flags, int pushcode)
/* Pushes an integer followed by a list of strings */
    {
    int n = 0;
    if(pushcode)
        { lua_pushinteger(L, flags); return 1; }

#define CASE(CODE,str) do { if( flags & CODE) { lua_pushstring(L, str); n++; } } while(0)
        CASE(AI_SCENE_FLAGS_INCOMPLETE, "incomplete");
        CASE(AI_SCENE_FLAGS_VALIDATED, "validated");
        CASE(AI_SCENE_FLAGS_VALIDATION_WARNING, "validation warning");
        CASE(AI_SCENE_FLAGS_NON_VERBOSE_FORMAT, "non verbose format");
        CASE(AI_SCENE_FLAGS_TERRAIN, "terrain");
#undef CASE
    if(flags==0) { lua_pushstring(L, "none"); n++; }
    return n;
    }

/*----------------------------------------------------------------------*
 | PostProcessFlags                                                     |
 *----------------------------------------------------------------------*/


unsigned int checkpostprocessflags(lua_State *L, int arg)
/* Accepts an integer or a list of strings starting from index=arg */
    {
    unsigned int flags = 0;
    const char *s;
    
    if(lua_type(L, arg) == LUA_TNUMBER)
        return (unsigned int)luaL_checkinteger(L, arg);

    if(lua_isnone(L, arg))
        return luaL_argerror(L, arg, "missing flags");

    while(lua_isstring(L, arg))
        {
        s = lua_tostring(L, arg++);
#define CASE(CODE,str) if((strcmp(s, str)==0)) do { flags |= CODE; goto done; } while(0)
        CASE(0, "none");
        CASE(aiProcess_CalcTangentSpace,"calc tangent space");
        CASE(aiProcess_JoinIdenticalVertices,"join identical vertices");
        CASE(aiProcess_MakeLeftHanded,"make left handed");
        CASE(aiProcess_Triangulate,"triangulate");
        CASE(aiProcess_RemoveComponent,"remove component");
        CASE(aiProcess_GenNormals,"gen normals");
        CASE(aiProcess_GenSmoothNormals,"gen smooth normals");
        CASE(aiProcess_SplitLargeMeshes,"split large meshes");
        CASE(aiProcess_PreTransformVertices,"pre transform vertices");
        CASE(aiProcess_LimitBoneWeights,"limit bone weights");
        CASE(aiProcess_ValidateDataStructure,"validate data structure");
        CASE(aiProcess_ImproveCacheLocality,"improve cache locality");
        CASE(aiProcess_RemoveRedundantMaterials,"remove redundant materials");
        CASE(aiProcess_FixInfacingNormals,"fix infacing normals");
        CASE(aiProcess_SortByPType,"sort by p type");
        CASE(aiProcess_FindDegenerates,"find degenerates");
        CASE(aiProcess_FindInvalidData,"find invalid data");
        CASE(aiProcess_GenUVCoords,"gen uv coords");
        CASE(aiProcess_TransformUVCoords,"transform uv coords");
        CASE(aiProcess_FindInstances,"find instances");
        CASE(aiProcess_OptimizeMeshes ,"optimize meshes");
        CASE(aiProcess_OptimizeGraph ,"optimize graph");
        CASE(aiProcess_FlipUVs,"flip uvs");
        CASE(aiProcess_FlipWindingOrder ,"flip winding order");
        CASE(aiProcess_SplitByBoneCount ,"split by bone count");
        CASE(aiProcess_Debone ,"debone");
#undef CASE
        return (unsigned int)luaL_argerror(L, --arg, badvalue(L,s));
        done: ;
        }
    return flags;
    }

int pushpostprocessflags(lua_State *L, unsigned int flags, int pushcode)
/* Pushes an integer followed by a list of strings */
    {
    int n = 0;
    if(pushcode)
        { lua_pushinteger(L, flags); return 1; }

#define CASE(CODE,str) do { if( flags & CODE) { lua_pushstring(L, str); n++; } } while(0)
        CASE(aiProcess_CalcTangentSpace,"calc tangent space");
        CASE(aiProcess_JoinIdenticalVertices,"join identical vertices");
        CASE(aiProcess_MakeLeftHanded,"make left handed");
        CASE(aiProcess_Triangulate,"triangulate");
        CASE(aiProcess_RemoveComponent,"remove component");
        CASE(aiProcess_GenNormals,"gen normals");
        CASE(aiProcess_GenSmoothNormals,"gen smooth normals");
        CASE(aiProcess_SplitLargeMeshes,"split large meshes");
        CASE(aiProcess_PreTransformVertices,"pre transform vertices");
        CASE(aiProcess_LimitBoneWeights,"limit bone weights");
        CASE(aiProcess_ValidateDataStructure,"validate data structure");
        CASE(aiProcess_ImproveCacheLocality,"improve cache locality");
        CASE(aiProcess_RemoveRedundantMaterials,"remove redundant materials");
        CASE(aiProcess_FixInfacingNormals,"fix infacing normals");
        CASE(aiProcess_SortByPType,"sort by p type");
        CASE(aiProcess_FindDegenerates,"find degenerates");
        CASE(aiProcess_FindInvalidData,"find invalid data");
        CASE(aiProcess_GenUVCoords,"gen uv coords");
        CASE(aiProcess_TransformUVCoords,"transform uv coords");
        CASE(aiProcess_FindInstances,"find instances");
        CASE(aiProcess_OptimizeMeshes ,"optimize meshes");
        CASE(aiProcess_OptimizeGraph ,"optimize graph");
        CASE(aiProcess_FlipUVs,"flip uvs");
        CASE(aiProcess_FlipWindingOrder ,"flip winding order");
        CASE(aiProcess_SplitByBoneCount ,"split by bone count");
        CASE(aiProcess_Debone ,"debone");

#undef CASE
    if(flags==0) { lua_pushstring(L, "none"); n++; }
    return n;
    }

