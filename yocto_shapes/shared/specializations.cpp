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
#include "yocto.h"
#include "yocto/yocto_math.h"

//
//
//

template<> yocto::vec2f LuaXS::GetArgBody<yocto::vec2f> (lua_State * L, int arg)
{
	luaL_checktype(L, arg, LUA_TTABLE);

	yocto::vec2f v;

	for (int i = 0; i < 2; ++i, lua_pop(L, 1))
	{
		lua_rawgeti(L, arg, i + 1); // ..., v, ..., value

		v[i] = LuaXS::Float(L, -1);
	}

	return v;
}

template<> yocto::vec3f LuaXS::GetArgBody<yocto::vec3f> (lua_State * L, int arg)
{
	luaL_checktype(L, arg, LUA_TTABLE);

	yocto::vec3f v;

	for (int i = 0; i < 3; ++i, lua_pop(L, 1))
	{
		lua_rawgeti(L, arg, i + 1); // ..., v, ..., value

		v[i] = LuaXS::Float(L, -1);
	}

	return v;
}

template<> yocto::vec4f LuaXS::GetArgBody<yocto::vec4f> (lua_State * L, int arg)
{
	luaL_checktype(L, arg, LUA_TTABLE);

	yocto::vec4f v;

	for (int i = 0; i < 4; ++i, lua_pop(L, 1))
	{
		lua_rawgeti(L, arg, i + 1); // ..., v, ..., value

		v[i] = LuaXS::Float(L, -1);
	}

	return v;
}

//
//
//

template<> yocto::vec2i LuaXS::GetArgBody<yocto::vec2i> (lua_State * L, int arg)
{
	luaL_checktype(L, arg, LUA_TTABLE);

	yocto::vec2i v;

	for (int i = 0; i < 2; ++i, lua_pop(L, 1))
	{
		lua_rawgeti(L, arg, i + 1); // ..., v, ..., value

		v[i] = luaL_checkint(L, -1);
	}

	return v;
}

template<> yocto::vec3i LuaXS::GetArgBody<yocto::vec3i> (lua_State * L, int arg)
{
	luaL_checktype(L, arg, LUA_TTABLE);

	yocto::vec3i v;

	for (int i = 0; i < 3; ++i, lua_pop(L, 1))
	{
		lua_rawgeti(L, arg, i + 1); // ..., v, ..., value

		v[i] = luaL_checkint(L, -1);
	}

	return v;
}

template<> yocto::vec4i LuaXS::GetArgBody<yocto::vec4i> (lua_State * L, int arg)
{
	luaL_checktype(L, arg, LUA_TTABLE);

	yocto::vec4i v;

	for (int i = 0; i < 4; ++i, lua_pop(L, 1))
	{
		lua_rawgeti(L, arg, i + 1); // ..., v, ..., value

		v[i] = luaL_checkint(L, -1);
	}

	return v;
}

//
//
//

template<> std::vector<int> * LuaXS::GetArgBody<std::vector<int> *> (lua_State * L, int arg) { return &GetVector1i(L, arg); }
template<> std::vector<yocto::vec2i> * LuaXS::GetArgBody<std::vector<yocto::vec2i> * > (lua_State * L, int arg) { return &GetVector2i(L, arg); }
template<> std::vector<yocto::vec3i> * LuaXS::GetArgBody<std::vector<yocto::vec3i> * > (lua_State * L, int arg) { return &GetVector3i(L, arg); }
template<> std::vector<yocto::vec4i> * LuaXS::GetArgBody<std::vector<yocto::vec4i> * > (lua_State * L, int arg) { return &GetVector4i(L, arg); }
template<> std::vector<float> * LuaXS::GetArgBody<std::vector<float> *> (lua_State * L, int arg) { return &GetVector1f(L, arg); }
template<> std::vector<yocto::vec2f> * LuaXS::GetArgBody<std::vector<yocto::vec2f> * > (lua_State * L, int arg) { return &GetVector2f(L, arg); }
template<> std::vector<yocto::vec3f> * LuaXS::GetArgBody<std::vector<yocto::vec3f> * > (lua_State * L, int arg) { return &GetVector3f(L, arg); }
template<> std::vector<yocto::vec4f> * LuaXS::GetArgBody<std::vector<yocto::vec4f> * > (lua_State * L, int arg) { return &GetVector4f(L, arg); }

//
//
//

template<> int GetVectorComponent<int> (lua_State * L, int arg) { return luaL_checkint(L, arg) - 1; }
template<> float GetVectorComponent<float> (lua_State * L, int arg) { return LuaXS::Float(L, arg); }

//
//
//

const int VectorCount<yocto::vec2i>::value = 2;
const int VectorCount<yocto::vec3i>::value = 3;
const int VectorCount<yocto::vec4i>::value = 4;
const int VectorCount<yocto::vec2f>::value = 2;
const int VectorCount<yocto::vec3f>::value = 3;
const int VectorCount<yocto::vec4f>::value = 4;

//
//
//

template<> int GetVector<int> (lua_State * L, int arg) { return GetVectorComponent<int>(L, arg); }
template<> float GetVector<float> (lua_State * L, int arg) { return GetVectorComponent<float>(L, arg); }