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

#include "stdafx.h"
#include "macros.h"
#include "types.h"

//
template<typename U, int Dir> struct Nested<Eigen::VectorwiseOp<U, Dir>> {
	using Type = U;
};

//
template<typename T> struct IsBasicVectorwise : std::false_type {};
template<typename U, int Dir> struct IsBasicVectorwise<Eigen::VectorwiseOp<U, Dir>> : IsBasic<U> {};

//
template<typename T, typename R, bool = IsBasicVectorwise<T>::value> struct VectorwiseWriteOpsGetters : InstanceGetters<T, R> {};
template<typename T, typename R> struct VectorwiseWriteOpsGetters<T, R, false> : TempInstanceGetters<T, R> {};

namespace details_vw {
    // Add some methods when the underlying type is real.
    template<typename U, int Dir, typename R, bool = !Eigen::NumTraits<typename R::Scalar>::IsComplex> struct NonComplex {
        using Getters = InstanceGetters<Eigen::VectorwiseOp<U, Dir>, R>;
        
        NonComplex (lua_State * L)
        {
            luaL_Reg methods[] = {
                {
                    EIGEN_MATRIX_GET_MATRIX_METHOD(maxCoeff)
                }, {
                    EIGEN_MATRIX_GET_MATRIX_METHOD(minCoeff)
                },
                { nullptr, nullptr }
            };
        
            luaL_register(L, nullptr, methods);
        }
    };
    
    template<typename U, int Dir, typename R> struct NonComplex<U, Dir, R, false> {
        NonComplex (lua_State *) {}
    };
    
    //
    template<typename U, int Dir, typename R, bool = IsLvalue<U>::value> struct NumericalWriteOps {
        using VR = VectorRef<R, Eigen::VectorwiseOp<U, Dir>::isHorizontal>;
        using Getters = VectorwiseWriteOpsGetters<VR, R>;

        NumericalWriteOps (lua_State * L)
        {
            luaL_Reg methods[] = {
                {
                    "addInPlace", [](lua_State * L)
                    {
                        *Getters::GetT(L) += *VR{L, 2};
                        
                        return SelfForChaining(L);
                    }
                }, {
                    EIGEN_MATRIX_VOID_METHOD(normalize)
                }, {
                    "subInPlace", [](lua_State * L)
                    {
                        *Getters::GetT(L) -= *VR(L, 2);
                        
                        return SelfForChaining(L);
                    }
                },
                { nullptr, nullptr }
            };
            
            luaL_register(L, nullptr, methods);
        }
    };
    
    template<typename U, int Dir, typename R> struct NumericalWriteOps<U, Dir, R, false> {
        NumericalWriteOps (lua_State *) {}
    };
    
    // Add methods that only makes sense for non-boolean matrices.
    template<typename U, int Dir, typename R> struct AddBoolMatrixDependent {
        using VR = VectorRef<R, Eigen::VectorwiseOp<U, Dir>::isHorizontal>;
        using Getters = InstanceGetters<Eigen::VectorwiseOp<U, Dir>, R>;
        
        AddBoolMatrixDependent (lua_State * L)
        {
            luaL_Reg methods[] = {
                {
                    "add", [](lua_State * L)
                    {
                        return NewRet<R>(L, *Getters::GetT(L) + *VR{L, 2});
                    }
                }, {
                    EIGEN_MATRIX_GET_MATRIX_METHOD(blueNorm)
                }, {
                    EIGEN_MATRIX_GET_MATRIX_METHOD(hypotNorm)
                }, {
                    "lp1Norm", [](lua_State * L)
                    {
                        EIGEN_MATRIX_GET_MATRIX(template lpNorm<1>);
                    }
                }, {
                    "lpInfNorm", [](lua_State * L)
                    {
                        EIGEN_MATRIX_GET_MATRIX(template lpNorm<Eigen::Infinity>);
                    }
                }, {
                    EIGEN_MATRIX_GET_MATRIX_METHOD(mean)
                }, {
                    EIGEN_MATRIX_GET_MATRIX_METHOD(norm)
                }, {
                    EIGEN_MATRIX_GET_MATRIX_METHOD(normalized)
                }, {
                    EIGEN_MATRIX_GET_MATRIX_METHOD(prod)
                }, {
                    EIGEN_MATRIX_GET_MATRIX_METHOD(squaredNorm)
                }, {
                    "sub", [](lua_State * L)
                    {
                        return NewRet<R>(L, *Getters::GetT(L) - *VR{L, 2});
                    }
                }, {
                    EIGEN_MATRIX_GET_MATRIX_METHOD(sum)
                },
                { nullptr, nullptr }
            };
        
            luaL_register(L, nullptr, methods);
        
            NonComplex<U, Dir, R> nc{L};
            NumericalWriteOps<U, Dir, R> nwo{L};
        }
    };

    // Add methods for BoolMatrix-based types.
    template<typename U, int Dir> struct AddBoolMatrixDependent<U, Dir, BoolMatrix> {
        using Getters = InstanceGetters<Eigen::VectorwiseOp<U, Dir>, BoolMatrix>;
        using R = BoolMatrix;

        AddBoolMatrixDependent (lua_State * L)
        {
            luaL_Reg methods[] = {
                {
                    EIGEN_MATRIX_GET_MATRIX_METHOD(all)
                }, {
                    EIGEN_MATRIX_GET_MATRIX_METHOD(any)
                }, {
                    "count", [](lua_State * L)
                    {
                        auto td = TypeData<Eigen::MatrixXi>::Get(L, GetTypeData::eFetchIfMissing);
                    
                        luaL_argcheck(L, td, 2, "Vectorwise count() requires int matrices");
                    
                        Eigen::MatrixXi im = Getters::GetT(L)->count().template cast<int>();
                    
                        PUSH_TYPED_DATA(im);
                    }
                },
                { nullptr, nullptr }
            };
        
            luaL_register(L, nullptr, methods);
        }
    };

    //
    template<typename U, int Dir, typename R, bool = IsLvalue<U>::value> struct WriteOps {
        using VR = VectorRef<R, Eigen::VectorwiseOp<U, Dir>::isHorizontal>;
        using Getters = VectorwiseWriteOpsGetters<VR, R>;

        WriteOps (lua_State * L)
        {
            luaL_Reg methods[] = {
                {
                    "assign", [](lua_State * L)
                    {
                        *Getters::GetT(L) = *VR{L, 2};
                        
                        return SelfForChaining(L);
                    }
                }, {
                    EIGEN_MATRIX_VOID_METHOD(reverseInPlace)
                },
                { nullptr, nullptr }
            };
            
            luaL_register(L, nullptr, methods);
        }
    };
    
    template<typename U, int Dir, typename R> struct WriteOps<U, Dir, R, false> {
        WriteOps (lua_State *) {}
    };
}

/***********************
* VectorwiseOp methods *
***********************/
template<typename U, int Dir, typename R> struct AttachMethods<Eigen::VectorwiseOp<U, Dir>, R> {
    using Getters = InstanceGetters<Eigen::VectorwiseOp<U, Dir>, R>;

	AttachMethods (lua_State * L)
	{
		luaL_Reg methods[] = {
			{
				"redux", [](lua_State * L)
				{
					return NewRet<R>(L, Redux<Eigen::VectorwiseOp<U, Dir>, R, R>(L));
				}
			}, {
				EIGEN_MATRIX_GET_MATRIX_COUNT_METHOD(replicate)
			}, {
				EIGEN_MATRIX_GET_MATRIX_METHOD(reverse)
			},
			{ nullptr, nullptr }
		};

		luaL_register(L, nullptr, methods);

        details_vw::AddBoolMatrixDependent<U, Dir, R> abmd{L};
        details_vw::WriteOps<U, Dir, R> wo{L};
	}
};

template<typename U, int Dir> struct AuxTypeName<Eigen::VectorwiseOp<U, Dir>> {
	AuxTypeName (luaL_Buffer * B, lua_State * L)
	{
		OpenType(B, "VectorwiseOp");

		AuxTypeName<U>(B, L);

		lua_pushfstring(L, ", %d>", Dir);	// ..., Dir
		luaL_addvalue(B);	// ...
	}
};
