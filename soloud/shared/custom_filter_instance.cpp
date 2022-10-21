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

CustomFilterInstance::CustomFilterInstance (CustomFilter * parent) : mParent{parent}
{
}

//
//
//

CustomFilterInstance::~CustomFilterInstance ()
{
	RemoveFromStore(mL, this);
}

//
//
//

void CustomFilterInstance::Init (lua_State * L, const ParentData * data)
{
	mL = L;

	CustomFilterInstance ** box = LuaXS::NewTyped<CustomFilterInstance *>(L); // ..., box

	LuaXS::AttachMethods(L, MT_NAME(CustomFilterInstance), [](lua_State * L) {
		luaL_Reg funcs[] = {
			{
				"getParam", [](lua_State * L)
				{
					CustomFilterInstance * instance = *LuaXS::CheckUD<CustomFilterInstance *>(L, 1, MT_NAME(CustomFilterInstance));
					int index = luaL_checkint(L, 2) - 1;

					if (index >= 0 && index < int(instance->mNumParams)) lua_pushnumber(L, instance->mParam[index]); // instance, index, param_value

					return 1;
				}
			}, {
				"hasParamChanged", [](lua_State * L)
				{
					CustomFilterInstance * instance = *LuaXS::CheckUD<CustomFilterInstance *>(L, 1, MT_NAME(CustomFilterInstance));
					int index = luaL_checkint(L, 2) - 1;

					lua_pushboolean(L, index >= 0 && index < int(instance->mNumParams) && !!(instance->mParamChanged & (1 << index))); // instance, index, changed

					return 1;
				}
			}, {
				"__newindex", [](lua_State * L)
				{
					if (lua_isstring(L, 2))
					{
						CustomFilterInstance * instance = *LuaXS::CheckUD<CustomFilterInstance *>(L, 1, MT_NAME(CustomFilterInstance));
						const char * key = lua_tostring(L, 2);
						bool found = true;

						// TODO: generalize?
						if (strcmp(key, "NumParams") == 0) CORONA_LOG_WARNING("Unable to assign params count");
						else found = false;

						if (found) return 0;
					}

					return SetEnvData(L);
				}
			},
			{ nullptr, nullptr }
		};

		luaL_register(L, nullptr, funcs);

		LuaXS::AttachProperties(L, [](lua_State * L) {
			if (lua_isstring(L, 2))
			{
				CustomFilterInstance * instance = *LuaXS::CheckUD<CustomFilterInstance *>(L, 1, MT_NAME(CustomFilterInstance));
				const char * key = lua_tostring(L, 2);
				bool found = true;

				// TODO: generalize?
				if (strcmp(key, "NumParams") == 0) lua_pushinteger(L, instance->mNumParams); // instance, k, num_params
				else found = false;

				if (found) return 1;
			}

			return GetEnvData(L);
		});
	});

	lua_createtable(L, 0, 3); // ..., box, env (hash part: interface, samples, data)

	DecodeObject(L, mParent->mInterface, mParent->mInterfaceLen); // ..., box, env, interface

	lua_setfield(L, -2, "interface"); // ..., box, env = { interface = interface[, parent_proxy] }

	if (mParent->mNewInstance)
	{
		DecodeObject(L, mParent->mNewInstance, mParent->mNewInstanceLen); // ..., box, env, newInstance

		if (data)
		{
			ParentDataWrapper * pdata = LuaXS::NewTyped<ParentDataWrapper>(L, &mParent->mData, *data); // ..., box, env, newInstance, parent_data

			pdata->Init(L);
		}

		lua_call(L, data ? 1 : 0, 1); // ..., box, env, data
		lua_setfield(L, -2, "data"); // ..., box, env = { interface[, parent_proxy], data = data }
	}

	FloatBuffer * samples = NewFloatBuffer(L); // box, env, samples

	samples->mCanWrite = true;
	samples->mOwnsData = false;

	lua_setfield(L, -2, "samples"); // box, env = { interface[, parent_proxy][, data], samples = samples }
	lua_setfenv(L, -2); // box; box.env = env

	initParams(mParent->getParamCount());

	*box = this;

	AddToStore(L, this);
}

//
//
//

bool CustomFilterInstance::PreCall (const char * name, const char * other, int & has_data)
{
	GetFromStore(mL, this); // ..., instance

	lua_getfenv(mL, -1); // ..., instance, env
	lua_getfield(mL, -1, "interface"); // ..., instance, env, interface
	lua_getfield(mL, -1, name); // ..., instance, env, interface, func

	bool exists = !lua_isnil(mL, -1);

	if (exists)
	{
		lua_pushvalue(mL, -4); // ..., instance, env, interface, func, instance
		lua_getfield(mL, -4, "data"); // ..., instance, env, interface, func, instance, data?

		has_data = !lua_isnil(mL, -1);

		if (!has_data) lua_pop(mL, 1); // ..., instance, env, interface, func, instance[, data]
		if (other) lua_getfield(mL, -(4 + has_data), other); // ..., instance, env, interface, func, instance[, data][, other]
	}

	return exists;
}

//
//
//

template<typename F>
bool Do (CustomFilterInstance * instance, const char * name, F && func, const char * other = nullptr)
{
	SoLoud::Soloud * soloud = instance->mSoloud;
	bool exists = false;

	soloud->unlockAudioMutex_internal();

	{
		LOCK_SECONDARY_STATE();

		int top = lua_gettop(instance->mL), has_data;

		exists = !instance->mHasError && instance->PreCall(name, other, has_data);

		if (exists)
		{
			bool ok = func(instance->mL, instance->mParent, has_data);

			if (!ok)
			{
				CORONA_LOG_ERROR("error in custom filter instance's logic (%s): %s, %i", name, lua_tostring(instance->mL, -1), lua_gettop(instance->mL)); // ..., result / err

				instance->mHasError = true;
			}
		}

		lua_settop(instance->mL, top); // ...
	}

	soloud->lockAudioMutex_internal();

	return exists;
}

//
//
//

void CustomFilterInstance::filter (float * buffer, unsigned int samples, unsigned int size, unsigned int channels, float sample_rate, SoLoud::time time)
{
	mParamChanged = 0;

	updateParams(time);

	bool did_filter = Do(this, "filter", [this, buffer, samples, size, channels, sample_rate, time](lua_State * L, const CustomFilter *, int has_data) {
		FloatBuffer * smp = GetFloatBuffer(L, -1);

		smp->mData = buffer;
		smp->mSize = channels * size;

		lua_insert(L, -(1 + has_data)); // ..., filter, instance, buffer[, data]
		lua_pushinteger(L, samples); // ..., filter, instance, buffer[, data], samples
		lua_pushinteger(L, size); // ..., filter, instance, buffer[, data], samples
		lua_pushinteger(L, channels); // ..., filter, instance, buffer[, data], samples, size, channels
		lua_pushnumber(L, sample_rate); // ..., filter, instance, buffer[, data], samples, size, channels, sample_rate
		lua_pushnumber(L, time); // ..., filter, instance, buffer[, data], samples, size, channels, sample_rate, time

		if (has_data)
		{
			lua_pushvalue(L, -6); // ..., filter, instance, buffer, data, samples, size, channels, sample_rate, time, data
			lua_remove(L, -7); // ..., filter, instance, buffer, samples, size, channels, sample_rate, time, data
		}

		return lua_pcall(L, 7 + has_data, 0, 0) == 0; // ...[, err]
	}, "samples");

	if (!did_filter)
	{
		for (unsigned int i = 0; i < channels; i++)
		{
			float * data = buffer + i * size;

			bool did = Do(this, "filterChannel", [this, data, samples, i, channels, sample_rate, time](lua_State * L, const CustomFilter *, int has_data) {
				FloatBuffer * smp = GetFloatBuffer(L, -1);

				smp->mData = data;
				smp->mSize = samples;

				lua_insert(L, -(1 + has_data)); // ..., filter, instance, buffer[, data]
				lua_pushinteger(L, samples); // ..., filter, instance, buffer[, data], samples
				lua_pushnumber(L, sample_rate); // ..., filter, instance, buffer[, data], samples, sample_rate
				lua_pushnumber(L, time); // ..., filter, instance, buffer[, data], samples, sample_rate, time
				lua_pushinteger(L, i); // ..., filter, instance, buffer[, data], samples, sample_rate, time, channel
				lua_pushinteger(L, channels); // ..., filter, instance, buffer[, data], samples, sample_rate, time, channel, channels

				if (has_data)
				{
					lua_pushvalue(L, -6);  // ..., filter, instance, buffer, data, samples, sample_rate, time, channel, channels, data
					lua_remove(L, -7); // ..., filter, instance, buffer, samples, sample_rate, time, channel, channels, data
				}

				return lua_pcall(L, 7 + has_data, 0, 0) == 0; // ...[, err]
			}, "samples");

			if (0 == i && !did) break;
		}
	}
}