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

#pragma once

#include "common.h"

#define PAR_OCTASPHERE_IMPLEMENTATION

#include "par_octasphere.h"

//
//
//

static void AddShapes (lua_State * L)
{
/*
typedef struct par_shapes_mesh_s {
    float* points;           // Flat list of 3-tuples (X Y Z X Y Z...)
    int npoints;             // Number of points
    PAR_SHAPES_T* triangles; // Flat list of 3-tuples (I J K I J K...)
    int ntriangles;          // Number of triangles
    float* normals;          // Optional list of 3-tuples (X Y Z X Y Z...)
    float* tcoords;          // Optional list of 2-tuples (U V U V U V...)
} par_shapes_mesh;

void par_shapes_free_mesh(par_shapes_mesh*);

// Generators ------------------------------------------------------------------

// Instance a cylinder that sits on the Z=0 plane using the given tessellation
// levels across the UV domain.  Think of "slices" like a number of pizza
// slices, and "stacks" like a number of stacked rings.  Height and radius are
// both 1.0, but they can easily be changed with par_shapes_scale.
par_shapes_mesh* par_shapes_create_cylinder(int slices, int stacks);

// Cone is similar to cylinder but the radius diminishes to zero as Z increases.
// Again, height and radius are 1.0, but can be changed with par_shapes_scale.
par_shapes_mesh* par_shapes_create_cone(int slices, int stacks);

// Create a disk of radius 1.0 with texture coordinates and normals by squashing
// a cone flat on the Z=0 plane.
par_shapes_mesh* par_shapes_create_parametric_disk(int slices, int stacks);

// Create a donut that sits on the Z=0 plane with the specified inner radius.
// The outer radius can be controlled with par_shapes_scale.
par_shapes_mesh* par_shapes_create_torus(int slices, int stacks, float radius);

// Create a sphere with texture coordinates and small triangles near the poles.
par_shapes_mesh* par_shapes_create_parametric_sphere(int slices, int stacks);

// Approximate a sphere with a subdivided icosahedron, which produces a nice
// distribution of triangles, but no texture coordinates.  Each subdivision
// level scales the number of triangles by four, so use a very low number.
par_shapes_mesh* par_shapes_create_subdivided_sphere(int nsubdivisions);

// More parametric surfaces.
par_shapes_mesh* par_shapes_create_klein_bottle(int slices, int stacks);
par_shapes_mesh* par_shapes_create_trefoil_knot(int slices, int stacks,
    float radius);
par_shapes_mesh* par_shapes_create_hemisphere(int slices, int stacks);
par_shapes_mesh* par_shapes_create_plane(int slices, int stacks);

// Create a parametric surface from a callback function that consumes a 2D
// point in [0,1] and produces a 3D point.
typedef void (*par_shapes_fn)(float const*, float*, void*);
par_shapes_mesh* par_shapes_create_parametric(par_shapes_fn, int slices,
    int stacks, void* userdata);

// Generate points for a 20-sided polyhedron that fits in the unit sphere.
// Texture coordinates and normals are not generated.
par_shapes_mesh* par_shapes_create_icosahedron();

// Generate points for a 12-sided polyhedron that fits in the unit sphere.
// Again, texture coordinates and normals are not generated.
par_shapes_mesh* par_shapes_create_dodecahedron();

// More platonic solids.
par_shapes_mesh* par_shapes_create_octahedron();
par_shapes_mesh* par_shapes_create_tetrahedron();
par_shapes_mesh* par_shapes_create_cube();

// Generate an orientable disk shape in 3-space.  Does not include normals or
// texture coordinates.
par_shapes_mesh* par_shapes_create_disk(float radius, int slices,
    float const* center, float const* normal);

// Create an empty shape.  Useful for building scenes with merge_and_free.
par_shapes_mesh* par_shapes_create_empty();

// Generate a rock shape that sits on the Y=0 plane, and sinks into it a bit.
// This includes smooth normals but no texture coordinates.  Each subdivision
// level scales the number of triangles by four, so use a very low number.
par_shapes_mesh* par_shapes_create_rock(int seed, int nsubdivisions);

// Create trees or vegetation by executing a recursive turtle graphics program.
// The program is a list of command-argument pairs.  See the unit test for
// an example.  Texture coordinates and normals are not generated.
par_shapes_mesh* par_shapes_create_lsystem(char const* program, int slices,
    int maxdepth);

// Queries ---------------------------------------------------------------------

// Dump out a text file conforming to the venerable OBJ format.
void par_shapes_export(par_shapes_mesh const*, char const* objfile);

// Take a pointer to 6 floats and set them to min xyz, max xyz.
void par_shapes_compute_aabb(par_shapes_mesh const* mesh, float* aabb);

// Make a deep copy of a mesh.  To make a brand new copy, pass null to "target".
// To avoid memory churn, pass an existing mesh to "target".
par_shapes_mesh* par_shapes_clone(par_shapes_mesh const* mesh,
    par_shapes_mesh* target);

// Transformations -------------------------------------------------------------

void par_shapes_merge(par_shapes_mesh* dst, par_shapes_mesh const* src);
void par_shapes_translate(par_shapes_mesh*, float x, float y, float z);
void par_shapes_rotate(par_shapes_mesh*, float radians, float const* axis);
void par_shapes_scale(par_shapes_mesh*, float x, float y, float z);
void par_shapes_merge_and_free(par_shapes_mesh* dst, par_shapes_mesh* src);

// Reverse the winding of a run of faces.  Useful when drawing the inside of
// a Cornell Box.  Pass 0 for nfaces to reverse every face in the mesh.
void par_shapes_invert(par_shapes_mesh*, int startface, int nfaces);

// Remove all triangles whose area is less than minarea.
void par_shapes_remove_degenerate(par_shapes_mesh*, float minarea);

// Dereference the entire index buffer and replace the point list.
// This creates an inefficient structure, but is useful for drawing facets.
// If create_indices is true, a trivial "0 1 2 3..." index buffer is generated.
void par_shapes_unweld(par_shapes_mesh* mesh, bool create_indices);

// Merge colocated verts, build a new index buffer, and return the
// optimized mesh.  Epsilon is the maximum distance to consider when
// welding vertices. The mapping argument can be null, or a pointer to
// npoints integers, which gets filled with the mapping from old vertex
// indices to new indices.
par_shapes_mesh* par_shapes_weld(par_shapes_mesh const*, float epsilon,
    PAR_SHAPES_T* mapping);

// Compute smooth normals by averaging adjacent facet normals.
void par_shapes_compute_normals(par_shapes_mesh* m);

// Global Config ---------------------------------------------------------------

void par_shapes_set_epsilon_welded_normals(float epsilon);
void par_shapes_set_epsilon_degenerate_sphere(float epsilon);
*/
}

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

void add_octasphere (lua_State * L)
{
    lua_newtable(L); // ..., shapes, octasphere

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

                par_octasphere_mesh * mesh = LuaXS::NewTyped<par_octasphere_mesh>(L); // params, ..., mesh

                lua_createtable(L, 0, 4); // params, ..., mesh, env
                
                mesh->positions = LuaXS::NewArray<float>(L, nverts * 3); // params, ..., mesh, env, positions
                mesh->normals = LuaXS::NewArray<float>(L, nverts * 3); // params, ..., mesh, env, positions, normals
                mesh->texcoords = LuaXS::NewArray<float>(L, nverts * 2); // params, ..., mesh, env, positions, normals, texcoords
                mesh->indices = LuaXS::NewArray<uint16_t>(L, nindices); // params, ..., mesh, env, positions, normals, texcoords, indices

                lua_setfield(L, -5, "indices"); // params, ..., mesh, env = { indices = indices }, positions, normals, texcoords
                lua_setfield(L, -4, "texcoords"); // params, ..., mesh, env = { indices, texcoords = texcoords }, positions, normals
                lua_setfield(L, -3, "normals"); // params, ..., mesh, env = { indices, texcoords, normals = normals }, positions
                lua_setfield(L, -2, "positions"); // params, ..., mesh, env = { indices, texcoords, normals, positions = positions }
                lua_setfenv(L, -2); // params, ... mesh; mesh.env = env
                
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
                                int index = luaL_checkint(L, 2) - 1;

                                luaL_argcheck(L, index >= 0 && index < mesh->num_vertices, 2, "Out-of-bounds normals index");
                                
                                for (int i = 0, offset = i * 3; i < 3; ++i) lua_pushinteger(L, mesh->normals[offset + i]); // mesh, index, x, y, z

                                return 1;
                            }
                        }, {
                            "get_position", [](lua_State * L)
                            {
                                par_octasphere_mesh * mesh = GetOctasphereMesh(L);
                                int index = luaL_checkint(L, 2) - 1;

                                luaL_argcheck(L, index >= 0 && index < mesh->num_vertices, 2, "Out-of-bounds positions index");
                                
                                for (int i = 0, offset = i * 3; i < 3; ++i) lua_pushinteger(L, mesh->positions[offset + i]); // mesh, index, x, y, z

                                return 1;
                            }
                        }, {
                            "get_texcoord", [](lua_State * L)
                            {
                                par_octasphere_mesh * mesh = GetOctasphereMesh(L);
                                int index = luaL_checkint(L, 2) - 1;

                                luaL_argcheck(L, index >= 0 && index < mesh->num_vertices, 2, "Out-of-bounds texcoords index");
                                
                                for (int i = 0, offset = i * 2; i < 2; ++i) lua_pushinteger(L, mesh->texcoords[offset + i]); // mesh, index, u, v

                                return 1;
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
                });

                return 1;
            }
        },
        { nullptr, nullptr }
    };

    luaL_register(L, nullptr, funcs);
    lua_setfield(L, -2, "octagon"); // ..., shapes = { ..., octasphere = octasphere }
}