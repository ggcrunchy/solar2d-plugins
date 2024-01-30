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
        JSValue stub = JS_NewObject(mJSContext);
        JSValue wasmBinary = JS_NewArrayBufferCopy(mJSContext, wasm_bytes, wasm_len);

        JS_SetPropertyStr(mJSContext, stub, "wasmBinary", wasmBinary);
        JS_SetProperty(mJSContext, global_obj, lib_atom, stub);
        JS_FreeAtom(mJSContext, lib_atom);
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

                    jsw->Destroy();

                    return 0;
                }
            }, {
                "Step", [](lua_State * L)
                {
                    // TODO: update jobs

                    return 0;
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

#if 0                 
    jsw->mWASMRuntime = m3_NewRuntime(jsw->mWASMEnv, 512 * 1024, nullptr/*&stuff*/);
        // stuff includes JSRuntime...

    if (!jsw->mWASMRuntime) return Error(L, "Failed to create WASM runtime");
#endif

    eval_buf(jsw->mJSContext, js_bytes, js_len, js_file + after_seps, JS_EVAL_TYPE_MODULE);
#if 0
        // after this, in theory, we can do calls, but might need _malloc, etc.
        JSAtom lib_atom = JS_NewAtomLen(jsw->mJSContext, js_file + after_seps, size_t(ext_index));
    JSValue p = JS_GetProperty(jsw->mJSContext, global_obj, lib_atom);
 CoronaLog("??? %i, %i", JS_VALUE_GET_TAG(p), JS_IsUndefined(p));
 JSValue f = JS_GetPropertyStr(jsw->mJSContext, p, "_malloc");
 CoronaLog("??? %i, %i", JS_VALUE_GET_TAG(f), JS_IsUndefined(f));
JSValue f2 = JS_GetPropertyStr(jsw->mJSContext, p, "_openmpt_module_get_duration_seconds");
 CoronaLog("??? %i, %i", JS_VALUE_GET_TAG(f2), JS_IsUndefined(f2));

    JS_FreeValue(jsw->mJSContext, p);
#else


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
    JSValue p = JS_GetProperty(jsw->mJSContext, global_obj, lib_atom);
    JSValue f2 = JS_GetPropertyStr(jsw->mJSContext, p, "_openmpt_get_library_version");
    CoronaLog("???? %i",  JS_IsUndefined(f2));
    if (!JS_IsUndefined(f2))
    {
        JSValue r = JS_Call(jsw->mJSContext, f2, JS_UNDEFINED, 0, nullptr);
        CoronaLog("!!! %i", JS_IsException(r));
        if (!JS_IsException(r))
        {CoronaLog("OOO");
            int32_t res;
            if (!JS_ToInt32(jsw->mJSContext, &res, r)) CoronaLog("R! %x", res);
            else CoronaLog("QQQ");
            CoronaLog("22");
        } else js_std_dump_error(jsw->mJSContext);
    }
    uint32_t size;
    uint8_t * b = m3_GetMemory(jsw->mWASM.mRuntime, &size, 0);
    CoronaLog("!!!bb %p", b);
    JSValue m = JS_GetPropertyStr(jsw->mJSContext, p, "_malloc");
    CoronaLog("????m %i",  JS_IsUndefined(m));
    JSValue f = JS_GetPropertyStr(jsw->mJSContext, p, "_free");
    CoronaLog("????f %i",  JS_IsUndefined(f));
    if (!JS_IsUndefined(m) && !JS_IsUndefined(f))
    {
        JSValue memsize = JS_NewInt32(jsw->mJSContext, 28);
        JSValue r = JS_Call(jsw->mJSContext, m, JS_UNDEFINED, 1, &memsize);
        CoronaLog("!!!ff %i", JS_IsException(r));
        if (!JS_IsException(r))
        {CoronaLog("ffOOO");
            int32_t res;
            if (!JS_ToInt32(jsw->mJSContext, &res, r)) CoronaLog("R! %x", res);
            else CoronaLog("QQQ");
            CoronaLog("22");
        } else js_std_dump_error(jsw->mJSContext);
    }
    /*
		d_openmpt_module_create_from_memory = (dll_openmpt_module_create_from_memory)getDllProc(dll, "openmpt_module_create_from_memory");
            openmpt_module * 	openmpt_module_create_from_memory (const void *filedata, size_t filesize, openmpt_log_func logfunc, void *loguser, const openmpt_module_initial_ctl *ctls)

            		mModfile = openmpt_module_create_from_memory((const void*)mParent->mData, mParent->mDataLen, NULL, NULL, NULL);		
		openmpt_module_set_repeat_count(mModfile, -1);

		d_openmpt_module_destroy = (dll_openmpt_module_destroy)getDllProc(dll, "openmpt_module_destroy");
            void 	openmpt_module_destroy (openmpt_module *mod)
		d_openmpt_module_read_float_stereo = (dll_openmpt_module_read_float_stereo)getDllProc(dll, "openmpt_module_read_float_stereo");
            size_t 	openmpt_module_read_float_stereo (openmpt_module *mod, int32_t samplerate, size_t count, float *left, float *right)

            			int res = openmpt_module_read_float_stereo(mModfile, (int)floor(mSamplerate), samples, aBuffer + outofs, aBuffer + outofs + aBufferSize);
			if (res == 0)
			{
				mPlaying = 0;
				return outofs;
			}
		d_openmpt_module_set_repeat_count
            int 	openmpt_module_set_repeat_count (openmpt_module *mod, int32_t repeat_count)
    */
#endif
    JS_FreeValue(jsw->mJSContext, global_obj);

    return 1;
}
