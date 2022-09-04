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
	lua_pushinteger(L, SoLoud::TYPE::NAME + 1);  \
	lua_setfield(L, -2, #TYPE "." #NAME)

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
					return Result(L, GetFilter<SoLoud::BassboostFilter>(L)->setParams(LuaXS::Float(L, 2)));
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

static void AddBiquadResonantFilter (lua_State * L)
{
	AddFilterType<SoLoud::BiquadResonantFilter>(L, [](lua_State * L) {
		luaL_Reg funcs[] = {
			{
				"setParams", [](lua_State * L)
				{
					const char * types[] = { "LOWPASS", "HIGHPASS", "BANDPASS", nullptr };

					return Result(L, GetFilter<SoLoud::BiquadResonantFilter>(L)->setParams(luaL_checkoption(L, 2, nullptr, types), LuaXS::Float(L, 3), LuaXS::Float(L, 4)));
				}
			},
			{ nullptr, nullptr }
		};

		luaL_register(L, nullptr, funcs);

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
					return Result(L, GetFilter<SoLoud::DCRemovalFilter>(L)->setParams(LuaXS::Float(L, 2)));
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
					return Result(L, GetFilter<SoLoud::DuckFilter>(L)->setParams(
						GetCore(L, 2), LuaXS::Uint(L, 3),
						OptFloat(L, 4, .1f), OptFloat(L, 5, .5f), OptFloat(L, 6, .1f) // TODO: options table?
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
					return Result(L, GetFilter<SoLoud::EchoFilter>(L)->setParams(
						LuaXS::Float(L, 2),
						OptFloat(L, 3, .7f), OptFloat(L, 4, 0) // TODO: options table?
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
					return Result(L, GetFilter<SoLoud::EqFilter>(L)->setParam(LuaXS::Uint(L, 2), LuaXS::Float(L, 3)));
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
					return Result(L, GetFilter<SoLoud::FlangerFilter>(L)->setParams(LuaXS::Float(L, 2), LuaXS::Float(L, 3)));
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
					return Result(L, GetFilter<SoLoud::FreeverbFilter>(L)->setParams(LuaXS::Float(L, 2), LuaXS::Float(L, 3), LuaXS::Float(L, 4), LuaXS::Float(L, 5)));
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
					return Result(L, GetFilter<SoLoud::LofiFilter>(L)->setParams(LuaXS::Float(L, 2), LuaXS::Float(L, 3)));
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
					GetFilter<SoLoud::RobotizeFilter>(L)->setParams(LuaXS::Float(L, 2), GetWaveform(L, 3));

					return Result(L, SoLoud::SO_NO_ERROR);
				}
			},
			{ nullptr, nullptr }
		};

		luaL_register(L, nullptr, funcs);

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
					return Result(L, GetFilter<SoLoud::WaveShaperFilter>(L)->setParams(LuaXS::Float(L, 2)));
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

void add_filters (lua_State * L)
{
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

	lua_pushinteger(L, 1); // soloud, 1
	lua_setfield(L, -2, "Filter.WET"); // soloud = { ..., ["Filter.WET"] = 1 }
}