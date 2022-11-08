// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation; either
// version 2.1 of the License, or (at your option) any later version.

#include "common.h"
#include "transform.h"

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

static const char * sKeys[] = { "nx", "ny", "nz", "px", "py", "pz", "tx", "ty", "tz", "u", nullptr };

//
//
//

template<> const char * const * NameList<generator::PathVertex> () { return sKeys; }
template<> const char * TransformKind<generator::PathVertex>() { return "path"; }

static const size_t ValueCount = sizeof(sKeys) / sizeof(sKeys[0]) - 1;

template<> double ** Values (generator::PathVertex & v)
{
	static double * sValues[ValueCount];

	sValues[0] = &v.normal[0];
	sValues[1] = &v.normal[1];
	sValues[2] = &v.normal[2];
	sValues[3] = &v.position[0];
	sValues[4] = &v.position[1];
	sValues[5] = &v.position[2];
	sValues[6] = &v.tangent[0];
	sValues[7] = &v.tangent[1];
	sValues[8] = &v.tangent[2];
	sValues[9] = &v.texCoord;

	return sValues;
}

//
//
//

static generator::PathVertex ** GetVertexBox (lua_State * L, int arg = 1)
{
	return LuaXS::CheckUD<generator::PathVertex *>(L, arg, PATH_VERTEX_NAME);
}

//
//
//

int WrapPath (lua_State * L, generator::AnyPath && path)
{
	LuaXS::NewTyped<generator::AnyPath>(L, std::move(path)); // path
	LuaXS::AttachMethods(L, PATH_NAME, [](lua_State * L) {
		luaL_Reg funcs[] = {
			{
				"AxisSwap", [](lua_State * L)
				{
					return WrapPath(L, generator::axisSwapPath(*GetPath(L), GetAxis(L, 2), GetAxis(L, 3), GetAxis(L, 4))); // path, xaxis, yaxis, zaxis, swapped
				}
			}, {
				"Edges", [](lua_State * L)
				{
					return WrapEdgeGenerator(L, GetPath(L)->edges()); // path, generator
				}
			}, {
				"Flip", [](lua_State * L)
				{
					return WrapPath(L, generator::flipPath(*GetPath(L))); // path, flipped
				}
			}, {
				"ForEachEdge", [](lua_State * L)
				{
					luaL_checktype(L, 2, LUA_TFUNCTION);

					for (auto && edge : GetPath(L)->edges())
					{
						lua_pushvalue(L, 2); // path, func, func
						lua_pushinteger(L, edge.vertices[0] + 1); // path, func, func, i1
						lua_pushinteger(L, edge.vertices[1] + 1); // path, func, func, i1, i2

						if (lua_pcall(L, 2, 0, 0) != 0) // path, func[, err]
						{
							const char * err = lua_isstring(L, -1) ? lua_tostring(L, -1) : "?";
							
							CORONA_LOG_ERROR("Error with edge (%i, %i): %s", edge.vertices[0] + 1, edge.vertices[1] + 1, err);

							lua_pop(L, 1); // path, func
						}
					}

					return 0;
				}
			}, {
				"__gc", LuaXS::TypedGC<generator::AnyPath>
			}, {
				"Merge", [](lua_State * L)
				{
					switch (lua_gettop(L))
					{
					case 2:
						return WrapPath(L, generator::mergePath(*GetPath(L), *GetPath(L, 2)));
						break;
					case 3:
						return WrapPath(L, generator::mergePath(*GetPath(L), *GetPath(L, 2), *GetPath(L, 3)));
						break;
					case 4:
						return WrapPath(L, generator::mergePath(*GetPath(L), *GetPath(L, 2), *GetPath(L, 3), *GetPath(L, 4)));
						break;
					default:
						return luaL_error(L, "Merge must take from 1 to 3 extra paths");
					} // path, path2, ..., merged
				}
			}, {
				"Repeat", [](lua_State * L)
				{
					return WrapPath(L, generator::repeatPath(*GetPath(L), luaL_checkint(L, 2), GetVec3(L, 3))); // path, count, delta, repeated
				}
			}, {
				"Rotate", [](lua_State * L)
				{
					if (lua_istable(L, 2))
					{
						lua_getfield(L, 2, "x"); // path, q, x
						lua_getfield(L, 2, "y"); // path, q, x, y
						lua_getfield(L, 2, "z"); // path, q, x, y, z
						lua_getfield(L, 2, "w"); // path, q, x, y, z, w

						gml::dquat q;

						for (int i = 0; i < 3; ++i) q.imag[i] = luaL_checknumber(L, -4 + i);

						q.real = luaL_checknumber(L, -1);

						return WrapPath(L, generator::rotatePath(*GetPath(L), q)); // path, q, x, y, z, w, rotated
					}

					else
					{
						double angle = luaL_checknumber(L, 2);

						if (lua_istable(L, 3)) return WrapPath(L, generator::rotatePath(*GetPath(L), angle, GetVec3(L, 3))); // path, angle, axis, rotated
						else return WrapPath(L, generator::rotatePath(*GetPath(L), angle, GetAxis(L, 3))); // path, angle, axis, rotated
					}
				}
			}, {
				"Scale", [](lua_State * L)
				{
					return WrapPath(L, generator::scalePath(*GetPath(L), GetVec3(L, 2))); // path, v, scaled
				}
			}, {
				"Subdivide", [](lua_State * L)
				{
					return WrapPath(L, generator::subdividePath(*GetPath(L))); // path, subdivided
				}
			}, {
				"Transform", [](lua_State * L)
				{
					luaL_checktype(L, 2, LUA_TFUNCTION);

					void * key;

					WrapPath(L, generator::transformPath(*GetPath(L), [L, &key](generator::PathVertex & v) {
						Transform(L, v, key);
					})); // path, xform, transformed

					return AddTransform(L, ValueCount, &key);
				}
			}, {
				"Translate", [](lua_State * L)
				{
					return WrapPath(L, generator::translatePath(*GetPath(L), GetVec3(L, 2))); // path, v, translated
				}
			}, {
				"Vertices", [](lua_State * L)
				{
					return WrapPathVertexGenerator(L, GetPath(L)->vertices()); // path, generator
				}
			},
			{ nullptr, nullptr }
		};

		luaL_register(L, nullptr, funcs);

		//
		//
		//

		MakePathVertexBox(L); // path, path_mt, vertex_box

		lua_pushcclosure(L, [](lua_State * L) {
			generator::AnyPath * path = GetPath(L);

			luaL_checktype(L, 2, LUA_TFUNCTION);
			lua_pushvalue(L, lua_upvalueindex(1)); // path, func, vertex_box

			generator::PathVertex ** box = GetVertexBox(L, -1);

			for (auto gen = path->vertices(); !gen.done(); gen.next())
			{
				generator::PathVertex vert = gen.generate();
				*box = &vert;

				lua_pushvalue(L, 2); // path, func, vertex_box, func
				lua_pushvalue(L, -1); // path, func, vertex_box, func, vertex_box

				if (lua_pcall(L, 1, 0, 0) != 0) // path, func, vertex_box[, err]
				{
					CORONA_LOG_ERROR(
						"Error with vertex = { normal = (%.3g, %.3g, %.3g), position = (%.3g, %.3g, %.3g), tangent = (%.3g, %.3g, %.3g), texCoord = %.3g: %s",
						vert.normal[0], vert.normal[1], vert.normal[2], vert.position[0], vert.position[1], vert.position[2], vert.tangent[0], vert.tangent[1], vert.tangent[2], vert.texCoord,
						lua_isstring(L, -1) ? lua_tostring(L, -1) : "?"
					);

					lua_pop(L, 1); // path, func, vertex_box
				}
			}

			*box = nullptr;

			return 0;
		}, 1); // path, path_mt, ForEachVertex; ForEachVertex.upvalue = vertex_box
		lua_setfield(L, -2, "ForEachVertex"); // path, path_mt = { ..., ForEachVertex = ForEachVertex }
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

//
//
//

void add_paths (lua_State * L)
{
	luaL_Reg funcs[] = {
		{
			"createEmptyPath", [](lua_State * L)
			{
				return WrapPath(L, generator::EmptyPath()); // [params, ]empty
			}
		}, {
			"createHelixPath", [](lua_State * L)
			{
				LuaXS::Options opts{L, 1};

				double radius = 1.0, size = 1.0, start = 0.0, sweep = gml::radians(720.0);
				int segments = 32;

				opts.NV_PAIR(radius)
					.NV_PAIR(size)
					.NV_PAIR(start)
					.NV_PAIR(sweep)
					.NV_PAIR(segments);

				return WrapPath(L, generator::HelixPath(radius, size, segments, start, sweep)); // [params, ]helix
			}
		}, {
			"createKnotPath", [](lua_State * L)
			{
				LuaXS::Options opts{L, 1};

				int p = 2, q = 3, segments = 96;

				opts.NV_PAIR(p)
					.NV_PAIR(q)
					.NV_PAIR(segments);

				return WrapPath(L, generator::KnotPath(p, q, segments)); // [params, ]knot
			}
		}, {
			"createLinePath", [](lua_State * L)
			{
				LuaXS::Options opts{L, 1};

				gml::dvec3 start = {0.0, 0.0, -1.0}, end = {0.0, 0.0, 1.0}, normal = {1.0, 0.0, 0.0};
				int segments = 8;

				opts.NV_PAIR(start)
					.NV_PAIR(end)
					.NV_PAIR(normal)
					.NV_PAIR(segments);

				return WrapPath(L, generator::LinePath(start, end, normal, segments)); // [params, ]box
			}
		}, {
			"createParametricPath", [](lua_State * L)
			{
				if (lua_istable(L, 1))
				{
					lua_getfield(L, 1, "func"); // params, func
					lua_insert(L, 1); // func, params
				}

				luaL_checktype(L, 1, LUA_TFUNCTION);

				LuaXS::Options opts{L, 2};

				int segments = 16;

				opts.NV_PAIR(segments);

				void * key;

				WrapPath(L, generator::ParametricPath([L, &key](double t) {
					lua_pushnumber(L, t); // ..., u

					generator::PathVertex v;

					Transform(L, v, key, "parametricPath", 1); // ...

					return v;
				}, segments)); // [params, ]parametric

				return AddTransform(L, ValueCount, &key);
			}
		},
		{ nullptr, nullptr }
	};

	luaL_register(L, nullptr, funcs);
}