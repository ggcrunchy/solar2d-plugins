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

#include "blend2d.h"
#include "CoronaLua.h"
#include "common.h"
#include "utils.h"

BLContextCore * GetContext (lua_State * L, int arg, bool * intact_ptr)
{
	return Get<BLContextCore>(L, arg, "blend2d.context", intact_ptr);
}

BLCompOp GetCompOp (lua_State * L, int arg)
{
	const char * names[] = { "SRC_OVER", "SRC_COPY", nullptr };
	BLCompOp ops[] = { BL_COMP_OP_SRC_OVER, BL_COMP_OP_SRC_COPY };
/*
	// BL_DEFINE_ENUM(BLCompOp) {
  //! Source-over [default].
  BL_COMP_OP_SRC_OVER = 0,
  //! Source-copy.
  BL_COMP_OP_SRC_COPY = 1,
  //! Source-in.
  BL_COMP_OP_SRC_IN = 2,
  //! Source-out.
  BL_COMP_OP_SRC_OUT = 3,
  //! Source-atop.
  BL_COMP_OP_SRC_ATOP = 4,
  //! Destination-over.
  BL_COMP_OP_DST_OVER = 5,
  //! Destination-copy [nop].
  BL_COMP_OP_DST_COPY = 6,
  //! Destination-in.
  BL_COMP_OP_DST_IN = 7,
  //! Destination-out.
  BL_COMP_OP_DST_OUT = 8,
  //! Destination-atop.
  BL_COMP_OP_DST_ATOP = 9,
  //! Xor.
  BL_COMP_OP_XOR = 10,
  //! Clear.
  BL_COMP_OP_CLEAR = 11,
  //! Plus.
  BL_COMP_OP_PLUS = 12,
  //! Minus.
  BL_COMP_OP_MINUS = 13,
  //! Modulate.
  BL_COMP_OP_MODULATE = 14,
  //! Multiply.
  BL_COMP_OP_MULTIPLY = 15,
  //! Screen.
  BL_COMP_OP_SCREEN = 16,
  //! Overlay.
  BL_COMP_OP_OVERLAY = 17,
  //! Darken.
  BL_COMP_OP_DARKEN = 18,
  //! Lighten.
  BL_COMP_OP_LIGHTEN = 19,
  //! Color dodge.
  BL_COMP_OP_COLOR_DODGE = 20,
  //! Color burn.
  BL_COMP_OP_COLOR_BURN = 21,
  //! Linear burn.
  BL_COMP_OP_LINEAR_BURN = 22,
  //! Linear light.
  BL_COMP_OP_LINEAR_LIGHT = 23,
  //! Pin light.
  BL_COMP_OP_PIN_LIGHT = 24,
  //! Hard-light.
  BL_COMP_OP_HARD_LIGHT = 25,
  //! Soft-light.
  BL_COMP_OP_SOFT_LIGHT = 26,
  //! Difference.
  BL_COMP_OP_DIFFERENCE = 27,
  //! Exclusion.
  BL_COMP_OP_EXCLUSION = 28,

  //! Count of composition & blending operators.
  BL_COMP_OP_COUNT = 29*/

	return ops[luaL_checkoption(L, arg, "SRC_OVER", names)];
}

static int NewContext (lua_State * L)
{
	BLContextCore * context = New<BLContextCore>(L);// image, context

	blContextInitAs(context, GetImage(L, 1), nullptr);

	if (luaL_newmetatable(L, "blend2d.context")) // image, context, mt
	{
		luaL_Reg context_funcs[] = {
			{
				"destroy", [](lua_State * L)
				{
					BLContextCore * context = GetContext(L, 1);

					blContextDestroy(context);
					Destroy<BLContextCore>(L);

					return 1;
				}
			}, {
				"end", [](lua_State * L)
				{
					blContextEnd(GetContext(L));

					return 0;
				}
			}, {
				"fillAll", [](lua_State * L)
				{
					blContextFillAll(GetContext(L));

					return 0;
				}
			}, {
				"fillPath", [](lua_State * L)
				{
					blContextFillPathD(GetContext(L), GetPath(L, 2));

					return 0;
				}
			}, {
				"__gc", [](lua_State * L)
				{
					bool intact;

					BLContextCore * context = GetContext(L, 1, &intact);

					if (intact) blContextDestroy(context);

					return 0;
				}
			}, {
				"__index", Index
			}, {
				"setFillStyle", [](lua_State * L)
				{
					blContextSetFillStyleRgba32(GetContext(L), (uint32_t)luaL_checkinteger(L, 2));

					return 0;
				}
			}, {
				"setCompOp", [](lua_State * L)
				{
					blContextSetCompOp(GetContext(L), GetCompOp(L, 2));

					return 0;
				}
			},
			{ nullptr, nullptr }
		};

		luaL_register(L, NULL, context_funcs);
	}

	lua_setmetatable(L, -2);// image, context

	return 1;
}

int add_context (lua_State * L)
{
	lua_newtable(L);// t
	lua_pushcfunction(L, NewContext);	// t, NewContext
	lua_setfield(L, -2, "New");	// t = { New = NewContext }

	return 1;
}