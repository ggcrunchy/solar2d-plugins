// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation; either
// version 2.1 of the License, or (at your option) any later version.

#include "common.h"

//
//
//

#define PATH_NAME "generator.AnyPath"
#define PATH_VERTEX_NAME "generator.PathVertex"

//
//
//

generator::AnyPath * GetPath (lua_State * L, int arg)
{
	return LuaXS::CheckUD<generator::AnyPath>(L, arg, PATH_NAME);
}

//
//
//

int WrapPath (lua_State * L, generator::AnyPath && path)
{
	*LuaXS::AllocTyped<generator::AnyPath>(L) = std::move(path); // path
	LuaXS::AttachMethods(L, PATH_NAME, [](lua_State * L) {
		luaL_Reg funcs[] = {
			{ nullptr, nullptr }
		};

		luaL_register(L, nullptr, funcs);
	});

	return 1;
}

//
//
//

generator::PathVertex * GetPathVertex (lua_State * L, int arg)
{
	generator::PathVertex ** box = LuaXS::CheckUD<generator::PathVertex *>(L, arg, PATH_VERTEX_NAME);

	luaL_argcheck(L, *box, arg, "Path vertex is not active");

	return *box;
}

//
//
//

void MakePathVertexBox (lua_State * L, generator::PathVertex * vertex)
{
	*LuaXS::NewTyped<generator::PathVertex *>(L) = vertex; // ..., box
	LuaXS::AttachMethods(L, PATH_VERTEX_NAME, [](lua_State * L) {
		luaL_Reg funcs[] = {
			{
				"GetBinormal", [](lua_State * L)
				{
					const gml::dvec3 b = GetPathVertex(L)->binormal();

					lua_pushnumber(L, b[0]); // vertex, x
					lua_pushnumber(L, b[1]); // vertex, x, y
					lua_pushnumber(L, b[2]); // vertex, x, y, z

					return 3;
				}
			}, {
				"GetNormal", [](lua_State * L)
				{
					const gml::dvec3 & n = GetPathVertex(L)->normal;

					lua_pushnumber(L, n[0]); // vertex, x
					lua_pushnumber(L, n[1]); // vertex, x, y
					lua_pushnumber(L, n[2]); // vertex, x, y, z

					return 3;
				}
			}, {
				"GetPosition", [](lua_State * L)
				{
					const gml::dvec3 & p = GetPathVertex(L)->position;

					lua_pushnumber(L, p[0]); // vertex, x
					lua_pushnumber(L, p[1]); // vertex, x, y
					lua_pushnumber(L, p[2]); // vertex, x, y, z

					return 3;
				}
			}, {
				"GetTangent", [](lua_State * L)
				{
					const gml::dvec3 & t = GetPathVertex(L)->tangent;

					lua_pushnumber(L, t[0]); // vertex, x
					lua_pushnumber(L, t[1]); // vertex, x, y
					lua_pushnumber(L, t[2]); // vertex, x, y, z

					return 3;
				}
			}, {
				"GetTexCoord", [](lua_State * L)
				{
					lua_pushnumber(L, GetPathVertex(L)->texCoord); // vertex, u

					return 1;
				}
			},
			{ nullptr, nullptr }
		};

		luaL_register(L, nullptr, funcs);
	});
}
/*
AxisSwapPath
FlipPath
MergePath
RepeatPath
RotatePath
ScalePath
SubdividePath
TransformPath
TranslatePath
*/
/*
	HelixPath(
		double radius = 1.0,
		double size = 1.0,
		int segments = 32,
		double start = 0.0,
		double sweep = gml::radians(720.0)
	);
*/
/*
	KnotPath(
		int p = 2,
		int q = 3,
		int segments = 96
	);
*/
/*
LinePath(
		const gml::dvec3& start = {0.0, 0.0, -1.0},
		const gml::dvec3& end = {0.0, 0.0, 1.0},
		const gml::dvec3& normal = {1.0, 0.0, 0.0},
		int segments = 8
	);
*/
/*
	explicit ParametricPath(
		std::function<PathVertex(double)> eval,
		int segments = 16
	) noexcept;
*/

//
//
//

void add_paths (lua_State * L)
{
}