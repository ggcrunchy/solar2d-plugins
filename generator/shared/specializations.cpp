// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation; either
// version 2.1 of the License, or (at your option) any later version.

#include "common.h"

//
//
//

template<> gml::dvec2 LuaXS::GetArgBody<gml::dvec2> (lua_State * L, int arg)
{
	luaL_checktype(L, arg, LUA_TTABLE);

	gml::dvec2 v;

	for (int i = 0; i < 2; ++i, lua_pop(L, 1))
	{
		lua_rawgeti(L, arg, i); // ..., v, ..., value

		v[i] = luaL_checknumber(L, -1);
	}

	return v;
}

template<> gml::dvec3 LuaXS::GetArgBody<gml::dvec3> (lua_State * L, int arg)
{
	luaL_checktype(L, arg, LUA_TTABLE);

	gml::dvec3 v;

	for (int i = 0; i < 3; ++i, lua_pop(L, 1))
	{
		lua_rawgeti(L, arg, i); // ..., v, ..., value

		v[i] = luaL_checknumber(L, -1);
	}

	return v;
}

template<> gml::ivec2 LuaXS::GetArgBody<gml::ivec2> (lua_State * L, int arg)
{
	luaL_checktype(L, arg, LUA_TTABLE);

	gml::ivec2 v;

	for (int i = 0; i < 2; ++i, lua_pop(L, 1))
	{
		lua_rawgeti(L, arg, i); // ..., v, ..., value

		v[i] = luaL_checkint(L, -1);
	}

	return v;
}

template<> gml::ivec3 LuaXS::GetArgBody<gml::ivec3> (lua_State * L, int arg)
{
	luaL_checktype(L, arg, LUA_TTABLE);

	gml::ivec3 v;

	for (int i = 0; i < 3; ++i, lua_pop(L, 1))
	{
		lua_rawgeti(L, arg, i); // ..., v, ..., value

		v[i] = luaL_checkint(L, -1);
	}

	return v;
}