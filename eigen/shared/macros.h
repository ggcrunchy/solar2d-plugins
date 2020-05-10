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

//
#define EIGEN_REL_OP(OP)	return Getters::WithArray(L, [L](const ArrayType & arr) {               \
								if (!HasType<T>(L, 2))                                              \
								{                                                                   \
									ArgObjectR<R> ao{L, 2};                                         \
                                                                                                    \
									if (ao.mObject) New<BoolMatrix>(L, arr OP ao.mObject->array()); \
									else New<BoolMatrix>(L, arr OP ao.mScalar);                     \
								}                                                                   \
                                                                                                    \
								else New<BoolMatrix>(L, arr OP Getters::GetR(L, 2).array());        \
							})
							
//
#define EIGEN_AS_ARRAY(METHOD)	return Getters::WithArray(L, [L](const ArrayType & arr) {	\
									New<R>(L, arr.METHOD());                                \
								})

//
#define EIGEN_AS_ARRAY_BOOL(METHOD)	return Getters::WithArray(L, [L](const ArrayType & arr) {	\
										New<BoolMatrix>(L, arr.METHOD());                       \
									})

//
#define EIGEN_MATRIX_GET_MATRIX(METHOD)	return NewRet<R>(L, Getters::GetT(L)->METHOD())

//
#define EIGEN_MATRIX_GET_MATRIX_COUNT(METHOD)	return NewRet<R>(L, Getters::GetT(L)->METHOD(LuaXS::Int(L, 2)))

//
#define EIGEN_MATRIX_GET_MATRIX_COUNT_PAIR(METHOD)	return NewRet<R>(L, Getters::GetT(L)->METHOD(LuaXS::Int(L, 2), LuaXS::Int(L, 3)))

//
#define EIGEN_MATRIX_GET_MATRIX_INDEX(METHOD)	return NewRet<R>(L, Getters::GetT(L)->METHOD(LuaXS::Int(L, 2) - 1))

//
#define EIGEN_MATRIX_GET_MATRIX_INDEX_PAIR(METHOD)	return NewRet<R>(L, Getters::GetT(L)->METHOD(LuaXS::Int(L, 2) - 1, LuaXS::Int(L, 3) - 1))

//
#define EIGEN_MATRIX_GET_MATRIX_MATRIX_PAIR(METHOD)	return NewRet<R>(L, Getters::GetT(L)->METHOD(Getters::GetR(L, 2)))

//
#define EIGEN_MATRIX_GET_MATRIX_SECOND_IS_MATRIX_OR_SCALAR(METHOD)	ArgObjectR<R> ao{L, 2};                                                     \
                                                                                                                                                \
																	if (ao.mObject) return NewRet<R>(L, Getters::GetT(L)->METHOD(*ao.mObject)); \
																	else return NewRet<R>(L, Getters::GetT(L)->METHOD(ao.mScalar))

//
#define EIGEN_MATRIX_PAIR_VOID(METHOD)	Getters::GetT(L)->METHOD(Getters::GetR(L, 2));  \
                                                                                        \
										return 0
//
#define EIGEN_MATRIX_SET_SCALAR(METHOD)	Getters::GetT(L)->METHOD(AsScalar<R>(L, 2));\
                                                                                    \
										return 0

//
#define EIGEN_MATRIX_SET_SCALAR_CHAIN(METHOD)	Getters::GetT(L)->METHOD(AsScalar<R>(L, 2));\
                                                                                            \
												return SelfForChaining(L)

//
#define EIGEN_MATRIX_PUSH_VALUE(METHOD)	return LuaXS::PushArgAndReturn(L, Getters::GetT(L)->METHOD())

//
#define EIGEN_MATRIX_VOID(METHOD)		Getters::GetT(L)->METHOD(); \
                                                                    \
										return 0

//
#define EIGEN_MATRIX_CHAIN(METHOD)		Getters::GetT(L)->METHOD();	\
																	\
										return SelfForChaining(L)

//
#define USING_COMPLEX_TYPE() using ComplexType = MatrixOf<std::complex<typename R::Scalar>>
#define GET_COMPLEX_TYPE_DATA()	auto td = TypeData<ComplexType>::Get(L);							\
																									\
								luaL_argcheck(L, td, 1, "Complex matrix type unavailable for cast")

//
#define PUSH_TYPED_DATA_NO_RET(item)	lua_getref(L, td->mPushRef);/* instance, ..., push_new_type */		\
										lua_pushlightuserdata(L, &item);/*instance, push_new_type, item */	\
										lua_call(L, 1, 1)	/* instance, conv_item */
#define PUSH_TYPED_DATA(item)	PUSH_TYPED_DATA_NO_RET(item);	/* instance, conv_item */	\
																							\
								return 1

//
#define EIGEN_REAL_GET_COMPLEX(METHOD)	GET_COMPLEX_TYPE_DATA();                        \
                                                                                        \
										ComplexType res = Getters::GetT(L)->METHOD();   \
                                                                                        \
										PUSH_TYPED_DATA(res)

//
// TODO: too fragile? autos like this lead to surprises :/ (thinking of going with proxies or tags)
#define EIGEN_PUSH_AUTO_RESULT(METHOD)  auto res = Getters::GetT(L)->METHOD();  \
                                                                                \
                                        return NewRet<decltype(res)>(L, res)

// Helper to package a name and method body as a luaL_Reg.
#define EIGEN_REG(NAME, CALL)	#NAME, [](lua_State * L)	\
								{							\
									CALL;					\
								}

// The above macros wrapped up.
#define EIGEN_ARRAY_METHOD(NAME) EIGEN_REG(NAME, EIGEN_AS_ARRAY(NAME))
#define EIGEN_ARRAY_METHOD_BOOL(NAME) EIGEN_REG(NAME, EIGEN_AS_ARRAY_BOOL(NAME))
#define EIGEN_MATRIX_CHAIN_METHOD(NAME) EIGEN_REG(NAME, EIGEN_MATRIX_CHAIN(NAME))
#define EIGEN_MATRIX_GET_MATRIX_METHOD(NAME) EIGEN_REG(NAME, EIGEN_MATRIX_GET_MATRIX(NAME))
#define EIGEN_MATRIX_GET_MATRIX_COUNT_METHOD(NAME) EIGEN_REG(NAME, EIGEN_MATRIX_GET_MATRIX_COUNT(NAME))
#define EIGEN_MATRIX_GET_MATRIX_COUNT_PAIR_METHOD(NAME)	EIGEN_REG(NAME, EIGEN_MATRIX_GET_MATRIX_COUNT_PAIR(NAME))
#define EIGEN_MATRIX_GET_MATRIX_INDEX_METHOD(NAME) EIGEN_REG(NAME, EIGEN_MATRIX_GET_MATRIX_INDEX(NAME))
#define EIGEN_MATRIX_GET_MATRIX_INDEX_PAIR_METHOD(NAME)	EIGEN_REG(NAME, EIGEN_MATRIX_GET_MATRIX_INDEX_PAIR(NAME))
#define EIGEN_MATRIX_GET_MATRIX_MATRIX_PAIR_METHOD(NAME) EIGEN_REG(NAME, EIGEN_MATRIX_GET_MATRIX_MATRIX_PAIR(NAME))
#define EIGEN_MATRIX_GET_MATRIX_SECOND_IS_MATRIX_OR_SCALAR_METHOD(NAME)	EIGEN_REG(NAME, EIGEN_MATRIX_GET_MATRIX_SECOND_IS_MATRIX_OR_SCALAR(NAME))
#define EIGEN_MATRIX_PAIR_VOID_METHOD(NAME) EIGEN_REG(NAME, EIGEN_MATRIX_PAIR_VOID(NAME))
#define EIGEN_MATRIX_PUSH_VALUE_METHOD(NAME) EIGEN_REG(NAME, EIGEN_MATRIX_PUSH_VALUE(NAME))
#define EIGEN_MATRIX_SET_SCALAR_CHAIN_METHOD(NAME) EIGEN_REG(NAME, EIGEN_MATRIX_SET_SCALAR_CHAIN(NAME))
#define EIGEN_MATRIX_SET_SCALAR_METHOD(NAME) EIGEN_REG(NAME, EIGEN_MATRIX_SET_SCALAR(NAME))
#define EIGEN_MATRIX_VOID_METHOD(NAME) EIGEN_REG(NAME, EIGEN_MATRIX_VOID(NAME))
#define EIGEN_REL_OP_METHOD(NAME, OP) EIGEN_REG(NAME, EIGEN_REL_OP(OP))
#define EIGEN_REAL_GET_COMPLEX_METHOD(NAME)	EIGEN_REG(NAME, EIGEN_REAL_GET_COMPLEX(NAME))
#define EIGEN_PUSH_AUTO_RESULT_METHOD(NAME) EIGEN_REG(NAME, EIGEN_PUSH_AUTO_RESULT(NAME))
