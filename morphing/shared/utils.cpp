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

template<typename T> void Write (lua_State *, const T &) {}

template<> void Write<double> (lua_State * L, const double & n)
{
	lua_pushnumber(L, n);	// ..., value
}

template<> void Write<Point2D> (lua_State * L, const Point2D & p)
{
	lua_createtable(L, 2, 0);	// ..., point

	double point[] = { p.x, p.y };

	for (int i = 0; i < 2; ++i)
	{
		lua_pushnumber(L, point[i]);// ..., point, comp
		lua_rawseti(L, -2, i + 1);	// ..., point = { ..., comp }
	}
}

template<typename T> void AuxEmit (lua_State * L, const std::vector<T> & vec, bool write_bytes, const char * key)
{
	if (write_bytes) lua_pushlstring(L, (const char *)&vec.front(), vec.size() * sizeof(T));	// ...[, t], bytes

	else
	{
		lua_createtable(L, vec.size(), 0);	// ...[, t], arr

		for (size_t i = 0; i < vec.size(); ++i)
		{
			Write(L, vec[i]);	// ...[, t], arr, value

			lua_rawseti(L, -2, i + 1);	// ...[, t], arr = { ..., value }
		}
	}

	if (key) lua_setfield(L, -2, key);	// ..., t = { ... key = data }
}

void Emit (lua_State * L, const dvec & vec, bool write_bytes, const char * key)
{
	AuxEmit(L, vec, write_bytes, key);
}

void Emit (lua_State * L, const pvec & vec, bool write_bytes, const char * key)
{
	AuxEmit(L, vec, write_bytes, key);
}

void GetPointFromString (lua_State * L, int arg, Point2D & p, size_t len, int offset)
{
	luaL_argcheck(L, len >= 2 * (1 + offset) * sizeof(double), arg, "Bytes too short to contain point");

	double * pp = (double *)lua_tostring(L, arg);

	p.x = pp[offset * 2 + 0];
	p.y = pp[offset * 2 + 1];
}

void GetPointFromTable (lua_State * L, int arg, Point2D & p, size_t len, int offset)
{
	luaL_argcheck(L, len >= size_t(2 * (1 + offset)), arg, "Too few components for point");
	lua_rawgeti(L, arg, offset * 2 + 1);	// ..., point, ..., x
	lua_rawgeti(L, arg, offset * 2 + 2);	// ..., point, ..., x, y

	p.x = luaL_checknumber(L, -2);
	p.y = luaL_checknumber(L, -1);

	lua_pop(L, 2);	// ..., point, ...
}

template<typename T> void Read (lua_State *, std::vector<T> &) {}

template<> void Read<double> (lua_State * L, dvec & v)
{
	v.push_back(luaL_checknumber(L, -1));
}

template<> void Read<Point2D> (lua_State * L, pvec & v)
{
	lua_rawgeti(L, -1, 1);	// ..., arr, object, x
	lua_rawgeti(L, -2, 2);	// ..., arr, object, x, y

	v.push_back(Point2D(luaL_checknumber(L, -2), luaL_checknumber(L, -1)));
}

template<typename T> void AuxPopulate (lua_State * L, std::vector<T> & vec)
{
	int top = lua_gettop(L);

	for (size_t pos = 1, n = lua_objlen(L, -2); pos <= n; ++pos)
	{
		lua_rawgeti(L, -2, pos);// .., t, object, item

		Read(L, vec);	// ..., t, object, ...

		lua_settop(L, top);	// ..., t, object
	}
}

void Populate (lua_State * L, dvec & vec)
{
	AuxPopulate(L, vec);
}

void Populate (lua_State * L, pvec & vec)
{
	AuxPopulate(L, vec);
}

void SetMethods (lua_State * L, luaL_Reg * methods)
{
	luaL_register(L, NULL, methods);
	lua_pushvalue(L, -1);	// ..., mt, mt
	lua_setfield(L, -2, "__index");	// ..., mt = { __index = mt }
}

bool CompareMeta (lua_State * L, const char * name)
{
	if (!lua_getmetatable(L, 2)) return false;	// ..., object_mt

	luaL_getmetatable(L, name);	// ..., object_mt, meta

	bool eq = lua_equal(L, -2, -1) != 0;

	lua_pop(L, 2);	// ...

	return eq;
}

bool GetWriteBytes (lua_State * L)
{
	lua_pushliteral(L, "write_bytes");	// object, write_bytes?, ..., "write_bytes"

	bool eq = lua_gettop(L) >= 3 && lua_equal(L, 2, -1) != 0;

	lua_settop(L, 1);	// object

	return eq;
}