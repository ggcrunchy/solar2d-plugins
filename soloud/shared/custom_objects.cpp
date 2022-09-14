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

//
//
//

static int sDecodeConstantsRef;

void GetDecodeConstants (lua_State * L)
{
	lua_getref(L, sDecodeConstantsRef); // ..., constants
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

static lua_State * CreateState ()
{
	lua_State * L = luaL_newstate();

	luaL_openlibs(L);
	lua_atpanic(L, [](lua_State * L) {
		if (lua_isstring(L, -1)) CORONA_LOG_WARNING("Unhandled error in SoLoud state: %s", lua_tostring(L, -1));

		return 0;
	});

	lua_newtable(L); // soloud

	AddBasics(L);

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

	if (!*box) *box = CreateState();

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