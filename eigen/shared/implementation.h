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

#include "config.h"
#include "types.h"
#include "utils.h"
#include "matrix.h"

// Add LinSpaced*() for non-boolean matrices.
template<typename M> struct AddLinSpaced {
    AddLinSpaced (lua_State * L)
    {
        luaL_Reg methods[] = {
            {
                "LinSpaced", [](lua_State * L)
                {
                    return NewRet<M>(L, LinSpacing<M, Eigen::Dynamic, 1>::Make(L, LuaXS::Int(L, 1)));
                }
            }, {
                "LinSpacedRow", [](lua_State * L)
                {
                    return NewRet<M>(L, LinSpacing<M, 1, Eigen::Dynamic>::Make(L, LuaXS::Int(L, 1)));
                }
            },
            { nullptr, nullptr }
        };

        luaL_register(L, nullptr, methods);
    }
};

template<> struct AddLinSpaced<BoolMatrix> {
    AddLinSpaced (lua_State *) {}
};

// Add Umeyama() for real floating point matrices.
template<typename M, bool = !Eigen::NumTraits<typename M::Scalar>::IsInteger && !Eigen::NumTraits<typename M::Scalar>::IsComplex> struct AddUmeyama {
	AddUmeyama (lua_State * L)
	{
		luaL_Reg funcs[] = {
			{
				"Umeyama", [](lua_State * L)
				{
					return NewRet<M>(L, Eigen::umeyama(*GetInstance<M>(L, 1), *GetInstance<M>(L, 2), !WantsBool(L, "NoScaling", 3)));	// src, dst[, no_scaling], xform
				}
			},
			{ nullptr, nullptr }
		};

		luaL_register(L, nullptr, funcs);
	}
};

template<typename M> struct AddUmeyama<M, false> {
	AddUmeyama (lua_State *) {}
};

// Enable some type-related code generation (in this module) for appropriate matrix families.
#if defined(EIGEN_CORE) || defined(EIGEN_PLUGIN_BASIC)
	template<> struct IsMatrixFamilyImplemented<BoolMatrix> : std::true_type {};
#endif

#ifdef WANT_INT
	template<> struct IsMatrixFamilyImplemented<Eigen::MatrixXi> : std::true_type {};
#endif

#ifdef WANT_FLOAT
	template<> struct IsMatrixFamilyImplemented<Eigen::MatrixXf> : std::true_type {};
#endif

#ifdef WANT_DOUBLE
	template<> struct IsMatrixFamilyImplemented<Eigen::MatrixXd> : std::true_type {};
#endif

#ifdef WANT_CFLOAT
	template<> struct IsMatrixFamilyImplemented<Eigen::MatrixXcf> : std::true_type {};
#endif

#ifdef WANT_CDOUBLE
	template<> struct IsMatrixFamilyImplemented<Eigen::MatrixXcd> : std::true_type {};
#endif

// Supplies some front end functions for the type, in particular various matrix factories.
template<typename M> static void AddType (lua_State * L)
{
	#if defined(EIGEN_CORE) || defined(EIGEN_PLUGIN_BASIC)
		lua_newtable(L);// eigen, module
	#endif
    
    typedef typename M::Scalar Scalar;

	luaL_Reg funcs[] = {
		{
			"Constant", [](lua_State * L)
			{
				int m = LuaXS::Int(L, 1), n = luaL_optint(L, 2, m);

				return NewRet<M>(L, M::Constant(m, n, AsScalar<M>(L, 3)));	// m[, n], v, k
			}
		}, {
			"Identity", [](lua_State * L)
			{
				int m = LuaXS::Int(L, 1), n = luaL_optint(L, 2, m);

				return NewRet<M>(L, M::Identity(m, n));	// m[, n], id
			}
		},
	#ifdef WANT_MAP
		{
			"Map", [](lua_State * L)
			{
				BlobXS::State state{L, 1};
				
				int m = luaL_checkint(L, 2), n = luaL_optint(L, 3, m);
				auto memory = state.PointToDataIfBound(L, 0, 0, n, m, 0, sizeof(Scalar)); // TODO: might be resizable...

				//
				if (memory)
				{
					Eigen::Map<M> map(reinterpret_cast<Scalar *>(memory), m, n);

					NEW_REF1_DECLTYPE_MOVE("map_bytes", map);	// memory, m[, n], map
				}

				//
				else
				{
					luaL_argcheck(L, !state.Bound() && lua_objlen(L, 1) >= m * n * sizeof(Scalar), 1, "Not enough memory for requested dimensions");

					const char * str = luaL_checkstring(L, 1);

					Eigen::Map<const M> map(reinterpret_cast<const Scalar *>(str), m, n);

					NEW_REF1_DECLTYPE_MOVE("map_bytes", map);	// memory, m[, n], map
				}
			}
		}, 
	#endif
	#ifdef WANT_MAP_WITH_CUSTOM_STRIDE
		/*{
			"MapWithInnerStride", [](lua_State * L)
			{
				lua_settop(L, 4);	// memory, m[, n], stride

				ByteReader memory{L, 1}; // see MatrixFromMemory

				if (!memory.mBytes) lua_error(L);

				int m = luaL_checkint(L, 2), arg3 = luaL_checkint(L, 3), n, stride;

				if (!lua_isnil(L, 4))
				{
					stride = luaL_checkint(L, 4);
					n = arg3;
				}

				else
				{
					stride = arg3;
					n = m;
				}

				Eigen::Map<M, 0, Eigen::InnerStride<>> map(static_cast<const M::Scalar *>(memory.mBytes), m, n, stride);

				NEW_REF1_DECLTYPE_MOVE("map_bytes", map);	// memory, m[, n], stride, map
			}
		}, {
			"MapWithOuterStride", [](lua_State * L)
			{
				lua_settop(L, 4);	// memory, m[, n], stride

				ByteReader memory{L, 1}; // see MatrixFromMemory

				if (!memory.mBytes) lua_error(L);

				int m = luaL_checkint(L, 2), arg3 = luaL_checkint(L, 3), n, stride;

				if (!lua_isnil(L, 4))
				{
					stride = luaL_checkint(L, 4);
					n = arg3;
				}

				else
				{
					stride = arg3;
					n = m;
				}

				Eigen::Map<M, 0, Eigen::OuterStride<>> map(static_cast<const M::Scalar *>(memory.mBytes), m, n, stride);

				NEW_REF1_DECLTYPE_MOVE("map_bytes", map);	// memory, m[, n], stride, map
			}
		},*/
	#endif
		{
			"Matrix", [](lua_State * L)
			{
				if (!lua_isnoneornil(L, 1))
				{
					int m = LuaXS::Int(L, 1), n = luaL_optint(L, 2, m);

					New<M>(L, m, n);// m[, n], M
				}

				else New<M>(L);	// M

				return 1;
			}
		}, {
			"Ones", [](lua_State * L)
			{
				int m = LuaXS::Int(L, 1), n = luaL_optint(L, 2, m);

				return NewRet<M>(L, M::Ones(m, n));// m[, n], m1
			}
		}, {
			"Random", [](lua_State * L)
			{
				int m = LuaXS::Int(L, 1), n = luaL_optint(L, 2, m);

				return NewRet<M>(L, M::Random(m, n));	// m[, n], r
			}
		}, {
			"RandomPermutation", [](lua_State * L)
			{
				Eigen::PermutationMatrix<Eigen::Dynamic, Eigen::Dynamic> perm{LuaXS::Int(L, 1)};

				perm.setIdentity();

				std::random_shuffle(perm.indices().data(), perm.indices().data() + perm.indices().size());

				return NewRet<M>(L, perm);// size, perm
			}
		}, {
			"RowVector", [](lua_State * L)
			{
				New<M>(L, 1, LuaXS::Int(L, 1));	// size, rv

				return 1;
			}
		}, {
			"Vector", [](lua_State * L)
			{
				New<M>(L, LuaXS::Int(L, 1), 1);	// size, v

				return 1;
			}
		}, {
			"Zero", [](lua_State * L)
			{
				int m = LuaXS::Int(L, 1), n = luaL_optint(L, 2, m);

				return NewRet<M>(L, M::Zero(m, n));// m[, n], m0
			}
		},
		{ nullptr, nullptr }
	};

	luaL_register(L, nullptr, funcs);

	AddUmeyama<M> au{L};

	#if defined(EIGEN_CORE) || defined(EIGEN_PLUGIN_BASIC)
		TypeName<typename M::Scalar>(L);// eigen, funcs, name

		lua_insert(L, -2);	// eigen, name, funcs
		lua_rawset(L, -3);	// eigen = { ..., name = funcs }
	#endif
}

// Build-specific library name.
#define EIGEN_LIB_NAME(name) #name

// Module entry point.
CORONA_EXPORT int PLUGIN_NAME (lua_State * L)
{
	tls_LuaState = L;

	// If this is the core or all-in-one module, create a cache.
	#if defined(EIGEN_CORE) || defined(EIGEN_PLUGIN_BASIC)
		lua_getglobal(L, "require");// ... require
		lua_pushliteral(L, "cachestack");	// ..., require, "plugin.cachestack"

		if (lua_pcall(L, 1, 1, 0) != 0) lua_error(L);	// ..., cachestack / err

		lua_getfield(L, -1, "NewCacheStack");	// ..., cachestack, NewCacheStack
		lua_call(L, 0, 2);	// ..., cachestack, NewType, WithLayer
	#endif

	// Register the module with Corona.
	// TODO: make robust for other threads
	luaL_Reg no_funcs[] = { { nullptr, nullptr } };

	CoronaLibraryNew(L, EIGEN_LIB_NAME(PLUGIN_SUFFIX), "com.xibalbastudios", 1, 0, no_funcs, nullptr);	// ...[, cachestack, NewType, WithLayer], M

	// If this is the core or all-in-one module, add the WithCache() routine just supplied.
	// Register the associated binding logic and create a metatable-to-type table.
	#if defined(EIGEN_CORE) || defined(EIGEN_PLUGIN_BASIC)
		lua_insert(L, -2);	// ..., cachestack, NewType, M, WithLayer
		lua_setfield(L, -2, "WithCache");	// ..., cachestack, NewType, M = { WithCache = WithLayer }
		lua_insert(L, -3);	// ..., M, cachestack, NewType
		lua_setfield(L, LUA_REGISTRYINDEX, EIGEN_NEW_TYPE_KEY);	// ..., M, cachestack; registry = { ..., NEW_TYPE_KEY = NewType }
		lua_newtable(L);// M, cachestack, meta_to_type_data
		lua_setfield(L, LUA_REGISTRYINDEX, EIGEN_META_TO_TYPE_DATA_KEY);// ..., cachestack; registry = { ..., NEW_TYPE_KEY, META_TO_TYPE_DATA_KEY = meta_to_type_data }
		lua_pushboolean(L, 1);	// ..., M, cachestack, true
		lua_rawset(L, LUA_REGISTRYINDEX);	// ..., M; registry = { ..., NewType, [cachestack] = true }

		AddType<BoolMatrix>(L);
	#endif
	
	#ifdef WANT_INT
		AddType<Eigen::MatrixXi>(L);
	#endif

	#ifdef WANT_FLOAT
		AddType<Eigen::MatrixXf>(L);
	#endif

	#ifdef WANT_DOUBLE
		AddType<Eigen::MatrixXd>(L);
	#endif

	#ifdef WANT_CFLOAT
		AddType<Eigen::MatrixXcf>(L);
	#endif

	#ifdef WANT_CDOUBLE
		AddType<Eigen::MatrixXcd>(L);
	#endif

	return 1;
}
