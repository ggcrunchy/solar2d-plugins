// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation; either
// version 2.1 of the License, or (at your option) any later version.

#pragma once

#include "CoronaLua.h"
#include "utils/LuaEx.h"
#include "generator.hpp"
#include <utility>

//
//
//

#define NV_PAIR(NAME) Add(#NAME, NAME)

//
//
//

void add_meshes (lua_State * L);
void add_paths (lua_State * L);
void add_shapes (lua_State * L);
void add_svg_writer (lua_State * L);

//
//
//

gml::dvec2 GetVec2 (lua_State * L, int arg);
gml::dvec3 GetVec3 (lua_State * L, int arg);
gml::dvec4 GetVec4 (lua_State * L, int arg);
gml::dquat GetQuat (lua_State * L, int arg);
gml::dvec3 GetColor (lua_State * L, int arg);
generator::Axis GetAxis (lua_State * L, int arg, const char * def = nullptr);

//
//
//

int AddTransform (lua_State * L, size_t nvalues, void ** key);
void LookupTransform (lua_State * L, void * key);

//
//
//

generator::AnyMesh * GetMesh (lua_State * L, int arg = 1);
int WrapMesh (lua_State * L, generator::AnyMesh && mesh);

//
//
//

generator::AnyPath * GetPath (lua_State * L, int arg = 1);
int WrapPath (lua_State * L, generator::AnyPath && path);

//
//
//

generator::AnyShape * GetShape (lua_State * L, int arg = 1);
int WrapShape (lua_State * L, generator::AnyShape && shape);

//
//
//

int WrapEdgeGenerator (lua_State * L, generator::AnyGenerator<generator::Edge> && edge_generator, int from = 1);
int WrapMeshVertexGenerator (lua_State * L, generator::AnyGenerator<generator::MeshVertex> && mesh_vertex_generator, int from = 1);
int WrapPathVertexGenerator (lua_State * L, generator::AnyGenerator<generator::PathVertex> && path_vertex_generator, int from = 1);
int WrapShapeVertexGenerator (lua_State * L, generator::AnyGenerator<generator::ShapeVertex> && shape_vertex_generator, int from = 1);
int WrapTriangleGenerator (lua_State * L, generator::AnyGenerator<generator::Triangle> && triangle_generator, int from = 1);

//
//
//

generator::MeshVertex * GetMeshVertex (lua_State * L, int arg = 1);
void MakeMeshVertexBox (lua_State * L, generator::MeshVertex * vertex = nullptr);

generator::PathVertex * GetPathVertex (lua_State * L, int arg = 1);
void MakePathVertexBox (lua_State * L, generator::PathVertex * vertex = nullptr);

generator::ShapeVertex * GetShapeVertex (lua_State * L, int arg = 1);
void MakeShapeVertexBox (lua_State * L, generator::ShapeVertex * vertex = nullptr);