// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation; either
// version 2.1 of the License, or (at your option) any later version.

#include "common.h"

//
//
//

static int AddSourceToEnv (lua_State * L, int from, bool extra = false)
{
	from = CoronaLuaNormalize(L, from);

	lua_createtable(L, 0, extra ? 2 : 1); // ..., generator[, box], env

	if (extra)
	{
		lua_insert(L, -2); // generator, env, box
		lua_setfield(L, -2, "box"); // generator, env = { box = box }
	}

	lua_pushvalue(L, from); // ..., generator, env, source
	lua_setfield(L, -2, "from"); // ..., generator, env = { [box, ]source = source }
	lua_setfenv(L, -2); // ..., generator; generator.env = env

	return 1;
}

//
//
//

template<typename T, generator::AnyGenerator<T> * (*getter)(lua_State *)> void AddCommonMethods (lua_State * L)
{
	luaL_Reg funcs[] = {
		{
			"Done", [](lua_State * L)
			{
				lua_pushboolean(L, getter(L)->done()); // generator, done

				return 1;
			}
		}, {
			"Next", [](lua_State * L)
			{
				try {
					getter(L)->next();

					return 0;
				} catch (std::out_of_range & ex) {
					return luaL_error(L, ex.what());
				}
			}
		},
		{ nullptr, nullptr }
	};

	luaL_register(L, nullptr, funcs);
}

//
//
//

#define EDGE_GENERATOR_NAME "generator.AnyEdgeGenerator"

//
//
//

static generator::AnyGenerator<generator::Edge> * GetEdgeGenerator (lua_State * L)
{
	return LuaXS::CheckUD<generator::AnyGenerator<generator::Edge>>(L, 1, EDGE_GENERATOR_NAME);
}

//
//
//

int WrapEdgeGenerator (lua_State * L, generator::AnyGenerator<generator::Edge> && edge_generator, int from)
{
	*LuaXS::AllocTyped<generator::AnyGenerator<generator::Edge>>(L) = std::move(edge_generator); // edge_generator
	LuaXS::AttachMethods(L, EDGE_GENERATOR_NAME, [](lua_State * L) {
		AddCommonMethods<generator::Edge, &GetEdgeGenerator>(L);

		luaL_Reg funcs[] = {
			"Generate", [](lua_State * L)
			{
				try {
					generator::Edge edge = GetEdgeGenerator(L)->generate();

					lua_pushinteger(L, edge.vertices[0] + 1); // edge_generator, i1
					lua_pushinteger(L, edge.vertices[1] + 1); // edge_generator, i1, i2

					return 2;
				} catch (std::out_of_range & ex) {
					return luaL_error(L, ex.what());
				}
			},
			{ nullptr, nullptr }
		};

		luaL_register(L, nullptr, funcs);
	});

	return AddSourceToEnv(L, from);
}

//
//
//

template<typename T> struct GeneratorAndVertex {
	GeneratorAndVertex (generator::AnyGenerator<T> && gen) : mGen(std::move(gen))
	{
	}

	generator::AnyGenerator<T> mGen;
	T mVertex;
};

//
//
//

#define MESH_VERTEX_GENERATOR_NAME "generator.AnyMeshVertexGenerator"

//
//
//

static generator::AnyGenerator<generator::MeshVertex> * GetMeshVertexGenerator (lua_State * L)
{
	return LuaXS::CheckUD<generator::AnyGenerator<generator::MeshVertex>>(L, 1, MESH_VERTEX_GENERATOR_NAME);
}

//
//
//

int WrapMeshVertexGenerator (lua_State * L, generator::AnyGenerator<generator::MeshVertex> && mesh_vertex_generator, int from)
{
	auto * gav = LuaXS::NewTyped<GeneratorAndVertex<generator::MeshVertex>>(L, std::move(mesh_vertex_generator)); // mesh_vertex_generator

	LuaXS::AttachMethods(L, MESH_VERTEX_GENERATOR_NAME, [](lua_State * L) {
		AddCommonMethods<generator::MeshVertex, &GetMeshVertexGenerator>(L);

		luaL_Reg funcs[] = {
			"Generate", [](lua_State * L)
			{
				auto * gen = reinterpret_cast<decltype(gav)>(GetMeshVertexGenerator(L));

				try {
					gen->mVertex = gen->mGen.generate();
				} catch (std::out_of_range & ex) {
					return luaL_error(L, ex.what());
				}

				lua_getfenv(L, 1); // mesh_vertex_generator, env
				lua_getfield(L, -1, "box"); // mesh_vertex_generator, env, box

				return 1;
			},
			{ nullptr, nullptr }
		};

		luaL_register(L, nullptr, funcs);
	});
	
	MakeMeshVertexBox(L, &gav->mVertex); // mesh_vertex_generator, box

	return AddSourceToEnv(L, from, true); // mesh_vertex_generator
}

//
//
//

#define PATH_VERTEX_GENERATOR_NAME "generator.AnyPathVertexGenerator"

//
//
//

static generator::AnyGenerator<generator::PathVertex> * GetPathVertexGenerator (lua_State * L)
{
	return LuaXS::CheckUD<generator::AnyGenerator<generator::PathVertex>>(L, 1, PATH_VERTEX_GENERATOR_NAME);
}

//
//
//

int WrapPathVertexGenerator (lua_State * L, generator::AnyGenerator<generator::PathVertex> && path_vertex_generator, int from)
{
	auto * gav = LuaXS::NewTyped<GeneratorAndVertex<generator::PathVertex>>(L, std::move(path_vertex_generator)); // path_vertex_generator

	LuaXS::AttachMethods(L, PATH_VERTEX_GENERATOR_NAME, [](lua_State * L) {
		AddCommonMethods<generator::PathVertex, &GetPathVertexGenerator>(L);

		luaL_Reg funcs[] = {
			"Generate", [](lua_State * L)
			{
				auto * gen = reinterpret_cast<decltype(gav)>(GetPathVertexGenerator(L));

				try {
					gen->mVertex = gen->mGen.generate();
				} catch (std::out_of_range & ex) {
					return luaL_error(L, ex.what());
				}

				lua_getfenv(L, 1); // path_vertex_generator, env
				lua_getfield(L, -1, "box"); // path_vertex_generator, env, box

				return 1;
			},
			{ nullptr, nullptr }
		};

		luaL_register(L, nullptr, funcs);
	});
	
	MakePathVertexBox(L, &gav->mVertex); // path_vertex_generator, box

	return AddSourceToEnv(L, from, true); // path_vertex_generator
}

//
//
//

#define SHAPE_VERTEX_GENERATOR_NAME "generator.AnyShapeVertexGenerator"

//
//
//

generator::AnyGenerator<generator::ShapeVertex> * GetShapeVertexGenerator (lua_State * L)
{
	return LuaXS::CheckUD<generator::AnyGenerator<generator::ShapeVertex>>(L, 1, SHAPE_VERTEX_GENERATOR_NAME);
}

//
//
//

int WrapShapeVertexGenerator (lua_State * L, generator::AnyGenerator<generator::ShapeVertex> && shape_vertex_generator, int from)
{
	auto * gav = LuaXS::NewTyped<GeneratorAndVertex<generator::ShapeVertex>>(L, std::move(shape_vertex_generator)); // shape_vertex_generator

	LuaXS::AttachMethods(L, SHAPE_VERTEX_GENERATOR_NAME, [](lua_State * L) {
		AddCommonMethods<generator::ShapeVertex, &GetShapeVertexGenerator>(L);

		luaL_Reg funcs[] = {
			"Generate", [](lua_State * L)
			{
				auto * gen = reinterpret_cast<decltype(gav)>(GetShapeVertexGenerator(L));

				try {
					gen->mVertex = gen->mGen.generate();
				} catch (std::out_of_range & ex) {
					return luaL_error(L, ex.what());
				}

				lua_getfenv(L, 1); // shape_vertex_generator, env
				lua_getfield(L, -1, "box"); // shape_vertex_generator, env, box

				return 1;
			},
			{ nullptr, nullptr }
		};

		luaL_register(L, nullptr, funcs);
	});
	
	MakeShapeVertexBox(L, &gav->mVertex); // shape_vertex_generator, box

	return AddSourceToEnv(L, from, true); // shape_vertex_generator
}

//
//
//

#define TRIANGLE_GENERATOR_NAME "generator.AnyTriangleGenerator"

//
//
//

static generator::AnyGenerator<generator::Triangle> * GetTriangleGenerator (lua_State * L)
{
	return LuaXS::CheckUD<generator::AnyGenerator<generator::Triangle>>(L, 1, TRIANGLE_GENERATOR_NAME);
}

//
//
//

int WrapTriangleGenerator (lua_State * L, generator::AnyGenerator<generator::Triangle> && triangle_generator, int from)
{
	*LuaXS::AllocTyped<generator::AnyGenerator<generator::Triangle>>(L) = std::move(triangle_generator); // triangle_generator
	LuaXS::AttachMethods(L, TRIANGLE_GENERATOR_NAME, [](lua_State * L) {
		AddCommonMethods<generator::Triangle, &GetTriangleGenerator>(L);

		luaL_Reg funcs[] = {
			"Generate", [](lua_State * L)
			{
				try {
					generator::Triangle triangle = GetTriangleGenerator(L)->generate();

					lua_pushinteger(L, triangle.vertices[0] + 1); // triangle_generator, i1
					lua_pushinteger(L, triangle.vertices[1] + 1); // triangle_generator, i1, i2
					lua_pushinteger(L, triangle.vertices[2] + 1); // triangle_generator, i1, i2, i3

					return 3;
				} catch (std::out_of_range & ex) {
					return luaL_error(L, ex.what());
				}
			},
			{ nullptr, nullptr }
		};

		luaL_register(L, nullptr, funcs);
	});

	return AddSourceToEnv(L, from);
}