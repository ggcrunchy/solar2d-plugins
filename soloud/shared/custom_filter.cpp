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

SoLoud::FilterInstance * CustomFilter::createInstance()
{
	CustomFilterInstance * instance = new CustomFilterInstance(this);

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

int CustomFilter::Init (lua_State * L)
{
	lua_settop(L, 2); // box, params

	CustomFilter * filter = static_cast<CustomFilter *>(GetFilter(L, 1)); // box, params, filter

	lua_pop(L, 1); // box, params

	filter->mL = L;

	ReplaceParamsTableWithCopy(L, 2); // box, params2

	// Move any methods into an interface table...
	lua_getfenv(L, 1); // box, params2, env
	lua_createtable(L, 0, 2); // box, params2, env, interface

	GetOptionalMember(L, "filter", LUA_TFUNCTION);
	GetOptionalMember(L, "filterChannel", LUA_TFUNCTION);

	// ...then encode it so it may be instantiated in another state. Any capture of the SoLoud
	// plugin module will be translated to its more minimal counterpart.
	filter->mInterface = EncodeObject(L, filter->mInterfaceLen); // box, params2, env, encoded_interface

	lua_setfield(L, -2, "interface"); // box, params2, env = { ..., interface = encoded_interface }

	// Save any instance constructor, encoding it like the interface. If the instance needs the
	// parent data, make note of that as well.
	GetOptionalMember(L, "newInstance", LUA_TFUNCTION, true); // box, params2, env, newInstance?

	if (!lua_isnil(L, -1)) filter->mNewInstance = EncodeObject(L, filter->mNewInstanceLen); // box, params2, env, encoded_newInstance

	lua_setfield(L, -2, "newInstance"); // box, params2, env = { ..., newInstance = encoded_newInstance / nil }
	lua_getfield(L, 2, "wantParentData"); // box, params2, env, wantParentData

	filter->mWantParentData = lua_toboolean(L, -1);

	lua_pop(L, 1); // box, params2, env

	// Add any class constants, used by property lookup.
	GetOptionalMember(L, "class", LUA_TTABLE);

	// Call an initialization method if available, with the object and remaining params as input.
	GetOptionalMember(L, "init", LUA_TFUNCTION, true); // box, params2, env, init?

	if (!lua_isnil(L, -1))
	{
		lua_insert(L, 1); // init, box, params2, env
		lua_insert(L, 1); // env, init, box, params2
		lua_call(L, 2, 0); // env
	}

	// Register any custom filter parameters.
	GetOptionalMember(L, "filterParams", LUA_TTABLE, true); // box, params2, env[, nil], filter_params?

	for (size_t i = 1, n = lua_objlen(L, -1); i <= n; ++i, lua_pop(L, 5))
	{
		CustomFilter::Param param;

		lua_rawgeti(L, -1, int(i)); // box, params2, env[, nil], filter_params, param
		luaL_checktype(L, -1, LUA_TTABLE);
		lua_getfield(L, -1, "max"); // box, params2, env[, nil], filter_params, param, max?
		lua_getfield(L, -2, "min"); // box, params2, env[, nil], filter_params, param, max?, min?
		lua_getfield(L, -3, "name"); // box, params2, env[, nil], filter_params, param, max?, min?, name?
		lua_getfield(L, -4, "type"); // box, params2, env[, nil], filter_params, param, max?, min?, name?, type?

		const char * names[] = { "FLOAT", "INT", "BOOL", nullptr };
		Filter::PARAMTYPE types[] = { Filter::FLOAT_PARAM, Filter::INT_PARAM, Filter::BOOL_PARAM };

		param.mType = types[luaL_checkoption(L, -1, "FLOAT", names)];
		param.mName = luaL_optstring(L, -2, "");
		param.mMin = (float)luaL_optnumber(L, -3, 0.0);
		param.mMax = (float)luaL_optnumber(L, -4, 0.0);
		param.mNamed = !lua_isnil(L, -2);

		if (param.mMax < param.mMin) std::swap(param.mMax, param.mMin);

		filter->mParams.push_back(param);
	}

	return 0;
}

//
//
//

int CustomFilter::getParamCount ()
{
	return mParams.size() + 1;
}

//
//
//

const char * CustomFilter::getParamName (unsigned int param_index)
{
	if (0 == param_index) return Filter::getParamName(0);
	else if (param_index <= mParams.size() && mParams[param_index - 1].mNamed) return mParams[param_index - 1].mName.c_str();
	else return nullptr;
}

//
//
//

unsigned int CustomFilter::getParamType (unsigned int param_index)
{
	if (0 == param_index) return Filter::getParamType(0);
	else if (param_index <= mParams.size()) return mParams[param_index - 1].mType;
	else return Filter::FLOAT_PARAM;
}

//
//
//

float CustomFilter::getParamMax (unsigned int param_index)
{
	if (0 == param_index) return Filter::getParamMax(0);
	else if (param_index <= mParams.size()) return mParams[param_index - 1].mMax;
	else return 0.0f;
}

//
//
//

float CustomFilter::getParamMin (unsigned int param_index)
{
	if (0 == param_index) return Filter::getParamMin(0);
	else if (param_index <= mParams.size()) return mParams[param_index - 1].mMin;
	else return 0.0f;
}