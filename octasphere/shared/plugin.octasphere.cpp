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
#include "utils/Path.h"

#define PAR_OCTASPHERE_IMPLEMENTATION

#include "par_octasphere.h"

//
//
//

static par_octasphere_config GetOctasphereConfig (lua_State * L)
{
    par_octasphere_config config;

    config.uv_mode = PAR_OCTASPHERE_UV_LATLONG;
    config.normals_mode = PAR_OCTASPHERE_NORMALS_SMOOTH;

    luaL_checktype(L, 1, LUA_TTABLE);
    lua_getfield(L, 1, "corner_radius"); // params, corner_radius
    lua_getfield(L, 1, "width"); // params, corner_radius, width
    lua_getfield(L, 1, "height"); // params, corner_radius, width, height
    lua_getfield(L, 1, "depth"); // params, corner_radius, width, height, depth
    lua_getfield(L, 1, "num_subdivisions"); // params, corner_radius, width, height, depth, num_subdivisions

    config.num_subdivisions = luaL_optinteger(L, -1, 3);
    config.depth = (float)luaL_optnumber(L, -2, 15);
    config.height = (float)luaL_optnumber(L, -3, 100);
    config.width = (float)luaL_optnumber(L, -4, 100);
    config.corner_radius = (float)luaL_optnumber(L, -5, 5);

    if (config.num_subdivisions < 0) config.num_subdivisions = 0;
    if (config.num_subdivisions >= PAR_OCTASPHERE_MAX_SUBDIVISIONS) config.num_subdivisions = PAR_OCTASPHERE_MAX_SUBDIVISIONS;

    float * vs[] = { &config.depth, &config.height, &config.width, &config.corner_radius };

    for (int i = 0; i < 4; ++i)
    {
        if (*vs[i] < 0) *vs[i] = 0;
        else if (*vs[i] > 100) *vs[i] = 100;
    }

    return config;
}

//
//
//

#define PAR_OCTASPHERE_NAME "shapes.par.octasphere"

//
//
//

par_octasphere_mesh * GetOctasphereMesh (lua_State * L)
{
    return LuaXS::CheckUD<par_octasphere_mesh>(L, 1, PAR_OCTASPHERE_NAME);
}

//
//
//

CORONA_EXPORT int luaopen_plugin_octasphere (lua_State* L)
{
    lua_newtable(L); // octasphere

    luaL_Reg funcs[] = {
        {
            "get_counts", [](lua_State * L)
            {
                par_octasphere_config config = GetOctasphereConfig(L); // params, ...
                uint32_t nindices, nverts;

                par_octasphere_get_counts(&config, &nindices, &nverts);

                lua_pushinteger(L, nindices); // params, ..., nindices
                lua_pushinteger(L, nverts); // params, ..., nindices, nverts

                return 2;
            }
        }, {
            "populate", [](lua_State * L)
            {
                par_octasphere_config config = GetOctasphereConfig(L); // params, ...
                uint32_t nindices, nverts;

                par_octasphere_get_counts(&config, &nindices, &nverts);

                lua_getfield(L, 1, "omit_normals"); // params, ..., omit_normals
                lua_getfield(L, 1, "omit_texcoords"); // params, ..., omit_normals, omit_texcoords

                bool omit_normals = lua_toboolean(L, -2), omit_texcoords = lua_toboolean(L, -1);

                par_octasphere_mesh * mesh = LuaXS::NewTyped<par_octasphere_mesh>(L); // params, ..., omit_normals, omit_texcoord, mesh

                lua_createtable(L, 0, 2 + !omit_normals + !omit_texcoords); // params, ..., omit_normals, omit_texcoord, mesh, env
                
                mesh->positions = LuaXS::NewArray<float>(L, nverts * 3); // params, ..., omit_normals, omit_texcoord, mesh, env, positions
                mesh->indices = LuaXS::NewArray<uint16_t>(L, nindices); // params, ..., omit_normals, omit_texcoord, mesh, env, positions, indices
                
                lua_setfield(L, -3, "indices"); // params, ..., omit_normals, omit_texcoord, mesh, env = { indices = indices }, positions
                lua_setfield(L, -2, "texcoords"); // params, ..., omit_normals, omit_texcoord, mesh, env = { indices, positions = positions }

                if (omit_normals) mesh->normals = nullptr;
                else
                {
                    mesh->normals = LuaXS::NewArray<float>(L, nverts * 3); // params, ..., omit_normals, omit_texcoord, mesh, env, normals
                
                    lua_setfield(L, -2, "normals"); // params, ..., omit_normals, omit_texcoord, mesh, env = { indices, positions, normals = normals }
                }

                if (omit_texcoords) mesh->texcoords = nullptr;
                else
                {
                    mesh->texcoords = LuaXS::NewArray<float>(L, nverts * 2); // params, ..., omit_normals, omit_texcoord, mesh, env, positions, normals, texcoords
                
                    lua_setfield(L, -2, "texcoords"); // params, ..., omit_normals, omit_texcoord, mesh, env = { indices, positions[, normals], texcoords = texcoords }
                }

                lua_setfenv(L, -2); // params, ..., omit_normals, omit_texcoord, mesh; mesh.env = env
                
                par_octasphere_populate(&config, mesh);

                LuaXS::AttachMethods(L, PAR_OCTASPHERE_NAME, [](lua_State * L) {
                    luaL_Reg funcs[] = {
                        {
                            "get_index", [](lua_State * L)
                            {
                                par_octasphere_mesh * mesh = GetOctasphereMesh(L);
                                int index = luaL_checkint(L, 2) - 1;

                                luaL_argcheck(L, index >= 0 && index < mesh->num_indices, 2, "Out-of-bounds indices index");
                                lua_pushinteger(L, mesh->indices[index]);

                                return 1;
                            }
                        }, {
                            "get_index_count", [](lua_State * L)
                            {
                                lua_pushinteger(L, GetOctasphereMesh(L)->num_indices); // mesh, count

                                return 1;
                            }
                        }, {
                            "get_normal", [](lua_State * L)
                            {
                                par_octasphere_mesh * mesh = GetOctasphereMesh(L);

                                if (!mesh->normals)
                                {
                                    lua_pushnil(L); // ..., nil

                                    return 1;
                                }

                                int index = luaL_checkint(L, 2) - 1;

                                luaL_argcheck(L, index >= 0 && index < mesh->num_vertices, 2, "Out-of-bounds normals index");
                                
                                for (int i = 0, offset = i * 3; i < 3; ++i) lua_pushinteger(L, mesh->normals[offset + i]); // mesh, index, x, y, z

                                return 3;
                            }
                        }, {
                            "get_position", [](lua_State * L)
                            {
                                par_octasphere_mesh * mesh = GetOctasphereMesh(L);
                                int index = luaL_checkint(L, 2) - 1;

                                luaL_argcheck(L, index >= 0 && index < mesh->num_vertices, 2, "Out-of-bounds positions index");
                                
                                for (int i = 0, offset = i * 3; i < 3; ++i) lua_pushinteger(L, mesh->positions[offset + i]); // mesh, index, x, y, z

                                return 3;
                            }
                        }, {
                            "get_texcoord", [](lua_State * L)
                            {
                                par_octasphere_mesh * mesh = GetOctasphereMesh(L);

                                if (!mesh->texcoords)
                                {
                                    lua_pushnil(L); // ..., nil

                                    return 1;
                                }

                                int index = luaL_checkint(L, 2) - 1;

                                luaL_argcheck(L, index >= 0 && index < mesh->num_vertices, 2, "Out-of-bounds texcoords index");
                                
                                for (int i = 0, offset = i * 2; i < 2; ++i) lua_pushinteger(L, mesh->texcoords[offset + i]); // mesh, index, u, v

                                return 2;
                            }
                        }, {
                            "get_vertex_count", [](lua_State * L)
                            {
                                lua_pushinteger(L, GetOctasphereMesh(L)->num_vertices); // mesh, count

                                return 1;
                            }
                        },
                        { nullptr, nullptr }
                    };

                    luaL_register(L, nullptr, funcs);

                    //
                    //
                    //

                    PathXS::Directories::Instantiate(L); // ..., mt, dirs

                    lua_pushcclosure(L, [](lua_State * L) {
                        par_octasphere_mesh * mesh = GetOctasphereMesh(L);
                        PathXS::Directories * dirs = LuaXS::UD<PathXS::Directories>(L, lua_upvalueindex(1));
                
                        luaL_checkstring(L, 2);

                        // Adapted from par_shapes_export(): 
                        FILE* objfile = fopen(dirs->Canonicalize(L, false, 2), "wt");
                        float const* points = mesh->positions;
                        float const* tcoords = mesh->texcoords;
                        float const* norms = mesh->normals;
                        uint16_t const* indices = mesh->indices;

                        if (tcoords && norms) {
                            for (int nvert = 0; nvert < mesh->num_vertices; nvert++) {
                                fprintf(objfile, "v %f %f %f\n", points[0], points[1], points[2]);
                                fprintf(objfile, "vt %f %f\n", tcoords[0], tcoords[1]);
                                fprintf(objfile, "vn %f %f %f\n", norms[0], norms[1], norms[2]);
                                points += 3;
                                norms += 3;
                                tcoords += 2;
                            }
                            for (int nface = 0; nface < mesh->num_indices / 3; nface++) {
                                int a = 1 + *indices++;
                                int b = 1 + *indices++;
                                int c = 1 + *indices++;
                                fprintf(objfile, "f %d/%d/%d %d/%d/%d %d/%d/%d\n",
                                    a, a, a, b, b, b, c, c, c);
                            }
                        } else if (norms) {
                            for (int nvert = 0; nvert < mesh->num_vertices; nvert++) {
                                fprintf(objfile, "v %f %f %f\n", points[0], points[1], points[2]);
                                fprintf(objfile, "vn %f %f %f\n", norms[0], norms[1], norms[2]);
                                points += 3;
                                norms += 3;
                            }
                            for (int nface = 0; nface < mesh->num_indices; nface++) {
                                int a = 1 + *indices++;
                                int b = 1 + *indices++;
                                int c = 1 + *indices++;
                                fprintf(objfile, "f %d//%d %d//%d %d//%d\n", a, a, b, b, c, c);
                            }
                        } else if (tcoords) {
                            for (int nvert = 0; nvert < mesh->num_vertices; nvert++) {
                                fprintf(objfile, "v %f %f %f\n", points[0], points[1], points[2]);
                                fprintf(objfile, "vt %f %f\n", tcoords[0], tcoords[1]);
                                points += 3;
                                tcoords += 2;
                            }
                            for (int nface = 0; nface < mesh->num_indices; nface++) {
                                int a = 1 + *indices++;
                                int b = 1 + *indices++;
                                int c = 1 + *indices++;
                                fprintf(objfile, "f %d/%d %d/%d %d/%d\n", a, a, b, b, c, c);
                            }
                        } else {
                            for (int nvert = 0; nvert < mesh->num_vertices; nvert++) {
                                fprintf(objfile, "v %f %f %f\n", points[0], points[1], points[2]);
                                points += 3;
                            }
                            for (int nface = 0; nface < mesh->num_indices; nface++) {
                                int a = 1 + *indices++;
                                int b = 1 + *indices++;
                                int c = 1 + *indices++;
                                fprintf(objfile, "f %d %d %d\n", a, b, c);
                            }
                        }
    
                        fclose(objfile);

                        return 0;
                    }, 1); // ..., mt, Export
                    lua_setfield(L, -2, "export"); // ..., mt = { ..., export = Export }

                });

                return 1;
            }
        },
        { nullptr, nullptr }
    };

    luaL_register(L, nullptr, funcs);

    return 1;
}