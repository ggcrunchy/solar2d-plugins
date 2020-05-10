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
#include "utils.h"
#include "arith_ops.h"
#include "write_ops.h"
#include "xpr_ops.h"

//
template<typename T, typename R> struct AttachBlockMethods {
	//
	AttachBlockMethods (lua_State * L)
	{
		LuaXS::MethodThunksProperty(L, [](lua_State * L) {
			New<R>(L);	// ..., meta, temp
		}, [](lua_State * L, const int ti) {
			*LuaXS::UD<R>(L, ti) = *GetInstance<T>(L);
		});

		luaL_Reg methods[] = {
			{
				"asMatrix", AsMatrix<T, R>
			}, {
				"__tostring", [](lua_State * L)
				{
					AsMatrix<T, R>(L);	// xpr, mat

                    return Print(L, InstanceGetters<T, R>::GetR(L, 2));
				}
			},
			{ nullptr, nullptr }
		};

		luaL_register(L, nullptr, methods);

		ArithOps<T, R> ao{L};
		WriteOps<T, R> wo{L};
		XprOps<T, R> xo{L}; // does special handling to avoid either xprs of xprs or xprs of a common temporary
	}
};

/*******************
* Diagonal methods *
*******************/
template<typename U, int I, typename R> struct AttachMethods<Eigen::Diagonal<U, I>, R> : AttachBlockMethods<Eigen::Diagonal<U, I>, R> {
	AttachMethods (lua_State * L) : AttachBlockMethods<Eigen::Diagonal<U, I>, R>(L)
	{
	}
};

template<typename U, int I> struct AuxTypeName<Eigen::Diagonal<U, I>> {
	AuxTypeName (luaL_Buffer * B, lua_State * L)
	{
		OpenType(B, "Diagonal");

		AuxTypeName<U>(B, L);

		CloseType(B);
	}
};

/********************
* Transpose methods *
********************/
template<typename U, typename R> struct AttachMethods<Eigen::Transpose<U>, R> : AttachBlockMethods<Eigen::Transpose<U>, R> {
	AttachMethods (lua_State * L) : AttachBlockMethods<Eigen::Transpose<U>, R>(L)
	{
		luaL_Reg methods[] = {
			{
				"transpose", Transposer<Eigen::Transpose<U>>::Do
			},
			{ nullptr, nullptr }
		};

		luaL_register(L, nullptr, methods);
	}
};

// Name handled by general case, q.v. types.h

/**********************
* VectorBlock methods *
**********************/
template<typename U, typename R> struct AttachMethods<Eigen::VectorBlock<U>, R> : AttachBlockMethods<Eigen::VectorBlock<U>, R> {
	AttachMethods (lua_State * L) : AttachBlockMethods<Eigen::VectorBlock<U>, R>(L)
	{
	}
};

template<typename U, int Size> struct AuxTypeName<Eigen::VectorBlock<U, Size>> {
	AuxTypeName (luaL_Buffer * B, lua_State * L)
	{
		OpenType(B, "VectorBlock");

		AuxTypeName<U>(B, L);

		AddComma(B);
		AddDynamicOrN(B, L, Size);
		CloseType(B);
	}
};
