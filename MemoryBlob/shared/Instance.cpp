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

#include "MemoryBlob.h"
#include "Macros.h"
#include "ByteReader.h"
#include "utils/LuaEx.h"
#include "utils/Memory.h"

template<typename T> void AppendElements (T * vec, ByteReader & reader)
{
	auto pbytes = static_cast<const unsigned char *>(reader.mBytes);

	vec->insert(vec->end(), pbytes, pbytes + reader.mCount);
}

template<typename T> void EraseElements (T * vec, int i1, int i2)
{
	vec->erase(vec->begin() + i1, vec->begin() + i2 + 1);
}

template<typename T> void InsertElements (T * vec, int pos, ByteReader & reader)
{
	auto pbytes = static_cast<const unsigned char *>(reader.mBytes);

	vec->insert(vec->begin() + pos, pbytes, pbytes + reader.mCount);
}

static int Append (lua_State * L, BlobPropViewer & bpv, ByteReader & reader)
{
	if (!bpv.mProps->mResizable) return LuaXS::PushArgAndReturn(L, 0U);	// blob, ..., 0

	WITH_ALIGNMENT_DO(bpv.mProps->mAlign, {
		AppendElements(MemoryBlob::GetVectorN<VA>(L, 1), reader);
	})

	return LuaXS::PushArgAndReturn(L, reader.mCount);// blob, ..., count
}

void MemoryBlob::AuxResize (lua_State * L, size_t align, int arg, size_t size)
{
	WITH_ALIGNMENT_DO(align, {
		GetVectorN<VA>(L, arg)->resize(size);
	})
}

void MemoryBlob::NewBlob (lua_State * L, size_t size, const BlobXS::CreateOpts * opts)
{
	size_t align = 0, extra = 0;
	const char * type = "xs.blob";
	bool bCanResize = false;

	if (opts)
	{
		bCanResize = opts->mResizable;

		if (opts->mType) type = opts->mType;
		if (opts->mAlignment)
		{
			ValidateAlignment(L, -1, opts->mAlignment);

			align = opts->mAlignment;
		}
	}

	void * ud;

	if (bCanResize)
	{
		WITH_ALIGNMENT_DO(align, {
			ud = LuaXS::NewTyped<BlobXS::VectorType<VA>::type>(L);
		}) // ..., ud
	}

    else if (align > std::alignment_of<double>::value)	// Lua gives back double-aligned memory
	{
		size_t cushioned = size + align - 1;

		void * ud = lua_newuserdata(L, cushioned);	// ..., ud

		extra = cushioned;

		MemoryXS::Align(align, size, ud, &cushioned);

		extra -= cushioned;

		luaL_argcheck(L, ud, -1, "Unable to align blob");
	}

	else lua_newuserdata(L, size);	// ..., ud

	LuaXS::AttachMethods(L, type, [](lua_State * L)
	{
		luaL_Reg blob_methods[] = {
			{
				"Append", [](lua_State * L)
				{
					BlobPropViewer bpv{L, 1};
					ByteReader reader{L, 2};

					if (!bpv.mProps || IsLocked(L, 1)) return LuaXS::PushArgAndReturn(L, 0U);// blob, bytes, 0

					return Append(L, bpv, reader);	// blob, bytes, count
				}
			}, {
				"Clone", [](lua_State * L)
				{
					lua_pushboolean(L, 0);	// blob, false

					BlobPropViewer bpv{L, 1};
					BlobXS::CreateOpts copts;

					if (bpv.mProps)
					{
						copts.mAlignment = bpv.mProps->mAlign;
						copts.mResizable = bpv.mProps->mResizable;
					}

					copts.mType = nullptr;

					lua_getmetatable(L, 1);	// blob, false[, props, prop], mt

					for (lua_pushnil(L); lua_next(L, LUA_REGISTRYINDEX); lua_pop(L, 1)) // blob, false[, props, prop], mt[, key, value]
					{
						if (lua_equal(L, -3, -1) && lua_type(L, -2) == LUA_TSTRING)
						{
							copts.mType = lua_tostring(L, -2);

							break;
						}
					}

					size_t size = GetSize(L, 1);

					MemoryBlob::NewBlob(L, size, &copts);	// blob, false[, props, prop], mt[, key, value], new_blob

					memcpy(GetData(L, -1), GetData(L, 1), size);

					lua_replace(L, bpv.mTop);	// blob, new_blob[, props, prop], mt[, key, value]

					return 1;
				}
			}, {
				"__gc", [](lua_State * L)
				{
					BlobPropViewer bpv{L, 1};

					if (bpv.mProps && bpv.mProps->mResizable)
					{
						WITH_ALIGNMENT_DO(bpv.mProps->mAlign, {
							LuaXS::DestructTyped<BlobXS::VectorType<VA>::type>(L);
						})
					}

					return 0;
				}
			}, {
				"GetBytes", [](lua_State * L)
				{
					unsigned char * data = GetData(L, 1);
					size_t count = GetSize(L, 1);
					int i1 = luaL_optinteger(L, 2, 1);
					int i2 = luaL_optinteger(L, 3, count);

					if (i1 < 0) i1 += count + 1;
					if (i2 < 0) i2 += count + 1;

					if (i1 <= 0 || i1 > i2 || size_t(i2) > count) lua_pushliteral(L, "");	// blob, ""

					else lua_pushlstring(L, reinterpret_cast<const char *>(data) + i1 - 1, size_t(i2 - i1 + 1));// blob, data

					return 1;
				}
			}, {
				"GetProperties", [](lua_State * L)
				{
					lua_settop(L, 2);	// blob, out?

					if (!lua_istable(L, 2))
					{
						lua_newtable(L);// blob, ???, out
						lua_replace(L, 2);	// blob, out
					}

					BlobPropViewer bpv{L, 1};
					BlobProps props;

					if (bpv.mProps) props = *bpv.mProps;

					LuaXS::SetField(L, 2, "alignment", props.mAlign);	// blob, out = { alignment = alignment }
					LuaXS::SetField(L, 2, "resizable", props.mResizable);	// blob, out = { alignment, resizable = resizable }

					return 1;
				}
			}, {
				"Insert", [](lua_State * L)
				{
					BlobPropViewer bpv{L, 1};
					ByteReader reader{L, 3};

					int pos = luaL_checkint(L, 2);
					size_t size = GetSize(L, 1);

					if (pos < 0) pos += size;

					else --pos;

					if (!reader.mBytes || reader.mCount == 0) return LuaXS::PushArgAndReturn(L, 0U);// blob, pos, bytes, 0
					if (pos < 0 || size_t(pos) > size) return LuaXS::PushArgAndReturn(L, 0U);	// ditto
					if (!bpv.mProps || IsLocked(L, 1)) return LuaXS::PushArgAndReturn(L, 0U);	// ditto

					if (size_t(pos) < size)
					{
						size_t count = reader.mCount;
						
						if (bpv.mProps->mResizable)
						{
							WITH_ALIGNMENT_DO(bpv.mProps->mAlign, {
								InsertElements(GetVectorN<VA>(L, 1), pos, reader);
							})
						}

						else
						{
							unsigned char * data = GetData(L, 1) + pos;
							size_t available = size - size_t(pos);

							count = (std::min)(count, available);

							if (available > count) memmove(data + count, data, available - count);

							memcpy(data, reader.mBytes, count);
						}

						return LuaXS::PushArgAndReturn(L, count);	// blob, pos, bytes, count
					}

					else return Append(L, bpv, reader);	// blob, pos, bytes, count
				}
			}, {
				"IsLocked", [](lua_State * L)
				{
					return LuaXS::PushArgAndReturn(L, IsLocked(L, 1));	// blob, is_locked
				}
			}, {
				"__len", [](lua_State * L)
				{
					return LuaXS::PushArgAndReturn(L, GetSize(L, 1));	// blob, size
				}
			}, {
				"Remove", [](lua_State * L)
				{
					BlobPropViewer bpv{L, 1};
					size_t count = GetSize(L, 1);
					int i1 = luaL_optinteger(L, 2, 1);
					int i2 = luaL_optinteger(L, 3, count);

					if (i1 < 0) i1 += count;

					else --i1;

					if (i2 < 0) i2 += count;

					else --i2;

					if (i1 < 0 || i1 > i2 || size_t(i2) >= count) return LuaXS::PushArgAndReturn(L, 0U);// blob[, i1[, i2]], ""
					if (!bpv.mProps || IsLocked(L, 1)) return LuaXS::PushArgAndReturn(L, 0U);	// ditto

					if (bpv.mProps->mResizable)
					{
						WITH_ALIGNMENT_DO(bpv.mProps->mAlign, {
							EraseElements(GetVectorN<VA>(L, 1), i1, i2);
						})
					}

					else
					{
						unsigned char * data = GetData(L, 1);

						memmove(data + i1, data + i2 + 1, count - size_t(i2) - 1);
					}

					return LuaXS::PushArgAndReturn(L, size_t(i2 - i1) + 1);	// blob[, i1[, i2]], count
				}
			}, {
				"Submit", [](lua_State * L)
				{
					BlobPropViewer bpv{L, 1, true};

					auto id = Submit(L, 1);

					if (id != BlobXS::BlobPimpl::BadID()) LuaXS::ValueToBytes(L, id);	// blob, id

					else lua_pushnil(L);// blob, nil

					return 1;
				}
			}, {
				"Sync", [](lua_State * L)
				{
					return LuaXS::PushArgAndReturn(L, Sync(L, 1, GetID(L, 2)));// blob, synced
				}
			}, {
				"Write", [](lua_State * L)
				{
					BlobPropViewer bpv{L, 1};
					ByteReader reader{L, 3};

					int pos = luaL_checkint(L, 2);
					size_t size = GetSize(L, 1);

					if (pos < 0) pos += size;

					else --pos;

					if (!reader.mBytes || reader.mCount == 0) return LuaXS::PushArgAndReturn(L, 0U);// blob, pos, bytes, 0
					if (pos < 0 || size_t(pos) > size) return LuaXS::PushArgAndReturn(L, 0U);	// ditto
					if (!bpv.mProps || IsLocked(L, 1)) return LuaXS::PushArgAndReturn(L, 0U);	// ditto
					
					if (size_t(pos) < size)
					{
						size_t count = reader.mCount;
						
						if (!bpv.mProps->mResizable) count = (std::min)(count, size - size_t(pos));

						else if (count + pos > size) AuxResize(L, bpv.mProps->mAlign, 1, count + pos);

						unsigned char * data = GetData(L, 1) + pos; // do here in case resize changes address

						memcpy(data, reader.mBytes, count);

						return LuaXS::PushArgAndReturn(L, count);	// blob, pos, bytes, count
					}

					else return Append(L, bpv, reader);	// blob, pos, bytes, count
				}
			},
			{ nullptr, nullptr }
		};

		luaL_register(L, nullptr, blob_methods);

		ByteReaderFunc * func = ByteReader::Register(L);

		func->mGetBytes = [](lua_State * L, ByteReader & reader, int arg, void *)
		{
			reader.mBytes = GetData(L, arg);
			reader.mCount = GetSize(L, arg);

			return false;
		};
		func->mContext = nullptr;

		LuaXS::SetField(L, -1, "__bytes", func);// ..., mt = { ..., __bytes = reader_func }
		LuaXS::SetField(L, -1, "__metatable", "blob");	// ..., mt = { ..., __bytes, __metatable = "blob" }
	});

	// Create the blob properties table, if necessary.
	lua_pushlightuserdata(L, BlobProps::GetKey());	// ..., ud, props_key
	lua_rawget(L, LUA_REGISTRYINDEX);	// ..., ud, props?

	if (lua_isnil(L, -1))
	{
		lua_pop(L, 1);	// ..., ud

		LuaXS::NewWeakKeyedTable(L);// ..., ud, props

		lua_pushlightuserdata(L, BlobProps::GetKey());	// ..., ud, props, props_key
		lua_pushvalue(L, -2);	// ..., ud, props, props_key, props
		lua_rawset(L, LUA_REGISTRYINDEX);	// ..., ud, props; registry[props_key] = props
	}

	lua_pushvalue(L, -2);	// ..., ud, props, ud

	// Create a property object and store it in that table, hooked up to the blob.
	auto props = LuaXS::NewTyped<BlobProps>(L);	// ..., ud, props, ud, prop

	props->mAlign = align;
	props->mShift = extra;
	props->mResizable = bCanResize;

	lua_rawset(L, -3);	// ..., ud, props = { ..., [ud] = prop }
	lua_pop(L, 1);	// ..., ud

	// If requested, give resizable blobs an initial size (done after properties are bound).
	if (bCanResize && size > 0U) AuxResize(L, align, -1, size);
}

void MemoryBlob::ValidateAlignment (lua_State * L, int arg, size_t align)
{
	luaL_argcheck(L, align >= 4U && (align & (align - 1)) == 0, arg, "Alignment must be a power-of-2 >= 4");
	luaL_argcheck(L, align <= 1024U, arg, "Alignments over 1024 not yet supported");
}