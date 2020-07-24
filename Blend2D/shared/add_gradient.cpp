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

/*
  BL_INLINE explicit BLGradient(uint32_t type, const double* values = nullptr) noexcept {
    blGradientInitAs(this, type, values, BL_EXTEND_MODE_PAD, nullptr, 0, nullptr);
  }

  BL_INLINE explicit BLGradient(const BLLinearGradientValues& values, uint32_t extendMode = BL_EXTEND_MODE_PAD) noexcept {
    blGradientInitAs(this, BL_GRADIENT_TYPE_LINEAR, &values, extendMode, nullptr, 0, nullptr);
  }

  BL_INLINE explicit BLGradient(const BLRadialGradientValues& values, uint32_t extendMode = BL_EXTEND_MODE_PAD) noexcept {
    blGradientInitAs(this, BL_GRADIENT_TYPE_RADIAL, &values, extendMode, nullptr, 0, nullptr);
  }

  BL_INLINE explicit BLGradient(const BLConicalGradientValues& values, uint32_t extendMode = BL_EXTEND_MODE_PAD) noexcept {
    blGradientInitAs(this, BL_GRADIENT_TYPE_CONICAL, &values, extendMode, nullptr, 0, nullptr);
  }
*/

#include "blend2d.h"
#include "CoronaLua.h"
#include "common.h"
#include "utils.h"

#define GRADIENT_MNAME "blend2d.gradient"

BLGradientCore * GetGradient (lua_State * L, int arg)
{
	return Get<BLGradientCore>(L, arg, GRADIENT_MNAME);
}

bool IsGradient (lua_State * L, int arg)
{
	return Is<BLGradientCore>(L, arg, GRADIENT_MNAME);
}

static uint32_t GetGeometry (lua_State * L, void * data)
{
	int top = lua_gettop(L);

	luaL_checktype(L, 1, LUA_TTABLE);
	lua_getfield(L, 1, "type");	// params, type

	const char * names[BL_GRADIENT_TYPE_COUNT + 1] = { "LINEAR", "RADIAL", "CONICAL", nullptr };
	uint32_t types[BL_GRADIENT_TYPE_COUNT] = { BL_GRADIENT_TYPE_LINEAR, BL_GRADIENT_TYPE_RADIAL, BL_GRADIENT_TYPE_CONICAL };

	uint32_t geometry_type = types[luaL_checkoption(L, -1, nullptr, names)];

	lua_getfield(L, 1, "x0");	// params, type, x0
	lua_getfield(L, 1, "y0");	// params, type, x0, y0

	if (geometry_type != BL_GRADIENT_TYPE_CONICAL)
	{
		lua_getfield(L, 1, "x1");	// params, type, x0, y0, x1
		lua_getfield(L, 1, "y1");	// params, type, x0, y0, x1, y1

		if (BL_GRADIENT_TYPE_LINEAR == geometry_type)
		{
			BLLinearGradientValues * values = static_cast<BLLinearGradientValues *>(data);

			values->x0 = luaL_optnumber(L, -4, 0.0);
			values->y0 = luaL_optnumber(L, -3, 0.0);
			values->x1 = luaL_optnumber(L, -2, 0.0);
			values->y1 = luaL_optnumber(L, -1, 0.0);
		}

		else
		{
			lua_getfield(L, 1, "y1");	// params, type, x0, y0, x1, y1, r0

			BLRadialGradientValues * values = static_cast<BLRadialGradientValues *>(data);

			values->x0 = luaL_optnumber(L, -5, 0.0);
			values->y0 = luaL_optnumber(L, -4, 0.0);
			values->x1 = luaL_optnumber(L, -3, 0.0);
			values->y1 = luaL_optnumber(L, -2, 0.0);
			values->r0 = luaL_optnumber(L, -1, 0.0);
		}
	}

	else
	{
		lua_getfield(L, 1, "angle");	// params, type, x0, y0, angle

		BLConicalGradientValues * values = static_cast<BLConicalGradientValues *>(data);

		values->x0 = luaL_optnumber(L, -3, 0.0);
		values->y0 = luaL_optnumber(L, -2, 0.0);
		values->angle = luaL_optnumber(L, -1, 0.0);
	}

	lua_settop(L, 2);

	return geometry_type;
}

static int NewGradient (lua_State * L)
{
	union {
		BLLinearGradientValues linear;
		BLRadialGradientValues radial;
		BLConicalGradientValues conical;
	} geometry;

	uint32_t type = GetGeometry(L, &geometry);
	BLGradientCore * gradient = New<BLGradientCore>(L);	// params, gradient

	blGradientInitAs(gradient, type, reinterpret_cast<double *>(&geometry), BL_EXTEND_MODE_PAD, nullptr, 0, nullptr);

	if (luaL_newmetatable(L, GRADIENT_MNAME)) // params, gradient, mt
	{
		luaL_Reg gradient_funcs[] = {
			{
				"addStop", [](lua_State * L)
				{
					blGradientAddStopRgba32(GetGradient(L), luaL_checknumber(L, 2), CheckUint32(L, 3));

					return 0;
				}
			}, {
				BLEND2D_DESTROY(Gradient)
			}, {
				BLEND2D_GC(Gradient)
			}, {
				"__index", Index
			}, {
				"resetStops", [](lua_State * L)
				{
					blGradientResetStops(GetGradient(L));

					return 0;
				}
			},
			{ nullptr, nullptr }
		};

		luaL_register(L, NULL, gradient_funcs);
	}

	lua_setmetatable(L, -2);// params, gradient

	return 1;
}

int add_gradient (lua_State * L)
{
	lua_newtable(L);// t
	lua_pushcfunction(L, NewGradient);	// t, NewGradient
	lua_setfield(L, -2, "New");	// t = { New = NewGradient }

	return 1;
}