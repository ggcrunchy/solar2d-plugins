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
#include "soloud_misc.h"

//
//
//

int PushHandle (lua_State * L, unsigned int handle)
{
	lua_pushnumber(L, handle); // ..., handle

	return 1;
}

//
//
//

unsigned int GetHandle (lua_State * L, int arg)
{
	return static_cast<unsigned int>(lua_tonumber(L, arg));
}

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

//
//
//

void AddToStore (lua_State * L)
{
	lua_getfield(L, LUA_REGISTRYINDEX, MT_NAME(Store)); // ..., object, store
	lua_pushlightuserdata(L, lua_touserdata(L, -2)); // ..., object, store, object_ptr
	lua_pushvalue(L, -3); // ..., object, store, object_ptr, object
	lua_rawset(L, -3); // ..., object, store = { ..., [object_ptr] = object }
	lua_pop(L, 1); // ..., object
}

//
//
//

void GetFromStore (lua_State * L, void * object)
{
	lua_getfield(L, LUA_REGISTRYINDEX, MT_NAME(Store)); // ..., store
	lua_pushlightuserdata(L, object); // .., store, object_ptr
	lua_rawget(L, -2); // ..., store, object_ptr, object
	lua_insert(L, -3); // ..., object, store, object_ptr
	lua_pop(L, 2); // ..., object
}

//
//
//

void RemoveFromStore (lua_State * L, void * object)
{
	if (!object) object = lua_touserdata(L, 1);

	lua_getfield(L, LUA_REGISTRYINDEX, MT_NAME(Store)); // [object, ]..., store
	lua_pushlightuserdata(L, object); // [object, ]..., store, object_ptr
	lua_pushnil(L); // [object, ]..., store, object_ptr, nil
	lua_rawset(L, -3); // [object, ]..., store = { ..., [object_ptr] = nil }
	lua_pop(L, 1); // [object, ]...
}

//
//
//

void RemoveFilterAndBufferRefs (lua_State * L)
{
	lua_getfenv(L, 1); // object, ..., filters

	for (int i = 1; i <= FILTERS_PER_STREAM; ++i)
	{
		lua_pushnil(L); // object, ..., filters, nil
		lua_rawseti(L, -2, i); // object, ..., filters = { ..., [i] = nil }
	}

	lua_getfield(L, -1, MT_NAME(FFTBuffer)); // object, ..., filters, fft_buffer?
	lua_getfield(L, -2, MT_NAME(WaveBuffer)); // object, ..., filters, fft_buffer, wave_buffer?
	lua_pushnil(L); // object, ..., filters, fft_buffer, wave_buffer?, nil
	lua_pushnil(L); // object, ..., filters, fft_buffer, wave_buffer?, nil, nil
	lua_setfield(L, -5, MT_NAME(FFTBuffer)); // object, ..., filters, fft_buffer, wave_buffer?, nil; filters = { ..., [MT_NAME(FFTBuffer)] = nil }
	lua_setfield(L, -4, MT_NAME(WaveBuffer)); // object, ..., filters, fft_buffer, wave_buffer?; filters = { ..., [MT_NAME(WaveBuffer)] = nil }

	if (!lua_isnil(L, -2)) GetFloatBuffer(L, -2)->mSourceGone = true;
	if (!lua_isnil(L, -1)) GetFloatBuffer(L, -1)->mSourceGone = true;

	lua_pop(L, 3); // object, ...
}

//
//
//

int sPluginRef;

void PushPluginModule (lua_State * L)
{
	lua_getref(L, sPluginRef); // ..., soloud
}

//
//
//

void Options::Get (lua_State * L, int opts)
{
	if (lua_istable(L, opts))
	{
		opts = CoronaLuaNormalize(L, opts);

		int top = lua_gettop(L);

		if (mWantPan)
		{
			lua_getfield(L, opts, "pan"); // ..., opts, ..., pan

			mPan = OptFloat(L, -1, 0);
		}

		if (mWantPaused)
		{
			lua_getfield(L, opts, "paused"); // ..., opts, ...[, pan?], paused?

			mPaused = lua_toboolean(L, -1);
		}

		if (mWantVelocity)
		{
			lua_getfield(L, opts, "vel_x"); // ..., opts, ...[[, pan?], paused?], vel_x
			lua_getfield(L, opts, "vel_y"); // ..., opts, ...[[, pan?], paused?], vel_x, vel_y
			lua_getfield(L, opts, "vel_z"); // ..., opts, ...[[, pan?], paused?], vel_x, vel_y, vel_z

			mVelX = OptFloat(L, -3, 0);
			mVelY = OptFloat(L, -2, 0);
			mVelZ = OptFloat(L, -1, 0);
		}

		if (mWantVolume)
		{
			lua_getfield(L, opts, "volume"); // ..., opts, ...[[[, pan?], paused?], vel_x, vel_y, vel_z], volume

			mVolume = OptFloat(L, -1, -1);
		}

		lua_settop(L, top);	// ..., opts, ...
	}

	else
	{
		if (mWantPan) mPan = 0;
		if (mWantPaused) mPaused = false;
		if (mWantVelocity) mVelX = mVelY = mVelZ = 0;
		if (mWantVolume) mVolume = -1;
	}
};

//
//
//

static int ApplicationEvent (lua_State * L)
{
	lua_getfield(L, 1, "type"); // event, type
	lua_pushliteral(L, "applicationSuspend"); // event, type, "applicationSuspend"
	lua_pushliteral(L, "applicationResume"); // event, type, "applicationSuspend", "applicationResume"

	int suspend = lua_equal(L, -3, -2);

	if (suspend || lua_equal(L, -3, -1))
	{
		lua_settop(L, 0); // (empty)
		lua_getfield(L, LUA_REGISTRYINDEX, MT_NAME(Store)); // store

		for (lua_pushnil(L); lua_next(L, -2); lua_pop(L, 1))
		{
			int is_source = luaL_getmetafield(L, -1, "__metatable"); // store, object_ptr, object[, name]

			lua_pop(L, is_source); // store, object_ptr, object

			if (!is_source)
			{
				lua_getfenv(L, -1); // store, core_ptr, core, env

				SoLoud::Soloud * core = GetCore(L, -2);
				const size_t uint_bits = sizeof(unsigned int);

				if (suspend)
				{
					unsigned int * unpaused = LuaXS::NewArray<unsigned int>(L, VOICE_COUNT / uint_bits); // store, core, true, env, unpaused
					
					core->lockAudioMutex_internal();

					unsigned int highest = core->mHighestVoice;

					core->unlockAudioMutex_internal();

					for (unsigned int i = 0; i < highest; ++i)
					{
						core->lockAudioMutex_internal();

						SoLoud::handle handle = core->getHandleFromVoice_internal(i);

						core->unlockAudioMutex_internal();
						
						if (core->isValidVoiceHandle(handle) && !core->getPause(handle))
						{
							int j = i / uint_bits, bit = i % uint_bits;

							unpaused[j] |= 1U << bit;
						}
					}

					core->setPauseAll(true);

					lua_setfield(L, -2, "unpaused"); // store, core_ptr, core, env = { ..., unpaused = unpaused }
					lua_pop(L, 1); // store, core_ptr, core
				}

				else
				{
					lua_getfield(L, -1, "unpaused"); // store, core_ptr, core, env, unpaused

					unsigned int * unpaused = LuaXS::UD<unsigned int>(L, -1);
					
					core->lockAudioMutex_internal();

					unsigned int highest = core->mHighestVoice;

					core->unlockAudioMutex_internal();

					for (unsigned int i = 0; i < highest; ++i)
					{
						core->lockAudioMutex_internal();

						SoLoud::handle handle = core->getHandleFromVoice_internal(i);

						core->unlockAudioMutex_internal();

						int j = i / uint_bits, bit = i % uint_bits;

						if (unpaused[j] & (1U << bit)) core->setPause(handle, false);
					}

					lua_pushnil(L); // store, core_ptr, coree, env, unpaused, nil
					lua_setfield(L, -3, "unpaused"); // store, core_ptr, core, env = { ..., unpaused = nil }, unpaused
					lua_pop(L, 2); // store, core_ptr, core
				}
			}
		}
	}

	return 0;
}

//
//
//

void AddBasics (lua_State * L)
{
	add_floatbuffer(L);

	//
	//
	//

	lua_pushcfunction(L, [](lua_State * L) {
		lua_pushnumber(L, SoLoud::Misc::generateWaveform(GetWaveform(L, 1), LuaXS::Float(L, 2))); // waveform, input, output

		return 1;
	}); // soloud, GenerateWaveform
	lua_setfield(L, -2, "generateWaveform"); // soloud = { ..., generateWaveform = GenerateWaveform }

	//
	//
	//

	lua_pushinteger(L, FILTERS_PER_STREAM); // soloud, FILTERS_PER_STREAM
	lua_setfield(L, -2, "FILTERS_PER_STREAM"); // soloud = { ..., FILTERS_PER_STREAM = FILTERS_PER_STREAM }
	lua_pushinteger(L, SAMPLE_GRANULARITY); // soloud, SAMPLE_GRANULARITY
	lua_setfield(L, -2, "SAMPLE_GRANULARITY"); // soloud = { ..., SAMPLE_GRANULARITY = SAMPLE_GRANULARITY }
	lua_pushinteger(L, FILTERS_PER_STREAM); // soloud, VOICE_COUNT
	lua_setfield(L, -2, "VOICE_COUNT"); // soloud = { ..., VOICE_COUNT = VOICE_COUNT }
	lua_pushinteger(L, FILTERS_PER_STREAM); // soloud, FILTERS_PER_STREAM
	lua_setfield(L, -2, "MAX_CHANNELS"); // soloud = { ..., MAX_CHANNELS = MAX_CHANNELS }
	lua_pushstring(L, GetResamplerString(L, SOLOUD_DEFAULT_RESAMPLER)); // soloud, SOLOUD_DEFAULT_RESAMPLER
	lua_setfield(L, -2, "DEFAULT_RESAMPLER"); // soloud = { ..., DEFAULT_RESAMPLER = SOLOUD_DEFAULT_RESAMPLER }
}

//
//
//

CORONA_EXPORT int luaopen_plugin_soloud (lua_State * L)
{
	lua_newtable(L);// soloud

	//
	//
	//

	add_audiosources(L);
	add_core(L);
	add_filters(L);

	//
	//
	//

	AddBasics(L);

	//
	//
	//

	lua_newtable(L); // soloud, store
	lua_setfield(L, LUA_REGISTRYINDEX, MT_NAME(Store)); // soloud; registry.NAME = store

	//
	//
	//
	
	lua_getglobal(L, "Runtime"); // soloud, Runtime
	lua_getfield(L, -1, "addEventListener"); // soloud, Runtime, Runtime.addEventListener
	lua_insert(L, -2); // soloud, Runtime.addEventListener, Runtime
	lua_pushliteral(L, "system"); // soloud, Runtime.addEventListener, Runtime, "system"
	lua_pushcfunction(L, ApplicationEvent); // soloud, Runtime.addEventListener, Runtime, "system", ApplicationEvent
	lua_call(L, 3, 0); // soloud
	
	//
	//
	//

	lua_pushvalue(L, 1); // soloud, soloud

	sPluginRef = lua_ref(L, 1); // soloud

	//
	//
	//

	return 1;
}



