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
#include "soloud_bassboostfilter.h"
#include "soloud_biquadresonantfilter.h"
#include "soloud_dcremovalfilter.h"
#include "soloud_duckfilter.h"
#include "soloud_echofilter.h"
#include "soloud_eqfilter.h"
#include "soloud_fftfilter.h"
#include "soloud_flangerfilter.h"
#include "soloud_freeverbfilter.h"
#include "soloud_lofifilter.h"
#include "soloud_robotizefilter.h"
#include "soloud_waveshaperfilter.h"

//
//
//

template<typename T> const char * GetFilterName () { return ""; }
template<typename T> const char * GetRawFilterName() { return ""; }

#define ADD_FILTER_NAME(NAME) \
	template<> const char * GetFilterName<SoLoud::NAME##Filter> () { return MT_PREFIX #NAME "Filter"; } \
	template<> const char * GetRawFilterName<SoLoud::NAME##Filter> () { return #NAME "Filter"; }													

ADD_FILTER_NAME(Bassboost)
ADD_FILTER_NAME(BiquadResonant)
ADD_FILTER_NAME(DCRemoval)
ADD_FILTER_NAME(Duck)
ADD_FILTER_NAME(Echo)
ADD_FILTER_NAME(Eq)
ADD_FILTER_NAME(FFT)
ADD_FILTER_NAME(Flanger)
ADD_FILTER_NAME(Freeverb)
ADD_FILTER_NAME(Lofi)
ADD_FILTER_NAME(Robotize)
ADD_FILTER_NAME(WaveShaper)

//
//
//

struct FilterBox {
	SoLoud::Filter * mFilter;
};

template<typename T>
FilterBox * GetFilterBox (lua_State * L)
{
	return LuaXS::CheckUD<FilterBox>(L, 1, GetFilterName<T>());
}

template<typename T>
T * GetFilter (lua_State * L)
{
	FilterBox * box = GetFilterBox<T>(L);

	luaL_argcheck(L, box->mFilter, 1, "Filter already destroyed");

	return static_cast<T *>(box->mFilter);
}

//
//
//

SoLoud::Filter * GetFilter (lua_State * L, int arg)
{	
	luaL_argcheck(L, HasMetatable(L, arg, MT_NAME(Filter)), arg, "Non-filter metatable");

	FilterBox * box = LuaXS::UD<FilterBox>(L, arg);

	luaL_argcheck(L, box->mFilter, 1, "Filter already destroyed");

	return box->mFilter;
}

//
//
//

static void GetFilterRef (lua_State * L, SoLoud::Filter * filter)
{
	if (filter)
	{
		lua_pushlightuserdata(L, filter); // ..., filter_ptr
		lua_rawget(L, LUA_REGISTRYINDEX); // ..., filter?
	}

	else lua_pushnil(L); // ..., nil
}

void SetFilterRefToEnvironment (lua_State * L, int arg, int index, SoLoud::Filter * filter)
{
	if (index >= 1 && index <= FILTERS_PER_STREAM)
	{
		lua_getfenv(L, arg); // ..., env

		GetFilterRef(L, filter); // ..., env, filter?

		lua_rawseti(L, -2, index); // ..., env = { ..., [index] = filter? }
		lua_pop(L, 1); // ...
	}
}

//
//
//

template<typename T> void AddCommonMethods (lua_State * L)
{
	luaL_Reg funcs[] = {
		{
			"destroy", [](lua_State * L)
			{
				FilterBox * box = GetFilterBox<T>(L);

				if (box->mFilter)
				{
					RemoveFromStore(L, box->mFilter);
					RemoveFromStore(L);
				}

				box->mFilter = nullptr;

				return 0;
			}
		}, {
			"__gc", [](lua_State * L)
			{
				FilterBox * box = GetFilterBox<T>(L);

				if (box->mFilter) RemoveFromStore(L, box->mFilter);

				return 0;
			}
		},{
			"getParamCount", [](lua_State * L)
			{
				lua_pushinteger(L, GetFilter<T>(L)->getParamCount()); // filter, count

				return 1;
			}
		}, {
			"getParamMax", [](lua_State * L)
			{
				unsigned int index = lua_tointeger(L, 2) - 1;

				lua_pushnumber(L, GetFilter<T>(L)->getParamMax(index)); // filter, index, max

				return 1;
			}
		}, {
			"getParamMin", [](lua_State * L)
			{
				unsigned int index = lua_tointeger(L, 2) - 1;

				lua_pushnumber(L, GetFilter<T>(L)->getParamMin(index)); // filter, index, min

				return 1;
			}
		}, {
			"getParamName", [](lua_State * L)
			{
				unsigned int index = lua_tointeger(L, 2) - 1;
				const char * name = GetFilter<T>(L)->getParamName(index);

				if (name) lua_pushstring(L, name); // filter, index, name
				else lua_pushnil(L); // filter, index, nil

				return 1;
			}
		}, {
			"getParamType", [](lua_State * L)
			{
				unsigned int index = lua_tointeger(L, 2) - 1;
				
				switch (GetFilter<T>(L)->getParamType(index))
				{
				case SoLoud::Filter::FLOAT_PARAM:
					lua_pushliteral(L, "FLOAT"); // filter, "FLOAT"
					break;
				case SoLoud::Filter::INT_PARAM:
					lua_pushliteral(L, "INT"); // filter, "INT"
					break;
				case SoLoud::Filter::BOOL_PARAM:
					lua_pushliteral(L, "BOOL"); // filter, "BOOL"
					break;
				default:
					return luaL_error(L, "Invalid param type");
				}

				return 1;
			}
		},
		{ nullptr, nullptr }
	};

	luaL_register(L, nullptr, funcs);
}

template<typename T> void AddFilterType (lua_State * L, lua_CFunction extra = nullptr)
{
	lua_pushliteral(L, "create"); // soloud, "create"
	lua_pushstring(L, GetRawFilterName<T>()); // soloud, "create", FilterName
	lua_concat(L, 2); // soloud, "create" .. FilterName
	
	if (extra) lua_pushcfunction(L, extra); // soloud, "create" .. FilterName, extra
	else lua_pushnil(L); // soloud, "create" .. FilterName, nil

	lua_pushcclosure(L, [](lua_State * L) {
		FilterBox * box = LuaXS::NewTyped<FilterBox>(L); // box
		T * filter = LuaXS::NewTyped<T>(L); // box, filter

		LuaXS::AttachTypedGC<SoLoud::Filter>(L, MT_NAME(RawFilter));

		AddToStore(L);

		lua_pop(L, 1); // box

		LuaXS::AttachMethods(L, GetFilterName<T>(), [](lua_State * L) {
			AddCommonMethods<T>(L);

			if (!lua_isnil(L, lua_upvalueindex(1)))
			{
				lua_pushvalue(L, lua_upvalueindex(1));	// mt, extra?
				lua_pushvalue(L, -2); // mt, extra, mt
				lua_call(L, 1, 0); // mt
			}

			lua_pushliteral(L, MT_NAME(Filter)); // mt, MT_NAME(Filter)
			lua_setfield(L, -2, "__metatable"); // mt; mt.__metatable = MT_NAME(Filter)
		});

		box->mFilter = filter;

		AddToStore(L);

		return 1;
	}, 1); // soloud, "create" .. FilterName, CreateFilter; CreateFilter.upvalue1 = extra / nil
	lua_rawset(L, -3); // soloud = { ..., ["create" .. FilterName] = CreateFilter }
}

//
//
//

#define ADD_FILTER_PARAMETER(TYPE, NAME) \
	lua_pushinteger(L, SoLoud::TYPE::NAME); /* filter_params, soloud, v */  \
	lua_setfield(L, -3, #TYPE "." #NAME) /* filter_params = { ..., [name] = v }, soloud */

//
//
//

static void AddBassboostFilter (lua_State * L)
{
	AddFilterType<SoLoud::BassboostFilter>(L, [](lua_State * L) {
		luaL_Reg funcs[] = {
			{
				"setParams", [](lua_State * L)
				{
					luaL_checktype(L, 2, LUA_TTABLE);
					lua_getfield(L, 2, "boost"); // lofi_filter, params, boost

					return Result(L, GetFilter<SoLoud::BassboostFilter>(L)->setParams(LuaXS::Float(L, -1)));
				}
			},
			{ nullptr, nullptr }
		};

		luaL_register(L, nullptr, funcs);

		return 0;
	});

	ADD_FILTER_PARAMETER(BassboostFilter, BOOST);
}

//
//
//

static void AugmentGetParamType (lua_State * L)
{
	lua_getfield(L, -2, "getParamType"); // mt, aug, GetParamType
	lua_pushcclosure(L, [](lua_State * L) {
		lua_pushliteral(L, "INT"); // object, index, "INT"
		lua_pushvalue(L, lua_upvalueindex(2)); // object, index, "INT", GetParamType
		lua_pushvalue(L, 1); // object, index, "INT", GetParamType, object
		lua_pushvalue(L, 2); // object, index, "INT", GetParamType, object, index
		lua_call(L, 2, 1); // object, index, "INT", type

		if (!lua_equal(L, -2, -1)) return 1;

		lua_settop(L, 2); // object, index
		lua_pushvalue(L, lua_upvalueindex(1)); // object, index, aug
		lua_insert(L, 1); // aug, object, index
		lua_call(L, 2, LUA_MULTRET); // ...

		return lua_gettop(L);
	}, 2); // mt, GetParamType2; GetParamType2.upvalues = { aug, GetParamType }
	lua_setfield(L, -2, "getParamType"); // mt = { ..., getParamType = GetParamType2 }
}

//
//
//

static bool FoundEnum (lua_State * L, const char ** list, int arg)
{
	if (!lua_isnumber(L, arg)) return false;

	int index = lua_tointeger(L, arg);

	if (index < 0) return false;

	for (int pos = 0; list[pos]; ++pos)
	{
		if (pos == index)
		{
			lua_pushstring(L, list[index]); // ..., v

			return true;
		}
	}

	return false;
}

//
//
//

static void AddBiquadResonantFilter (lua_State * L)
{
	static const char * sTypes[] = { "LOWPASS", "HIGHPASS", "BANDPASS", nullptr };

	AddFilterType<SoLoud::BiquadResonantFilter>(L, [](lua_State * L) {
		luaL_Reg funcs[] = {
			{
				"setParams", [](lua_State * L)
				{
					luaL_checktype(L, 2, LUA_TTABLE);
					lua_getfield(L, 2, "type"); // biquad_filter, params, type
					lua_getfield(L, 2, "frequency"); // biquad_filter, params, type, frequency
					lua_getfield(L, 2, "resonance"); // biquad_filter, params, type, frequency, resonance

					return Result(L, GetFilter<SoLoud::BiquadResonantFilter>(L)->setParams(luaL_checkoption(L, -3, nullptr, sTypes), LuaXS::Float(L, -2), LuaXS::Float(L, -1)));
				}
			},
			{ nullptr, nullptr }
		};

		luaL_register(L, nullptr, funcs);

		lua_pushcfunction(L, [](lua_State * L) {
			// n.b. index ignored; TYPE is only INT
			lua_pushliteral(L, "INT"); // index, "INT"
			lua_newuserdata(L, 0); // index, "INT", type_proxy

			LuaXS::AttachMethods(L, MT_NAME(BiquadResonantFilterTypeProxy), [](lua_State * L) {
				LuaXS::AttachProperties(L, [](lua_State * L) {
					if (FoundEnum(L, sTypes, 2)) return 1; // type_proxy, index[, v]

					return 0;
				});
			});

			return 2;
		}); // mt, aug
		AugmentGetParamType(L); // mt

		return 0;
	});
	
	ADD_FILTER_PARAMETER(BiquadResonantFilter, TYPE);
	ADD_FILTER_PARAMETER(BiquadResonantFilter, FREQUENCY);
	ADD_FILTER_PARAMETER(BiquadResonantFilter, RESONANCE);
}

//
//
//

static void AddDCRemovaltFilter (lua_State * L)
{
	AddFilterType<SoLoud::DCRemovalFilter>(L, [](lua_State * L) {
		luaL_Reg funcs[] = {
			{
				"setParams", [](lua_State * L)
				{
					if (lua_istable(L, 2)) lua_getfield(L, 2, "length"); // dc_removal_filter, params, length
					else lua_pushnil(L); // dc_removal_filter[, params], nil

					return Result(L, GetFilter<SoLoud::DCRemovalFilter>(L)->setParams(OptFloat(L, -1, .1f)));
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

static void AddDuckFilter (lua_State * L)
{
	AddFilterType<SoLoud::DuckFilter>(L, [](lua_State * L) {
		luaL_Reg funcs[] = {
			{
				"setParams", [](lua_State * L)
				{
					luaL_checktype(L, 2, LUA_TTABLE);
					lua_getfield(L, 2, "core"); // duck_filter, params, core
					lua_getfield(L, 2, "listenTo"); // duck_filter, params, core, listen_to
					lua_getfield(L, 2, "onRamp"); // duck_filter, params, core, listen_to, on_ramp
					lua_getfield(L, 2, "offRamp"); // duck_filter, params, core, listen_to, on_ramp, off_ramp
					lua_getfield(L, 2, "level"); // duck_filter, params, core, listen_to, on_ramp, off_ramp, level

					return Result(L, GetFilter<SoLoud::DuckFilter>(L)->setParams(
						GetCore(L, -5), GetHandle(L, -4),
						OptFloat(L, -3, .1f), OptFloat(L, -2, .5f), OptFloat(L, -1, .1f)
					));
				}
			},
			{ nullptr, nullptr }
		};

		luaL_register(L, nullptr, funcs);

		return 0;
	});

	ADD_FILTER_PARAMETER(DuckFilter, ONRAMP);
	ADD_FILTER_PARAMETER(DuckFilter, OFFRAMP);
	ADD_FILTER_PARAMETER(DuckFilter, LEVEL);
}

//
//
//

static void AddEchoFilter (lua_State * L)
{
	AddFilterType<SoLoud::EchoFilter>(L, [](lua_State * L) {
		luaL_Reg funcs[] = {
			{
				"setParams", [](lua_State * L)
				{
					luaL_checktype(L, 2, LUA_TTABLE);
					lua_getfield(L, 2, "delay"); // echo_filter, params, delay
					lua_getfield(L, 2, "decay"); // echo_filter, params, delay, decay
					lua_getfield(L, 2, "filter"); // echo_filter, params, delay, decay, filter

					return Result(L, GetFilter<SoLoud::EchoFilter>(L)->setParams(
						LuaXS::Float(L, -3),
						OptFloat(L, -2, .7f), OptFloat(L, -1, 0)
					));
				}
			},
			{ nullptr, nullptr }
		};

		luaL_register(L, nullptr, funcs);

		return 0;
	});

	ADD_FILTER_PARAMETER(EchoFilter, DELAY);
	ADD_FILTER_PARAMETER(EchoFilter, DECAY);
	ADD_FILTER_PARAMETER(EchoFilter, FILTER);
}

//
//
//

static void AddEQFilter (lua_State * L)
{
	AddFilterType<SoLoud::EqFilter>(L, [](lua_State * L) {
		luaL_Reg funcs[] = {
			{
				"setParams", [](lua_State * L)
				{
					luaL_checktype(L, 2, LUA_TTABLE);
					lua_getfield(L, 2, "band"); // eq_filter, params, band
					lua_getfield(L, 2, "volume"); // eq_filter, params, band, volume

					unsigned int band = 0;

					if (LUA_TSTRING == lua_type(L, -2))
					{
						const char * str = lua_tostring(L, -2);

						if (lua_objlen(L, -2) == 5 && strncmp(str, "BAND", 4) == 0 && isdigit(str[4])) band = SoLoud::EqFilter::BAND1 + str[4] - '1';
					}

					else band = LuaXS::Uint(L, -2);

					return Result(L, GetFilter<SoLoud::EqFilter>(L)->setParam(band, LuaXS::Float(L, -1)));
				}
			},
			{ nullptr, nullptr }
		};

		luaL_register(L, nullptr, funcs);

		return 0;
	});

	ADD_FILTER_PARAMETER(EqFilter, BAND1);
	ADD_FILTER_PARAMETER(EqFilter, BAND2);
	ADD_FILTER_PARAMETER(EqFilter, BAND3);
	ADD_FILTER_PARAMETER(EqFilter, BAND4);
	ADD_FILTER_PARAMETER(EqFilter, BAND5);
	ADD_FILTER_PARAMETER(EqFilter, BAND6);
	ADD_FILTER_PARAMETER(EqFilter, BAND7);
	ADD_FILTER_PARAMETER(EqFilter, BAND8);
}

//
//
//

static void AddFFTFilter (lua_State * L)
{
	AddFilterType<SoLoud::FFTFilter>(L);
}

//
//
//

static void AddFlangerFilter (lua_State * L)
{
	AddFilterType<SoLoud::FlangerFilter>(L, [](lua_State * L) {
		luaL_Reg funcs[] = {
			{
				"setParams", [](lua_State * L)
				{
					luaL_checktype(L, 2, LUA_TTABLE);
					lua_getfield(L, 2, "delay"); // flanger_filter, params, delay
					lua_getfield(L, 2, "frequency"); // flanger_filter, params, delay, frequency

					return Result(L, GetFilter<SoLoud::FlangerFilter>(L)->setParams(LuaXS::Float(L, -2), LuaXS::Float(L, -1)));
				}
			},
			{ nullptr, nullptr }
		};

		luaL_register(L, nullptr, funcs);

		return 0;
	});

	ADD_FILTER_PARAMETER(FlangerFilter, DELAY);
	ADD_FILTER_PARAMETER(FlangerFilter, FREQ);
}

//
//
//

static void AddFreeverbFilter (lua_State * L)
{	
	AddFilterType<SoLoud::FreeverbFilter>(L, [](lua_State * L) {
		luaL_Reg funcs[] = {
			{
				"setParams", [](lua_State * L)
				{
					luaL_checktype(L, 2, LUA_TTABLE);
					lua_getfield(L, 2, "mode"); // freeverb_filter, params, mode
					lua_getfield(L, 2, "roomSize"); // freeverb_filter, params, delay, mode, room_size
					lua_getfield(L, 2, "damp"); // freeverb_filter, params, mode, room_size, damp
					lua_getfield(L, 2, "width"); // freeverb_filter, params, delay, mode, room_size, damp, width

					return Result(L, GetFilter<SoLoud::FreeverbFilter>(L)->setParams(LuaXS::Float(L, -4), LuaXS::Float(L, -3), LuaXS::Float(L, -2), LuaXS::Float(L, -1)));
				}
			},
			{ nullptr, nullptr }
		};

		luaL_register(L, nullptr, funcs);

		return 0;
	});

	ADD_FILTER_PARAMETER(FreeverbFilter, FREEZE);
	ADD_FILTER_PARAMETER(FreeverbFilter, ROOMSIZE);
	ADD_FILTER_PARAMETER(FreeverbFilter, DAMP);
	ADD_FILTER_PARAMETER(FreeverbFilter, WIDTH);
}

//
//
//

static void AddLofiFilter (lua_State * L)
{
	AddFilterType<SoLoud::LofiFilter>(L, [](lua_State * L) {
		luaL_Reg funcs[] = {
			{
				"setParams", [](lua_State * L)
				{
					luaL_checktype(L, 2, LUA_TTABLE);
					lua_getfield(L, 2, "sampleRate"); // lofi_filter, params, sample_rate
					lua_getfield(L, 2, "bitDepth"); // lofi_filter, params, delay, sample_rate, bit_depth

					return Result(L, GetFilter<SoLoud::LofiFilter>(L)->setParams(LuaXS::Float(L, -2), LuaXS::Float(L, -1)));
				}
			},
			{ nullptr, nullptr }
		};

		luaL_register(L, nullptr, funcs);

		return 0;
	});

	ADD_FILTER_PARAMETER(LofiFilter, SAMPLERATE);
	ADD_FILTER_PARAMETER(LofiFilter, BITDEPTH);
}

//
//
//

static void AddRobotizeFilter (lua_State * L)
{
	AddFilterType<SoLoud::RobotizeFilter>(L, [](lua_State * L) {
		luaL_Reg funcs[] = {
			{
				"setParams", [](lua_State * L)
				{
					luaL_checktype(L, 2, LUA_TTABLE);
					lua_getfield(L, 2, "frequency"); // lofi_filter, params, frequency
					lua_getfield(L, 2, "waveform"); // lofi_filter, params, delay, frequency, waveform

					GetFilter<SoLoud::RobotizeFilter>(L)->setParams(LuaXS::Float(L, -2), GetWaveform(L, -1));

					return Result(L, SoLoud::SO_NO_ERROR);
				}
			},
			{ nullptr, nullptr }
		};

		luaL_register(L, nullptr, funcs);

		lua_pushcfunction(L, [](lua_State * L) {
			// n.b. index ignored; WAVE is only INT
			lua_pushliteral(L, "INT"); // index, "INT"
			lua_newuserdata(L, 0); // index, "INT", wave_proxy

			LuaXS::AttachMethods(L, MT_NAME(RobotizeFilterWaveProxy), [](lua_State * L) {
				LuaXS::AttachProperties(L, [](lua_State * L) {
					if (FoundEnum(L, GetWaveformModelList(), 2)) return 1; // wave_proxy, index[, v]

					return 0;
				});
			});

			return 2;
		}); // mt, aug
		AugmentGetParamType(L); // mt

		return 0;
	});

	ADD_FILTER_PARAMETER(RobotizeFilter, FREQ);
	ADD_FILTER_PARAMETER(RobotizeFilter, WAVE);
}

//
//
//

static void AddWaveShaperFilter (lua_State * L)
{
	AddFilterType<SoLoud::WaveShaperFilter>(L, [](lua_State * L) {
		luaL_Reg funcs[] = {
			{
				"setParams", [](lua_State * L)
				{
					luaL_checktype(L, 2, LUA_TTABLE);
					lua_getfield(L, 2, "amount"); // lofi_filter, params, amount

					return Result(L, GetFilter<SoLoud::WaveShaperFilter>(L)->setParams(LuaXS::Float(L, -1)));
				}
			},
			{ nullptr, nullptr }
		};

		luaL_register(L, nullptr, funcs);

		return 0;
	});

	ADD_FILTER_PARAMETER(WaveShaperFilter, AMOUNT);
}

//
//
//

static int sFilterParametersRef;

void PushFilterParameters (lua_State * L)
{
	lua_getref(L, sFilterParametersRef); // ..., filter_params
}

//
//

void add_filters (lua_State * L)
{
	lua_newtable(L); // soloud, filter_params
	lua_pushinteger(L, 0); // soloud, filter_params
	lua_setfield(L, -2, "Filter.WET"); // soloud, filter_params = { ..., ["Filter.WET"] = 1 }
	lua_insert(L, -2); // filter_params, soloud

	AddBassboostFilter(L);
	AddBiquadResonantFilter(L);
	AddDCRemovaltFilter(L);
	AddDuckFilter(L);
	AddEchoFilter(L);
	AddEQFilter(L);
	AddFFTFilter(L);
	AddFlangerFilter(L);
	AddFreeverbFilter(L);
	AddLofiFilter(L);
	AddRobotizeFilter(L);
	AddWaveShaperFilter(L);

	lua_insert(L, -2); // soloud, filter_params

	sFilterParametersRef = lua_ref(L, 1); // soloud; ref = filter_params
}