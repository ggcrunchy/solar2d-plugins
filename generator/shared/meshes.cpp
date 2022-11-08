// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation; either
// version 2.1 of the License, or (at your option) any later version.

#include "common.h"
#include "transform.h"
#include <vector>

//
//
//

#define MESH_NAME "generator.AnyMesh"
#define MESH_VERTEX_NAME "generator.MeshVertex"

//
//
//

generator::AnyMesh * GetMesh (lua_State * L, int arg)
{
	return LuaXS::CheckUD<generator::AnyMesh>(L, arg, MESH_NAME);
}

//
//
//

static const char * sKeys[] = { "nx", "ny", "nz", "px", "py", "pz", "u", "v", nullptr };

//
//
//

template<> const char * const * NameList<generator::MeshVertex> () { return sKeys; }
template<> const char * TransformKind<generator::MeshVertex>() { return "mesh"; }

static const size_t ValueCount = sizeof(sKeys) / sizeof(sKeys[0]) - 1;

template<> double ** Values (generator::MeshVertex & v)
{
	static double * sValues[ValueCount];

	sValues[0] = &v.normal[0];
	sValues[1] = &v.normal[1];
	sValues[2] = &v.normal[2];
	sValues[3] = &v.position[0];
	sValues[4] = &v.position[1];
	sValues[5] = &v.position[2];
	sValues[6] = &v.texCoord[0];
	sValues[7] = &v.texCoord[1];

	return sValues;
}

//
//
//

static generator::MeshVertex ** GetVertexBox (lua_State * L, int arg = 1)
{
	return LuaXS::CheckUD<generator::MeshVertex *>(L, arg, MESH_VERTEX_NAME);
}

//
//
//

int WrapMesh (lua_State * L, generator::AnyMesh && mesh)
{
	LuaXS::NewTyped<generator::AnyMesh>(L, std::move(mesh)); // mesh
	LuaXS::AttachMethods(L, MESH_NAME, [](lua_State * L) {
		luaL_Reg funcs[] = {
			{
				"AxisSwap", [](lua_State * L)
				{
					return WrapMesh(L, generator::axisSwapMesh(*GetMesh(L), GetAxis(L, 2), GetAxis(L, 3), GetAxis(L, 4))); // mesh, xaxis, yaxis, zaxis, swapped
				}
			}, {
				"Flip", [](lua_State * L)
				{
					return WrapMesh(L, generator::flipMesh(*GetMesh(L))); // mesh, flipped
				}
			}, {
				"ForEachTriangle", [](lua_State * L)
				{
					luaL_checktype(L, 2, LUA_TFUNCTION);

					for (auto && tri : GetMesh(L)->triangles())
					{
						lua_pushvalue(L, 2); // mesh, func, func
						lua_pushinteger(L, tri.vertices[0] + 1); // mesh, func, func, i1
						lua_pushinteger(L, tri.vertices[1] + 1); // mesh, func, func, i1, i2
						lua_pushinteger(L, tri.vertices[2] + 1); // mesh, func, func, i1, i2, i3

						if (lua_pcall(L, 3, 0, 0) != 0) // mesh, func[, err]
						{
							const char * err = lua_isstring(L, -1) ? lua_tostring(L, -1) : "?";
							
							CORONA_LOG_ERROR("Error with triangle (%i, %i, %i): %s", tri.vertices[0] + 1, tri.vertices[1] + 1, tri.vertices[2] + 1, err);

							lua_pop(L, 1); // mesh, func
						}
					}

					return 0;
				}
			}, {
				"__gc", LuaXS::TypedGC<generator::AnyMesh>
			}, {
				"Merge", [](lua_State * L)
				{
					switch (lua_gettop(L))
					{
					case 2:
						return WrapMesh(L, generator::mergeMesh(*GetMesh(L), *GetMesh(L, 2)));
						break;
					case 3:
						return WrapMesh(L, generator::mergeMesh(*GetMesh(L), *GetMesh(L, 2), *GetMesh(L, 3)));
						break;
					case 4:
						return WrapMesh(L, generator::mergeMesh(*GetMesh(L), *GetMesh(L, 2), *GetMesh(L, 3), *GetMesh(L, 4)));
						break;
					default:
						return luaL_error(L, "Merge must take from 1 to 3 extra meshes");
					} // mesh, mesh2, ..., merged
				}
			}, {
				"Repeat", [](lua_State * L)
				{
					return WrapMesh(L, generator::repeatMesh(*GetMesh(L), luaL_checkint(L, 2), GetVec3(L, 3))); // mesh, count, delta, repeated
				}
			}, {
				"Rotate", [](lua_State * L)
				{
					if (lua_istable(L, 2))
					{
						lua_getfield(L, 2, "x"); // mesh, q, x
						lua_getfield(L, 2, "y"); // mesh, q, x, y
						lua_getfield(L, 2, "z"); // mesh, q, x, y, z
						lua_getfield(L, 2, "w"); // mesh, q, x, y, z, w

						gml::dquat q;

						for (int i = 0; i < 3; ++i) q.imag[i] = luaL_checknumber(L, -4 + i);

						q.real = luaL_checknumber(L, -1);

						return WrapMesh(L, generator::rotateMesh(*GetMesh(L), q)); // mesh, q, x, y, z, w, rotated
					}

					else
					{
						double angle = luaL_checknumber(L, 2);

						if (lua_istable(L, 3)) return WrapMesh(L, generator::rotateMesh(*GetMesh(L), angle, GetVec3(L, 3))); // mesh, angle, axis, rotated
						else return WrapMesh(L, generator::rotateMesh(*GetMesh(L), angle, GetAxis(L, 3))); // mesh, angle, axis, rotated
					}
				}
			}, {
				"Scale", [](lua_State * L)
				{
					return WrapMesh(L, generator::scaleMesh(*GetMesh(L), GetVec3(L, 2))); // mesh, v, scaled
				}
			}, {
				"Spherify", [](lua_State * L)
				{
					return WrapMesh(L, generator::spherifyMesh(*GetMesh(L), LuaXS::Double(L, 2), LuaXS::Double(L, 3))); // mesh, radius, factor, spherified
				}
			}, {
				"Subdivide", [](lua_State * L)
				{
					switch (luaL_checkint(L, 2))
					{
					case 1:
						return WrapMesh(L, generator::SubdivideMesh<generator::AnyMesh, 1>(*GetMesh(L)));
						break;
					case 2:
						return WrapMesh(L, generator::SubdivideMesh<generator::AnyMesh, 2>(*GetMesh(L)));
						break;
					case 3:
						return WrapMesh(L, generator::SubdivideMesh<generator::AnyMesh, 3>(*GetMesh(L)));
						break;
					case 4:
						return WrapMesh(L, generator::SubdivideMesh<generator::AnyMesh, 4>(*GetMesh(L)));
						break;
					default:
						return luaL_error(L, "Subdivision count must be positive and < 5");
					} // mesh, count, subdivided
				}
			}, {
				"Transform", [](lua_State * L)
				{
					luaL_checktype(L, 2, LUA_TFUNCTION);

					void * key;

					WrapMesh(L, generator::transformMesh(*GetMesh(L), [L, &key](generator::MeshVertex & v) {
						Transform(L, v, key);
					})); // mesh, xform, transformed

					return AddTransform(L, ValueCount, &key);
				}
			}, {
				"Translate", [](lua_State * L)
				{
					return WrapMesh(L, generator::translateMesh(*GetMesh(L), GetVec3(L, 2))); // mesh, v, translated
				}
			}, {
				"Triangles", [](lua_State * L)
				{
					return WrapTriangleGenerator(L, GetMesh(L)->triangles()); // mesh, generator
				}
			}, {
				"UvFlip", [](lua_State * L)
				{
					return WrapMesh(L, generator::uvFlipMesh(*GetMesh(L), lua_toboolean(L, 2), lua_toboolean(L, 3))); // mesh, flipped
				}
			}, {
				"UvSwap", [](lua_State * L)
				{
					return WrapMesh(L, generator::uvSwapMesh(*GetMesh(L))); // mesh, u, v, swapped
				}
			}, {
				"Vertices", [](lua_State * L)
				{
					return WrapMeshVertexGenerator(L, GetMesh(L)->vertices()); // mesh, generator
				}
			}, {
				"WriteOBJ", [](lua_State * L)
				{
					generator::ObjWriter writer;
					
					writer.writeMesh(*GetMesh(L));

					lua_pushstring(L, writer.str().c_str()); // mesh, obj_str

					return 1;
				}
			},
			{ nullptr, nullptr }
		};

		luaL_register(L, nullptr, funcs);

		//
		//
		//

		MakeMeshVertexBox(L); // mesh, mesh_mt, vertex_box

		lua_pushcclosure(L, [](lua_State * L) {
			generator::AnyMesh * mesh = GetMesh(L);

			luaL_checktype(L, 2, LUA_TFUNCTION);
			lua_pushvalue(L, lua_upvalueindex(1)); // mesh, func, vertex_box

			generator::MeshVertex ** box = GetVertexBox(L, -1);

			for (auto gen = mesh->vertices(); !gen.done(); gen.next())
			{
				generator::MeshVertex vert = gen.generate();
				*box = &vert;

				lua_pushvalue(L, 2); // mesh, func, vertex_box, func
				lua_pushvalue(L, -1); // mesh, func, vertex_box, func, vertex_box

				if (lua_pcall(L, 1, 0, 0) != 0) // mesh, func, vertex_box[, err]
				{
					CORONA_LOG_ERROR(
						"Error with vertex = { normal = (%.3g, %.3g, %.3g), position = (%.3g, %.3g, %.3g), texCoord = (%.3g, %.3g): %s",
						vert.normal[0], vert.normal[1], vert.normal[2], vert.position[0], vert.position[1], vert.position[2], vert.texCoord[0], vert.texCoord[1],
						lua_isstring(L, -1) ? lua_tostring(L, -1) : "?"
					);

					lua_pop(L, 1); // mesh, func, vertex_box
				}
			}

			*box = nullptr;

			return 0;
		}, 1); // mesh, mesh_mt, ForEachVertex; ForEachVertex.upvalue = vertex_box
		lua_setfield(L, -2, "ForEachVertex"); // mesh, mesh_mt = { ..., ForEachVertex = ForEachVertex }
	});

	return 1;
}

//
//
//

static generator::MeshVertex * GetMeshVertex (lua_State * L, int arg)
{
	generator::MeshVertex ** box = GetVertexBox(L, arg);

	luaL_argcheck(L, *box, arg, "Mesh vertex is not active");

	return *box;
}

//
//
//

void MakeMeshVertexBox (lua_State * L, generator::MeshVertex * vertex)
{
	*LuaXS::NewTyped<generator::MeshVertex *>(L) = vertex; // ..., box
	LuaXS::AttachMethods(L, MESH_VERTEX_NAME, [](lua_State * L) {
		luaL_Reg funcs[] = {
			{
				"GetNormal", [](lua_State * L)
				{
					const gml::dvec3 & n = GetMeshVertex(L)->normal;

					lua_pushnumber(L, n[0]); // vertex, x
					lua_pushnumber(L, n[1]); // vertex, x, y
					lua_pushnumber(L, n[2]); // vertex, x, y, z

					return 3;
				}
			}, {
				"GetPosition", [](lua_State * L)
				{
					const gml::dvec3 & p = GetMeshVertex(L)->position;

					lua_pushnumber(L, p[0]); // vertex, x
					lua_pushnumber(L, p[1]); // vertex, x, y
					lua_pushnumber(L, p[2]); // vertex, x, y, z

					return 3;
				}
			}, {
				"GetTexCoord", [](lua_State * L)
				{
					const gml::dvec2 & t = GetMeshVertex(L)->texCoord;

					lua_pushnumber(L, t[0]); // vertex, u
					lua_pushnumber(L, t[1]); // vertex, u, v

					return 2;
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

template<int D1, int D0> int BezierMesh (lua_State * L, const gml::ivec2 & segments)
{
	gml::dvec3 control[D0][D1];

	for (int i = 1; i <= D0; ++i, lua_pop(L, 1))
	{
		lua_rawgeti(L, 1, i); // control[, params], row

		for (int j = 1; j <= D1; ++j, lua_pop(L, 1))
		{
			lua_rawgeti(L, -1, j); // control[, params], row, v

			control[i - 1][j - 1] = LuaXS::GetArg<gml::dvec3>(L, -1);
		}
	}

	return WrapMesh(L, generator::BezierMesh<D1, D0>{control, segments}); // [params, ]bezier
}

//
//
//

void add_meshes (lua_State * L)
{
	luaL_Reg funcs[] = {
		{
			"createBezierMesh", [](lua_State * L)
			{
				luaL_checktype(L, 1, LUA_TTABLE);
				lua_getfield(L, 1, "control"); // params, control?

				if (lua_istable(L, -1)) lua_insert(L, 1); // control, params

				size_t rn = lua_objlen(L, 1);

				luaL_argcheck(L, rn > 1 && rn <= 5, 1, "Bezier mesh must have row count >= 2 and <= 5");

				size_t cn1;

				for (size_t i = 1; i <= rn; ++i, lua_pop(L, 1))
				{
					lua_rawgeti(L, 1, i); // control[, params], row
					luaL_checktype(L, -1, LUA_TTABLE);

					size_t cn = lua_objlen(L, -1);

					if (i == 1)
					{
						luaL_argcheck(L, cn > 1 && cn <= 5, -1, "Bezier mesh must have column count >= 2 and <= 5");

						cn1 = cn;
					}

					else luaL_argcheck(L, cn == cn1, -1, "Bezier mesh column count mismatch");
				}

				LuaXS::Options opts{L, 2};

				gml::ivec2 segments = {16, 16};

				opts.NV_PAIR(segments);

				if (2 == rn)
				{
					if (2 == cn1) return BezierMesh<2, 2>(L, segments);
					else if (3 == cn1) return BezierMesh<2, 3>(L, segments);
					else if (4 == cn1) return BezierMesh<2, 4>(L, segments);
					else return BezierMesh<2, 5>(L, segments);
				}

				else if (3 == rn)
				{
					if (2 == cn1) return BezierMesh<3, 2>(L, segments);
					else if (3 == cn1) return BezierMesh<3, 3>(L, segments);
					else if (4 == cn1) return BezierMesh<3, 4>(L, segments);
					else return BezierMesh<3, 5>(L, segments);
				}

				else if (4 == rn)
				{
					if (2 == cn1) return BezierMesh<4, 2>(L, segments);
					else if (3 == cn1) return BezierMesh<4, 3>(L, segments);
					else if (4 == cn1) return BezierMesh<4, 4>(L, segments);
					else return BezierMesh<4, 5>(L, segments);
				}

				else
				{
					if (2 == cn1) return BezierMesh<5, 2>(L, segments);
					else if (3 == cn1) return BezierMesh<5, 3>(L, segments);
					else if (4 == cn1) return BezierMesh<5, 4>(L, segments);
					else return BezierMesh<5, 5>(L, segments);
				} // control[, params], bezier
			}
		}, {
			"createBoxMesh", [](lua_State * L)
			{
				LuaXS::Options opts{L, 1};

				gml::dvec3 size = {1.0, 1.0, 1.0};
				gml::ivec3 segments = {8, 8, 8};

				opts.NV_PAIR(size)
					.NV_PAIR(segments);

				return WrapMesh(L, generator::BoxMesh(size, segments)); // [params, ]box
			}
		}, {
			"createCappedConeMesh", [](lua_State * L)
			{
				LuaXS::Options opts{L, 1};

				double radius = 1.0, size = 1.0, start = 0.0, sweep = gml::radians(360.0);
				int slices = 32, segments = 8, rings = 4;

				opts.NV_PAIR(radius)
					.NV_PAIR(size)
					.NV_PAIR(start)
					.NV_PAIR(sweep)
					.NV_PAIR(slices)
					.NV_PAIR(segments)
					.NV_PAIR(rings);

				return WrapMesh(L, generator::CappedConeMesh(radius, size, slices, segments, rings, start, sweep)); // [params, ]box
			}
		}, {
			"createCappedCylinderMesh", [](lua_State * L)
			{
				LuaXS::Options opts{L, 1};

				double radius = 1.0, size = 1.0, start = 0.0, sweep = gml::radians(360.0);
				int slices = 32, segments = 8, rings = 4;

				opts.NV_PAIR(radius)
					.NV_PAIR(size)
					.NV_PAIR(start)
					.NV_PAIR(sweep)
					.NV_PAIR(slices)
					.NV_PAIR(segments)
					.NV_PAIR(rings);

				return WrapMesh(L, generator::CappedCylinderMesh(radius, size, slices, segments, rings, start, sweep)); // [params, ]box
			}
		}, {
			"createCappedTubeMesh", [](lua_State * L)
			{
				LuaXS::Options opts{L, 1};

				double radius = 1.0, innerRadius = 0.75, size = 1.0, start = 0.0, sweep = gml::radians(360.0);
				int slices = 32, segments = 8, rings = 1;

				opts.NV_PAIR(radius)
					.NV_PAIR(innerRadius)
					.NV_PAIR(size)
					.NV_PAIR(start)
					.NV_PAIR(sweep)
					.NV_PAIR(slices)
					.NV_PAIR(segments)
					.NV_PAIR(rings);

				return WrapMesh(L, generator::CappedTubeMesh(radius, innerRadius, size, slices, segments, rings, start, sweep)); // [params, ]box
			}
		}, {
			"createCapsuleMesh", [](lua_State * L)
			{
				LuaXS::Options opts{L, 1};

				double radius = 1.0, size = 0.5, start = 0.0, sweep = gml::radians(360.0);
				int slices = 32, segments = 4, rings = 8;

				opts.NV_PAIR(radius)
					.NV_PAIR(size)
					.NV_PAIR(start)
					.NV_PAIR(sweep)
					.NV_PAIR(slices)
					.NV_PAIR(segments)
					.NV_PAIR(rings);

				return WrapMesh(L, generator::CapsuleMesh(radius, size, slices, segments, rings, start, sweep)); // [params, ]capsule
			}
		}, {
			"createConeMesh", [](lua_State * L)
			{
				LuaXS::Options opts{L, 1};

				double radius = 1.0, size = 1.0, start = 0.0, sweep = gml::radians(360.0);
				int slices = 32, segments = 8;

				opts.NV_PAIR(radius)
					.NV_PAIR(size)
					.NV_PAIR(start)
					.NV_PAIR(sweep)
					.NV_PAIR(slices)
					.NV_PAIR(segments);

				return WrapMesh(L, generator::ConeMesh(radius, size, slices, segments, start, sweep)); // [params, ]cone
			}
		}, {
			"createConvexPolygonMesh", [](lua_State * L)
			{
				LuaXS::Options opts{L, 1};
				std::vector<gml::dvec2> v2;
				std::vector<gml::dvec3> v3;

				double radius = 1.0;
				int sides = 5, segments = 4, rings = 4;

				opts.WithFieldDo("vec2List", [L, &v2, &segments, &rings](){
						segments = rings = 1;

						for (size_t i = 1, n = lua_objlen(L, -1); i <= n; ++i) v2.push_back(LuaXS::GetArg<gml::dvec2>(L));
					})
					.WithFieldDo("vec3List", [L, &v2, &v3, &segments, &rings](){
						luaL_argcheck(L, v2.empty(), 1, "Attempt to populate list of vec3s, after already doing so for vec2s");
						
						segments = rings = 1;

						for (size_t i = 1, n = lua_objlen(L, -1); i <= n; ++i) v3.push_back(LuaXS::GetArg<gml::dvec3>(L));
					})
					.NV_PAIR(radius)
					.NV_PAIR(sides)
					.NV_PAIR(segments)
					.NV_PAIR(rings);

				if (!v2.empty()) return WrapMesh(L, generator::ConvexPolygonMesh(v2, segments, rings)); // [params, ]mesh
				else if (!v3.empty()) return WrapMesh(L, generator::ConvexPolygonMesh(v3, segments, rings)); // [params, ]mesh
				else return WrapMesh(L, generator::ConvexPolygonMesh(radius, sides, segments, rings)); // [params, ]mesh
			}
		}, {
			"createCylinderMesh", [](lua_State * L)
			{
				LuaXS::Options opts{L, 1};

				double radius = 1.0, size = 1.0, start = 0.0, sweep = gml::radians(360.0);
				int slices = 32, segments = 8;

				opts.NV_PAIR(radius)
					.NV_PAIR(size)
					.NV_PAIR(start)
					.NV_PAIR(sweep)
					.NV_PAIR(slices)
					.NV_PAIR(segments);

				return WrapMesh(L, generator::CylinderMesh(radius, size, slices, segments, start, sweep)); // [params, ]cylinder
			}
		}, {
			"createDiskMesh", [](lua_State * L)
			{
				LuaXS::Options opts{L, 1};

				double radius = 1.0, innerRadius = 0.0, start = 0.0, sweep = gml::radians(360.0);
				int slices = 32, rings = 4;

				opts.NV_PAIR(radius)
					.NV_PAIR(innerRadius)
					.NV_PAIR(start)
					.NV_PAIR(sweep)
					.NV_PAIR(slices)
					.NV_PAIR(rings);

				return WrapMesh(L, generator::DiskMesh(radius, innerRadius, slices, rings, start, sweep)); // [params, ]disk
			}
		}, {
			"createDodecahedronMesh", [](lua_State * L)
			{
				LuaXS::Options opts{L, 1};

				double radius = 1.0;
				int segments = 1, rings = 1;

				opts.NV_PAIR(radius)
					.NV_PAIR(segments)
					.NV_PAIR(rings);

				return WrapMesh(L, generator::DodecahedronMesh(radius, segments, rings)); // [params, ]dodeca
			}
		}, {
			"createEmptyMesh", [](lua_State * L)
			{
				return WrapMesh(L, generator::EmptyMesh()); // [params, ]empty
			}
		}, {
			"createIcosahedronMesh", [](lua_State * L)
			{
				LuaXS::Options opts{L, 1};

				double radius = 1.0;
				int segments = 1;

				opts.NV_PAIR(radius)
					.NV_PAIR(segments);

				return WrapMesh(L, generator::IcosahedronMesh(radius, segments)); // [params, ]icosa
			}
		}, {
			"createIcoSphereMesh", [](lua_State * L)
			{
				LuaXS::Options opts{L, 1};

				double radius = 1.0;
				int segments = 4;

				opts.NV_PAIR(radius)
					.NV_PAIR(segments);

				return WrapMesh(L, generator::IcoSphereMesh(radius, segments)); // [params, ]sphere
			}
		}, {
			"createParametricMesh", [](lua_State * L)
			{
				if (lua_istable(L, 1))
				{
					lua_getfield(L, 1, "func"); // params, func
					lua_insert(L, 1); // func, params
				}

				luaL_checktype(L, 1, LUA_TFUNCTION);

				LuaXS::Options opts{L, 2};

				gml::ivec2 segments = {16, 16};

				opts.NV_PAIR(segments);

				void * key;

				WrapMesh(L, generator::ParametricMesh([L, &key](const gml::dvec2 & t) {
					lua_pushnumber(L, t[0]); // ..., u
					lua_pushnumber(L, t[1]); // ..., u, v

					generator::MeshVertex v;

					Transform(L, v, key, "parametricMesh", 2); // ...

					return v;
				}, segments)); // [params, ]parametric

				return AddTransform(L, ValueCount, &key);
			}
		}, {
			"createPlaneMesh", [](lua_State * L)
			{
				LuaXS::Options opts{L, 1};

				gml::dvec2 size = {1.0, 1.0};
				gml::ivec2 segments = {8, 8};

				opts.NV_PAIR(size)
					.NV_PAIR(segments);

				return WrapMesh(L, generator::PlaneMesh(size, segments)); // [params, ]plane
			}
		}, {
			"createRoundedBoxMesh", [](lua_State * L)
			{
				LuaXS::Options opts{L, 1};

				gml::dvec3 size = {0.75, 0.75, 0.75};
				gml::ivec3 segments = {8, 8, 8};
				double radius = 0.25;
				int slices = 4;

				opts.NV_PAIR(size)
					.NV_PAIR(segments)
					.NV_PAIR(radius)
					.NV_PAIR(slices);

				return WrapMesh(L, generator::RoundedBoxMesh(radius, size, slices, segments)); // [params, ]box
			}
		}, {
			"createSphereMesh", [](lua_State * L)
			{
				LuaXS::Options opts{L, 1};

				double radius = 1.0, sliceStart = 0.0, sliceSweep = gml::radians(360.0), segmentStart = 0.0, segmentSweep = gml::radians(180.0);
				int slices = 32, segments = 16;

				opts.NV_PAIR(radius)
					.NV_PAIR(sliceStart)
					.NV_PAIR(sliceSweep)
					.NV_PAIR(segmentStart)
					.NV_PAIR(segmentSweep)
					.NV_PAIR(slices)
					.NV_PAIR(segments);

				return WrapMesh(L, generator::SphereMesh(radius, slices, segments, sliceStart, sliceSweep, segmentStart, segmentSweep)); // [params, ]sphere
			}
		}, {
			"createSphericalConeMesh", [](lua_State * L)
			{
				LuaXS::Options opts{L, 1};

				double radius = 1.0, size = 1.0, start = 0.0, sweep = gml::radians(360.0);
				int slices = 32, segments = 8, rings = 4;

				opts.NV_PAIR(radius)
					.NV_PAIR(size)
					.NV_PAIR(start)
					.NV_PAIR(sweep)
					.NV_PAIR(slices)
					.NV_PAIR(segments)
					.NV_PAIR(rings);

				return WrapMesh(L, generator::SphericalConeMesh(radius, size, slices, segments, rings, start, sweep)); // [params, ]cone
			}
		}, {
			"createSphericalTriangleMesh", [](lua_State * L)
			{
				LuaXS::Options opts{L, 1};
				gml::dvec3 v0, v1, v2;

				bool using_vectors = false;
				double radius = 1.0;
				int segments = 4;

				opts.NV_PAIR(radius)
					.NV_PAIR(segments)
					.WithFieldDo("v0", [L, &v0, &v1, &v2, &using_vectors]() {
						lua_getfield(L, 1, "v1"); // params, v0, v1
						lua_getfield(L, 1, "v2"); // params, v0, v1, v2

						v0 = LuaXS::GetArg<gml::dvec3>(L, -3);
						v1 = LuaXS::GetArg<gml::dvec3>(L, -2);
						v2 = LuaXS::GetArg<gml::dvec3>(L, -1);

						using_vectors = true;
					});

				if (using_vectors) return WrapMesh(L, generator::SphericalTriangleMesh(v0, v1, v2, segments)); // [params, ]triangle
				else return WrapMesh(L, generator::SphericalTriangleMesh(radius, segments)); // [params, ]triangle
			}
		}, {
			"createSpringMesh", [](lua_State * L)
			{
				LuaXS::Options opts{L, 1};

				double minor = 0.25, major = 1.0, size = 1.0, minorStart = 0.0, minorSweep = gml::radians(360.0), majorStart = 0.0, majorSweep = gml::radians(720.0);
				int slices = 8, segments = 32;

				opts.NV_PAIR(minor)
					.NV_PAIR(major)
					.NV_PAIR(size)
					.NV_PAIR(minorStart)
					.NV_PAIR(minorSweep)
					.NV_PAIR(majorStart)
					.NV_PAIR(majorSweep)
					.NV_PAIR(slices)
					.NV_PAIR(segments);

				return WrapMesh(L, generator::SpringMesh(minor, major, size, slices, segments, minorStart, minorSweep, majorStart, majorSweep)); // [params, ]spring
			}
		}, {
			"createTeapotMesh", [](lua_State * L)
			{
				LuaXS::Options opts{L, 1};

				int segments = 8;

				opts.NV_PAIR(segments);

				return WrapMesh(L, generator::TeapotMesh(segments)); // [params, ]teapot
			}
		}, {
			"createTorusMesh", [](lua_State * L)
			{
				LuaXS::Options opts{L, 1};

				double minor = 0.25, major = 1.0, minorStart = 0.0, minorSweep = gml::radians(360.0), majorStart = 0.0, majorSweep = gml::radians(360.0);
				int slices = 16, segments = 32;

				opts.NV_PAIR(minor)
					.NV_PAIR(major)
					.NV_PAIR(minorStart)
					.NV_PAIR(minorSweep)
					.NV_PAIR(majorStart)
					.NV_PAIR(majorSweep)
					.NV_PAIR(slices)
					.NV_PAIR(segments);

				return WrapMesh(L, generator::TorusMesh(minor, major, slices, segments, minorStart, minorSweep, majorStart, majorSweep)); // [params, ]torus
			}
		}, {
			"createTorusKnotMesh", [](lua_State * L)
			{
				LuaXS::Options opts{L, 1};

				int p = 2, q = 3, slices = 8, segments = 96;

				opts.NV_PAIR(p)
					.NV_PAIR(q)
					.NV_PAIR(slices)
					.NV_PAIR(segments);

				return WrapMesh(L, generator::TorusKnotMesh(p, q, slices, segments)); // [params, ]torus_knot
			}
		}, {
			"createTriangleMesh", [](lua_State * L)
			{
				LuaXS::Options opts{L, 1};
				gml::dvec3 v0, v1, v2;

				double radius = 1.0;
				int segments = 4;
				bool using_vectors = false;

				opts.NV_PAIR(radius)
					.NV_PAIR(segments)
					.WithFieldDo("v0", [L, &v0, &v1, &v2, &using_vectors]() {
						lua_getfield(L, 1, "v1"); // params, v0, v1
						lua_getfield(L, 1, "v2"); // params, v0, v1, v2

						v0 = LuaXS::GetArg<gml::dvec3>(L, -3);
						v1 = LuaXS::GetArg<gml::dvec3>(L, -2);
						v2 = LuaXS::GetArg<gml::dvec3>(L, -1);

						using_vectors = true;
					});

				if (using_vectors)  return WrapMesh(L, generator::TriangleMesh(v0, v1, v2, segments)); // [params, ]triangle
				else return WrapMesh(L, generator::TriangleMesh(radius, segments)); // [params, ]triangle
			}
		}, {
			"createTubeMesh", [](lua_State * L)
			{
				LuaXS::Options opts{L, 1};

				double radius = 1.0, innerRadius = 0.75, size = 1.0, start = 0.0, sweep = gml::radians(360.0);
				int slices = 32, segments = 8;

				opts.NV_PAIR(radius)
					.NV_PAIR(innerRadius)
					.NV_PAIR(size)
					.NV_PAIR(start)
					.NV_PAIR(sweep)
					.NV_PAIR(slices)
					.NV_PAIR(segments);

				return WrapMesh(L, generator::TubeMesh(radius, innerRadius, size, slices, segments, start, sweep)); // [params, ]tube
			}
		},
		{ nullptr, nullptr }
	};

	luaL_register(L, nullptr, funcs);
}