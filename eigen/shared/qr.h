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
template<typename U, typename R> struct QRMethodsBase : SolverMethodsBase<U, R> {
    using Getters = InstanceGetters<U, R>;

	QRMethodsBase (lua_State * L)
	{
		luaL_Reg methods[] = {
			{
				EIGEN_MATRIX_PUSH_VALUE_METHOD(absDeterminant)
			}, {
				EIGEN_MATRIX_PUSH_VALUE_METHOD(logAbsDeterminant)
			}, {
				EIGEN_MATRIX_GET_MATRIX_MATRIX_PAIR_METHOD(solve)
			},
			{ nullptr, nullptr }
		};

		luaL_register(L, nullptr, methods);
	}
};

/************************
* HouseholderQR methods *
************************/
template<typename U, typename R> struct AttachMethods<Eigen::HouseholderQR<U>, R> : QRMethodsBase<Eigen::HouseholderQR<U>, R> {
    using Getters = InstanceGetters<Eigen::HouseholderQR<U>, R>;

	AttachMethods (lua_State * L) : QRMethodsBase<Eigen::HouseholderQR<U>, R>(L)
	{
		luaL_Reg methods[] = {
			{
				EIGEN_MATRIX_GET_MATRIX_METHOD(householderQ)
			}, {
				EIGEN_MATRIX_GET_MATRIX_METHOD(matrixQR)
			},
			{ nullptr, nullptr }
		};

		luaL_register(L, nullptr, methods);
	}
};

SOLVER_TYPE_NAME(HouseholderQR);

//
template<typename U, typename R> struct QRExMethodsBase : QRMethodsBase<U, R> {
	QRExMethodsBase (lua_State * L) : QRMethodsBase<U, R>(L)
	{
        QRMethodsBase<U, R>::template QRExtensions(L);
	}
};

/******************************
* ColPivHouseholderQR methods *
******************************/
template<typename U, typename R> struct AttachMethods<Eigen::ColPivHouseholderQR<U>, R> : QRExMethodsBase<Eigen::ColPivHouseholderQR<U>, R> {
    using Getters = InstanceGetters<Eigen::ColPivHouseholderQR<U>, R>;

	AttachMethods (lua_State * L) : QRExMethodsBase<Eigen::ColPivHouseholderQR<U>, R>(L)
	{
		luaL_Reg methods[] = {
			{
				EIGEN_MATRIX_GET_MATRIX_METHOD(colsPermutation)
			}, {
				EIGEN_MATRIX_GET_MATRIX_METHOD(inverse)
			}, {
				EIGEN_MATRIX_GET_MATRIX_METHOD(matrixQR)
			}, {
				EIGEN_MATRIX_GET_MATRIX_METHOD(matrixR)
			},
			{ nullptr, nullptr }
		};

		luaL_register(L, nullptr, methods);
	}
};

SOLVER_TYPE_NAME(ColPivHouseholderQR);

/******************************************
* CompleteOrthogonalDecomposition methods *
******************************************/
template<typename U, typename R> struct AttachMethods<Eigen::CompleteOrthogonalDecomposition<U>, R> : QRExMethodsBase<Eigen::CompleteOrthogonalDecomposition<U>, R> {
    using Getters = InstanceGetters<Eigen::CompleteOrthogonalDecomposition<U>, R>;

	AttachMethods (lua_State * L) : QRExMethodsBase<Eigen::CompleteOrthogonalDecomposition<U>, R>(L)
	{
		luaL_Reg methods[] = {
			{
				EIGEN_MATRIX_GET_MATRIX_METHOD(householderQ)
			}, {
                "info", QRExMethodsBase<Eigen::CompleteOrthogonalDecomposition<U>, R>::template Info<>
			}, {
				EIGEN_MATRIX_GET_MATRIX_METHOD(matrixQTZ)
			}, {
				EIGEN_MATRIX_GET_MATRIX_METHOD(matrixT)
			}, {
				EIGEN_MATRIX_GET_MATRIX_METHOD(matrixZ)
			}, {
				EIGEN_MATRIX_GET_MATRIX_METHOD(pseudoInverse)
			},
			{ nullptr, nullptr }
		};

		luaL_register(L, nullptr, methods);
	}
};

SOLVER_TYPE_NAME(CompleteOrthogonalDecomposition);

/*******************************
* FullPivHouseholderQR methods *
*******************************/
template<typename U, typename R> struct AttachMethods<Eigen::FullPivHouseholderQR<U>, R> : QRExMethodsBase<Eigen::FullPivHouseholderQR<U>, R> {
    using Getters = InstanceGetters<Eigen::FullPivHouseholderQR<U>, R>;

	AttachMethods (lua_State * L) : QRExMethodsBase<Eigen::FullPivHouseholderQR<U>, R>(L)
	{
		luaL_Reg methods[] = {
			{
				EIGEN_MATRIX_GET_MATRIX_METHOD(inverse)
			}, {
				EIGEN_MATRIX_GET_MATRIX_METHOD(matrixQ)
			}, {
				EIGEN_MATRIX_GET_MATRIX_METHOD(matrixQR)
			}, {
				"rowsTranspositions", [](lua_State * L)
				{
					auto td = TypeData<Eigen::MatrixXi>::Get(L);

					luaL_argcheck(L, td, 2, "rowsTranspositions() requires int matrices");

                    Eigen::MatrixXi im = Getters::GetT(L)->rowsTranspositions().template cast<int>();

					PUSH_TYPED_DATA(im);
				}
			},
			{ nullptr, nullptr }
		};

		luaL_register(L, nullptr, methods);
	}
};

SOLVER_TYPE_NAME(FullPivHouseholderQR);
