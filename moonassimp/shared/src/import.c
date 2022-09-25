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

static int EnableVerboseLogging(lua_State *L)
    {
    aiEnableVerboseLogging(checkboolean(L, 1));
    return 0;
    }

static int IsExtensionSupported(lua_State *L)
    {
    const char *ext = luaL_checkstring(L, 1);
    lua_pushboolean(L, aiIsExtensionSupported(ext));
    return 1;
    }

static int GetExtensionList(lua_State *L)
    {
    struct aiString out;
    out.data[0]='\0';
    out.length = 0;
    aiGetExtensionList(&out);
    lua_pushstring(L, out.data);
    return 1;
    }

static int ImportFile(lua_State *L)
    {
    const char *file = luaL_checkstring(L, 1);
    unsigned int flags = checkpostprocessflags(L, 2);
    const scene_t* scene = aiImportFile(file, flags);

    if(!scene)
        return nilerrmsg(L);
    
    return newscene(L, (scene_t*)scene);
    }


static int ImportFileFromMemory(lua_State *L)
    {
    size_t len;
    const char *buffer = luaL_checklstring(L, 1, &len);
    unsigned int flags = checkpostprocessflags(L, 2);
    const char *hint = luaL_optstring(L, 3, NULL);
    const scene_t* scene = aiImportFileFromMemory(buffer, (unsigned int)len, flags, hint);

    if(!scene)
        return nilerrmsg(L);
    
    return newscene(L, (scene_t*)scene);
    }

static int ApplyPostProcessing(lua_State *L)
    {
    const scene_t *scene;
    const scene_t *orig = checkscene(L, 1);
    unsigned int flags = checkpostprocessflags(L, 2);
    
    scene = aiApplyPostProcessing(orig, flags);
    if(!scene)
        return nilerrmsg(L);

    return newscene(L, (scene_t*)scene);
    }


static int GetMemoryRequirements(lua_State *L)
    {
    struct aiMemoryInfo mem;
    const scene_t *scene = checkscene(L, 1);
    memset(&mem, 0, sizeof(mem));
    aiGetMemoryRequirements(scene, &mem);
    lua_newtable(L);
#define SET(what)   do { lua_pushinteger(L, mem.what); lua_setfield(L, -2, #what); } while(0)
    SET(textures);
    SET(materials);
    SET(meshes);
    SET(nodes);
    SET(animations);
    SET(cameras);
    SET(lights);
    SET(total);
#undef SET
    return 1;
    }


/*------------------------------------------------------------------------------*
 | Log stream                                                                   |
 *------------------------------------------------------------------------------*/


static int CallbackRef = LUA_NOREF;
static struct aiLogStream LogStream;
static struct aiLogStream *FileLogStream = NULL;

static void LogStreamCallback(const char* message, char* user)
    {
#define L ((lua_State*)user)
    if(pushvalue(L, CallbackRef) != LUA_TFUNCTION)
        { unexpected(L); return; }
    lua_pushstring(L, message);
    if(lua_pcall(L, 1, 0, 0) != LUA_OK)
        { unexpected(L); return; }
#undef L
    }

static int DetachUserLogStream(lua_State *L)
    {
    if(CallbackRef == LUA_NOREF) return 0; /* nothing to do */
    aiDetachLogStream(&LogStream);
    unreference(L, CallbackRef);    
    return 0;
    }

static int AttachUserLogStream(lua_State *L)
    {
    if(lua_type(L, 2) != LUA_TFUNCTION)
        return luaL_argerror(L, 2, "function expected");
    DetachUserLogStream(L); /* the previous attached one, if any */
    reference(L, CallbackRef, 2);
    LogStream.callback = LogStreamCallback;
    LogStream.user = (char*)L;
    aiAttachLogStream(&LogStream);
    return 0;
    }


static int Attach_log_stream(lua_State *L)
    {
    struct aiLogStream c;
    const char *filename = NULL;
    unsigned int t = checklogstream(L, 1);
    if(t==0)
        return AttachUserLogStream(L);
    if(t==aiDefaultLogStream_FILE)
        filename = luaL_checkstring(L, 2);

    c = aiGetPredefinedLogStream((enum aiDefaultLogStream)t, filename);
    if(!c.callback)
        return luaL_error(L, "cannot get predefined log stream");
    aiAttachLogStream(&c);

    if(t==aiDefaultLogStream_FILE)
        FileLogStream = &c;
    return 0;
    }

static int Detach_log_stream(lua_State *L)
    {
    struct aiLogStream c;
    unsigned int t = checklogstream(L, 1);

    if(t==0)
        return DetachUserLogStream(L);

    if(t==aiDefaultLogStream_FILE)
        {
        if(FileLogStream == NULL) return 0; /* nothing to do */
        aiDetachLogStream(FileLogStream);
        FileLogStream = NULL;
        return 0;
        }
    
    c = aiGetPredefinedLogStream((enum aiDefaultLogStream)t, NULL);
    if(!c.callback)
        return luaL_error(L, "cannot get predefined log stream");
    aiDetachLogStream(&c);
    return 0;
    }

static int Detach_all_log_streams(lua_State *L)
    {
    DetachUserLogStream(L);
    aiDetachAllLogStreams();
    FileLogStream = NULL;
    return 0;
    }


/*------------------------------------------------------------------------------*
 | Registration                                                                 |
 *------------------------------------------------------------------------------*/

static const struct luaL_Reg Functions[] = 
    {
        { "enable_verbose_logging", EnableVerboseLogging },
        { "is_extension_supported", IsExtensionSupported },
        { "get_extension_list", GetExtensionList },
        { "import_file", ImportFile },
        { "import_file_from_memory", ImportFileFromMemory },
        { "apply_post_processing", ApplyPostProcessing },
        { "get_memory_requirements", GetMemoryRequirements },
        { "attach_log_stream", Attach_log_stream },
        { "detach_log_stream", Detach_log_stream },
        { "detach_all_log_streams", Detach_all_log_streams },
        { NULL, NULL } /* sentinel */
    };

void moonassimp_open_import(lua_State *L)
    {
    luaL_setfuncs(L, Functions, 0);
    }





#if(0) 
//@@ USED INTERNALLY:
ASSIMP_API void aiReleaseImport(const C_STRUCT aiScene* pScene);
ASSIMP_API const char* aiGetErrorString(); 

//@@ NOT SUPPORTED:
// Import with custom I/O functions: ----------------------------------------------------
ASSIMP_API const C_STRUCT aiScene* aiImportFileEx(const char* pFile, unsigned int pFlags,
    C_STRUCT aiFileIO* pFS);
// Import with property settings: -------------------------------------------------------
struct aiPropertyStore { char sentinel; };
ASSIMP_API const C_STRUCT aiScene* aiImportFileExWithProperties( const char* pFile, 
    unsigned int pFlags,    C_STRUCT aiFileIO* pFS, const C_STRUCT aiPropertyStore* pProps);
ASSIMP_API const C_STRUCT aiScene* aiImportFileFromMemoryWithProperties( 
    const char* pBuffer, unsigned int pLength,  unsigned int pFlags,
    const char* pHint,  const C_STRUCT aiPropertyStore* pProps);
ASSIMP_API C_STRUCT aiPropertyStore* aiCreatePropertyStore(void);
ASSIMP_API void aiReleasePropertyStore(C_STRUCT aiPropertyStore* p);
ASSIMP_API void aiSetImportPropertyInteger(C_STRUCT aiPropertyStore* store,
    const char* szName, int value);
ASSIMP_API void aiSetImportPropertyFloat(C_STRUCT aiPropertyStore* store, 
    const char* szName, float value);
ASSIMP_API void aiSetImportPropertyString(C_STRUCT aiPropertyStore* store,
    const char* szName, const C_STRUCT aiString* st);

// Math utilities: -----------------------------------------------------------------------
ASSIMP_API void aiCreateQuaternionFromMatrix(C_STRUCT aiQuaternion* quat,
    const C_STRUCT aiMatrix3x3* mat);
ASSIMP_API void aiDecomposeMatrix(const C_STRUCT aiMatrix4x4* mat,
    C_STRUCT aiVector3D* scaling, C_STRUCT aiQuaternion* rotation,
    C_STRUCT aiVector3D* position);
ASSIMP_API void aiTransposeMatrix4(C_STRUCT aiMatrix4x4* mat);
ASSIMP_API void aiTransposeMatrix3(C_STRUCT aiMatrix3x3* mat);
ASSIMP_API void aiTransformVecByMatrix3(C_STRUCT aiVector3D* vec, const C_STRUCT aiMatrix3x3* mat);
ASSIMP_API void aiTransformVecByMatrix4(C_STRUCT aiVector3D* vec, const C_STRUCT aiMatrix4x4* mat);
ASSIMP_API void aiMultiplyMatrix4(C_STRUCT aiMatrix4x4* dst, const C_STRUCT aiMatrix4x4* src);
ASSIMP_API void aiMultiplyMatrix3(C_STRUCT aiMatrix3x3* dst, const C_STRUCT aiMatrix3x3* src);
ASSIMP_API void aiIdentityMatrix3(C_STRUCT aiMatrix3x3* mat);
ASSIMP_API void aiIdentityMatrix4(C_STRUCT aiMatrix4x4* mat);

#endif
