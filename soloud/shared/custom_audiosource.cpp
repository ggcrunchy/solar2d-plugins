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
#include <utility>

//
//
//

SoLoud::AudioSourceInstance * CustomSource::createInstance()
{
	CustomSourceInstance * instance = new CustomSourceInstance(this);

	LOCK_SECONDARY_STATE();

	lua_State * L = GetSoLoudState(mL);
	int top = lua_gettop(L);

	instance->Init(L, mWantParentData ? &mData : nullptr);

	lua_settop(L, top);

	return instance;
}

//
//
//

int CustomSource::Init (lua_State * L)
{
	lua_settop(L, 2); // source, params

	CustomSource * source = LuaXS::UD<CustomSource>(L, 1);

	source->mL = L;

	ReplaceParamsTableWithCopy(L, 2); // source, params2

	// Move any methods into an interface table...
	lua_getfenv(L, 1); // source, params2, env
	lua_createtable(L, 0, 5); // source, params2, env, interface
	lua_getfield(L, 2, "getAudio"); // source, params2, env, interface, getAudio

	AssignMember(L, "getAudio", LUA_TFUNCTION); // source, params2, env, interface
	GetOptionalMember(L, "hasEnded", LUA_TFUNCTION);
	GetOptionalMember(L, "seek", LUA_TFUNCTION);
	GetOptionalMember(L, "rewind", LUA_TFUNCTION);
	GetOptionalMember(L, "getInfo", LUA_TFUNCTION);

	lua_getfield(L, -1, "seek"); // source, params2, env, interface, seek?

	source->mHasSeek = !lua_isnil(L, -1);

	lua_pop(L, 1); // source, params2, env, interface

	// ...then encode it so it may be instantiated in another state. Any capture of the SoLoud
	// plugin module will be translated to its more minimal counterpart.
	source->mInterface = EncodeObject(L, source->mInterfaceLen); // source, params2, env, encoded_interface

	lua_setfield(L, -2, "interface"); // source, params2, env = { ..., interface = encoded_interface }

	// Save any instance constructor, encoding it like the interface. If the instance needs the
	// parent data, make note of that as well.
	GetOptionalMember(L, "newInstance", LUA_TFUNCTION, true); // source, params2, env, newInstance?

	if (!lua_isnil(L, -1)) source->mNewInstance = EncodeObject(L, source->mNewInstanceLen); // source, params2, env, encoded_newInstance

	lua_setfield(L, -2, "newInstance"); // source, params2, env = { ..., newInstance = encoded_newInstance / nil }
	lua_getfield(L, 2, "wantParentData"); // source, params2, env, wantParentData

	source->mWantParentData = lua_toboolean(L, -1);

	lua_pop(L, 1); // source, params2, env

	// Add any class constants, used by property lookup.
	GetOptionalMember(L, "class", LUA_TTABLE);

	// Call an initialization method if available, with the source and remaining params as input.
	GetOptionalMember(L, "init", LUA_TFUNCTION, true); // source, params2, env, init?

	if (!lua_isnil(L, -1))
	{
		lua_insert(L, 1); // init, source, params2, env
		lua_insert(L, 1); // env, init, source, params2
		lua_call(L, 2, 0); // env
	}

	return 0;
}