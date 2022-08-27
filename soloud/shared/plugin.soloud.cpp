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

//
//
//

unsigned int GetAttenuationModel (lua_State * L, int arg)
{
	const char * models[] = { "NO_ATTENUATION", "INVERSE_DISTANCE", "LINEAR_DISTANCE", "EXPONENTIAL_DISTANCE", nullptr };

	return luaL_checkoption(L, arg, nullptr, models);
}

//
//
//

static const char * sResamplers[] = { "POINT", "LINEAR", "CATMULLROM", nullptr };

unsigned int GetResampler (lua_State * L, int arg)
{
	return luaL_checkoption(L, arg, nullptr, sResamplers);
}

const char * GetResamplerString (lua_State * L, unsigned int resampler)
{
	luaL_argcheck(L, resampler <= sizeof(sResamplers) / sizeof(sResamplers[0]), 1, "Invalid resampler index");
	
	return sResamplers[resampler];
}

//
//
//

int GetWaveform (lua_State * L, int arg, const char * def)
{
	const char * models[] = { "SQUARE", "SAW", "SIN", "TRIANGLE", "BOUNCE", "JAWS", "HUMPS", "FSQUARE", "FSAW", nullptr };

	return luaL_checkoption(L, arg, def, models);
}

//
//
//

int HasMetatable (lua_State * L, int arg, const char * name)
{
	luaL_checktype(L, arg, LUA_TUSERDATA);

	int top = lua_gettop(L), has_table = 0;

	if (luaL_getmetafield(L, arg, "__metatable")) // object, ...[, name1]
	{
		lua_pushstring(L, name); // object, ..., name1, name2

		has_table = lua_equal(L, -2, -1);
	}
		
	lua_settop(L, top); // object, ...

	return has_table;
}

//
//
//

float OptFloat (lua_State * L, int arg, float def)
{
	return (float)luaL_optnumber(L, arg, def);
}

//
//
//

#define ERROR_CASE(WHAT) case SoLoud::WHAT: lua_pushliteral(L, #WHAT); break

int Result (lua_State * L, SoLoud::result err)
{
	lua_pushboolean(L, err == SoLoud::SO_NO_ERROR); // ..., ok

	if (err == SoLoud::SO_NO_ERROR) return 1;

	else
	{
		static SoLoud::Soloud dummy; // hack: not a static method

		lua_pushstring(L, dummy.getErrorString(err)); // ..., false, err

		return 2;
	}
}

CORONA_EXPORT int luaopen_plugin_soloud (lua_State * L)
{
	lua_newtable(L);// soloud

	add_audiosources(L);
	add_core(L);
	add_filters(L);

	return 1;
}



