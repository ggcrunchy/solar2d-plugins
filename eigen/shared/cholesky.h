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

// Common Cholesky methods body.
template<typename U, typename R> struct CholeskyMethodsBase : SolverMethodsBase<U, R> {
    using Getters = InstanceGetters<U, R>;
	using Real = typename Eigen::NumTraits<typename R::Scalar>::Real;

	CholeskyMethodsBase (lua_State * L)
	{
		luaL_Reg methods[] = {
			{
				"adjoint", [](lua_State * L)
				{
					return SelfForChaining(L); // already self-adjoint
				}
			}, {
                "info", SolverMethodsBase<U, R>::template Info<>
			}, {
				EIGEN_MATRIX_GET_MATRIX_METHOD(matrixL)
			}, {
				EIGEN_MATRIX_GET_MATRIX_METHOD(matrixU)
			}, {
				"rankUpdate", [](lua_State * L)
				{   
                    Getters::GetT(L)->rankUpdate(*ColumnVector<R>{L, 2}, LuaXS::GetArg<Real>(L, 3));

					return SelfForChaining(L);
				}
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

/***************
* LDLT methods *
***************/
template<typename U, typename R, int UpLo> struct AttachMethods<Eigen::LDLT<U, UpLo>, R> : CholeskyMethodsBase<Eigen::LDLT<U, UpLo>, R> {
    using Getters = InstanceGetters<Eigen::LDLT<U, UpLo>, R>;

	AttachMethods (lua_State * L) : CholeskyMethodsBase<Eigen::LDLT<U, UpLo>, R>(L)
	{
		luaL_Reg methods[] = {
			{
				EIGEN_MATRIX_PUSH_VALUE_METHOD(isNegative)
			}, {
				EIGEN_MATRIX_PUSH_VALUE_METHOD(isPositive)
			}, {
				EIGEN_MATRIX_GET_MATRIX_METHOD(matrixLDLT)
			}, {
				EIGEN_MATRIX_VOID_METHOD(setZero)
			}, {
				"transpositionsP", [](lua_State * L)
				{
					auto td = TypeData<Eigen::MatrixXi>::Get(L);

					luaL_argcheck(L, td, 2, "transpositionsP() requires int matrices");

                    Eigen::MatrixXi im = Getters::GetT(L)->transpositionsP().indices();

					PUSH_TYPED_DATA(im);
				}
			}, {
				EIGEN_MATRIX_GET_MATRIX_METHOD(vectorD)
			},
			{ nullptr, nullptr }
		};

		luaL_register(L, nullptr, methods);
	}
};

SOLVER_TYPE_NAME_EX(LDLT);

/**************
* LLT methods *
**************/
template<typename U, typename R, int UpLo> struct AttachMethods<Eigen::LLT<U, UpLo>, R> : CholeskyMethodsBase<Eigen::LLT<U, UpLo>, R> {
    using Getters = InstanceGetters<Eigen::LLT<U, UpLo>, R>;

	AttachMethods (lua_State * L) : CholeskyMethodsBase<Eigen::LLT<U, UpLo>, R>(L)
	{
		luaL_Reg methods[] = {
			{
				EIGEN_MATRIX_GET_MATRIX_METHOD(matrixLLT)
			},
			{ nullptr, nullptr }
		};

		luaL_register(L, nullptr, methods);
	}
};

SOLVER_TYPE_NAME_EX(LLT);
