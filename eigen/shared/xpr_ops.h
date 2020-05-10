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

#include "types.h"
#include "macros.h"

//
#define NEW_XPR(METHOD, ...)	XprSource<T, R> xprs{L};										\
																								\
								NEW_REF1_DECLTYPE("xpr_from", xprs.mPtr->METHOD(__VA_ARGS__))

#define EIGEN_OBJECT_GET_XPR_COUNT(METHOD)	NEW_XPR(METHOD, LuaXS::Int(L, 2))
#define EIGEN_OBJECT_GET_XPR_COUNT_PAIR(METHOD)	NEW_XPR(METHOD, LuaXS::Int(L, 2), LuaXS::Int(L, 3))
#define EIGEN_OBJECT_GET_XPR_INDEX(METHOD)	NEW_XPR(METHOD, LuaXS::Int(L, 2) - 1)
#define EIGEN_OBJECT_GET_XPR_INDEX_PAIR(METHOD)	NEW_XPR(METHOD, LuaXS::Int(L, 2) - 1, LuaXS::Int(L, 3) - 1)

//
template<typename R> struct VectorRing {
	static const int N = 4;

	ColumnVector<R> mEntries[N];//
	int mIndex{0};	//

	ColumnVector<R> & GetEntry (void)
	{
		auto & entry = mEntries[mIndex++];

		mIndex %= N;

		return entry;
	}
	// ^^^ TODO (and in what follows): these could very well be row vectors
};

//
template<typename T> static ColumnVector<T> & GetVectorFromRing (lua_State * L)
{
	using VRing = VectorRing<T>;

	auto td = TypeData<T>::Get(L);

	//
	if (td->mVectorRingRef == LUA_NOREF)
	{
		LuaXS::NewTyped<VRing>(L);	// ..., vring

		lua_pushliteral(L, "vector_ring:");	// ..., vring, "vector_ring:"
		lua_pushstring(L, td->GetName());	// ..., vring, "vector_ring:", name
		lua_concat(L, 2);	// ..., vring, "vector_ring:" .. name
		lua_insert(L, -2);	// ..., "vector_ring:" .. name, vring

		LuaXS::AttachTypedGC<VRing>(L, lua_tostring(L, -2));

		lua_remove(L, -2);	// ..., vring

		td->mVectorRingRef = lua_ref(L, 1);	// ...
	}

	//
	lua_rawgeti(L, LUA_REGISTRYINDEX, td->mVectorRingRef);	// ..., vring

	auto * pring = LuaXS::UD<VRing>(L, -1);

	lua_pop(L, 1);	// ...

	return pring->GetEntry();
}

#define EIGEN_NEW_VECTOR_BLOCK(METHOD, ...)	ColumnVector<R> & ring_vec = GetVectorFromRing<R>(L);			\
																											\
											ring_vec.Init(L);												\
																											\
											NEW_REF1_DECLTYPE("xpr_from", ring_vec->METHOD(__VA_ARGS__))

#define EIGEN_OBJECT_GET_XPR_COUNT_METHOD(NAME) EIGEN_REG(NAME, EIGEN_OBJECT_GET_XPR_COUNT(NAME))
#define EIGEN_OBJECT_GET_XPR_COUNT_PAIR_METHOD(NAME) EIGEN_REG(NAME, EIGEN_OBJECT_GET_XPR_COUNT_PAIR(NAME))
#define EIGEN_OBJECT_GET_XPR_INDEX_METHOD(NAME)	EIGEN_REG(NAME, EIGEN_OBJECT_GET_XPR_INDEX(NAME))
#define EIGEN_OBJECT_GET_XPR_INDEX_PAIR_METHOD(NAME) EIGEN_REG(NAME, EIGEN_OBJECT_GET_XPR_INDEX_PAIR(NAME))

//
template<typename T, typename R, bool = IsBasic<T>::value> struct XprSource {
    T * mPtr;
    
    XprSource (lua_State * L) : mPtr{InstanceGetters<T, R>::GetT(L)}
    {
    }
};

template<typename T, typename R> struct XprSource<T, R, false> {
    R * mPtr;
    
    XprSource (lua_State * L)
    {
        mPtr = New<R>(L, *InstanceGetters<T, R>::GetT(L));// xpr, ..., new_mat
        
        lua_replace(L, 1);	// new_mat, ...
    }
};
//
template<typename T, typename R> struct XprOps {
	XprOps (lua_State * L)
	{
		luaL_Reg methods[] = {
			{
				"block", [](lua_State * L)
				{
					NEW_XPR(block, LuaXS::Int(L, 2) - 1, LuaXS::Int(L, 3) - 1, LuaXS::Int(L, 4), LuaXS::Int(L, 5));	// mat, x, y, w, h, block
				}
			}, {
				EIGEN_OBJECT_GET_XPR_COUNT_PAIR_METHOD(bottomLeftCorner)
			}, {
				EIGEN_OBJECT_GET_XPR_COUNT_PAIR_METHOD(bottomRightCorner)
			}, {
				EIGEN_OBJECT_GET_XPR_COUNT_METHOD(bottomRows)
			}, {
				EIGEN_OBJECT_GET_XPR_INDEX_METHOD(col)
			}, {
				"diagonal", [](lua_State * L)
				{
					NEW_XPR(diagonal, luaL_optint(L, 2, 0));// mat[, index], diagonal
				}
			}, {
				"head", [](lua_State * L)
				{
					EIGEN_NEW_VECTOR_BLOCK(head, LuaXS::Int(L, 2));	// mat, n, hseg
				}
			}, {
				EIGEN_OBJECT_GET_XPR_COUNT_METHOD(leftCols)
			}, {
				EIGEN_OBJECT_GET_XPR_INDEX_PAIR_METHOD(middleCols)
			}, {
				EIGEN_OBJECT_GET_XPR_INDEX_PAIR_METHOD(middleRows)
			}, {
				EIGEN_OBJECT_GET_XPR_COUNT_METHOD(rightCols)
			}, {
				EIGEN_OBJECT_GET_XPR_INDEX_METHOD(row)
			}, {
				"segment", [](lua_State * L)
				{
					EIGEN_NEW_VECTOR_BLOCK(segment, LuaXS::Int(L, 2) - 1, LuaXS::Int(L, 3));// mat, pos, n, seg
				}
			}, {
				"tail", [](lua_State * L)
				{
					EIGEN_NEW_VECTOR_BLOCK(tail, LuaXS::Int(L, 2));	// mat, n, tseg
				}
			}, {
				EIGEN_OBJECT_GET_XPR_COUNT_PAIR_METHOD(topLeftCorner)
			}, {
				EIGEN_OBJECT_GET_XPR_COUNT_PAIR_METHOD(topRightCorner)
			}, {
				EIGEN_OBJECT_GET_XPR_COUNT_METHOD(topRows)
			},
			{ nullptr, nullptr }
		};

		luaL_register(L, nullptr, methods);
	}
};
