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

#include "CoronaLua.h"
#include "types.h"
#include "utils.h"
#include "macros.h"
#include "xprs.h"
#include "arith_ops.h"
#include "real_ops.h"
#include "solver_ops.h"
#include "stock_ops.h"
#include "unary_view.h"
#include "write_ops.h"
#include "xpr_ops.h"

namespace detail_matrix {
    // Helper to cast the matrix to another type, which may be in another shared library.
    template<typename T, typename R, typename U, bool = Eigen::NumTraits<typename T::Scalar>::IsComplex && !Eigen::NumTraits<U>::IsComplex> struct CastTo {
        static MatrixOf<U> Do (lua_State * L)
        {
            return InstanceGetters<T, R>::GetT(L)->real().template cast<U>();
        }
    };
    
    template<typename T, typename R, typename U> struct CastTo<T, R, U, false> {
        static MatrixOf<U> Do (lua_State * L)
        {
            return InstanceGetters<T, R>::GetT(L)->template cast<U>();
        }
    };
    
    template<typename T, typename R, typename U> struct Cast {
		using MT = MatrixOf<U>;

		Cast (lua_State * L)
		{
			auto td = TypeData<MT>::Get(L, GetTypeData::eFetchIfMissing);

			luaL_argcheck(L, td, 2, "Matrix type unavailable for cast");

            MT m = CastTo<T, R, U>::Do(L);

			if (td->mDatum) *static_cast<MT *>(td->mDatum) = m;

			else
			{
				PUSH_TYPED_DATA_NO_RET(m);
			}
		}
	};

	// Helper to query the precision used for comparing matrices from the stack.
	template<typename T> static typename Eigen::NumTraits<typename T::Scalar>::Real GetPrecision (lua_State * L, int arg)
	{
		auto prec = Eigen::NumTraits<typename T::Scalar>::dummy_precision();

		return !lua_isnoneornil(L, arg) ? LuaXS::GetArg<decltype(prec)>(L, arg) : prec;
	}

	// 
	template<typename T, typename R, bool = IsBasic<T>::value, bool = Eigen::NumTraits<typename T::Scalar>::IsComplex> struct AddComplexComponentViews {
		AddComplexComponentViews (lua_State * L)
		{
			luaL_Reg methods[] = {
				{
					"imag", [](lua_State * L)
					{
						return NewRet<R>(L, R{});
					}
				}, {
					"real", AsMatrix<T, R>
				},
				{ nullptr, nullptr }
			};

			luaL_register(L, nullptr, methods);
		}
	};

	// 
	template<typename T, typename R> struct AddComplexComponentViews<T, R, true, true> {
		using Getters = InstanceGetters<T, R>;

		AddComplexComponentViews (lua_State * L)
		{
			luaL_Reg methods[] = {
				{
					EIGEN_PUSH_AUTO_RESULT_METHOD(imag)
				}, {
					EIGEN_PUSH_AUTO_RESULT_METHOD(real)
				},
				{ nullptr, nullptr }
			};

			luaL_register(L, nullptr, methods);
		}
	};

	template<typename T, typename R> struct AddComplexComponentViews<T, R, false, true> {
		using Getters = InstanceGetters<T, R>;

		AddComplexComponentViews (lua_State * L)
		{
			using Real = typename Eigen::NumTraits<typename T::Scalar>::Real;

			luaL_Reg methods[] = {
				{
					"imag", [](lua_State * L)
					{
						auto td = TypeData<MatrixOf<Real>>::Get(L, GetTypeData::eFetchIfMissing);

						luaL_argcheck(L, td, 2, "imag() requires real matrices");

						MatrixOf<Real> imag = Getters::GetT(L)->imag();

						PUSH_TYPED_DATA(imag);
					}
				}, {
					"real", [](lua_State * L)
					{
						auto td = TypeData<MatrixOf<Real>>::Get(L, GetTypeData::eFetchIfMissing);

						luaL_argcheck(L, td, 2, "real() requires real matrices");

						MatrixOf<Real> real = Getters::GetT(L)->real();

						PUSH_TYPED_DATA(real);
					}
				},
				{ nullptr, nullptr }
			};

			luaL_register(L, nullptr, methods);
		}
	};

	// Typical form of methods returning a boolean.
	#define EIGEN_MATRIX_PREDICATE(METHOD) return LuaXS::PushArgAndReturn(L, Getters::GetT(L)->METHOD(detail_matrix::GetPrecision<T>(L, 2)))
	#define EIGEN_MATRIX_PREDICATE_METHOD(NAME) EIGEN_REG(NAME, EIGEN_MATRIX_PREDICATE(NAME))

	//
	template<typename T, typename R> struct AttachBody {
		using Getters = InstanceGetters<T, R>;

		AttachBody (lua_State * L)
		{
            typedef typename Getters::ArrayType ArrayType; // Visual Studio workaround
            
			luaL_Reg methods[] = {
				{
					EIGEN_ARRAY_METHOD(acos)
				}, {
					"add", [](lua_State * L)
					{
						return NewRet<R>(L, *Getters::GetT(L) + Getters::GetR(L, 2));
					}
				}, {
					EIGEN_MATRIX_GET_MATRIX_METHOD(adjoint)
				}, {
					EIGEN_MATRIX_PUSH_VALUE_METHOD(allFinite)
				}, {
					EIGEN_ARRAY_METHOD(arg)
				}, {
					EIGEN_ARRAY_METHOD(asin)
				}, {
					EIGEN_ARRAY_METHOD(atan)
				}, {
					"cast", [](lua_State * L)
					{
						const char * names[] = { "int", "float", "double", "cfloat", "cdouble", nullptr };
						ScalarType types[] = { eInt, eFloat, eDouble, eCfloat, eCdouble };

						switch (types[luaL_checkoption(L, 2, nullptr, names)])
						{
						case eInt:
                            detail_matrix::Cast<T, R, int>{L};	// mat, type, im
							break;
						case eFloat:
							detail_matrix::Cast<T, R, float>{L};	// mat, type, fm
							break;
						case eDouble:
							detail_matrix::Cast<T, R, double>{L};// mat, type, dm
							break;
						case eCfloat:
							detail_matrix::Cast<T, R, std::complex<float>>{L};	// mat, type, cfm
							break;
						case eCdouble:
							detail_matrix::Cast<T, R, std::complex<double>>{L};	// mat, type, cdm
							break;
                        default:
                            luaL_error(L, "Bad type");
						}

						return 1;
					}
				}, {
					EIGEN_MATRIX_GET_MATRIX_METHOD(conjugate)
				}, {
					EIGEN_ARRAY_METHOD(cos)
				}, {
					EIGEN_ARRAY_METHOD(cosh)
				}, {
					EIGEN_ARRAY_METHOD(cube)
				}, {
					EIGEN_MATRIX_GET_MATRIX_METHOD(cwiseAbs)
				}, {
					EIGEN_MATRIX_GET_MATRIX_METHOD(cwiseAbs2)
				}, {
					EIGEN_MATRIX_GET_MATRIX_METHOD(cwiseInverse)
				}, {
					EIGEN_MATRIX_GET_MATRIX_MATRIX_PAIR_METHOD(cwiseProduct)
				}, {
					EIGEN_MATRIX_GET_MATRIX_MATRIX_PAIR_METHOD(cwiseQuotient)
				}, {
					EIGEN_MATRIX_GET_MATRIX_METHOD(cwiseSign)
				}, {
					EIGEN_MATRIX_GET_MATRIX_METHOD(cwiseSqrt)
				}, {
					"dot", [](lua_State * L)
					{
						return LuaXS::PushArgAndReturn(L, ColumnVector<R>{L}->dot(*ColumnVector<R>{L, 2}));
					}
				}, {
					EIGEN_ARRAY_METHOD(exp)
				}, {
					EIGEN_MATRIX_PUSH_VALUE_METHOD(hasNaN)
				}, {
					"isApprox", [](lua_State * L)
					{
                        return LuaXS::PushArgAndReturn(L, Getters::GetT(L)->isApprox(Getters::GetR(L, 2), detail_matrix::GetPrecision<T>(L, 3)));
					}
				}, {
					"isConstant", [](lua_State * L)
					{
                        return LuaXS::PushArgAndReturn(L, Getters::GetT(L)->isConstant(AsScalar<R>(L, 2), detail_matrix::GetPrecision<T>(L, 3)));
					}
				}, {
					EIGEN_MATRIX_PREDICATE_METHOD(isDiagonal)
				}, {
					EIGEN_ARRAY_METHOD_BOOL(isFinite)
				}, {
					EIGEN_MATRIX_PREDICATE_METHOD(isIdentity)
				}, {
					EIGEN_ARRAY_METHOD_BOOL(isInf)
				}, {
					EIGEN_MATRIX_PREDICATE_METHOD(isLowerTriangular)
				}, {
					EIGEN_MATRIX_PREDICATE_METHOD(isMuchSmallerThan)
				}, {
					EIGEN_ARRAY_METHOD_BOOL(isNaN)
				}, {
					EIGEN_MATRIX_PREDICATE_METHOD(isOnes)
				}, {
					"isOrthogonal", [](lua_State * L)
					{
                        return LuaXS::PushArgAndReturn(L, ColumnVector<R>{L}->isOrthogonal(*ColumnVector<R>{L, 2}, detail_matrix::GetPrecision<T>(L, 3)));
					}
				}, {
					EIGEN_MATRIX_PREDICATE_METHOD(isUnitary)
				}, {
					EIGEN_MATRIX_PREDICATE_METHOD(isUpperTriangular)
				}, {
					EIGEN_MATRIX_PREDICATE_METHOD(isZero)
				}, {
					EIGEN_ARRAY_METHOD(log)
				}, {
					EIGEN_ARRAY_METHOD(log10)
				}, {
					"lp1Norm", [](lua_State * L)
					{
						EIGEN_MATRIX_PUSH_VALUE(template lpNorm<1>);
					}
				}, {
					"lpInfNorm", [](lua_State * L)
					{
						EIGEN_MATRIX_PUSH_VALUE(template lpNorm<Eigen::Infinity>);
					}
				}, {
					EIGEN_MATRIX_PUSH_VALUE_METHOD(mean)
				}, {
					EIGEN_MATRIX_PUSH_VALUE_METHOD(norm)
				}, {
					EIGEN_MATRIX_GET_MATRIX_METHOD(normalized)
				}, {
					EIGEN_MATRIX_PUSH_VALUE_METHOD(prod)
				}, {
					EIGEN_ARRAY_METHOD(sin)
				}, {
					EIGEN_ARRAY_METHOD(sinh)
				}, {
					EIGEN_ARRAY_METHOD(square)
				}, {
					EIGEN_MATRIX_PUSH_VALUE_METHOD(squaredNorm)
				}, {
					"stableNorm", [](lua_State * L)
					{
						return LuaXS::PushArgAndReturn(L, ColumnVector<R>{L}->stableNorm());
					}
				}, {
					"stableNormalized", [](lua_State * L)
					{
						ColumnVector<R> cv{L};

						R nv = cv->stableNormalized();

						cv.RestoreShape(&nv);

						return NewRet<R>(L, nv);
					}
				}, {
					"sub", [](lua_State * L)
					{
						return NewRet<R>(L, *Getters::GetT(L) - Getters::GetR(L, 2));
					}
				}, {
					EIGEN_MATRIX_PUSH_VALUE_METHOD(sum)
				}, {
					EIGEN_ARRAY_METHOD(tan)
				}, {
					EIGEN_ARRAY_METHOD(tanh)
				}, {
					EIGEN_MATRIX_PUSH_VALUE_METHOD(trace)
				}, {
					"unitOrthogonal", [](lua_State * L)
					{
						ColumnVector<R> cv{L};

						R nv = cv->stableNormalized();

						cv.RestoreShape(&nv);

						return NewRet<R>(L, nv);
					}
				},
				{ nullptr, nullptr }
			};

			luaL_register(L, nullptr, methods);

			ArithOps<T, R> arith_ops{L};
			RealOps<T, R> real_ops{L};
			SolverOps<T, R> solver_ops{L};
			StockOps<T, R> so{L};
			WriteOps<T, R> wo{L};
			XprOps<T, R> xo{L};

			AddComplexComponentViews<T, R> accv{L};
		}
	};

	template<typename T> struct AttachBody<T, BoolMatrix> {
		using Getters = InstanceGetters<T, BoolMatrix>;

		AttachBody (lua_State * L)
		{
		#if defined(EIGEN_CORE) || defined(EIGEN_PLUGIN_BASIC)

			luaL_Reg methods[] = {
				{
					EIGEN_MATRIX_PUSH_VALUE_METHOD(all)
				}, {
					EIGEN_MATRIX_PUSH_VALUE_METHOD(any)
				}, {
					"band", [](lua_State * L)
					{
						return NewRet<BoolMatrix>(L, *Getters::GetT(L) && Getters::GetR(L, 2));
					}
				}, {
					"bor", [](lua_State * L)
					{
						return NewRet<BoolMatrix>(L, *Getters::GetT(L) || Getters::GetR(L, 2));
					}
				}, {
					EIGEN_MATRIX_PUSH_VALUE_METHOD(count)
				}, {
					"select", [](lua_State * L)
					{
						// For non-matrix objects such as maps, make a temporary and pass it
						// along to the select function. (Resolving this in the select logic
						// itself seems to cause major compilation slowdown.)
						ArgObjectR<BoolMatrix> bm{L, 1};

						if (!std::is_same<T, BoolMatrix>::value)
						{
							lua_pushlightuserdata(L, bm.mObject);	// mat, then, else, conv_mat
							lua_replace(L, 1);	// conv_mat, then, else
						}

						// Invoke the select logic appropriate to the supplied objects.
						GetTypeData * td1 = GetTypeData::FromObject(L, 2);
						GetTypeData * td2 = GetTypeData::FromObject(L, 3);

						luaL_argcheck(L, td1 || td2, 2, "Two scalars supplied to select()");
						luaL_argcheck(L, !td1 || !td2 || td1->GetName() == td2->GetName(), 2, "Mixed types supplied to select()");

						return (td1 ? td1 : td2)->Select(L);	// selection
					}
				},
				{ nullptr, nullptr }
			};

			luaL_register(L, nullptr, methods);

			StockOps<T, BoolMatrix> so{L};
			WriteOps<T, BoolMatrix> wo{L};
			XprOps<T, BoolMatrix> xo{L};

		#endif
		}
	};
}

// Common matrix methods attachment body.
template<typename T, typename R> struct AttachMatrixMethods {
	AttachMatrixMethods (lua_State * L)
	{
		detail_matrix::AttachBody<T, R> ab{L};
	}
};

/****************
* Block methods *
****************/
template<typename U, int Rows, int Cols, bool InnerPanel, typename R> struct AttachMethods<Eigen::Block<U, Rows, Cols, InnerPanel>, R> : AttachMatrixMethods<Eigen::Block<U, Rows, Cols, InnerPanel>, R> {
	AttachMethods (lua_State * L) : AttachMatrixMethods<Eigen::Block<U, Rows, Cols, InnerPanel>, R>(L)
	{
	}
};

template<typename U, int R, int C, bool InnerPanel> struct AuxTypeName<Eigen::Block<U, R, C, InnerPanel>> {
	AuxTypeName (luaL_Buffer * B, lua_State * L)
	{
		OpenType(B, "Block");

		AuxTypeName<U>(B, L);

		AddComma(B);
		AddDynamicOrN(B, L, R);
		AddComma(B);
		AddDynamicOrN(B, L, C);

		if (InnerPanel) luaL_addstring(B, ", true");

		CloseType(B);
	}
};

/*****************
* Matrix methods *
*****************/
template<typename T, int Rows, int Cols, int Options, int MaxRows, int MaxCols, typename R> struct AttachMethods<Eigen::Matrix<T, Rows, Cols, Options, MaxRows, MaxCols>, R> : AttachMatrixMethods<Eigen::Matrix<T, Rows, Cols, Options, MaxRows, MaxCols>, R> {
	AttachMethods (lua_State * L) : AttachMatrixMethods<Eigen::Matrix<T, Rows, Cols, Options, MaxRows, MaxCols>, R>(L)
	{
	}
};

template<typename Scalar, int R, int C, int O, int MR, int MC> struct AuxTypeName<Eigen::Matrix<Scalar, R, C, O, MR, MC>> {
	AuxTypeName (luaL_Buffer * B, lua_State * L)
	{
		AuxTypeName<Scalar>(B, L);

		if (R == 1 || C == 1)
		{
			AddFormatted(B, L, "_%s_vector[", R == 1 ? "row" : "col");
			AddDynamicOrN(B, L, R == 1 ? C : R);
		}

		else
		{
			luaL_addstring(B, "_matrix[");

			AddDynamicOrN(B, L, R);
			AddComma(B);
			AddDynamicOrN(B, L, C);
		}

		bool bDimsDiffer = MR != R || MC != C;

		if (O != MatrixOf<Scalar>::Options || bDimsDiffer)
		{
			int params[] = { O, MR, MC }, count = bDimsDiffer ? 3 : 1;

			for (int i = 0; i < count; ++i) AddFormatted(B, L, ", %d", params[i]);
		}

		luaL_addstring(B, "]");
	}
};

/************************
* Mapped matrix methods *
************************/
template<typename T, int Rows, int Cols, int Options, int MaxRows, int MaxCols, int MapOptions, typename S, typename R> struct AttachMethods<Eigen::Map<Eigen::Matrix<T, Rows, Cols, Options, MaxRows, MaxCols>, MapOptions, S>, R> : AttachMatrixMethods<Eigen::Map<Eigen::Matrix<T, Rows, Cols, Options, MaxRows, MaxCols>, MapOptions, S>, R> {
	AttachMethods (lua_State * L) : AttachMatrixMethods<Eigen::Map<Eigen::Matrix<T, Rows, Cols, Options, MaxRows, MaxCols>, MapOptions, S>, R>(L)
	{
	}
};

template<typename U, int O, typename S> struct AuxTypeName<Eigen::Map<U, O, S>> {
	AuxTypeName (luaL_Buffer * B, lua_State * L)
	{
		OpenType(B, "Map");

		AuxTypeName<U>(B, L);

		AddFormatted(B, L, ", %d", O);

		AuxTypeName<S>(B, L);

		CloseType(B);
	}
};
