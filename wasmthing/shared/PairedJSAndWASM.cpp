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

#include "utils.h"
#include "jsutils.h"

//
//
//

template<bool = sizeof(JSValue) <= sizeof(lua_Number)>
struct Value {
    union ValueUnion {
        lua_Number n;
        JSValue v;
    };

    static bool IsValid (lua_State * L)
    {
        return lua_isnumber(L, -1);
    }

    static JSValue Get (lua_State * L)
    {
        ValueUnion u;

        u.n = lua_tonumber(L, -1);

        return u.v;
    }

    static void Push (lua_State * L, JSValue v)
    {
        ValueUnion u;

        u.v = v;

        lua_pushnumber(L, u.n); // ..., env, ..., v
    }
};

//
//
//

template<>
struct Value<false> {
    static bool IsValid (lua_State * L)
    {
        return lua_type(L, -1) == LUA_TUSERDATA && lua_objlen(L, -1) == sizeof(JSValue);
    }

    static JSValue Get (lua_State * L)
    {
        JSValue v;

        memcpy(&v, lua_touserdata(L, -1), sizeof(JSValue));

        return v;
    }

    static void Push (lua_State * L, JSValue v)
    {
        *New<JSValue>(L) = v; // ..., env, ..., v
    }
};

//
//
//

using ValueT = Value<>;

//
//
//

struct JsWasm {
    JSRuntime * mJSRuntime;
    JSContext * mJSContext;
    WASMPair mWASM;
    JSValue mModule;
    JSValue mMalloc;
    JSValue mFree;

    //
    //
    //

    void Destroy (lua_State * L, int arg = 1)
    {
        lua_getfenv(L, arg); // ..., jsw, ..., env

        for (lua_pushnil(L); lua_next(L, -2); lua_pop(L, 1))
        {
            if (ValueT::IsValid(L)) JS_FreeValue(mJSContext, ValueT::Get(L));
        }

        //
        //
        //

        if (mJSRuntime) JS_RunGC(mJSRuntime);

        // TODO: jobs stuff?

        if (mJSContext) JS_FreeContext(mJSContext);
        if (mJSRuntime) JS_FreeRuntime(mJSRuntime);

        // TODO: curl stuff?

        if (mWASM.mEnv) m3_FreeEnvironment(mWASM.mEnv);

        // TODO: loop stuff?

        //
        //
        //

        Reset();
    }

    //
    //
    //

    void Reset ()
    {
        mMalloc = mFree = JS_UNDEFINED;

        memset(this, 0, sizeof(*this));
    }

    //
    //
    //

    void AddBoostrap (JSValue global_obj)
    {
        // start bootstrap
        JSAtom bootstrap_ns_atom = JS_NewAtom(mJSContext, "__bootstrap");
        JSValue bootstrap_ns = JS_NewObjectProto(mJSContext, JS_NULL);

        JS_DupValue(mJSContext, bootstrap_ns);  // JS_SetProperty frees the value.
        JS_SetProperty(mJSContext, global_obj, bootstrap_ns_atom, bootstrap_ns);

        tjs__mod_wasm_init(mJSContext, bootstrap_ns);

        size_t wap_len;
        const char * wa_polyfill = GetWebAssemblyPolyfill(&wap_len);

        eval_buf(mJSContext, wa_polyfill, wap_len, "WebAssembly.js", JS_EVAL_TYPE_MODULE);

        // end bootstrap
        JS_DeleteProperty(mJSContext, global_obj, bootstrap_ns_atom, 0);
        JS_FreeAtom(mJSContext, bootstrap_ns_atom);
        JS_FreeValue(mJSContext, bootstrap_ns);
    }

    //
    //
    //

    void AddModuleStub (JSValue global_obj, const char * name, size_t name_len, const uint8_t * wasm_bytes, size_t wasm_len)
    {
        JSAtom lib_atom = JS_NewAtomLen(mJSContext, name, name_len);
        
        mModule = JS_NewObjectProto(mJSContext, JS_NULL);

        JSValue wasmBinary = JS_NewArrayBufferCopy(mJSContext, wasm_bytes, wasm_len);

        JS_DefinePropertyValueStr(mJSContext, mModule, "wasmBinary", wasmBinary, JS_PROP_C_W_E);
        JS_DefinePropertyValue(mJSContext, global_obj, lib_atom, mModule, JS_PROP_C_W_E);
        JS_FreeAtom(mJSContext, lib_atom);
    }

    //
    //
    //

    void Step ()
    {
       for(;;) {
            JSContext *ctx1;
            int err = JS_ExecutePendingJob(mJSRuntime, &ctx1);
            if (err <= 0) {
                if (err < 0) {
                    js_std_dump_error(ctx1);
                }
                break;
            }
        }
    }

    //
    //
    //

    bool FindFunc (lua_State * L, const char * name, JSValue & func, int arg = 1)
    {
        lua_getfenv(L, arg); // ..., jsw, ..., env
        lua_getfield(L, -1, name); // jsw, ..., env, func?

        bool ok = true;

        if (!ValueT::IsValid(L))
        {
            func = JS_GetPropertyStr(mJSContext, mModule, name);
            ok = JS_IsFunction(mJSContext, func);

            if (ok)
            {
                ValueT::Push(L, func); // ..., jsw, ..., env, func, v

                lua_setfield(L, -3, name); // ..., jsw, ..., env, func; env[k] = v
            }

            else
            {
                JS_FreeValue(mJSContext, func); // TODO: if exception...

                func = JS_UNDEFINED; // TODO: error info! / cleanup exception
            }
        }
        
        else func = ValueT::Get(L);

        lua_pop(L, 2); // ..., jsw, env, ...

        return ok;
    }

    //
    //
    //

    void PushValue (lua_State * L, JSValue v)
    {
        if (JS_IsNumber(v))
        {
            lua_Number n;
                        
            JS_ToFloat64(mJSContext, &n, v); // n.b. safe due to IsNumber()

            lua_pushnumber(L, n); // ..., n 
        }

        else if (JS_IsBool(v)) lua_pushboolean(L, JS_ToBool(mJSContext, v)); // ..., b

        else if (JS_IsString(v))
        {
            size_t len;
            const char * str = JS_ToCStringLen(mJSContext, &len, v);

            if (str)
            {
                lua_pushlstring(L, str, len); // ..., str

                JS_FreeCString(mJSContext, str);
            }

            else ; // TODO!
        }

        // TODO: if object, really want to wrap it up and ... something

        else lua_pushnil(L); // ..., nil
    }

    //
    //
    //

    static void Methods (lua_State * L)
    {
        luaL_Reg funcs[] = {
            {
                "__call", [](lua_State * L)
                {
                    JsWasm * jsw = Get<JsWasm>(L);
                    const char * name = luaL_checkstring(L, 2);

                    //
                    //
                    //

                    JSValue func;

                    if (!jsw->FindFunc(L, name, func))
                    {
                        lua_pushnil(L); // jsw, name, ...args..., env, bad_value, nil
                        lua_pushliteral(L, "ERROR!"); // TODO! :)

                        return 2;
                    }

                    //
                    //
                    //

                    const int kBeforeArgs = 2, kMaxArgs = 32;
                    JSValue argv[kMaxArgs];

                    int n = 0, nargs = lua_gettop(L) - kBeforeArgs, top = lua_gettop(L);
                    
                    luaL_argcheck(L, nargs <= kMaxArgs, 1, "Too many arguments to __call");
                    
                    //
                    //
                    //

                    for (int i = kBeforeArgs + 1, top = lua_gettop(L); i <= top; ++i)
                    {
                        JSValue v = JS_UNDEFINED;

                        switch (lua_type(L, i))
                        {
                        case LUA_TNUMBER:
                            v = JS_NewFloat64(jsw->mJSContext, lua_tonumber(L, i));
                            break;
                        case LUA_TBOOLEAN:
                            v = JS_NewBool(jsw->mJSContext, lua_toboolean(L, i));
                            break;
                        case LUA_TSTRING:
                            v = JS_NewStringLen(jsw->mJSContext, lua_tostring(L, i), lua_objlen(L, i));
                            break;
                        case LUA_TNIL:
                            v = JS_NULL;
                        case LUA_TUSERDATA:
                            // TODO! (would just be ArrayBuffer, but not sure how to extract
                            break;
                        case LUA_TTABLE:
                            lua_pushfstring(L, "Table argument #%i to __call: NYI!", i); // jsw, name, ...args..., err
                            break;
                        default:
                            lua_pushfstring(L, "Bad argument #%i to __call: `%s`", i, luaL_typename(L, i)); // jsw, name, ...args..., err
                        }
                        
                        if (JS_IsException(v))
                        {
                            lua_pushfstring(L, "Exception!"); // TODO!

                            JS_FreeValue(jsw->mJSContext, v);

                            break;
                        }

                        else if (JS_IsUndefined(v)) break;

                        else argv[n++] = v;
                    }
                    
                    //
                    //
                    //

                    JSValue result = JS_UNDEFINED;

                    if (n == nargs) result = JS_Call(jsw->mJSContext, func, JS_UNDEFINED, nargs, argv);


                    //
                    //
                    //
                    
                    for (int i = 0; i < n; ++i) JS_FreeValue(jsw->mJSContext, argv[i]);

                    //
                    //
                    //
                    
                    if (JS_IsException(result))
                    {
                        lua_pushnil(L); // jsw, name, ...args..., env, func, nil
                        lua_pushliteral(L, "Exception!"); // TODO! :)

                        return 2;
                    }

                    //
                    //
                    //
                    
                    jsw->PushValue(L, result); // jsw, name, ...args..., env, func, result
                    
                    //
                    //
                    //

                    JS_FreeValue(jsw->mJSContext, result);

                    //
                    //
                    //
                    
                    return 1;
                }
            }, {
                "CopyToLinearMemory", [](lua_State * L)
                {
                    JsWasm * jsw = Get<JsWasm>(L);
                    
                    luaL_argcheck(L, lua_type(L, 2) == LUA_TUSERDATA || lua_type(L, 2) == LUA_TSTRING, 2, "Expected string or full userdata");

                    if (!JS_IsFunction(jsw->mJSContext, jsw->mMalloc)) luaL_error(L, "No `malloc` available");

                    JSValue alloc_size = JS_NewUint32(jsw->mJSContext, lua_objlen(L, 2));
                    JSValue result = JS_Call(jsw->mJSContext, jsw->mMalloc, JS_UNDEFINED, 1, &alloc_size);

                    if (!JS_IsException(result)) // alloc assumed to have succeeded, so memory valid
                    {
                        uint8_t * bytes = m3_GetMemory(jsw->mWASM.mRuntime, nullptr, 0);
                        int32_t offset;

                        JS_ToInt32(jsw->mJSContext, &offset, result);

                        memcpy(bytes + offset, lua_type(L, 2) == LUA_TUSERDATA ? lua_touserdata(L, 2) : lua_tostring(L, 2), lua_objlen(L, 2));

                        lua_pushinteger(L, offset); // jsw, size, offset
                    }

                    else
                    {
                        js_std_dump_error(jsw->mJSContext);

                        lua_pushnil(L); // jsw, size, nil
                    }

                    return 1;
                }
            }, {
                "CopyToLinearMemoryDirect", [](lua_State * L)
                {
                    JsWasm * jsw = Get<JsWasm>(L);
                    
                    luaL_argcheck(L, lua_type(L, 2) == LUA_TUSERDATA || lua_type(L, 2) == LUA_TSTRING, 2, "Expected string or full userdata");
                    
                    uint32_t size;
                    uint8_t * bytes = m3_GetMemory(jsw->mWASM.mRuntime, &size, 0);
                    size_t len = lua_objlen(L, 2);

                    int offset = luaL_checkint(L, 3);

                    luaL_argcheck(L, offset >= 0 && uint32_t(offset + len) <= size, 3, "Copy not contained to linear memory");

                    memcpy(bytes + offset, lua_type(L, 2) == LUA_TUSERDATA ? lua_touserdata(L, 2) : lua_tostring(L, 2), len);

                    return 0;
                }
            }, {
                "__gc", [](lua_State * L)
                {
                    JsWasm * jsw = Get<JsWasm>(L);
                    /*
                    lua_getglobal(L, "audio");
                    lua_getfield(L, -1, "endStuff");
                    lua_call(L, 0, 0);*/

                    jsw->Destroy(L);

                    return 0;
                }
            }, {
                "GetValue", [](lua_State * L)
                {
                    JsWasm * jsw = Get<JsWasm>(L);
                    JSValue v = JS_GetPropertyStr(jsw->mJSContext, jsw->mModule, lua_tostring(L, 2));

                    jsw->PushValue(L, v); // jsw, key, v

                    JS_FreeValue(jsw->mJSContext, v);

                    return 1;
                }
            }, {
                "HasValue", [](lua_State * L)
                {
                    JsWasm * jsw = Get<JsWasm>(L);
                    JSValue v = JS_GetPropertyStr(jsw->mJSContext, jsw->mModule, lua_tostring(L, 2));

                    lua_pushboolean(L, !JS_IsUndefined(v)); // jsw, key, defined

                    JS_FreeValue(jsw->mJSContext, v);

                    return 1;
                }
            }, {
                "Step", [](lua_State * L)
                {
                    JsWasm * jsw = Get<JsWasm>(L);
                    
                    jsw->Step();

                    return 0;
                }
            }, {
                "_INTERLEAVE_", [](lua_State * L)
                {
                    JsWasm * jsw = Get<JsWasm>(L);
                    int buf = luaL_checkint(L, 2), n = luaL_checkint(L, 3), max_samples = luaL_checkint(L, 4);

                    uint32_t size;
                    uint8_t * bytes = m3_GetMemory(jsw->mWASM.mRuntime, &size, 0);

                    luaL_argcheck(L, buf >= 0 && size_t(buf) < size, 2, "Invalid buf");
                    luaL_argcheck(L, buf + 2 * max_samples * sizeof(float) < size, 2, "Invalid buf range");
                    luaL_argcheck(L, n <= max_samples, 4, "Too many samples");

                    // n.b. will read four bytes for float (left side of buffer), THEN write four bytes for the two int16s,
                    // so safe to do in-place. In the end we only use the left side for output.
                    int16_t * out = reinterpret_cast<int16_t *>(bytes + buf);
                    const float * finput = reinterpret_cast<const float *>(out);

                    for (int i = 0; i < n; ++i)
                    {
                        out[i * 2 + 0] = static_cast<int16_t>(finput[i] * 32767.);
                        out[i * 2 + 1] = static_cast<int16_t>(finput[i + max_samples] * 32767.);
                    }

                    lua_pushlightuserdata(L, out);

                    return 1;
                }
            }, {
                "_PTR_", [](lua_State * L)
                {
                    JsWasm * jsw = Get<JsWasm>(L);
                    int offset = luaL_checkint(L, 2);

                    uint32_t size;
                    uint8_t * bytes = m3_GetMemory(jsw->mWASM.mRuntime, &size, 0);

                    luaL_argcheck(L, offset >= 0 && size_t(offset) < size, 2, "Invalid offset");

                    lua_pushlightuserdata(L, bytes + offset);

                    return 1;
                }
            },
            { nullptr, nullptr }
        };

        luaL_register(L, nullptr, funcs);
    }

    //
    //
    //

    static const char * MetatableName ()
    {
        return "JS_WASM_meta";
    }
};

//
//
//

int LoadJSWasmPair (lua_State * L)
{
    luaL_argcheck(L, lua_istable(L, 1), 1, "Non-table params");
    lua_getfield(L, 1, "jsFilename"); // params, filename
    luaL_argcheck(L, lua_isstring(L, -1), -1, "Non-string filename");

    //
    //
    //

    const char * js_file = lua_tostring(L, -1);
                
    // TODO: check ends in .js?
        // builds would work a little differently, if packed in .lu files...

    int len = int(lua_objlen(L, -1)), after_seps = GetIndexAfterLastSeparator(js_file, len);
    int ext_index = GetExtensionIndex(js_file, len);

    luaL_argcheck(L, ext_index >= 0, -1, "Filename has no extension");
    luaL_argcheck(L, ext_index > after_seps, -1, "Filename's extension includes separator(s)");
    lua_pushvalue(L, -1); // params, filename, filename

    LoadFile(L); // params, filename, js_file?

    if (lua_isnil(L, -1)) return Error(L, "Unable to load .js file");

    //
    //
    //

    lua_pushlstring(L, js_file, ext_index); // params, filename, js_file, filename_minus_extension (n.b. ext_index is 0-indexed)
    lua_pushliteral(L, ".wasm"); // params, filename, js_file, filename_minus_extension, ".wasm"
    lua_concat(L, 2); // params, filename, js_file, wasm_filename

    LoadFile(L); // params, filename, js_file, wasm_file?
    
    if (lua_isnil(L, -1)) return Error(L, "Unable to load .wasm file");

    //
    //
    //

    const char * js_bytes = lua_tostring(L, -2);
    const void * wasm_bytes = lua_tostring(L, -1);
    size_t js_len = lua_objlen(L, -2), wasm_len = lua_objlen(L, -1);

    //
    //
    //

    JsWasm * jsw = New<JsWasm>(L); // params, filename, js_file, wasm_file, jsw

    lua_newtable(L); // params, filename, js_file, wasm_file, jsw, env
    lua_setfenv(L, -2); // params, filename, js_file, wasm_file, jsw; jsw.environment = env

    jsw->Reset();

    //
    //
    //

    AddMethods(L, JsWasm::Methods, JsWasm::MetatableName());

    //
    //
    //

    jsw->mJSRuntime = JS_NewRuntime();

    if (!jsw->mJSRuntime) return Error(L, "Failed to create JS runtime");
                    
    jsw->mJSContext = JS_NewContext(jsw->mJSRuntime);
        // TODO ^^^ could be raw?

    if (!jsw->mJSContext) return Error(L, "Failed to create JS context");

    jsw->mWASM.mEnv = m3_NewEnvironment();

    if (!jsw->mWASM.mEnv) return Error(L, "Failed to create WASM environment");

    JS_SetContextOpaque(jsw->mJSContext, &jsw->mWASM);
/*
    JS_SetMemoryLimit(qrt->rt, options->mem_limit);
    JS_SetMaxStackSize(qrt->rt, options->stack_size);
*/
    // TODO: loop stuff?

    JS_SetModuleLoaderFunc(jsw->mJSRuntime, nullptr, nullptr, nullptr/*tjs_module_normalizer, tjs_module_loader, qrt*/);
/*
    JS_SetHostPromiseRejectionTracker(qrt->rt, tjs__promise_rejection_tracker, NULL);
*/
    JSValue global_obj = JS_GetGlobalObject(jsw->mJSContext);

    js_std_add_helpers(jsw->mJSContext);

    jsw->AddBoostrap(global_obj);
    jsw->AddModuleStub(global_obj, js_file + after_seps, size_t(ext_index), Ptr<uint8_t>(wasm_bytes), wasm_len);

    // will pick up stub:
    eval_buf(jsw->mJSContext, js_bytes, js_len, js_file + after_seps, JS_EVAL_TYPE_GLOBAL);

    /* execute the pending jobs */
    jsw->Step();

    jsw->FindFunc(L, "_malloc", jsw->mMalloc, -1);
    jsw->FindFunc(L, "_free", jsw->mFree, -1);

    JS_FreeValue(jsw->mJSContext, global_obj);

    return 1;
}
