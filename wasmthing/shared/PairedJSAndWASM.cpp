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

struct JSWasm {
    JSRuntime * mJSRuntime;
    JSContext * mJSContext;
    WASMPair mWASM;

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


        tjs__mod_wasm_init(mJSContext, bootstrap_ns_atom);

        eval_buf(mJSContext, kWebAssemblyPolyfill, sizeof(kWebAssemblyPolyfill) - 1, "WebAssembly.js", JS_EVAL_TYPE_MODULE);

        // end bootstrap
        JS_DeleteProperty(mJSContext, global_obj, bootstrap_ns_atom, 0);
        JS_FreeAtom(mJSContext, bootstrap_ns_atom);
        JS_FreeValue(mJSContext, bootstrap_ns);
    }

    //
    //
    //

    void AddModule (JSValue global_obj, const char * name, size_t name_len, const uint8_t * wasm_bytes, size_t wasm_len)
    {
        JSAtom lib_atom = JS_NewAtomLen(mJSContext, name, name_len);
        JSValue lib_proto = JS_NewObjectProto(mJSContext, JS_NULL);
        JSValue wasmBinary = JS_NewArrayBufferCopy(mJSContext, wasm_bytes, wasm_len);

        JS_SetPropertyStr(mJSContext, lib_proto, "wasmBinary", wasmBinary);

        //...

        JS_SetProperty(mJSContext, global_obj, lib_atom, lib_proto);
        JS_DeleteProperty(mJSContext, global_obj, lib_atom, 0);
        JS_FreeAtom(mJSContext, lib_atom);
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
    lua_pushvalue(L, lua_upvalueindex(1)); // params, filename, LoadFile
    lua_pushvalue(L, -2); // params, filename, LoadFile, filename

    LoadFile(L); // params, filename, js_file?

    if (lua_isnil(L, -1)) return Error(L, "Unable to load .js file");

    //
    //
    //

    lua_pushvalue(L, lua_upvalueindex(1)); // params, filename, js_file, LoadFile
    lua_pushlstring(L, js_file, ext_index); // params, filename, js_file, LoadFile, filename_minus_extension (n.b. ext_index is 0-indexed)
    lua_pushliteral(L, ".wasm"); // params, filename, js_file, LoadFile, filename_minus_extension, ".wasm"
    lua_concat(L, 2); // params, filename, js_file, LoadFile, wasm_filename

    LoadFile(L); // params, filename, js_file, wasm_file?
    
    if (lua_isnil(L, -1)) return Error(L, "Unable to load .wasm file");

    //
    //
    //

    const char * js_bytes = lua_tostring(L, -2);
    const void * wasm_bytes = lua_topointer(L, -1);
    size_t js_len = lua_objlen(L, -2), wasm_len = lua_objlen(L, -1);

    //
    //
    //

    JSWasm * jsw = New<JSWasm>(L); // params, filename, js_file, wasm_file, jsw

    jsw->Reset();

    //
    //
    //

    AddMethods(L, [](lua_State * L) {
        luaL_Reg funcs[] = {
            {
                "__gc", [](lua_State * L)
                {
                    JSWasm * jsw = Get<JSWasm>(L);

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
    }, JSWasm::MetatableName());

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

    jsw->AddBoostrap(global_obj);
    jsw->AddModule(global_obj, js_bytes + after_seps, size_t(ext_index), Ptr<uint8_t>(wasm_bytes), wasm_len);

#if 0                 
    jsw->mWASMRuntime = m3_NewRuntime(jsw->mWASMEnv, 512 * 1024, nullptr/*&stuff*/);
        // stuff includes JSRuntime...

    if (!jsw->mWASMRuntime) return Error(L, "Failed to create WASM runtime");
#endif

    JS_FreeValue(jsw->mJSContext, global_obj);

    return 0;
}
