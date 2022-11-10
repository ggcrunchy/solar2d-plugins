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

#include "yocto/yocto_shape.h"
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

//
//
//

#define NV_PAIR(NAME) Add(#NAME, NAME)

//
//
//

template<typename T> struct BoxOrVector {
	union {
		std::vector<T> mVec;
		std::vector<T> * mVecPtr;
	};
	bool mIsBox;

	//
	//
	//

	BoxOrVector () : mIsBox{false}
	{
		new (&mVec) std::vector<T>;
	}

	BoxOrVector (lua_State * L, int from) : mIsBox{true}
	{
		mVecPtr = nullptr;

		//
		//
		//

		PushWeakValuedParentTable(L); // ..., source, ..., vec, wvpt

		lua_pushvalue(L, -2); // ..., source, ..., vec, wvpt, vec
		lua_pushvalue(L, from < 0 ? from + 3 : from); // ..., source, ..., vec, wvpt, vec, source
		lua_rawset(L, -3); // ..., source, ..., vec, wvpt = { ..., [vec] = source }
		lua_pop(L, 1); // ..., source, ..., vec
	}

	//
	//
	//

	~BoxOrVector ()
	{
		if (!mIsBox) mVec.~vector();
	}

	//
	//
	//

	static std::vector<T> & Get (lua_State * L, int arg, const char * name)
	{
		BoxOrVector * bov = LuaXS::CheckUD<BoxOrVector>(L, arg, name);

		return bov->mIsBox ? *bov->mVecPtr : bov->mVec;
	}
};

//
//
//

template<int N, typename T> int PushEvalResult (lua_State * L, T && v, int tpos = 4)
{
	if (lua_istable(L, tpos)) lua_settop(L, tpos); // shape, element[, uv], out
	else lua_createtable(L, N, 0); // shape, element[, uv], out

	for (int i = 0; i < N; ++i)
	{
		lua_pushnumber(L, v[i]); // shape, element[, uv], out, component
		lua_rawseti(L, -2, i + 1); // shape, element[, uv], out = { ..., component }
	}

	return 1;
}

//
//
//

template<typename T> T GetVectorComponent (lua_State * L, int arg = -1);

//
//
//

template<typename T> struct VectorCount { static const int value; };

//
//
//

template<typename T> T GetVector (lua_State * L, int arg)
{
	T v;

	if (lua_istable(L, arg))
	{
		for (int i = 1; i <= VectorCount<T>::value; ++i, lua_pop(L, 1))
		{
			lua_rawgeti(L, arg, i); // ..., t, ..., v

			v[i - 1] = GetVectorComponent<decltype(v.x)>(L);
		}
	}

	else
	{
		arg = CoronaLuaNormalize(L, arg);

		for (int i = 0; i < VectorCount<T>::value; ++i, lua_pop(L, 1)) v[i - 1] = GetVectorComponent<decltype(v.x)>(L, arg + i);
	}

	return v;
}

template<> int GetVector<int> (lua_State *, int);
template<> float GetVector<float> (lua_State *, int);

//
//
//

template<typename T, std::vector<T> & (*getter)(lua_State *, int)> int AppendToVector (lua_State * L)
{
	getter(L, 1).push_back(GetVector<T>(L, 2));

	return 0;
}

//
//
//

template<typename T, std::vector<T> & (*getter)(lua_State *, int)> int GetLength (lua_State * L)
{
	lua_pushinteger(L, getter(L, 1).size()); // v, length

	return 1;
}

//
//
//

static void PushComponent (lua_State * L, float f)
{
	lua_pushnumber(L, f); // ..., v
}

static void PushComponent (lua_State * L, int i)
{
	lua_pushinteger(L, i + 1); // ..., v
}

template<typename T, std::vector<T> & (*getter)(lua_State *, int)> int GetValue (lua_State * L)
{
	auto & vec = getter(L, 1);
	int index = luaL_checkint(L, 2) - 1;

	if constexpr (!std::is_same_v<T, int> && !std::is_same_v<T, float>)
	{
		const T & item = vec[index];

		for (int i = 0; i < VectorCount<T>::value; ++i) PushComponent(L, item[i]); // ..., v1, ...

		return VectorCount<T>::value;
	}

	else
	{
		PushComponent(L, vec[index]); // ..., v

		return 1;
	}
}

//
//
//

template<typename T, std::vector<T> & (*getter)(lua_State *, int)> int UpdateVector (lua_State * L)
{
	auto & vec = getter(L, 1);
	int index = luaL_checkint(L, 2) - 1;

	if (index >= 0 && size_t(index) < vec.size()) vec[index] = GetVector<T>(L, 3);

	return 0;
}

//
//
//

void PushWeakValuedParentTable (lua_State * L);

//
//
//

void SetComponentCount (lua_State * L, int count);
int GetComponentCount (lua_State * L, int arg);

//
//
//

int PushStrings (lua_State * L, std::vector<std::string> && strs);

//
//
//

int WrapShapeData (lua_State * L, yocto::shape_data && sd);
int WrapFVShapeData (lua_State * L, yocto::fvshape_data && sd);

yocto::shape_data & GetShapeData (lua_State * L, int arg = 1);
yocto::fvshape_data & GetFVShapeData (lua_State * L, int arg = 1);

//
//
//

int WrapVector1i (lua_State * L, std::vector<int> && v);
int WrapVector2i (lua_State * L, std::vector<yocto::vec2i> && v);
int WrapVector3i (lua_State * L, std::vector<yocto::vec3i> && v);
int WrapVector4i (lua_State * L, std::vector<yocto::vec4i> && v);

int RefVector1i (lua_State * L, std::vector<int> * v, int from = 1);
int RefVector2i (lua_State * L, std::vector<yocto::vec2i> * v, int from = 1);
int RefVector3i (lua_State * L, std::vector<yocto::vec3i> * v, int from = 1);
int RefVector4i (lua_State * L, std::vector<yocto::vec4i> * v, int from = 1);

std::vector<int> & GetVector1i (lua_State * L, int arg = 1);
std::vector<yocto::vec2i> & GetVector2i (lua_State * L, int arg = 1);
std::vector<yocto::vec3i> & GetVector3i (lua_State * L, int arg = 1);
std::vector<yocto::vec4i> & GetVector4i (lua_State * L, int arg = 1);

//
//
//

int WrapVector1f (lua_State * L, std::vector<float> && v);
int WrapVector2f (lua_State * L, std::vector<yocto::vec2f> && v);
int WrapVector3f (lua_State * L, std::vector<yocto::vec3f> && v);
int WrapVector4f (lua_State * L, std::vector<yocto::vec4f> && v);

int RefVector1f (lua_State * L, std::vector<float> * v, int from = 1);
int RefVector2f (lua_State * L, std::vector<yocto::vec2f> * v, int from = 1);
int RefVector3f (lua_State * L, std::vector<yocto::vec3f> * v, int from = 1);
int RefVector4f (lua_State * L, std::vector<yocto::vec4f> * v, int from = 1);

std::vector<float> & GetVector1f (lua_State * L, int arg = 1);
std::vector<yocto::vec2f> & GetVector2f (lua_State * L, int arg = 1);
std::vector<yocto::vec3f> & GetVector3f (lua_State * L, int arg = 1);
std::vector<yocto::vec4f> & GetVector4f (lua_State * L, int arg = 1);