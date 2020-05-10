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

#include "ByteReader.h"
#include "utils/Byte.h"
#include "utils/LuaEx.h"
#include "utils/Memory.h"
#include "utils/Thread.h"
#include <vector>

ThreadXS::TLS<MemoryXS::ScopedListSystem *> tls_streamlines;

void FailAssert (const char * what) { tls_streamlines->FailAssert(what); }
void * MallocMS (size_t size) { return tls_streamlines->Malloc(size); }
void * CallocMS (size_t num, size_t _) { return tls_streamlines->Calloc(num, 1); }
void * ReallocMS (void * ptr, size_t size) { return tls_streamlines->Realloc(ptr, size); }
void FreeMS (void * ptr) { tls_streamlines->Free(ptr); }

#define ASSERT(cond) if (!(cond)) FailAssert(#cond)

#define PAR_MALLOC(T, N) ((T*) MallocMS(N * sizeof(T)))
#define PAR_CALLOC(T, N) ((T*) CallocMS(N * sizeof(T), 1))
#define PAR_REALLOC(T, BUF, N) ((T*) ReallocMS(BUF, sizeof(T) * (N)))
#define PAR_FREE(BUF) FreeMS(BUF)

#define PAR_STREAMLINES_IMPLEMENTATION

#include "par_streamlines.h"

static int AuxGetFlag (lua_State * L, int sarg)
{
	#define LIST() WITH(WIREFRAME), WITH(ANNOTATIONS), WITH(SPINE_LENGTHS), WITH(RANDOM_OFFSETS), WITH(CURVE_GUIDES)
	#define WITH(n) #n

	const char * const names[] = { LIST(), nullptr };

	#undef WITH
	#define WITH(n) PARSL_FLAG_##n

	const int flags[] = { LIST() };

	#undef WITH
	#undef LIST

	return flags[luaL_checkoption(L, sarg, nullptr, names)];
}

#undef NAME
#undef FLAG

static int GetFlags (lua_State * L, int arg, bool bMulti = false)
{
	int flags = 0;

	switch (lua_type(L, arg))
	{
	case LUA_TSTRING:
		flags = AuxGetFlag(L, arg);
		break;
	case LUA_TTABLE:
		for (size_t i = 1, n = lua_objlen(L, arg); i <= n; ++i)
		{
			lua_rawgeti(L, arg, i);	// ..., flag_table, ..., flag

			flags |= AuxGetFlag(L, -1);

			lua_pop(L, 1);	// ..., flag_table, ...
		}

		break;
	}

	return flags;
}

template<bool = sizeof(unsigned long long) <= sizeof(lua_Number)> struct WithID {
	static unsigned long long Get (lua_State * L)
	{
		lua_Number n = lua_tonumber(L, -1);

		return *reinterpret_cast<unsigned long long *>(&n);
	}

	static void Set (lua_State * L, unsigned long long id, bool = false)
	{
		lua_pushnumber(L, *reinterpret_cast<lua_Number *>(&id));// ..., env, id
		lua_rawseti(L, -2, 1);	// ..., env = { id[, ...] }
	}
};

template<> unsigned long long WithID<false>::Get (lua_State * L)
{
	return *LuaXS::UD<unsigned long long>(L, -1);
}

template<> void WithID<false>::Set (lua_State * L, unsigned long long id, bool bNew)
{
	if (bNew) lua_newuserdata(L, sizeof(unsigned long long));	// ..., env, id

	*LuaXS::UD<unsigned long long>(L, -1) = id;

	lua_rawseti(L, -2, 1);	// ..., env = { id[, ...] }
}

struct Proxy {
	enum Generation : unsigned long long { eNone = 0ULL, eFirst = 1ULL };
	enum Mode { eAnnotations, ePositions, eRandomOffsets, eSpineLengths, eTriangleIndices };

	Mode mMode{ePositions};
	unsigned long long mGeneration{eNone};

	const parsl_mesh * GetMesh (lua_State * L, int arg = 1)
	{
		if (eNone == mGeneration) return nullptr;

		lua_getfenv(L, arg);// ..., proxy, ..., env
		lua_rawgeti(L, -1, 1);	// ..., proxy, ..., env, id

		const parsl_mesh * mesh{nullptr};

		if (WithID<>::Get(L) == mGeneration)
		{
			lua_rawgeti(L, -2, 2);	// ..., proxy, ... env, id, mesh

			mesh = LuaXS::UD<const parsl_mesh>(L, -1);

			lua_pop(L, 1);	// ..., proxy, ..., env, id
		}

		lua_pop(L, 2);	// ..., proxy, ...

		return mesh;
	}
};

static Proxy * GetProxy (lua_State * L)
{
	return LuaXS::CheckUD<Proxy>(L, 1, "streamlines.proxy");
}

static int NewProxy (lua_State * L)
{
	LuaXS::NewTyped<Proxy>(L);	// proxy
	LuaXS::AttachMethods(L, "streamlines.proxy", [](lua_State * L)
	{
		luaL_Reg proxy_funcs[] = {
			{
				"GetMode", [](lua_State * L)
				{
					Proxy * proxy = GetProxy(L);

					if (!proxy->GetMesh(L)) lua_pushliteral(L, "none");	// proxy, "none"

					else
					{
						switch (proxy->mMode)
						{
						case Proxy::eAnnotations:
							lua_pushliteral(L, "annotations");	// proxy, "annotations"
							break;
							break;
						case Proxy::ePositions:
							lua_pushliteral(L, "positions");// proxy, "positions"
							break;
						case Proxy::eRandomOffsets:
							lua_pushliteral(L, "random_offsets");	// proxy, "random_offsets"
							break;
						case Proxy::eSpineLengths:
							lua_pushliteral(L, "spine_lengths");	// proxy, "spine_lengths"
							break;
						case Proxy::eTriangleIndices:
							lua_pushliteral(L, "triangle_indices");	// proxy, "tirangle_indices"
						}
					}

					return 1;
				}
			}, {
				"Reset", [](lua_State * L)
				{
					GetProxy(L)->mGeneration = Proxy::eNone;

					return 0;
				}
			},
			{ nullptr, nullptr }
		};

		luaL_register(L, nullptr, proxy_funcs);

		ByteReaderFunc * func = ByteReader::Register(L);

		func->mGetBytes = [](lua_State * L, ByteReader & reader, int arg, void *)
		{
			Proxy * proxy = LuaXS::UD<Proxy>(L, arg);
			const parsl_mesh * mesh = proxy->GetMesh(L, arg);

			void * ptr{nullptr};
			size_t n;

			if (mesh)
			{
				switch (proxy->mMode)
				{
				case Proxy::eAnnotations:
					ptr = mesh->annotations;
					n = sizeof(parsl_annotation) * mesh->num_vertices;
					break;
				case Proxy::ePositions:
					ptr = mesh->positions;
					n = sizeof(parsl_position) * mesh->num_vertices;
					break;
				case Proxy::eRandomOffsets:
					ptr = mesh->random_offsets;
					n = sizeof(float) * mesh->num_vertices;
					break;
				case Proxy::eSpineLengths:
					ptr = mesh->spine_lengths;
					n = sizeof(float) * mesh->num_vertices;
					break;
				case Proxy::eTriangleIndices:
					ptr = mesh->triangle_indices;
					n = sizeof(uint32_t) * 3U * mesh->num_triangles;
					break;
				}
			}

			if (ptr)
			{
				reader.mBytes = ptr;
				reader.mCount = n;
			}

			else
			{
				reader.mBytes = nullptr;
				reader.mCount = 0U;
			}

			return true;
		};

		lua_pushlightuserdata(L, func);	// proxy, meta, func
		lua_setfield(L, -2, "__bytes");	// ..., proxy, meta = { ..., __bytes = func }
	});

	return 1;
}

static bool BindProxy (lua_State * L, int n, Proxy::Mode mode)
{
	if (LuaXS::IsType(L, "streamlines.proxy"))
	{
		Proxy * proxy = LuaXS::UD<Proxy>(L, -1);

		lua_getfenv(L, 1);	// mesh, ..., proxy, env
		lua_rawgeti(L, -1, 1);	// mesh, ..., proxy, env, id

		proxy->mGeneration = WithID<>::Get(L); // n.b. mesh still valid when called

		lua_pop(L, 1);	// mesh, ..., proxy, env
		lua_setfenv(L, -1);	// mesh, ..., proxy; proxy.env = env

		proxy->mMode = mode;

		return true;
	}

	if (!lua_istable(L, -1)) n ? lua_createtable(L, n, 0) : lua_pushnil(L);	// ...[, non_out], out / nil

	return false;
}

static const parsl_mesh * GetMesh (lua_State * L)
{
	const parsl_mesh * mesh = *LuaXS::CheckUD<const parsl_mesh *>(L, 1, "streamlines.mesh");

	lua_getfenv(L, 1);	// mesh, ..., env
	lua_rawgeti(L, -1, 1);	// mesh, ..., env, id

	luaL_argcheck(L, WithID<>::Get(L) != Proxy::eNone, 1, "Mesh's underlying context has been collected");
	lua_pop(L, 2);	// mesh, ...

	return mesh;
}

static int WrapMesh (lua_State * L, const parsl_mesh * mesh, MemoryXS::ScopedList & mem)
{
	// A two-tiered environment is used to manage the context, its mesh, and any proxies. The
	// layout goes:
	//
	// { { id, mesh_ptr }, mesh }
	//
	// where `mesh` stores a reference to the context's mesh, initialized on first use.
	//
	// The inner environment is given to the mesh as well, then given to a proxy when bound.
	// Said binding involves setting the proxy's ID to the context's current value. Each mesh
	// operation will increment the context's ID, so the proxy can compare its own value to
	// this one to know when it becomes invalid. For the same reason, a context will also be
	// given a "no ID" value when being garbage collected; in this case, it does double duty
	// by making any subsequent mesh methods error out.

	lua_getfenv(L, 1);	// context, ..., cenv
	lua_rawgeti(L, -1, 1);	// context, ..., cenv, menv
	lua_rawgeti(L, -2, 2);	// context, ..., cenv, menv, mesh?

	if (!lua_isnil(L, -1)) // already exists?
	{
		lua_replace(L, -3);// context, ..., mesh, menv
		lua_rawgeti(L, -1, 1);	// context, ..., mesh, menv, id

		WithID<>::Set(L, WithID<>::Get(L) + 1ULL); // context, ..., mesh, menv = { id + 1, mesh_ptr }

		lua_pop(L, 1);	// context, ..., mesh
	}

	else
	{
		lua_pop(L, 1);	// context, ..., cenv, menv

		WithID<>::Set(L, Proxy::eFirst);// context, ..., cenv, menv = { first_id }

		lua_pushlightuserdata(L, const_cast<parsl_mesh *>(mesh));	// context, ..., cenv, menv, mesh_ptr
		lua_rawseti(L, -2, 2);	// context, ..., cenv, menv = { first_id, mesh_ptr }

		LuaXS::NewTyped<const parsl_mesh *>(L, mesh);	// context, ..., cenv, menv, mesh

		lua_pushvalue(L, -1);	// context, ..., cenv, menv, mesh, mesh
		lua_rawseti(L, -3, 2);	// context, ..., cenv = { menv, mesh }, menv, mesh
		lua_replace(L, -3);	// context, ..., mesh, menv
		lua_setfenv(L, -2);	// context, ..., mesh; mesh.env = menv
	}

	mem.RemoveAll();

	LuaXS::AttachMethods(L, "streamlines.mesh", [](lua_State * L) {
		luaL_Reg mesh_funcs[] = {
			{
				"GetAnnotations", [](lua_State * L)
				{
					const parsl_mesh * mesh = GetMesh(L);

					lua_settop(L, 2);	// mesh, opts

					if (lua_istable(L, 2))
					{
						lua_getfield(L, 2, "out");	// mesh, opts, out?
						lua_replace(L, 2);	// mesh, out?
					}

					uint32_t n = mesh->annotations ? mesh->num_vertices : 0U;

					if (BindProxy(L, n, Proxy::eAnnotations)) return 1;	// mesh, opts / out, annotations? / proxy

					for (int i = 0; i < int(n); ++i)
					{
						lua_rawgeti(L, -1, i + 1);	// mesh, opts / out, annotations, t?

						if (!lua_istable(L, -1))
						{
							lua_remove(L, -1);	// mesh, opts / out, annotations
							lua_createtable(L, 0, 4);	// mesh, opts / out, annotations, t
							lua_pushvalue(L, -1);	// mesh, opts / out, annotations, t, t
							lua_rawseti(L, -3, i + 1);	// mesh, opts / out, annotations = { ..., t, ... }, t
						}

						parsl_annotation & annotation = mesh->annotations[i];

						lua_pushnumber(L, annotation.u_along_curve);	// mesh, opts / out, annotations, t, u_along_curve
						lua_setfield(L, -2, "x");	// mesh, opts / out, annotations, t = { x = u_along_curve }
						lua_pushnumber(L, annotation.v_across_curve);	// mesh, opts / out, annotations, t, v_across_curve
						lua_setfield(L, -2, "y");	// mesh, opts / out, annotations, t = { x, y = v_across_curve }
						lua_pushnumber(L, annotation.spine_to_edge_x);	// mesh, opts / out, annotations, t, spine_to_edge_x
						lua_setfield(L, -2, "z");	// mesh, opts / out, annotations, t = { x, y, z = spine_to_edge_x }
						lua_pushnumber(L, annotation.spine_to_edge_y);	// mesh, opts / out, annotations, t, v_across_curve
						lua_setfield(L, -2, "w");	// mesh, opts / out, annotations, t = { x, y, z, w = spine_to_edge_y }
						lua_pop(L, 1);	// mesh, opts / out, annotations
					}

					return 1;
				}
			}, {
				"GetNumTriangles", [](lua_State * L)
				{
					return LuaXS::PushArgAndReturn(L, GetMesh(L)->num_triangles);	// mesh, ntris
				}
			}, {
				"GetNumVertices", [](lua_State * L)
				{
					return LuaXS::PushArgAndReturn(L, GetMesh(L)->num_vertices);	// mesh, nvertices
				}
			}, {
				"GetPositions", [](lua_State * L)
				{
					const parsl_mesh * mesh = GetMesh(L);

					lua_settop(L, 2);	// mesh, opts

					if (lua_istable(L, 2))
					{
						lua_getfield(L, 2, "out");	// mesh, opts, out?
						lua_replace(L, 2);	// mesh, out?
					}

					if (BindProxy(L, mesh->num_vertices * 2U, Proxy::ePositions)) return 1;	// mesh, opts / out, positions / proxy

					for (int i = 0; i < int(mesh->num_vertices); ++i)
					{
						lua_pushnumber(L, mesh->positions[i].x);// mesh, opts / out, positions, x
						lua_rawseti(L, -2, 2 * i + 1);	// mesh, opts / out, positions = { ..., x }
						lua_pushnumber(L, mesh->positions[i].y);// mesh, opts / out, positions, y
						lua_rawseti(L, -2, 2 * i + 2);	// mesh, opts / out, positions = { ..., x, y }
					}

					return 1;
				}
			}, {
				"GetRandomOffsets", [](lua_State * L)
				{
					const parsl_mesh * mesh = GetMesh(L);

					lua_settop(L, 2);	// mesh, opts

					if (lua_istable(L, 2))
					{
						lua_getfield(L, 2, "out");	// mesh, opts, out?
						lua_replace(L, 2);	// mesh, out?
					}

					uint32_t n = mesh->random_offsets ? mesh->num_vertices : 0U;

					if (BindProxy(L, mesh->num_vertices, Proxy::eRandomOffsets)) return 1;	// mesh, opts / out, random_offsets? / proxy

					for (int i = 0; i < int(n); ++i)
					{
						lua_pushnumber(L, mesh->random_offsets[i]);	// mesh, opts / out, random_offsets, offset
						lua_rawseti(L, -2, i + 1);	// mesh, opts / out, random_offsets = { ..., offset }
					}

					return 1;
				}
			}, {
				"GetSpineLengths", [](lua_State * L)
				{
					const parsl_mesh * mesh = GetMesh(L);

					lua_settop(L, 2);	// mesh, opts

					if (lua_istable(L, 2))
					{
						lua_getfield(L, 2, "out");	// mesh, opts, out?
						lua_replace(L, 2);	// mesh, out?
					}

					uint32_t n = mesh->spine_lengths ? mesh->num_vertices : 0U;

					if (BindProxy(L, mesh->num_vertices, Proxy::eSpineLengths)) return 1;	// mesh, opts / out, spine_lengths? / proxy

					for (int i = 0; i < int(n); ++i)
					{
						lua_pushnumber(L, mesh->spine_lengths[i]);	// mesh, opts / out, spine_lengths, length
						lua_rawseti(L, -2, i + 1);	// mesh, opts / out, spine_lengths = { ..., length }
					}

					return 1;
				}
			}, {
				"GetTriangles", [](lua_State * L)
				{
					const parsl_mesh * mesh = GetMesh(L);
					uint32_t extra = 0U;

					if (lua_istable(L, 2))
					{
						lua_getfield(L, 2, "one_based");// mesh, opts, one_based
						lua_getfield(L, 2, "out");	// mesh, opts, one_based, out?
						lua_replace(L, 2);	// mesh, out?, one_based

						extra = lua_toboolean(L, -1);
					}

					lua_settop(L, 2);	// mesh, opts / out

					uint32_t n = mesh->num_triangles * 3U;

					if (BindProxy(L, n, Proxy::eTriangleIndices)) return 1;	// mesh, opts / out, indices / proxy

					for (int i = 0; i < int(n); ++i)
					{
						lua_pushinteger(L, mesh->triangle_indices[i] + extra);	// mesh, opts / out, indices, index
						lua_rawseti(L, -2, i + 1);	// mesh, opts / out, indices = { ..., index }
					}

					return 1;
				}
			},
			{ nullptr, nullptr }
		};

		luaL_register(L, nullptr, mesh_funcs);
	});

	return 1;
}

static parsl_context * GetContext (lua_State * L)
{
	return *LuaXS::CheckUD<parsl_context *>(L, 1, "streamlines.context");
}

struct SpineList {
	std::vector<parsl_position> mVertices;
	std::vector<uint16_t> mSpineLengths;
};

static parsl_spine_list GetSpineList (lua_State * L, SpineList & temp)
{
	parsl_spine_list list;

	luaL_checktype(L, 2, LUA_TTABLE);
	lua_getfield(L, 2, "vertices");	// context, params, vertices

	ByteReader vertices{L, -1};	// context, params, vertices[, err]

	if (vertices.mBytes)
	{
		list.vertices = (parsl_position *)vertices.mBytes;
		list.num_vertices = vertices.mCount / sizeof(parsl_position);
	}

	else
	{
		lua_pop(L, 1);	// context, params, vertices
		luaL_checktype(L, -1, LUA_TTABLE);

		for (size_t i = 1, n = lua_objlen(L, -1); i < n; i += 2, lua_pop(L, 2))
		{
			lua_rawgeti(L, -1, i);	// context, params, vertices, x
			lua_rawgeti(L, -2, i + 1);	// context, params, vertices, x, y

			parsl_position v = { LuaXS::Float(L, -2), LuaXS::Float(L, -1) };

			temp.mVertices.push_back(v);
		}

		list.vertices = temp.mVertices.data();
		list.num_vertices = temp.mVertices.size();
	}
	
	lua_getfield(L, 2, "spine_lengths");// context, params, vertices, spine_lengths

	ByteReader spine_lengths{L, -1};	// context, params, vertices, spine_lengths[, err]

	if (spine_lengths.mBytes)
	{
		list.spine_lengths = (uint16_t *)spine_lengths.mBytes;
		list.num_spines = spine_lengths.mCount / sizeof(uint16_t);
	}

	else
	{
		lua_pop(L, 1);	// context, params, vertices, spine_lengths
		luaL_checktype(L, -1, LUA_TTABLE);

		for (size_t i = 1, n = lua_objlen(L, -1); i <= n; ++i, lua_pop(L, 1))
		{
			lua_rawgeti(L, 4, i);	// context, params, vertices, spine_lengths, length

			int length = luaL_checkinteger(L, -1);

			luaL_argcheck(L, length > 0, 2, "Expected positive length");

			temp.mSpineLengths.push_back(uint16_t(length));
		}

		list.spine_lengths = temp.mSpineLengths.data();
		list.num_spines = temp.mSpineLengths.size();
	}

	uint16_t n = 0U;

	for (uint16_t i = 0; i < list.num_spines; ++i) n += list.spine_lengths[i];

	luaL_argcheck(L, n <= list.num_vertices, 2, "Spine lengths exceed vertices");
	lua_getfield(L, 2, "closed");	// context, params, vertices, spine_lengths, closed

	list.closed = lua_toboolean(L, -1) != 0;

	return list;
}

static int NewContext (lua_State * L, parsl_context * context, MemoryXS::ScopedList & mem)
{
    LuaXS::NewTyped<parsl_context *>(L, context);  // ..., context

	lua_createtable(L, 2, 0);	// ..., context, cenv
	lua_createtable(L, 2, 0);	// ..., context, cenv, menv
	
	WithID<>::Set(L, Proxy::eNone, true);	// context, cenv, menv = { none }

	lua_rawseti(L, -2, 1);	// ..., context, cenv = { menv }
	lua_setfenv(L, -2);	// ..., context; context.env = env

	mem.RemoveAll();

    LuaXS::AttachMethods(L, "streamlines.context", [](lua_State * L) {
		luaL_reg context_methods[] = {
            {
                "__gc", [](lua_State * L)
                {
					lua_getfenv(L, 1);	// context, cenv
					lua_rawgeti(L, -1, 1);	// context, cenv, menv

					WithID<>::Set(L, Proxy::eNone);	// context, cenv, menv = { none[, mesh_ptr] }

                    parsl_destroy_context(GetContext(L));
                    
                    return 0;
                }
            }, {
				"mesh_from_curves_cubic", [](lua_State * L)
				{
					SpineList temp;

					auto bm = tls_streamlines->Bookmark();
					parsl_mesh * mesh = parsl_mesh_from_curves_cubic(GetContext(L), GetSpineList(L, temp));

					return WrapMesh(L, mesh, bm);
				}
			}, {
				"mesh_from_curves_quadratic", [](lua_State * L)
				{
					SpineList temp;

					auto bm = tls_streamlines->Bookmark();
					parsl_mesh * mesh = parsl_mesh_from_curves_quadratic(GetContext(L), GetSpineList(L, temp));

					return WrapMesh(L, mesh, bm);
					
				}
			}, {
				"mesh_from_lines", [](lua_State * L)
				{
					SpineList temp;

					auto bm = tls_streamlines->Bookmark();
					parsl_mesh * mesh = parsl_mesh_from_lines(GetContext(L), GetSpineList(L, temp));

					return WrapMesh(L, mesh, bm);
				}
			}, {
				"mesh_from_streamlines", [](lua_State * L)
				{
					luaL_getmetafield(L, 2, "__call");	// context, advect, first_tick, num_ticks, __call

					bool has_call = !lua_isnil(L, -1);

					lua_pop(L, 1);	// context, advect, first_tick, num_ticks
					luaL_argcheck(L, has_call || lua_isfunction(L, 2), 2, "Uncallable `advect`");

					int first_tick = luaL_checkinteger(L, 3);
					int num_ticks = luaL_checkinteger(L, 4);

					luaL_argcheck(L, first_tick >= 0, 3, "First tick must be non-negative integer");
					luaL_argcheck(L, num_ticks > 0, 3, "Number of ticks must be positive integer");

					struct AdvectState {
						lua_State * mL;
						bool mOK;
					} state = { L, true };
					
					auto bm = tls_streamlines->Bookmark();
					parsl_mesh * mesh = parsl_mesh_from_streamlines(GetContext(L), [](parsl_position * point, void * ud) {
						AdvectState * as = static_cast<AdvectState *>(ud);

						if (!as->mOK) return;

						lua_pushnumber(as->mL, point->x);	// context, advect, first_tick, num_ticks, x
						lua_pushnumber(as->mL, point->y);	// context, advect, first_tick, num_ticks, x, y

						if (lua_pcall(as->mL, 2, 2, 0) != 0)	// context, advect, first_tick, num_ticks, newx? / err[, newy?]
						{
							if (lua_type(as->mL, -2) == LUA_TNUMBER) point->x = float(lua_tonumber(as->mL, -2));
							if (lua_type(as->mL, -1) == LUA_TNUMBER) point->y = float(lua_tonumber(as->mL, -1));

							lua_pop(as->mL, 2);	// context, advect, first_tick, num_ticks
						}

						else as->mOK = false;
					}, uint32_t(first_tick), uint32_t(num_ticks), &state);

					if (!state.mOK) return 1;

					return WrapMesh(L, mesh, bm);
				}
			},
			{ nullptr, nullptr }
		};

		luaL_register(L, nullptr, context_methods);
	});

	return 1;
}

CORONA_EXPORT int luaopen_plugin_streamlines (lua_State * L)
{
	tls_streamlines = MemoryXS::ScopedListSystem::New(L);

	lua_newtable(L);// sl

	luaL_Reg streamlines_funcs[] = {
		{
			"create_context", [](lua_State * L)
			{
				parsl_config config = { 0 };

				if (lua_istable(L, 1))
				{
					lua_getfield(L, 1, "flags");// params, flags

					config.flags = GetFlags(L, -1);

					lua_getfield(L, 1, "u_mode");	// params, flags, u_mode

					const char * names[] = { "NORMALIZED_DISTANCE", "DISTANCE", "SEGMENT_INDEX", "SEGMENT_FRACTION", nullptr };
					parsl_u_mode modes[] = { PAR_U_MODE_NORMALIZED_DISTANCE, PAR_U_MODE_DISTANCE, PAR_U_MODE_SEGMENT_INDEX, PAR_U_MODE_SEGMENT_FRACTION };

					config.u_mode = modes[luaL_checkoption(L, -1, "NORMALIZED_DISTANCE", names)];

					lua_getfield(L, 1, "curves_max_flatness");	// params, flags, u_mode, curves_max_flatness

					config.curves_max_flatness = float(luaL_optnumber(L, -1, 0.0));

					lua_getfield(L, 1, "streamlines_seed_spacing");	// params, flags, u_mode, curves_max_flatness, seed_spacing

					config.streamlines_seed_spacing = float(luaL_optnumber(L, -1, 0.0));

					lua_getfield(L, 1, "streamlines_seed_viewport");	// params, flags, u_mode, curves_max_flatness, seed_spacing, seed_viewport

					if (!lua_isnil(L, -1))
					{
						lua_getfield(L, -1, "left");	// params, flags, u_mode, curves_max_flatness, seed_spacing, seed_viewport, left
						lua_getfield(L, -2, "right");	// params, flags, u_mode, curves_max_flatness, seed_spacing, seed_viewport, left, right
						lua_getfield(L, -3, "top");	// params, flags, u_mode, curves_max_flatness, seed_spacing, seed_viewport, left, right, top
						lua_getfield(L, -4, "bottom");	// params, flags, u_mode, curves_max_flatness, seed_spacing, seed_viewport, left, right, top, bottom

						config.streamlines_seed_viewport.left = LuaXS::Float(L, -4);
						config.streamlines_seed_viewport.right = LuaXS::Float(L, -3);
						config.streamlines_seed_viewport.top = LuaXS::Float(L, -2);
						config.streamlines_seed_viewport.bottom = LuaXS::Float(L, -1);
					}

					lua_getfield(L, 1, "miter_limit");	// params, flags, u_mode, curves_max_flatness, seed_spacing, seed_viewport[, left, right, top, bottom], miter_limit

					config.miter_limit = float(luaL_optnumber(L, -1, 0.0));

					lua_getfield(L, 1, "thickness");// params, flags, u_mode, curves_max_flatness, seed_spacing, seed_viewport[, left, right, top, bottom], miter_limit, thickness
					lua_replace(L, 1);	// thickness, flags, u_mode, curves_max_flatness, seed_spacing, seed_viewport[, left, right, top, bottom], miter_limit
				}

				config.thickness = LuaXS::Float(L, 1);

				luaL_argcheck(L, config.thickness, 1, "Must have positive thickness");
				
				auto bm = tls_streamlines->Bookmark();
				parsl_context * context = parsl_create_context(config);

				return NewContext(L, context, bm);// thickness[, flags, u_mode, curves_max_flatness, seed_spacing, seed_viewport[, left, right, top, bottom], miter_limit], context
			}
		}, {
			"NewProxy", NewProxy
		},
		{ nullptr, nullptr }
	};

	luaL_register(L, nullptr, streamlines_funcs);

	return 1;
}
