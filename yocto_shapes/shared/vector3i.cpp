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

#define YOCTO_VECTOR3I_NAME "shapes.yocto.vector3i"

//
//
//

std::vector<yocto::vec3i> & GetVector3i (lua_State * L, int arg)
{
	return BoxOrVector<yocto::vec3i>::Get(L, arg, YOCTO_VECTOR3I_NAME);
}

//
//
//

static int AddMethods (lua_State * L)
{
	LuaXS::AttachMethods(L, YOCTO_VECTOR3I_NAME, [](lua_State * L) {
		luaL_Reg funcs[] = {
			{
				"append_triangle", AppendToVector<yocto::vec3i, &GetVector3i>
			}, {
				"flip_triangles", [](lua_State * L)
				{
					return WrapVector3i(L, yocto::flip_triangles(GetVector3i(L))); // triangles, flipped
				}
			}, {
				"__gc", LuaXS::TypedGC<BoxOrVector<yocto::vec3i>>
			}, {
				"get_triangle", GetValue<yocto::vec3i, &GetVector3i>
			}, {
				"__len", GetLength<yocto::vec3i, &GetVector3i>
			}, {
				"merge_triangles", [](lua_State * L)
				{
					luaL_checktype(L, 2, LUA_TTABLE);

					LuaXS::Options params{L, 2};

					std::vector<yocto::vec2f> * merge_texcoords, * texcoords;
					std::vector<yocto::vec3i> * merge_triangles;
					std::vector<yocto::vec3f> * merge_normals, * merge_positions, * normals, * positions;

					params.NV_PAIR(merge_texcoords)
						.NV_PAIR(texcoords)
						.NV_PAIR(merge_triangles)
						.NV_PAIR(merge_normals)
						.NV_PAIR(merge_positions)
						.NV_PAIR(normals)
						.NV_PAIR(positions);

					yocto::merge_triangles(GetVector3i(L, 1), *positions, *normals, *texcoords, *merge_triangles, *merge_positions, *merge_normals, *merge_texcoords);

					return 0;
				}
			}, {
				"subdivide_triangles", [](lua_State * L)
				{
					switch (GetComponentCount(L, 2))
					{
					case 1:
						{
							auto subdivided = yocto::subdivide_triangles(GetVector3i(L), GetVector1f(L, 2));

							WrapVector3i(L, std::move(subdivided.first)); // triangles, vertices, subdivided_triangles
							WrapVector1f(L, std::move(subdivided.second)); // triangles, vertices, subdivided_triangles, subdivided_vertices
						}

						break;
					case 2:
						{
							auto subdivided = yocto::subdivide_triangles(GetVector3i(L), GetVector2f(L, 2));

							WrapVector3i(L, std::move(subdivided.first)); // triangles, vertices, subdivided_triangles
							WrapVector2f(L, std::move(subdivided.second)); // triangles, vertices, subdivided_triangles, subdivided_vertices
						}

						break;
					case 3:
						{
							auto subdivided = yocto::subdivide_triangles(GetVector3i(L), GetVector3f(L, 2));

							WrapVector3i(L, std::move(subdivided.first)); // triangles, vertices, subdivided_triangles
							WrapVector3f(L, std::move(subdivided.second)); // triangles, vertices, subdivided_triangles, subdivided_vertices
						}

						break;
					case 4:
						{
							auto subdivided = yocto::subdivide_triangles(GetVector3i(L), GetVector4f(L, 2));

							WrapVector3i(L, std::move(subdivided.first)); // triangles, vertices, subdivided_triangles
							WrapVector4f(L, std::move(subdivided.second)); // triangles, vertices, subdivided_triangles, subdivided_vertices
						}

						break;
					default:
						return luaL_error(L, "Invalid vector size");
					}

					return 2;
				}
			}, {
				"triangles_normals", [](lua_State * L)
				{
					if (!lua_isnil(L, 3))
					{
						lua_settop(L, 3); // triangles, positions, normals

						yocto::triangles_normals(GetVector3f(L, 3), GetVector3i(L), GetVector3f(L, 2));

						return 1;
					}

					else return WrapVector3f(L, yocto::triangles_normals(GetVector3i(L), GetVector3f(L, 2))); // triangles, positions, normals
				}
			}, {
				"triangles_tangent_spaces", [](lua_State * L)
				{
					return WrapVector4f(L, yocto::triangles_tangent_spaces(GetVector3i(L), GetVector3f(L, 2), GetVector3f(L, 3), GetVector2f(L, 4))); // triangles, positions, normals, texcoords, tangents
				}
			}, {
				"triangles_to_quads", [](lua_State * L)
				{
					return WrapVector4i(L, yocto::triangles_to_quads(GetVector3i(L))); // triangles, quads
				}
			}, {
				"update_triangle", UpdateVector<yocto::vec3i, &GetVector3i>
			}, {
				"weld_triangles", [](lua_State * L)
				{
					auto weld = yocto::weld_triangles(GetVector3i(L), GetVector3f(L, 2), LuaXS::Float(L, 3));

					WrapVector3i(L, std::move(weld.first)); // triangles, positions, threshold, welded_triangles
					WrapVector3f(L, std::move(weld.second)); // triangles, positions, threshold, welded_triangles, welded_positions

					return 2;
				}
			},
			{ nullptr, nullptr }
		};

		luaL_register(L, nullptr, funcs);

		SetComponentCount(L, 3);
	});

	return 1;
}

//
//
//

int WrapVector3i (lua_State * L, std::vector<yocto::vec3i> && sd)
{
	LuaXS::NewTyped<BoxOrVector<yocto::vec3i>>(L)->mVec = std::move(sd); // ..., shape_data
	
	return AddMethods(L);
}

//
//
//

int RefVector3i (lua_State * L, std::vector<yocto::vec3i> * v, int from)
{
	LuaXS::NewTyped<BoxOrVector<yocto::vec3i>>(L, L, from)->mVecPtr = v; // ..., source, ..., shape_data

	return AddMethods(L);
}