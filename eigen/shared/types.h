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

// Alias a few recurring cases.
template<typename T> using ColVector = Eigen::Matrix<T, Eigen::Dynamic, 1>;
template<typename T> using RowVector = Eigen::Matrix<T, 1, Eigen::Dynamic>;
template<typename T, int O = 0, typename S = Eigen::Stride<0, 0>> using MappedColVector = Eigen::Map<ColVector<T>, O, S>;
template<typename T, int O = 0, typename S = Eigen::Stride<0, 0>> using MappedRowVector = Eigen::Map<RowVector<T>, O, S>;
template<typename S, int Rows = Eigen::Dynamic, int Cols = Eigen::Dynamic> using MatrixOf = Eigen::Matrix<S, Rows, Cols>;

// Matrices of booleans.
typedef MatrixOf<bool> BoolMatrix;

// Trait to detect expression types, as these often require special handling.
template<typename T> struct IsXpr : std::false_type {};
template<typename U, int R, int C, bool B> struct IsXpr<Eigen::Block<U, R, C, B>> : std::true_type {};
template<typename U, int I> struct IsXpr<Eigen::Diagonal<U, I>> : std::true_type {};
template<typename U, int Size> struct IsXpr<Eigen::VectorBlock<U, Size>> : std::true_type {};

// Trait to detect maps.
template<typename T> struct IsMap : std::false_type {};
template<typename U, int O, typename S> struct IsMap<Eigen::Map<U, O, S>> : std::true_type {};

// Trait to detect matrices.
template<typename T> struct IsMatrix : std::false_type {};
template<typename U, int Rows, int Cols, int Options, int MaxRows, int MaxCols> struct IsMatrix<Eigen::Matrix<U, Rows, Cols, Options, MaxRows, MaxCols>> : std::true_type {};

// Trait to detect "basic" configuations and prevent runaway compilation from certain methods.
template<typename T> struct IsBasic : IsMatrix<T> {};
template<typename U, int O, typename S> struct IsBasic<Eigen::Map<U, O, S>> : IsMatrix<U> {};

// Extension of the last trait to add simple blocks.
template<typename T> struct IsBasicMaybeBlock : IsBasic<T> {};
template<typename U, int R, int C, bool B> struct IsBasicMaybeBlock<Eigen::Block<U, R, C, B>> : IsMatrix<U> {};

// Trait to detect maps with non-trivial strides.
template<typename T> struct HasNormalStride : std::false_type {};
template<typename T, int Rows, int Cols, int Options, int MaxRows, int MaxCols> struct HasNormalStride<Eigen::Matrix<T, Rows, Cols, Options, MaxRows, MaxCols>> : std::true_type {};
template<typename U> struct HasNormalStride<Eigen::Map<U>> : std::true_type {};

// Trait that flags whether some type-related code should be generated (within this module)
// for the matrix family in question.
template<typename R> struct IsMatrixFamilyImplemented : std::false_type {};

// Associate some numeric constants with each scalar type. Also, count the number
// of bits needed to pack them into a bitfield in the type data.
// See e.g. https://hbfs.wordpress.com/2016/03/22/log2-with-c-metaprogramming/
template<int N> struct CompileTimeBits : std::integral_constant<int, CompileTimeBits<N / 2>::value + 1> {};
template<> struct CompileTimeBits<1> : std::integral_constant<int, 1> {};

enum ScalarType {
	eInt, eFloat, eDouble, eCfloat, eCdouble, eBool,
	kNumTypes,
	kBits = CompileTimeBits<kNumTypes>::value
};

template<typename S> struct GetScalarType {};
template<> struct GetScalarType<int> : std::integral_constant<ScalarType, eInt> {};
template<> struct GetScalarType<float> : std::integral_constant<ScalarType, eFloat> {};
template<> struct GetScalarType<double> : std::integral_constant<ScalarType, eDouble> {};
template<> struct GetScalarType<std::complex<float>> : std::integral_constant<ScalarType, eCfloat> {};
template<> struct GetScalarType<std::complex<double>> : std::integral_constant<ScalarType, eCdouble> {};
template<> struct GetScalarType<bool> : std::integral_constant<ScalarType, eBool> {};

// Keys to some global state for the type system.
#define EIGEN_META_TO_TYPE_DATA_KEY "EIGEN::META_TO_TYPE_DATA"
#define EIGEN_NEW_TYPE_KEY "EIGEN::NEW_TYPE"

// Various ways to acquire type data that might not yet be present. This is a base type in
// order to avoid dealing with templates for simple lookups, as well as to service any
// logic that must be called in the absence of a known type.
struct GetTypeData {
	enum Options { eDoNothing, eCreateIfMissing, eFetchIfMissing };

	struct Info {
		bool mIsConvertible : 1;// Can this be converted to a MatrixOf<Scalar>?
		bool mIsPrimitive : 1;	// For that matter, is the type MatrixOf<Scalar>?
		ScalarType mType : kBits;	// The type corresponding to Scalar
	};

	int mSelectRef{LUA_NOREF};	// Method used to select some matrix / scalar combination
	Info mInfo;	// Some information about the type
	const char * mName;	// Cached full name
	void * mDatum{nullptr};	// Pointer to transient datum for some quick operations

	const Info & GetInfo (void) const { return mInfo; }
	const char * GetName (void) const { return mName; }

	// Helper to make datum-dependent calls that are robust against errors.
	void BindDatumAndCall (lua_State * L, int nargs, int nresults, void * datum)
	{
		mDatum = datum;

		bool bOK = lua_pcall(L, nargs, nresults, 0) == 0;	// ..., results / err

		mDatum = nullptr;

		if (!bOK) lua_error(L);
	}

	// Find a type data for an Eigen object on the stack.
	static GetTypeData * FromObject (lua_State * L, int arg)
	{
		if (!lua_getmetatable(L, arg)) return nullptr;	// ...[, meta]

		lua_getfield(L, LUA_REGISTRYINDEX, EIGEN_META_TO_TYPE_DATA_KEY);// ..., meta, meta_to_type_data
		lua_insert(L, -2);	// ..., meta_to_type_data, meta
		lua_rawget(L, -2);	// ..., meta_to_type_data, type_data?

		GetTypeData * td = LuaXS::UD<GetTypeData>(L, -1);

		lua_pop(L, 2);	// ...

		return td;
	}

	// Invoke select logic for the type.
	int Select (lua_State * L)
	{
		luaL_argcheck(L, mSelectRef != LUA_NOREF, -1, "Type does not support select()");
		lua_getref(L, mSelectRef);	// bm, then, else, select
		lua_insert(L, 1);	// select, bm, then, else
		lua_settop(L, 4);	// select, bm, then, else
		lua_call(L, 3, 1);	// selection

		return 1;
	}
};

//
template<typename T> struct AuxTypeName;
template<typename T> const char * TypeName (lua_State *);
template<typename T, typename U> int NewRet (lua_State *, U &&);
template<typename T> T * GetInstance (lua_State *, int = -1);
template<typename R> R GetInstanceEx (lua_State *, int = 1);
template<typename T, typename R = MatrixOf<typename T::Scalar>> struct TypeData;

//
namespace detail {
	//
	template<typename R> static int Select (lua_State * L)
	{
		auto bm = LuaXS::UD<BoolMatrix>(L, 1); // see the note in BoolMatrix::select()

		return NewRet<R>(L, WithMatrixScalarCombination<R>(L, [bm](const R & m1, const R & m2) {
			return bm->select(m1, m2);
		}, [bm](const R & m, const typename R::Scalar & s) {
			return bm->select(m, s);
		}, [bm](const typename R::Scalar & s, const R & m) {
			return bm->select(s, m);
		}, 2, 3));
	}

    // Add push and select functions, allowing interop with code that may be in other modules.
    // The instance in question is converted to a temporary, so these are only available for
    // types where this is possible.
    template<typename T, typename R, bool = std::is_convertible<T, R>::value> struct AddPushAndSelect {
        AddPushAndSelect (lua_State * L, TypeData<T> * td)
        {
            lua_pushcfunction(L, [](lua_State * L) {
                return NewRet<R>(L, *LuaXS::UD<T>(L, 1));
            });	// meta, push
            lua_pushcfunction(L, Select<R>);// meta, push, select
            
            td->mSelectRef = lua_ref(L, 1);	// ..., push; registry = { ..., ref = select }
            td->mPushRef = lua_ref(L, 1);	// ...; registry = { ..., select, ref = push }
        }
    };
    
    template<typename T, typename R> struct AddPushAndSelect<T, R, false> {
        AddPushAndSelect (lua_State *, TypeData<T> *) {}
    };

    //
    template<typename T> struct OnCache : std::false_type {
        static int Do (lua_State * L) { return 0; }
    };
    
    template<typename U, int Rows, int Cols, int Options, int MaxRows, int MaxCols> struct OnCache<Eigen::Matrix<U, Rows, Cols, Options, MaxRows, MaxCols>> : std::true_type {
        static int Do (lua_State * L)
        {
            using M = Eigen::Matrix<U, Rows, Cols, Options, MaxRows, MaxCols>;
            
            *LuaXS::UD<M>(L, 1) = M{};
            
            return 0;
        }
    };
    
    //
    template<typename TD, typename R, bool = IsMatrixFamilyImplemented<R>::value> struct Create {
		static TD * Do (lua_State * L)
		{
			luaL_error(L, "Unable to implement %s\n", lua_tostring(L, -1));

			return nullptr;
		}
	};

	template<typename T, typename R> struct Create<TypeData<T>, R, true> {
        static TypeData<T> * Do (lua_State * L)
        {
            // Register the new type data. Its metatable is a few steps away from existence, i.e.
            // in New(), but since nothing external will need it beforehand, we can use the name
            // as a provisional key.
            lua_getfield(L, LUA_REGISTRYINDEX, EIGEN_META_TO_TYPE_DATA_KEY);// ..., name, meta_to_type_data
            lua_pushvalue(L, -2);	// ..., name, meta_to_type_data, name
            
            auto td = LuaXS::NewTyped<TypeData<T>>(L);// ..., name, meta_to_type_data, name, td
            
            lua_rawset(L, -3);	// ..., meta_to_type_data = { ..., name = td }
            lua_pop(L, 1);	// ...
            
            // Fetch the cache binding logic and wire the new type into the cache. Some types can
            // safely do some cleanup when cached, rather than when being fetched, in which cases
            // this additional logic is supplied as well.
            lua_getfield(L, LUA_REGISTRYINDEX, EIGEN_NEW_TYPE_KEY);	// ..., name, new_type
            
            if (OnCache<T>::value) lua_pushcfunction(L, OnCache<T>::Do);	// ..., name, new_type[, on_cache]
            
            lua_call(L, OnCache<T>::value ? 1 : 0, 1);	// ..., name, CacheFunc
            
            td->mCacheFuncRef = lua_ref(L, 1);	// ..., name
            
            // Hook up routines to push a matrix, e.g. from another shared library, and with similar
            // reasoning to use such matrices in BoolMatrix::select().
            AddPushAndSelect<T, R> apas{L, td};
            
            // Capture some information needed when the exact type is unknown.
            td->mInfo.mIsConvertible = std::is_convertible<T, R>::value;
            td->mInfo.mIsPrimitive = std::is_same<T, R>::value;
            td->mInfo.mType = GetScalarType<typename R::Scalar>::value;
            
            // Save the name for use down the road.
            td->mName = lua_tostring(L, -1);
            
            return td;
        }
    };
}

// Get a given type's data, if present. Since lookup involves building up a name, the pointer
// is saved for subsequent calls. The most common use cases are HasType(), GetInstance(), and
// GetInstanceEx(), where the type will already be loaded on account of an instance existing,
// or in the first case its very absence is enough to answer the question. On account of this,
// the default is to not try to add an absent type. Special options exist to accommodate
// object creation as well as operations like cast(), select(), and complex <-> real functions
// that might cross shared library boundaries and only need to fetch already-loaded type data.
template<typename TD, typename R> struct GetTypeDataInstance : GetTypeData {
	static TD * Get (lua_State * L, Options opts = eDoNothing)
	{
		static ThreadXS::TLS<TD *> sThis;

		if (!sThis && (opts == eCreateIfMissing || opts == eFetchIfMissing))
		{
			int top = lua_gettop(L);

			// Build up the type's name.
			luaL_Buffer buff;

			luaL_buffinit(L, &buff);
			luaL_addstring(&buff, "eigen.");

			AuxTypeName<typename TD::Type>(&buff, L);

			luaL_pushresult(&buff);	// ..., name

			// If the library is loaded in parts, e.g. as eigencore + eigendouble, the type
			// might already exist. An instantiated type will have a named metatable, whereas
			// one that has only been created will be temporarily registered under its name.
			// If either approach yields the type, our work is done.
			lua_getfield(L, LUA_REGISTRYINDEX, EIGEN_META_TO_TYPE_DATA_KEY);// ..., name, meta_to_type_data
			luaL_getmetatable(L, lua_tostring(L, -2));	// ..., name, meta_to_type_data, meta?
			lua_rawget(L, -2);	// ..., name, meta_to_type_data, td?

			if (lua_isnil(L, -1))
			{
				lua_pushvalue(L, -3);	// ..., name, meta_to_type_data, nil, name
				lua_rawget(L, -3);	// name, meta_to_type_data, nil, td?
			}

			sThis = LuaXS::UD<TD>(L, -1);

			// Failing that, create the type on demand.
			lua_settop(L, top + 1);	// ..., name

            if (!sThis && opts == eCreateIfMissing) sThis = detail::Create<TD, R>::Do(L);

			lua_settop(L, top);	// ...
		}

		return sThis;
	}
};

// Per-type data.
template<typename T, typename R> struct TypeData : GetTypeDataInstance<TypeData<T>, R> {
	typedef T Type;

	int mCacheFuncRef;	// Function used to interact with the cache
	int mPushRef{LUA_NOREF};// Function used to push instance on stack
	int mVectorRingRef{LUA_NOREF};	// Used by vector blocks to maintain a transient ring of vectors

	// If the object on the stack is weakly keyed to an item in the given category, pushes it
	// onto the stack; otherwise, pushes nil. (This is useful for objects like maps and views
	// that must keep around some sort of "parent" object.)
	int GetRef (lua_State * L, const char * category, int arg) const
	{
		arg = CoronaLuaNormalize(L, arg);

		lua_getref(L, mCacheFuncRef);	// ..., CacheFunc
		lua_pushliteral(L, "get_ref");	// ..., CacheFunc, "get_ref"
		lua_pushvalue(L, arg);	// ..., CacheFunc, "get_ref", object
		lua_pushstring(L, category);// ..., CacheFunc, "get_ref", object, category
		lua_call(L, 3, 1);	// ..., value?

		return 1;
	}

	// Given an object on the stack, weakly keys it in a given category to the item on the
	// top of the stack, removing the item in the process.
	void Ref (lua_State * L, const char * category, int arg)
	{
		luaL_argcheck(L, arg != -1 && arg != lua_gettop(L), arg, "Attempt to ref self");
		lua_pushvalue(L, arg);	// ..., item_to_ref, object
		lua_pushliteral(L, "ref");	// ..., item_to_ref, object, "ref"
		lua_insert(L, -2);	// ..., item_to_ref, "ref, object
		lua_pushstring(L, category);// ..., item_to_ref, "ref", object, category
		lua_pushvalue(L, -4);	// ..., item_to_ref, "ref", object, category, item_to_ref
		lua_getref(L, mCacheFuncRef);	// ..., item_to_ref, "ref", object, category, item_to_ref, CacheFunc
		lua_replace(L, -6);	// ..., CacheFunc, "ref", object, category, item_to_ref
		lua_call(L, 4, 0);	// ...
	}

	// Variant of Ref() that looks for the item somewhere other than the top of the stack,
	// with that instead being the object's default position. The stack is left intact.
	void RefAt (lua_State * L, const char * category, int pos, int arg = -1)
	{
		arg = CoronaLuaNormalize(L, arg);

		lua_pushvalue(L, pos);	// ..., arg, ..., value

		Ref(L, category, arg);	// ..., arg, ...
	}

	// Given an object on the stack, removes the value (if any) to which it is weakly keyed in
	// a given category. (Currently unused.)
	void Unref (lua_State * L, const char * category, int arg)
	{
		arg = CoronaLuaNormalize(L, arg);

		lua_getref(L, mCacheFuncRef);	// ..., CacheFunc
		lua_pushliteral(L, "unref");// ..., CacheFunc, "unref"
		lua_pushvalue(L, arg);	// ..., CacheFunc, "unref", object
		lua_pushstring(L, category);// ..., CacheFunc, "unref", object, category
		lua_call(L, 3, 0);	// ...
	}
};

// Does the object on the stack have the given type?
template<typename T> bool HasType (lua_State * L, int arg)
{
	auto td = TypeData<T>::Get(L);

	return td != nullptr && td == GetTypeData::FromObject(L, arg);
}

// Helper to convert an object to a matrix, say for an "asMatrix" method. This also has a
// special case for internal use that populates a stack-based temporary.
template<typename T, typename R> int AsMatrix (lua_State * L)
{
	auto td = TypeData<R>::Get(L);
	T & object = *GetInstance<T>(L);

	if (!td->mDatum) return NewRet<R>(L, object);

	else
	{
		*static_cast<R *>(td->mDatum) = object;

		return 0;
	}
}

// This is specialized to populate a given type's methods and properties.
template<typename T, typename R = MatrixOf<typename T::Scalar>> struct AttachMethods {
	AttachMethods (lua_State * L)
	{
		luaL_error(L, "Unsupported type: %s", TypeName<T>(L));
	}
};

// Instantiate a type, rigging up the type itself on the first call.
template<typename T, typename ... Args> T * New (lua_State * L, Args && ... args)
{
	// Try to fetch an instance from the cache. If found, reuse its memory.
	auto td = TypeData<T>::Get(L, GetTypeData::eCreateIfMissing);

	lua_getref(L, td->mCacheFuncRef);	// ..., CacheFunc
	lua_pushliteral(L, "fetch");// ..., CacheFunc, "fetch"
	lua_call(L, 1, 1);	// ..., object?

	T * object = nullptr;

	if (!lua_isnil(L, -1)) object = LuaXS::UD<T>(L, -1);

	else lua_pop(L, 1);	// ...

	if (object)
	{
		LuaXS::DestructTyped<T>(L, -1);

		new (object) T(std::forward<Args>(args)...);
	}

	// Otherwise, add a new object. If the type itself is new, attach its methods as well.
	else
	{
		object = LuaXS::NewTyped<T>(L, std::forward<Args>(args)...);// ..., object

		LuaXS::AttachMethods(L, td->GetName(), [](lua_State * L) {
			AttachMethods<T> am{L};

			// Now that we have a metatable, patch the type data lookup.
			auto td = TypeData<T>::Get(L);

			lua_getfield(L, LUA_REGISTRYINDEX, EIGEN_META_TO_TYPE_DATA_KEY);// ..., meta, meta_to_type_data
			lua_pushvalue(L, -2);	// ..., meta, meta_to_type_data, meta
			lua_getfield(L, -2, td->GetName());	// ..., meta, meta_to_type_data, meta, td
			lua_rawset(L, -3);	// ..., meta, meta_to_type_data = { ..., [meta] = td }
			lua_pushnil(L);	// ..., meta, meta_to_type_data, nil
			lua_setfield(L, -2, td->GetName());	// ..., meta, meta_to_type_data = { ..., [meta] = td, name = nil }
			lua_pop(L, 1);	// ..., meta

			// Without exception, add a GC metamethod.
			LuaXS::AttachGC(L, LuaXS::TypedGC<T>);

			// Add a type name getter method, allowing for interal queries for the type key.
			lua_pushstring(L, td->GetName());	// ..., meta, name
			lua_pushcclosure(L, [](lua_State * L) {
				lua_pushvalue(L, lua_upvalueindex(1));	// [object, ]name

				return 1;
			}, 1);	// ..., meta, GetTypeName
			lua_setfield(L, -2, "getTypeName");	// ..., meta = { ..., getTypeName = GetTypeName }
		});
	}

	// If caching is active, register the object for later reclamation.
	lua_getref(L, td->mCacheFuncRef);	// ..., object, CacheFunc
	lua_pushliteral(L, "register");	// ..., object, CacheFunc, "register"
	lua_pushvalue(L, -3);	// ..., object, CacheFunc, "register", object
	lua_call(L, 2, 0);	// ..., object

	return object;
}

// Helper for the common case where a new instance is immediately returned.
template<typename T, typename U> int NewRet (lua_State * L, U && m)
{
	New<T>(L, std::forward<U>(m));

	return 1;
}

// Variant of NewRet that constructs the new instance as well.
template<typename T, typename ... Args> int NewRvalue (lua_State * L, Args && ... args)
{
	New<T>(L, T(std::forward<Args>(args)...));

	return 1;
}

// Helper to ensure a matrix has vector shape.
template<typename T> void CheckVector (lua_State * L, const T & mat, int arg)
{
	luaL_argcheck(L, mat.cols() == 1 || mat.rows() == 1, arg, "Non-vector: row and column counts both exceed 1");
}

namespace detail {
    template<typename R, bool bRow> struct RowShape {
        // Does the object have the shape appropriate to the vector type?
        static bool IsCorrect (R * object)
        {
            return object->rows() == 1;
        }
        
        // Attach the appropriately-typed vector object to the reference.
        template<typename Type> static void Bind (std::unique_ptr<Type> & ref, R * object)
        {
            ref.reset(new Type{object->row(0)});
        }
    };
    
    template<typename R> struct RowShape<R, false> {
        static bool IsCorrect (R * object)
        {
            return object->cols() == 1;
        }
    
        template<typename Type> static void Bind (std::unique_ptr<Type> & ref, R * object)
        {
            ref.reset(new Type{object->col(0)});
        }
    };
}

// Structure a matrix or similar object as a vector via Eigen's Ref type.
template<typename R, bool bRow> struct VectorRef {
	using V = typename std::conditional<bRow,
		RowVector<typename R::Scalar>,
		ColVector<typename R::Scalar>
	>::type;
	using Type = typename std::conditional<bRow,
		Eigen::Ref<V, 0, Eigen::InnerStride<>>,
		Eigen::Ref<V>
	>::type;

	std::unique_ptr<Type> mRef;	// Reference to an object to interpret as a vector
	R mTemp;// Temporary in cases of row vectors or complex types
	bool mChanged;	// Did we have to resort to a temporary?
	bool mTransposed;	// Did we also need to transpose that temporary?

    VectorRef (VectorRef && vr) : mRef{std::move(vr.mRef)}, mTemp{std::move(vr.mTemp)}, mChanged{vr.mChanged}, mTransposed{vr.mTransposed}
    {
    }
    
	bool Changed (void) const { return mChanged; }

	// Add some operators rather than methods for convenient Ref access.
	Type & operator * (void)
	{
		return *mRef;
	}

	Type * operator -> (void)
	{
		return mRef.get();
	}

	// When returning or updating a vector, we generally want the end result to have the same
	// shape as the source, even though it was convenient to work in one form. This will 
	void RestoreShape (R * object = nullptr)
	{
		if (!object) object = &mTemp;
		if (mTransposed) object->transposeInPlace();
	}

	// Allow stashing in data structures.
	VectorRef (void) = default;

	// Bind a vector, possibly via a temporary. This is separate from the constructor for
	// cases when these are inside other data structures.
	void Init (lua_State * L, int arg = 1)
	{
        using Shape = detail::RowShape<R, bRow>;

		GetTypeData * td = GetTypeData::FromObject(L, arg);

		luaL_argcheck(L, td && td->GetInfo().mIsConvertible, arg, "Not a convertible Eigen object");

		R * ptr = td->GetInfo().mIsPrimitive ? GetInstance<R>(L, arg) : nullptr;

		mChanged = mTransposed = false;

        if (!ptr || !Shape::IsCorrect(ptr))
		{
			mTemp = ptr ? *ptr : GetInstanceEx<R>(L, arg);
			ptr = &mTemp;

			if (!Shape::IsCorrect(&mTemp))
			{
				mTemp.transposeInPlace();

				mTransposed = true;
			}

			luaL_argcheck(L, Shape::IsCorrect(&mTemp), arg, "Non-vector: row and column counts both exceed 1");
		}

        Shape::Bind(mRef, ptr);
	}

	VectorRef (lua_State * L, int arg = 1)
	{
		Init(L, arg);
	}
};

// Column vectors are used pervasively, so alias them.
template<typename R> using ColumnVector = VectorRef<R, false>;

// Acquire an instance whose exact type is expected.
template<typename T> T * GetInstance (lua_State * L, int arg)
{
	luaL_argcheck(L, HasType<T>(L, arg), arg, "Instance does not have the specified type");

	return LuaXS::UD<T>(L, arg);
}

// Acquire an instance resolved to a matrix type, with shortcuts for common source types.
template<typename R> R GetInstanceEx (lua_State * L, int arg)
{
	R out;

	if (HasType<Eigen::Block<R>>(L, arg)) out = *LuaXS::UD<Eigen::Block<R>>(L, arg);
	else if (HasType<Eigen::Transpose<R>>(L, arg)) out = *LuaXS::UD<Eigen::Transpose<R>>(L, arg);
	else SetTemp<R, true>(L, &out, arg);

	return out;
}

// Standard boilerplate for getters. Aside from the case of binary metamethods, the first
// type in a given method will be a "self" whose type we can assume, and thus we return a
// pointer to an instance of that very type. Other arguments are generally given some leeway,
// since they might be blocks, maps, transposes, etc. In these cases, the object is copied
// to a new matrix (of some commonly resolved type) that gets returned; this is potentially
// expensive, but offers flexibility. (The "R" originally stood for "return value", though
// "raw", "right-hand side", "reduced", and "resolved" tend to fit as well.)
namespace detail {
    template<typename> struct NonBasicReject : std::true_type {};
    template<typename U, int Rows, int Cols, bool B> struct NonBasicReject<Eigen::Block<U, Rows, Cols, B>> : IsMap<U> {};
    template<typename U> struct NonBasicReject<Eigen::Transpose<U>> : IsMap<U> {};
}

template<typename T, typename R> struct InstanceGetters {
	static T * GetT (lua_State * L, int arg = 1) { return GetInstance<T>(L, arg); }
	static R GetR (lua_State * L, int arg = 1) { return GetInstanceEx<R>(L, arg); }

	//
	using ArraySourceType = typename std::conditional<
        IsBasic<T>::value || !detail::NonBasicReject<T>::value,
		const T &,
		R
	>::type;

	//
	using ArrayType = decltype(std::declval<ArraySourceType>().array());

	//
	template<typename WA> static int WithArray (lua_State * L, WA && body)
	{
		ArraySourceType as = *GetT(L);

		body(as.array());

		return 1;
	}

	//
	using RefArgType = typename std::conditional<
		std::is_same<T, R>::value,
		//std::is_constructible<Eigen::Ref<const R>, const T &>::value,
		// TODO: Visual Studio seems to bail on this version ^^^, but is it viable?
		const T &,
		R
	>::type;

	//
    using RefType = R;
	// using RefType = Eigen::Ref<const R>; // TODO: see note for RefArgType

	template<typename WR> static int WithRef (lua_State * L, WR && body)
	{
		RefArgType rarg = *GetT(L);

		body(rarg);

		return 1;
	}
};

//
template<typename T> struct Nested {
	using Type = T;
};

template<typename U, int O, typename S> struct Nested<Eigen::Map<U, O, S>> {
	using Type = U;
};

template<typename U, int O, typename S> struct Nested<Eigen::Ref<U, O, S>> {
	using Type = U;
};

template<typename U, int R, int C, bool B> struct Nested<Eigen::Block<U, R, C, B>> {
	using Type = U;
};

template<typename U> struct Nested<Eigen::Transpose<U>> {
	using Type = U;
};

//
template<typename T> struct IsLvalue : Eigen::internal::is_lvalue<typename Nested<T>::Type> {};
template<typename U> struct IsLvalue<const U> : std::false_type {};

namespace detail {
    //
    template<typename T> struct FromType {
        using N = typename Nested<T>::Type;
        using Type = T *;
        
        static T * Get (lua_State * L, int arg)
        {
            return GetInstance<T>(L, arg);
        }
    };
    
    template<typename U, bool B> struct FromType<VectorRef<U, B>> {
        using N = typename VectorRef<U, B>::V;
        using Type = VectorRef<U, B>;

        static VectorRef<U, B> Get (lua_State * L, int arg)
        {
            return std::move(VectorRef<U, B>{L, arg});
        }
    };
}

// 
template<typename T> struct TempRAII {
    using FT = typename detail::FromType<T>;
    using N = typename FT::N;
	using R = MatrixOf<typename N::Scalar, N::RowsAtCompileTime, N::ColsAtCompileTime>;

    typename FT::Type mFrom;   //
	R mTemp;//

	//
	R & operator * (void)
	{
		return mTemp;
	}

	R * operator -> (void)
	{
		return &mTemp;
	}

	//
	TempRAII (lua_State * L, int arg = 1) : mFrom{FT::Get(L, arg)}, mTemp{*mFrom}
	{
	}
    
    //
    TempRAII (TempRAII && other) : mFrom{std::move(other.mFrom)}, mTemp{std::move(other.mTemp)}
    {
    }
    
	//
	~TempRAII (void)
	{
		*mFrom = mTemp;
	}
};

//
template<typename T, typename R> struct TempInstanceGetters : InstanceGetters<TempRAII<T>, R>
{
	static TempRAII<T> GetT (lua_State * L, int arg = 1)
	{
        return std::move(TempRAII<T>{L, arg});
	}
};

// The following builds up a name piecemeal by unraveling important pieces of the type. In
// theory <typeinfo> might do as well, but quoting http://en.cppreference.com/w/cpp/types/type_info/name,
// "No guarantees are given, in particular, the returned string can be identical for several
// types and change between invocations of the same program.", which is especially relevant
// since the implementation may be spread among two or more shared libraries (with a common
// Lua state), e.g. eigencore + eigendouble.
template<typename T> struct AuxTypeName {
	AuxTypeName (luaL_Buffer * B, lua_State * L)
	{
		luaL_pushresult(B);	// ..., result
		luaL_error(L, "Unsupported type; current progress: %s", lua_tostring(L, -1));
	}
};

template<typename T> struct AuxTypeName<const T> {
	AuxTypeName (luaL_Buffer * B, lua_State * L)
	{
		luaL_addstring(B, "const ");

		AuxTypeName<T> atn{B, L};
	}
};

template<> struct AuxTypeName<bool> {
	AuxTypeName (luaL_Buffer * B, lua_State *) { luaL_addstring(B, "bool"); }
};

template<> struct AuxTypeName<int> {
	AuxTypeName (luaL_Buffer * B, lua_State *) { luaL_addstring(B, "int"); }
};

template<> struct AuxTypeName<float> {
	AuxTypeName (luaL_Buffer * B, lua_State *) { luaL_addstring(B, "float"); }
};

template<> struct AuxTypeName<double> {
	AuxTypeName (luaL_Buffer * B, lua_State *) { luaL_addstring(B, "double"); }
};

template<> struct AuxTypeName<std::complex<float>> {
	AuxTypeName (luaL_Buffer * B, lua_State *) { luaL_addstring(B, "cfloat"); }
};

template<> struct AuxTypeName<std::complex<double>> {
	AuxTypeName (luaL_Buffer * B, lua_State *) { luaL_addstring(B, "cdouble"); }
};

void AddComma (luaL_Buffer * B)
{
	luaL_addstring(B, ", ");
}

void AddFormatted (luaL_Buffer * B, lua_State * L, const char * fmt, int arg)
{
	lua_pushfstring(L, fmt, arg);	// ..., arg
	luaL_addvalue(B);	// ...
}

void AddFormatted (luaL_Buffer * B, lua_State * L, const char * fmt, const char * arg)
{
	lua_pushfstring(L, fmt, arg);	// ..., arg
	luaL_addvalue(B);	// ...
}

void CloseType (luaL_Buffer * B)
{
	luaL_addstring(B, ">");
}

void OpenType (luaL_Buffer * B, const char * name)
{
	luaL_addstring(B, name);
	luaL_addstring(B, "<");
}

void AddDynamicOrN (luaL_Buffer * B, lua_State * L, int n)
{
	if (n == Eigen::Dynamic) luaL_addstring(B, "Dynamic");
	else AddFormatted(B, L, "%d", n);
}

template<int Outer, int Inner> struct AuxTypeName<Eigen::Stride<Outer, Inner>> {
	AuxTypeName (luaL_Buffer * B, lua_State * L)
	{
		if (Outer == 0 && Inner == 0) return;

		OpenType(B, ", Stride");
		AddFormatted(B, L, "%d", Outer);
		AddFormatted(B, L, ", %d", Inner);
		CloseType(B);
	}
};

template<int Value> struct AuxTypeName<Eigen::InnerStride<Value>> {
	AuxTypeName (luaL_Buffer * B, lua_State * L)
	{
		OpenType(B, ", InnerStride");

		if (Value != Eigen::Dynamic) AddFormatted(B, L, "%d", Value);

		CloseType(B);
	}
};

template<int Value> struct AuxTypeName<Eigen::OuterStride<Value>> {
	AuxTypeName (luaL_Buffer * B, lua_State * L)
	{
		OpenType(B, ", OuterStride");

		if (Value != Eigen::Dynamic) AddFormatted(B, L, "%d", Value);

		CloseType(B);
	}
};

template<typename U, int O, typename S> struct AuxTypeName<Eigen::Ref<U, O, S>> {
	AuxTypeName (luaL_Buffer * B, lua_State * L)
	{
		OpenType(B, "Ref");

		AuxTypeName<U>(B, L);

		AddFormatted(B, L, ", %d", O);

		AuxTypeName<S>(B, L);

		CloseType(B);
	}
};

//
template<typename U> struct AuxTypeName<Eigen::Transpose<U>> {
	AuxTypeName (luaL_Buffer * B, lua_State * L)
	{
		OpenType(B, "Transpose");

		AuxTypeName<U>(B, L);

		CloseType(B);
	}
};

template<typename T> const char * TypeName (lua_State * L)
{
	luaL_Buffer B;

	luaL_buffinit(L, &B);

	AuxTypeName<T>(&B, L);

	luaL_pushresult(&B);// ..., str

	return lua_tostring(L, -1);
}
