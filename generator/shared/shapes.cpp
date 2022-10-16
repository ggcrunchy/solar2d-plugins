// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation; either
// version 2.1 of the License, or (at your option) any later version.

#include "common.h"
#include "transform.h"

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

static const char * sKeys[] = { "px", "py", "tx", "ty", "u", nullptr };

//
//
//

template<> const char * const * NameList<generator::ShapeVertex> () { return sKeys; }
template<> const char * TransformKind<generator::ShapeVertex>() { return "shape"; }

static const size_t ValueCount = sizeof(sKeys) / sizeof(sKeys[0]) - 1;

template<> double ** Values (generator::ShapeVertex & v)
{
	static double * sValues[ValueCount];

	sValues[0] = &v.position[0];
	sValues[1] = &v.position[1];
	sValues[2] = &v.tangent[0];
	sValues[3] = &v.tangent[1];
	sValues[4] = &v.texCoord;

	return sValues;
}

//
//
//

static generator::ShapeVertex ** GetVertexBox (lua_State * L, int arg = 1)
{
	return LuaXS::CheckUD<generator::ShapeVertex *>(L, arg, SHAPE_VERTEX_NAME);
}

//
//
//

int WrapShape (lua_State * L, generator::AnyShape && shape)
{
	*LuaXS::AllocTyped<generator::AnyShape>(L) = std::move(shape); // shape
	LuaXS::AttachMethods(L, SHAPE_NAME, [](lua_State * L) {
		luaL_Reg funcs[] = {
			{
				"AxisSwap", [](lua_State * L)
				{
					return WrapShape(L, generator::axisSwapShape(*GetShape(L))); // shape, swapped
				}
			}, {
				"Flip", [](lua_State * L)
				{
					return WrapShape(L, generator::flipShape(*GetShape(L))); // shape, flipped
				}
			}, {
				"Edges", [](lua_State * L)
				{
					return WrapEdgeGenerator(L, GetShape(L)->edges()); // shape, generator
				}
			}, {
				"Extrude", [](lua_State * L)
				{
					return WrapMesh(L, generator::extrudeMesh(*GetShape(L), *GetPath(L, 2))); // shape, path, extruded
				}
			}, {
				"ForEachEdge", [](lua_State * L)
				{
					luaL_checktype(L, 2, LUA_TFUNCTION);

					for (auto && edge : GetShape(L)->edges())
					{
						lua_pushvalue(L, 2); // shape, func, func
						lua_pushinteger(L, edge.vertices[0] + 1); // shape, func, func, i1
						lua_pushinteger(L, edge.vertices[1] + 1); // shape, func, func, i1, i2

						if (lua_pcall(L, 2, 0, 0) != 0) // shape, func[, err]
						{
							const char * err = lua_isstring(L, -1) ? lua_tostring(L, -1) : "?";
							
							CORONA_LOG_ERROR("Error with edge (%i, %i): %s", edge.vertices[0] + 1, edge.vertices[1] + 1, err);

							lua_pop(L, 1); // shape, func
						}
					}

					return 0;
				}
			}, {
				"__gc", LuaXS::TypedGC<generator::AnyShape>
			}, {
				"Lathe", [](lua_State * L)
				{
					LuaXS::Options opts{L, 2};
					
					generator::Axis axis = generator::Axis::X;
					double start = 0.0, sweep = gml::radians(360.0);
					int slices = 32;

					opts.NV_PAIR(axis)
						.NV_PAIR(start)
						.NV_PAIR(sweep)
						.NV_PAIR(slices);

					return WrapMesh(L, generator::lathe(*GetShape(L), axis, slices, start, sweep)); // shape, params, swapped
				}
			}, {
				"Merge", [](lua_State * L)
				{
					switch (lua_gettop(L))
					{
					case 2:
						return WrapShape(L, generator::mergeShape(*GetShape(L), *GetShape(L, 2)));
						break;
					case 3:
						return WrapShape(L, generator::mergeShape(*GetShape(L), *GetShape(L, 2), *GetShape(L, 3)));
						break;
					case 4:
						return WrapShape(L, generator::mergeShape(*GetShape(L), *GetShape(L, 2), *GetShape(L, 3), *GetShape(L, 4)));
						break;
					default:
						return luaL_error(L, "Merge must take from 1 to 3 extra shapes");
					} // shape, shape2, ..., merged
				}
			}, {
				"Repeat", [](lua_State * L)
				{
					return WrapShape(L, generator::repeatShape(*GetShape(L), luaL_checkint(L, 2), GetVec2(L, 3))); // shape, count, delta, repeated
				}
			}, {
				"Rotate", [](lua_State * L)
				{
					return WrapShape(L, generator::rotateShape(*GetShape(L), luaL_checknumber(L, 2))); // shape, angle, rotated
				}
			}, {
				"Scale", [](lua_State * L)
				{
					return WrapShape(L, generator::scaleShape(*GetShape(L), GetVec2(L, 2))); // shape, v, scaled
				}
			}, {
				"Subdivide", [](lua_State * L)
				{
					return WrapShape(L, generator::SubdivideShape<generator::AnyShape>(*GetShape(L))); // shape, count, subdivided
				}
			}, {
				"Transform", [](lua_State * L)
				{
					luaL_checktype(L, 2, LUA_TFUNCTION);

					void * key;

					WrapShape(L, generator::transformShape(*GetShape(L), [L, &key](generator::ShapeVertex & v) {
						Transform(L, v, key);
					})); // shape, xform, transformed

					return AddTransform(L, ValueCount, &key);
				}
			}, {
				"Translate", [](lua_State * L)
				{
					return WrapShape(L, generator::translateShape(*GetShape(L), GetVec2(L, 2))); // shape, v, translated
				}
			}, {
				"Vertices", [](lua_State * L)
				{
					return WrapShapeVertexGenerator(L, GetShape(L)->vertices()); // shape, generator
				}
			},
			{ nullptr, nullptr }
		};

		luaL_register(L, nullptr, funcs);

		//
		//
		//

		MakeMeshVertexBox(L); // shape, shape_mt, vertex_box

		lua_pushcclosure(L, [](lua_State * L) {
			generator::AnyShape * shape = GetShape(L);

			luaL_checktype(L, 2, LUA_TFUNCTION);
			lua_pushvalue(L, lua_upvalueindex(1)); // shape, func, vertex_box

			generator::ShapeVertex ** box = GetVertexBox(L, -1);

			for (auto gen = shape->vertices(); !gen.done(); gen.next())
			{
				generator::ShapeVertex vert = gen.generate();
				*box = &vert;

				lua_pushvalue(L, 2); // shape, func, vertex_box, func
				lua_pushvalue(L, -1); // shape, func, vertex_box, func, vertex_box

				if (lua_pcall(L, 1, 0, 0) != 0) // shape, func, vertex_box[, err]
				{
					CORONA_LOG_ERROR(
						"Error with vertex = { position = (%.3g, %.3g), tangent = (%.3g, %.3g), texCoord = %.3g: %s",
						vert.position[0], vert.position[1], vert.tangent[0], vert.tangent[1], vert.texCoord,
						lua_isstring(L, -1) ? lua_tostring(L, -1) : "?"
					);

					lua_pop(L, 1); // shape, func, vertex_box
				}
			}

			*box = nullptr;

			return 0;
		}, 1); // shape, shape_mt, ForEachVertex; ForEachVertex.upvalue = vertex_box
		lua_setfield(L, -2, "ForEachVertex"); // shape, shape_mt = { ..., ForEachVertex = ForEachVertex }
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

//
//
//

template<int D> int BezierShape (lua_State * L, int segments)
{
	gml::dvec2 control[D];

	for (int i = 1; i <= D; ++i, lua_pop(L, 1))
	{
		lua_rawgeti(L, 1, i); // control[, params], v

		control[i - 1] = LuaXS::GetArg<gml::dvec2>(L, -1);
	}

	return WrapShape(L, generator::BezierShape<D>{control, segments}); // [params, ]bezier
}

//
//
//

void add_shapes (lua_State * L)
{
	luaL_Reg funcs[] = {
		{
			"createBezierShape", [](lua_State * L)
			{
				if (lua_istable(L, 1))
				{
					lua_getfield(L, 1, "control"); // params, control
					lua_insert(L, 1); // control, params
				}

				luaL_checktype(L, 1, LUA_TTABLE);

				size_t rn = lua_objlen(L, 1);

				luaL_argcheck(L, rn > 1 && rn <= 10, 1, "Bezier shape must have count >= 2 and <= 8");

				LuaXS::Options opts{L, 2};

				int segments = 16;

				opts.NV_PAIR(segments);

				switch (rn)
				{
				case 2:
					return BezierShape<2>(L, segments);
				case 3:
					return BezierShape<3>(L, segments);
				case 4:
					return BezierShape<4>(L, segments);
				case 5:
					return BezierShape<5>(L, segments);
				case 6:
					return BezierShape<6>(L, segments);
				case 7:
					return BezierShape<7>(L, segments);
				case 8:
					return BezierShape<8>(L, segments);
				default:
					return luaL_error(L, "Invalid Bezier shape count");
				} // control[, params], bezier
			}
		}, {
			"createCircleShape", [](lua_State * L)
			{
				LuaXS::Options opts{L, 1};
				
				double radius = 1.0, start = 0.0, sweep = gml::radians(360.0);
				int segments = 32;

				opts.NV_PAIR(radius)
					.NV_PAIR(start)
					.NV_PAIR(sweep)
					.NV_PAIR(segments);

				return WrapShape(L, generator::CircleShape(radius, segments, start, sweep)); // [params, ]circle
			}
		}, {
			"createGridShape", [](lua_State * L)
			{
				LuaXS::Options opts{L, 1};

				gml::dvec2 size = {1.0, 1.0};
				gml::ivec2 segments = {4, 4}, subSegments = {2, 2};

				opts.NV_PAIR(size)
					.NV_PAIR(segments)
					.NV_PAIR(subSegments);

				return WrapShape(L, generator::GridShape(size, segments, subSegments)); // [params, ]grid
			}
		}, {
			"createLineShape", [](lua_State * L)
			{
				LuaXS::Options opts{L, 1};

				gml::dvec2 start = {0.0, -1.0}, end = {0.0, 1.0};
				int segments = 8;

				opts.NV_PAIR(start)
					.NV_PAIR(end)
					.NV_PAIR(segments);

				return WrapShape(L, generator::LineShape(start, end, segments)); // [params, ]line
			}
		}, {
			"createParametricShape", [](lua_State * L)
			{
				if (lua_istable(L, 1))
				{
					lua_getfield(L, 1, "func"); // params, func
					lua_insert(L, 1); // func, params
				}

				luaL_checktype(L, 2, LUA_TFUNCTION);

				LuaXS::Options opts{L, 2};

				int segments = 16;

				opts.NV_PAIR(segments);
				
				void * key;

				WrapShape(L, generator::ParametricShape([L, &key](double t) {
					lua_pushnumber(L, t); // ..., u

					generator::ShapeVertex v;

					Transform(L, v, key, "parametricShape", 1); // ...

					return v;
				}, segments)); // [params, ]parametric

				return AddTransform(L, ValueCount, &key);
			}
		}, {
			"createRectangleShape", [](lua_State * L)
			{
				LuaXS::Options opts{L, 1};

				gml::dvec2 size = gml::dvec2{1.0, 1.0};
				gml::ivec2 segments = gml::ivec2{8, 8};

				opts.NV_PAIR(size)
					.NV_PAIR(segments);

				return WrapShape(L, generator::RectangleShape(size, segments)); // [params, ]rect
			}
		}, {
			"createRoundedRectangleShape", [](lua_State * L)
			{
				LuaXS::Options opts{L, 1};

				gml::dvec2 size = {0.75, 0.75};
				gml::ivec2 segments = {8, 8};
				double radius = 0.25;
				int slices = 4;

				opts.NV_PAIR(size)
					.NV_PAIR(segments)
					.NV_PAIR(radius)
					.NV_PAIR(slices);

				return WrapShape(L, generator::RoundedRectangleShape(radius, size, slices, segments)); // [params, ]rect
			}
		},
		{ nullptr, nullptr }
	};

	luaL_register(L, nullptr, funcs);
}