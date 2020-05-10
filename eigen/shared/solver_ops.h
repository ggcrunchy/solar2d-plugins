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
#include "macros.h"
#include "solvers.h"

// Conditionally add certain solvers according to whether the matrix is complex.
template<typename T, typename R, bool = Eigen::NumTraits<typename T::Scalar>::IsComplex> struct IgnoreWhenComplex {
    using Getters = InstanceGetters<T, R>;

    IgnoreWhenComplex (lua_State * L)
    {
        typedef typename Getters::RefType RefType; // Visual Studio workaround

        luaL_Reg methods[] = {
            {
                "eigenSolver", [](lua_State * L)
                {
                    return Getters::WithRef(L, [L](const RefType & ref) {
                        NewRvalue<Eigen::ComplexEigenSolver<R>>(L, ref, !WantsBool(L, "NoEigenvectors"));
                    });
                }
            }, {
                "schur", [](lua_State * L)
                {
                    return Getters::WithRef(L, [L](const RefType & ref) {
                        NewRvalue<Eigen::ComplexSchur<R>>(L, ref, !WantsBool(L, "NoU"));
                    });
                }
            },
            { nullptr, nullptr }
        };
    
        luaL_register(L, nullptr, methods);
    }
};

template<typename T, typename R> struct IgnoreWhenComplex<T, R, false> {
    using Getters = InstanceGetters<T, R>;
    
    IgnoreWhenComplex (lua_State * L)
    {
        typedef typename Getters::RefType RefType; // Visual Studio workaround

        luaL_Reg methods[] = {
            {
                "eigenSolver", [](lua_State * L)
                {
                    return Getters::WithRef(L, [L](const RefType & ref) {
                        NewRvalue<Eigen::EigenSolver<R>>(L, ref, !WantsBool(L, "NoEigenvectors"));
                    });
                }
            }, {
                "generalizedEigenSolver", [](lua_State * L)
                {
                    return Getters::WithRef(L, [L](const RefType & ref) {
                        NewRvalue<Eigen::GeneralizedEigenSolver<R>>(L, ref, Getters::GetR(L, 2), !WantsBool(L, "NoEigenvectors"));
                    });
                }
            }, {
                "realQz", [](lua_State * L)
                {
                    return Getters::WithRef(L, [L](const RefType & ref) {
                        NewRvalue<Eigen::RealQZ<R>>(L, ref, Getters::GetR(L, 2), !WantsBool(L, "NoQZ"));
                    });
                }
            }, {
                "schur", [](lua_State * L)
                {
                    return Getters::WithRef(L, [L](const RefType & ref) {
                        NewRvalue<Eigen::RealSchur<R>>(L, ref, !WantsBool(L, "NoU"));
                    });
                }
            },
            { nullptr, nullptr }
        };
    
        luaL_register(L, nullptr, methods);
    }
};

// Methods assigned to matrices with non-integer types.
template<typename T, typename R, bool = !Eigen::NumTraits<typename T::Scalar>::IsInteger> struct SolverOps {
    using Getters = InstanceGetters<T, R>;

	// Helper to supply options to SVD solvers.
	static unsigned int GetOpts (lua_State * L)
	{
		const char * names[] = { "FullU", "ThinU", "FullV", "ThinV", nullptr };
		int flags[] = { Eigen::ComputeFullU, Eigen::ComputeThinU, Eigen::ComputeFullV, Eigen::ComputeThinV };
		unsigned int opts = 0;

		for (size_t i = 1, n = lua_objlen(L, 2); i <= n; ++i, lua_pop(L, 1))
		{
			lua_rawgeti(L, 2, int(i));	// mat, t, flag

			opts |= flags[luaL_checkoption(L, 3, nullptr, names)];
		}

		return opts;
	}

	SolverOps (lua_State * L)
	{
        typedef typename Getters::RefType RefType; // Visual Studio workaround
        
        luaL_Reg methods[] = {
			{
				"bdcSvd", [](lua_State * L)
				{
					return Getters::WithRef(L, [L](const RefType & ref) {
						New<Eigen::BDCSVD<R>>(L, ref.bdcSvd(lua_istable(L, 2) ? GetOpts(L) : 0U));
					});
				}
			}, {
				"colPivHouseholderQr", [](lua_State * L)
				{
					return Getters::WithRef(L, [L](const RefType & ref) {
						New<Eigen::ColPivHouseholderQR<R>>(L, ref.colPivHouseholderQr());
					});
				}
			}, {
				"completeOrthogonalDecomposition", [](lua_State * L)
				{
					return Getters::WithRef(L, [L](const RefType & ref) {
						New<Eigen::CompleteOrthogonalDecomposition<R>>(L, ref.completeOrthogonalDecomposition());
					});
				}
			}, {
				EIGEN_MATRIX_PUSH_VALUE_METHOD(determinant)
			}, {
				"fullPivHouseholderQr", [](lua_State * L)
				{
					return Getters::WithRef(L, [L](const RefType & ref) {
						New<Eigen::FullPivHouseholderQR<R>>(L, ref.fullPivHouseholderQr());
					});
				}
			}, {
				"fullPivLu", [](lua_State * L)
				{
					return Getters::WithRef(L, [L](const RefType & ref) {
						New<Eigen::FullPivLU<R>>(L, ref.fullPivLu());
					});
				}
			}, {
				"generalizedSelfAdjointEigenSolver", [](lua_State * L)
				{
					return Getters::WithRef(L, [L](const RefType & ref) {
						auto compute = Eigen::ComputeEigenvectors;
						auto method = Eigen::Ax_lBx;

						if (lua_istable(L, 3))
						{
							lua_getfield(L, 3, "no_eigenvectors");	// a, b, opts, no_eigenvectors?

							if (lua_toboolean(L, -1)) compute = Eigen::EigenvaluesOnly;

							lua_getfield(L, 3, "method");	// a, b, opts, no_eigenvectors?, method

							const char * names[] = { "ABx_lx", "Ax_lBx", "BAx_lx", nullptr };
							decltype(method) methods[] = { Eigen::Ax_lBx, Eigen::Ax_lBx, Eigen::BAx_lx };

							method = methods[luaL_checkoption(L, -1, "", names)];
						}

                        NewRvalue<Eigen::GeneralizedSelfAdjointEigenSolver<R>>(L, ref, Getters::GetR(L, 2), compute | method);
					});
				}
			}, {
				"hessenbergDecomposition", [](lua_State * L)
				{
					return Getters::WithRef(L, [L](const RefType & ref) {
						NewRvalue<Eigen::HessenbergDecomposition<R>>(L, ref);
					});
				}
			}, {
				"householderQr", [](lua_State * L)
				{
					return Getters::WithRef(L, [L](const RefType & ref) {
                        New<Eigen::HouseholderQR<R>>(L, ref.householderQr());
					});
				}
			}, {
				EIGEN_MATRIX_GET_MATRIX_METHOD(inverse)
			}, {
				"jacobiSvd", [](lua_State * L)
				{
					return Getters::WithRef(L, [L](const RefType & ref) {
						unsigned int opts = 0U;

						if (lua_istable(L, 2))
						{
							opts = GetOpts(L);

							lua_getfield(L, 2, "preconditioner");	// mat, opts, precond
							lua_replace(L, 2);	// mat, precond
						}

						const char * choices[] = { "", "fullPiv", "householder", "none", nullptr };

						switch (luaL_checkoption(L, 2, "", choices))
						{
						case 0:
							return NewRet<Eigen::JacobiSVD<R>>(L, ref.jacobiSvd(opts));
						case 1:
							return NewRvalue<Eigen::JacobiSVD<R, Eigen::FullPivHouseholderQRPreconditioner>>(L, ref, opts);
						case 2:
							return NewRvalue<Eigen::JacobiSVD<R, Eigen::HouseholderQRPreconditioner>>(L, ref, opts);
						default: // luaL_checkoption will catch anything else
							return NewRvalue<Eigen::JacobiSVD<R, Eigen::NoQRPreconditioner>>(L, ref, opts);
						}
					});
				}
			}, {
				"ldlt", [](lua_State * L)
				{
					lua_settop(L, 2);	// mat, how?
					lua_pushliteral(L, "upper");// mat[, how], "upper"

					return Getters::WithRef(L, [L](const RefType & ref) {
						if (!lua_equal(L, 2, 3)) New<Eigen::LDLT<R, Eigen::Lower>>(L, ref.ldlt());
						else NewRvalue<Eigen::LDLT<R, Eigen::Upper>>(L, ref);
					});
				}
			}, {
				"llt", [](lua_State * L)
				{
					lua_settop(L, 2);	// mat, how?
					lua_pushliteral(L, "upper");// mat[, how], "upper"

					return Getters::WithRef(L, [L](const RefType & ref) {
						if (!lua_equal(L, 2, 3)) New<Eigen::LLT<R, Eigen::Lower>>(L, ref.llt());
						else NewRvalue<Eigen::LLT<R, Eigen::Upper>>(L, ref);
					});
				}
			}, {
				"partialPivLu", [](lua_State * L)
				{
					return Getters::WithRef(L, [L](const RefType & ref) {
						New<Eigen::PartialPivLU<R>>(L, ref.partialPivLu());
					});
				}
			}, {
				EIGEN_MATRIX_PUSH_VALUE_METHOD(operatorNorm)
			}, {
				"selfAdjointEigenSolver", [](lua_State * L)
				{
					return Getters::WithRef(L, [L](const RefType & ref) {
						auto opts = WantsBool(L, "NoEigenvectors") ? Eigen::EigenvaluesOnly : Eigen::ComputeEigenvectors;

						NewRvalue<Eigen::SelfAdjointEigenSolver<R>>(L, ref, opts);
					});
				}
			}, {
				"tridiagonalization", [](lua_State * L)
				{
					return Getters::WithRef(L, [L](const RefType & ref) {
						NewRvalue<Eigen::Tridiagonalization<R>>(L, ref);
					});
				}
			},
			{ nullptr, nullptr }
		};

		luaL_register(L, nullptr, methods);
		lua_getfield(L, -1, "partialPivLu");// methods, pplu
		lua_setfield(L, -2, "lu");	// methods = { lu = pplu }

        IgnoreWhenComplex<T, R> iwc{L};
	}
};

// No-op for integer types.
template<typename T, typename R> struct SolverOps<T, R, false> {
	SolverOps (lua_State *) {}
};
