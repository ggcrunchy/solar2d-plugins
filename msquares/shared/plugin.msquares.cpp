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

//
ThreadXS::TLS<MemoryXS::ScopedListSystem *> tls_msquares;

//
void FailAssert (const char * what) { tls_msquares->FailAssert(what); }
void * MallocMS (size_t size) { return tls_msquares->Malloc(size); }
void * CallocMS (size_t num, size_t _) { return tls_msquares->Calloc(num, 1); }
void * ReallocMS (void * ptr, size_t size) { return tls_msquares->Realloc(ptr, size); }
void FreeMS (void * ptr) { tls_msquares->Free(ptr); }

#define PAR_MALLOC(T, N) ((T*) MallocMS(N * sizeof(T)))
#define PAR_CALLOC(T, N) ((T*) CallocMS(N * sizeof(T), 1))
#define PAR_CALLOC_1(T, N) ((T*) calloc(N * sizeof(T), 1))
#define PAR_REALLOC(T, BUF, N) ((T*) ReallocMS(BUF, sizeof(T) * (N)))
#define PAR_FREE(BUF) FreeMS(BUF)

#define PAR_MSQUARES_IMPLEMENTATION

#include "par_msquares.h"

enum { kNumBits = sizeof(int) * 8, kAsOctets = 1 << (kNumBits - 2), kThresholdAsOctets = 1 << (kNumBits - 1) };

//
static int AuxGetFlag (lua_State * L, int sarg)
{
	#define LIST() WITH(INVERT), WITH(DUAL), WITH(HEIGHTS), WITH(SNAP), WITH(CONNECT), WITH(SIMPLIFY), WITH(SWIZZLE), WITH(CLEAN)
	#define WITH(n) #n

	const char * const names[] = { LIST(), "as_octets", "threshold_as_octets", nullptr };

	#undef WITH
	#define WITH(n) PAR_MSQUARES_##n

	const int flags[] = { LIST(), kAsOctets, kThresholdAsOctets };

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

	if (bMulti)
	{
		if (flags & PAR_MSQUARES_CLEAN) flags &= ~PAR_MSQUARES_SIMPLIFY;

		flags &= ~PAR_MSQUARES_DUAL;
		flags &= ~PAR_MSQUARES_INVERT;
		flags &= ~PAR_MSQUARES_SNAP;
	}
	
	if (flags & PAR_MSQUARES_SNAP) flags |= PAR_MSQUARES_DUAL;
	if (flags & (PAR_MSQUARES_CONNECT | PAR_MSQUARES_SNAP)) flags |= PAR_MSQUARES_HEIGHTS;

	return flags;
}

static const par_msquares_mesh * GetMesh (lua_State * L)
{
	return *LuaXS::CheckUD<const par_msquares_mesh *>(L, 1, "msquares.mesh");
}

ThreadXS::TLS<int> sMeshToMeshlistRef = LUA_NOREF;

static par_msquares_meshlist * GetMeshList (lua_State * L, int index = 1)
{
	return *LuaXS::CheckUD<par_msquares_meshlist *>(L, index, "msquares.meshlist");
}

struct Proxy {
	const par_msquares_mesh * mMesh{nullptr};
	bool mTriangles{false};
};

static Proxy * GetProxy (lua_State * L)
{
	return LuaXS::CheckUD<Proxy>(L, 1, "msquares.proxy");
}

static int NewProxy (lua_State * L)
{
	LuaXS::NewTyped<Proxy>(L);	// proxy

	LuaXS::AttachMethods(L, "msquares.proxy", [](lua_State * L)
	{
		luaL_Reg proxy_funcs[] = {
			{
				"GetMode", [](lua_State * L)
				{
					Proxy * proxy = GetProxy(L);

					if (!proxy->mMesh) lua_pushliteral(L, "none");// proxy, "none"
					else if (proxy->mTriangles) lua_pushliteral(L, "triangles");// proxy, "triangles"
					else lua_pushliteral(L, "points");	// proxy, "points"

					return 1;
				}
			}, {
				"Reset", [](lua_State * L)
				{
					Proxy * proxy = GetProxy(L);

					if (proxy->mMesh)
					{
						lua_getref(L, sMeshToMeshlistRef);	// proxy, mesh_to_meshlist
						lua_pushvalue(L, 1);// proxy, mesh_to_meshlist, proxy
						lua_pushnil(L);	// proxy, mesh_to_meshlist, proxy, nil
						lua_rawset(L, -3);	// proxy, mesh_to_meshlist = { ..., [proxy] = nil }

						proxy->mMesh = nullptr;
					}

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

			if (!proxy->mMesh)
			{
				reader.mBytes = nullptr;
				reader.mCount = 0U;
			}

			else if (proxy->mTriangles)
			{
				reader.mBytes = proxy->mMesh->triangles;
				reader.mCount = size_t(3 * proxy->mMesh->ntriangles);
			}

			else
			{
				reader.mBytes = proxy->mMesh->points;
				reader.mCount = size_t(proxy->mMesh->dim * proxy->mMesh->npoints);
			}

			return true;
		};

		lua_pushlightuserdata(L, func);	// proxy, meta, func
		lua_setfield(L, -2, "__bytes");	// ..., proxy, meta = { ..., __bytes = func }
	});

	return 1;
}

static bool BindProxy (lua_State * L, const par_msquares_mesh * mesh, int n, bool bTriangles)
{
	if (LuaXS::IsType(L, "msquares.proxy"))
	{
		Proxy * proxy = LuaXS::UD<Proxy>(L, -1);
		
		proxy->mMesh = mesh;
		proxy->mTriangles = bTriangles;

		lua_getref(L, sMeshToMeshlistRef);	// mesh, ..., proxy, mesh_to_meshlist
		lua_pushvalue(L, -2);	// mesh, ..., proxy, mesh_to_meshlist, proxy
		lua_pushvalue(L, 1);// mesh, ..., proxy, mesh_to_meshlist, proxy, mesh
		lua_rawget(L, -3);	// mesh, ..., proxy, mesh_to_meshlist, proxy, meshlist
		lua_rawset(L, -3);	// mesh, ..., proxy, mesh_to_meshlist = { ..., [proxy] = meshlist }
		lua_pop(L, 1);	// mesh, ..., proxy

		return true;
	}

	if (!lua_istable(L, -1)) lua_createtable(L, n, 0);	// ...[, non_out], out

	return false;
}

static int WrapMesh (lua_State * L, const par_msquares_mesh * mesh)
{
	LuaXS::NewTyped<const par_msquares_mesh *>(L, mesh);// meshlist, ..., mesh

	LuaXS::AttachMethods(L, "msquares.mesh", [](lua_State * L) {
		luaL_Reg mesh_funcs[] = {
			{
				"GetBoundary", [](lua_State * L)
				{
					auto bm = tls_msquares->Bookmark();
					const par_msquares_mesh * mesh = GetMesh(L);

					if (mesh->ntriangles)
					{
						par_msquares_boundary * boundary = par_msquares_extract_boundary(mesh);

						lua_createtable(L, boundary->nchains, 0);	// mlist, boundary

						for (int i = 0; i < boundary->nchains; ++i)
						{
							int n = mesh->dim * int(boundary->lengths[i]);

							lua_createtable(L, n, 0);	// mlist, boundary, chain

							for (int j = 0; j < n; ++j)
							{
								lua_pushnumber(L, boundary->chains[i][j]);	// mlist, boundary, chain, comp
								lua_rawseti(L, -2, j + 1);	// mlist, boundary, chain = { ..., comp }
							}

							lua_rawseti(L, -2, i + 1);	// mlist, boundary = { ..., chain }
						}

						bm.RemoveAll();

						par_msquares_free_boundary(boundary);
					}

					else lua_pushnil(L);// mlist, nil

					return 1;
				}
			}, {
				"GetColor", [](lua_State * L)
				{	
					return LuaXS::PushArgAndReturn(L, GetMesh(L)->color);	// mesh, color
				}
			}, {
				"GetDim", [](lua_State * L)
				{
					return LuaXS::PushArgAndReturn(L, GetMesh(L)->dim);	// mesh, dim
				}
			}, {
				"GetNumPoints", [](lua_State * L)
				{
					return LuaXS::PushArgAndReturn(L, GetMesh(L)->npoints);	// mesh, npoints
				}
			}, {
				"GetNumTriangles", [](lua_State * L)
				{
					return LuaXS::PushArgAndReturn(L, GetMesh(L)->ntriangles);	// mesh, ntris
				}
			}, {
				"GetPoints", [](lua_State * L)
				{
					const par_msquares_mesh * mesh = GetMesh(L);
					int n = mesh->npoints * mesh->dim;

					lua_settop(L, 2);	// mesh, opts

					if (lua_istable(L, 2))
					{
						lua_getfield(L, 2, "out");	// mesh, opts, out?
						lua_replace(L, 2);	// mesh, out?
					}

					if (BindProxy(L, mesh, n, false)) return 1;	// mesh, opts / out, points / proxy

					for (int i = 0; i < n; ++i)
					{
						lua_pushnumber(L, mesh->points[i]);	// mesh, opts / out, points, comp
						lua_rawseti(L, -2, i + 1);	// mesh, opts / out, points = { ..., comp }
					}

					return 1;
				}
			}, {
				"GetTriangles", [](lua_State * L)
				{
					const par_msquares_mesh * mesh = GetMesh(L);
					int n = mesh->ntriangles * 3, extra = 0;

					if (lua_istable(L, 2))
					{
						lua_getfield(L, 2, "one_based");// mesh, opts, one_based
						lua_getfield(L, 2, "out");	// mesh, opts, one_based, out?
						lua_replace(L, 2);	// mesh, out?, one_based

						extra = lua_toboolean(L, -1);
					}

					lua_settop(L, 2);	// mesh, opts / out

					if (BindProxy(L, mesh, n, true)) return 1;	// mesh, opts / out, indices / proxy

					for (int i = 0; i < n; ++i)
					{
						lua_pushinteger(L, mesh->triangles[i] + extra);	// mesh, opts / out, indices, index
						lua_rawseti(L, -2, i + 1);	// mesh, opts / out, indices = { ..., index }
					}

					return 1;
				}
			},
			{ nullptr, nullptr }
		};

		luaL_register(L, nullptr, mesh_funcs);
	});

	lua_getref(L, sMeshToMeshlistRef);	// meshlist, ..., mesh, mesh_to_meshlist
	lua_pushvalue(L, -2);	// meshlist, ..., mesh, mesh_to_list, mesh
	lua_pushvalue(L, 1);// meshlist, ..., mesh, mesh_to_list, mesh, meshlist
	lua_rawset(L, -3);	// meshlist, ..., mesh, mesh_to_list = { ..., [mesh] = meshlist }
	lua_pop(L, 1);	// meshlist, ..., mesh

	return 1;
}

static int NewMeshlist (lua_State * L, par_msquares_meshlist * meshlist, MemoryXS::ScopedList & mem)
{
    LuaXS::NewTyped<par_msquares_meshlist *>(L, meshlist);  // ..., mlist

    mem.RemoveAll(); // n.b. will "leak" on first use, but those tables endure until shutdown

    LuaXS::AttachMethods(L, "msquares.meshlist", [](lua_State * L) {
		luaL_reg meshlist_methods[] = {
            {
                "__gc", [](lua_State * L)
                {
                    par_msquares_free(GetMeshList(L));
                    
                    return 0;
                }
            }, {
				"GetMesh", [](lua_State * L)
				{
					par_msquares_meshlist * mlist = GetMeshList(L);
					int index = luaL_checkint(L, 2) - 1;

					luaL_argcheck(L, index >= 0 && index < par_msquares_get_count(mlist), 2, "Invalid mesh index");

					return WrapMesh(L, par_msquares_get_mesh(mlist, index));
				}
			}, {
				"__len", [](lua_State * L)
				{
					lua_pushinteger(L, par_msquares_get_count(GetMeshList(L)));

					return 1;
				}
			},
			{ nullptr, nullptr }
		};

		luaL_register(L, nullptr, meshlist_methods);
	});

	return 1;
}

static int RawFlags (int flags) { return flags & ~(kAsOctets | kThresholdAsOctets); }

CORONA_EXPORT int luaopen_plugin_msquares (lua_State * L)
{
	tls_msquares = MemoryXS::ScopedListSystem::New(L);

	LuaXS::NewWeakKeyedTable(L);// mesh_to_meshlist

	sMeshToMeshlistRef = lua_ref(L, 1);	// ...

	lua_newtable(L);// ms

	luaL_Reg msquares_funcs[] = {
		{
			"color", [](lua_State * L)
			{
				ByteReader reader{L, 1};

				if (!reader.mBytes) lua_error(L);

				int w = luaL_checkint(L, 2);
				int h = luaL_checkint(L, 3);
				int cellsize = luaL_checkint(L, 4);
				uint32_t color = luaL_checkinteger(L, 5);
				int bpp = luaL_checkint(L, 6);
				int flags = GetFlags(L, 7);

				// N.B. par does checks

				auto data = ByteXS::EnsureN<const uint8_t>(L, reader, size_t(w * h), size_t(bpp));
				auto bm = tls_msquares->Bookmark();
	
				par_msquares_meshlist * mlist = par_msquares_color(data, w, h, cellsize, color, bpp, RawFlags(flags));

				return NewMeshlist(L, mlist, bm);	// data, w, h, cellsize, color, bpp[, flags], list
			}
		}, {
			"color_multi", [](lua_State * L)
			{
				ByteReader reader{L, 1};

				if (!reader.mBytes) lua_error(L);

				int w = luaL_checkint(L, 2);
				int h = luaL_checkint(L, 3);
				int cellsize = luaL_checkint(L, 4);
				int bpp = luaL_checkint(L, 5);
				int flags = GetFlags(L, 6);

				// N.B. par does checks

				auto data = ByteXS::EnsureN<const uint8_t>(L, reader, size_t(w * h), size_t(bpp));
				auto bm = tls_msquares->Bookmark();

				par_msquares_meshlist * mlist = par_msquares_color_multi(data, w, h, cellsize, bpp, RawFlags(flags));

				return NewMeshlist(L, mlist, bm);	// data, w, h, cellsize, bpp[, flags], list
			}
		}, {
			"grayscale", [](lua_State * L)
			{
				ByteReader reader{L, 1};

				if (!reader.mBytes) lua_error(L);

				int w = luaL_checkint(L, 2);
				int h = luaL_checkint(L, 3);
				int cellsize = luaL_checkint(L, 4);
				float threshold = LuaXS::Float(L, 5);
				int flags = GetFlags(L, 6);

				// N.B. par does checks

				const float * data = ByteXS::EnsureFloatsN(L, 1, size_t(w * h), (flags & kAsOctets) != 0);
				auto bm = tls_msquares->Bookmark();

				par_msquares_meshlist * mlist = par_msquares_grayscale(data, w, h, cellsize, threshold, RawFlags(flags));

				return NewMeshlist(L, mlist, bm);	// data, w, h, cellsize, threshold[, flags], list
			}
		}, {
			"grayscale_multi", [](lua_State * L)
			{
				ByteReader reader{L, 1};

				if (!reader.mBytes) lua_error(L);

				int flags = GetFlags(L, 6), nthresholds = ByteXS::GetCount<float>(L, 5);
				const float * thresholds = ByteXS::EnsureFloatsN(L, 5, nthresholds, (flags & kThresholdAsOctets) != 0);
				int w = luaL_checkint(L, 2);
				int h = luaL_checkint(L, 3);
				int cellsize = luaL_checkint(L, 4);

				// N.B. par does checks

				const float * data = ByteXS::EnsureFloatsN(L, 1, size_t(w * h), (flags & kAsOctets) != 0);
				auto bm = tls_msquares->Bookmark();

				par_msquares_meshlist * mlist = par_msquares_grayscale_multi(data, w, h, cellsize, thresholds, int(nthresholds), RawFlags(flags));

				return NewMeshlist(L, mlist, bm);	// data, w, h, cellsize, thresholds[, flags], list
			}
		}, {
			"NewProxy", NewProxy
		},
		{ nullptr, nullptr }
	};

	luaL_register(L, nullptr, msquares_funcs);

	return 1;
}
