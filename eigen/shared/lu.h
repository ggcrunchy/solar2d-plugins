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

#include "solver_base.h"

//
template<typename U, typename R> struct LUMethodsBase : SolverMethodsBase<U, R> {
    using Getters = InstanceGetters<U, R>;

	LUMethodsBase (lua_State * L)
	{
		luaL_Reg methods[] = {
			{
				EIGEN_MATRIX_PUSH_VALUE_METHOD(determinant)
			}, {
				EIGEN_MATRIX_GET_MATRIX_METHOD(inverse)
			}, {
				EIGEN_MATRIX_GET_MATRIX_METHOD(matrixLU)
			}, {
				EIGEN_MATRIX_GET_MATRIX_METHOD(permutationP)
			}, {
				EIGEN_MATRIX_PUSH_VALUE_METHOD(rcond)
			}, {
				EIGEN_MATRIX_GET_MATRIX_METHOD(reconstructedMatrix)
			}, {
				EIGEN_MATRIX_GET_MATRIX_MATRIX_PAIR_METHOD(solve)
			},
			{ nullptr, nullptr }
		};

		luaL_register(L, nullptr, methods);
	}
};

/********************
* FullPivLU methods *
********************/
template<typename U, typename R> struct AttachMethods<Eigen::FullPivLU<U>, R> : LUMethodsBase<Eigen::FullPivLU<U>, R> {
    using Getters = InstanceGetters<Eigen::FullPivLU<U>, R>;

	AttachMethods (lua_State * L) : LUMethodsBase<Eigen::FullPivLU<U>, R>(L)
	{
		luaL_Reg methods[] = {
			{
				EIGEN_MATRIX_PUSH_VALUE_METHOD(dimensionOfKernel)
			}, {
				"image", [](lua_State * L)
				{
                    return NewRet<R>(L, Getters::GetT(L)->image(Getters::GetR(L, 2)));
				}
			}, {
				EIGEN_MATRIX_GET_MATRIX_METHOD(kernel)
			}, {
				EIGEN_MATRIX_GET_MATRIX_METHOD(permutationQ)
			},
			{ nullptr, nullptr }
		};

		luaL_register(L, nullptr, methods);

        LUMethodsBase<Eigen::FullPivLU<U>, R>::template QRExtensions(L);
	}
};

SOLVER_TYPE_NAME(FullPivLU);

/***********************
* PartialPivLU methods *
***********************/
template<typename U, typename R> struct AttachMethods<Eigen::PartialPivLU<U>, R> : LUMethodsBase<Eigen::PartialPivLU<U>, R> {
	AttachMethods (lua_State * L) : LUMethodsBase<Eigen::PartialPivLU<U>, R>(L)
	{
	}
};

SOLVER_TYPE_NAME(PartialPivLU);
