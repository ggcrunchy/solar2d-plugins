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
template<typename U, typename R> struct SVDMethodsBase : SolverMethodsBase<U, R> {
    using Getters = InstanceGetters<U, R>;

	SVDMethodsBase (lua_State * L)
	{
		luaL_Reg methods[] = {
			{
				EIGEN_MATRIX_PUSH_VALUE_METHOD(computeU)
			}, {
				EIGEN_MATRIX_PUSH_VALUE_METHOD(computeV)
			}, {
				EIGEN_MATRIX_GET_MATRIX_METHOD(matrixU)
			}, {
				EIGEN_MATRIX_GET_MATRIX_METHOD(matrixV)
			}, {
				EIGEN_MATRIX_PUSH_VALUE_METHOD(nonzeroSingularValues)
			}, {
				EIGEN_MATRIX_PUSH_VALUE_METHOD(rank)
			}, {
                "setThreshold", SolverMethodsBase<U, R>::template SetThreshold<>
			}, {
				EIGEN_MATRIX_GET_MATRIX_METHOD(singularValues)
			}, {
				EIGEN_MATRIX_GET_MATRIX_MATRIX_PAIR_METHOD(solve)
			}, {
				EIGEN_MATRIX_PUSH_VALUE_METHOD(threshold)
			},
			{ nullptr, nullptr }
		};

		luaL_register(L, nullptr, methods);
	}
};

/*****************
* BDCSVD methods *
*****************/
template<typename U, typename R> struct AttachMethods<Eigen::BDCSVD<U>, R> : SVDMethodsBase<Eigen::BDCSVD<U>, R> {
	AttachMethods (lua_State * L) : SVDMethodsBase<Eigen::BDCSVD<U>, R>(L)
	{
	}
};

SOLVER_TYPE_NAME(BDCSVD);

/********************
* JacobiSVD methods *
********************/
template<typename U, typename R, int QRP> struct AttachMethods<Eigen::JacobiSVD<U, QRP>, R> : SVDMethodsBase<Eigen::JacobiSVD<U, QRP>, R> {
	AttachMethods (lua_State * L) : SVDMethodsBase<Eigen::JacobiSVD<U, QRP>, R>(L)
	{
	}
};

SOLVER_TYPE_NAME_EX(JacobiSVD);
