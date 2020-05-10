/*
* Permission is hereby granted, free of charge, to any person obtaining
* a copy of this software and associated documentation files (the
* "Software"), to deal in the Software without restriction, including
* without limitation the rights to use, copy, modify, merge, publish,
* distribute, sublicense, and/or sell copies of the Software, and to
* permit persons to whom the Software is furnished to do so, subject to
* the following conditions:
*
* The above copyright notice and this permission notice shall be
* included in all copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*
* [ MIT license: http://www.opensource.org/licenses/mit-license.php ]
*/

#include "CoronaLua.h"
#include "ByteReader.h"
#include "utils/LuaEx.h"

#ifdef __ANDROID__
    #include <jni.h>
    #include <sys/types.h>
    #include <android/asset_manager.h>
    #include <android/asset_manager_jni.h>
#else
    #error "Not implemented on this platform"
#endif

static jfieldID sLuaThreadID;

// Adapted from JNLua source:
static jclass referenceclass (JNIEnv * env, const char * name)
{
    jclass clazz = env->FindClass(name);
    
    if (!clazz) return nullptr;

    return (jclass)env->NewGlobalRef(clazz);
}

static lua_State * getluathread (JNIEnv * env, jobject javastate)
{
    return (lua_State *) (uintptr_t) env->GetLongField(javastate, sLuaThreadID);
}

extern "C" {
    static AAssetManager * sManager{nullptr};
    static jobject sRef{nullptr};

    JNIEXPORT void JNICALL Java_plugin_AssetReader_LuaLoader_BindAssetManager (
        JNIEnv * env, jobject thiz, jobject asset_manager
    ) {
        sRef = env->NewGlobalRef(asset_manager);
        sManager = AAssetManager_fromJava(env, sRef);
    }

    JNIEXPORT jint JNICALL Java_plugin_AssetReader_LuaLoader_Read (
        JNIEnv * env, jobject thiz, jobject javastate
    ) {
        lua_State * L = getluathread(env, javastate);
        const char * filename = luaL_checkstring(L, 1);
        AAsset * asset = AAssetManager_open(sManager, filename, AASSET_MODE_BUFFER);
        
        if (asset)
        {
            lua_pushlstring(L, static_cast<const char *>(AAsset_getBuffer(asset)), AAsset_getLength(asset));   // filename, contents

            AAsset_close(asset);
        }
        
        else lua_pushnil(L);// filename, nil
        
        return 1;
    }
    
    #define PROXY_NAME "AssetReader.Proxy"
    
    struct Proxy {
        AAsset * mAsset{nullptr};
        size_t mLength, mChunk{0U};
        
        void Clear (void)
        {
            if (mAsset) AAsset_close(mAsset);
            
            mAsset = nullptr;
            mChunk = 0U;
        }
        
        ~Proxy (void)
        {
            Clear();
        }
    };

    static Proxy * GetProxy (lua_State * L, int arg = 1)
    {
        return LuaXS::CheckUD<Proxy>(L, arg, PROXY_NAME);
    }

    JNIEXPORT jint JNICALL Java_plugin_AssetReader_LuaLoader_NewProxy (
        JNIEnv * env, jobject thiz, jobject javastate
    ) {
        lua_State * L = getluathread(env, javastate);
        
        LuaXS::NewTyped<Proxy>(L); // ..., proxy
            
        LuaXS::AttachMethods(L, PROXY_NAME, [](lua_State * L) {
            luaL_Reg proxy_methods[] = {
                {
                    "Bind", [](lua_State * L)
                    {
                        Proxy * proxy = GetProxy(L);
                        const char * filename = luaL_checkstring(L, 2);

                        proxy->Clear();
                        
                        if (lua_istable(L, 3))
                        {
                            // TODO:
                            // mode = "random", "streaming"
                            // chunk_size
                        }
                        
                        AAsset * asset = AAssetManager_open(sManager, filename, AASSET_MODE_BUFFER);
                        
                        if (asset)
                        {
                            proxy->mAsset = asset;
                            proxy->mLength = AAsset_getLength(asset);
                        }
                        
                        return LuaXS::PushArgAndReturn(L, asset != nullptr);// proxy, filename[, opts], ok
                    }
                }, {
                    "Clear", [](lua_State * L)
                    {
                        GetProxy(L)->Clear();
                        
                        return 0;
                    }
                }, {
                    "__gc", LuaXS::TypedGC<Proxy>
                }, {
                    "GetRemainingLength", [](lua_State * L)
                    {
                        AAsset * asset = GetProxy(L)->mAsset;

                        return LuaXS::PushArgAndReturn(L, asset ? AAsset_getRemainingLength(asset) : 0U);
                    }
                },
                { nullptr, nullptr }
            };
            
            luaL_register(L, nullptr, proxy_methods);
            
            ByteReaderFunc * func = ByteReader::Register(L);
            
            func->mGetBytes = [](lua_State * L, ByteReader & reader, int arg, void *) {
                Proxy * proxy = GetProxy(L, arg);
                
                if (!proxy->mAsset)
                {
                    reader.mBytes = nullptr;
                    reader.mCount = 0U;
                    
                    return false;
                }
                
                if (proxy->mChunk)
                {
                    // TODO
                }
                
                else
                {
                    reader.mBytes = AAsset_getBuffer(proxy->mAsset);
                    reader.mCount = proxy->mLength;
                }
                
                return true;
            };
            
            lua_pushlightuserdata(L, func); // proxy, proxy_mt, func
            lua_setfield(L, -2, "__bytes"); // proxy, proxy_mt = { ..., __bytes = func }
        });
            
        return 1;
    }

    JNIEXPORT jint JNICALL JNI_OnLoad (JavaVM * vm, void *)
    {
        JNIEnv * env;
        
        jint result = vm->GetEnv(reinterpret_cast<void **>(&env), JNI_VERSION_1_6);
        
        if (result != JNI_OK) return JNI_VERSION_1_6;
        
        jclass luastate_class;
        
        if (!(luastate_class = referenceclass(env, "com/naef/jnlua/LuaState")) ||
            !(sLuaThreadID = env->GetFieldID(luastate_class, "luaThread", "J"))) return JNI_VERSION_1_6;
        
        return JNI_VERSION_1_6;
    }
}
