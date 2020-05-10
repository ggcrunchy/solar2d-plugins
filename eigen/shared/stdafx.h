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

// Corona bits...
#include "CoronaLua.h"
#include "CoronaLibrary.h"

// ...and Native utilities + ByteReader.
#include "utils/LuaEx.h"
#include "utils/Blob.h"
#include "utils/Thread.h"
#include "ByteReader.h"

// Propagate asserts to Lua. This is bound in implementation.h.
static ThreadXS::TLS<lua_State *> tls_LuaState;

#ifndef eigen_assert
	#define eigen_assert(x) if (!(x)) luaL_error(tls_LuaState, "Eigen error: " #x);
#endif

// STL...
#include <algorithm>
#include <complex>
#include <sstream>
#include <type_traits>
#include <utility>

// ...and Eigen itself.
#include <Eigen/Eigen>

// Forward declarations.
template<typename R, bool bSetTemp = false> R * SetTemp (lua_State * L, R * temp, int arg, bool bMissingOK = false);
template<typename R, typename MM, typename MS, typename SM> R WithMatrixScalarCombination (lua_State * L, MM && both, MS && mat_scalar, SM && scalar_mat, int arg1 = 1, int arg2 = 2);
