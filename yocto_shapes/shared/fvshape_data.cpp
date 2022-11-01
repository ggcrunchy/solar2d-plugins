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

#define YOCTO_FVSHAPE_DATA_NAME "shapes.yocto.fvshape_data"

//
//
//

yocto::fvshape_data & GetFVShapeData (lua_State * L, int arg)
{
	return *LuaXS::CheckUD<yocto::fvshape_data>(L, arg, YOCTO_FVSHAPE_DATA_NAME);
}

//
//
//

int WrapFVShapeData (lua_State * L, yocto::fvshape_data && sd)
{
	*LuaXS::NewTyped<yocto::fvshape_data>(L) = std::move(sd); // fvshape_data
	LuaXS::AttachMethods(L, YOCTO_FVSHAPE_DATA_NAME, [](lua_State * L) {
		luaL_Reg funcs[] = {
			{
				"compute_normals", [](lua_State * L)
				{
					if (!lua_isnil(L, 2))
					{
						lua_settop(L, 2); // fvshape, normals

						yocto::compute_normals(GetVector3f(L, 2), GetFVShapeData(L));
					}

					else WrapVector3f(L, yocto::compute_normals(GetFVShapeData(L))); // fvshape, normals

					return 1;
				}
			}, {
				"eval_element_normal", [](lua_State * L)
				{
					return PushEvalResult<3>(L, yocto::eval_element_normal(GetFVShapeData(L), luaL_checkint(L, 2) - 1), 3); // fvshape, element, normal
				}
			}, {
				"eval_normal", [](lua_State * L)
				{
					return PushEvalResult<3>(L, yocto::eval_normal(GetFVShapeData(L), luaL_checkint(L, 2) - 1, LuaXS::GetArg<yocto::vec2f>(L, 3))); // fvshape, element, uv, normal
				}
			}, {
				"eval_position", [](lua_State * L)
				{
					return PushEvalResult<3>(L, yocto::eval_position(GetFVShapeData(L), luaL_checkint(L, 2) - 1, LuaXS::GetArg<yocto::vec2f>(L, 3))); // fvshape, element, uv, position
				}
			}, {
				"eval_texcoord", [](lua_State * L)
				{
					return PushEvalResult<2>(L, yocto::eval_texcoord(GetFVShapeData(L), luaL_checkint(L, 2) - 1, LuaXS::GetArg<yocto::vec2f>(L, 3))); // fvshape, element, uv, texcoord
				}
			}, {
				"__gc", LuaXS::TypedGC<yocto::fvshape_data>
			}, {
				"fvshape_stats", [](lua_State * L)
				{
					return PushStrings(L, yocto::fvshape_stats(GetFVShapeData(L), lua_toboolean(L, 2))); // fvshape[, verbose], stats
				}
			}, {
				"fvshape_to_shape", [](lua_State * L)
				{
					return WrapShapeData(L, yocto::fvshape_to_shape(GetFVShapeData(L))); // fvshape, shape
				}
			}, {
				"get_normals", [](lua_State * L)
				{
					return RefVector3f(L, &GetFVShapeData(L).normals); // fvshape, normals_ref
				}
			}, {
				"get_positions", [](lua_State * L)
				{
					return RefVector3f(L, &GetFVShapeData(L).positions); // fvshape, positions_ref
				}
			}, {
				"get_quadsnorm", [](lua_State * L)
				{
					return RefVector4i(L, &GetFVShapeData(L).quadsnorm); // fvshape, quadsnorm_ref
				}
			}, {
				"get_quadspos", [](lua_State * L)
				{
					return RefVector4i(L, &GetFVShapeData(L).quadspos); // fvshape, quadspos_ref
				}
			}, {
				"get_quadstexcoord", [](lua_State * L)
				{
					return RefVector4i(L, &GetFVShapeData(L).quadstexcoord); // fvshape, quadstexcoord_ref
				}
			}, {
				"get_texcoords", [](lua_State * L)
				{
					return RefVector2f(L, &GetFVShapeData(L).texcoords); // fvshape, texcoords_ref
				}
			}, {
				"set_normals", [](lua_State * L)
				{
					GetFVShapeData(L).normals = GetVector3f(L, 2);

					return 0;
				}
			}, {
				"set_positions", [](lua_State * L)
				{
					GetFVShapeData(L).positions = GetVector3f(L, 2);

					return 0;
				}
			}, {
				"set_quadsnorm", [](lua_State * L)
				{
					GetFVShapeData(L).quadsnorm = GetVector4i(L, 2);

					return 0;
				}
			}, {
				"set_quadspos", [](lua_State * L)
				{
					GetFVShapeData(L).quadspos = GetVector4i(L, 2);

					return 0;
				}
			}, {
				"set_quadstexcoord", [](lua_State * L)
				{
					GetFVShapeData(L).quadstexcoord = GetVector4i(L, 2);

					return 0;
				}
			}, {
				"set_texcoords", [](lua_State * L)
				{
					GetFVShapeData(L).texcoords = GetVector2f(L, 2);

					return 0;
				}
			}, {
				"subdivide_fvshape", [](lua_State * L)
				{
					return WrapFVShapeData(L, yocto::subdivide_fvshape(GetFVShapeData(L), luaL_checkint(L, 2), lua_toboolean(L, 3))); // fvshape, subdivisions, catmullclark, subdivided
				}
			},
			{ nullptr, nullptr }
		};

		luaL_register(L, nullptr, funcs);
	});

	return 1;
}