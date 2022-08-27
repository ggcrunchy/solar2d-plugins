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

template<typename T, T * (*getter)(lua_State *)> void BusCore (lua_State * L)
{
	luaL_Reg funcs[] = {
		{
			"calcFFT", [](lua_State * L)
			{
				float * fft = getter(L)->calcFFT();
				// TODO!
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
				lua_pushnumber(L, getter(L)->getApproximateVolume(LuaXS::Uint(L, 2)));

				return 1;
			}
		}, {
			"getWave", [](lua_State * L)
			{
				float * wave = getter(L)->getWave();
				// TODO!
				return 1;
			}
		}, {
			"play", [](lua_State * L)
			{
				lua_pushinteger(L, getter(L)->play(
					*GetAudioSource(L, 2),
					OptFloat(L, 3, 1), OptFloat(L, 4, 0), lua_toboolean(L, 5)
				)); // object, source[, volume, pan, paused], handle

				return 1;
			}
		}, {
			"playClocked", [](lua_State * L)
			{
				lua_pushinteger(L, getter(L)->playClocked(
					lua_tonumber(L, 2), *GetAudioSource(L, 3),
					OptFloat(L, 4, 1), OptFloat(L, 5, 0)
				)); // object, time, source[, volume, pan], handle

				return 1;
			}
		}, {
			"play3d", [](lua_State * L)
			{
				lua_pushinteger(L, getter(L)->play3d(
					*GetAudioSource(L, 2), LuaXS::Float(L, 3), LuaXS::Float(L, 4), LuaXS::Float(L, 5),
					OptFloat(L, 6, 0), OptFloat(L, 7, 0), OptFloat(L, 8, 0),
					OptFloat(L, 9, 1), lua_toboolean(L, 10)
				)); // object, source, x, y, z[, vel_x, vel_y, vel_z, volume, paused], handle

				return 1;
			}
		}, {
			"play3dClocked", [](lua_State * L)
			{
				lua_pushinteger(L, getter(L)->play3dClocked(
					lua_tonumber(L, 2), *GetAudioSource(L, 3), LuaXS::Float(L, 3), LuaXS::Float(L, 4), LuaXS::Float(L, 5),
					OptFloat(L, 6, 0), OptFloat(L, 7, 0), OptFloat(L, 8, 0),
					OptFloat(L, 9, 1)
				)); // object, time, source, x, y, z[, vel_x, vel_y, vel_z, volume], handle

				return 1;
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