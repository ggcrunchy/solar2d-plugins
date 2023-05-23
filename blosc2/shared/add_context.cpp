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

#include "common.h"

//
//
//

#define CONTEXT_METATABLE_NAME "blosc.context"

//
//
//

blosc2_context * GetContext (lua_State * L, int arg)
{
    return *LuaXS::CheckUD<blosc2_context *>(L, arg, CONTEXT_METATABLE_NAME);
}

//
//
//

#if 0
BLOSC_EXPORT int blosc2_set_maskout(blosc2_context *ctx, bool *maskout, int nblocks);

BLOSC_EXPORT int blosc2_compress_ctx(
        blosc2_context* context, const void* src, int32_t srcsize, void* dest,
        int32_t destsize);
BLOSC_EXPORT int blosc2_decompress_ctx(blosc2_context* context, const void* src,
                                       int32_t srcsize, void* dest, int32_t destsize);
BLOSC_EXPORT int blosc2_getitem_ctx(blosc2_context* context, const void* src,
                                    int32_t srcsize, int start, int nitems, void* dest,
                                    int32_t destsize);
#endif

//
//
//

static void Methods (lua_State * L)
{
	luaL_Reg funcs[] = {
        {
            "__gc", [](lua_State * L)
            {
                blosc2_free_ctx(GetContext(L));

                return 0;
            }
        }, {
            "get_cparams", [](lua_State * L)
            {
                blosc2_cparams cparams;

                blosc2_ctx_get_cparams(GetContext(L), &cparams);

                LuaXS::NewTyped<ParamsBox<blosc2_cparams>>(L, cparams); // context, cparams

                WeakKeyPair(L, -1, 1);

                return 1;
            }
        }, {
            "get_dparams", [](lua_State * L)
            {
                blosc2_dparams dparams;

                blosc2_ctx_get_dparams(GetContext(L), &dparams);

                LuaXS::NewTyped<ParamsBox<blosc2_dparams>>(L, dparams); // context, dparams

                WeakKeyPair(L, -1, 1);

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

static void WrapContext (lua_State * L, blosc2_context * context)
{
    LuaXS::NewTyped<blosc2_context *>(L, context); // ..., context
	LuaXS::AttachMethods(L, CONTEXT_METATABLE_NAME, Methods);
}

//
//
//

void AddContext (lua_State * L)
{
	luaL_Reg funcs[] = {
		{
			"create_cctx", [](lua_State * L)
			{
                blosc2_context * ctx = blosc2_create_cctx(*GetCparams(L));

                if (ctx) WrapContext(L, ctx); // cparams, context
                else lua_pushnil(L); // cparams, nil

				return 1;
			}
		}, {
            "blosc2_create_dctx", [](lua_State * L)
            {
                blosc2_context * ctx = blosc2_create_dctx(*GetDparams(L));

                if (ctx) WrapContext(L, ctx); // dparams, context
                else lua_pushnil(L); // dparams, nil

				return 1;
            }
        },
		{ nullptr, nullptr }
	};

	luaL_register(L, nullptr, funcs);
}