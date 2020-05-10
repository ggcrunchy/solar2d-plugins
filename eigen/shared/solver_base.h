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

//
#define SOLVER_TYPE_NAME(TYPE)	template<typename T> struct AuxTypeName<Eigen::TYPE<T>> {	\
									AuxTypeName (luaL_Buffer * B, lua_State * L)			\
									{														\
										luaL_addstring(B, #TYPE "<");						\
																							\
										AuxTypeName<T>(B, L);								\
																							\
										luaL_addstring(B, ">");								\
									}														\
								}

//
#define SOLVER_TYPE_NAME_EX(TYPE)	template<typename T, int EX> struct AuxTypeName<Eigen::TYPE<T, EX>> {	\
										AuxTypeName (luaL_Buffer * B, lua_State * L)						\
										{																	\
											luaL_addstring(B, #TYPE "<");									\
																											\
											AuxTypeName<T>(B, L);											\
																											\
											lua_pushfstring(L, ", %d>", EX); /* ..., ex */					\
											luaL_addvalue(B);	/* ... */									\
										}																	\
									}

//
template<typename T, typename R> struct SolverMethodsBase {
    using Getters = InstanceGetters<T, R>;
    
	//
	template<bool = true> static int Info (lua_State * L) // dummy template parameter as poor man's enable_if
	{
        switch (Getters::GetT(L)->info())
		{
		case Eigen::Success:
			lua_pushliteral(L, "Success");	// ..., "Success"
			break;
		case Eigen::NumericalIssue:
			lua_pushliteral(L, "NumericalIssue");	// ..., "NumericalIssue"
			break;
		default:	// Errors trapped by asserts
			lua_pushliteral(L, "NoConvergence");// ..., "NoConvergence"
		}

		return 1;
	}

	//
	template<bool = true> static int SetMaxIterations (lua_State * L)
	{
        Getters::GetT(L)->setMaxIterations(LuaXS::Int(L, 2));

		return SelfForChaining(L);	// solver, count, solver
	}

	//
	template<bool = true> static int SetThreshold (lua_State * L)
	{
		lua_settop(L, 2);	// solver, ..., how
		lua_pushliteral(L, "Default");	// solver, ..., how, "Default"

        if (!lua_equal(L, 2, 3)) Getters::GetT(L)->setThreshold(LuaXS::GetArg<typename Eigen::NumTraits<typename R::Scalar>::Real>(L, 2));

        else Getters::GetT(L)->setThreshold(Eigen::Default_t{});

		return SelfForChaining(L);	// solver, ..., how, "Default", solver
	}

	template<bool = true> void QRExtensions (lua_State * L)
	{
		luaL_Reg methods[] = {
			{
				EIGEN_MATRIX_PUSH_VALUE_METHOD(isInjective)
			}, {
				EIGEN_MATRIX_PUSH_VALUE_METHOD(isInvertible)
			}, {
				EIGEN_MATRIX_PUSH_VALUE_METHOD(isSurjective)
			}, {
				EIGEN_MATRIX_PUSH_VALUE_METHOD(maxPivot)
			}, {
				EIGEN_MATRIX_PUSH_VALUE_METHOD(nonzeroPivots)
			}, {
				EIGEN_MATRIX_PUSH_VALUE_METHOD(rank)
			}, {
				"setThreshold", SetThreshold<>
			}, {
				EIGEN_MATRIX_PUSH_VALUE_METHOD(threshold)
			},
			{ nullptr, nullptr }
		};

		luaL_register(L, nullptr, methods);
	}
};
