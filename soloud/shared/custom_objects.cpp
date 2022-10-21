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
#include "custom_objects.h"

extern "C" {
	#include "marshal.h"
}

//
//
//

void ReplaceParamsTableWithCopy (lua_State * L, int arg)
{
	arg = CoronaLuaNormalize(L, arg);

	luaL_checktype(L, arg, LUA_TTABLE);
	lua_newtable(L); // ..., params, params2
	lua_pushnil(L); // ..., params, params2, nil

	while (lua_next(L, arg))
	{
		lua_pushvalue(L, -2); // ..., params, params2, k, v, k
		lua_insert(L, -2); // ..., params, params2, k, k, v
		lua_rawset(L, -4); // ..., params, params2 = { ..., [k] = v }, k
	}

	lua_replace(L, arg); // ..., params2
}

//
//
//

static int sDecodeConstantsRef;

//
//
//

void DecodeObject (lua_State * L, const char * encoded, size_t len)
{
	lua_pushcfunction(L, mar_decode); // ..., mar_decode
	lua_pushlstring(L, encoded, len); // ..., mar_decode, encoded
	lua_getref(L, sDecodeConstantsRef); // ..., mar_decode, encoded, constants
	lua_call(L, 2, 1); // ..., decoded
}

//
//
//

const char * EncodeObject (lua_State * L, size_t & len)
{
	lua_pushcfunction(L, mar_encode); // ..., object, mar_encode
	lua_insert(L, -2); // ..., mar_encode, object
	lua_createtable(L, 1, 0); // ..., mar_encode, object, constants

	PushPluginModule(L); // ..., mar_encode, object, constants, soloud

	lua_rawseti(L, -2, 1); // ..., mar_encode, object, constants = { soloud }
	lua_call(L, 2, 1); // ..., encoded

	len = lua_objlen(L, -1);

	return lua_tostring(L, -1);
}

//
//
//

static int sSecondaryStateRef;

void CreateSecondaryState (lua_State * L)
{
	*LuaXS::NewTyped<lua_State *>(L) = nullptr; // ..., box

	// The state is lazily populated on first use, to avoid any unnecessary cost. However, an
	// object is created on module startup, so that the state is GC'd after its objects.
	LuaXS::AttachGC(L, MT_NAME(SoloudState), [](lua_State * L) {
		lua_State ** box = LuaXS::CheckUD<lua_State *>(L, 1, MT_NAME(SoloudState));

		if (*box) lua_close(*box);

		return 0;
	});

	sSecondaryStateRef = lua_ref(L, 1); // ...; ref = state
}

//
//
//

static void ImportLibs (lua_State * to, lua_State * from)
{
	int top = lua_gettop(from);

	lua_getglobal(from, "package"); // ..., package

	if (!lua_istable(from, -1)) CORONA_LOG_WARNING("Unable to find `package`, or not a table");
	else
	{
		lua_getfield(from, -1, "loaded"); // ..., package, package.loaded

		if (!lua_istable(from, -1)) CORONA_LOG_WARNING("Unable to find `package.loaded`, or not a table");
		else
		{
			lua_getfield(from, -1, "plugin_luaproc"); // ..., package, package.loaded, luaproc?

			if (lua_istable(from, -1))
			{
				const char * names[] = { "get_integer", "get_number", "update_integer", "update_number", nullptr };

				lua_createtable(to, 0, sizeof(names) / sizeof(names[0]) - 1); // luaproc2

				for (int i = 0; names[i]; ++i, lua_pop(from, 1))
				{
					lua_getfield(from, -1, names[i]); // ..., package, package.loaded, luaproc, func

					if (!lua_iscfunction(from, -1)) CORONA_LOG_WARNING("luaproc's '%s' not a C function", names[i]);
					else
					{
						lua_pushcfunction(to, lua_tocfunction(from, -1)); // luaproc2, func
						lua_setfield(to, -2, names[i]); // luaproc2 = { ..., [name] = func }
					}
				}

				lua_setglobal(to, "luaproc"); // ...; _G.luaproc = luaproc
				lua_getfield(from, -2, "plugin_MemoryBlob"); // ..., package, package.loaded, luaproc, MemoryBlob?

				if (lua_istable(from, -1))
				{
					lua_getfield(from, -1, "Reloader"); // ..., package, package.loaded, luaproc, MemoryBlob, MemoryBlob.Reloader

					if (!lua_iscfunction(from, -1)) CORONA_LOG_WARNING("MemoryBlob's 'Reloader' not a C function");
					else
					{
						lua_pushcfunction(to, lua_tocfunction(from, -1)); // Reloader
						
						if (lua_pcall(to, 0, 0, 0) != 0)
						{
							CORONA_LOG_WARNING("MemoryBlob 'Reloader' error: %s", lua_isstring(to, -1) ? lua_tostring(to, -1) : "?");

							lua_pop(to, 1); // ...
						}
					}
				}
			}
		}
	}

	lua_settop(from, top); // ...
}

//
//
//

static lua_State * CreateState (lua_State * from)
{
	lua_State * L = luaL_newstate();

	luaL_openlibs(L);
	lua_atpanic(L, [](lua_State * L) {
		if (lua_isstring(L, -1)) CORONA_LOG_WARNING("Unhandled error in SoLoud state: %s", lua_tostring(L, -1));

		return 0;
	});

	lua_newtable(L); // soloud

	AddBasics(L);
	ImportLibs(L, from);

	lua_createtable(L, 1, 0); // soloud, constants
	lua_pushvalue(L, -2); // soloud, constants, soloud
	lua_rawseti(L, -2, 1); // soloud, constants = { soloud }

	sDecodeConstantsRef = lua_ref(L, 1); // soloud; ref = constants

	lua_setglobal(L, "soloud"); // (empty); _G.soloud = soloud

	return L;
}

//
//
//

lua_State * GetSoLoudState (lua_State * L)
{
	lua_getref(L, sSecondaryStateRef); // ..., secondary

	lua_State ** box = LuaXS::CheckUD<lua_State *>(L, -1, MT_NAME(SoloudState));

	lua_pop(L, 1); // ...

	if (!*box) *box = CreateState(L);

	return *box;
}

//
//
//

std::mutex & GetCustomObjectsMutex ()
{
	static std::mutex sMutex;

	return sMutex;
}