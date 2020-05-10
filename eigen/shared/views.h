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

#include "macros.h"
#include "types.h"
#include "utils.h"
#include "views.h"

// TODO: might replace the functions that produce these by some more lightweight packets and collapse
// to matrices otherwise, since they seem to complicate builds with maps and hunting down all the
// scalar ops seems unpleasant
template<typename ScalarOp> struct ScalarOpName;

#define SCALAR_TYPE_NAME(OP)	template<typename S> struct ScalarOpName<Eigen::internal::scalar_##OP##_op<S>> {	\
									ScalarOpName (luaL_Buffer * B, lua_State * L)									\
									{																				\
										OpenType(B, "scalar_" #OP "_op");                                           \
																													\
										AuxTypeName<S>(B, L);														\
																													\
										CloseType(B);                                                               \
									}																				\
								}

SCALAR_TYPE_NAME(conjugate);
SCALAR_TYPE_NAME(imag);
SCALAR_TYPE_NAME(imag_ref);
SCALAR_TYPE_NAME(real);
SCALAR_TYPE_NAME(real_ref);

template<typename U, typename V> struct AuxTypeName<Eigen::CwiseUnaryOp<U, V>> {
	AuxTypeName (luaL_Buffer * B, lua_State * L)
	{
		OpenType(B, "CwiseUnaryOp");

		ScalarOpName<U> son{B, L};

		luaL_addstring(B, ", ");

		AuxTypeName<V>(B, L);

		CloseType(B);
	}
};

template<typename U, typename V> struct AuxTypeName<Eigen::CwiseUnaryView<U, V>> {
	AuxTypeName (luaL_Buffer * B, lua_State * L)
	{
		luaL_addstring(B, "CwiseUnaryView<");

		ScalarOpName<U> son{B, L};

		luaL_addstring(B, ", ");

		AuxTypeName<V>(B, L);

		luaL_addstring(B, ">");
	}
};
