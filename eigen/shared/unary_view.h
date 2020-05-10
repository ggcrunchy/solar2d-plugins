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
#include "views.h"

namespace detail_uv {
    //
    template<typename> struct IsRealOp : std::false_type {};
    template<typename S> struct IsRealOp<Eigen::internal::scalar_real_ref_op<S>> : std::true_type {};
    
    template<typename OP, typename LHS, typename RHS> void Assign (LHS & lhs, const RHS * rhs)
    {
        if (IsRealOp<OP>::value) lhs = rhs->real();
        
        else lhs = rhs->imag();
    }
    
    template<typename OP, typename V, typename R, bool = IsLvalue<V>::value> struct AddWriteOps {
        using Getters = InstanceGetters<Eigen::CwiseUnaryView<OP, V>, R>;

        AddWriteOps (lua_State * L)
        {
            using Real = typename Eigen::NumTraits<typename R::Scalar>::Real;
        
            luaL_Reg methods[] = {
                "assign", [](lua_State * L)
                {
                    #define CALL_IF(RHS) if (HasType<RHS>(L, 2)) Assign<OP>(*Getters::GetT(L), LuaXS::UD<RHS>(L, 2))
                    #define WRAP2(T, A, B) T<A, B>

                    CALL_IF(WRAP2(Eigen::CwiseUnaryView, OP, V));
                    else CALL_IF(WRAP2(Eigen::CwiseUnaryOp, OP, const V));
                    else CALL_IF(R);
                    else
                    {
                        MatrixOf<Real> result;
                    
                        Assign<OP>(*Getters::GetT(L), SetTemp(L, &result, 2));
                    }
                
                    #undef CALL_IF
                    #undef WRAP2
                    
                    return 0;
                },
                { nullptr, nullptr }
            };
        
            luaL_register(L, nullptr, methods);
        }
    };
    
    template<typename OP, typename V, typename R> struct AddWriteOps<OP, V, R, false> {
        AddWriteOps (lua_State *) {}
    };
}

/*********************
* Unary view methods *
*********************/
template<typename OP, typename V, typename R> struct AttachMethods<Eigen::CwiseUnaryView<OP, V>, R> {

	AttachMethods (lua_State * L)
	{
        detail_uv::AddWriteOps<OP, V, R> awo{L};
	}
};
