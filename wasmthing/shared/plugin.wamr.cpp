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

//
//
//

int my_main(void)
{
/*
        wasm3::wasm_function lib_version = runtime.find_function("xa");
        wasm3::wasm_function core_version = runtime.find_function("za");
        wasm3::wasm_function is_extension_supported = runtime.find_function("Ga");
        wasm3::wasm_function get_string = runtime.find_function("Da");

        auto value = lib_version.call<int32_t>();
        std::cout << std::hex << "lib version: " << value << std::endl;

        value = core_version.call<int32_t>();
        std::cout << std::hex << "core version: " << value << std::endl;
CoronaLog("A");
        uint32_t size;
        uint8_t * mem = m3_GetMemory(r, &size, 0);
CoronaLog("!!!! %p, %u", mem, size);
        uint32_t offset = 250000;
        strcpy(reinterpret_cast<char *>(mem + offset), "BLURG");
        auto value2 = is_extension_supported.call<int>(offset);
        std::cout << "supported extensions: " << value2 << std::endl;
CoronaLog("B");
        uint32_t offset2 = 350000;
        strcpy(reinterpret_cast<char *>(mem + offset2), "library_version");
        auto value3 = get_string.call<int32_t>(offset2);
CoronaLog("??? %i", value3);
        void *p = mem + value3;
CoronaLog("!!! %p", p);
        std::cout << "get string: " << (const char *)p << std::endl;
    }
    catch(std::runtime_error &e) {
        std::cerr << "WASM3 error: " << e.what() << std::endl;
        return 1;
    }
*/
    return 0;
}

#if 0

jobs:
    JSContext *ctx1;
    int err;

    /* execute the pending jobs */
    for (;;) {
        err = JS_ExecutePendingJob(JS_GetRuntime(ctx), &ctx1);
        if (err <= 0) {
            if (err < 0)
                tjs_dump_error(ctx1);
            break;
        }
    }

#endif

#if 0

jobs, #2:
    int r;
    do {
        uv__maybe_idle(qrt);
        r = uv_run(&qrt->loop, UV_RUN_DEFAULT);
    } while (r == 0 && JS_IsJobPending(qrt->rt));

    JSValue exc = JS_GetException(qrt->ctx);
    if (!JS_IsNull(exc)) {
        tjs_dump_error1(qrt->ctx, exc);
        ret = 1;
    }

    JS_FreeValue(qrt->ctx, exc);

#endif

#if 0

JSValue TJS_EvalModule(JSContext *ctx, const char *filename, bool is_main) {
    // snip

    /* Compile then run to be able to set import.meta */
    ret = JS_Eval(ctx, (char *) dbuf.buf, dbuf_size - 1, filename, JS_EVAL_TYPE_MODULE | JS_EVAL_FLAG_COMPILE_ONLY);
    if (!JS_IsException(ret)) {
        js_module_set_import_meta(ctx, ret, TRUE, is_main);
        ret = JS_EvalFunction(ctx, ret);
    }

    /* Emit window 'load' event. */
    if (!JS_IsException(ret) && is_main) {
        static char emit_window_load[] = "window.dispatchEvent(new Event('load'));";
        JSValue ret1 = JS_Eval(ctx, emit_window_load, strlen(emit_window_load), "<global>", JS_EVAL_TYPE_GLOBAL);
        if (JS_IsException(ret1)) {
            tjs_dump_error(ctx);
        }
    }
    // ^^^ can ignore?

    // snip

    return ret;
}

#endif


#if 0

    const module = new WebAssembly.Module(bytes);
    const wasi = new WebAssembly.WASI({ args: subargv.slice(1) });
    const importObject = { wasi_unstable: wasi.wasiImport };
    const instance = new WebAssembly.Instance(module, importObject);

#endif

// JS:Step()

// expose a "L" object in globalThis?
    // then through properties would be able to expose objects
        // if primitive or light userdata (> 5.1 also include light C function), tagged value
        // else, an Object with a Lua reference
            // need to see Lua state during use...
            // could be just a ptr -> object map if maintenance easier
        // calls:
            // function or getmetafield with __call
            // else error
            // would decode one by one and add to stack
            // return values...
                // multiple
                // nil? none?
        // passing functions around...
        // methods



struct JSOnly {
    JSContext * mContext;
    JSRuntime * mRuntime;

    JSOnly ();
    ~JSOnly ();

    // loop?
};

struct WASMOnly {
    M3Runtime * mWASM;
    M3Environment * mEnv;

    WASMOnly ();
    ~WASMOnly ();
};

struct WebAssemblyObject {
    M3Runtime * mWASM; // ??
    JSValue mBuffer;
    bool mDirty;
    bool mShared;

    void Dirty (JSRuntime * runtime) // called by ResizeMemory() or .grow()
    {
        if (!mDirty)
        {
            if (!mShared) /* Detach */ ;

            mDirty = true;
        }
    }
};

CORONA_EXPORT int luaopen_plugin_wasmthing (lua_State* L)
{
    lua_newtable(L);

    //
    //
    //

    luaL_Reg closures[] = {
        {
            "LoadJS", [](lua_State * L)
            {
                luaL_argcheck(L, lua_istable(L, 1), 1, "Non-table params");
                // if Lua, takes state... so that we can do registry, etc.
                // methods, machinery

                return 0;
            }
        }, {
            "LoadJSWasmPair", LoadJSWasmPair
        }, {
            "LoadWASM", [](lua_State * L)
            {
                luaL_argcheck(L, lua_istable(L, 1), 1, "Non-table params");
                // stripped down version of LoadJSWasmPair
                return 0;
            }
        },
        { nullptr, nullptr }
    };

    //
    //
    //

    RegisterWithLoadFile(L, closures);

    //
    //
    //

	return 1;
}
