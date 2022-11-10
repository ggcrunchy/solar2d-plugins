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

#define YOCTO_VECTOR3F_NAME "shapes.yocto.vector3f"

//
//
//

std::vector<yocto::vec3f> & GetVector3f (lua_State * L, int arg)
{
	return BoxOrVector<yocto::vec3f>::Get(L, arg, YOCTO_VECTOR3F_NAME);
}

//
//
//

static int AddMethods (lua_State * L)
{
	LuaXS::AttachMethods(L, YOCTO_VECTOR3F_NAME, [](lua_State * L) {
		luaL_Reg funcs[] = {
/*
pair<vector<vec3f>, vector<vec3f>> skin_vertices(const vector<vec3f>& positions,
    const vector<vec3f>& normals, const vector<vec4f>& weights,
    const vector<vec4i>& joints, const vector<frame3f>& xforms);
// Apply skinning as specified in Khronos glTF.
void skin_vertices(vector<vec3f>& skinned_positions,
    vector<vec3f>& skinned_normals, const vector<vec3f>& positions,
    const vector<vec3f>& normals, const vector<vec4f>& weights,
    const vector<vec4i>& joints, const vector<frame3f>& xforms);

pair<vector<vec3f>, vector<vec3f>> skin_matrices(const vector<vec3f>& positions,
    const vector<vec3f>& normals, const vector<vec4f>& weights,
    const vector<vec4i>& joints, const vector<mat4f>& xforms);
// Update skinning
void skin_matrices(vector<vec3f>& skinned_positions,
    vector<vec3f>& skinned_normals, const vector<vec3f>& positions,
    const vector<vec3f>& normals, const vector<vec4f>& weights,
    const vector<vec4i>& joints, const vector<mat4f>& xforms);
*/
			{
				"align_vertices", [](lua_State * L)
				{
					luaL_checktype(L, 2, LUA_TTABLE);

					yocto::vec3i alignment;

					for (int i = 1; i <= 3; ++i, lua_pop(L, 1))
					{
						lua_rawgeti(L, 2, i); // positions, alignment, avalue

						const char * names[] = { "none", "min", "max", "center", nullptr };

						alignment[i - 1] = luaL_checkoption(L, -1, nullptr, names);
					}

					return WrapVector3f(L, yocto::align_vertices(GetVector3f(L), alignment)); // positions, alignment, aligned
				}
			}, {
				"append", AppendToVector<yocto::vec3f, &GetVector3f>
			}, {
				"flip_normals", [](lua_State * L)
				{
					return WrapVector3f(L, yocto::flip_normals(GetVector3f(L))); // normals, flipped
				}
			}, {
				"__gc", LuaXS::TypedGC<BoxOrVector<yocto::vec3f>>
			}, {
				"get", GetValue<yocto::vec3f, &GetVector3f>
			}, {
				"__len", GetLength<yocto::vec3f, &GetVector3f>
			}, {
				"lines_to_cylinders", [](lua_State * L)
				{
					LuaXS::Options opts{L, 2};

					float scale = 0.01f;
					int steps = 4;

					opts.NV_PAIR(scale)
						.NV_PAIR(steps);

					return WrapShapeData(L, yocto::lines_to_cylinders(GetVector3f(L), steps, scale)); // lines[, params], cylinders
				}
			}, {
				"points_to_spheres", [](lua_State * L)
				{
					LuaXS::Options opts{L, 2};

					float scale = 0.01f;
					int steps = 4;

					opts.NV_PAIR(scale)
						.NV_PAIR(steps);

					return WrapShapeData(L, yocto::points_to_spheres(GetVector3f(L), steps, scale)); // lines[, params], spheres
				}
			}, {
				"polyline_to_cylinders", [](lua_State * L)
				{
					LuaXS::Options opts{L, 2};

					float scale = 0.01f;
					int steps = 4;

					opts.NV_PAIR(scale)
						.NV_PAIR(steps);

					return WrapShapeData(L, yocto::polyline_to_cylinders(GetVector3f(L), steps, scale)); // lines[, params], cylinders
				}
			}, {
				"update", UpdateVector<yocto::vec3f, &GetVector3f>
			}, {
				"weld_vertices", [](lua_State * L)
				{
					auto weld = yocto::weld_vertices(GetVector3f(L), LuaXS::Float(L, 2));

					WrapVector3f(L, std::move(weld.first)); // positions, threshold, welded_positions
					WrapVector1i(L, std::move(weld.second)); // positions, threshold, welded_positions, welded_indices

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

int WrapVector3f (lua_State * L, std::vector<yocto::vec3f> && sd)
{
	LuaXS::NewTyped<BoxOrVector<yocto::vec3f>>(L)->mVec = std::move(sd); // ..., shape_data
	
	return AddMethods(L);
}

//
//
//

int RefVector3f (lua_State * L, std::vector<yocto::vec3f> * v, int from)
{
	LuaXS::NewTyped<BoxOrVector<yocto::vec3f>>(L, L, from)->mVecPtr = v; // ..., source, ..., shape_data

	return AddMethods(L);
}