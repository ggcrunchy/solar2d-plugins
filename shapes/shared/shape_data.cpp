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

#define YOCTO_SHAPE_DATA_NAME "shapes.yocto.shape_data"

//
//
//

yocto::shape_data & GetShapeData (lua_State * L, int arg)
{
	return *LuaXS::CheckUD<yocto::shape_data>(L, arg, YOCTO_SHAPE_DATA_NAME);
}

//
//
//

static void PushShapePoint (lua_State * L, const yocto::shape_point & point)
{
	lua_createtable(L, 0, 3); // ..., point
	lua_pushinteger(L, point.element + 1); // ..., point, element
	lua_setfield(L, -2, "element"); // ..., point = { element = element }
	lua_createtable(L, 2, 0); // ..., point, uv
	lua_pushnumber(L, point.uv.x); // ..., point, uv, u
	lua_rawseti(L, -2, 1); // ..., point, uv = { u = u }
	lua_pushnumber(L, point.uv.y); // ..., point, uv, v
	lua_rawseti(L, -2, 2); // ..., point, uv = { u, v = v }
	lua_setfield(L, -2, "uv"); // ..., point = { element, uv = uv }
}

//
//
//

int WrapShapeData (lua_State * L, yocto::shape_data && sd)
{
	*LuaXS::NewTyped<yocto::shape_data>(L) = std::move(sd); // shape_data
	LuaXS::AttachMethods(L, YOCTO_SHAPE_DATA_NAME, [](lua_State * L) {

		luaL_Reg funcs[] = {
			{
				"compute_normals", [](lua_State * L)
				{
					if (!lua_isnil(L, 2))
					{
						lua_settop(L, 2); // shape, normals

						yocto::compute_normals(GetVector3f(L, 2), GetShapeData(L));
					}

					else WrapVector3f(L, yocto::compute_normals(GetShapeData(L))); // shape, normals

					return 1;
				}
			}, {
				"eval_color", [](lua_State * L)
				{
					return PushEvalResult<4>(L, yocto::eval_color(GetShapeData(L), luaL_checkint(L, 2) - 1, LuaXS::GetArg<yocto::vec2f>(L, 3))); // shape, element, uv, color
				}
			}, {
				"eval_element_normal", [](lua_State * L)
				{
					return PushEvalResult<3>(L, yocto::eval_element_normal(GetShapeData(L), luaL_checkint(L, 2) - 1), 3); // shape, element, normal
				}
			}, {
				"eval_normal", [](lua_State * L)
				{
					return PushEvalResult<3>(L, yocto::eval_normal(GetShapeData(L), luaL_checkint(L, 2) - 1, LuaXS::GetArg<yocto::vec2f>(L, 3))); // shape, element, uv, normal
				}
			}, {
				"eval_position", [](lua_State * L)
				{
					return PushEvalResult<3>(L, yocto::eval_position(GetShapeData(L), luaL_checkint(L, 2) - 1, LuaXS::GetArg<yocto::vec2f>(L, 3))); // shape, element, uv, position
				}
			}, {
				"eval_radius", [](lua_State * L)
				{
					lua_pushnumber(L, yocto::eval_radius(GetShapeData(L), luaL_checkint(L, 2) - 1, LuaXS::GetArg<yocto::vec2f>(L, 3))); // shape, element, uv, radius

					return 1;
				}
			}, {
				"eval_tangent", [](lua_State * L)
				{
					return PushEvalResult<3>(L, yocto::eval_tangent(GetShapeData(L), luaL_checkint(L, 2) - 1, LuaXS::GetArg<yocto::vec2f>(L, 3))); // shape, element, uv, tangent
				}
			}, {
				"eval_texcoord", [](lua_State * L)
				{
					return PushEvalResult<2>(L, yocto::eval_texcoord(GetShapeData(L), luaL_checkint(L, 2) - 1, LuaXS::GetArg<yocto::vec2f>(L, 3))); // shape, element, uv, texcoord
				}
			}, {
				"__gc", LuaXS::TypedGC<yocto::shape_data>
			}, {
				"get_colors", [](lua_State * L)
				{
					return RefVector4f(L, &GetShapeData(L).colors); // shape, colors_ref
				}
			}, {
				"set_lines", [](lua_State * L)
				{
					return RefVector2i(L, &GetShapeData(L).lines); // shape, lines_ref
				}
			}, {
				"set_normals", [](lua_State * L)
				{
					return RefVector3f(L, &GetShapeData(L).normals); // shape, normals_ref
				}
			}, {
				"set_points", [](lua_State * L)
				{
					return RefVector1i(L, &GetShapeData(L).points); // shape, points_ref
				}
			}, {
				"set_positions", [](lua_State * L)
				{
					return RefVector3f(L, &GetShapeData(L).positions); // shape, positions_ref
				}
			}, {
				"set_quads", [](lua_State * L)
				{
					return RefVector4i(L, &GetShapeData(L).quads); // shape, quads_ref
				}
			}, {
				"set_radius", [](lua_State * L)
				{
					return RefVector1f(L, &GetShapeData(L).radius); // shape, radius_ref
				}
			}, {
				"set_tangents", [](lua_State * L)
				{
					return RefVector4f(L, &GetShapeData(L).tangents); // shape, tangents_ref
				}
			}, {
				"set_texcoords", [](lua_State * L)
				{
					return RefVector2f(L, &GetShapeData(L).texcoords); // shape, texcoords_ref
				}
			}, {
				"set_triangles", [](lua_State * L)
				{
					return RefVector3i(L, &GetShapeData(L).triangles); // shape, triangles_ref
				}
			}, {
				"make_hair", [](lua_State * L)
				{
					LuaXS::Options opts{L, 2};

					yocto::vec2i steps = {8, 65536};
					yocto::vec2f length = {0.1f, 0.1f}, radius = {0.001f, 0.001f}, noise = {0, 10}, clump = {0, 128}, rotation = {0, 0};
					int seed = 7;

					opts.NV_PAIR(steps)
						.NV_PAIR(length)
						.NV_PAIR(radius)
						.NV_PAIR(noise)
						.NV_PAIR(clump)
						.NV_PAIR(rotation)
						.NV_PAIR(seed);

					return WrapShapeData(L, yocto::make_hair(GetShapeData(L), steps, length, radius, noise, clump, rotation, seed)); // shape[, params], hair
				}
			}, {
				"make_hair2", [](lua_State * L)
				{
					LuaXS::Options opts{L, 2};

					yocto::vec2i steps = {8, 65536};
					yocto::vec2f length = {0.1f, 0.1f}, radius = {0.001f, 0.001f};
					float noise = 0, gravity = 0.001f;
					int seed = 7;

					opts.NV_PAIR(steps)
						.NV_PAIR(length)
						.NV_PAIR(radius)
						.NV_PAIR(noise)
						.NV_PAIR(gravity)
						.NV_PAIR(seed);

					return WrapShapeData(L, yocto::make_hair2(GetShapeData(L), steps, length, radius, noise, gravity, seed)); // shape[, params], hair
				}
			}, {
				"quads_to_triangles", [](lua_State * L)
				{
					return WrapShapeData(L, yocto::quads_to_triangles(GetShapeData(L))); // quad_shape, tri_shape
				}
			}, {
				"quads_to_triangles_inplace", [](lua_State * L)
				{
					yocto::quads_to_triangles_inplace(GetShapeData(L));

					return 0;
				}
			}, {
				"sample_shape", [](lua_State * L)
				{
					if (lua_isnumber(L, 2))
					{
						std::vector<yocto::shape_point> points = yocto::sample_shape(GetShapeData(L), luaL_checkint(L, 2), luaL_optinteger(L, 3, 98729387));

						lua_createtable(L, points.size(), 0); // shape, num_samples[, seed], points

						for (size_t i = 0; i < points.size(); ++i)
						{
							PushShapePoint(L, points[i]); // shape, num_samples[, seed], points, point

							lua_rawseti(L, -2, int(i + 1)); // shape, num_samples[, seed], points = { ..., point }
						}
					}

					else PushShapePoint(L, yocto::sample_shape(GetShapeData(L), GetVector1f(L, 2), LuaXS::Float(L, 3), LuaXS::GetArg<yocto::vec2f>(L, 4))); // shape, cdf, rn, ruv, point

					return 1;
				}
			}, {
				"sample_shape_cdf", [](lua_State * L)
				{
					if (!lua_isnil(L, 2))
					{
						lua_settop(L, 2); // shape, cdf

						yocto::sample_shape_cdf(GetVector1f(L, 2), GetShapeData(L));
					}

					else WrapVector1f(L, yocto::sample_shape_cdf(GetShapeData(L)));

					return 1;
				}
			}, {
				"set_colors", [](lua_State * L)
				{
					GetShapeData(L).colors = GetVector4f(L, 2);

					return 0;
				}
			}, {
				"set_lines", [](lua_State * L)
				{
					GetShapeData(L).lines = GetVector2i(L, 2);

					return 0;
				}
			}, {
				"set_normals", [](lua_State * L)
				{
					GetShapeData(L).normals = GetVector3f(L, 2);

					return 0;
				}
			}, {
				"set_points", [](lua_State * L)
				{
					GetShapeData(L).points = GetVector1i(L, 2);

					return 0;
				}
			}, {
				"set_positions", [](lua_State * L)
				{
					GetShapeData(L).positions = GetVector3f(L, 2);

					return 0;
				}
			}, {
				"set_quads", [](lua_State * L)
				{
					GetShapeData(L).quads = GetVector4i(L, 2);

					return 0;
				}
			}, {
				"set_radius", [](lua_State * L)
				{
					GetShapeData(L).radius = GetVector1f(L, 2);

					return 0;
				}
			}, {
				"set_tangents", [](lua_State * L)
				{
					GetShapeData(L).tangents = GetVector4f(L, 2);

					return 0;
				}
			}, {
				"set_texcoords", [](lua_State * L)
				{
					GetShapeData(L).texcoords = GetVector2f(L, 2);

					return 0;
				}
			}, {
				"set_triangles", [](lua_State * L)
				{
					GetShapeData(L).triangles = GetVector3i(L, 2);

					return 0;
				}
			}, {
				"shape_stats", [](lua_State * L)
				{
					return PushStrings(L, yocto::shape_stats(GetShapeData(L), lua_toboolean(L, 2))); // shape[, verbose], stats
				}
			}, {
				"shape_to_fvshape", [](lua_State * L)
				{
					return WrapFVShapeData(L, yocto::shape_to_fvshape(GetShapeData(L))); // shape, fvshape
				}
			}, {
				"subdivide_shape", [](lua_State * L)
				{
					return WrapShapeData(L, yocto::subdivide_shape(GetShapeData(L), luaL_checkint(L, 2), lua_toboolean(L, 3))); // shape, subdivisions, catmullclark, subdivided
				}
			},
			{ nullptr, nullptr }
		};

		luaL_register(L, nullptr, funcs);
	});

	return 1;
}