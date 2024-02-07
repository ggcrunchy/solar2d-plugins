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

static JSValue sP;

const uint32_t kSize = 65536;

static const char * sB;
static size_t sBlen;

//
//
//

struct JsWasm {
    JSRuntime * mJSRuntime;
    JSContext * mJSContext;
    WASMPair mWASM;
/*
    JSValue mGlobal;
    JSValue mMalloc;
    JSValue mFree;
*/

    //
    //
    //

    void Destroy ()
    {
        if (mJSRuntime) JS_RunGC(mJSRuntime);

        // TODO: jobs stuff?
        if (mJSContext) JS_FreeContext(mJSContext);
        if (mJSRuntime) JS_FreeRuntime(mJSRuntime);

        // TODO: curl stuff?

//        if (mWASMRuntime) m3_FreeRuntime(mWASMRuntime);
        if (mWASM.mEnv) m3_FreeEnvironment(mWASM.mEnv);

        // TODO: loop stuff?

        Reset();
    }

    //
    //
    //

    void Reset ()
    {
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
        JSValue stub = JS_NewObjectProto(mJSContext, JS_NULL);
        JSValue wasmBinary = JS_NewArrayBufferCopy(mJSContext, wasm_bytes, wasm_len);

        JS_DefinePropertyValueStr(mJSContext, stub, "wasmBinary", wasmBinary, JS_PROP_C_W_E);
        JS_DefinePropertyValue(mJSContext, global_obj, lib_atom, stub, JS_PROP_C_W_E);
        JS_FreeAtom(mJSContext, lib_atom);
    }

    //
    //
    //

    static void Methods (lua_State * L)
    {

        static bool sOK;
        static JSValue r, memsize, r2, r3, m, f, cfm, d, rs, src;
        static uint8_t * pp;
        static int32_t rres;















        luaL_Reg funcs[] = {
            {
                "__call", [](lua_State * L)
                {
                    // TODO: interpret args...
                        // intermediate memory for strings...
                        // int / float?
                        // userdata, function, thread not possible
                    return 0;
                }
            }, {
                "__gc", [](lua_State * L)
                {
                    JsWasm * jsw = Get<JsWasm>(L);

                    if (r3)
                    {
                                JSValue r4 = JS_Call(jsw->mJSContext, d, JS_UNDEFINED, 1, &r3);
            if (!JS_IsException(r4)) ;
            else js_std_dump_error(jsw->mJSContext);
            if (JS_VALUE_HAS_REF_COUNT(r4)) {
        JSRefCountHeader *p = (JSRefCountHeader *)JS_VALUE_GET_PTR(r4);
    }
                        JS_FreeValue(jsw->mJSContext, r4);
                            
                    }
            
                    if (r2)
                    {
            JSValue r5=JS_Call(jsw->mJSContext, f, JS_UNDEFINED, 1, &r2);
            if (!JS_IsException(r5)) ;
            else js_std_dump_error(jsw->mJSContext);
            if (JS_VALUE_HAS_REF_COUNT(r5)) {
        JSRefCountHeader *p = (JSRefCountHeader *)JS_VALUE_GET_PTR(r5);
    }
                        JS_FreeValue(jsw->mJSContext, r5);

                    }

                    if (r)
                    {
        JSValue r3=JS_Call(jsw->mJSContext, f, JS_UNDEFINED, 1, &r);
        if (!JS_IsException(r3))
        {
        } else js_std_dump_error(jsw->mJSContext);
            if (JS_VALUE_HAS_REF_COUNT(r3)) {
        JSRefCountHeader *p = (JSRefCountHeader *)JS_VALUE_GET_PTR(r3);
    }
                        JS_FreeValue(jsw->mJSContext, r3);
                    }

                    r = r2 = r3 = 0;
          
    JS_FreeValue(jsw->mJSContext, f);
    JS_FreeValue(jsw->mJSContext, m);
    JS_FreeValue(jsw->mJSContext, cfm);
    JS_FreeValue(jsw->mJSContext, d);
    JS_FreeValue(jsw->mJSContext, rs);
    JS_FreeValue(jsw->mJSContext, src);
                    /*
                    lua_getglobal(L, "audio");
                    lua_getfield(L, -1, "endStuff");
                    lua_call(L, 0, 0);*/

                    jsw->Destroy();

                    return 0;
                }
            }, {
                "Step", [](lua_State * L)
                {
                    // TODO: update jobs

                    return 0;
                }
            }, {

                "_START_", [](lua_State * L)
                {
                    JsWasm * jsw = Get<JsWasm>(L);
                    JSValue p = sP;
    uint32_t size;
    uint8_t * bytes = m3_GetMemory(jsw->mWASM.mRuntime, &size, 0);

    m = JS_GetPropertyStr(jsw->mJSContext, p, "_malloc");
    f = JS_GetPropertyStr(jsw->mJSContext, p, "_free");
    cfm = JS_GetPropertyStr(jsw->mJSContext, p, "_openmpt_module_create_from_memory");
    d = JS_GetPropertyStr(jsw->mJSContext, p, "_openmpt_module_destroy");
    rs = JS_GetPropertyStr(jsw->mJSContext, p, "_openmpt_module_read_interleaved_stereo");
    src = JS_GetPropertyStr(jsw->mJSContext, p, "_openmpt_module_set_repeat_count");

    memsize = JS_NewUint32(jsw->mJSContext, sBlen);
    r = JS_Call(jsw->mJSContext, m, JS_UNDEFINED, 1, &memsize);

    if (!JS_IsException(r))
    {
        int32_t res;
        if (!JS_ToInt32(jsw->mJSContext, &res, r)) ;
        else ;

        memcpy(bytes + res, sB, sBlen);

        JSValue args1[] = { r, memsize, JS_NewInt32(jsw->mJSContext, 0), JS_NewInt32(jsw->mJSContext, 0), JS_NewInt32(jsw->mJSContext, 0) };

        r3 = JS_Call(jsw->mJSContext, cfm, JS_UNDEFINED, _countof(args1), args1);

        if (!JS_IsException(r3))
        {

            JSValue memsize2 = JS_NewUint32(jsw->mJSContext, kSize);
            r2 = JS_Call(jsw->mJSContext, m, JS_UNDEFINED, 1, &memsize2);
                    
            if (!JS_ToInt32(jsw->mJSContext, &rres, r2)) ;
            else ;

            pp = bytes + rres;
        } else js_std_dump_error(jsw->mJSContext);


    } else js_std_dump_error(jsw->mJSContext);












                    return 0;
                }
            }, {
                "_GETDATA_", [](lua_State * L)
                {
                    JsWasm * jsw = Get<JsWasm>(L);

                JS_DupValue(jsw->mJSContext, r3); // just an integer, so not needed?

                const uint32_t kNumSamples = kSize / sizeof(int16_t);
                const uint32_t kFramesPerChannel = kNumSamples / 2;

                JSValue rsargs[] = { r3, JS_NewUint32(jsw->mJSContext, 44100), JS_NewUint32(jsw->mJSContext, kFramesPerChannel), r2 };//, JS_NewUint32(jsw->mJSContext, rres + kHalf) };
            //    (mModfile, (int)floor(mSamplerate), samples, aBuffer + outofs, aBuffer + outofs + aBufferSize);
		    //	if (res == 0)
                int32_t res;
                JSValue r6 = JS_Call(jsw->mJSContext, rs, JS_UNDEFINED, _countof(rsargs), rsargs);
                if (JS_IsException(r6)) js_std_dump_error(jsw->mJSContext);
                else if (!JS_ToInt32(jsw->mJSContext, &res, r6)) ;//CoronaLog("R3! %x", res);
                else CoronaLog("QQQ");


                    lua_pushlightuserdata(L, pp);
                    lua_pushinteger(L, res);


                    return 2;
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
    for(;;) {
            JSContext *ctx1;
            int err = JS_ExecutePendingJob(jsw->mJSRuntime, &ctx1);
        if (err <= 0) {
            if (err < 0) {
                js_std_dump_error(ctx1);
            }
            break;
        }
    }

    JSAtom lib_atom = JS_NewAtomLen(jsw->mJSContext, js_file + after_seps, size_t(ext_index));
    sP = JS_GetProperty(jsw->mJSContext, global_obj, lib_atom);

    JS_FreeAtom(jsw->mJSContext, lib_atom);
    uint32_t size;
    uint8_t * bytes = m3_GetMemory(jsw->mWASM.mRuntime, &size, 0);

    lua_pushliteral(L, "leaving_sanity.mod");
                
    LoadFile(L); // ...
    
    if (lua_isnil(L, -1)) return Error(L, "Unable to load mod");

    sB = lua_tostring(L, -1);
    sBlen = lua_objlen(L, -1);




    lua_ref(L, 1);//lua_pop(L, 1);

    JS_FreeValue(jsw->mJSContext, global_obj);

    return 1;
}
