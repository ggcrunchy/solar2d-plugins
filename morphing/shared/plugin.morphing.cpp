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

// plugin.morphing.cpp : Defines the exported functions for the DLL application.
//

#include "stdafx.h"
#include "classes.h"
#include "utils.h"

template<typename T> void Assign (lua_State * L, int index, std::vector<T> & out)
{
	size_t n = lua_objlen(L, index) / sizeof(T);
	T * p = (T *)lua_tostring(L, index); 

	out.assign(p, p + n);
}

static void LoadPoly (lua_State * L, pvec & poly)
{
	switch (lua_type(L, 1))
	{
	case LUA_TSTRING:
		Assign(L, 1, poly);

		break;

	case LUA_TTABLE:
		{
			size_t n = lua_objlen(L, 1);

			poly.resize(n / 2);

			for (size_t i = 1, j = 0; i <= n; i += 2, ++j)
			{
				lua_rawgeti(L, 1, i);	// poly, edges, ..., x
				lua_rawgeti(L, 1, i + 1);	// poly, edges, ..., x, y

				Point2D p;

				p.x = luaL_checknumber(L, -2);
				p.y = luaL_checknumber(L, -1);

				poly[j] = p;

				lua_pop(L, 2);	// poly, edges, ...
			}
		}
		break;

	default:
		luaL_error(L, "Invalid poly type: %s\n", luaL_typename(L, 1));
	}
}

static void LoadEdges (lua_State * L, ivec & edges, size_t poly_size)
{
	switch (lua_type(L, 2))
	{
	case LUA_TSTRING:
		Assign(L, 2, edges);

		break;

	case LUA_TTABLE:
		{
			size_t n = lua_objlen(L, 2);

			edges.resize(n);

			for (size_t i = 1; i <= n; ++i)
			{
				lua_rawgeti(L, 2, i);	// poly, edges, ..., index

				edges[i - 1] = luaL_checkint(L, -1);

				lua_pop(L, 1);	// poly, edges, ...
			}
		}

		break;

	default:
		luaL_error(L, "Invalid edges type: %s\n", luaL_typename(L, 1));
	}

	// 
	if (edges.size() % 2 != 0) edges.pop_back();

	for (size_t i = 0; i < edges.size(); i += 2)
	{
		--edges[i];
		--edges[i + 1];

		luaL_argcheck(L, edges[i] != edges[i + 1], 2, "Degenerate edge");
		luaL_argcheck(L, edges[i] >= 0 && size_t(edges[i]) < poly_size, 2, "Edge point #1 outside polygon");
		luaL_argcheck(L, edges[i + 1] >= 0 && size_t(edges[i + 1]) < poly_size, 2, "Edge point #2 outside polygon");
	}
}

static void LoadCircle (lua_State * L, BaseCircle & circ)
{
	circ.cx = luaL_optnumber(L, -4, 0.0);
	circ.cy = luaL_optnumber(L, -3, 0.0);
	circ.r = luaL_optnumber(L, -2, 0.0);
}

static void LoadCoords (lua_State * L, dvec & vec, const char * key, size_t n)
{
	lua_getfield(L, 1, key);	// data, cc, t

	if (!lua_istable(L, 3)) luaL_error(L, "%s is not a table", key);

	size_t tn = lua_objlen(L, 3);

	if (tn < 2 * n) luaL_error(L, "%s coords are too small", key);

	Populate(L, vec);

	lua_pop(L, 1);	// data, cc
}

static luaL_Reg morphing_funcs[] = {
	{
		"CubicCoordsFromData", [](lua_State * L)
		{
			lua_settop(L, 1);	// data
			luaL_checktype(L, 1, LUA_TTABLE);
			lua_getfield(L, 1, "vcoords");	// data, coords

			if (!lua_istable(L, 2)) luaL_error(L, "vcoords is not a table");

			size_t n = lua_objlen(L, 2);

			CubicCoords * cc = CubicCoords::Instantiate(L, n, true);	// data, coords, cc

			Populate(L, cc->mCoords);

			lua_replace(L, 2);	// data, cc

			LoadCoords(L, cc->mGNCoords, "gn_coords", n);
			LoadCoords(L, cc->mGTCoords, "gt_coords", n);

			cc->mGNCoords.resize(2 * n);
			cc->mGTCoords.resize(2 * n);

			return 1;
		}
	}, {
		"CubicStateFromData", [](lua_State * L)
		{
			// TODO! (still uncertain)

			return 0;
		}
	}, {
		"MinCircle", [](lua_State * L)
		{
			pvec poly;

			LoadPoly(L, poly);

			BaseCircle circle;

			minCircle(poly, circle);
										
			lua_pushnumber(L, circle.cx);	// poly, cx
			lua_pushnumber(L, circle.cy);	// poly, cx, cy
			lua_pushnumber(L, circle.r);// poly, cx, cy, r

			return 3;
		}
	}, {
		"NewPoly", [](lua_State * L)
		{
			lua_settop(L, 4);	// points, cx, cy, r

			Poly * poly = New<Poly>(L);

			LoadPoly(L, poly->mPoints);
			LoadCircle(L, poly->mCirc);

			if (luaL_newmetatable(L, "morphing.poly")) SetMethods(L, normal_methods); // points, cx, cy, r, poly, poly_mt

			lua_setmetatable(L, -2);// points, cx, cy, r, poly

			return 1;
		}
	}, {
		"NewPolyWithHoles", [](lua_State * L)
		{
			lua_settop(L, 5);	// points, edges, cx, cy, r

			Poly * poly = New<Poly>(L);	// points, edges, cx, cy, r, poly

			LoadPoly(L, poly->mPoints);
			LoadEdges(L, poly->mEdges, poly->mPoints.size());
			LoadCircle(L, poly->mCirc);

			if (luaL_newmetatable(L, "morphing.poly_with_holes")) SetMethods(L, holes_methods);	// points, edges, cx, cy, r, poly, poly_mt

			lua_setmetatable(L, -2);// points, edges, cx, cy, r, poly

			return 1;
		}
	}, {
		"PoissonCoordsFromData", [](lua_State * L)
		{
			lua_settop(L, 1);	// data
			luaL_checktype(L, 1, LUA_TTABLE);

			size_t n = lua_objlen(L, 1);

			PoissonCoords * pc = PoissonCoords::Instantiate(L, n, true);	// data, pc

			Populate(L, pc->mCoords);

			return 1;
		}
	}, {
		"PoissonStateFromData", [](lua_State * L)
		{
			lua_settop(L, 1);	// data
			luaL_checktype(L, 1, LUA_TTABLE);

			size_t n = lua_objlen(L, 1) / 2;

			PoissonState * ps = PoissonState::Instantiate(L, n, true);	// data, ps

			Populate(L, ps->mPoints);

			return 1;
		}
	}, 
	{ NULL, NULL }
};

extern "C" {
	
LUALIB_API int luaopen_plugin_morphing (lua_State * L)
{
	CoronaLibraryNew(L, "morphing", "com.xibalbastudios", 1, 0, morphing_funcs, NULL);	// morphing

	return 1;
}

}