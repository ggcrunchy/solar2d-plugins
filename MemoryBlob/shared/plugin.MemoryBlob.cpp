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
#include "MemoryBlob.h"
#include "utils/Compat.h"
#include "utils/Blob.h"
#include "utils/Byte.h"
#include "utils/LuaEx.h"
#include <vector>

//
static int sBlobDispatcher;

//
static luaL_Reg blob_funcs[] = {
	{
		"ExistsInStorage", [](lua_State * L)
		{
			return LuaXS::PushArgAndReturn(L, MemoryBlob::Exists(MemoryBlob::GetID(L, 1)));	// id, exists
		}
	}, {
		"GetBlobDispatcher", [](lua_State * L)
		{
			if (!LuaXS::IsMainState(L)) luaL_error(L, "GetBlobDispatcher() called outside main state");

			if (sBlobDispatcher == LUA_NOREF)
			{
				lua_getglobal(L, "system");	// ..., system
				luaL_argcheck(L, lua_istable(L, -1), -1, "`system` is not a table");
				lua_getfield(L, -1, "newEventDispatcher");	// ..., system, system.newEventDispatcher
				luaL_argcheck(L, lua_isfunction(L, -1), -1, "`system.newEventDispatcher` is not a function");
				lua_call(L, 0, 1);	// ..., system, dispatcher
				lua_pushvalue(L, -1);	// ..., system, dispatcher, dispatcher

				sBlobDispatcher = lua_ref(L, 1);// system, dispatcher
			}

			else lua_getref(L, sBlobDispatcher);// ..., dispatcher

			return 1;
		}
	}, {
		"GetQueueReference", [](lua_State * L)
		{
			if (lua_isnoneornil(L, 2)) return MemoryBlob::AsQueue(L);	// qid[, nil], queue

			const char * tokens[] = { "consumer", "producer", nullptr };
			int is_producer = luaL_checkoption(L, 2, nullptr, tokens);

			return is_producer ? MemoryBlob::NewProducerToken(L) : MemoryBlob::NewConsumerToken(L);	// qid, "consumer" / "producer", queue_with_ctoken / queue_with_ptoken
		}
	}, {
		"IsBlob", [](lua_State * L)
		{
			const char * type = nullptr;

			if (lua_isstring(L, 2)) type = lua_tostring(L, 2);

			return LuaXS::PushArgAndReturn(L, MemoryBlob::IsBlob(L, 1, type));	// blob?[, type], is_blob
		}
	}, {
		"New", [](lua_State * L)
		{
			BlobXS::CreateOpts copts;
			size_t size = 0U;

			if (LuaXS::Options{L, 1}.Add("alignment", copts.mAlignment)
									.Add("resizable", copts.mResizable)
									.Add("size", size)
									.Add("type", copts.mType)
			.WasSkipped())
			{
				if (lua_isnumber(L, 1)) size = size_t((std::max)(0, luaL_checkint(L, 1)));

				else copts.mResizable = true;
			}

			MemoryBlob::NewBlob(L, size, &copts);	// [opts / size][, alignment, resizable, size, type, ]blob

			return 1;
		}
	}, {
		"NewQueue", MemoryBlob::NewQueue
	}, {
		"StepStorageFrame", [](lua_State * L)
		{
			MemoryBlob::StepStorageFrame();

			return 0;
		}
	},
	{ nullptr, nullptr }
};

//
static void Instantiate (lua_State * L, size_t size, const char * name)
{
	void * ud = lua_newuserdata(L, size);	// ..., ud

	memset(ud, 0, size);

	ByteXS::AddBytesMetatable(L, name);
}

//
static BlobXS::State::Pimpl * Make (void)
{
	return new BlobStateImpl{};
}

//
static int Preload (lua_State * L)
{
	// Register blob implementation.
	BlobXS::PushImplKey(L);	// key

	auto impl = LuaXS::NewTyped<BlobXS::Pimpls>(L);	// key, pimpls

	impl->mBlobImpl = GetBlobPimpl();
	impl->mInstantiate = &Instantiate;
	impl->mMake = &Make;

	LuaXS::AttachTypedGC<BlobXS::Pimpls>(L, "memory_blob_pimpls");

	lua_rawset(L, LUA_REGISTRYINDEX);	// (empty); registry = { [key] = pimpls }

	// Register functions.
	lua_newtable(L);// MemoryBlob
	luaL_register(L, nullptr, blob_funcs);

	if (!LuaXS::IsMainState(L))
	{
		lua_pushcfunction(L, [](lua_State * L) {
			return luaL_error(L, "PurgeStaleStorage() called outside main state");
		}); // MemoryBlob, PurgeStaleStorage
		lua_setfield(L, -2, "PurgeStaleStorage");	// MemoryBlob = { ..., PurgeStaleStorage = PurgeStaleStorage }
	}

	return 1;
}

static int MemoryBlobDestructors (lua_State * L)
{
#ifndef _WIN32
    MemoryBlob::StorageDestructors();
#endif
	return 0;
}

//
CORONA_EXPORT int luaopen_plugin_MemoryBlob (lua_State * L)
{
	sBlobDispatcher = LUA_NOREF;

	Preload(L);	// ..., MemoryBlob

	static const char SendStaleEventsLua[] =
		"local StaleEvent, ids = {}, {}\n"

		"return function(dispatcher)\n"
		"	for i = #ids, 1, -1 do\n"
		"		StaleEvent.name, StaleEvent.id, ids[i] = 'stale_entry', ids[i]\n"

		"		dispatcher:dispatchEvent(StaleEvent)\n"
		"	end\n"
		"end, ids";

	if (luaL_loadstring(L, SendStaleEventsLua) != 0) lua_error(L);	// ..., MemoryBlob, code

	lua_call(L, 0, 2);	// ..., MemoryBlob, SendStaleEntryEvents, ids
	lua_newuserdata(L, 0);	// ..., MemoryBlob, SendStaleEntryEvents, ids, cleanup

	LuaXS::AttachGC(L, MemoryBlobDestructors);

	lua_pushcclosure(L, [](lua_State * L)
	{
		std::vector<BlobXS::BlobPimpl::storage_id> removed;

		uint64_t past = luaL_optinteger(L, 1, 0ULL);

		MemoryBlob::CleanStorage(removed, past);

		if (sBlobDispatcher != LUA_NOREF && !removed.empty())	// dispatcher lazy-loaded on first GetBlobDispatcher()... obviously,
																// that's the earliest any listener would be added
		{
			lua_pushvalue(L, lua_upvalueindex(2));	// [past, ]ids

			for (size_t i = 0; i < removed.size(); ++i)
			{
				LuaXS::ValueToBytes(L, removed[i]);	// [past, ]ids, id

				lua_rawseti(L, -2, int(i + 1));	// [past, ]ids = { ..., id }
			}

			lua_pushvalue(L, lua_upvalueindex(1));	// [past, ]ids, SendStaleEntryEvents
			lua_getref(L, sBlobDispatcher);	// [past, ]ids, SendStaleEntryEvents, dispatcher
			lua_call(L, 1, 0);	// [past, ]ids
		}

		return 0;
	}, 3);	// ..., MemoryBlob, PurgeStaleStorage

	lua_setfield(L, -2, "PurgeStaleStorage");	// ..., MemoryBlob = { ..., PurgeStaleStorage = PurgeStaleStorage }
	lua_pushcfunction(L, Preload);	// ..., MemoryBlob, Preload
	lua_setfield(L, -2, "Reloader");// ..., MemoryBlob = { ..., PurgeStaleStorage, Reloader = Preload }

	return 1;
}
