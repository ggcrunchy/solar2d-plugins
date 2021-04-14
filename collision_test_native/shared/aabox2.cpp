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

#include "CoronaLua.h"
#include "common.h"
#include "vector2d.h"

struct AABox {
	Vec2 center{}, extent{};
};

#define AABOX2_METATABLE "ctnative.aabox2"

static const AABox & GetConstAABox (lua_State * L, int arg = 1)
{
	return *static_cast<const AABox *>(luaL_checkudata(L, arg, AABOX2_METATABLE));
}

static AABox & GetAABox (lua_State * L, int arg = 1)
{
	return *static_cast<AABox *>(luaL_checkudata(L, arg, AABOX2_METATABLE));
}

int NewAABox2 (lua_State * L)
{
	lua_newuserdata(L, sizeof(AABox)); // aa_box

	if (luaL_newmetatable(L, AABOX2_METATABLE)) // aa_box, aa_box_mt
	{
		luaL_Reg methods[] = {
			{
				"Augment", [](lua_State * L)
				{
					const Vec2 & delta = GetConstVec2(L, 2);

					GetAABox(L).extent += { abs(delta.x), abs(delta.y) };

					return 0;
				}
			},
			{
				"CanTriviallyRejectSegment", [](lua_State * L)
				{
					const AABox & box = GetConstAABox(L);
					const Vec2 & p1 = GetConstVec2(L, 2), & p2 = GetConstVec2(L, 3);
					const Vec2 pmax = p1.Max(p2), pmin = p1.Min(p2);

					lua_pushboolean(L, p1.Min(p2) > box.center + box.extent && p1.Max(p2) < box.center - box.extent); // box, p1, p2, trivally_outside?

					return 1;
				}
			},
			{
				"ContainsPoint", [](lua_State * L)
				{
					const AABox & box = GetConstAABox(L);
					const Vec2 & delta = GetConstVec2(L, 2) - box.center;

					lua_toboolean(L, !(delta.Abs() > box.extent)); // box, point, contains?

					return 1;
				}
			},
			{
				"__index", [](lua_State * L)
				{
					if (lua_isstring(L, 2))
					{
						const char * str = lua_tostring(L, 2);

						if (strcmp(str, "center") == 0) return AuxNewVec2(L, GetConstAABox(L).center); // t, k, center
						else if (strcmp(str, "extent") == 0) return AuxNewVec2(L, GetConstAABox(L).extent); // t, k, extent
					}

					lua_getmetatable(L, 1); // t, k, mt
					lua_replace(L, 1); // mt, k
					lua_rawget(L, 1); // mt, value?

					return 1;
				}
			},
			{
				"__newindex", [](lua_State * L)
				{
					if (lua_isstring(L, 2))
					{
						const char * str = lua_tostring(L, 2);
						
						if (strcmp(str, "center") == 0) GetAABox(L).center = GetConstVec2(L, 3);
						else if (strcmp(str, "extent") == 0) GetAABox(L).extent = GetConstVec2(L, 3);
					}

					return 0;
				}
			},
			{
				"SetCenter", [](lua_State * L)
				{
					GetAABox(L).center = GetConstVec2(L, 2);

					return 0;
				}
			},
			{
				"SetFromCircle", [](lua_State * L)
				{
					AABox & box = GetAABox(L);
					lua_Number radius = luaL_checknumber(L, 3);

					box.center = GetConstVec2(L, 2);
					box.extent = { radius, radius };

					return 0;
				}
			},
			{ nullptr, nullptr }
		};

		luaL_register(L, nullptr, methods);
	}

	lua_setmetatable(L, -2); // aa_box

	return 1;
}