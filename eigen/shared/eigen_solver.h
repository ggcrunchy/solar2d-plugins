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

/*****************************
* ComplexEigenSolver methods *
*****************************/
template<typename T, typename R> struct AttachMethods<Eigen::ComplexEigenSolver<T>, R> : SolverMethodsBase<Eigen::ComplexEigenSolver<T>, R> {
    using Getters = InstanceGetters<Eigen::ComplexEigenSolver<T>, R>;

	AttachMethods (lua_State * L)
	{
		luaL_Reg methods[] = {
			{
				EIGEN_MATRIX_GET_MATRIX_METHOD(eigenvalues)
			}, {
				EIGEN_MATRIX_GET_MATRIX_METHOD(eigenvectors)
			}, {
				EIGEN_MATRIX_PUSH_VALUE_METHOD(getMaxIterations)
			}, {
                "info", SolverMethodsBase<Eigen::ComplexEigenSolver<T>, R>::template Info<>
			}, {
				"setMaxIterations", SolverMethodsBase<Eigen::ComplexEigenSolver<T>, R>::template SetMaxIterations<>
			},
			{ nullptr, nullptr }
		};

		luaL_register(L, nullptr, methods);
	}
};

SOLVER_TYPE_NAME(ComplexEigenSolver);

/**********************
* EigenSolver methods *
**********************/
template<typename T, typename R> struct AttachMethods<Eigen::EigenSolver<T>, R> : SolverMethodsBase<Eigen::EigenSolver<T>, R> {
    using Getters = InstanceGetters<Eigen::EigenSolver<T>, R>;
    
    AttachMethods (lua_State * L)
	{
		USING_COMPLEX_TYPE();

		luaL_Reg methods[] = {
			{
				EIGEN_REAL_GET_COMPLEX_METHOD(eigenvalues)
			}, {
				EIGEN_REAL_GET_COMPLEX_METHOD(eigenvectors)
			}, {
				EIGEN_MATRIX_PUSH_VALUE_METHOD(getMaxIterations)
			}, {
				"info", SolverMethodsBase<Eigen::EigenSolver<T>, R>::template Info<>
			}, {
				EIGEN_MATRIX_GET_MATRIX_METHOD(pseudoEigenvalueMatrix)
			}, {
				EIGEN_MATRIX_GET_MATRIX_METHOD(pseudoEigenvectors)
			}, {
				"setMaxIterations", SolverMethodsBase<Eigen::EigenSolver<T>, R>::template SetMaxIterations<>
			},
			{ nullptr, nullptr }
		};

		luaL_register(L, nullptr, methods);
	}
};

SOLVER_TYPE_NAME(EigenSolver);

/*********************************
* GeneralizedEigenSolver methods *
*********************************/
template<typename T, typename R> struct AttachMethods<Eigen::GeneralizedEigenSolver<T>, R> : SolverMethodsBase<Eigen::GeneralizedEigenSolver<T>, R> {
    using Getters = InstanceGetters<Eigen::GeneralizedEigenSolver<T>, R>;
    
    AttachMethods (lua_State * L)
	{
		USING_COMPLEX_TYPE();

		luaL_Reg methods[] = {
			{
				EIGEN_REAL_GET_COMPLEX_METHOD(alphas)
			}, {
				EIGEN_MATRIX_GET_MATRIX_METHOD(betas)
			}, {
				EIGEN_REAL_GET_COMPLEX_METHOD(eigenvalues)
			}, {
				EIGEN_REAL_GET_COMPLEX_METHOD(eigenvectors)
			}, {
                "setMaxIterations", SolverMethodsBase<Eigen::GeneralizedEigenSolver<T>, R>::template SetMaxIterations<>
			},
			{ nullptr, nullptr }
		};

		luaL_register(L, nullptr, methods);
	}
};

SOLVER_TYPE_NAME(GeneralizedEigenSolver);

//
template<typename T, typename R> struct SelfAdjointEigensolverMethodsBase : SolverMethodsBase<T, R> {
    using Getters = InstanceGetters<T, R>;
    
    SelfAdjointEigensolverMethodsBase (lua_State * L)
	{
		luaL_Reg methods[] = {
			{
				EIGEN_MATRIX_GET_MATRIX_METHOD(eigenvalues)
			}, {
				EIGEN_MATRIX_GET_MATRIX_METHOD(eigenvectors)
			}, {
                "info", SolverMethodsBase<T, R>::template Info<>
			}, {
				EIGEN_MATRIX_GET_MATRIX_METHOD(operatorInverseSqrt)
			}, {
				EIGEN_MATRIX_GET_MATRIX_METHOD(operatorSqrt)
			},
			{ nullptr, nullptr }
		};

		luaL_register(L, nullptr, methods);
	}
};

/********************************************
* GeneralizedSelfAdjointEigenSolver methods *
********************************************/
template<typename T, typename R> struct AttachMethods<Eigen::GeneralizedSelfAdjointEigenSolver<T>, R> : SelfAdjointEigensolverMethodsBase<Eigen::GeneralizedSelfAdjointEigenSolver<T>, R> {
	AttachMethods (lua_State * L) : SelfAdjointEigensolverMethodsBase<Eigen::GeneralizedSelfAdjointEigenSolver<T>, R>(L)
	{
	}
};

SOLVER_TYPE_NAME(GeneralizedSelfAdjointEigenSolver);

/*********************************
* SelfAdjointEigenSolver methods *
*********************************/
template<typename T, typename R> struct AttachMethods<Eigen::SelfAdjointEigenSolver<T>, R> : SelfAdjointEigensolverMethodsBase<Eigen::SelfAdjointEigenSolver<T>, R> {
	AttachMethods (lua_State * L) : SelfAdjointEigensolverMethodsBase<Eigen::SelfAdjointEigenSolver<T>, R>(L)
	{
	}
};

SOLVER_TYPE_NAME(SelfAdjointEigenSolver);
