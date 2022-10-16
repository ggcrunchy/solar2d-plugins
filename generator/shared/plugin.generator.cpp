// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation; either
// version 2.1 of the License, or (at your option) any later version.

#include "common.h"

//
//
//

gml::dvec2 GetVec2 (lua_State * L, int arg)
{
	arg = CoronaLuaNormalize(L, arg);

	luaL_checktype(L, arg, LUA_TTABLE);
	lua_getfield(L, arg, "x"); // ..., x
	lua_getfield(L, arg, "y"); // ..., x, y

	return gml::dvec2{luaL_checknumber(L, -2), luaL_checknumber(L, -1)};
}

//
//
//

gml::dvec3 GetVec3 (lua_State * L, int arg)
{
	arg = CoronaLuaNormalize(L, arg);

	luaL_checktype(L, arg, LUA_TTABLE);
	lua_getfield(L, arg, "x"); // ..., x
	lua_getfield(L, arg, "y"); // ..., x, y
	lua_getfield(L, arg, "z"); // ..., x, y, z

	return gml::dvec3{luaL_checknumber(L, -3), luaL_checknumber(L, -2), luaL_checknumber(L, -1)};
}

//
//
//

generator::Axis GetAxis (lua_State * L, int arg, const char * def)
{
	const char * names[] = { "X", "Y", "Z", nullptr };
	generator::Axis axes[] = { generator::Axis::X, generator::Axis::Y, generator::Axis::Z };

	return axes[luaL_checkoption(L, arg, def, names)];
}

//
//
//

static int sTransformLookupsRef;

//
//
//

int AddTransform (lua_State * L, size_t nvalues, void ** key)
{
	*key = lua_touserdata(L, -1);

	lua_createtable(L, 0, 2); // object, xform, transformed, env
	lua_pushvalue(L, 2); // object, xform, transformed, env, xform
	lua_setfield(L, -2, "xform"); // object, xform, transformed, env = { xform = xform }
	lua_createtable(L, 0, int(nvalues)); // object, xform, transformed, env, output
	lua_setfield(L, -2, "output"); // object, xform, transformed, env = { xform, output = output }
	lua_setfenv(L, -2); // object, xform, transformed; transformed.env = env
	lua_getref(L, sTransformLookupsRef); // object, xform, transformed, lookups
	lua_pushlightuserdata(L, *key); // object, xform, transformed, lookups, key
	lua_pushvalue(L, -3); // object, xform, transformed, lookups, key, transformed
	lua_rawset(L, -3); // object, xform, transformed, lookups; lookups = { ..., [key] = transformed }
	lua_pop(L, 1); // object, xform, transformed

	return 1;
}

//
//
//

void LookupTransform (lua_State * L, void * key)
{
	lua_getref(L, sTransformLookupsRef); // ..., lookups
	lua_pushlightuserdata(L, key); // ..., lookups, key
	lua_rawget(L, -2); // ..., lookups, transformed?
	luaL_argcheck(L, !lua_isnil(L, -1), -1, "Transform object does not exist");
	lua_getfenv(L, -1); // ..., lookups, transformed, env
	lua_getfield(L, -1, "xform"); // ..., lookups, transformed, env, xform
	lua_replace(L, -3); // ..., lookups, xform, env, output
	lua_getfield(L, -1, "output"); // ..., lookups, xform, env, output
	lua_pushvalue(L, -1); // ..., lookups, xform, env, output, output
	lua_insert(L, -5); // ..., output, xform, env, output
	lua_remove(L, -2); // ..., output, xform, output
}

//
//
//

CORONA_EXPORT int luaopen_plugin_generator (lua_State* L)
{
    lua_newtable(L); // generator

	//
	//
	//

	add_meshes(L);
	add_paths(L);
	add_shapes(L);

	//
	//
	//

	lua_newtable(L); // generator, lookups
	lua_createtable(L, 0, 1); // generator, lookups, lookups_mt
	lua_pushliteral(L, "v"); // generator, lookups, lookups_mt, "v"
	lua_setfield(L, -2, "__mode"); // generator, lookups, lookups_mt = { __mode = "v" }
	lua_setmetatable(L, -2); // generator, lookups; lookups.mt = lookups_mt

	sTransformLookupsRef = lua_ref(L, 1); // generator; ref = lookups

	//
	//
	//

	return 1;
}
