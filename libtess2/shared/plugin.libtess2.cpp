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

#include "Include/tesselator.h"
#include "utils/Byte.h"
#include "utils/LuaEx.h"
#include "ByteReader.h"

static bool AsBytes (lua_State * L)
{
	lua_pushliteral(L, "as_bytes");	// tess[, what], "as_bytes"

	bool bAsBytes = lua_gettop(L) > 2 && lua_equal(L, 2, -1) != 0;

	lua_pop(L, 1);	// test[, what]

	return bAsBytes;
}

static TessElementType GetElementType (lua_State * L, int arg)
{
	#define LIST() WITH(POLYGONS), WITH(CONNECTED_POLYGONS), WITH(BOUNDARY_CONTOURS)
	#define WITH(n) #n

	const char * names[] = { LIST(), nullptr };

	#undef WITH
	#define WITH(n) TESS_##n

	TessElementType values[] = { LIST() };

	#undef WITH
	#undef LIST

	return values[luaL_checkoption(L, arg, nullptr, names)];
}

static TessOption GetOption (lua_State * L, int arg)
{
	#define LIST() WITH(CONSTRAINED_DELAUNAY_TRIANGULATION), WITH(REVERSE_CONTOURS)
	#define WITH(n) #n

	const char * names[] = { LIST(), nullptr };

	#undef WITH
	#define WITH(n) TESS_##n

	TessOption values[] = { LIST() };

	#undef WITH
	#undef LIST

	return values[luaL_checkoption(L, arg, nullptr, names)];
}

static TessWindingRule GetWindingRule (lua_State * L, int arg)
{
	#define LIST() WITH(ODD), WITH(NONZERO), WITH(POSITIVE), WITH(NEGATIVE), WITH(ABS_GEQ_TWO)
	#define WITH(n) #n

	const char * names[] = { LIST(), nullptr };

	#undef WITH
	#define WITH(n) TESS_WINDING_##n

	TessWindingRule values[] = { LIST() };

	#undef WITH
	#undef LIST

	return values[luaL_checkoption(L, arg, nullptr, names)];
}

struct TessInfo {
	TESStesselator * mTess;
	bool mWasConnectedType{false}, mWasUsing3{false};
	int mSize{2}, mPolySize{0};
};

static TESStesselator * Get (lua_State * L)
{
	return LuaXS::UD<TessInfo>(L, 1)->mTess;
}

static TessInfo * Info (lua_State * L, int arg = 1)
{
	return LuaXS::UD<TessInfo>(L, arg);
}

static TessInfo * ProxyToInfo (lua_State * L, int arg)
{
	lua_pushvalue(L, arg);	// ..., proxy
	lua_getfenv(L, -1);	// ..., proxy, proxy_to_info
	lua_insert(L, -2);	// ..., proxy_to_info, proxy
	lua_rawget(L, -2);	// ..., proxy_to_info, info

	TessInfo * info =  Info(L, -1);

	lua_pop(L, 2);	// ...

	return info;
}

static int GetBytesProxy (lua_State * L, const char * what)
{
	ByteXS::BytesMetatableOpts opts;

	opts.mMore = [](lua_State * L)
	{
		lua_pushvalue(L, -3);	// ..., func, proxy, mt, func
		lua_setfield(L, -2, "__bytes");	// ..., func, proxy, mt = { ..., __bytes = func }
	};

	lua_pushvalue(L, lua_upvalueindex(1));	// ..., func
	lua_pushvalue(L, lua_upvalueindex(2));	// ..., func, proxy_to_info
	lua_newuserdata(L, 0U);	// ..., func, proxy_to_info, proxy
	lua_pushvalue(L, -2);	// ..., func, proxy_to_info, proxy, proxy_to_info
	lua_setfenv(L, -2);	// func, proxy_to_info, proxy; proxy.env = proxy_to_info

	ByteXS::AddBytesMetatable(L, what, &opts);

	lua_pushvalue(L, -1);	// ..., func, proxy_to_info, proxy, proxy
	lua_pushvalue(L, 1);// ..., func, proxy_to_info, proxy, proxy, info
	lua_rawset(L, -4);	// ..., func, proxy_to_info = { ..., [proxy] = info }, proxy

	return 1;
}

static int GetIndices (lua_State * L, const TESSindex * indices, int n, const char * what)
{
	int inc = lua_toboolean(L, 3);

	lua_settop(L, 2);	// tess, t / "as_bytes" / nil

	if (AsBytes(L)) return GetBytesProxy(L, what);	// tess, "as_bytes", proxy

	else
	{
		if (!lua_istable(L, 2))
		{
			lua_createtable(L, n, 0);	// tess, ?, t
			lua_replace(L, 2);	// tess, t
		}

		for (int i = 0; i < n; ++i)
		{
			lua_pushinteger(L, indices[i] != TESS_UNDEF ? indices[i] + inc : TESS_UNDEF);	// tess, t, index
			lua_rawseti(L, 2, i + 1);	// tess, t = { ..., index }
		}

		return 1;
	}
}

//
static luaL_Reg tessellator_funcs[] = {
	{
		"NewTess", [](lua_State * L)
		{
			TessInfo * tinfo = LuaXS::NewTyped<TessInfo>(L);// tess

			tinfo->mTess = tessNewTess(nullptr);

			if (!tinfo->mTess) return LuaXS::PushArgAndReturn(L, LuaXS::Nil{});	// tess, nil

			LuaXS::AttachMethods(L, "libtess2.tesselator", [](lua_State * L)
			{
				luaL_Reg funcs[] = {
					{
						"AddContour", [](lua_State * L)
						{
							int vsize = Info(L)->mSize, nbytes = sizeof(float) * vsize;
							int stride = nbytes, count = 0;

							LuaXS::Options{L, 3}	.Add("stride", stride)
													.Add("count", count);

							luaL_argcheck(L, stride >= nbytes, 3, "Invalid stride");
							luaL_argcheck(L, count >= 0, 3, "Invalid count");

							size_t n = lua_objlen(L, 2) / (lua_istable(L, 2) ? vsize : stride);

							if (count == 0) count = int(n);

							int nfloats = count * vsize;

							if (lua_istable(L, 2) || vsize == nbytes)
							{
								const float * floats = ByteXS::EnsureFloatsN(L, 2, nfloats, false);

								tessAddContour(Get(L), vsize, floats, nbytes, count);
							}

							else
							{
								ByteReader reader{L, 2};

								if (reader.mBytes) tessAddContour(Get(L), vsize, reader.mBytes, stride, int(reader.mCount) / stride);
							}

							return 0;
						}
					}, {
						"__gc", [](lua_State * L)
						{
							TessInfo * info = Info(L);

							tessDeleteTess(info->mTess);

							return 0;
						}
					}, {
						"GetElementCount", [](lua_State * L)
						{
							return LuaXS::PushArgAndReturn(L, tessGetElementCount(Get(L)));	// tess, nelems
						}
					}, {
						"GetPolySize", [](lua_State * L)
						{
							return LuaXS::PushArgAndReturn(L, Info(L)->mPolySize);
						}
					}, {
						"GetVertexCount", [](lua_State * L)
						{
							return LuaXS::PushArgAndReturn(L, tessGetVertexCount(Get(L)));	// tess, nverts
						}
					}, {
						"GetVertexSize", [](lua_State * L)
						{
							return LuaXS::PushArgAndReturn(L, Info(L)->mSize);
						}
					}, {
						"SetOption", [](lua_State * L)
						{
							tessSetOption(Get(L), GetOption(L, 2), lua_toboolean(L, 3));

							return 0;
						}
					}, {
						"Tesselate", [](lua_State * L)
						{
							TessWindingRule rule = GetWindingRule(L, 2);
							TessElementType type = GetElementType(L, 3);
							TessInfo * tinfo = Info(L);

							if (lua_istable(L, 4))
							{
								lua_settop(L, 4);	// tess, winding_rule, element_type, opts
								lua_getfield(L, 4, "poly_size");// tess, winding_rule, element_type, opts, poly_size?
								lua_getfield(L, 4, "normal");	// tess, winding_rule, element_type, opts, poly_size?, normal?
								lua_remove(L, 4);	// tess, winding_rule, element_type, poly_size?, normal?
							}

							tinfo->mWasConnectedType = type == TESS_CONNECTED_POLYGONS;
							tinfo->mWasUsing3 = tinfo->mSize == 3;
							tinfo->mPolySize = luaL_optint(L, 4, 3);

							const float * normal = !lua_isnoneornil(L, 5) ? ByteXS::EnsureFloatsN(L, 5, 3, false) : nullptr;

							return LuaXS::PushArgAndReturn(L, tessTesselate(Get(L), rule, type, tinfo->mPolySize, tinfo->mSize, normal) != 0);
						}
					}, {
						"UseVertexSizeOf3", [](lua_State * L)
						{
							Info(L)->mSize = 2 + lua_toboolean(L, 2);

							return 0;
						}
					},
					{ nullptr, nullptr }
				};

				luaL_register(L, nullptr, funcs);

				struct {
					const char * mName;
					lua_CFunction mFunc;
					decltype(ByteReaderFunc::mGetBytes) mGetBytes;
				} getter_closures[] = {
					{
						"GetElements", [](lua_State * L)
						{
							TessInfo * tinfo = Info(L);
							int nelems = tessGetElementCount(tinfo->mTess);

							if (tinfo->mWasConnectedType) nelems *= 2;

							return GetIndices(L, tessGetElements(tinfo->mTess), nelems * tinfo->mPolySize, "libtess2.Elements");
						}, [](lua_State * L, ByteReader & reader, int arg, void *) {
							TESStesselator * tess = ProxyToInfo(L, arg)->mTess;

							reader.mBytes = tessGetElements(tess);
							reader.mCount = tessGetElementCount(tess) * sizeof(TESSindex);

							return true;
						}
					}, {
						"GetVertexIndices", [](lua_State * L)
						{
							TessInfo * tinfo = Info(L);

							return GetIndices(L, tessGetVertexIndices(tinfo->mTess), tessGetVertexCount(tinfo->mTess), "libtess2.VertexIndices");
						}, [](lua_State * L, ByteReader & reader, int arg, void *) {
							TESStesselator * tess = ProxyToInfo(L, arg)->mTess;

							reader.mBytes = tessGetVertexIndices(tess);
							reader.mCount = tessGetVertexCount(tess) * sizeof(TESSindex);

							return true;
						}
					}, {
						"GetVertices", [](lua_State * L)
						{
							lua_settop(L, 2);	// tess, t / "as_bytes" / nil

							TessInfo * tinfo = Info(L);
							int n = tessGetVertexCount(tinfo->mTess) * tinfo->mSize;
							const TESSreal * verts = tessGetVertices(tinfo->mTess);

							if (AsBytes(L)) return GetBytesProxy(L, "libtess2.Vertices");	// tess, "as_bytes", proxy

							else
							{
								if (!lua_istable(L, 2))
								{
									lua_createtable(L, n, 0);	// tess, ?, t
									lua_replace(L, 2);	// tess, t
								}

								for (int i = 0; i < n; ++i)
								{
									lua_pushnumber(L, verts[i]);// tess, t, vert
									lua_rawseti(L, 2, i + 1);	// tess, t = { ..., vert }
								}

								return 1;
							}
						}, [](lua_State * L, ByteReader & reader, int arg, void *) {
							TessInfo * tinfo = ProxyToInfo(L, arg);

							reader.mBytes = tessGetVertexIndices(tinfo->mTess);
							reader.mCount = tessGetVertexCount(tinfo->mTess) * sizeof(float) * (tinfo->mWasUsing3 ? 3 : 2);

							return true;
						}
					}
				};

				LuaXS::NewWeakKeyedTable(L);// ..., methods, proxy_to_info

				lua_insert(L, -2);	// ..., proxy_to_info, methods

				for (auto && getter : getter_closures)
				{
					ByteReaderFunc * func = ByteReader::Register(L);

					func->mGetBytes = getter.mGetBytes;

					lua_pushlightuserdata(L, func);	// ..., proxy_to_info, methods, func
					lua_pushvalue(L, -3);	// ..., proxy_to_info, methods, func, proxy_to_info
					lua_pushcclosure(L, getter.mFunc, 2);	// ..., proxy_to_info, methods, closure
					lua_setfield(L, -2, getter.mName);	// ..., proxy_to_info, methods = { ..., name = closure }
				}

				lua_remove(L, -2);	// ..., methods
			});

			return 1;
		}
	}, {
		"Undef", [](lua_State * L)
		{
			return LuaXS::PushArgAndReturn(L, lua_Integer(TESS_UNDEF));	// undef
		}
	},
	{ nullptr, nullptr }
};

//
CORONA_EXPORT int luaopen_plugin_libtess2 (lua_State * L)
{
	lua_newtable(L);// libtess2
	luaL_register(L, nullptr, tessellator_funcs);

	return 1;
}