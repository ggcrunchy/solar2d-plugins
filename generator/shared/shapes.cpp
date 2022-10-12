// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation; either
// version 2.1 of the License, or (at your option) any later version.

#include "common.h"

//
//
//

#define SHAPE_NAME "generator.AnyShape"
#define SHAPE_VERTEX_NAME "generator.ShapeVertex"

//
//
//

generator::AnyShape * GetShape (lua_State * L, int arg)
{
	return LuaXS::CheckUD<generator::AnyShape>(L, arg, SHAPE_NAME);
}

//
//
//

int WrapShape (lua_State * L, generator::AnyShape && shape)
{
	*LuaXS::AllocTyped<generator::AnyShape>(L) = std::move(shape); // shape
	LuaXS::AttachMethods(L, SHAPE_NAME, [](lua_State * L) {
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

generator::ShapeVertex * GetShapeVertex (lua_State * L, int arg)
{
	generator::ShapeVertex ** box = LuaXS::CheckUD<generator::ShapeVertex *>(L, arg, SHAPE_VERTEX_NAME);

	luaL_argcheck(L, *box, arg, "Shape vertex is not active");

	return *box;
}

//
//
//

void MakeShapeVertexBox (lua_State * L, generator::ShapeVertex * vertex)
{
	*LuaXS::NewTyped<generator::ShapeVertex *>(L) = vertex; // ..., box
	LuaXS::AttachMethods(L, SHAPE_VERTEX_NAME, [](lua_State * L) {
		luaL_Reg funcs[] = {
			{
				"GetNormal", [](lua_State * L)
				{
					const gml::dvec2 n = GetShapeVertex(L)->normal();

					lua_pushnumber(L, n[0]); // vertex, x
					lua_pushnumber(L, n[1]); // vertex, x, y

					return 3;
				}
			}, {
				"GetPosition", [](lua_State * L)
				{
					const gml::dvec2 & p = GetShapeVertex(L)->position;

					lua_pushnumber(L, p[0]); // vertex, x
					lua_pushnumber(L, p[1]); // vertex, x, y

					return 3;
				}
			}, {
				"GetTangent", [](lua_State * L)
				{
					const gml::dvec2 & t = GetShapeVertex(L)->tangent;

					lua_pushnumber(L, t[0]); // vertex, x
					lua_pushnumber(L, t[1]); // vertex, x, y

					return 3;
				}
			}, {
				"GetTexCoord", [](lua_State * L)
				{
					lua_pushnumber(L, GetShapeVertex(L)->texCoord); // vertex, u

					return 1;
				}
			},
			{ nullptr, nullptr }
		};

		luaL_register(L, nullptr, funcs);
	});
}

/*
AxisSwapShape
FlipShape
MergeShape
RepeatShape
RotateShape
ScaleShape
SubdivideShape
TransformShape
TranslateShape
*/
/*, {
				"Extrude", [](lua_State * L)
				{
					return WrapMesh(L, generator::extrudeMesh(*GetShape(L), GetPath(L, 2))); // shape, path, extruded
				}
			}*/
/*{
				"Lathe", [](lua_State * L)
				{
	Shape shape,
	Axis axis = Axis::X,
	int slices = 32,
	double start = 0.0,
	double sweep = gml::radians(360.0)
					return WrapMesh(L, generator::lathe(*GetShape(L), GetAxis(L, 2), GetAxis(L, 3), GetAxis(L, 4))); // mesh, xaxis, yaxis, zaxis, swapped
				}
			}, */
/*
	explicit BezierShape(const gml::dvec2 (&p)[D], int segments = 16) :
		// Work around a msvc lambda capture bug by wrapping the array.
		BezierShape{ArrayWrapper{p}, segments}
*/
/*
	CircleShape(
		double radius = 1.0,
		int segments = 32,
		double start = 0.0,
		double sweep = gml::radians(360.0)
	);
*/
/*
explicit GridShape(
		const gml::dvec2& size = {1.0, 1.0},
		const gml::ivec2& segments = {4, 4},
		const gml::ivec2& subSegments = {2, 2}
	) noexcept;
*/
/*
	LineShape(
		const gml::dvec2& start = {0.0, -1.0},
		const gml::dvec2& end = {0.0, 1.0},
		int segments = 8
	);
*/
/*
	explicit ParametricShape(
		std::function<ShapeVertex(double)> eval,
		int segments = 16
	) noexcept;
*/
/*
	RectangleShape(
		const gml::dvec2& size = gml::dvec2{1.0, 1.0},
		const gml::ivec2& segments = gml::ivec2{8, 8}
	);
*/
/*
	RoundedRectangleShape(
		double radius = 0.25,
		const gml::dvec2& size = {0.75, 0.75},
		int slices = 4,
		const gml::ivec2& segments = {8, 8}
	);
*/

//
//
//

void add_shapes (lua_State * L)
{
}