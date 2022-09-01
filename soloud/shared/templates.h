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

#pragma once

//
//
//

static FloatBuffer * GetFloatBuffer (lua_State * L, const char * name)
{
	lua_getfenv(L, 1); // object, ..., env
	lua_getfield(L, -1, name); // object, ..., env, buffer?

	if (lua_isnil(L, -1))
	{
		FloatBuffer * buffer = NewFloatBuffer(L); // object, ..., env, nil, buffer

		buffer->mSize = 256;
		buffer->mOwnsData = false;

		lua_remove(L, -2); // object, ..., env, buffer
		lua_pushvalue(L, -1); // object, ..., env, buffer, buffer
		lua_setfield(L, -3, name); // object, ..., env = { ..., [name] = buffer }, buffer
	}

	lua_remove(L, -2); // object, ..., buffer

	return LuaXS::UD<FloatBuffer>(L, -1);
}

//
//
//

template<typename T, T * (*getter)(lua_State *)> void BusCore (lua_State * L)
{
	luaL_Reg funcs[] = {
		{
			"calcFFT", [](lua_State * L)
			{
				FloatBuffer * buffer = GetFloatBuffer(L, "fft_buffer"); // object, buffer

				buffer->mData = getter(L)->calcFFT();

				return 1;
			}
		}, {
			"getActiveVoiceCount", [](lua_State * L)
			{
				lua_pushinteger(L, getter(L)->getActiveVoiceCount()); // object, count

				return 1;
			}
		}, {
			"getApproximateVolume", [](lua_State * L)
			{
				lua_pushnumber(L, getter(L)->getApproximateVolume(LuaXS::Uint(L, 2) - 1));

				return 1;
			}
		}, {
			"getWave", [](lua_State * L)
			{
				FloatBuffer * buffer = GetFloatBuffer(L, "wave_buffer"); // object, buffer

				buffer->mData = getter(L)->getWave();

				return 1;
			}
		}, {
			"play", [](lua_State * L)
			{
				Options opts;

				opts.mWantPan = opts.mWantVolume = opts.mWantPaused = true;

				opts.Get(L, 3);

				return PushHandle(L, getter(L)->play(
					*GetAudioSource(L, 2),
					opts.mVolume, opts.mPan, opts.mPaused
				)); // object, source[, opts], handle
			}
		}, {
			"playClocked", [](lua_State * L)
			{
				Options opts;

				opts.mWantPan = opts.mWantVolume = true;

				opts.Get(L, 4);

				return PushHandle(L, getter(L)->playClocked(
					lua_tonumber(L, 2), *GetAudioSource(L, 3),
					opts.mVolume, opts.mPan
				)); // object, time, source[, opts], handle
			}
		}, {
			"play3d", [](lua_State * L)
			{
				Options opts;

				opts.mWantVelocity = opts.mWantVolume = opts.mWantPaused = true;

				opts.Get(L, 6);

				return PushHandle(L, getter(L)->play3d(
					*GetAudioSource(L, 2), LuaXS::Float(L, 3), LuaXS::Float(L, 4), LuaXS::Float(L, 5),
					opts.mVelX, opts.mVelY, opts.mVelZ,
					opts.mVolume, opts.mPaused
				)); // object, source, x, y, z[, opts], handle
			}
		}, {
			"play3dClocked", [](lua_State * L)
			{
				Options opts;

				opts.mWantVelocity = opts.mWantVolume = true;

				opts.Get(L, 6);

				return PushHandle(L, getter(L)->play3dClocked(
					lua_tonumber(L, 2), *GetAudioSource(L, 3), LuaXS::Float(L, 3), LuaXS::Float(L, 4), LuaXS::Float(L, 5),
					opts.mVelX, opts.mVelY, opts.mVelZ,
					opts.mVolume
				)); // object, time, source, x, y, z[, opts], handle
			}
		}, {
			"setVisualizationEnable", [](lua_State * L)
			{
				getter(L)->setVisualizationEnable(lua_toboolean(L, 2));

				return 0;
			}
		},
		{ nullptr, nullptr }
	};

	luaL_register(L, nullptr, funcs);
}