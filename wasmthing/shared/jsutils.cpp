// Adapted from https://github.com/quickjs-ng/quickjs

/*
 * C utilities
 *
 * Copyright (c) 2017 Fabrice Bellard
 * Copyright (c) 2018 Charlie Gordon
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
#include "jsutils.h"

//
//
//

static void js_dump_obj(JSContext *ctx, FILE *f, JSValue val)
{
    const char *str;

    str = JS_ToCString(ctx, val);
    if (str) {
        CoronaLog(str);//fprintf(f, "%s\n", str);
        JS_FreeCString(ctx, str);
    } else {
        CoronaLog("[exception]");//fprintf(f, "[exception]\n");
    }
}

//
//
//

static void js_std_dump_error1(JSContext *ctx, JSValue exception_val)
{
    JSValue val;
    int is_error = JS_IsError(ctx, exception_val);
    js_dump_obj(ctx, stderr, exception_val);
    if (is_error) {
        val = JS_GetPropertyStr(ctx, exception_val, "stack");
        if (!JS_IsUndefined(val)) {
            js_dump_obj(ctx, stderr, val);
        }
        JS_FreeValue(ctx, val);
    }
}

//
//
//

void js_std_dump_error(JSContext *ctx)
{
    JSValue exception_val;

    exception_val = JS_GetException(ctx);
    js_std_dump_error1(ctx, exception_val);
    JS_FreeValue(ctx, exception_val);
}

//
//
//

static void pstrcpy(char *buf, int buf_size, const char *str)
{
    int c;
    char *q = buf;

    if (buf_size <= 0)
        return;

    for(;;) {
        c = *str++;
        if (c == 0 || q >= buf + buf_size - 1)
            break;
        *q++ = c;
    }
    *q = '\0';
}

//
//
//

/* strcat and truncate. */
static char *pstrcat(char *buf, int buf_size, const char *s)
{
    int len;
    len = strlen(buf);
    if (len < buf_size)
        pstrcpy(buf + len, buf_size - len, s);
    return buf;
}

//
//
//

int js_module_set_import_meta(JSContext *ctx, JSValue func_val,
                              JS_BOOL use_realpath, JS_BOOL is_main)
{
    JSModuleDef *m;
    char buf[4096];
    JSValue meta_obj;
    JSAtom module_name_atom;
    const char *module_name;

    assert(JS_VALUE_GET_TAG(func_val) == JS_TAG_MODULE);
    m = (JSModuleDef *)JS_VALUE_GET_PTR(func_val);

    module_name_atom = JS_GetModuleName(ctx, m);
    module_name = JS_AtomToCString(ctx, module_name_atom);
    JS_FreeAtom(ctx, module_name_atom);
    if (!module_name)
        return -1;
    if (!strchr(module_name, ':')) {
        strcpy(buf, "file://");
#if !defined(_WIN32)
        /* realpath() cannot be used with modules compiled with qjsc
           because the corresponding module source code is not
           necessarily present */
        if (use_realpath) {
            char *res = realpath(module_name, buf + strlen(buf));
            if (!res) {
                JS_ThrowTypeError(ctx, "realpath failure");
                JS_FreeCString(ctx, module_name);
                return -1;
            }
        } else
#endif
        {
            pstrcat(buf, sizeof(buf), module_name);
        }
    } else {
        pstrcpy(buf, sizeof(buf), module_name);
    }
    JS_FreeCString(ctx, module_name);

    meta_obj = JS_GetImportMeta(ctx, m);
    if (JS_IsException(meta_obj))
        return -1;
    JS_DefinePropertyValueStr(ctx, meta_obj, "url",
                              JS_NewString(ctx, buf),
                              JS_PROP_C_W_E);
    JS_DefinePropertyValueStr(ctx, meta_obj, "main",
                              JS_NewBool(ctx, is_main),
                              JS_PROP_C_W_E);
    JS_FreeValue(ctx, meta_obj);
    return 0;
}

//
//
//

int eval_buf(JSContext *ctx, const char *buf, int buf_len,
                    const char *filename, int eval_flags)
{
    JSValue val;
    int ret;

    if ((eval_flags & JS_EVAL_TYPE_MASK) == JS_EVAL_TYPE_MODULE) {
        /* for the modules, we compile then run to be able to set
           import.meta */
        val = JS_Eval(ctx, buf, buf_len, filename,
                      eval_flags | JS_EVAL_FLAG_COMPILE_ONLY);
        if (!JS_IsException(val)) {
            js_module_set_import_meta(ctx, val, true, true);
            val = JS_EvalFunction(ctx, val);
        }
    } else {
        val = JS_Eval(ctx, buf, buf_len, filename, eval_flags);
    }
    if (JS_IsException(val)) {
        js_std_dump_error(ctx);
        ret = -1;
    } else {
        ret = 0;
    }
    JS_FreeValue(ctx, val);
    return ret;
}

//
//
//

#define BUFFER_SIZE 16384

//
//
//

static void log (char *out, int &index)
{
    out[index] = '\0';

    if (index) CoronaLog(out);

    index = 0;
}

//
//
//

static void write (const char *s, size_t n, size_t len, char *out, int &index)
{
    assert(n == 1);
    assert(index < BUFFER_SIZE);

    if (index + len > BUFFER_SIZE) log(out, index);

    memcpy(&out[index], s, len);

    index += len;
}

//
//
//

static void put (const char c, char *out, int &index)
{
    assert(index < BUFFER_SIZE);

    if (index + 1 == BUFFER_SIZE) log(out, index);

    out[index++] = c;
}

//
//
//

static JSValue js_print(JSContext *ctx, JSValue this_val,
                        int argc, JSValue *argv)
{
    int i, index = 0;
    const char *str;
    char out[BUFFER_SIZE];
    size_t len;

    for(i = 0; i < argc; i++) {
        if (i != 0)
            put(' ', out, index);//putchar(' ');
        str = JS_ToCStringLen(ctx, &len, argv[i]);
        if (!str)
            return JS_EXCEPTION;
        write(str, 1, len, out, index);//fwrite(str, 1, len, stdout);
        JS_FreeCString(ctx, str);
    }
    put('\n', out, index);//putchar('\n');
    log(out, index);
    //fflush(stdout);
    return JS_UNDEFINED;
}

//
//
//

void js_std_add_helpers(JSContext *ctx)//, int argc, char **argv)
{
    JSValue global_obj, console;//, args;
    // int i;

    /* XXX: should these global definitions be enumerable? */
    global_obj = JS_GetGlobalObject(ctx);

    console = JS_NewObject(ctx);
    JS_SetPropertyStr(ctx, console, "log",
                      JS_NewCFunction(ctx, js_print, "log", 1));
    JS_SetPropertyStr(ctx, console, "error",
                      JS_NewCFunction(ctx, js_print, "error", 1));
    JS_SetPropertyStr(ctx, global_obj, "console", console);

    /* same methods as the mozilla JS shell */
    /* if (argc >= 0) {
        args = JS_NewArray(ctx);
        for(i = 0; i < argc; i++) {
            JS_SetPropertyUint32(ctx, args, i, JS_NewString(ctx, argv[i]));
        }
        JS_SetPropertyStr(ctx, global_obj, "scriptArgs", args);
    } */

    JS_SetPropertyStr(ctx, global_obj, "print",
                      JS_NewCFunction(ctx, js_print, "print", 1));
    /* JS_SetPropertyStr(ctx, global_obj, "__loadScript",
                      JS_NewCFunction(ctx, js_loadScript, "__loadScript", 1)); */

    JS_FreeValue(ctx, global_obj);
}