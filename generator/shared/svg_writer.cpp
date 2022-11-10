// Copyright 2015 Markus Ilmola
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation; either
// version 2.1 of the License, or (at your option) any later version.

#include "common.h"

//
//
//

#define WRITER_NAME "generator.SvgWriter"

//
//
//

static generator::SvgWriter * GetWriter (lua_State * L)
{
	return LuaXS::CheckUD<generator::SvgWriter>(L, 1, WRITER_NAME);
}

//
//
//

static void GetWriteOptions (lua_State * L, bool & write_vertices, bool & write_normals)
{
	if (!lua_istable(L, 3)) return;

	lua_getfield(L, 3, "writeVertices"); // writer, object, params, write_vertices
	lua_getfield(L, 3, "writeNormals"); // writer, object, params, write_vertices, write_normals

	write_vertices = lua_toboolean(L, -2);
	write_normals = lua_toboolean(L, -1);
}

//
//
//

void add_svg_writer (lua_State * L)
{
	luaL_Reg funcs[] = {
		"createSvgWriter", [](lua_State * L)
		{
			LuaXS::NewTyped<generator::SvgWriter>(L, luaL_checkint(L, 1), luaL_checkint(L, 2)); // w, h, writer
			LuaXS::AttachMethods(L, WRITER_NAME, [](lua_State * L) {
				luaL_Reg funcs[] = {
					{
						"cullface", [](lua_State * L)
						{
							GetWriter(L)->cullface(lua_toboolean(L, 2));

							return 0;
						}
					}, {
						"getResult", [](lua_State * L)
						{
							lua_pushstring(L, GetWriter(L)->str().c_str()); // writer, result

							return 1;
						}
					}, {
						"modelView", [](lua_State * L)
						{
							gml::dmat4 mv;

							luaL_checktype(L, 2, LUA_TTABLE);
							lua_getfield(L, 2, "translation"); // writer, params, translation?

							if (!lua_isnil(L, -1))
							{
								lua_getfield(L, 2, "scaling"); // writer, params, translation, scaling
								lua_getfield(L, 2, "axis"); // writer, params, translation, scaling, axis?

								if (!lua_isnil(L, -1))
								{
									lua_getfield(L, 2, "angle"); // writer, params, translation, scaling, axis, angle

									mv = gml::translateRotateScale(GetVec3(L, -4), luaL_checknumber(L, -1), GetVec3(L, -2), GetVec3(L, -3));
								}

								else
								{
									lua_getfield(L, 2, "rotation"); // writer, params, translation, scaling, rotation

									mv = gml::translateRotateScale(GetVec3(L, -3), GetQuat(L, -1), GetVec3(L, -2));
								}
							}

							else
							{
								size_t n = lua_objlen(L, 2);

								luaL_argcheck(L, n, 2, "Empty modelview stages table");

								int top = lua_gettop(L);

								for (size_t i = 1; i <= n; ++i, lua_settop(L, top))
								{
									lua_rawgeti(L, 2, int(i)); // writer, params, nil, stage
									luaL_checktype(L, -1, LUA_TTABLE);
									lua_getfield(L, -1, "type"); // writer, params, nil, stage, type

									gml::dmat4 stage;

									const char * names[] = { "rotate", "scale", "translate", nullptr };
									int opt = luaL_checkoption(L, -1, nullptr, names);

									if (0 == opt)
									{
										lua_getfield(L, -2, "rotation"); // writer, params, nil, stage, "rotate", rotation?

										if (!lua_isnil(L, -1)) stage = gml::rotate(GetQuat(L, -1));

										else
										{
											lua_getfield(L, -3, "angles"); // writer, params, nil, stage, "rotate", nil, angles?

											if (!lua_isnil(L, -1)) stage = gml::rotate(GetVec3(L, -1));

											else
											{
												lua_getfield(L, -4, "angle"); // writer, params, nil, stage, "rotate", nil, nil, angle?
												lua_getfield(L, -5, "axis"); // writer, params, nil, stage, "rotate", nil, nil, angle, axis

												stage = gml::rotate(luaL_checknumber(L, -2), GetVec3(L, -1));
											}
										}
									}

									else if (1 == opt) stage = gml::scale(GetVec3(L, -2));
									else stage = gml::translate(GetVec3(L, -2));

									if (i > 1) mv = mv * stage;
									else mv = stage;
								}
							}

							GetWriter(L)->modelView(mv);

							return 0;
						}
					}, {
						"ortho", [](lua_State * L)
						{
							GetWriter(L)->ortho(luaL_checknumber(L, 2), luaL_checknumber(L, 3), luaL_checknumber(L, 4), luaL_checknumber(L, 5));

							return 0;
						}
					}, {
						"perspective", [](lua_State * L)
						{
							GetWriter(L)->perspective(luaL_checknumber(L, 2), luaL_checknumber(L, 3), luaL_checknumber(L, 4), luaL_checknumber(L, 5));

							return 0;
						}
					}, {
						"viewport", [](lua_State * L)
						{
							GetWriter(L)->viewport(luaL_checkint(L, 2), luaL_checkint(L, 3), luaL_checkint(L, 4), luaL_checkint(L, 5));

							return 0;
						}
					}, {
						"writeLine", [](lua_State * L)
						{
							GetWriter(L)->writeLine(GetVec3(L, 2), GetVec3(L, 3), !lua_isnil(L, 4) ? GetColor(L, 4) : gml::dvec3{});

							return 0;
						}
					}, {
						"writeMesh", [](lua_State * L)
						{
							bool write_vertices = false, write_normals = false;

							GetWriteOptions(L, write_vertices, write_normals);

							GetWriter(L)->writeMesh(*GetMesh(L, 2), write_vertices, write_normals);
						
							return 0;
						}
					}, {
						"writePath", [](lua_State * L)
						{
							bool write_vertices = false, write_normals = false;

							GetWriteOptions(L, write_vertices, write_normals);

							GetWriter(L)->writePath(*GetPath(L, 2), write_vertices, write_normals);

							return 0;
						}
					}, {
						"writePoint", [](lua_State * L)
						{
							GetWriter(L)->writePoint(GetVec3(L, 2), !lua_isnil(L, 3) ? GetColor(L, 3) : gml::dvec3{});

							return 0;
						}
					}, {
						"writeShape", [](lua_State * L)
						{
							bool write_vertices = false, write_normals = false;

							GetWriteOptions(L, write_vertices, write_normals);

							GetWriter(L)->writeShape(*GetShape(L, 2), write_vertices, write_normals);

							return 0;
						}
					}, {
						"writeTriangle", [](lua_State * L)
						{
							if (!lua_isnil(L, 5))
							{
								GetWriter(L)->writeTriangle(GetVec3(L, 2), GetVec3(L, 3), GetVec3(L, 4), GetColor(L, 5));
							}

							else
							{
								GetWriter(L)->writeTriangle(GetVec3(L, 2), GetVec3(L, 3), GetVec3(L, 4));
							}

							return 0;
						}
					},
					{ nullptr, nullptr }
				};

				luaL_register(L, nullptr, funcs);
			});

			return 1;
		},
		{ nullptr, nullptr }
	};

	luaL_register(L, nullptr, funcs);
}