// Adapted from https://github.com/saghul/txiki.js

/*
 * txiki.js
 *
 * Copyright (c) 2019-present Saúl Ibarra Corretgé <s@saghul.net>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include "common.h"
#include "m3_api_wasi.h"
#include "jsutils.h"

//
//
//

#define countof(x) (sizeof(x) / sizeof((x)[0]))

//
//
//

#define TJS__WASM_MAX_ARGS 32

static JSClassID tjs_wasm_module_class_id;

typedef struct {
    IM3Module module;
    struct {
        uint8_t *bytes;
        size_t size;
    } data;
    bool loaded;
} TJSWasmModule;

static void tjs_wasm_module_finalizer(JSRuntime *rt, JSValue val) {
    TJSWasmModule *m = JS_GetOpaque(val, tjs_wasm_module_class_id);
    if (m) {
        if (m->module && !m->loaded)
            m3_FreeModule(m->module);
        js_free_rt(rt, m->data.bytes);
        js_free_rt(rt, m);
    }
}

static JSClassDef tjs_wasm_module_class = {
    "Module",
    .finalizer = tjs_wasm_module_finalizer,
};

static JSClassID tjs_wasm_instance_class_id;

typedef struct {
    IM3Runtime runtime;
    IM3Module module;
    JSContext *context;
    JSValue *funcs;
    size_t fsize;
    size_t fmaxsize;
} TJSWasmInstance;

static void tjs_wasm_instance_finalizer(JSRuntime *rt, JSValue val) {
    TJSWasmInstance *i = JS_GetOpaque(val, tjs_wasm_instance_class_id);
    if (i) {
        if (i->module) {
            // Free the module, only if it wasn't previously loaded.
            /*if (!i->loaded)
                m3_FreeModule(i->module);*/
        }
        if (i->runtime)
            m3_FreeRuntime(i->runtime);
        if (i->funcs)
            js_free_rt(rt, i->funcs);
        js_free_rt(rt, i);
    }
}

static JSClassDef tjs_wasm_instance_class = {
    "Instance",
    .finalizer = tjs_wasm_instance_finalizer,
};

static JSValue tjs_new_wasm_module(JSContext *ctx) {
    TJSWasmModule *m;
    JSValue obj;

    obj = JS_NewObjectClass(ctx, tjs_wasm_module_class_id);
    if (JS_IsException(obj))
        return obj;

    m = js_mallocz(ctx, sizeof(*m));
    if (!m) {
        JS_FreeValue(ctx, obj);
        return JS_EXCEPTION;
    }

    JS_SetOpaque(obj, m);
    return obj;
}

static TJSWasmModule *tjs_wasm_module_get(JSContext *ctx, JSValueConst obj) {
    return JS_GetOpaque2(ctx, obj, tjs_wasm_module_class_id);
}

static JSValue tjs_new_wasm_instance(JSContext *ctx) {
    TJSWasmInstance *i;
    JSValue obj;

    obj = JS_NewObjectClass(ctx, tjs_wasm_instance_class_id);
    if (JS_IsException(obj))
        return obj;

    i = js_mallocz(ctx, sizeof(*i));
    if (!i) {
        JS_FreeValue(ctx, obj);
        return JS_EXCEPTION;
    }

    JS_SetOpaque(obj, i);
    return obj;
}

static TJSWasmInstance *tjs_wasm_instance_get(JSContext *ctx, JSValueConst obj) {
    return JS_GetOpaque2(ctx, obj, tjs_wasm_instance_class_id);
}

JSValue tjs_throw_wasm_error(JSContext *ctx, const char *name, M3Result r) {
    JSValue obj = JS_NewError(ctx);
    JS_DefinePropertyValueStr(ctx, obj, "message", JS_NewString(ctx, r), JS_PROP_WRITABLE | JS_PROP_CONFIGURABLE);
    JS_DefinePropertyValueStr(ctx, obj, "wasmError", JS_NewString(ctx, name), JS_PROP_WRITABLE | JS_PROP_CONFIGURABLE);
    if (JS_IsException(obj))
        obj = JS_NULL;
    return JS_Throw(ctx, obj);
}

static JSValue tjs__wasm_result(JSContext *ctx, M3ValueType type, const void *stack) {
    switch (type) {
        case c_m3Type_i32: {
            int32_t val = *(int32_t *) stack;
            return JS_NewInt32(ctx, val);
        }
        case c_m3Type_i64: {
            int64_t val = *(int64_t *) stack;
            if (val == (int32_t) val)
                return JS_NewInt32(ctx, (int32_t) val);
            else
                return JS_NewBigInt64(ctx, val);
        }
        case c_m3Type_f32: {
            float val = *(float *) stack;
            return JS_NewFloat64(ctx, (double) val);
        }
        case c_m3Type_f64: {
            double val = *(double *) stack;
            return JS_NewFloat64(ctx, val);
        }
        default:
            return JS_UNDEFINED;
    }
}

static JSValue tjs_wasm_callfunction(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {
    TJSWasmInstance *i = tjs_wasm_instance_get(ctx, this_val);
    if (!i)
        return JS_EXCEPTION;

    const char *fname = JS_ToCString(ctx, argv[0]);
    if (!fname)
        return JS_EXCEPTION;

    IM3Function func;
    M3Result r = m3_FindFunction(&func, i->runtime, fname);
    if (r) {
        JS_FreeCString(ctx, fname);
        return tjs_throw_wasm_error(ctx, "RuntimeError", r);
    }

    JS_FreeCString(ctx, fname);

    int nargs = argc - 1;
    if (nargs == 0) {
        r = m3_Call(func, 0, NULL);
    } else {
        const char *m3_argv[TJS__WASM_MAX_ARGS + 1];
        for (int i = 0; i < nargs; i++) {
            m3_argv[i] = JS_ToCString(ctx, argv[i + 1]);
        }
        m3_argv[nargs] = NULL;
        r = m3_CallArgv(func, nargs, m3_argv);
        for (int i = 0; i < nargs; i++) {
            JS_FreeCString(ctx, m3_argv[i]);
        }
    }

    if (r)
        return tjs_throw_wasm_error(ctx, "RuntimeError", r);

    // https://webassembly.org/docs/js/ See "ToJSValue"
    // NOTE: here we support returning BigInt, because we can.

    int ret_count = m3_GetRetCount(func);

    if (ret_count > TJS__WASM_MAX_ARGS)
        return tjs_throw_wasm_error(ctx, "RuntimeError", "Too many return values");

    uint64_t valbuff[TJS__WASM_MAX_ARGS];
    const void *valptrs[TJS__WASM_MAX_ARGS];
    memset(valbuff, 0, sizeof(valbuff));
    for (int i = 0; i < ret_count; i++) {
        valptrs[i] = &valbuff[i];
    }

    r = m3_GetResults(func, ret_count, valptrs);
    if (r)
        return tjs_throw_wasm_error(ctx, "RuntimeError", r);

    if (ret_count == 1) {
        return tjs__wasm_result(ctx, m3_GetRetType(func, 0), valptrs[0]);
    } else {
        JSValue rets = JS_NewArray(ctx);
        for (int i = 0; i < ret_count; i++) {
            JS_SetPropertyUint32(ctx, rets, i, tjs__wasm_result(ctx, m3_GetRetType(func, i), valptrs[i]));
        }
        return rets;
    }
}

static JSValue tjs_wasm_callindexedfunction(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {
    TJSWasmInstance *i = tjs_wasm_instance_get(ctx, this_val);
    if (!i)
        return JS_EXCEPTION;
    
    uint32_t index;
    if (JS_ToUint32(ctx, &index, argv[0]))
        return JS_EXCEPTION;

    IM3Function func;
    M3Result r = m3_GetTableFunction(&func, i->module, index);
    if (r)
        return tjs_throw_wasm_error(ctx, "RuntimeError", r);

    int nargs = argc - 1;
    if (nargs == 0) {
        r = m3_Call(func, 0, NULL);
    } else {
        const char *m3_argv[TJS__WASM_MAX_ARGS + 1];
        for (int i = 0; i < nargs; i++) {
            m3_argv[i] = JS_ToCString(ctx, argv[i + 1]);
        }
        m3_argv[nargs] = NULL;
        r = m3_CallArgv(func, nargs, m3_argv);
        for (int i = 0; i < nargs; i++) {
            JS_FreeCString(ctx, m3_argv[i]);
        }
    }

    if (r)
        return tjs_throw_wasm_error(ctx, "RuntimeError", r);

    // https://webassembly.org/docs/js/ See "ToJSValue"
    // NOTE: here we support returning BigInt, because we can.

    int ret_count = m3_GetRetCount(func);

    if (ret_count > TJS__WASM_MAX_ARGS)
        return tjs_throw_wasm_error(ctx, "RuntimeError", "Too many return values");

    uint64_t valbuff[TJS__WASM_MAX_ARGS];
    const void *valptrs[TJS__WASM_MAX_ARGS];
    memset(valbuff, 0, sizeof(valbuff));
    for (int i = 0; i < ret_count; i++) {
        valptrs[i] = &valbuff[i];
    }

    r = m3_GetResults(func, ret_count, valptrs);
    if (r)
        return tjs_throw_wasm_error(ctx, "RuntimeError", r);

    if (ret_count == 1) {
        return tjs__wasm_result(ctx, m3_GetRetType(func, 0), valptrs[0]);
    } else {
        JSValue rets = JS_NewArray(ctx);
        for (int i = 0; i < ret_count; i++) {
            JS_SetPropertyUint32(ctx, rets, i, tjs__wasm_result(ctx, m3_GetRetType(func, i), valptrs[i]));
        }
        return rets;
    }
}

static JSValue tjs_wasm_linkwasi(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {
    TJSWasmInstance *i = tjs_wasm_instance_get(ctx, this_val);
    if (!i)
        return JS_EXCEPTION;

    M3Result r = m3_LinkWASI(i->module);
    if (r)
        return tjs_throw_wasm_error(ctx, "LinkError", r);

    return JS_UNDEFINED;
}

static JSValue tjs_wasm_buildinstance(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {
    TJSWasmModule *m = tjs_wasm_module_get(ctx, argv[0]);
    if (!m)
        return JS_EXCEPTION;

    JSValue obj = tjs_new_wasm_instance(ctx);
    if (JS_IsException(obj))
        return obj;

    TJSWasmInstance *i = tjs_wasm_instance_get(ctx, obj);
    WASMPair *wp = JS_GetContextOpaque(ctx);

    //M3Result r = m3_ParseModule(wp->mEnv, &i->module, m->data.bytes, m->data.size);
    // CHECK_NULL(r);  // Should never fail because we already parsed it. TODO: clone it?

    i->module = m->module;
    i->context = ctx;

    /* Create a runtime per module to avoid symbol clash. */
    i->runtime = m3_NewRuntime(wp->mEnv, /* TODO: adjust */ 512 * 1024, i);
    if (!i->runtime) {
        JS_FreeValue(ctx, obj);
        return JS_ThrowOutOfMemory(ctx);
    }
    wp->mRuntime = i->runtime;

    M3Result r = m3_LoadModule(i->runtime, i->module);
    if (r) {
        JS_FreeValue(ctx, obj);
        return tjs_throw_wasm_error(ctx, "LinkError", r);
    }

    m/*i*/->loaded = true;

    return obj;
}

static m3ApiRawFunction(CallJSFunc)
{
    TJSWasmInstance *i = (TJSWasmInstance *)runtime->userdata;

    u16 nargs = m3_GetArgCount(_ctx->function), nrets = m3_GetRetCount(_ctx->function);
    if (nargs > TJS__WASM_MAX_ARGS) {
        m3ApiTrap("Too many arguments!");
    } else if (nrets >= 2) {
        m3ApiTrap("Too many return values!");
    }

    void *raw_return;
    if (nrets != 0)
        raw_return = (_sp++);

    JSValue vals[TJS__WASM_MAX_ARGS]; // n.b. no ref counts

    for (u16 j = 0; j < nargs; j++) {
       switch (m3_GetArgType(_ctx->function, j)) {
            case c_m3Type_i32: {
                m3ApiGetArg(int32_t, v);
                vals[j] = JS_NewInt32(i->context, v);
                break;
            }
            case c_m3Type_i64: {
                m3ApiGetArg(int64_t, v);
                vals[j] = JS_NewInt64(i->context, v);
                break;
            }
            case c_m3Type_f32: {
                m3ApiGetArg(float, v);
                vals[j] = JS_NewFloat64(i->context, v);
                break;
            }
            case c_m3Type_f64: {
                m3ApiGetArg(double, v);
                vals[j] = JS_NewFloat64(i->context, v);
                break;
            }
            default:
                break;
            // ERR!
        }
        // error-check?
    }
    
    size_t index = (size_t)_ctx->userdata;
    JSValue r = JS_Call(i->context, i->funcs[index], JS_UNDEFINED, nargs, vals);

    if (JS_IsException(r)) {
        js_std_dump_error(i->context);
        m3ApiTrap("ERROR!");
    }

    int ok;
    if (nrets != 0) {
        switch (m3_GetRetType(_ctx->function, 0)) {
            case c_m3Type_i32: {
                ok = JS_ToInt32(i->context, (int32_t*)raw_return, r);
                break;
            }
            case c_m3Type_i64: {
                ok = JS_ToInt64(i->context, (int64_t*)raw_return, r);
                break;
            }
            case c_m3Type_f32: {
                double out;
                ok = JS_ToFloat64(i->context, &out, r);
                float cast = (float)out;
                memcpy(raw_return, &cast, sizeof(float));
                break;
            }
            case c_m3Type_f64: {
                ok = JS_ToFloat64(i->context, (double*)raw_return, r);
                break;
            }
            default:
                break;
                // ERR!
        }
    }

    // TODO: if not ok...

    JS_FreeValue(i->context, r);

    m3ApiSuccess();
}

static JSValue tjs_wasm_linkimport(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {
    TJSWasmInstance *i = tjs_wasm_instance_get(ctx, this_val);
    if (!i)
        return JS_EXCEPTION;

    if (i->fsize == i->fmaxsize) {
        if (i->fmaxsize == 0) {
            i->fmaxsize = 32;
        } else {
            i->fmaxsize *= 2;
        }

        i->funcs = js_realloc(ctx, i->funcs, i->fmaxsize * sizeof(JSValue));
    } 
    JS_DupValue(i->context, argv[2]);
    i->funcs[i->fsize++] = argv[2];

    M3Result r = m3_LinkRawFunctionEx(i->module, JS_ToCString(ctx, argv[0]), JS_ToCString(ctx, argv[1]), NULL, &CallJSFunc, (void *)(i->fsize - 1));

    if (r != m3Err_none) {
        // TODO: Report error
    }
    return JS_NewBool(ctx, r == m3Err_none);
}

static JSValue tjs_wasm_moduleexports(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {
    TJSWasmModule *m = tjs_wasm_module_get(ctx, argv[0]);
    if (!m)
        return JS_EXCEPTION;

    JSValue exports = JS_NewArray(ctx);
    if (JS_IsException(exports))
        return exports;

    size_t j = 0;
    for (size_t i = 0; i < m->module->numFunctions; ++i) {
        IM3Function f = &m->module->functions[i];
        const char *name = m3_GetFunctionName(f);
        if (!f->import.fieldUtf8 && f->numNames > 0) {
            JSValue item = JS_NewObjectProto(ctx, JS_NULL);
            JS_DefinePropertyValueStr(ctx, item, "name", JS_NewString(ctx, name), JS_PROP_C_W_E);
            JS_DefinePropertyValueStr(ctx, item, "kind", JS_NewString(ctx, "function"), JS_PROP_C_W_E);
            JS_DefinePropertyValueUint32(ctx, exports, j, item, JS_PROP_C_W_E);
            j++;
        }
    }

    const char *mname = m->module->memoryExportName;
    if (mname) {
        JSValue item = JS_NewObjectProto(ctx, JS_NULL);
        JS_DefinePropertyValueStr(ctx, item, "name", JS_NewString(ctx, mname), JS_PROP_C_W_E);
        JS_DefinePropertyValueStr(ctx, item, "kind", JS_NewString(ctx, "memory"), JS_PROP_C_W_E);
        JS_DefinePropertyValueUint32(ctx, exports, j, item, JS_PROP_C_W_E);
        j++;
    }

    const char *tname = m->module->table0ExportName;
    if (tname) {
        JSValue item = JS_NewObjectProto(ctx, JS_NULL);
        JS_DefinePropertyValueStr(ctx, item, "name", JS_NewString(ctx, tname), JS_PROP_C_W_E);
        JS_DefinePropertyValueStr(ctx, item, "kind", JS_NewString(ctx, "table"), JS_PROP_C_W_E);
        JS_DefinePropertyValueStr(ctx, item, "size", JS_NewUint32(ctx, m->module->table0Size), JS_PROP_C_W_E);
        JS_DefinePropertyValueUint32(ctx, exports, j, item, JS_PROP_C_W_E);
    }
    // TODO: other export types.

    return exports;
}

static JSValue tjs_wasm_moduleimports(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {
    TJSWasmModule *m = tjs_wasm_module_get(ctx, argv[0]);
    if (!m)
        return JS_EXCEPTION;

    JSValue imports = JS_NewArray(ctx);
    if (JS_IsException(imports))
        return imports;
    
    if (m->module->numFuncImports > 0) {
        for (size_t i = 0, j = 0; i < m->module->numFunctions; ++i) {
            IM3Function f = &m->module->functions[i];
            if (f->import.fieldUtf8 && f->import.moduleUtf8) {
                JSValue item = JS_NewObjectProto(ctx, JS_NULL);
                JS_DefinePropertyValueStr(ctx, item, "module", JS_NewString(ctx, f->import.moduleUtf8), JS_PROP_C_W_E);
                JS_DefinePropertyValueStr(ctx, item, "name", JS_NewString(ctx, f->import.fieldUtf8), JS_PROP_C_W_E);
                JS_DefinePropertyValueStr(ctx, item, "kind", JS_NewString(ctx, "function"), JS_PROP_C_W_E);
                JS_DefinePropertyValueUint32(ctx, imports, j, item, JS_PROP_C_W_E);
                j++;
                if (j == m->module->numFuncImports) {
                    break;
                }
            }
        }
    }

    // TODO: memory

    return imports;
}

static JSValue tjs_wasm_parsemodule(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {
    WASMPair *wp = JS_GetContextOpaque(ctx);

    size_t size;
    uint8_t *buf = JS_GetArrayBuffer(ctx, &size, argv[0]);

    if (!buf) {
        /* Reset the exception. */
        JS_FreeValue(ctx, JS_GetException(ctx));

        /* Check if it's a typed array. */
        size_t aoffset, asize;
        JSValue abuf = JS_GetTypedArrayBuffer(ctx, argv[0], &aoffset, &asize, NULL);
        if (JS_IsException(abuf))
            return abuf;
        buf = JS_GetArrayBuffer(ctx, &size, abuf);
        JS_FreeValue(ctx, abuf);
        if (!buf) {
            // It's possible the buffer is NULL and there is no exception, in case of
            // an array buffer of size 0.
            JS_FreeValue(ctx, JS_GetException(ctx));
            JS_ThrowTypeError(ctx, "invalid buffer");
            return JS_EXCEPTION;
        }
        buf += aoffset;
        size = asize;
    }

    JSValue obj = tjs_new_wasm_module(ctx);
    TJSWasmModule *m = tjs_wasm_module_get(ctx, obj);
    m->data.bytes = js_malloc(ctx, size);
    if (!m->data.bytes) {
        JS_FreeValue(ctx, obj);
        return JS_EXCEPTION;
    }
    memcpy(m->data.bytes, buf, size);
    m->data.size = size;

    M3Result r = m3_ParseModule(wp->mEnv, &m->module, m->data.bytes, m->data.size);
    if (r) {
        JS_FreeValue(ctx, obj);
        return tjs_throw_wasm_error(ctx, "CompileError", r);
    }

    return obj;
}

static JSValue tjs_wasm_memorybuffer(JSContext *ctx, JSValueConst this_val) {
    uint32_t size;
    WASMPair * wp = JS_GetContextOpaque(ctx);
    uint8_t *memory = m3_GetMemory(wp->mRuntime, &size, 0);

    return JS_NewArrayBuffer(ctx, memory, size, NULL, NULL, 0);
}

static JSValue tjs_wasm_requestmemorypages(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {
    uint32_t delta;
    if (JS_ToUint32(ctx, &delta, argv[0]))
        return JS_EXCEPTION;
    
    WASMPair * wp = JS_GetContextOpaque(ctx);
    uint32_t current = wp->mRuntime->memory.numPages;

    if (current + delta > wp->mRuntime->memory.maxPages)
        return JS_ThrowRangeError(ctx, "Attempt to allocate more than max pages");

    M3Result r = ResizeMemory(wp->mRuntime, current + delta);
    if (r)
        return tjs_throw_wasm_error(ctx, "RuntimeError", r);

    return JS_NewUint32(ctx, current);
}

#define TJS_CFUNC_DEF(name, length, func1)                                                                             \
    {                                                                                                                  \
        name, JS_PROP_C_W_E, JS_DEF_CFUNC, 0, .u = {.func = { length, JS_CFUNC_generic, { .generic = func1 } } }       \
    }

static const JSCFunctionListEntry tjs_wasm_funcs[] = {
    TJS_CFUNC_DEF("buildInstance", 1, tjs_wasm_buildinstance),
    TJS_CFUNC_DEF("moduleExports", 1, tjs_wasm_moduleexports),
    TJS_CFUNC_DEF("moduleImports", 1, tjs_wasm_moduleimports),
    TJS_CFUNC_DEF("parseModule", 1, tjs_wasm_parsemodule),    
    JS_CGETSET_DEF("memoryBuffer", tjs_wasm_memorybuffer, NULL),
    TJS_CFUNC_DEF("requestMemoryPages", 1, tjs_wasm_requestmemorypages),
};

static const JSCFunctionListEntry tjs_wasm_instance_funcs[] = {
    TJS_CFUNC_DEF("callFunction", 1, tjs_wasm_callfunction),
    TJS_CFUNC_DEF("callIndexedFunction", 1, tjs_wasm_callindexedfunction),
    TJS_CFUNC_DEF("linkImport", 3, tjs_wasm_linkimport),
    TJS_CFUNC_DEF("linkWasi", 0, tjs_wasm_linkwasi),
};

void tjs__mod_wasm_init(JSContext *ctx, JSValue ns) {
    JSRuntime *rt = JS_GetRuntime(ctx);

    /* Module object */
    JS_NewClassID(rt, &tjs_wasm_module_class_id);
    JS_NewClass(rt, tjs_wasm_module_class_id, &tjs_wasm_module_class);
    JS_SetClassProto(ctx, tjs_wasm_module_class_id, JS_NULL);

    /* Instance object */
    JS_NewClassID(rt, &tjs_wasm_instance_class_id);
    JS_NewClass(rt, tjs_wasm_instance_class_id, &tjs_wasm_instance_class);
    JSValue proto = JS_NewObject(ctx);

    JS_SetPropertyFunctionList(ctx, proto, tjs_wasm_instance_funcs, countof(tjs_wasm_instance_funcs));
    JS_SetClassProto(ctx, tjs_wasm_instance_class_id, proto);

    JSValue obj = JS_NewObjectProto(ctx, JS_NULL);
    JS_SetPropertyFunctionList(ctx, obj, tjs_wasm_funcs, countof(tjs_wasm_funcs));

    JS_DefinePropertyValueStr(ctx, ns, "wasm", obj, JS_PROP_C_W_E);
}