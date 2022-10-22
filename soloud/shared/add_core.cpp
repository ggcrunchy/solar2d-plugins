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
#include "templates.h"
#include <vector>

//
//
//

struct CoreBox {
	SoLoud::Soloud mCore;
	bool mDestroyed{false};
};

static CoreBox * GetCoreBox (lua_State * L, int arg)
{
	return LuaXS::CheckUD<CoreBox>(L, arg, MT_NAME(Soloud));
}

SoLoud::Soloud * GetCore (lua_State * L, int arg)
{
	CoreBox * box = GetCoreBox(L, arg);

	luaL_argcheck(L, !box->mDestroyed, arg, "Core already destroyed");

	return &box->mCore;
}

static SoLoud::Soloud * GetSoloud (lua_State * L)
{
	return GetCore(L);
}

//
//
//

static CoreBox * sCurrentCore;

//
//
//

#define DO_VOID(NAME) #NAME, [](lua_State * L) { \
    GetSoloud(L)->NAME();                        \
    return 0;                                    \
}

#define DO_BOOLEAN(NAME) #NAME, [](lua_State * L) { \
    GetSoloud(L)->NAME(lua_toboolean(L, 2));        \
    return 0;                                       \
}

#define DO_FLOAT(NAME) #NAME, [](lua_State * L) { \
    GetSoloud(L)->NAME(LuaXS::Float(L, 2));       \
    return 0;                                     \
}

#define DO_NUMBER(NAME) #NAME, [](lua_State * L) { \
    GetSoloud(L)->NAME(lua_tonumber(L, 2));        \
    return 0;                                      \
}

#define DO_FLOAT3(NAME) #NAME, [](lua_State * L) {                                  \
    GetSoloud(L)->NAME(LuaXS::Float(L, 2), LuaXS::Float(L, 3), LuaXS::Float(L, 4)); \
    return 0;                                                                       \
}

#define DO_HANDLE(NAME) #NAME, [](lua_State * L) { \
    GetSoloud(L)->NAME(GetHandle(L, 2));           \
    return 0;                                      \
}

#define DO_HANDLE_BOOLEAN(NAME) #NAME, [](lua_State * L) {    \
    GetSoloud(L)->NAME(GetHandle(L, 2), lua_toboolean(L, 3)); \
    return 0;                                                 \
}

#define DO_HANDLE_FLOAT(NAME) #NAME, [](lua_State * L) {     \
    GetSoloud(L)->NAME(GetHandle(L, 2), LuaXS::Float(L, 3)); \
    return 0;                                                \
}

#define DO_HANDLE_NUMBER(NAME) #NAME, [](lua_State * L) {    \
    GetSoloud(L)->NAME(GetHandle(L, 2), lua_tonumber(L, 3)); \
    return 0;                                                \
}

#define DO_HANDLE_FLOAT2(NAME) #NAME, [](lua_State * L) {                        \
    GetSoloud(L)->NAME(GetHandle(L, 2), LuaXS::Float(L, 3), LuaXS::Float(L, 4)); \
    return 0;                                                                    \
}

#define DO_HANDLE_FLOAT3(NAME) #NAME, [](lua_State * L) {                                            \
    GetSoloud(L)->NAME(GetHandle(L, 2), LuaXS::Float(L, 3), LuaXS::Float(L, 4), LuaXS::Float(L, 5)); \
    return 0;                                                                                        \
}

#define GET_BOOLEAN_VOID(NAME) #NAME, [](lua_State * L) { \
    lua_pushboolean(L, GetSoloud(L)->NAME());             \
    return 1;                                             \
}

#define GET_INTEGER_VOID(NAME) #NAME, [](lua_State * L) { \
    lua_pushinteger(L, GetSoloud(L)->NAME());             \
    return 1;                                             \
}

#define GET_NUMBER_VOID(NAME) #NAME, [](lua_State * L) { \
    lua_pushnumber(L, GetSoloud(L)->NAME());             \
    return 1;                                            \
}

#define GET_BOOLEAN_HANDLE(NAME) #NAME, [](lua_State * L) {  \
    lua_pushboolean(L, GetSoloud(L)->NAME(GetHandle(L, 2))); \
    return 1;                                                \
}

#define GET_NUMBER_HANDLE(NAME) #NAME, [](lua_State * L) {  \
    lua_pushnumber(L, GetSoloud(L)->NAME(GetHandle(L, 2))); \
    return 1;                                               \
}

#define FADE(NAME) #NAME, [](lua_State * L) {                                    \
    GetSoloud(L)->NAME(GetHandle(L, 2), LuaXS::Float(L, 3), lua_tonumber(L, 4)); \
    return 0;                                                                    \
}

#define OSCILLATE(NAME) #NAME, [](lua_State * L) {                                                   \
    GetSoloud(L)->NAME(GetHandle(L, 2), LuaXS::Float(L, 3), LuaXS::Float(L, 4), lua_tonumber(L, 5)); \
    return 0;                                                                                        \
}

//
//
//

static unsigned int Index (lua_State * L, int arg)
{
	return LuaXS::Uint(L, arg) - 1;
}

//
//
//

static unsigned int FilterAttribute (lua_State * L, int arg)
{
	if (lua_type(L, arg) == LUA_TSTRING) // lua_isstring() will coerce indices
	{
		arg = CoronaLuaNormalize(L, arg);

		PushFilterParameters(L); // ..., name, ..., filter_params

		lua_pushvalue(L, arg); // ..., name, ..., filter_params, name
		lua_rawget(L, -2); // ..., name, ..., filter_params, value?
		luaL_checktype(L, -1, LUA_TNUMBER);

		unsigned int result = lua_tointeger(L, -1);

		lua_pop(L, 2); // ..., name, ...

		return result;
	}

	else return Index(L, arg);
}

//
//
//

static int sCallbacksRef;

//
//
//

static void IssueCallbacks (lua_State * L, SoLoud::Soloud & core)
{
	std::vector<SoLoud::OnComplete> finished;

	core.swapOnComplete(finished);

	//
	//
	//

	lua_getref(L, sCallbacksRef); // ..., callbacks
	lua_getfield(L, -1, "handlers"); // ..., callbacks, handlers
	lua_getfield(L, -2, "event"); // ..., callbacks, handlers, event

	for (auto iter = finished.begin(); iter != finished.end(); ++iter)
	{
		lua_pushinteger(L, iter->mID); // ..., callbacks, handlers, event, id
		lua_rawget(L, -3); // ..., callbacks, handlers, event, on_complete
		lua_pushinteger(L, iter->mID); // ..., callbacks, handlers, event, on_complete, id
		lua_pushnil(L); // ..., callbacks, handlers, event, on_complete, id, nil
		lua_rawset(L, -5); // ..., callbacks, handlers = { ..., [id] = nil }, event, on_complete
		lua_pushvalue(L, -2); // ..., callbacks, handlers, event, on_complete, event
		lua_pushboolean(L, iter->mDone); // ..., callbacks, handlers, event, on_complete, event, done
		lua_setfield(L, -2, "completed"); // ..., callbacks, handlers, event, on_complete, event = { completed = done }

		if (lua_pcall(L, 1, 0, 0) != 0) // ..., callbacks, handlers, event[, err]
		{
			CORONA_LOG_WARNING("Error in `onComplete` callback: %s", lua_isstring(L, -1) ? lua_tostring(L, -1) : "?");

			lua_pop(L, 1); // ..., callbacks, handlers, event
		}
	}

	lua_pop(L, 3); // ...
}

//
//
//

static void Deinitialize (SoLoud::Soloud & core)
{
	core.~Soloud();
}

static void Shutdown (lua_State * L, SoLoud::Soloud & core, int arg = 1)
{
	IssueCallbacks(L, core);
	Deinitialize(core);

	RemoveEnvironment(L, arg);
	RemoveFromStore(L, &core);
}

//
//
//

static int DoOnCompletes (lua_State * L)
{
	if (sCurrentCore) IssueCallbacks(L, sCurrentCore->mCore);
	else CORONA_LOG_WARNING("No core present to deliver `onComplete` results");
	
	lua_pushliteral(L, "enterFrame"); // ..., "enterFrame"
	lua_getref(L, sCallbacksRef); // ..., "enterFrame", callbacks
	lua_getfield(L, -1, "handlers"); // ..., "enterFrame", callbacks, handlers
	lua_pushnil(L); // ..., "enterFrame", callbacks, handlers, nil

	if (lua_next(L, -2) == 0) // ..., "enterFrame", callbacks, handlers[, k, v]
	{
		lua_pop(L, 1); // ..., "enterFrame", callbacks
		lua_pushnil(L); // ..., "enterFrame", callbacks, nil
		lua_setfield(L, -2, "added"); // ..., "enterFrame", callbacks = { ..., added = nil }
		lua_getfield(L, -1, "Runtime"); // ..., "enterFrame", callbacks, Runtime
		lua_getfield(L, -1, "removeEventListener"); // ..., "enterFrame", callbacks, Runtime, Runtime:removeEventListener
		lua_insert(L, -4); // ..., Runtime:removeEventListener, "enterFrame", callbacks, Runtime
		lua_insert(L, -3); // ..., Runtime:removeEventListener, Runtime, "enterFrame", callbacks
		lua_call(L, 3, 0); // ...
	}

	return 0;
}

//
//
//

void PrepareOnComplete (lua_State * L, unsigned int id)
{
	lua_getref(L, sCallbacksRef); // ..., on_complete, callbacks
	lua_getfield(L, -1, "Runtime"); // ..., on_complete, callbacks, Runtime

	if (lua_isnil(L, -1))
	{
		CORONA_LOG_ERROR("Unable to find Runtime to register `onComplete` callback");

		lua_pop(L, 3); // ..., on_complete
	}

	else
	{
		lua_insert(L, -3); // ..., Runtime, on_complete, callbacks
		lua_getfield(L, -1, "handlers"); // ..., Runtime, on_complete, callbacks, handlers
		lua_pushinteger(L, id); // ..., Runtime, on_complete, callbacks, handlers, id
		lua_pushvalue(L, -4); // ..., Runtime, on_complete, callbacks, handlers, id, on_complete
		lua_rawset(L, -3); // ..., Runtime, on_complete, callbacks, handlers = { ..., [id] = on_complete }
		lua_getfield(L, -2, "added"); // ..., Runtime, on_complete, callbacks, handlers, added?

		bool already_added = lua_toboolean(L, -1);

		lua_pop(L, 2); // ..., Runtime, on_complete, callbacks

		if (!already_added)
		{
			lua_pushboolean(L, 1); // ..., Runtime, on_complete, callbacks, true
			lua_setfield(L, -2, "added"); // ..., Runtime, on_complete, callbacks = { ..., added = true }
			lua_pushliteral(L, "enterFrame"); // ..., Runtime, on_complete, callbacks, "enterFrame"
			lua_replace(L, -3); // ..., Runtime, "enterFrame", callbacks
			lua_getfield(L, -3, "addEventListener"); // ..., Runtime, "enterFrame", callbacks = { ..., [id] = on_complete }, Runtime:addEventListener
			lua_insert(L, -4); // ..., Runtime:addEventListener, Runtime, "enterFrame", callbacks = { ..., [id] = on_complete }
			lua_call(L, 3, 0); // ...
		}
	}
}

//
//
//

void SoloudMethods(lua_State * L)
{
	BusCore<SoLoud::Soloud, &GetSoloud>(L);

	luaL_Reg funcs[] = {
		{
			"addVoiceToGroup", [](lua_State * L)
			{
				return Result(L, GetSoloud(L)->addVoiceToGroup(GetHandle(L, 2), GetHandle(L, 3)));
			}
		}, {
			"countAudioSource", [](lua_State * L)
			{
				lua_pushinteger(L, GetSoloud(L)->countAudioSource(*GetAudioSource(L, 2))); // soloud, source, count

				return 1;
			}
		}, {
			"createVoiceGroup", [](lua_State * L)
			{
				lua_pushnumber(L, GetSoloud(L)->createVoiceGroup()); // soloud, group

				return 1;
			}
		}, {
			"destroy", [](lua_State * L)
			{
				CoreBox * box = GetCoreBox(L, 1);

				if (!box->mDestroyed)
				{
					luaL_argcheck(L, box == sCurrentCore, 1, "More than one core active");

					sCurrentCore = nullptr;

					Shutdown(L, box->mCore);
				}

				box->mDestroyed = true;

				return 0;
			}
		}, {
			"destroyVoiceGroup", [](lua_State * L)
			{
				return Result(L, GetSoloud(L)->destroyVoiceGroup(GetHandle(L, 2)));
			}
		}, {
			"fadeFilterParameter", [](lua_State * L)
			{
				GetSoloud(L)->fadeFilterParameter(GetHandle(L, 2), Index(L, 3), FilterAttribute(L, 4), LuaXS::Float(L, 5), lua_tonumber(L, 6));

				return 0;
			}
		}, {
			FADE(fadeRelativePlaySpeed)
		}, {
			FADE(fadePan)
		}, {
			FADE(fadeVolume)
		}, {
			"__gc", [](lua_State * L)
			{
				CoreBox * box = GetCoreBox(L, 1);

				if (!box->mDestroyed)
				{
					box->mCore.mShuttingDown = true;

					Deinitialize(box->mCore);
				}

				return 0;
			}
		}, {
			GET_BOOLEAN_HANDLE(getAutoStop)
		},{
			"getFilterParameter", [](lua_State * L)
			{
				lua_pushnumber(L, GetSoloud(L)->getFilterParameter(GetHandle(L, 2), Index(L, 3), FilterAttribute(L, 4))); // soloud, handle, filter_id, attribute_id, param

				return 1;
			}
		}, {
			GET_NUMBER_VOID(getGlobalVolume)
		}, {
			GET_INTEGER_VOID(getBackendBufferSize)
		}, {
			GET_INTEGER_VOID(getBackendChannels)
		}, {
			GET_INTEGER_VOID(getBackendId)
		}, {
			GET_INTEGER_VOID(getBackendSamplerate)
		}, {
			"getBackendString", [](lua_State * L)
			{
				const char * str = GetSoloud(L)->getBackendString();

				if (str) lua_pushstring(L, str); // soloud, str
				else lua_pushnil(L); // soloud, nil

				return 1;
			}
		}, {
			"getInfo", [](lua_State * L)
			{
				lua_pushnumber(L, GetSoloud(L)->getInfo(GetHandle(L, 2), Index(L, 3))); // soloud, handle, key, info

				return 1;
			}
		}, {
			"getLoopCount", [](lua_State * L)
			{
				lua_pushinteger(L, GetSoloud(L)->getLoopCount(GetHandle(L, 2)));

				return 1;
			}
		}, {
			GET_BOOLEAN_HANDLE(getLooping)
		}, {
			GET_NUMBER_HANDLE(getLoopPoint)
		}, {
			"getMainResampler", [](lua_State * L)
			{
				lua_pushstring(L, GetResamplerString(L, GetSoloud(L)->getMainResampler())); // soloud, resampler_name

				return 1;
			}
		}, {
			GET_INTEGER_VOID(getMaxActiveVoiceCount)
		}, {
			GET_NUMBER_HANDLE(getOverallVolume)
		}, {
			GET_NUMBER_HANDLE(getPan)
		}, {
			GET_BOOLEAN_HANDLE(getPause)
		}, {
			GET_NUMBER_VOID(getPostClipScaler)
		}, {
			GET_BOOLEAN_HANDLE(getProtectVoice)
		}, {
			GET_NUMBER_HANDLE(getRelativePlaySpeed)
		}, {
			GET_NUMBER_HANDLE(getSamplerate)
		}, {
			"getSpeakerPosition", [](lua_State * L)
			{
				float x, y, z;
				SoLoud::result result = GetSoloud(L)->getSpeakerPosition(Index(L, 2), x, y, z);

				if (SoLoud::SO_NO_ERROR == result)
				{
					lua_pushnumber(L, x); // soloud, channel, x
					lua_pushnumber(L, y); // soloud, channel, x, y
					lua_pushnumber(L, z); // soloud, channel, x, y, z

					return 3;
				}

				else return Result(L, result);
			}
		}, {
			GET_NUMBER_HANDLE(getStreamPosition)
		}, {
			GET_NUMBER_HANDLE(getStreamTime)
		}, {
			GET_INTEGER_VOID(getVersion)
		}, {
			GET_INTEGER_VOID(getVoiceCount)
		}, {
			GET_NUMBER_HANDLE(getVolume)
		}, {
			GET_NUMBER_VOID(get3dSoundSpeed)
		}, {
			GET_BOOLEAN_HANDLE(isValidVoiceHandle)
		}, {
			GET_BOOLEAN_HANDLE(isVoiceGroup)
		}, {
			GET_BOOLEAN_HANDLE(isVoiceGroupEmpty)
		}, {
			"mix", [](lua_State * L)
			{
				SoLoud::Soloud * soloud = GetSoloud(L);
				std::vector<float> floats;
				float * data;
				unsigned int samples;

				if (lua_isuserdata(L, 2))
				{
					FloatBuffer * buffer = GetFloatBuffer(L, 2);
					size_t def_size = buffer->mSize / soloud->mChannels;

					samples = luaL_optinteger(L, 3, def_size);
					data = buffer->mData;
			
					luaL_argcheck(L, buffer->mOwnsData, 2, "Attempt to mix() unowned data");
					luaL_argcheck(L, samples > 0, !lua_isnoneornil(L, 3) ? 3 : 2, "mix() must have > 0 samples");
					luaL_argcheck(L, samples <= def_size, 3, "mix() not given enough space for samples");
					lua_pushboolean(L, 1); // buffer[, samples], true
				}

				else
				{
					samples = luaL_checkinteger(L, 2);

					luaL_argcheck(L, samples > 0, 2, "mix() must have > 0 samples");

					floats.resize(samples * soloud->mChannels);

					data = floats.data();
				}

				soloud->mix(data, samples);

				if (!floats.empty()) lua_pushlstring(L, reinterpret_cast<char *>(data), floats.size() * sizeof(float)); // samples, data

				return 1;
			}
		}, {
			"mixSigned16", [](lua_State * L)
			{
				// TODO? could add buffers for signed16 (also 8-bit)
				SoLoud::Soloud * soloud = GetSoloud(L);
				unsigned int samples = luaL_checkinteger(L, 2);

				luaL_argcheck(L, samples > 0, 2, "mixSigned16() must have > 0 samples");

				std::vector<short> shorts(samples);

				soloud->mixSigned16(shorts.data(), samples * soloud->mChannels);

				if (!shorts.empty()) lua_pushlstring(L, reinterpret_cast<char *>(shorts.data()), samples * sizeof(short)); // samples, data

				return 1;
			}
		}, {
			"oscillateFilterParameter", [](lua_State * L)
			{
				GetSoloud(L)->oscillateFilterParameter(GetHandle(L, 2), Index(L, 3), FilterAttribute(L, 4), LuaXS::Float(L, 5), LuaXS::Float(L, 6), lua_tonumber(L, 7));

				return 0;
			}
		}, {
			"oscillateGlobalVolume", [](lua_State * L)
			{
				GetSoloud(L)->oscillateGlobalVolume(LuaXS::Float(L, 2), LuaXS::Float(L, 3), lua_tonumber(L, 4));

				return 0;
			}
		}, {
			OSCILLATE(oscillatePan)
		}, {
			OSCILLATE(oscillateRelativePlaySpeed)
		}, {
			OSCILLATE(oscillateVolume)
		}, {
			"playBackground", [](lua_State * L)
			{
				Options opts;

				opts.mWantVolume = opts.mPaused = opts.mWantCallback = true;

				opts.Get(L, 3); // soloud, source[, opts][, on_complete]

				unsigned int id = 0;
				SoLoud::handle handle = GetSoloud(L)->playBackground(
					*GetAudioSource(L, 2), opts.mGotCallback ? &id : nullptr,
					opts.mVolume, opts.mPaused
				);

				if (opts.mGotCallback && id > 0) PrepareOnComplete(L, id); // soloud, source, opts

				return PushHandle(L, handle); // soloud, source[, opts][, on_complete], handle
			}
		}, {
			DO_HANDLE_NUMBER(schedulePause)
		}, {
			DO_HANDLE_NUMBER(scheduleStop)
		}, {
			"seek", [](lua_State * L)
			{
				return Result(L, GetSoloud(L)->seek(GetHandle(L, 2), lua_tonumber(L, 3)));
			}
		}, {
			DO_HANDLE_BOOLEAN(setAutoStop)
		}, {
			"setChannelVolume", [](lua_State * L)
			{
				GetSoloud(L)->setChannelVolume(GetHandle(L, 2), Index(L, 3), LuaXS::Float(L, 4));

				return 0;
			}
		}, {
			"setDelaySamples", [](lua_State * L)
			{
				GetSoloud(L)->setDelaySamples(GetHandle(L, 2), LuaXS::Uint(L, 3));

				return 0;
			}
		}, {
			"setFilterParameter", [](lua_State * L)
			{
				GetSoloud(L)->setFilterParameter(GetHandle(L, 2), Index(L, 3), FilterAttribute(L, 4), LuaXS::Float(L, 5));

				return 0;
			}
		}, {
			"setGlobalFilter", [](lua_State * L)
			{
				SoLoud::Soloud * soloud = GetSoloud(L);
				unsigned int index = LuaXS::Uint(L, 2) - 1;
				SoLoud::Filter * filter = GetFilter(L, 3); // soloud, index, box / nil[, filter]

				SetFilterRefToEnvironment(L, 1, int(index) + 1); // soloud, index[, box, filter]

				soloud->setGlobalFilter(index, filter);

				return 0;
			}
		}, {
			DO_FLOAT(setGlobalVolume)
		}, {
			"setInaudibleBehavior", [](lua_State * L)
			{
				GetSoloud(L)->setInaudibleBehavior(GetHandle(L, 2), lua_toboolean(L, 3), lua_toboolean(L, 4));

				return 0;
			}
		}, {
			"setLooping", [](lua_State * L)
			{
				bool is_number = lua_isnumber(L, 3);
				int count = is_number ? lua_tointeger(L, 3) : 0;

				GetSoloud(L)->setLooping(GetHandle(L, 2), !is_number && lua_toboolean(L, 3), count);

				return 0;
			}
		}, {
			DO_HANDLE_NUMBER(setLoopPoint)
		}, {
			"setMaxActiveVoiceCount", [](lua_State * L)
			{
				return Result(L, GetSoloud(L)->setMaxActiveVoiceCount(LuaXS::Uint(L, 2)));
			}
		}, {
			"setMainResampler", [](lua_State * L)
			{
				GetSoloud(L)->setMainResampler(GetResampler(L, 2));

				return 0;
			}
		}, {
			DO_HANDLE_FLOAT(setPan)
		}, {
			DO_HANDLE_FLOAT2(setPanAbsolute)
		}, {
			DO_HANDLE_BOOLEAN(setPause)
		}, {
			DO_BOOLEAN(setPauseAll)
		}, {
			DO_FLOAT(setPostClipScaler)
		}, {
			DO_HANDLE_BOOLEAN(setProtectVoice)
		}, {
			DO_HANDLE_FLOAT(setRelativePlaySpeed)
		}, {
			DO_HANDLE_FLOAT(setSamplerate)
		}, {
			"setSpeakerPosition", [](lua_State * L)
			{
				GetSoloud(L)->setSpeakerPosition(Index(L, 2), LuaXS::Float(L, 3), LuaXS::Float(L, 4), LuaXS::Float(L, 5));
    
				return 0;
			}
		}, {
			DO_HANDLE_FLOAT(setVolume)
		}, {
			DO_FLOAT3(set3dListenerAt)
		}, {
			"set3dListenerParameters", [](lua_State * L)
			{
				Options opts;

				opts.mWantVelocity = true;

				opts.Get(L, 11);

				GetSoloud(L)->set3dListenerParameters(
					LuaXS::Float(L, 2), LuaXS::Float(L, 3), LuaXS::Float(L, 4),
					LuaXS::Float(L, 5), LuaXS::Float(L, 6), LuaXS::Float(L, 7),
					LuaXS::Float(L, 8), LuaXS::Float(L, 9), LuaXS::Float(L, 10),
					opts.mVelX, opts.mVelY, opts.mVelZ
				);

				return 0;
			}
		}, {
			DO_FLOAT3(set3dListenerPosition)
		}, {
			DO_FLOAT3(set3dListenerUp)
		}, {
			DO_FLOAT3(set3dListenerVelocity)
		}, {
			"set3dSourceAttenuation", [](lua_State * L)
			{
				GetSoloud(L)->set3dSourceAttenuation(GetHandle(L, 2), GetAttenuationModel(L, 3), LuaXS::Float(L, 4));

				return 0;
			}
		}, {
			DO_HANDLE_FLOAT(set3dSourceDopplerFactor)
		}, {
			DO_HANDLE_FLOAT2(set3dSourceMinMaxDistance)
		}, {
			"set3dSourceParameters", [](lua_State * L)
			{
				Options opts;

				opts.mWantVelocity = opts.mWantVolume = true;

				opts.Get(L, 6);

				GetSoloud(L)->set3dSourceParameters(GetHandle(L, 2),
					LuaXS::Float(L, 3), LuaXS::Float(L, 4), LuaXS::Float(L, 5),
					opts.mVelX, opts.mVelY, opts.mVelZ
				);

				return 0;
			}
		}, {
			DO_HANDLE_FLOAT3(set3dSourcePosition)
		}, {
			DO_HANDLE_FLOAT3(set3dSourceVelocity)
		}, {
			DO_HANDLE(stop)
		}, {
			DO_VOID(stopAll)
		}, {
			"stopAudioSource", [](lua_State * L)
			{
				GetSoloud(L)->stopAudioSource(*GetAudioSource(L, 2));

				return 0;
			}
		}, {
			DO_VOID(update3dAudio)
		},
		{ nullptr, nullptr }
	};

	luaL_register(L, nullptr, funcs);
}

//
//
//

void GetCreateCoreParams (lua_State * L, unsigned int & flags, unsigned int & backend, unsigned int & sample_rate, unsigned int & buffer_size, unsigned int & channels)
{
	lua_getfield(L, 1, "flags"); // params, flags?

	const char * flag_names[] = { "CLIP_ROUNDOFF", "ENABLE_VISUALIZATION", "LEFT_HANDED_3D", "NO_FPU_REGISTER_CHANGE", nullptr };

	if (lua_isstring(L, -1)) flags = 1U << luaL_checkoption(L, -1, nullptr, flag_names);
	else if (!lua_isnil(L, -1))
	{
		luaL_checktype(L, -1, LUA_TTABLE);

		for (size_t i = 0, len = lua_objlen(L, -1); i < len; ++i, lua_pop(L, 1))
		{
			lua_rawgeti(L, -1, int(i + 1)); // params, flags, flag

			flags |= 1U << luaL_checkoption(L, -1, nullptr, flag_names);
		}
	}

	//
	//
	//

	lua_getfield(L, 1, "backend"); // params, flags?, backend?

	const char * backend_names[] = { "AUTO", "SDL1", "SDL2", "PORTAUDIO", "WINMM", "XAUDIO2", "WASAPI", "ALSA", "JACK", "OSS", "OPENAL", "COREAUDIO", "OPENSLES", "VITA_HOMEBREW", "MINIAUDIO", "NOSOUND", "NULLDRIVER", nullptr };

	backend = luaL_checkoption(L, -1, "AUTO", backend_names);

	//
	//
	//

	lua_getfield(L, 1, "samplerate"); // params, flags?, backend?, samplerate?
	lua_getfield(L, 1, "buffersize"); // params, flags?, backend?, samplerate?, buffersize?
	lua_getfield(L, 1, "channels"); // params, flags?, backend?, samplerate?, channels?

	sample_rate = luaL_optinteger(L, -3, sample_rate);
	buffer_size = luaL_optinteger(L, -2, buffer_size);
	channels = luaL_optinteger(L, -1, channels);
}

int CreateCore (lua_State * L)
{
	if (sCurrentCore)
	{
		CORONA_LOG_WARNING("Creating SoLoud core, but another was still active; shutting old one down");
		
		sCurrentCore->mDestroyed = true; // avoid "shutting down" logic
		
		GetFromStore(L, sCurrentCore); // [params, ]old_core
		Shutdown(L, sCurrentCore->mCore, -1);

		lua_pop(L, 1); // [params]

		sCurrentCore = nullptr;
	}

	unsigned int flags = SoLoud::Soloud::CLIP_ROUNDOFF, backend = SoLoud::Soloud::AUTO, sample_rate = 0U, buffer_size = 0U, channels = 2U;

	if (lua_istable(L, 1)) GetCreateCoreParams(L, flags, backend, sample_rate, buffer_size, channels);
		
	CoreBox * box = LuaXS::NewTyped<CoreBox>(L); // [params, ]core
	SoLoud::result result = box->mCore.init(flags, backend, sample_rate, buffer_size, channels);

	if (SoLoud::SO_NO_ERROR != result) return Result(L, result);

	// hash part: fft table, wave table, unpaused, state
	lua_createtable(L, FILTERS_PER_STREAM, 4); // [params, ]core, env
	lua_setfenv(L, -2); // [params, ]source; source.env = env

	LuaXS::AttachMethods(L, MT_NAME(Soloud), SoloudMethods);

	AddToStore(L);

	sCurrentCore = box;

	return 1;
}

//
//
//

void add_core (lua_State * L)
{
	lua_pushcfunction(L, CreateCore); // soloud, CreateCore
	lua_setfield(L, -2, "createCore"); // soloud = { ..., createCore = CreateCore }

	//
	//
	//

	sCurrentCore = nullptr;

	//
	//
	//

	lua_createtable(L, 0, 5); // soloud, callbacks
	lua_newtable(L); // soloud, callbacks, handlers
	lua_setfield(L, -2, "handlers"); // soloud, callbacks = { handlers = handlers }
	lua_pushcfunction(L, DoOnCompletes); // soloud, callbacks, DoOnCompletes
	lua_setfield(L, -2, "enterFrame"); // soloud, callbacks = { handlers, enterFrame = DoOnCompletes }
	lua_createtable(L, 0, 2); // soloud, callbacks, event = {}
	lua_setfield(L, -2, "event"); // soloud, callbacks = { handlers, enterFrame, event = event }
	lua_getglobal(L, "Runtime"); // soloud, callbacks, Runtime
	lua_setfield(L, -2, "Runtime"); // soloud, callbacks = { handlers, enterFrame, event, Runtime = Runtime }

	sCallbacksRef = lua_ref(L, 1); // soloud
}
