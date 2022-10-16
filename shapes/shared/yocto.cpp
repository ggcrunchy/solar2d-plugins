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

static int sWeakParentRef;

//
//
//

void PushWeakValuedParentTable (lua_State * L)
{
    lua_getref(L, sWeakParentRef); // ..., wvpt
}

//
//
//

static int sComponentCountRef;

//
//
//

void SetComponentCount (lua_State * L, int count)
{
    lua_getref(L, sComponentCountRef); // ..., mt, counts
    lua_pushvalue(L, -2); // ..., mt, counts, mt
    lua_pushinteger(L, count); // ..., mt, counts, mt, count
    lua_rawset(L, -3); // ..., mt, counts = { ..., [mt] = count }
    lua_pop(L, 1); // ..., mt
}

//
//
//

int GetComponentCount (lua_State * L, int arg)
{
    luaL_argcheck(L, lua_getmetatable(L, arg), arg, "Object has no metatable"); // ..., object, ..., mt
    lua_getref(L, sComponentCountRef); // ..., object, ..., mt, counts
    lua_insert(L, -2); // ..., object, ..., counts, mt
    lua_rawget(L, -2); // ..., object, ..., counts, count

    int count = luaL_checkint(L, -1);

    lua_pop(L, 2); // ..., object, ...

    return count;
}

//
//
//

int PushStrings (lua_State * L, std::vector<std::string> && strs)
{
	lua_createtable(L, strs.size(), 0); // ..., strs

	for (size_t i = 0; i < strs.size(); ++i)
	{
		lua_pushstring(L, strs[i].c_str()); // ..., strs, str
		lua_rawseti(L, -2, int(i + 1)); // ..., strs = { ..., str }
	}

	return 1;
}

//
//
//

void add_yocto (lua_State * L)
{
    lua_newtable(L); // yocto

	luaL_Reg funcs[] = {
        {
            "make_bent_floor", [](lua_State * L)
            {
                LuaXS::Options opts{L, 1};

                yocto::vec2f scale = {10, 10}, uvscale = {10, 10};
                yocto::vec2i steps = {1, 1};
                float bent = 0.5f;

                opts.NV_PAIR(scale)
                    .NV_PAIR(uvscale)
                    .NV_PAIR(steps)
                    .NV_PAIR(bent);

                return WrapShapeData(L, yocto::make_bent_floor(steps, scale, uvscale, bent)); // [params, ]floor
            }
        }, {
            "make_box", [](lua_State * L)
            {
                LuaXS::Options opts{L, 1};

                yocto::vec3f scale = {1, 1, 1}, uvscale = {1, 1, 1};
                yocto::vec3i steps = {1, 1, 1};

                opts.NV_PAIR(scale)
                    .NV_PAIR(uvscale)
                    .NV_PAIR(steps);

                return WrapShapeData(L, yocto::make_box(steps, scale, uvscale)); // [params, ]box
            }
        }, {
            "make_bulged_disk", [](lua_State * L)
            {
                LuaXS::Options opts{L, 1};

                float scale = 1, uvscale = 1, height = 0.3f;
                int steps = 32;

                opts.NV_PAIR(scale)
                    .NV_PAIR(uvscale)
                    .NV_PAIR(steps);

                return WrapShapeData(L, yocto::make_bulged_disk(steps, scale, uvscale, height)); // [params, ]disk
            }
        }, {
            "make_bulged_rect", [](lua_State * L)
            {
                LuaXS::Options opts{L, 1};

                yocto::vec2f scale = {1, 1}, uvscale = {1, 1};
                yocto::vec2i steps = {1, 1};
                float radius = 0.3f;

                opts.NV_PAIR(scale)
                    .NV_PAIR(uvscale)
                    .NV_PAIR(steps)
                    .NV_PAIR(radius);

                return WrapShapeData(L, yocto::make_bulged_rect(steps, scale, uvscale, radius)); // [params, ]rect
            }
        }, {
            "make_bulged_recty", [](lua_State * L)
            {
                LuaXS::Options opts{L, 1};

                yocto::vec2f scale = {1, 1}, uvscale = {1, 1};
                yocto::vec2i steps = {1, 1};
                float radius = 0.3f;

                opts.NV_PAIR(scale)
                    .NV_PAIR(uvscale)
                    .NV_PAIR(steps)
                    .NV_PAIR(radius);

                return WrapShapeData(L, yocto::make_bulged_recty(steps, scale, uvscale, radius)); // [params, ]rect
            }
        }, {
            "make_capped_uvsphere", [](lua_State * L)
            {
                LuaXS::Options opts{L, 1};

                yocto::vec2i steps = {32, 32};
                yocto::vec2f uvscale = {1, 1};
                float scale = 1, height = 0.3f;

                opts.NV_PAIR(steps)
                    .NV_PAIR(uvscale)
                    .NV_PAIR(scale)
                    .NV_PAIR(height);

                return WrapShapeData(L, yocto::make_capped_uvsphere(steps, scale, uvscale, height)); // [params, ]sphere
            }
        }, {
            "make_capped_uvspherey", [](lua_State * L)
            {
                LuaXS::Options opts{L, 1};

                yocto::vec2i steps = {32, 32};
                yocto::vec2f uvscale = {1, 1};
                float scale = 1, height = 0.3f;

                opts.NV_PAIR(steps)
                    .NV_PAIR(uvscale)
                    .NV_PAIR(scale)
                    .NV_PAIR(height);

                return WrapShapeData(L, yocto::make_capped_uvspherey(steps, scale, uvscale, height)); // [params, ]sphere
            }
        }, {
            "make_cube", [](lua_State * L)
            {
                LuaXS::Options opts{L, 1};

                float scale = 1;
                int subdivisions = 0;

                opts.NV_PAIR(scale)
                    .NV_PAIR(subdivisions);

                return WrapShapeData(L, yocto::make_cube(scale, subdivisions)); // [params, ]cube
            }
        }, {
            "make_disk", [](lua_State * L)
            {
                LuaXS::Options opts{L, 1};

                float scale = 1, uvscale = 1;
                int steps = 0;

                opts.NV_PAIR(scale)
                    .NV_PAIR(uvscale)
                    .NV_PAIR(steps);

                return WrapShapeData(L, yocto::make_disk(steps, scale, uvscale)); // [params, ]disk
            }
        }, {
            "make_floor", [](lua_State * L)
            {
                LuaXS::Options opts{L, 1};

                yocto::vec2i steps = {1, 1};
                yocto::vec2f scale = {10, 10}, uvscale = {10, 10};

                opts.NV_PAIR(steps)
                    .NV_PAIR(scale)
                    .NV_PAIR(uvscale);

                return WrapShapeData(L, yocto::make_floor(steps, scale, uvscale)); // [params, ]floor
            }
        }, {
            "make_fvbox", [](lua_State * L)
            {
                LuaXS::Options opts{L, 1};

                yocto::vec3i steps = {1, 1, 1};
                yocto::vec3f scale = {1, 1, 1}, uvscale = {1, 1, 1};

                opts.NV_PAIR(steps)
                    .NV_PAIR(scale)
                    .NV_PAIR(uvscale);

                return WrapFVShapeData(L, yocto::make_fvbox(steps, scale, uvscale)); // [params, ]box
            }
        }, {
            "make_fvcube", [](lua_State * L)
            {
                LuaXS::Options opts{L, 1};

                float scale = 1;
                int subdivisions = 0;

                opts.NV_PAIR(scale)
                    .NV_PAIR(subdivisions);

                return WrapFVShapeData(L, yocto::make_fvcube(scale, subdivisions)); // [params, ]cube
            }
        }, {
            "make_fvrect", [](lua_State * L)
            {
                LuaXS::Options opts{L, 1};

                yocto::vec2i steps = {1, 1};
                yocto::vec2f scale = {1, 1}, uvscale = {1, 1};

                opts.NV_PAIR(steps)
                    .NV_PAIR(scale)
                    .NV_PAIR(uvscale);

                return WrapFVShapeData(L, yocto::make_fvrect(steps, scale, uvscale)); // [params, ]rect
            }
        }, {
            "make_fvsphere", [](lua_State * L)
            {
                LuaXS::Options opts{L, 1};

                float scale = 1, uvscale = 1;
                int steps = 32;

                opts.NV_PAIR(scale)
                    .NV_PAIR(uvscale)
                    .NV_PAIR(steps);

                return WrapFVShapeData(L, yocto::make_fvsphere(steps, scale, uvscale)); // [params, ]sphere
            }
        }, {
            "make_geosphere", [](lua_State * L)
            {
                LuaXS::Options opts{L, 1};

                float scale = 1;
                int subdivisions = 0;

                opts.NV_PAIR(scale)
                    .NV_PAIR(subdivisions);

                return WrapShapeData(L, yocto::make_geosphere(scale, subdivisions)); // [params, ]sphere
            }
        }, {
            "make_lines", [](lua_State * L)
            {
                LuaXS::Options opts{L, 1};

                yocto::vec2i steps = {4, 65536};
                yocto::vec2f scale = {1, 1}, uvscale = {1, 1}, radius = {0.001f, 0.001f};

                opts.NV_PAIR(steps)
                    .NV_PAIR(scale)
                    .NV_PAIR(uvscale)
                    .NV_PAIR(radius);

                return WrapShapeData(L, yocto::make_lines(steps, scale, uvscale, radius)); // [params, ]lines
            }
        }, {
            "make_monkey", [](lua_State * L)
            {
                LuaXS::Options opts{L, 1};

                float scale = 1;
                int subdivisions = 0;

                opts.NV_PAIR(scale)
                    .NV_PAIR(subdivisions);

                return WrapShapeData(L, yocto::make_monkey(scale, subdivisions)); // [params, ]monkey
            }
        }, {
            "make_point", [](lua_State * L)
            {
                LuaXS::Options opts{L, 1};

                float radius = 0.001f;

                opts.NV_PAIR(radius);

                return WrapShapeData(L, yocto::make_point(radius)); // [params, ]point
            }
        }, {
            "make_points", [](lua_State * L)
            {
                LuaXS::Options opts{L, 1};

                float uvscale = 1, radius = 0.001f;
                int num = 65536;

                opts.NV_PAIR(uvscale)
                    .NV_PAIR(radius)
                    .NV_PAIR(num);

                return WrapShapeData(L, yocto::make_points(num, uvscale, radius)); // [params, ]points
            }
        }, {
            "make_points2", [](lua_State * L)
            {
                LuaXS::Options opts{L, 1};

                yocto::vec2i steps = {256, 256};
                yocto::vec2f size = {1, 1}, uvscale = {1, 1}, radius = {0.001f, 0.001f};

                opts.NV_PAIR(steps)
                    .NV_PAIR(size)
                    .NV_PAIR(uvscale)
                    .NV_PAIR(radius);

                return WrapShapeData(L, yocto::make_points(steps, size, uvscale, radius)); // [params, ]points
            }
        }, {
            "make_quad", [](lua_State * L)
            {
                LuaXS::Options opts{L, 1};

                float scale = 1;
                int subdivisions = 0;

                opts.NV_PAIR(scale)
                    .NV_PAIR(subdivisions);

                return WrapShapeData(L, yocto::make_quad(scale, subdivisions)); // [params, ]quad
            }
        }, {
            "make_quady", [](lua_State * L)
            {
                LuaXS::Options opts{L, 1};

                float scale = 1;
                int subdivisions = 0;

                opts.NV_PAIR(scale)
                    .NV_PAIR(subdivisions);

                return WrapShapeData(L, yocto::make_quady(scale, subdivisions)); // [params, ]quad
            }
        }, {
            "make_random_points", [](lua_State * L)
            {
                LuaXS::Options opts{L, 1};

                yocto::vec3f size = {1, 1, 1};
                float uvscale = 1, radius = 0.001f;
                int num = 65536;
                uint64_t seed = 17;

                opts.NV_PAIR(size)
                    .NV_PAIR(uvscale)
                    .NV_PAIR(radius)
                    .NV_PAIR(num)
                    .NV_PAIR(seed);

                return WrapShapeData(L, yocto::make_random_points(num, size, uvscale, radius, seed)); // [params, ]points
            }
        }, {
            "make_rect", [](lua_State * L)
            {
            LuaXS::Options opts{L, 1};

                yocto::vec2i steps = {1, 1};
                yocto::vec2f scale = {1, 1}, uvscale = {1, 1};

                opts.NV_PAIR(steps)
                    .NV_PAIR(scale)
                    .NV_PAIR(uvscale);

                return WrapShapeData(L, yocto::make_rect(steps, scale, uvscale)); // [params, ]rect
            }
        }, {
            "make_rect_stack", [](lua_State * L)
            {
                LuaXS::Options opts{L, 1};

                yocto::vec2f uvscale = {1, 1};
                yocto::vec3i steps = {1, 1, 1};
                yocto::vec3f scale = {1, 1, 1};

                opts.NV_PAIR(uvscale)
                    .NV_PAIR(steps)
                    .NV_PAIR(scale);

                return WrapShapeData(L, yocto::make_rect_stack(steps, scale, uvscale)); // [params, ]stack
            }
        }, {
            "make_recty", [](lua_State * L)
            {
                LuaXS::Options opts{L, 1};

                yocto::vec2i steps = {1, 1};
                yocto::vec2f scale = {1, 1}, uvscale = {1, 1};

                opts.NV_PAIR(steps)
                    .NV_PAIR(scale)
                    .NV_PAIR(uvscale);

                return WrapShapeData(L, yocto::make_recty(steps, scale, uvscale)); // [params, ]rect
            }
        }, {
            "make_rounded_box", [](lua_State * L)
            {
                LuaXS::Options opts{L, 1};

                yocto::vec3i steps = {1, 1, 1};
                yocto::vec3f scale = {1, 1, 1}, uvscale = {1, 1, 1};
                float radius = 0.3f;

                opts.NV_PAIR(steps)
                    .NV_PAIR(scale)
                    .NV_PAIR(uvscale)
                    .NV_PAIR(radius);

                return WrapShapeData(L, yocto::make_rounded_box(steps, scale, uvscale, radius)); // [params, ]box
            }
        }, {
            "make_rounded_uvcylinder", [](lua_State * L)
            {
                LuaXS::Options opts{L, 1};
                
                yocto::vec2f scale = {1, 1};
                yocto::vec3f uvscale = {1, 1, 1};
                yocto::vec3i steps = {32, 32, 32};
                float radius = 0.3f;

                opts.NV_PAIR(scale)
                    .NV_PAIR(uvscale)
                    .NV_PAIR(steps)
                    .NV_PAIR(radius);

                return WrapShapeData(L, yocto::make_rounded_uvcylinder(steps, scale, uvscale, radius)); // [params, ]cylinder
            }
        }, {
            "make_sphere", [](lua_State * L)
            {
                LuaXS::Options opts{L, 1};

                float scale = 1, uvscale = 1;
                int steps = 32;

                opts.NV_PAIR(scale)
                    .NV_PAIR(uvscale)
                    .NV_PAIR(steps);

                return WrapShapeData(L, yocto::make_sphere(steps, scale, uvscale)); // [params, ]sphere
            }
        }, {
            "make_uvcylinder", [](lua_State * L)
            {
                LuaXS::Options opts{L, 1};
                
                yocto::vec2f scale = {1, 1};
                yocto::vec3f uvscale = {1, 1, 1};
                yocto::vec3i steps = {32, 32, 32};

                opts.NV_PAIR(scale)
                    .NV_PAIR(uvscale)
                    .NV_PAIR(steps);

                return WrapShapeData(L, yocto::make_uvcylinder(steps, scale, uvscale)); // [params, ]cylinder
            }
        }, {
            "make_uvdisk", [](lua_State * L)
            {
                LuaXS::Options opts{L, 1};

                yocto::vec2i steps = {1, 1};
                yocto::vec2f uvscale = {1, 1};
                float scale = 1;

                opts.NV_PAIR(steps)
                    .NV_PAIR(uvscale)
                    .NV_PAIR(scale);

                return WrapShapeData(L, yocto::make_uvdisk(steps, scale, uvscale)); // [params, ]disk
            }
        }, {
            "make_uvsphere", [](lua_State * L)
            {
                LuaXS::Options opts{L, 1};

                yocto::vec2i steps = {1, 1};
                yocto::vec2f uvscale = {1, 1};
                float scale = 1;

                opts.NV_PAIR(steps)
                    .NV_PAIR(uvscale)
                    .NV_PAIR(scale);

                return WrapShapeData(L, yocto::make_uvsphere(steps, scale, uvscale)); // [params, ]sphere
            }
        }, {
            "make_uvspherey", [](lua_State * L)
            {
                LuaXS::Options opts{L, 1};

                yocto::vec2i steps = {1, 1};
                yocto::vec2f uvscale = {1, 1};
                float scale = 1;

                opts.NV_PAIR(steps)
                    .NV_PAIR(uvscale)
                    .NV_PAIR(scale);

                return WrapShapeData(L, yocto::make_uvspherey(steps, scale, uvscale)); // [params, ]sphere
            }
        },
		{ nullptr, nullptr }
	};

	luaL_register(L, nullptr, funcs);
    lua_setfield(L, -2, "yocto"); // ..., shapes

    //
    //
    //

    lua_newtable(L); // ..., shapes, wvpt
    lua_createtable(L, 0, 1); // ..., shapes, wvpt, weak_mt
    lua_pushliteral(L, "v"); // ..., shapes, wvpt, weak_mt, "v"
    lua_setfield(L, -2, "__mode"); // ..., shapes, wvpt, weak_mt = { __mode = "v" }
    lua_setmetatable(L, -2); // ..., shapes, wvpt; wvpt.metatable = weak_mt

    sWeakParentRef = lua_ref(L, 1); // ..., shapes; ref = wvpt

    //
    //
    //

    lua_createtable(L, 0, 8); // ..., shapes, counts

    sComponentCountRef = lua_ref(L, 1); // ..., shapes; ref = counts
}

/*


// -----------------------------------------------------------------------------
// SHAPE SAMPLING
// -----------------------------------------------------------------------------
namespace yocto {

    adjacencies, bvh, hash??
*/