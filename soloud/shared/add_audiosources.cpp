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
#include "ByteReader.h"
#include "utils/Path.h"
#include "soloud_ay.h"
#include "soloud_bus.h"
#include "soloud_monotone.h"
#include "soloud_noise.h"
#include "soloud_openmpt.h"
#include "soloud_queue.h"
#include "soloud_sfxr.h"
#include "soloud_speech.h"
#include "soloud_tedsid.h"
#include "soloud_vic.h"
#include "soloud_vizsn.h"
#include "soloud_wav.h"
#include "soloud_wavstream.h"
#include "custom_objects.h"
#include "templates.h"
#include <vector>

//
//
//

template<typename T> const char * GetAudioSourceName () { return ""; }
template<typename T> const char * GetRawAudioSourceName () { return ""; }

#define ADD_AUDIOSOURCE_NAME(NAME) \
	template<> const char * GetAudioSourceName<SoLoud::NAME> () { return MT_PREFIX #NAME; } \
	template<> const char * GetRawAudioSourceName<SoLoud::NAME> () { return #NAME; }

ADD_AUDIOSOURCE_NAME(Ay)
ADD_AUDIOSOURCE_NAME(Bus)
ADD_AUDIOSOURCE_NAME(Monotone)
ADD_AUDIOSOURCE_NAME(Noise)
ADD_AUDIOSOURCE_NAME(Openmpt)
ADD_AUDIOSOURCE_NAME(Queue)
ADD_AUDIOSOURCE_NAME(Sfxr)
ADD_AUDIOSOURCE_NAME(Speech)
ADD_AUDIOSOURCE_NAME(TedSid)
ADD_AUDIOSOURCE_NAME(Vic)
ADD_AUDIOSOURCE_NAME(Vizsn)
ADD_AUDIOSOURCE_NAME(Wav)
ADD_AUDIOSOURCE_NAME(WavStream)

template<> const char * GetAudioSourceName<CustomSource> () { return MT_PREFIX "CustomSource"; }
template<> const char * GetRawAudioSourceName<CustomSource> () { return "CustomSource"; }

//
//
//

template<typename T>
T * GetAudioSource (lua_State * L)
{
	return LuaXS::CheckUD<T>(L, 1, GetAudioSourceName<T>());
}

//
//
//

SoLoud::AudioSource * GetAudioSource (lua_State * L, int arg)
{
	luaL_argcheck(L, HasMetatable(L, arg, MT_NAME(AudioSource)), arg, "Non-audio source metatable");

	return LuaXS::UD<SoLoud::AudioSource>(L, arg);
}

//
//
//

template<typename T, SoLoud::result (T::*body)(const char *)> void AddFilenameMethod (lua_State * L, const char * name)
{
	PathXS::Directories::Instantiate(L); // funcs, directories

	lua_pushcclosure(L, [](lua_State * L) {
		PathXS::Directories * dir = LuaXS::UD<PathXS::Directories>(L, lua_upvalueindex(1));
		const char * filename = dir->Canonicalize(L, true, 2);

		lua_getfenv(L, 1); // source, filename, env
		lua_pushnil(L); // source, filename, env, nil
		lua_setfield(L, -2, "memory"); // source, filename, env = { ..., memory = nil }

		return Result(L, (GetAudioSource<T>(L)->*body)(filename));
	}, 1); // funcs, func
	lua_setfield(L, -2, name); // funcs = { ..., name = func }
}

//
//
//

template<typename T> void AddLoadMethods (lua_State * L)
{
	AddFilenameMethod<T, &T::load>(L, "load");

	luaL_Reg funcs[] = {
		{
			"loadFile", [](lua_State * L)
			{
				return luaL_error(L, "NYI: loadFile");
			}
		}, {
			"loadMem", [](lua_State * L)
			{
				ByteReader mem{ L, 2 };

				const unsigned char * bytes = static_cast<const unsigned char *>(mem.mBytes);

				if (mem.mBytes)
				{
					lua_getfenv(L, 1); // source, memory, env
					lua_pushvalue(L, 2); // source, memory, env, memory
					lua_setfield(L, -2, "memory"); // source, memory, env = { ..., memory = memory }
				}

				return Result(L, GetAudioSource<T>(L)->loadMem(const_cast<unsigned char *>(bytes), mem.mCount, false, false));
			}
		},
		{ nullptr, nullptr }
	};

	luaL_register(L, nullptr, funcs);
}

//
//
//

static int sAudioSourceCleanupRef;

//
//
//

template<typename T> void AddCommonMethods (lua_State * L)
{
	luaL_Reg funcs[] = {
		{
			"destroy", [](lua_State * L)
			{
				if (!HasMetatable(L, 1, MT_NAME(AudioSourceDestroyed)))
				{
					GetAudioSource(L); // do checks

					lua_settop(L, 1); // source

					LuaXS::DestructTyped<SoLoud::AudioSource>(L);
					LuaXS::AttachMethods(L, MT_NAME(AudioSourceDestroyed), [](lua_State * L) {
						lua_pushliteral(L, MT_NAME(AudioSourceDestroyed)); // ..., mt, MT_NAME(AudioSourceDestroyed)
						lua_setfield(L, -2, "__metatable"); // ..., mt; mt.__metatable = MT_NAME(AudioSourceDestroyed)
					});

					RemoveEnvironment(L);
					RemoveFromStore(L);
				}

				return 0;
			}
		}, {
			"__gc", [](lua_State * L)
			{
				if (!HasMetatable(L, 1, MT_NAME(AudioSourceDestroyed)))
				{
					// Since these were in the store, the plugin must be being unloaded, thus making it dangerous
					// to call audio source destructors: stopping the source will involve the mutex, potentially
					// terminating the process, e.g. see the remarks in https://docs.microsoft.com/en-us/windows/win32/api/synchapi/nf-synchapi-entercriticalsection.
					// This also means audio source instances are still alive, and many of them rely on the parent
					// source. Thus, any non-destroyed audio sources are put in a list, whose __gc will fire off
					// after those for the core (if any) and secondary Lua state (where the instances reside).
					SoLoud::AudioSource * source = GetAudioSource(L);

					lua_getref(L, sAudioSourceCleanupRef); // source, late_audiosource_cleanup

					LuaXS::UD<std::vector<SoLoud::AudioSource *>>(L, -1)->push_back(source);
				}

				return 0;
			}
		}, {
			"getLoopPoint", [](lua_State * L)
			{
				lua_pushnumber(L, GetAudioSource(L)->getLoopPoint()); // source, loop_point

				return 1;
			}
		}, {
			"setAutoStop", [](lua_State * L)
			{
				GetAudioSource(L)->setAutoStop(lua_toboolean(L, 2));

				return 0;
			}
		}, {
			"setFilter", [](lua_State * L)
			{
				SoLoud::AudioSource * source = GetAudioSource(L);
				unsigned int index = LuaXS::Uint(L, 2) - 1;
				SoLoud::Filter * filter = GetFilter(L, 3); // source, index, box / nil[, filter]

				SetFilterRefToEnvironment(L, 1, int(index) + 1); // source, index[, box, filter]

				source->setFilter(index, filter);

				return 0;
			}
		}, {
			"setInaudibleBehavior", [](lua_State * L)
			{
				GetAudioSource(L)->setInaudibleBehavior(lua_toboolean(L, 2), lua_toboolean(L, 3));

				return 0;
			}
		}, {
			"setLooping", [](lua_State * L)
			{
				bool is_number = lua_isnumber(L, 2);
				int count = is_number ? lua_tointeger(L, 2) : 0;

				GetAudioSource(L)->setLooping(!is_number && lua_toboolean(L, 2), count);

				return 0;
			}
		}, {
			"setLoopPoint", [](lua_State * L)
			{
				GetAudioSource(L)->setLoopPoint(lua_tonumber(L, 2));

				return 1;
			}
		}, {
			"setSingleInstance", [](lua_State * L)
			{
				GetAudioSource(L)->setSingleInstance(lua_toboolean(L, 2));

				return 0;
			}
		}, {
			"setVolume", [](lua_State * L)
			{
				GetAudioSource(L)->setVolume(LuaXS::Float(L, 2));

				return 0;
			}
		}, {
			"set3dMinMaxDistance", [](lua_State * L)
			{
				GetAudioSource(L)->set3dMinMaxDistance(LuaXS::Float(L, 2), LuaXS::Float(L, 3));

				return 0;
			}
		}, {
			"set3dAttenuation", [](lua_State * L)
			{
				GetAudioSource(L)->set3dAttenuation(GetAttenuationModel(L, 2), LuaXS::Float(L, 3));

				return 0;
			}
		}, {
			"set3dDopplerFactor", [](lua_State * L)
			{
				GetAudioSource(L)->set3dDopplerFactor(LuaXS::Float(L, 2));

				return 0;
			}
		}, {
			"set3dListenerRelative", [](lua_State * L)
			{
				GetAudioSource(L)->set3dListenerRelative(lua_toboolean(L, 2));

				return 0;
			}
		}, {
			"set3dDistanceDelay", [](lua_State * L)
			{
				GetAudioSource(L)->set3dDistanceDelay(lua_toboolean(L, 2));

				return 0;
			}
		}, {
			"set3dCollider", [](lua_State * L)
			{
				// void set3dCollider(AudioCollider *aCollider, int aUserData = 0);
				return luaL_error(L, "NYI: set3dCollider");
			}
		}, {
			"set3dAttenuator", [](lua_State * L)
			{
				// void set3dAttenuator(AudioAttenuator *aAttenuator);
				return luaL_error(L, "NYI: set3dAttenuator");
			}
		}, {
			"stop", [](lua_State * L)
			{
				GetAudioSource(L)->stop();

				return 0;
			}
		},
		{ nullptr, nullptr }
	};

	luaL_register(L, nullptr, funcs);
}

template<typename T> void AddAudioSourceType (lua_State * L, lua_CFunction body, lua_CFunction init = nullptr)
{
	lua_pushliteral(L, "create"); // soloud, "create"
	lua_pushstring(L, GetRawAudioSourceName<T>()); // soloud, "create", AudioSourceName
	lua_concat(L, 2); // soloud, "create" .. AudioSourceName
	lua_pushcfunction(L, body); // soloud, "create" .. AudioSourceName, body

	if (init) lua_pushcfunction(L, init); // soloud, "create" .. AudioSourceName, body[, init]

	lua_pushcclosure(L, [](lua_State * L) {
		lua_settop(L, 1); // args?

		T * source = LuaXS::NewTyped<T>(L); // args?, source
		bool is_custom = !lua_isnoneornil(L, lua_upvalueindex(2));

		// hash part: fft table, wave table; cf. CustomSource::HashValues for rest
		lua_createtable(L, FILTERS_PER_STREAM, 2 + (is_custom ? CustomSource::HashValues : 0)); // args?, source, env
		lua_setfenv(L, -2); // args?, source; source.env = env

		LuaXS::AttachMethods(L, GetAudioSourceName<T>(), [](lua_State * L) {
			AddCommonMethods<T>(L);

			lua_pushvalue(L, lua_upvalueindex(1));	// mt, body
			lua_pushvalue(L, -2); // mt, body, mt
			lua_call(L, 1, 0); // mt
			lua_pushliteral(L, MT_NAME(AudioSource)); // mt, MT_NAME(AudioSource)

			lua_setfield(L, -2, "__metatable"); // mt; mt.__metatable = MT_NAME(AudioSource)
		});

		// Custom source? If so, since it now has a __gc, do any initialization before storing.
		if (is_custom)
		{
			lua_pushvalue(L, lua_upvalueindex(2)); // args?, source, init
			lua_pushvalue(L, -2); // args?, source, init, source
			lua_pushvalue(L, 1); // args?, source, init, source, args?
			lua_call(L, 2, 0); // args?, source
		}

		AddToStore(L);

		return 1;
	}, init ? 2 : 1); // soloud, "create" .. AudioSourceName, CreateAudioSource; CreateAudioSource.upvalues = { body[, init] }
	lua_rawset(L, -3); // sloud = { ..., ["create" .. AudioSourceName] = CreateAudioSource }
}

//
//
//

static void AddAy (lua_State * L)
{
	AddAudioSourceType<SoLoud::Ay>(L, [](lua_State * L) {
		AddLoadMethods<SoLoud::Ay>(L);

		return 0;
	});
}

//
//
//

static void AddBus (lua_State * L)
{
	AddAudioSourceType<SoLoud::Bus>(L, [](lua_State * L) {
		BusCore<SoLoud::Bus, &GetAudioSource<SoLoud::Bus>>(L);

		luaL_Reg funcs[] = {
			{
				"annexSound", [](lua_State * L)
				{
					GetAudioSource<SoLoud::Bus>(L)->annexSound(LuaXS::Uint(L, 2));

					return 0;
				}
			}, {
				"getResampler", [](lua_State * L)
				{
					lua_pushstring(L, GetResamplerString(L, GetAudioSource<SoLoud::Bus>(L)->getResampler())); // bus, resampler_name

					return 1;
				}
			}, {
				"setChannels", [](lua_State * L)
				{
					return Result(L, GetAudioSource<SoLoud::Bus>(L)->setChannels(LuaXS::Uint(L, 2)));
				}
			}, {
				"setResampler", [](lua_State * L)
				{
					GetAudioSource<SoLoud::Bus>(L)->setResampler(GetResampler(L, 2));

					return 0;
				}
			},
			{ nullptr, nullptr }
		};

		luaL_register(L, nullptr, funcs);

		return 0;
	});
}

//
//
//

static void AddMonotone (lua_State * L)
{
	AddAudioSourceType<SoLoud::Monotone>(L, [](lua_State * L) {
		AddLoadMethods<SoLoud::Monotone>(L);

		luaL_Reg funcs[] = {
			{
				"setParams", [](lua_State * L)
				{
					luaL_checktype(L, 2, LUA_TTABLE);
					lua_getfield(L, 2, "hardwareChannels"); // monotone, params, hardware_channels
					lua_getfield(L, 2, "waveform"); // flanger_filter, params, hardware_channels, waveform

					return Result(L, GetAudioSource<SoLoud::Monotone>(L)->setParams(LuaXS::Int(L, -2), GetWaveform(L, -1, "SQUARE")));
				}
			},
			{ nullptr, nullptr }
		};

		luaL_register(L, nullptr, funcs);

		return 0;
	});
}

//
//
//

static void AddNoise (lua_State * L)
{
	AddAudioSourceType<SoLoud::Noise>(L, [](lua_State * L) {
		luaL_Reg funcs[] = {
			{
				"setOctaveScale", [](lua_State * L)
				{
					GetAudioSource<SoLoud::Noise>(L)->setOctaveScale(
						LuaXS::Float(L, 2), LuaXS::Float(L, 3), LuaXS::Float(L, 4), LuaXS::Float(L, 5), LuaXS::Float(L, 6),
						LuaXS::Float(L, 7), LuaXS::Float(L, 8), LuaXS::Float(L, 9), LuaXS::Float(L, 10), LuaXS::Float(L, 11)
					);

					return 0;
				}
			}, {
				"setType", [](lua_State * L)
				{
					const char * types[] = { "WHITE", "PINK", "BROWNISH", "BLUEISH", nullptr };

					GetAudioSource<SoLoud::Noise>(L)->setType(luaL_checkoption(L, 2, nullptr, types));

					return 0;
				}
			},
			{ nullptr, nullptr }
		};

		luaL_register(L, nullptr, funcs);

		return 0;
	});
}

//
//
//

static void AddOpenmpt (lua_State * L)
{
	AddAudioSourceType<SoLoud::Openmpt>(L, [](lua_State * L) {
		AddLoadMethods<SoLoud::Openmpt>(L);

		return 0;
	});
}

//
//
//

static void AddQueue (lua_State * L)
{
	AddAudioSourceType<SoLoud::Queue>(L, [](lua_State * L) {
		luaL_Reg funcs[] = {
			{
				"getQueueCount", [](lua_State * L)
				{
					lua_pushinteger(L, GetAudioSource<SoLoud::Queue>(L)->getQueueCount()); //queue, queue_count

					return 1;
				}
			}, {
				"isCurrentlyPlaying", [](lua_State * L)
				{
					lua_pushboolean(L, GetAudioSource<SoLoud::Queue>(L)->isCurrentlyPlaying(*GetAudioSource(L, 2))); // queue, source, isPlaying

					return 1;
				}
			}, {
				"play", [](lua_State * L)
				{
					Options opts;

					opts.mWantCallback = true;

					opts.Get(L, 3); // queue, source[, opts][, on_complete]

					unsigned int id = 0;
					SoLoud::result result = GetAudioSource<SoLoud::Queue>(L)->play(*GetAudioSource(L, 2), opts.mGotCallback ? &id : nullptr);
	
					if (opts.mGotCallback && id > 0) PrepareOnComplete(L, id); // queue, source, opts

					return Result(L, result);
				}
			}, {
				"setParams", [](lua_State * L)
				{
					luaL_checktype(L, 2, LUA_TTABLE);
					lua_getfield(L, 2, "sampleRate"); // queue, params, sample_rate
					lua_getfield(L, 2, "channels"); // queue, params, sample_rate, channels

					return Result(L, GetAudioSource<SoLoud::Queue>(L)->setParams(LuaXS::Float(L, -2), luaL_optinteger(L, -1, 2U)));
				}
			}, {
				"setParamsFromAudioSource", [](lua_State * L)
				{
					return Result(L, GetAudioSource<SoLoud::Queue>(L)->setParamsFromAudioSource(*GetAudioSource(L, 2)));
				}
			},
			{ nullptr, nullptr }
		};

		luaL_register(L, nullptr, funcs);

		return 0;
	});
}

//
//
//

static void AddSfxr (lua_State * L)
{
	AddAudioSourceType<SoLoud::Sfxr>(L, [](lua_State * L) {
		AddFilenameMethod<SoLoud::Sfxr, &SoLoud::Sfxr::loadParams>(L, "loadParams");

		luaL_Reg funcs[] = {
			{
				"loadParamsFile", [](lua_State * L)
				{
					return luaL_error(L, "NYI: loadParamsFile");
				}
			}, {
				"loadParamsMem", [](lua_State * L)
				{
					ByteReader mem{ L, 2 };

					const unsigned char * bytes = static_cast<const unsigned char *>(mem.mBytes);

					return Result(L, GetAudioSource<SoLoud::Sfxr>(L)->loadParamsMem(const_cast<unsigned char *>(bytes), mem.mCount, false, false));
				}
			}, {
				"loadPreset", [](lua_State * L)
				{
					const char * presets[] = { "COIN", "LASER", "EXPLOSION", "POWERUP", "HURT", "JUMP", "BLIP", nullptr };

					return Result(L, GetAudioSource<SoLoud::Sfxr>(L)->loadPreset(luaL_checkoption(L, 2, nullptr, presets), LuaXS::Int(L, 3)));
				}
			}, {
				"resetParams", [](lua_State * L)
				{
					GetAudioSource<SoLoud::Sfxr>(L)->resetParams();

					return 0;
				}
			},
			{ nullptr, nullptr }
		};

		luaL_register(L, nullptr, funcs);

		return 0;
	});
}

//
//
//

static void AddSpeech (lua_State * L)
{
	AddAudioSourceType<SoLoud::Speech>(L, [](lua_State * L) {
		luaL_Reg funcs[] = {
			{
				"setParams", [](lua_State * L)
				{
					luaL_checktype(L, 2, LUA_TTABLE);
					lua_getfield(L, 2, "baseFrequency"); // speech, params, base_frequency
					lua_getfield(L, 2, "baseSpeed"); // speech, params, base_frequency, base_speed
					lua_getfield(L, 2, "baseDeclination"); // speech, params, base_frequency, base_speed, base_declination
					lua_getfield(L, 2, "baseWaveform"); // speech, params, base_frequency, , base_speed, base_declination, base_waveform

					const char * klatt[] = { "SAW", "TRIANGLE", "SIN", "SQUARE", "PULSE", "NOISE", "WARBLE", nullptr };

					return Result(L, GetAudioSource<SoLoud::Speech>(L)->setParams(luaL_optinteger(L, -4, 1330U), OptFloat(L, -3, 10), OptFloat(L, -2, .5f), luaL_checkoption(L, -1, "TRIANGLE", klatt)));
				}
			}, {
				"setText", [](lua_State * L)
				{
					GetAudioSource<SoLoud::Speech>(L)->setText(lua_tostring(L, 2));

					return 0;
				}
			},
			{ nullptr, nullptr }
		};

		luaL_register(L, nullptr, funcs);

		return 0;
	});
}

//
//
//

static void AddTedSid (lua_State * L)
{
	AddAudioSourceType<SoLoud::TedSid>(L, [](lua_State * L) {
		AddLoadMethods<SoLoud::TedSid>(L);

		return 0;
	});
}

//
//
//

#define VIC_CASE(NAME) case SoLoud::Vic::NAME: lua_pushliteral(L, #NAME); break

static int GetVicRegister (lua_State * L)
{
	const char * registers[] = { "BASS", "ALTO", "SOPRANO", "NOISE", nullptr };

	return luaL_checkoption(L, 2, nullptr, registers );
}

static void AddVic (lua_State * L)
{
	AddAudioSourceType<SoLoud::Vic>(L, [](lua_State * L) {
		luaL_Reg funcs[] = {
			{
				"getModel", [](lua_State * L)
				{
					switch (GetAudioSource<SoLoud::Vic>(L)->getModel())
					{
					VIC_CASE(PAL);
					VIC_CASE(NTSC);
					default:
						return luaL_error(L, "Invalid model");
					}

					return 1;
				}
			}, {
				"getRegister", [](lua_State * L)
				{
					lua_pushinteger(L, GetAudioSource<SoLoud::Vic>(L)->getRegister(GetVicRegister(L))); // vic, register, value

					return 1;
				}
			}, {
				"setModel", [](lua_State * L)
				{
					const char * models[] = { "PAL", "NTSC", nullptr };

					GetAudioSource<SoLoud::Vic>(L)->setModel(luaL_checkoption(L, 2, nullptr, models ));

					return 0;
				}
			}, {
				"setRegister", [](lua_State * L)
				{
					GetAudioSource<SoLoud::Vic>(L)->setRegister(GetVicRegister(L), (unsigned char)lua_tointeger(L, 3));

					return 0;
				}
			},
			{ nullptr, nullptr }
		};

		luaL_register(L, nullptr, funcs);

		return 0;
	});
}

//
//
//

static void AddVizsn (lua_State * L)
{
	AddAudioSourceType<SoLoud::Vizsn>(L, [](lua_State * L) {
		luaL_Reg funcs[] = {
			{
				"setText", [](lua_State * L)
				{
					GetAudioSource<SoLoud::Vizsn>(L)->setText(const_cast<char *>(lua_tostring(L, 2)));

					return 0;
				}
			},
			{ nullptr, nullptr }
		};

		luaL_register(L, nullptr, funcs);

		return 0;
	});
}

//
//
//

static void GetRawWaveOptions (lua_State * L, unsigned int & count, float & sample_rate, unsigned int & channels)
{
	const float def_sample_rate = 44100;
	const unsigned int def_channels = 1U;

	if (!lua_istable(L, 3))
	{
		sample_rate = def_sample_rate;
		channels = def_channels;
	}

	else
	{
		lua_getfield(L, 3, "count"); // wav, bytes, count?
		lua_getfield(L, 3, "sampleRate"); // wav, bytes, count?, sample_rate?
		lua_getfield(L, 3, "channels"); // wav, bytes, count?, sample_rate?, channels?

		unsigned current_count = count;

		count = luaL_optinteger(L, -3, current_count);
		sample_rate = OptFloat(L, -2, def_sample_rate);
		channels = luaL_optinteger(L, -1, def_channels);

		if (count > current_count) count = current_count;
	}
}

static void AddWav (lua_State * L)
{
	AddAudioSourceType<SoLoud::Wav>(L, [](lua_State * L) {
		AddLoadMethods<SoLoud::Wav>(L);

		luaL_Reg funcs[] = {
			{
				"getLength", [](lua_State * L)
				{
					lua_pushnumber(L, GetAudioSource<SoLoud::Wav>(L)->getLength()); // wav, length

					return 1;
				}
			}, {
				"loadRawWave", [](lua_State * L)
				{
					ByteReader mem{L, 2};

					const float * bytes = static_cast<const float *>(mem.mBytes);
					unsigned int count = mem.mCount / sizeof(float), channels;
					float sample_rate;

					GetRawWaveOptions(L, count, sample_rate, channels);

					// TODO? does not actually track ownership (so will do a copy)
					return Result(L, GetAudioSource<SoLoud::Wav>(L)->loadRawWave(const_cast<float *>(bytes), count, sample_rate, channels, false, false));
				}
			}, {
				"loadRawWave8", [](lua_State * L)
				{
					ByteReader mem{L, 2};

					const unsigned char * bytes = static_cast<const unsigned char *>(mem.mBytes);
					unsigned int count = mem.mCount, channels;
					float sample_rate;

					GetRawWaveOptions(L, count, sample_rate, channels);

					return Result(L, GetAudioSource<SoLoud::Wav>(L)->loadRawWave8(const_cast<unsigned char *>(bytes), count, sample_rate, channels));
				}
			}, {
				"loadRawWave16", [](lua_State * L)
				{
					ByteReader mem{L, 2};

					const short * bytes = static_cast<const short *>(mem.mBytes);
					unsigned int count = mem.mCount / sizeof(short), channels;
					float sample_rate;

					GetRawWaveOptions(L, count, sample_rate, channels);

					return Result(L, GetAudioSource<SoLoud::Wav>(L)->loadRawWave16(const_cast<short *>(bytes), count, sample_rate, channels));
				}
			},
			{ nullptr, nullptr }
		};

		luaL_register(L, nullptr, funcs);

		LuaXS::AttachProperties(L, [](lua_State * L) {
			const char * key = lua_tostring(L, 2);

			if (!key || strcmp(key, "SampleCount") != 0) return 0;
			
			lua_pushinteger(L, GetAudioSource<SoLoud::Wav>(L)->mSampleCount); // wav, "SampleCount", sample_count

			return 1;
		});

		return 0;
	});
}

//
//
//

static void AddWavStream (lua_State * L)
{
	AddAudioSourceType<SoLoud::WavStream>(L, [](lua_State * L) {
		AddFilenameMethod<SoLoud::WavStream, &SoLoud::WavStream::loadToMem>(L, "loadToMem");
		AddLoadMethods<SoLoud::WavStream>(L);

		luaL_Reg funcs[] = {
			{
				"getLength", [](lua_State * L)
				{
					lua_pushnumber(L, GetAudioSource<SoLoud::WavStream>(L)->getLength()); // wav_stream, length

					return 1;
				}
			}, {
				"loadFileToMem", [](lua_State * L)
				{
					return luaL_error(L, "NYI: loadFileToMem");
				}
			},
			{ nullptr, nullptr }
		};

		luaL_register(L, nullptr, funcs);

		LuaXS::AttachProperties(L, [](lua_State * L) {
			const char * key = lua_tostring(L, 2);

			if (!key || strcmp(key, "SampleCount") != 0) return 0;
			
			lua_pushinteger(L, GetAudioSource<SoLoud::WavStream>(L)->mSampleCount); // wav_stream, "SampleCount", sample_count

			return 1;
		});

		return 0;
	});
}

//
//
//

void AddCustomSource (lua_State * L)
{
	AddAudioSourceType<CustomSource>(L, [](lua_State * L) {
		luaL_Reg funcs[] = {
			{
				"__newindex", [](lua_State * L)
				{
					if (CheckForKey(L, "class")) return 0; // source, k, v[, mt, v?]

					return SetData(L, GetAudioSource<CustomSource>(L)->mData, true);
				}
			},
			{ nullptr, nullptr }
		};

		luaL_register(L, nullptr, funcs);

		LuaXS::AttachProperties(L, [](lua_State * L) {
			if (CheckForKeyInEnv(L, "class")) return 1; // source, k, env, class[, v?]

			lua_settop(L, 2); // source, k

			return GetData(L, GetAudioSource<CustomSource>(L)->mData);
		});

		return 0;
	}, CustomSource::Init);
}

//
//
//

void add_audiosources (lua_State * L)
{
	LuaXS::NewTyped<std::vector<SoLoud::AudioSource *>>(L); // ..., late_audiosource_cleanup
	LuaXS::AttachGC(L, [](lua_State * L) {
		using Vector = std::vector<SoLoud::AudioSource *>;

		Vector & cleanup = *LuaXS::UD<Vector>(L, 1);

		for (size_t i = 0; i < cleanup.size(); ++i)
		{
			cleanup[i]->mSoloud = nullptr; // will already be gone

			cleanup[i]->~AudioSource();
		}

		cleanup.~vector();

		return 0;
	});

	sAudioSourceCleanupRef = lua_ref(L, 1); // ...; ref = late_audiosource_cleanup

	AddAy(L);
	AddBus(L);
	AddMonotone(L);
	AddNoise(L);
	AddOpenmpt(L);
	AddQueue(L);
	AddSfxr(L);
	AddSpeech(L);
	AddTedSid(L);
	AddVic(L);
	AddVizsn(L);
	AddWav(L);
	AddWavStream(L);
	AddCustomSource(L);
}