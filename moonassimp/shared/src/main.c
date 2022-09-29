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

static void AddVersions(lua_State *L)
/* Add version strings to the ai table */
    {
    lua_pushstring(L, "_VERSION");
    lua_pushstring(L, "MoonAssimp "MOONASSIMP_VERSION);
    lua_settable(L, -3);

    lua_pushstring(L, "_ASSIMP_VERSION");
    lua_pushfstring(L, "Assimp %d.%d.%d", 
            aiGetVersionMajor(),
            aiGetVersionMinor(),
            aiGetVersionRevision());
    lua_settable(L, -3);
    }

static int GetVersion(lua_State *L)
    {
    lua_pushinteger(L, aiGetVersionMajor());
    lua_pushinteger(L, aiGetVersionMinor());
    lua_pushinteger(L, aiGetVersionRevision());
    return 3;
    }

static int GetLegalString(lua_State *L)
    {
    lua_pushstring(L, aiGetLegalString());
    return 1;
    }

static int GetCompileFlags(lua_State *L)
    {
    int n = 0;
    unsigned int flags = aiGetCompileFlags();
#define ADD(F, f) if(flags & ASSIMP_CFLAGS_##F) { lua_pushstring(L, f); n++; }
    ADD(SHARED, "shared")
    ADD(STLPORT, "stlport")
    ADD(DEBUG, "debug")
    ADD(NOBOOST, "noboost")
    ADD(SINGLETHREADED, "singlethreaded")
#undef ADD
    return n;
    }

static int AddConstants(lua_State *L)
    {
#define ADD(c) do {  /* ai.XXXXX constants for AI_XXXXX defines */  \
    lua_pushinteger(L, AI_##c); lua_setfield(L, -2, #c); } while(0)
    /* limits */
    ADD(MAX_FACE_INDICES);
    ADD(MAX_BONE_WEIGHTS);
    ADD(MAX_VERTICES);
    ADD(MAX_FACES);
    ADD(MAX_NUMBER_OF_COLOR_SETS);
    ADD(MAX_NUMBER_OF_TEXTURECOORDS);
    /* scene flags */
    ADD(SCENE_FLAGS_INCOMPLETE);
    ADD(SCENE_FLAGS_VALIDATED);
    ADD(SCENE_FLAGS_VALIDATION_WARNING);
    ADD(SCENE_FLAGS_NON_VERBOSE_FORMAT);
    ADD(SCENE_FLAGS_TERRAIN);
#undef ADD
#define ADD(c) do {  /* ai.Xxxx constants for aiXxxx enums */  \
    lua_pushinteger(L, ai##c); lua_setfield(L, -2, #c); } while(0)
    /* post process flags */
    ADD(Process_CalcTangentSpace);
    ADD(Process_JoinIdenticalVertices);
    ADD(Process_MakeLeftHanded);
    ADD(Process_Triangulate);
    ADD(Process_RemoveComponent);
    ADD(Process_GenNormals);
    ADD(Process_GenSmoothNormals);
    ADD(Process_SplitLargeMeshes);
    ADD(Process_PreTransformVertices);
    ADD(Process_LimitBoneWeights);
    ADD(Process_ValidateDataStructure);
    ADD(Process_ImproveCacheLocality);
    ADD(Process_RemoveRedundantMaterials);
    ADD(Process_FixInfacingNormals);
    ADD(Process_SortByPType);
    ADD(Process_FindDegenerates);
    ADD(Process_FindInvalidData);
    ADD(Process_GenUVCoords);
    ADD(Process_TransformUVCoords);
    ADD(Process_FindInstances);
    ADD(Process_OptimizeMeshes);
    ADD(Process_OptimizeGraph);
    ADD(Process_FlipUVs);
    ADD(Process_FlipWindingOrder);
    ADD(Process_SplitByBoneCount);
    ADD(Process_Debone);
    /* texture flags */
    ADD(TextureFlags_Invert);
    ADD(TextureFlags_UseAlpha);
    ADD(TextureFlags_IgnoreAlpha);
#undef ADD
    return 0;
    }

static int TraceObjects(lua_State *L)
    {
    trace_enabled = checkboolean(L, 1);
    return 0;
    }


/*------------------------------------------------------------------------------*
 | Registration                                                                 |
 *------------------------------------------------------------------------------*/

static void AtExit(void)
    {
    aiDetachAllLogStreams();
    }

static const struct luaL_Reg Functions[] = 
    {
        { "get_version", GetVersion },
        { "get_legal_string", GetLegalString },
        { "get_compile_flags", GetCompileFlags },
        { "trace_objects", TraceObjects },
        { NULL, NULL } /* sentinel */
    };


CORONA_EXPORT int luaopen_plugin_moonassimp(lua_State *L) // <- STEVE CHANGE
/* Lua calls this function to load the module */
    {
    lua_newtable(L); /* the ai table */
    AddVersions(L);
    AddConstants(L);
    atexit(AtExit);

    /* add assimp functions: */
    luaL_setfuncs(L, Functions, 0);
    moonassimp_open_import(L);
    moonassimp_open_scene(L);
    moonassimp_open_node(L);
    moonassimp_open_mesh(L);
    moonassimp_open_animmesh(L);
    moonassimp_open_material(L);
    moonassimp_open_animation(L);
    moonassimp_open_texture(L);
    moonassimp_open_light(L);
    moonassimp_open_camera(L);
    moonassimp_open_face(L);
    moonassimp_open_bone(L);
    moonassimp_open_nodeanim(L);
    moonassimp_open_meshanim(L);
    moonassimp_open_additional(L);

    return 1;
    }

