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

// Helper to return the instance again for method chaining.
inline int SelfForChaining (lua_State * L)
{
	lua_pushvalue(L, 1);// self, ..., self

	return 1;
}

// Check for strings whose presence means true.
inline bool WantsBool (lua_State * L, const char * str, int arg = -1)
{
	arg = CoronaLuaNormalize(L, arg);

	lua_pushstring(L, str);	// ..., arg, ..., opt

	bool bWants = lua_equal(L, arg, -1) != 0;

	lua_pop(L, 1);	// ..., arg, ...

	return bWants;
}

// Helper to read complex numbers in various formats.
template<typename T> static std::complex<T> Complex (lua_State * L, int arg)
{
    switch (lua_type(L, arg))
    {
	case LUA_TNUMBER:
		return std::complex<T>{static_cast<T>(lua_tonumber(L, arg)), static_cast<T>(0)};
	case LUA_TTABLE:
        arg = CoronaLuaNormalize(L, arg);
            
        lua_rawgeti(L, arg, 1);	// ..., complex, ..., r
        lua_rawgeti(L, arg, 2);	// ..., complex, ..., r, i
            
        {
            double r = luaL_optnumber(L, -2, 0.0), i = luaL_optnumber(L, -1, 0.0);
            
            lua_pop(L, 2);	// ..., complex, ...
            
            return std::complex<T>{static_cast<T>(r), static_cast<T>(i)};
        }
    default:
        ByteReader reader{L, arg};
            
        luaL_argcheck(L, reader.mBytes && reader.mCount >= sizeof(std::complex<T>), arg, "Invalid complex number");
            
        std::complex<T> comp;
            
        memcpy(&comp, reader.mBytes, sizeof(std::complex<T>));
            
        return comp;
    }
}

// Helper to fetch a scalar from the stack.
template<typename T, bool = Eigen::NumTraits<typename T::Scalar>::IsComplex> struct AuxAsScalar {
    static typename T::Scalar Do (lua_State * L, int arg)
    {
        return Complex<typename Eigen::NumTraits<typename T::Scalar>::Real>(L, arg);
    }
};

template<typename T> struct AuxAsScalar<T, false> {
    static typename T::Scalar Do (lua_State * L, int arg)
    {
        return LuaXS::GetArg<typename T::Scalar>(L, arg);
    }
};

template<typename T> typename T::Scalar AsScalar (lua_State * L, int arg)
{
    return AuxAsScalar<T>::Do(L, arg);
}

// Helper to fetch a coefficient from a matrix.
template<typename T> int Call (lua_State * L)
{
	const T & m = *GetInstance<T>(L);
	int a = LuaXS::Int(L, 2) - 1;
	typename T::Scalar result;

	if (lua_gettop(L) == 2)
	{
		CheckVector(L, m, 1);

		result = m.cols() == 1 ? m(a, 0) : m(0, a);
	}

	else result = m(a, LuaXS::Int(L, 3) - 1);

	return LuaXS::PushArgAndReturn(L, result);
}

// Convert a matrix to a pretty-printed string, e.g. for use by __tostring.
template<typename T> int Print (lua_State * L, const T & m)
{
	std::stringstream ss;

	ss << m;

	lua_pushstring(L, ss.str().c_str());// m, str

	return 1;
}

// Helper to reduce an object given some user-supplied function.
template<typename T, typename R, typename RRT> RRT Redux (lua_State * L)
{
	auto func = [L](const typename T::Scalar & x, const typename T::Scalar & y)
	{
		LuaXS::PushMultipleArgs(L, LuaXS::StackIndex{L, 2}, x, y);	// mat, func[, how], func, x, y

		lua_call(L, 2, 1);	// mat, func[, how], result

		typename T::Scalar result(0);

		if (!lua_isnil(L, -1)) result = AsScalar<R>(L, -1);

		lua_pop(L, 1);	// mat, func

		return result;
	};

	return GetInstance<T>(L)->redux(func);
}

// Specialize PushArg() for complex types to streamline code elsewhere, e.g. in macros.
template<> inline void LuaXS::PushArg<std::complex<double>> (lua_State * L, std::complex<double> c)
{
	lua_createtable(L, 2, 0);	// ..., c
	lua_pushnumber(L, c.real());// ..., c, c.r
	lua_rawseti(L, -2, 1);	// ..., c = { r }
	lua_pushnumber(L, c.imag());// ..., c, c.i
	lua_rawseti(L, -2, 2);	// ..., c = { r, i }
}

template<> inline void LuaXS::PushArg<std::complex<float>> (lua_State * L, std::complex<float> c)
{
	LuaXS::PushArg(L, std::complex<double>{c.real(), c.imag()});
}

// Customizes SetTemp()'s early-out behavior.
template<typename R, bool bSetTemp> struct EarlyOut {
	static R * Do (lua_State * L, int arg, R *)
	{
		return LuaXS::UD<R>(L, arg);
	}
};

template<typename R> struct EarlyOut<R, true> {
	static R * Do (lua_State * L, int arg, R * temp)
	{
		R * ud = LuaXS::UD<R>(L, arg);

		*temp = *ud;

		return ud;
	}
};

// Populate a matrix (typically a stack temporary) by going through an object's "asMatrix"
// method. These methods are all built on AsMatrix(), which special cases the logic here.
template<typename R, bool bSetTemp> R * SetTemp (lua_State * L, R * temp, int arg, bool bMissingOK)
{
	static_assert(std::is_same<R, MatrixOf<typename R::Scalar>>::value, "Temp type must be primitive");

	// As a sanity check, check for inputs with different scalar types. If found, cast them
	// to the one we expect; otherwise, simply fetch the object from the stack.
	GetTypeData * td = GetTypeData::FromObject(L, arg);

	if (!td)
	{
		if (!bMissingOK) luaL_error(L, "Input is not an Eigen type");

		return nullptr;
	}

	bool bIsConvertible = td->GetInfo().mIsConvertible;

	if (td->GetInfo().mType != GetScalarType<typename R::Scalar>::value)
	{
		lua_pushvalue(L, arg);	// ..., other
		luaL_argcheck(L, luaL_getmetafield(L, -1, "cast"), arg, "Object does not support cast");// ..., other, cast
		lua_insert(L, -2);	// ..., cast, other

		switch (GetScalarType<typename R::Scalar>::value)
		{
		case eInt:
			lua_pushliteral(L, "int");	// ..., cast, other, "int"
			break;
		case eFloat:
			lua_pushliteral(L, "float");// ..., cast, other, "float"
			break;
		case eDouble:
			lua_pushliteral(L, "double");	// ..., cast, other, "double"
			break;
		case eCfloat:
			lua_pushliteral(L, "cfloat");	// ..., cast, other, "cfloat"
			break;
		case eCdouble:
			lua_pushliteral(L, "cdouble");	// ..., cast, other, "cdouble"
			break;
		default:
			luaL_error(L, "Unsupported type");
		}

		lua_call(L, 2, 1);	// ..., casted

		bIsConvertible = true;
	}

	else
	{
		if (td->GetInfo().mIsPrimitive) return EarlyOut<R, bSetTemp>::Do(L, arg, temp);

		lua_pushvalue(L, arg);	// ..., other
	}

	// At this point, invoke the object's own logic to convert it.
	luaL_argcheck(L, bIsConvertible && luaL_getmetafield(L, -1, "asMatrix"), arg, "Type has no conversion method");	// ..., other, asMatrix
	lua_insert(L, -2);	// ..., asMatrix, other

	TypeData<R>::Get(L, GetTypeData::eCreateIfMissing)->BindDatumAndCall(L, 1, 0, temp);// ...

	return temp;
}

// Fetches an item from the stack, resolving either to a matrix or a scalar.
template<typename R> struct ArgObjectR {
	R mTemp, * mObject;	// "object" for conformity with ArgObject
	typename R::Scalar mScalar;	// q.v. ArgObject

	ArgObjectR (lua_State * L, int arg)
	{
		mObject = SetTemp(L, &mTemp, arg, true);

		if (!mObject)
		{
			if (lua_isuserdata(L, arg))
			{
				luaL_argcheck(L, luaL_getmetafield(L, arg, "__bytes"), arg, "Invalid scalar userdata");	// ..., object, ..., __bytes
				lua_pop(L, 1);	// ..., object, ...
			}

			mScalar = AsScalar<R>(L, arg);
		}
	}
};

// Gets two matrices from items on the stack, where at least one is assumed to resolve to
// a matrix type. Scalar items are converted to constant matrices, whereas otherwise the
// ArgObjectR logic is applied.
template<typename R> struct TwoMatrices {
	R mK, * mMat1, * mMat2;
	ArgObjectR<R> mO1, mO2;

	static void CheckTwo (lua_State * L, bool bOK, int arg1 = 1, int arg2 = 2)
	{
		if (!bOK) luaL_error(L, "At least one of arguments %d and %d must resolve to a matrix", arg1, arg2);
	}

	TwoMatrices (lua_State * L, int arg1 = 1, int arg2 = 2) : mO1{L, arg1}, mO2{L, arg2}
	{
		if (mO1.mObject && mO2.mObject)
		{
			mMat1 = mO1.mObject;
			mMat2 = mO2.mObject;
		}

		else if (mO1.mObject)
		{
			mMat1 = mO1.mObject;
			mMat2 = &mK;

			mK.setConstant(mMat1->rows(), mMat1->cols(), mO2.mScalar);
		}

		else
		{
			CheckTwo(L, mO2.mObject != nullptr, arg1, arg2);

			mMat1 = &mK;
			mMat2 = mO2.mObject;

			mK.setConstant(mMat2->rows(), mMat2->cols(), mO1.mScalar);
		}
	}
};

// Perform some binary operation from items on the stack where at least one of them is
// assumed to resolve to a matrix type. Scalars are left as is, whereas matrices are
// found via the ArgObjectR approach.
template<typename R, typename MM, typename MS, typename SM> R WithMatrixScalarCombination (lua_State * L, MM && both, MS && mat_scalar, SM && scalar_mat, int arg1, int arg2)
{
	ArgObjectR<R> o1{L, arg1}, o2{L, arg2};

	if (!o2.mObject) return mat_scalar(*o1.mObject, o2.mScalar);
	else if (!o1.mObject) return scalar_mat(o1.mScalar, *o2.mObject);
	else
	{
		TwoMatrices<R>::CheckTwo(L, o1.mObject != nullptr && o2.mObject != nullptr, arg1, arg2);

		return both(*o1.mObject, *o2.mObject);
	}
}

namespace detail {
    
    template<typename T, typename V, bool = Eigen::NumTraits<typename T::Scalar>::IsComplex> struct Make {
        static V Do (lua_State * L, int n)
        {
            using RealV = MatrixOf<typename Eigen::NumTraits<typename T::Scalar>::Real, V::RowsAtCompileTime, V::ColsAtCompileTime>;
        
            typename T::Scalar low = AsScalar<T>(L, 2), high = AsScalar<T>(L, 3);
            V cv;
        
            cv.resize(n);
        
            cv.real() = RealV::LinSpaced(n, low.real(), high.real());
            cv.imag() = RealV::LinSpaced(n, low.imag(), high.imag());
        
            return cv;
        }
    };

    template<typename T, typename V> struct Make<T, V, false> {
        static V Do (lua_State * L, int n)
        {
            return V::LinSpaced(n, AsScalar<T>(L, 2), AsScalar<T>(L, 3));
        }
    };
}

// Veneer over the LinSpaced factory that also allows for complex types.
template<typename T, int R, int C> struct LinSpacing {
    using V = Eigen::Matrix<typename T::Scalar, R, C>;
    
    static V Make (lua_State * L, int n)
    {
        return detail::Make<T, V>::Do(L, n);
    }
};

// Common forms for instantiating dependent types.
#define NEW_REF1_NO_RET(T, KEY, INPUT)	New<T>(L, INPUT);	/* source, ..., new_object */	\
										TypeData<T>::Get(L)->RefAt(L, KEY, 1)
#define NEW_REF1(T, KEY, INPUT)	NEW_REF1_NO_RET(T, KEY, INPUT);	/* source, ..., new_object */	\
																								\
								return 1
#define NEW_REF1_DECLTYPE(KEY, INPUT) NEW_REF1(decltype(INPUT), KEY, INPUT) /* source, ..., new_object */
#define NEW_REF1_DECLTYPE_MOVE(KEY, INPUT) NEW_REF1(decltype(INPUT), KEY, std::move(INPUT)) /* source, ..., new_object */

// Helper to transpose an object without needlessly creating types.
template<typename T> struct Transposer {
	static int Do (lua_State * L)
	{
		NEW_REF1_DECLTYPE("transposed_from", GetInstance<T>(L)->transpose());	// object, transp
	}
};

template<typename T> int TransposedFrom (lua_State * L)
{
	return TypeData<T>::Get(L)->GetRef(L, "transposed_from", 1);
}

template<typename U> struct Transposer<Eigen::Transpose<U>> {
	static int Do (lua_State * L)
	{
		return TransposedFrom<Eigen::Transpose<U>>(L);
	}
};
