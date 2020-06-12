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
#include <vector>

#define PATH_MNAME "blend2d.path"

BLPathCore * GetPath (lua_State * L, int arg, bool * intact_ptr)
{
	return Get<BLPathCore>(L, arg, PATH_MNAME, intact_ptr);
}

bool IsPath (lua_State * L, int arg)
{
	return Is<BLPathCore>(L, arg, PATH_MNAME);
}

static int NewPath (lua_State * L)
{
	BLPathCore * path = New<BLPathCore>(L);	// path

	blPathInit(path);

	if (luaL_newmetatable(L, PATH_MNAME)) // path, mt
	{
		luaL_Reg path_funcs[] = {
			{
				"arcQuadrantTo", [](lua_State * L)
				{
					blPathArcQuadrantTo(GetPath(L),
						luaL_checknumber(L, 2), luaL_checknumber(L, 3),
						luaL_checknumber(L, 4), luaL_checknumber(L, 5));

					return 0;
				}
			}, {
				"arcTo", [](lua_State * L)
				{
					blPathArcTo(GetPath(L),
						luaL_checknumber(L, 2), luaL_checknumber(L, 3),
						luaL_checknumber(L, 4), luaL_checknumber(L, 5),
						luaL_checknumber(L, 6), luaL_checknumber(L, 6), lua_toboolean(L, 7) != 0);

					return 0;
				}
			}, {
				"clear", [](lua_State * L)
				{
					blPathClear(GetPath(L));
					
					return 0;
				}
			}, {
				"destroy", [](lua_State * L)
				{
					BLPathCore * path = GetPath(L);

					blPathDestroy(path);
					Destroy(path);

					return 1;
				}
			}, {
				"__gc", [](lua_State * L)
				{
					bool intact;

					BLPathCore * path = GetPath(L, 1, &intact);

					if (intact) blPathDestroy(path);

					return 0;
				}
			}, {
				"__index", Index
			}, {
				"cubicTo", [](lua_State * L)
				{
					blPathCubicTo(GetPath(L),
						luaL_checknumber(L, 2), luaL_checknumber(L, 3),
						luaL_checknumber(L, 4), luaL_checknumber(L, 5),
						luaL_checknumber(L, 6), luaL_checknumber(L, 7));

					return 0;
				}
			}, {
				"lineTo", [](lua_State * L)
				{
					blPathLineTo(GetPath(L), luaL_checknumber(L, 2), luaL_checknumber(L, 3));

					return 0;
				}
			}, {
				"moveTo", [](lua_State * L)
				{
					blPathMoveTo(GetPath(L), luaL_checknumber(L, 2), luaL_checknumber(L, 3));

					return 0;
				}
			}, {
				"polyTo", [](lua_State * L)
				{
					luaL_argcheck(L, lua_istable(L, 2), 2, "Non-table polynomial");

					size_t n = lua_objlen(L, 2);

					std::vector<BLPoint> poly(n);

					for (int i = 1; i <= int(n); ++i, lua_pop(L, 2))
					{
						lua_rawgeti(L, 2, i); // path, poly, point
						lua_getfield(L, -1, "x"); // path, poly, point, x
						lua_getfield(L, -2, "y"); // path, poly, point, x, y

						poly[i - 1] = BLPoint{ luaL_checknumber(L, -2), luaL_checknumber(L, -1) };
					}

					blPathPolyTo(GetPath(L), poly.data(), poly.size());

					return 0;
				}
			}, {
				"quadTo", [](lua_State * L)
				{
					blPathQuadTo(GetPath(L),
						luaL_checknumber(L, 2), luaL_checknumber(L, 3),
						luaL_checknumber(L, 4), luaL_checknumber(L, 5));

					return 0;
				}
			}, {
				"smoothCubicTo", [](lua_State * L)
				{
					blPathSmoothCubicTo(GetPath(L),
						luaL_checknumber(L, 2), luaL_checknumber(L, 3),
						luaL_checknumber(L, 4), luaL_checknumber(L, 5));

					return 0;
				}
			}, {
				"smoothQuadTo", [](lua_State * L)
				{
					blPathSmoothQuadTo(GetPath(L),
						luaL_checknumber(L, 2), luaL_checknumber(L, 3));

					return 0;
				}
			},
			{ nullptr, nullptr }
		};

		luaL_register(L, NULL, path_funcs);
	}

	lua_setmetatable(L, -2);// path

	return 1;
}

int add_path (lua_State * L)
{
	lua_newtable(L);// t
	lua_pushcfunction(L, NewPath);	// t, NewPath
	lua_setfield(L, -2, "New");	// t = { New = NewPath }

	return 1;
}