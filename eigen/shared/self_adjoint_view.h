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

#include "views.h"

namespace detail_sav {
    //
    template<typename T, typename R, bool = std::is_same<typename R::Scalar, bool>::value> struct AddNonBool {
        using Getters = InstanceGetters<T, R>;
		using Scalar = typename R::Scalar;

        AddNonBool (lua_State * L)
        {
            luaL_Reg methods[] = {
                {
                    "rankUpdate", [](lua_State * L)
                    {
                        bool bHasV = HasType<R>(L, 3);
                        int spos = bHasV ? 4 : 3;
                        Scalar alpha = !lua_isnoneornil(L, spos) ? AsScalar<R>(L, spos) : Scalar(1);
                
                        if (bHasV) Getters::GetT(L)->rankUpdate(*ColumnVector<R>{L, 2}, *ColumnVector<R>{L, 3}, alpha);
                        else Getters::GetT(L)->rankUpdate(Getters::GetR(L, 2), alpha);
                
                        return SelfForChaining(L);	// sav, u[, v][, scalar], v
                    }
                },
                { nullptr, nullptr }
            };
    
            luaL_register(L, nullptr, methods);
        }
    };

    template<typename T, typename R> struct AddNonBool<T, R, true> {
        AddNonBool (lua_State *) {}
    };

    //
    template<typename MT, unsigned int UpLo, typename R> static void BaseDerivedSAV (lua_State * L)
    {
        using Getters = InstanceGetters<Eigen::SelfAdjointView<MT, UpLo>, R>;
    
        luaL_Reg methods[] = {
            {
                "assign", [](lua_State * L)
                {
                    return 0;	// TODO
                }
            }, {
                "coeffAssign", [](lua_State * L)
                {
                    return 0;	// TODO
                }
            }, {
                EIGEN_MATRIX_PUSH_VALUE_METHOD(innerStride)
            }, {
                EIGEN_MATRIX_PUSH_VALUE_METHOD(outerStride)
            },
            { nullptr, nullptr }
        };
        
        luaL_register(L, nullptr, methods);
        
        AddNonBool<Eigen::SelfAdjointView<MT, UpLo>, R> anb{L};
    }

    //
    template<typename T, typename R, bool = Eigen::NumTraits<typename R::Scalar>::IsComplex> struct EigenvaluesSAV {
        static int Do (lua_State * L)
        {
            using Getters = InstanceGetters<T, R>;

            EIGEN_MATRIX_GET_MATRIX(eigenvalues);
        }
    };
    
    template<typename T, typename R> struct EigenvaluesSAV<T, R, false> {
        static int Do (lua_State * L)
        {
            using Getters = InstanceGetters<T, R>;

            USING_COMPLEX_TYPE();
            EIGEN_REAL_GET_COMPLEX(eigenvalues);
        }
    };

    //
    template<typename MT, unsigned int UpLo, typename R, bool = Eigen::NumTraits<typename R::Scalar>::IsInteger> struct AddNonInt {
        using Getters = InstanceGetters<Eigen::SelfAdjointView<MT, UpLo>, R>;

        AddNonInt (lua_State * L)
        {
            luaL_Reg methods[] = {
                {
                    "eigenvalues", EigenvaluesSAV<Eigen::SelfAdjointView<MT, UpLo>, R>::Do
                }, {
                    EIGEN_PUSH_AUTO_RESULT_METHOD(ldlt) // n.b. earlier LDLT<T, UpLo> for normal matrices, i.e. not row-major, thus here
                }, {
                    EIGEN_PUSH_AUTO_RESULT_METHOD(llt)	// ditto
                }, {
                    EIGEN_MATRIX_PUSH_VALUE_METHOD(operatorNorm)
                },
                { nullptr, nullptr }
            };
        
            luaL_register(L, nullptr, methods);
        }
    };
    
    template<typename MT, unsigned int UpLo, typename R> struct AddNonInt<MT, UpLo, R, true> {
        AddNonInt (lua_State *) {}
    };
    
    //
    template<typename MT, unsigned int UpLo, typename R, bool = IsMatrix<MT>::value> struct NotMap {
        using Getters = InstanceGetters<Eigen::SelfAdjointView<MT, UpLo>, R>;

        NotMap (lua_State * L)
        {
            luaL_Reg methods[] = {
                {
                    "triangularView", [](lua_State * L)
                    {
                        const char * names[] = { "Lower", "StrictlyLower", "StrictlyUpper", "UnitLower", "UnitUpper", "Upper", nullptr };
                        
                        #define PUSH_RES(METHOD) { EIGEN_PUSH_AUTO_RESULT(template METHOD); }
                        
                        switch (luaL_checkoption(L, 2, nullptr, names))
                        {
                            case 0:	// Lower-triangular
                                PUSH_RES(triangularView<Eigen::Lower>)
                            case 1:// Strictly lower-triangular
                                PUSH_RES(triangularView<Eigen::StrictlyLower>)
                            case 2:// Strictly upper-triangular
                                PUSH_RES(triangularView<Eigen::StrictlyUpper>)
                            case 3:// Upper-triangular
                                PUSH_RES(triangularView<Eigen::UnitLower>)
                            case 4:// Unit lower-triangular
                                PUSH_RES(triangularView<Eigen::UnitUpper>)
                            default:// Unit upper-triangular
                                PUSH_RES(triangularView<Eigen::Upper>)
                        }
                        
                        #undef PUSH_RES
                    }
                },
                { nullptr, nullptr }
            };
            
            luaL_register(L, nullptr, methods);
        };
    };
    
    template<typename MT, unsigned int UpLo, typename R> struct NotMap<MT, UpLo, R, false> {
        NotMap (lua_State *) {}
    };

    //
    template<typename MT, unsigned int UpLo, typename R> struct DerivedSAV {
        using Getters = InstanceGetters<Eigen::SelfAdjointView<MT, UpLo>, R>;
        
        DerivedSAV (lua_State * L)
        {
            luaL_Reg methods[] = {
                {
                    EIGEN_PUSH_AUTO_RESULT_METHOD(adjoint)
                }, {
                    EIGEN_PUSH_AUTO_RESULT_METHOD(conjugate)
                }, {
                    EIGEN_PUSH_AUTO_RESULT_METHOD(transpose)
                },
                { nullptr, nullptr }
            };
            
            luaL_register(L, nullptr, methods);
            
            BaseDerivedSAV<MT, UpLo, R>(L);
            AddNonInt<MT, UpLo, R> ani{L};
            NotMap<MT, UpLo, R> nm{L};
        }
    };
    
    //
    template<typename U, unsigned int N, typename R> struct DerivedSAV<Eigen::Transpose<U>, N, R> {
        DerivedSAV (lua_State * L)
        {
            BaseDerivedSAV<Eigen::Transpose<U>, N, R>(L);
        }
    };
    
    //
    template<typename U, unsigned int N, typename R> struct DerivedSAV<const U, N, R> {
        DerivedSAV (lua_State *) {}
    };
    
    template<typename U, typename V, unsigned int N, typename R> struct DerivedSAV<Eigen::CwiseUnaryOp<U, V>, N, R> {
        DerivedSAV (lua_State *) {}
    };
};

/**************************
* SelfAdjointView methods *
**************************/
template<typename MT, unsigned int UpLo, typename R> struct AttachMethods<Eigen::SelfAdjointView<MT, UpLo>, R> {
    using Getters = InstanceGetters<Eigen::SelfAdjointView<MT, UpLo>, R>;

	//
	AttachMethods (lua_State * L)
	{
		luaL_Reg methods[] = {
			{
				"asMatrix", AsMatrix<Eigen::SelfAdjointView<MT, UpLo>, R>
			}, {
				"__call", Call<Eigen::SelfAdjointView<MT, UpLo>>
			}, {
				EIGEN_MATRIX_PUSH_VALUE_METHOD(cols)
			}, {
				EIGEN_MATRIX_GET_MATRIX_METHOD(diagonal)
			}, {
				"__mul", [](lua_State * L)
				{
					/*
					template<typename OtherDerived>
					117     EIGEN_DEVICE_FUNC
					118     const Product<SelfAdjointView,OtherDerived>
					119     operator*(const MatrixBase<OtherDerived>& rhs) const
					120     {
					121       return Product<SelfAdjointView,OtherDerived>(*this, rhs.derived());
					122     }
					123
					125     template<typename OtherDerived> friend
					126     EIGEN_DEVICE_FUNC
					127     const Product<OtherDerived,SelfAdjointView>
					128     operator*(const MatrixBase<OtherDerived>& lhs, const SelfAdjointView& rhs)
					129     {
					130       return Product<OtherDerived,SelfAdjointView>(lhs.derived(),rhs);
					131     }
					132
					133     friend EIGEN_DEVICE_FUNC
					134     const SelfAdjointView<const EIGEN_SCALAR_BINARYOP_EXPR_RETURN_TYPE(Scalar,MatrixType,product),UpLo>
					135     operator*(const Scalar& s, const SelfAdjointView& mat)
					136     {
					137       return (s*mat.nestedExpression()).template selfadjointView<UpLo>();
					138     }
					*/
					return 1;	// TODO
				}
			}, {
				EIGEN_MATRIX_PUSH_VALUE_METHOD(rows)
			},
			{ nullptr, nullptr }
		};

		luaL_register(L, nullptr, methods);

        detail_sav::DerivedSAV<MT, UpLo, R> derived{L};
	}
};

template<typename U, unsigned int E> struct AuxTypeName<Eigen::SelfAdjointView<U, E>> {
	AuxTypeName (luaL_Buffer * B, lua_State * L)
	{
		OpenType(B, "SelfAdjointView");

		AuxTypeName<U>(B, L);

		lua_pushfstring(L, ", %d>", E);	// ..., E
		luaL_addvalue(B);	// ...
	}
};
