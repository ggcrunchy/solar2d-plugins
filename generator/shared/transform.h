// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation; either
// version 2.1 of the License, or (at your option) any later version.

#pragma once

#include "common.h"

//
//
//

template<typename T> const char * const * NameList () { return nullptr; }
template<typename T> double ** Values (T &) { return nullptr; }
template<typename T> const char * TransformKind() { return nullptr; }

//
//
//

template<typename T> void Transform (lua_State * L, T & v, void * key, const char * what = "transform", int nargs = 0)
{
	LookupTransform(L, key); // ...[, args], output, xform, output

	for (int i = 0; i < nargs; ++i) lua_pushvalue(L, -(nargs + 3) + i); // ...[, args], output, xform, output[, args]

	bool ok = lua_pcall(L, nargs + 1, 0, 0) == 0; // ...[, args], output[, err]

	if (ok)
	{
		const char * const * names = NameList<T>();
		double ** values = Values<T>(v);

		ok = true;

		for (size_t i = 0; ok && names[i]; ++i, lua_pop(L, 1))
		{
			lua_getfield(L, -2, names[i]); // ...[, args], object, v

			if (lua_isnumber(L, -1)) *values[i] = lua_tonumber(L, -1);
			else
			{
				CORONA_LOG_ERROR("Expected number for key %s as %s %s result, but got %s", TransformKind<T>(), names[i], what, luaL_typename(L, -1));

				ok = false;
			}
		}
	}

	//
	//
	//

	else
	{
		CORONA_LOG_ERROR("Error in %s transform callback: %s", lua_isstring(L, -1) ? lua_tostring(L, -1) : "?");

		lua_pop(L, 1); // ...[, args], output
	}

	//
	//
	//

	if (!ok) v = T{};

	lua_pop(L, nargs + 1); // ...
}
