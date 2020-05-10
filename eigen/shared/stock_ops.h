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
#include "self_adjoint_view.h"
#include "triangular_view.h"
#include "vectorwise.h"

namespace details_stock {
	//
	template<typename T, typename, bool = IsBasic<T>::value> struct Transpose {
		static int Do (lua_State * L)
		{
			return Transposer<T>::Do(L);
		}
	};

	template<typename T, typename R> struct Transpose<T, R, false> {
		using Getters = InstanceGetters<T, R>;

		static int Do (lua_State * L)
		{
			EIGEN_MATRIX_GET_MATRIX(transpose);
		}
	};

    //
    template<typename T, typename R, bool = IsBasic<T>::value> struct AddBasicOps {
		using Getters = InstanceGetters<T, R>;

		AddBasicOps (lua_State * L)
		{
			luaL_Reg methods[] = {
			#ifdef WANT_MAP
				{
					"reshape", [](lua_State * L)
					{
						using M = MatrixOf<typename T::Scalar>;
                    
                        T & m = *Getters::GetT(L);
						Eigen::Map<M> map{m.data(), LuaXS::Int(L, 2), LuaXS::Int(L, 3)};
                    
						NEW_REF1_DECLTYPE_MOVE("mapped_from", map);	// mat, m, n, map
					}
				},
			#endif
			#ifdef WANT_MAP_WITH_CUSTOM_STRIDE
				{
					"reshapeWithInnerStride", [](lua_State * L)
					{
						using M = MatrixOf<T::Scalar>;
                    
                        T & m = *Getters::GetT(L);
						Eigen::Map<M, 0, Eigen::InnerStride<>> map{m.data(), LuaXS::Int(L, 2), LuaXS::Int(L, 3), LuaXS::Int(L, 4)};
                    
						NEW_REF1_DECLTYPE_MOVE("mapped_from", map);
					}
				}, {
					"reshapeWithOuterStride", [](lua_State * L)
					{
						using M = MatrixOf<T::Scalar>;
                    
                        T & m = *Getters::GetT(L);
						Eigen::Map<M, 0, Eigen::OuterStride<>> map{m.data(), LuaXS::Int(L, 2), LuaXS::Int(L, 3), LuaXS::Int(L, 4)};
                    
						NEW_REF1_DECLTYPE_MOVE("mapped_from", map);
					}
				},
			#endif
				{
					"selfadjointView", [](lua_State * L)
					{
						const char * names[] = { "Lower", "Upper", nullptr };
                    
						switch (luaL_checkoption(L, 2, nullptr, names))
						{
							case 0:	// Lower-triangular
								NEW_REF1_DECLTYPE("sav_viewed_from", Getters::GetT(L)->template selfadjointView<Eigen::Lower>());	// mat[, opt], sav
							default:// Upper-triangular
								NEW_REF1_DECLTYPE("sav_viewed_from", Getters::GetT(L)->template selfadjointView<Eigen::Upper>());	// mat[, opt], sav
						}
					}
				}, {
					"transpose", Transposer<T>::Do
				}, {
					"triangularView", [](lua_State * L)
					{
						const char * names[] = { "Lower", "StrictlyLower", "StrictlyUpper", "UnitLower", "UnitUpper", "Upper", nullptr };
                    
						switch (luaL_checkoption(L, 2, nullptr, names))
						{
							case 0:	// Lower-triangular
								NEW_REF1_DECLTYPE("tv_viewed_from", Getters::GetT(L)->template triangularView<Eigen::Lower>());	// mat[, opt], tv
							case 1: // Strictly lower-triangular
								NEW_REF1_DECLTYPE("tv_viewed_from", Getters::GetT(L)->template triangularView<Eigen::StrictlyLower>());	// mat[, opt], tv
							case 2: // Strictly upper-triangular
								NEW_REF1_DECLTYPE("tv_viewed_from", Getters::GetT(L)->template triangularView<Eigen::StrictlyUpper>());	// mat[, opt], tv
							case 3: // Upper-triangular
								NEW_REF1_DECLTYPE("tv_viewed_from", Getters::GetT(L)->template triangularView<Eigen::UnitLower>());	// mat[, opt], tv
							case 4: // Unit lower-triangular
								NEW_REF1_DECLTYPE("tv_viewed_from", Getters::GetT(L)->template triangularView<Eigen::UnitUpper>());	// mat[, opt], tv
							default: // Unit upper-triangular
								NEW_REF1_DECLTYPE("tv_viewed_from", Getters::GetT(L)->template triangularView<Eigen::Upper>());	// mat[, opt], tv
						}
					}
				},
				{ nullptr, nullptr }
			};
        
			luaL_register(L, nullptr, methods);
		}
	};
    
    template<typename T, typename R> struct AddBasicOps<T, R, false> {
		AddBasicOps (lua_State * L)
		{
			luaL_Reg methods[] = {
				{
					"transpose", Transpose<T, R>::Do
				},
				{ nullptr, nullptr }
			};
        
			luaL_register(L, nullptr, methods);
		}
	};

	//
	template<typename T, typename R> static int BinaryExpr (lua_State * L)
	{
		using Getters = InstanceGetters<T, R>;

		return NewRet<R>(L, Getters::GetT(L)->binaryExpr(Getters::GetR(L, 2), [L](const typename T::Scalar & x, const typename T::Scalar & y) {
			LuaXS::PushMultipleArgs(L, LuaXS::StackIndex{L, 3}, x, y);	// mat1, mat2, func, func, x, y

			lua_call(L, 2, 1);	// mat1, mat2, func, result

			typename T::Scalar result(0);

			if (!lua_isnil(L, -1)) result = AsScalar<R>(L, -1);

			lua_pop(L, 1);	// mat1, mat2, func

			return result;
		}));
	}

	template<typename T, typename R> static int UnaryExpr (lua_State * L)
	{
		return NewRet<R>(L, InstanceGetters<T, R>::GetT(L)->unaryExpr([L](const typename T::Scalar & x) {
			LuaXS::PushMultipleArgs(L, LuaXS::StackIndex{L, 2}, x);// mat, func, func, x

			lua_call(L, 1, 1);	// mat, func, result

			typename T::Scalar result(0);

			if (!lua_isnil(L, -1)) result = AsScalar<R>(L, -1);

			lua_pop(L, 1);	// mat, func

			return result;
		}));
	}

	template<typename T, typename R> static int Visit (lua_State * L)
	{
		struct Visitor {
			lua_State * mL;

			Visitor (lua_State * L) : mL{L}
			{
			}

			inline void Do (int arg, const typename T::Scalar & x, Eigen::Index i, Eigen::Index j)
			{
				LuaXS::PushMultipleArgs(mL, LuaXS::StackIndex{mL, arg}, x, int(i + 1), int(j + 1));	// mat, init, rest, func, x, i, j

				lua_call(mL, 3, 0);	// mat, init, rest
			}

			inline void init (const typename T::Scalar & x, Eigen::Index i, Eigen::Index j)
			{
				Do(2, x, i, j);
			}

			inline void operator ()(const typename T::Scalar & x, Eigen::Index i, Eigen::Index j)
			{
				Do(3, x, i, j);
			}
		} v{L};

		InstanceGetters<T, R>::GetT(L)->visit(v);

		return 0;
	}
}

//
template<typename T, typename R> struct StockOps {
    using Getters = InstanceGetters<T, R>;

    StockOps (lua_State * L)
	{
        typedef typename Getters::ArrayType ArrayType; // Visual Studio workaround
        
		luaL_Reg methods[] = {
			{
				EIGEN_MATRIX_GET_MATRIX_METHOD(asDiagonal)
			}, {
				"asMatrix", AsMatrix<T, R>
			}, {
				"binaryExpr", details_stock::BinaryExpr<T, R>
			}, {
				"__call", Call<T>
			}, {
				EIGEN_MATRIX_PUSH_VALUE_METHOD(cols)
			}, {
				EIGEN_MATRIX_PUSH_VALUE_METHOD(colStride)
			}, {
				"colwise", [](lua_State * L)
				{
                    NEW_REF1_DECLTYPE("vectorwise_from", Getters::GetT(L)->colwise());	// mat, vw
				}
			}, {
				EIGEN_REL_OP_METHOD(cwiseEqual, ==)
			}, {
				EIGEN_REL_OP_METHOD(cwiseNotEqual, !=)
			}, {
				EIGEN_MATRIX_PUSH_VALUE_METHOD(data)
			}, {
				EIGEN_MATRIX_PUSH_VALUE_METHOD(diagonalSize)
			}, {
				"__eq", [](lua_State * L)
				{
					bool bHas1 = HasType<T>(L, 1), bHas2 = HasType<T>(L, 2), result;

                    if (bHas1 && bHas2) result = *Getters::GetT(L) == *Getters::GetT(L, 2);
                    else if (bHas1) result = *Getters::GetT(L) == Getters::GetR(L, 2);
                    else result = Getters::GetR(L, 1) == *Getters::GetT(L, 2);

					return LuaXS::PushArgAndReturn(L, result);
				}
			}, {
				EIGEN_MATRIX_PUSH_VALUE_METHOD(innerSize)
			}, {
				EIGEN_MATRIX_PUSH_VALUE_METHOD(innerStride)
			}, {
				"__len", [](lua_State * L)
				{
					EIGEN_MATRIX_PUSH_VALUE(size);
				}
			}, {
				EIGEN_MATRIX_PUSH_VALUE_METHOD(outerSize)
			}, {
				EIGEN_MATRIX_PUSH_VALUE_METHOD(outerStride)
			}, {
				"redux", [](lua_State * L)
				{
					return LuaXS::PushArgAndReturn(L, Redux<T, R, typename T::Scalar>(L));
				}
			}, {
				EIGEN_MATRIX_GET_MATRIX_COUNT_PAIR_METHOD(replicate)
			}, {
				EIGEN_MATRIX_GET_MATRIX_METHOD(reverse)
			}, {
				EIGEN_MATRIX_PUSH_VALUE_METHOD(rows)
			}, {
				EIGEN_MATRIX_PUSH_VALUE_METHOD(rowStride)
			}, {
				"rowwise", [](lua_State * L)
				{
                    NEW_REF1_DECLTYPE("vectorwise_from", Getters::GetT(L)->rowwise());	// mat, vw
				}
			}, {
				EIGEN_MATRIX_PUSH_VALUE_METHOD(size)
			}, {
				"__tostring", [](lua_State * L)
				{
					return Print(L, *Getters::GetT(L));
				}
			}, {
				"unaryExpr", details_stock::UnaryExpr<T, R>
			}, {
				EIGEN_MATRIX_PUSH_VALUE_METHOD(value)
			}, {
				"visit", details_stock::Visit<T, R>
			},
			{ nullptr, nullptr }
		};

		luaL_register(L, nullptr, methods);

        details_stock::AddBasicOps<T, R> abo{L};
	}
};
