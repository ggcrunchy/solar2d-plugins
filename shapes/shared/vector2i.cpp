/*
* Permission is hereby granted, free of charge, to any person obtaining
* a copy of this software and associated documentation files (the
* "Software"), to deal in the Software without restriction, including
* without limitation the rights to use, copy, modify, merge, publish,
* distribute, sublicense, and/or sell copies of the Software, and to
* permit persons to whom the Software is furnished to do so, subject to
* the following conditions:
*
* The above copyright notice and this permission notice shall be
* included in all copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*
* [ MIT license: http://www.opensource.org/licenses/mit-license.php ]
*/

#include "common.h"
#include "yocto.h"

//
//
//

#define YOCTO_VECTOR2I_NAME "shapes.yocto.vector2i"

//
//
//

std::vector<yocto::vec2i> & GetVector2i (lua_State * L, int arg)
{
	return BoxOrVector<yocto::vec2i>::Get(L, arg, YOCTO_VECTOR2I_NAME);
}

//
//
//

static int AddMethods (lua_State * L)
{
	LuaXS::AttachMethods(L, YOCTO_VECTOR2I_NAME, [](lua_State * L) {
		luaL_Reg funcs[] = {
			{
				"append_line", AppendToVector<yocto::vec2i, &GetVector2i>
			}, {
				"__gc", LuaXS::TypedGC<BoxOrVector<yocto::vec2i>>
			}, {
				"lines_tangents", [](lua_State * L)
				{
					if (!lua_isnil(L, 3))
					{
						lua_settop(L, 3); // lines, positions, tangents

						yocto::lines_tangents(GetVector3f(L, 3), GetVector2i(L), GetVector3f(L, 2));

						return 1;
					}

					else return WrapVector3f(L, yocto::lines_tangents(GetVector2i(L), GetVector3f(L, 2))); // lines, positions, tangents
				}
			}, {
				"lines_to_cylinders", [](lua_State * L)
				{
					luaL_checktype(L, 2, LUA_TTABLE);

					LuaXS::Options params{L, 2};

					std::vector<yocto::vec3f> * positions;
					float scale = 0.01f;
					int steps = 4;

					params.NV_PAIR(positions)
						.NV_PAIR(scale)
						.NV_PAIR(steps);

					return WrapShapeData(L, yocto::lines_to_cylinders(GetVector2i(L), *positions, steps, scale)); // lines, positions, cylinders
				}
			}, {
				"merge_lines", [](lua_State * L)
				{
					luaL_checktype(L, 2, LUA_TTABLE);

					LuaXS::Options params{L, 2};

					std::vector<float> * merge_radius, * radius;
					std::vector<yocto::vec2f> * merge_texcoords, * texcoords;
					std::vector<yocto::vec2i> * merge_lines;
					std::vector<yocto::vec3f> * merge_normals, * merge_positions, * normals, * positions;

					params.NV_PAIR(merge_radius)
						.NV_PAIR(radius)
						.NV_PAIR(merge_texcoords)
						.NV_PAIR(texcoords)
						.NV_PAIR(merge_lines)
						.NV_PAIR(merge_normals)
						.NV_PAIR(merge_positions)
						.NV_PAIR(normals)
						.NV_PAIR(positions);

					yocto::merge_lines(GetVector2i(L, 1), *positions, *normals, *texcoords, *radius, *merge_lines, *merge_positions, *merge_normals, *merge_texcoords, *merge_radius);

					return 0;
				}
			}, {
				"subdivide_lines", [](lua_State * L)
				{
					switch (GetComponentCount(L, 2))
					{
					case 1:
						{
							auto subdivided = yocto::subdivide_lines(GetVector2i(L), GetVector1f(L, 2));

							WrapVector2i(L, std::move(subdivided.first)); // lines, vertices, subdivided_lines
							WrapVector1f(L, std::move(subdivided.second)); // lines, vertices, subdivided_lines, subdivided_vertices
						}

						break;
					case 2:
						{
							auto subdivided = yocto::subdivide_lines(GetVector2i(L), GetVector2f(L, 2));

							WrapVector2i(L, std::move(subdivided.first)); // lines, vertices, subdivided_lines
							WrapVector2f(L, std::move(subdivided.second)); // lines, vertices, subdivided_lines, subdivided_vertices
						}

						break;
					case 3:
						{
							auto subdivided = yocto::subdivide_lines(GetVector2i(L), GetVector3f(L, 2));

							WrapVector2i(L, std::move(subdivided.first)); // lines, vertices, subdivided_lines
							WrapVector3f(L, std::move(subdivided.second)); // lines, vertices, subdivided_lines, subdivided_vertices
						}

						break;
					case 4:
						{
							auto subdivided = yocto::subdivide_lines(GetVector2i(L), GetVector4f(L, 2));

							WrapVector2i(L, std::move(subdivided.first)); // lines, vertices, subdivided_lines
							WrapVector4f(L, std::move(subdivided.second)); // lines, vertices, subdivided_lines, subdivided_vertices
						}

						break;
					default:
						return luaL_error(L, "Invalid vector size");
					}

					return 2;
				}
			}, {
				"update_line", AppendToVector<yocto::vec2i, &GetVector2i>
			},
			{ nullptr, nullptr }
		};

		luaL_register(L, nullptr, funcs);

		SetComponentCount(L, 2);
	});

	return 1;
}

//
//
//

int WrapVector2i (lua_State * L, std::vector<yocto::vec2i> && sd)
{
	LuaXS::NewTyped<BoxOrVector<yocto::vec2i>>(L)->mVec = std::move(sd); // ..., shape_data
	
	return AddMethods(L);
}

//
//
//

int RefVector2i (lua_State * L, std::vector<yocto::vec2i> * v, int from)
{
	LuaXS::NewTyped<BoxOrVector<yocto::vec2i>>(L, L, from)->mVecPtr = v; // ..., source, ..., shape_data

	return AddMethods(L);
}