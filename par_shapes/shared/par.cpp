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

#include "CoronaLua.h"
#include "CoronaLog.h"
#include "utils/LuaEx.h"
#include <vector>

#define PAR_SHAPES_IMPLEMENTATION

#include "par_shapes.h"

//
//
//

#define PAR_SHAPES_MESH_NAME "par_shapes.mesh"

//
//
//

par_shapes_mesh ** GetMeshBox (lua_State * L, int arg)
{
    return LuaXS::CheckUD<par_shapes_mesh *>(L, arg, PAR_SHAPES_MESH_NAME);
}

par_shapes_mesh * GetMesh (lua_State * L, int arg = 1)
{
    par_shapes_mesh ** box = GetMeshBox(L, arg);

    luaL_argcheck(L, *box, arg, "Mesh has been freed");

    return *box;
}

//
//
//

static bool GetVec3 (lua_State * L, int arg, float & x, float & y, float & z)
{
    bool is_table = lua_istable(L, arg);

    if (is_table)
    {
        lua_getfield(L, arg, "x"); // ..., vec, x
        lua_getfield(L, arg, "y"); // ..., vec, x, y
        lua_getfield(L, arg, "z"); // ..., vec, x, y, z

        ++arg;
    }

    x = LuaXS::Float(L, arg);
    y = LuaXS::Float(L, arg + 1);
    z = LuaXS::Float(L, arg + 2);

    return is_table;
}

//
//
//

static int BoxMesh (lua_State * L, par_shapes_mesh * mesh)
{
    *LuaXS::NewTyped<par_shapes_mesh *>(L) = mesh; // ..., box

    LuaXS::AttachMethods(L, PAR_SHAPES_MESH_NAME, [](lua_State * L){
        luaL_Reg funcs[] = {
            {
                "clone", [](lua_State * L)
                {
                    par_shapes_mesh * out = nullptr;

                    if (!lua_isnil(L, 2))
                    {
                        par_shapes_clone(GetMesh(L), GetMesh(L, 2));

                        lua_settop(L, 2); // mesh, clone

                        return 1;
                    }

                    else return BoxMesh(L, par_shapes_clone(GetMesh(L), nullptr)); // mesh, clone
                }
            }, {
                "compute_aabb", [](lua_State * L)
                {
                    float aabb[6];

                    par_shapes_compute_aabb(GetMesh(L), aabb);

                    for (int i = 0; i < 6; ++i) lua_pushnumber(L, aabb[i]); // minx, miny, minz, maxx, maxy, maxz

                    return 6;
                }
            }, {
                "compute_normals", [](lua_State * L)
                {
                    par_shapes_compute_normals(GetMesh(L));

                    return 0;
                }
            }, {
                "free_mesh", [](lua_State * L)
                {
                    par_shapes_mesh ** box = GetMeshBox(L, 1);

                    if (*box)
                    {
                        par_shapes_free_mesh(*box);

                        *box = nullptr;
                    }

                    return 0;
                }
            }, {
                "__gc", [](lua_State * L)
                {
                    par_shapes_mesh ** box = GetMeshBox(L, 1);

                    if (*box) par_shapes_free_mesh(*box);

                    return 0;
                }
            }, {
                "get_normal", [](lua_State * L)
                {
                    par_shapes_mesh * mesh = GetMesh(L);
                    int index = luaL_checkint(L, 2) - 1;

                    luaL_argcheck(L, index >= 0 && index < mesh->npoints, 2, "Out-of-bounds normals index");
                                
                    for (int i = 0, offset = i * 3; i < 3; ++i) lua_pushinteger(L, mesh->normals[offset + i]); // mesh, index, x, y, z

                    return 1;
                }
            }, {
                "get_point", [](lua_State * L)
                {
                    par_shapes_mesh * mesh = GetMesh(L);
                    int index = luaL_checkint(L, 2) - 1;

                    luaL_argcheck(L, index >= 0 && index < mesh->npoints, 2, "Out-of-bounds points index");
                                
                    for (int i = 0, offset = i * 3; i < 3; ++i) lua_pushinteger(L, mesh->points[offset + i]); // mesh, index, x, y, z

                    return 1;
                }
            }, {
                "get_point_count", [](lua_State * L)
                {
                    lua_pushinteger(L, GetMesh(L)->npoints); // mesh, count

                    return 1;
                }
            }, {
                "get_texcoord", [](lua_State * L)
                {
                    par_shapes_mesh * mesh = GetMesh(L);
                    int index = luaL_checkint(L, 2) - 1;

                    luaL_argcheck(L, index >= 0 && index < mesh->npoints, 2, "Out-of-bounds tcoords index");
                                
                    for (int i = 0, offset = i * 2; i < 2; ++i) lua_pushinteger(L, mesh->tcoords[offset + i]); // mesh, index, u, v

                    return 1;
                }
            }, {
                "get_triangle", [](lua_State * L)
                {
                    par_shapes_mesh * mesh = GetMesh(L);
                    int index = luaL_checkint(L, 2) - 1;

                    luaL_argcheck(L, index >= 0 && index < mesh->ntriangles, 2, "Out-of-bounds triangles index");

                    for (int i = 0, offset = i * 3; i < 3; ++i) lua_pushinteger(L, mesh->triangles[offset + i]); // mesh, index, i1, i2, i3

                    return 1;
                }
            }, {
                "get_triangle_count", [](lua_State * L)
                {
                    lua_pushinteger(L, GetMesh(L)->ntriangles); // mesh, count

                    return 1;
                }
            }, {
                "invert", [](lua_State * L)
                {
                    par_shapes_invert(GetMesh(L), luaL_checkint(L, 2) - 1, luaL_checkint(L, 3));

                    return 0;
                }
            }, {
                "merge", [](lua_State * L)
                {
                    par_shapes_merge(GetMesh(L), GetMesh(L, 2));

                    return 0;
                }
            }, {
                "merge_and_free", [](lua_State * L)
                {
                    par_shapes_merge_and_free(GetMesh(L), GetMesh(L, 2));

                    *GetMeshBox(L, 2) = nullptr;

                    return 0;
                }
            }, {
                "remove_degenerate", [](lua_State * L)
                {
                    par_shapes_remove_degenerate(GetMesh(L), LuaXS::Float(L, 2));

                    return 0;
                }
            }, {
                "rotate", [](lua_State * L)
                {
                    float axis[3];

                    GetVec3(L, 3, axis[0], axis[1], axis[2]);

                    par_shapes_rotate(GetMesh(L), LuaXS::Float(L, 2), axis);

                    return 0;
                }
            }, {
                "scale", [](lua_State * L)
                {
                    float x, y, z;

                    GetVec3(L, 2, x, y, z);

                    par_shapes_scale(GetMesh(L), x, y, z);

                    return 0;
                }
            }, {
                "translate", [](lua_State * L)
                {
                    float x, y, z;

                    GetVec3(L, 2, x, y, z);

                    par_shapes_translate(GetMesh(L), x, y, z);

                    return 0;
                }
            }, {
                "unweld", [](lua_State * L)
                {
                    par_shapes_unweld(GetMesh(L), lua_toboolean(L, 2));

                    return 0;
                }
            }, {
                "weld", [](lua_State * L)
                {
                    par_shapes_mesh * mesh = GetMesh(L);

                    std::vector<PAR_SHAPES_T> indices;

                    if (lua_toboolean(L, 3)) indices.resize(mesh->npoints);

                    BoxMesh(L, par_shapes_weld(mesh, LuaXS::Float(L, 2), !indices.empty() ? indices.data() : nullptr)); // mesh, epsilon[, mapping], welded

                    if (indices.empty()) return 1;

                    else
                    {
                        lua_createtable(L, indices.size(), 0); // mesh, epsilon[, mapping], welded, mapping

                        for (int i = 0; i < indices.size(); ++i)
                        {
                            lua_pushinteger(L, indices[i]); // mesh, epsilon[, mapping], welded, mapping, index
                            lua_rawseti(L, -2, i + 1); // mesh, epsilon[, mapping], welded, mapping = { ..., index }
                        }

                        return 2;
                    }
                }
            },
            { nullptr, nullptr }
        };

        luaL_register(L, nullptr, funcs);
    });

    return 1;
}

//
//
//

CORONA_EXPORT int luaopen_plugin_parshapes (lua_State* L)
{
    lua_newtable(L); // par_shapes

    luaL_Reg funcs[] = {
        {
            "create_cone", [](lua_State * L)
            {
                return BoxMesh(L, par_shapes_create_cone(luaL_checkint(L, 1), luaL_checkint(L, 2))); // slices, stacks, cone
            }
        }, {
            "create_cube", [](lua_State * L)
            {
                return BoxMesh(L, par_shapes_create_cube());
            }
        }, {
            "create_cylinder", [](lua_State * L)
            {
                return BoxMesh(L, par_shapes_create_cylinder(luaL_checkint(L, 1), luaL_checkint(L, 2))); // slices, stacks, cylinder
            }
        }, {
            "create_disk", [](lua_State * L)
            {
                int nindex = 4, slices = luaL_checkint(L, 2);
                float radius = LuaXS::Float(L, 1), center[3], normal[3];
                bool was_center_table = GetVec3(L, 3, center[0], center[1], center[2]); // radius, slices, center / x[, y, z], normal[, cx, cy, cz]

                if (was_center_table) lua_pop(L, 3); // radius, slices, center, normal
                else nindex += 3;

                GetVec3(L, nindex, normal[0], normal[1], normal[2]);

                return BoxMesh(L, par_shapes_create_disk(radius, slices, center, normal)); // radius, slices, center, normal / x[, y, z], disk
            }
        }, {
            "create_dodecahedron", [](lua_State * L)
            {
                return BoxMesh(L, par_shapes_create_dodecahedron()); // dodec
            }
        }, {
            "create_empty", [](lua_State * L)
            {
                return BoxMesh(L, par_shapes_create_empty()); // empty
            }
        }, {
            "create_icosahedron", [](lua_State * L)
            {
                return BoxMesh(L, par_shapes_create_icosahedron()); // icosa
            }
        }, {
            "create_octahedron", [](lua_State * L)
            {
                return BoxMesh(L, par_shapes_create_octahedron()); // octo
            }
        }, {
            "create_hemisphere", [](lua_State * L)
            {
                return BoxMesh(L, par_shapes_create_hemisphere(luaL_checkint(L, 1), luaL_checkint(L, 2))); // slices, stacks, hemi
            }
        }, {
            "create_klein_bottle", [](lua_State * L)
            {
                return BoxMesh(L, par_shapes_create_klein_bottle(luaL_checkint(L, 1), luaL_checkint(L, 2))); // slices, stacks, klein
            }
        }, {
            "create_lsystem", [](lua_State * L)
            {
                return BoxMesh(L, par_shapes_create_lsystem(luaL_checkstring(L, 1), luaL_checkint(L, 2), luaL_checkint(L, 3))); // text, slices, maxdepth, lsystem
            }
        }, {
            "create_parametric", [](lua_State * L)
            {
                struct ParametricInfo {
                    lua_State * mL;
                    bool mAnyErrors;
                } info = { L, false };

                luaL_checktype(L, 1, LUA_TFUNCTION);
                return BoxMesh(L, par_shapes_create_parametric([](const float * in, float * out, void * context) {
                    ParametricInfo * info = static_cast<ParametricInfo *>(context);

                    if (!info->mAnyErrors)
                    {
                        lua_pushvalue(info->mL, 1); // callback, slices, stacks, callback, x
                        lua_pushnumber(info->mL, in[0]); // callback, slices, stacks, callback, x
                        lua_pushnumber(info->mL, in[1]); // callback, slices, stacks, callback, x, y

                        bool ok = lua_pcall(info->mL, 2, 3, 0) == 0; // callback, slices, stacks, x / err[, y, z]

                        if (ok)
                        {
                            for (int i = 0; ok && i < 3; ++i)
                            {
                                if (lua_isnumber(info->mL, -i)) out[i] = LuaXS::Float(info->mL, -i);
                                else
                                {
                                    lua_pushfstring(info->mL, "Result %i expected to be number, got %s", i, luaL_typename(info->mL, -i)); // callback, slices, stacks[, x?[, y?[, z?]]], err

                                    ok = false;
                                }
                            }
                        }

                        if (!ok)
                        {
                            CORONA_LOG_ERROR("Error in create_parametric callback: %s", lua_isstring(info->mL, -1) ? lua_tostring(info->mL, -1) : "?");
                            
                            out[0] = out[1] = out[2] = 0;
                            info->mAnyErrors = true;
                        }
                    }
                }, luaL_checkint(L, 2), luaL_checkint(L, 3), &info)); // callback, slices, stacks, parametric
            }
        }, {
            "create_parametric_disk", [](lua_State * L)
            {
                
                return BoxMesh(L, par_shapes_create_parametric_disk(luaL_checkint(L, 1), luaL_checkint(L, 2))); // slices, stacks, disk
            }
        }, {
            "create_parametric_sphere", [](lua_State * L)
            { 
                return BoxMesh(L, par_shapes_create_parametric_sphere(luaL_checkint(L, 1), luaL_checkint(L, 2))); // slices, stacks, sphere
            }
        }, {
            "create_plane", [](lua_State * L)
            {
                return BoxMesh(L, par_shapes_create_plane(luaL_checkint(L, 1), luaL_checkint(L, 2))); // slices, stacks, plane
            }
        }, {
            "create_rock", [](lua_State * L)
            {

                return BoxMesh(L, par_shapes_create_rock(luaL_checkint(L, 1), luaL_checkint(L, 2))); // seed, nsubdivisions, rock
            }
        }, {
            "create_subdivided_sphere", [](lua_State * L)
            {
                
                return BoxMesh(L, par_shapes_create_subdivided_sphere(luaL_checkint(L, 1))); // nsubdivisions, sphere
            }
        }, {
            "create_torus", [](lua_State * L)
            {
                
                return BoxMesh(L, par_shapes_create_torus(luaL_checkint(L, 1), luaL_checkint(L, 2), LuaXS::Float(L, 3))); // slices, stacks, radius, torus
            }
        }, {
            "create_tetrahedron", [](lua_State * L)
            {
                return BoxMesh(L, par_shapes_create_tetrahedron()); // tetra
            }
        }, {
            "create_trefoil_knot", [](lua_State * L)
            {
                return BoxMesh(L, par_shapes_create_trefoil_knot(luaL_checkint(L, 1), luaL_checkint(L, 2), LuaXS::Float(L, 3))); // slices, stacks, radius, knot
            }
        }, {
            "set_epsilon_welded_normals", [](lua_State * L)
            {
                par_shapes_set_epsilon_welded_normals(LuaXS::Float(L, 1));

                return 0;
            }
        }, {
            "set_epsilon_degenerate_sphere", [](lua_State * L)
            {
                par_shapes_set_epsilon_degenerate_sphere(LuaXS::Float(L, 1));

                return 0;
            }
        },
        { nullptr, nullptr }
    };

    luaL_register(L, nullptr, funcs);

    return 1;
}