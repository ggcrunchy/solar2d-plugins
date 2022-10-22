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

CustomSourceInstance::CustomSourceInstance (CustomSource * parent) : mParent{parent}
{
}

//
//
//

CustomSourceInstance::~CustomSourceInstance ()
{
	RemoveFromStore(mL, this);
}

//
//
//

void CustomSourceInstance::Init (lua_State * L, const ParentData * data)
{
	mL = L;

	CustomSourceInstance ** box = LuaXS::NewTyped<CustomSourceInstance *>(L); // ..., box

	LuaXS::AttachMethods(L, MT_NAME(CustomSourceInstance), [](lua_State * L) {
		luaL_Reg funcs[] = {
			{
				"__newindex", [](lua_State * L)
				{
					if (lua_isstring(L, 2))
					{
						CustomSourceInstance * instance = *LuaXS::CheckUD<CustomSourceInstance *>(L, 1, MT_NAME(CustomSourceInstance));
						const char * key = lua_tostring(L, 2);
						bool found = true;

						// TODO: generalize?
						if (strcmp(key, "Channels") == 0) CORONA_LOG_WARNING("Unable to assign channels");
						else if (strcmp(key, "BaseSamplerate") == 0) instance->mBaseSamplerate = LuaXS::Float(L, 3);
						else if (strcmp(key, "Samplerate") == 0) instance->mSamplerate = LuaXS::Float(L, 3);
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
				CustomSourceInstance * instance = *LuaXS::CheckUD<CustomSourceInstance *>(L, 1, MT_NAME(CustomSourceInstance));
				const char * key = lua_tostring(L, 2);
				bool found = true;

				// TODO: generalize?
				if (strcmp(key, "Channels") == 0) lua_pushinteger(L, instance->mChannels); // instance, k, channels
				else if (strcmp(key, "BaseSamplerate") == 0) lua_pushnumber(L, instance->mBaseSamplerate); // instance, k, base_sample_rate
				else if (strcmp(key, "Samplerate") == 0) lua_pushnumber(L, instance->mSamplerate); // instance, k, sample_rate
				else found = false;

				if (found) return 1;
			}

			return GetEnvData(L);
		});
	});

	lua_createtable(L, 0, mParent->mHasSeek ? 4 : 3); // ..., box, env (hash part: interface, samples, data[, scratch])

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

	if (mParent->mHasSeek)
	{
		FloatBuffer * scratch = NewFloatBuffer(L); // box, env, scratch
		
		scratch->mCanWrite = true;
		scratch->mOwnsData = false;

		lua_setfield(L, -2, "scratch"); // box, env = { interface[, parent_proxy][, data], samples, scratch = scratch }
	}

	lua_setfenv(L, -2); // box; box.env = env

	*box = this;

	AddToStore(L, this);
}

//
//
//

bool CustomSourceInstance::PreCall (const char * name, const char * other, int & has_data)
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
bool Do (CustomSourceInstance * instance, const char * name, F && func, const char * other = nullptr)
{
	SoLoud::Soloud * soloud = instance->mParent->mSoloud;
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
				CORONA_LOG_ERROR("error in custom source instance's logic (%s): %s, %i", name, lua_tostring(instance->mL, -1), lua_gettop(instance->mL)); // ..., result / err

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

static bool DoCall (lua_State * L, int nargs)
{
	return lua_pcall(L, nargs, 1, 0) == 0; // ...[, result / err]
}

//
//
//

unsigned int CustomSourceInstance::getAudio (float * buffer, unsigned int samples, unsigned int size)
{
	unsigned int result = 0;

	Do(this, "getAudio", [&result, this, buffer, samples, size](lua_State * L, const CustomSource * parent, int has_data) {
		FloatBuffer * smp = GetFloatBuffer(L, -1);

		smp->mData = buffer;
		smp->mSize = mChannels * size;

		lua_insert(L, -(1 + has_data)); // ..., get_audio, instance, buffer[, data]
		lua_pushinteger(L, samples); // ..., get_audio, instance, buffer[, data], samples
		lua_pushinteger(L, size); // ..., get_audio, instance, buffer[, data], samples, size

		if (has_data)
		{
			lua_insert(L, -3); // ..., get_audio, instance, buffer, size, data, samples
			lua_insert(L, -3); // ..., get_audio, instance, buffer, samples, size, data
		}

		bool ok = DoCall(L, 4 + has_data); // ..., count / err

		if (ok && lua_isnumber(L, -1)) result = lua_tointeger(L, -1);

		return ok;
	}, "samples");

	return result;
}

//
//
//

bool CustomSourceInstance::hasEnded ()
{
	bool result = false;

	Do(this, "hasEnded", [&result](lua_State * L, const CustomSource *, int has_data) {
		bool ok = DoCall(L, 1 + has_data); // ..., ended / err

		if (ok) result = lua_toboolean(L, -1);

		return ok;
	});

	return result;
}

//
//
//

void GetResultFromStack (SoLoud::result & result, lua_State * L, bool file_ops = false)
{
	if (lua_isstring(L, -1))
	{
		const char * what = lua_tostring(L, -1);

		if (strcmp(what, "INVALID_PARAMETER") == 0) result = SoLoud::INVALID_PARAMETER;
		else if (strcmp(what, "NOT_IMPLEMENTED") == 0) result = SoLoud::NOT_IMPLEMENTED;
		else if (strcmp(what, "OUT_OF_MEMORY") == 0) result = SoLoud::OUT_OF_MEMORY;
		else if (file_ops && strcmp(what, "FILE_NOT_FOUND") == 0) result = SoLoud::FILE_NOT_FOUND;
		else if (file_ops && strcmp(what, "FILE_LOAD_FAILED") == 0) result = SoLoud::FILE_LOAD_FAILED;
	}

	else if (lua_type(L, -1) != LUA_TBOOLEAN || lua_toboolean(L, -1)) result = SoLoud::SO_NO_ERROR;
}

//
//
//

SoLoud::result CustomSourceInstance::seek (float seconds, float * scratch, int scratch_size)
{
	SoLoud::result result = SoLoud::UNKNOWN_ERROR;

	bool found = Do(this, "seek", [&result, seconds, scratch, scratch_size](lua_State * L, const CustomSource *, int has_data) {
		FloatBuffer * buffer = GetFloatBuffer(L, -1);

		buffer->mData = scratch;
		buffer->mSize = size_t(scratch_size);

		lua_insert(L, -(1 + has_data)); // ..., seek, instance, scratch[, data]
		lua_pushnumber(L, seconds); // ..., seek, instance, scratch[, data], seconds
		lua_insert(L, -(2 + has_data)); // ..., seek, instance, seconds, scratch[, data]

		bool ok = DoCall(L, 3 + has_data); // ..., result? / err

		if (ok) GetResultFromStack(result, L);

		return ok;
	});

	if (!found) return AudioSourceInstance::seek(seconds, scratch, scratch_size);

	return result;
}

//
//
//

SoLoud::result CustomSourceInstance::rewind ()
{
	SoLoud::result result = SoLoud::UNKNOWN_ERROR;

	bool found = Do(this, "rewind", [&result](lua_State * L, const CustomSource *, int has_data) {
		bool ok = DoCall(L, 1 + has_data); // ..., result? / err
			
		if (ok) GetResultFromStack(result, L);

		return ok;
	});

	if (!found) return AudioSourceInstance::rewind();

	return result;
}

//
//
//

float CustomSourceInstance::getInfo (unsigned int key)
{
	float result = 0;

	Do(this, "getInfo", [&result, this, key](lua_State * L, const CustomSource *, int has_data) {
		lua_pushinteger(L, key); // get_info, instance[, data], key
		lua_insert(L, -(1 + has_data)); // get_info, instance, key[, data]

		bool ok = DoCall(L, 2 + has_data);

		if (ok && lua_isnumber(L, -1)) result = LuaXS::Float(L, -1);

		return ok;
	});

	return result;
}