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
template<typename U, typename R> struct SchurMethodsBase : SolverMethodsBase<U, R> {
    using Getters = InstanceGetters<U, R>;

	SchurMethodsBase (lua_State * L)
	{
		luaL_Reg methods[] = {
			{
				EIGEN_MATRIX_PUSH_VALUE_METHOD(getMaxIterations)
			}, {
                "info", SolverMethodsBase<U, R>::template Info<>
			}, {
				EIGEN_MATRIX_GET_MATRIX_METHOD(matrixT)
			}, {
				EIGEN_MATRIX_GET_MATRIX_METHOD(matrixU)
			}, {
                "setMaxIterations", SolverMethodsBase<U, R>::template SetMaxIterations<>
			},
			{ nullptr, nullptr }
		};

		luaL_register(L, nullptr, methods);
	}
};

/***********************
* ComplexSchur methods *
***********************/
template<typename U, typename R> struct AttachMethods<Eigen::ComplexSchur<U>, R> : SchurMethodsBase<Eigen::ComplexSchur<U>, R> {
	AttachMethods (lua_State * L) : SchurMethodsBase<Eigen::ComplexSchur<U>, R>(L)
	{
	}
};

SOLVER_TYPE_NAME(ComplexSchur);

/********************
* RealSchur methods *
********************/
template<typename U, typename R> struct AttachMethods<Eigen::RealSchur<U>, R> : SchurMethodsBase<Eigen::RealSchur<U>, R> {
	AttachMethods (lua_State * L) : SchurMethodsBase<Eigen::RealSchur<U>, R>(L)
	{
	}
};

SOLVER_TYPE_NAME(RealSchur);

/**********************************
* HessenburgDecomposition methods *
 **********************************/

//
template<typename T, typename R, bool = Eigen::NumTraits<typename R::Scalar>::IsComplex> struct MakeSchur {
    static int Do (lua_State * L)
    {
        T & hd = *InstanceGetters<T, R>::GetT(L);
        Eigen::ComplexSchur<R> schur;
    
        schur.computeFromHessenberg(hd.matrixH(), hd.matrixQ(), !WantsBool(L, "NoU"));
    
        return NewRet<Eigen::ComplexSchur<R>>(L, std::move(schur));
    }
};

//
template<typename T, typename R> struct MakeSchur<T, R, false> {
    static int Do (lua_State * L)
    {
        auto & hd = *InstanceGetters<T, R>::GetT(L);
        Eigen::RealSchur<R> schur;
    
        schur.computeFromHessenberg(hd.matrixH(), hd.matrixQ(), !WantsBool(L, "NoU"));
    
        return NewRet<Eigen::RealSchur<R>>(L, std::move(schur));
    }
};

template<typename U, typename R> struct AttachMethods<Eigen::HessenbergDecomposition<U>, R> : SolverMethodsBase<Eigen::HessenbergDecomposition<U>, R> {
	using T = Eigen::HessenbergDecomposition<U>;
    using Getters = InstanceGetters<T, R>;

	AttachMethods (lua_State * L)
	{
		luaL_Reg methods[] = {
			{
				EIGEN_MATRIX_GET_MATRIX_METHOD(householderCoefficients)
			}, {
				EIGEN_MATRIX_GET_MATRIX_METHOD(matrixH)
			}, {
				EIGEN_MATRIX_GET_MATRIX_METHOD(matrixQ)
			}, {
				EIGEN_MATRIX_GET_MATRIX_METHOD(packedMatrix)
			}, {
                "schur", MakeSchur<T, R>::Do
			},
			{ nullptr, nullptr }
		};

		luaL_register(L, nullptr, methods);
	}
};

SOLVER_TYPE_NAME(HessenbergDecomposition);

/*****************
* RealQZ methods *
*****************/
template<typename U, typename R> struct AttachMethods<Eigen::RealQZ<U>, R> : SolverMethodsBase<Eigen::RealQZ<U>, R> {
    using Getters = InstanceGetters<Eigen::RealQZ<U>, R>;

	AttachMethods (lua_State * L)
	{
		luaL_Reg methods[] = {
			{
                "info", SolverMethodsBase<Eigen::RealQZ<U>, R>::template Info<>
			}, {
				EIGEN_MATRIX_PUSH_VALUE_METHOD(iterations)
			}, {
				EIGEN_MATRIX_GET_MATRIX_METHOD(matrixQ)
			}, {
				EIGEN_MATRIX_GET_MATRIX_METHOD(matrixS)
			}, {
				EIGEN_MATRIX_GET_MATRIX_METHOD(matrixT)
			}, {
				EIGEN_MATRIX_GET_MATRIX_METHOD(matrixZ)
			}, {
                "setMaxIterations", SolverMethodsBase<Eigen::RealQZ<U>, R>::template SetMaxIterations<>
			},
			{ nullptr, nullptr }
		};

		luaL_register(L, nullptr, methods);
	}
};

SOLVER_TYPE_NAME(RealQZ);

/*****************************
* Tridiagonalization methods *
*****************************/
template<typename U, typename R> struct AttachMethods<Eigen::Tridiagonalization<U>, R> : SolverMethodsBase<Eigen::Tridiagonalization<U>, R> {
    using Getters = InstanceGetters<Eigen::Tridiagonalization<U>, R>;

	AttachMethods (lua_State * L)
	{
		luaL_Reg methods[] = {
			{
				EIGEN_MATRIX_GET_MATRIX_METHOD(diagonal)
			}, {
				"generalizedSelfAdjointEigenSolver", [](lua_State * L)
				{
                    auto & tri = *Getters::GetT(L);
					Eigen::GeneralizedSelfAdjointEigenSolver<R> gsaes;

					gsaes.computeFromTridiagonal(tri.diagonal(), tri.subDiagonal(), WantsBool(L, "NoEigenvectors") ? Eigen::EigenvaluesOnly : Eigen::ComputeEigenvectors);

					return NewRet<Eigen::GeneralizedSelfAdjointEigenSolver<R>>(L, std::move(gsaes));
				}
			}, {
				EIGEN_MATRIX_GET_MATRIX_METHOD(householderCoefficients)
			}, {
				EIGEN_MATRIX_GET_MATRIX_METHOD(matrixQ)
			}, {
				EIGEN_MATRIX_GET_MATRIX_METHOD(matrixT)
			}, {
				EIGEN_MATRIX_GET_MATRIX_METHOD(packedMatrix)
			}, {
				"selfAdjointEigenSolver", [](lua_State * L)
				{
                    auto & tri = *Getters::GetT(L);
					Eigen::SelfAdjointEigenSolver<R> saes;

					saes.computeFromTridiagonal(tri.diagonal(), tri.subDiagonal(), WantsBool(L, "NoEigenvectors") ? Eigen::EigenvaluesOnly : Eigen::ComputeEigenvectors);

					return NewRet<Eigen::SelfAdjointEigenSolver<R>>(L, std::move(saes));
				}
			}, {
				EIGEN_MATRIX_GET_MATRIX_METHOD(subDiagonal)
			},
			{ nullptr, nullptr }
		};

		luaL_register(L, nullptr, methods);
	}
};

SOLVER_TYPE_NAME(Tridiagonalization);
