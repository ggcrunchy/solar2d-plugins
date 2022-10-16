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

#define YOCTO_VECTOR4I_NAME "shapes.yocto.vector4i"

//
//
//

std::vector<yocto::vec4i> & GetVector4i (lua_State * L, int arg)
{
	return BoxOrVector<yocto::vec4i>::Get(L, arg, YOCTO_VECTOR4I_NAME);
}

//
//
//

static int AddMethods (lua_State * L)
{
	LuaXS::AttachMethods(L, YOCTO_VECTOR4I_NAME, [](lua_State * L) {
		luaL_Reg funcs[] = {
			{
				"append_quad", AppendToVector<yocto::vec4i, &GetVector4i>
			}, {
				"bezier_to_lines", [](lua_State * L)
				{
					return WrapVector2i(L, yocto::bezier_to_lines(GetVector4i(L))); // bezier, lines
				}
			}, {
				"flip_quads", [](lua_State * L)
				{
					return WrapVector4i(L, yocto::flip_quads(GetVector4i(L))); // quads, flipped
				}
			}, {
				"__gc", LuaXS::TypedGC<BoxOrVector<yocto::vec4i>>
			}, {
				"merge_quads", [](lua_State * L)
				{
					luaL_checktype(L, 2, LUA_TTABLE);

					LuaXS::Options params{L, 2};

					std::vector<yocto::vec2f> * merge_texcoords, * texcoords;
					std::vector<yocto::vec3f> * merge_normals, * merge_positions, * normals, * positions;
					std::vector<yocto::vec4i> * merge_quads;

					params.NV_PAIR(merge_texcoords)
						.NV_PAIR(texcoords)
						.NV_PAIR(merge_quads)
						.NV_PAIR(merge_normals)
						.NV_PAIR(merge_positions)
						.NV_PAIR(normals)
						.NV_PAIR(positions);

					yocto::merge_quads(GetVector4i(L, 1), *positions, *normals, *texcoords, *merge_quads, *merge_positions, *merge_normals, *merge_texcoords);

					return 0;
				}
			}, {
				"quads_normals", [](lua_State * L)
				{
					if (!lua_isnil(L, 3))
					{
						lua_settop(L, 3); // quads, positions, normals

						yocto::quads_normals(GetVector3f(L, 3), GetVector4i(L), GetVector3f(L, 2));

						return 1;
					}

					else return WrapVector3f(L, yocto::quads_normals(GetVector4i(L), GetVector3f(L, 2))); // quads, positions, normals
				}
			}, {
				"quads_to_triangles", [](lua_State * L)
				{
					return WrapVector3i(L, yocto::quads_to_triangles(GetVector4i(L))); // quads, triangles
				}
			}, {
				"split_facevarying", [](lua_State * L)
				{
					LuaXS::Options options{L, 2};

					std::vector<yocto::vec2f> tvec, * texcoords = nullptr, * split_texcoords = &tvec;
					std::vector<yocto::vec3f> nvec, pvec, * normals = nullptr, * split_normals = &nvec, * positions = nullptr, * split_positions = &pvec;
					std::vector<yocto::vec4i> qvec, qnone, * split_quads = &qvec, * qnorms = &qnone, * qtex = &qnone;

					options.WithFieldDo("split_quads", [L, &split_quads]() {
						split_quads = &GetVector4i(L, -1);
					})
						.WithFieldDo("positions", [L, &positions]() {
						positions = &GetVector3f(L, -1);
					})
						.WithFieldDo("split_positions", [L, positions, &split_positions]() {
						if (positions) split_positions = &GetVector3f(L, -1);
					})
						.WithFieldDo("normals", [L, &normals]() {
						normals = &GetVector3f(L, -1);
					})
						.WithFieldDo("split_normals", [L, normals, &split_normals]() {
						if (normals) split_normals = &GetVector3f(L, -1);
					})
						.WithFieldDo("texcoords", [L, &texcoords]() {
						texcoords = &GetVector2f(L, -1);
					})
						.WithFieldDo("split_texcoords", [L, texcoords, &split_texcoords]() {
						if (texcoords) split_texcoords = &GetVector2f(L, -1);
					})
						.WithFieldDo("quadsnorm", [L, &qnorms]() {
						qnorms = &GetVector4i(L, -1);
					})
						.WithFieldDo("quadstexcoord", [L, &qtex]() {
						qtex = &GetVector4i(L, -1);
					});

					yocto::split_facevarying(
						*split_quads, *split_positions, *split_normals, *split_texcoords,
						GetVector4i(L), *qnorms, *qtex,
						positions ? *positions : pvec, normals ? *normals : nvec, texcoords ? *texcoords : tvec);

					int nwrapped = 0;

					if (&pvec == split_positions && positions) nwrapped += WrapVector3f(L, std::move(pvec)); // quads[, split_positions]
					if (&nvec == split_normals && normals) nwrapped += WrapVector3f(L, std::move(nvec)); // quads[, split_positions][, split_normals]
					if (&tvec == split_texcoords && texcoords) nwrapped += WrapVector2f(L, std::move(tvec)); // quads[, split_positions][, split_normals][, split_texcoords]

					return nwrapped;
				}
			}, {
				"subdivide_beziers", [](lua_State * L)
				{
					switch (GetComponentCount(L, 2))
					{
					case 1:
						{
							auto subdivided = yocto::subdivide_beziers(GetVector4i(L), GetVector1f(L, 2));

							WrapVector4i(L, std::move(subdivided.first)); // beziers, vertices, subdivided_beziers
							WrapVector1f(L, std::move(subdivided.second)); // beziers, vertices, subdivided_beziers, subdivided_vertices
						}

						break;
					case 2:
						{
							auto subdivided = yocto::subdivide_beziers(GetVector4i(L), GetVector2f(L, 2));

							WrapVector4i(L, std::move(subdivided.first)); // beziers, vertices, subdivided_beziers
							WrapVector2f(L, std::move(subdivided.second)); // beziers, vertices, subdivided_beziers, subdivided_vertices
						}

						break;
					case 3:
						{
							auto subdivided = yocto::subdivide_beziers(GetVector4i(L), GetVector3f(L, 2));

							WrapVector4i(L, std::move(subdivided.first)); // beziers, vertices, subdivided_beziers
							WrapVector3f(L, std::move(subdivided.second)); // beziers, vertices, subdivided_beziers, subdivided_vertices
						}

						break;
					case 4:
						{
							auto subdivided = yocto::subdivide_beziers(GetVector4i(L), GetVector4f(L, 2));

							WrapVector4i(L, std::move(subdivided.first)); // beziers, vertices, subdivided_beziers
							WrapVector4f(L, std::move(subdivided.second)); // beziers, vertices, subdivided_beziers, subdivided_vertices
						}

						break;
					default:
						return luaL_error(L, "Invalid vector size");
					}

					return 2;
				}
			}, {
				"subdivide_catmullclark", [](lua_State * L)
				{
					bool lock_boundary = lua_toboolean(L, 3);

					switch (GetComponentCount(L, 2))
					{
					case 1:
						{
							auto subdivided = yocto::subdivide_catmullclark(GetVector4i(L), GetVector1f(L, 2), lock_boundary);

							WrapVector4i(L, std::move(subdivided.first)); // quads, vertices[, lock_boundary], subdivided_quads
							WrapVector1f(L, std::move(subdivided.second)); // quads, vertices[, lock_boundary], subdivided_quads, subdivided_vertices
						}

						break;
					case 2:
						{
							auto subdivided = yocto::subdivide_catmullclark(GetVector4i(L), GetVector2f(L, 2), lock_boundary);

							WrapVector4i(L, std::move(subdivided.first)); // quads, vertices[, lock_boundary], subdivided_quads
							WrapVector2f(L, std::move(subdivided.second)); // quads, vertices[, lock_boundary], subdivided_quads, subdivided_vertices
						}

						break;
					case 3:
						{
							auto subdivided = yocto::subdivide_catmullclark(GetVector4i(L), GetVector3f(L, 2), lock_boundary);

							WrapVector4i(L, std::move(subdivided.first)); // quads, vertices[, lock_boundary], subdivided_quads
							WrapVector3f(L, std::move(subdivided.second)); // quads, vertices[, lock_boundary], subdivided_quads, subdivided_vertices
						}

						break;
					case 4:
						{
							auto subdivided = yocto::subdivide_catmullclark(GetVector4i(L), GetVector4f(L, 2), lock_boundary);

							WrapVector4i(L, std::move(subdivided.first)); // quads, vertices[, lock_boundary], subdivided_quads
							WrapVector4f(L, std::move(subdivided.second)); // quads, vertices[, lock_boundary], subdivided_quads, subdivided_vertices
						}

						break;
					default:
						return luaL_error(L, "Invalid vector size");
					}

					return 2;
				}
			}, {
				"subdivide_quads", [](lua_State * L)
				{
					switch (GetComponentCount(L, 2))
					{
					case 1:
						{
							auto subdivided = yocto::subdivide_quads(GetVector4i(L), GetVector1f(L, 2));

							WrapVector4i(L, std::move(subdivided.first)); // quads, vertices, subdivided_quads
							WrapVector1f(L, std::move(subdivided.second)); // quads, vertices, subdivided_quads, subdivided_vertices
						}

						break;
					case 2:
						{
							auto subdivided = yocto::subdivide_quads(GetVector4i(L), GetVector2f(L, 2));

							WrapVector4i(L, std::move(subdivided.first)); // quads, vertices, subdivided_quads
							WrapVector2f(L, std::move(subdivided.second)); // quads, vertices, subdivided_quads, subdivided_vertices
						}

						break;
					case 3:
						{
							auto subdivided = yocto::subdivide_quads(GetVector4i(L), GetVector3f(L, 2));

							WrapVector4i(L, std::move(subdivided.first)); // quads, vertices, subdivided_quads
							WrapVector3f(L, std::move(subdivided.second)); // quads, vertices, subdivided_quads, subdivided_vertices
						}

						break;
					case 4:
						{
							auto subdivided = yocto::subdivide_quads(GetVector4i(L), GetVector4f(L, 2));

							WrapVector4i(L, std::move(subdivided.first)); // quads, vertices, subdivided_quads
							WrapVector4f(L, std::move(subdivided.second)); // quads, vertices, subdivided_quads, subdivided_vertices
						}

						break;
					default:
						return luaL_error(L, "Invalid vector size");
					}

					return 2;
				}
			}, {
				"update_quad", UpdateVector<yocto::vec4i, &GetVector4i>
			}, {
				"weld_quads", [](lua_State * L)
				{
					auto weld = yocto::weld_quads(GetVector4i(L), GetVector3f(L, 2), LuaXS::Float(L, 3));

					WrapVector4i(L, std::move(weld.first)); // quads, positions, threshold, welded_quads
					WrapVector3f(L, std::move(weld.second)); // quads, positions, threshold, welded_quads, welded_positions

					return 2;
				}
			},
			{ nullptr, nullptr }
		};

		luaL_register(L, nullptr, funcs);

		SetComponentCount(L, 4);
	});

	return 1;
}

//
//
//

int WrapVector4i (lua_State * L, std::vector<yocto::vec4i> && sd)
{
	LuaXS::NewTyped<BoxOrVector<yocto::vec4i>>(L)->mVec = std::move(sd); // ..., shape_data
	
	return AddMethods(L);
}

//
//
//

int RefVector4i (lua_State * L, std::vector<yocto::vec4i> * v, int from)
{
	LuaXS::NewTyped<BoxOrVector<yocto::vec4i>>(L, L, from)->mVecPtr = v; // ..., source, ..., shape_data

	return AddMethods(L);
}