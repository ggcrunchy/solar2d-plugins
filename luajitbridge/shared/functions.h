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
//
//

#include <stddef.h>

//
//
//

struct StateData;
	
typedef int (*CFunc)(StateData *);
	
//
//
//

#define NO_RET

// "R" FUNC: has kREGISTRYINDEX as implicit argument
// "S" FUNC: has state data (self) as implicit argument
// "B" FUNC: basic or boilerplate

#define WITH_FUNCTIONS_DO()	\
\
	RFUNC_0(return, int, Ref, L_ref)SEP \
	RFUNC_1(NO_RET, void, GetRef, _rawgeti, int, ref)SEP \
	RFUNC_1(NO_RET, void, Unref, L_unref, int, ref)SEP \
\
	SFUNC_1(NO_RET, void, PushCFunc, L_pushcfunc, CFunc, func)SEP\
\
	BFUNC_1(return, int, LoadString, L_loadstring, const char *, code)SEP \
	BFUNC_3(return, int, Pcall, _pcall, int, nargs, int, nresults, int, errfunc)SEP \
	BFUNC_0(return, int, Error, _error)SEP \
	BFUNC_3(NO_RET, void, ArgCheck, L_argcheck, int, cond, int, index, const char *, extramsg)SEP \
	BFUNC_1(return, void *, NewUserdata, _newuserdata, size_t, size)SEP \
\
	BFUNC_0(NO_RET, void, PushNil, _pushnil)SEP \
	BFUNC_1(NO_RET, void, PushBoolean, _pushboolean, int, b)SEP \
	BFUNC_1(NO_RET, void, PushLightUserdata, _pushlightuserdata, void *, p)SEP \
	BFUNC_1(NO_RET, void, PushNumber, _pushnumber, double, n)SEP \
	BFUNC_1(NO_RET, void, PushString, _pushstring, const char *, s)SEP \
	BFUNC_2(NO_RET, void, PushLstring, _pushlstring, const char *, s, size_t, len)SEP \
	BFUNC_1(NO_RET, void, PushValue, _pushvalue, int, index)SEP \
\
	BFUNC_1(return, int, ToBoolean, _toboolean, int, index)SEP \
	BFUNC_1(return, void *, ToUserdata, _touserdata, int, index)SEP \
	BFUNC_1(return, double, ToNumber, _tonumber, int, index)SEP \
	BFUNC_2(return, const char *, ToLstring, _tolstring, int, index, size_t *, size)SEP \
	BFUNC_1(return, ptrdiff_t, ToInteger, _tointeger, int, index)SEP \
\
	BFUNC_1(NO_RET, void, RawGet, _rawget, int, t)SEP \
	BFUNC_1(NO_RET, void, RawSet, _rawset, int, t)SEP \
	BFUNC_2(return, int, RawEqual, _rawequal, int, index1, int, index2)SEP \
	BFUNC_1(return, int, NewMetatable, L_newmetatable, const char *, name)SEP \
	BFUNC_1(return, int, GetMetatable, _getmetatable, int, index)SEP \
	BFUNC_1(return, int, SetMetatable, _setmetatable, int, index)SEP \
\
	BFUNC_1(return, int, CheckStack, _checkstack, int, n)SEP \
	BFUNC_1(return, int, Type, _type, int, index)SEP \
	BFUNC_0(return, int, GetTop, _gettop)SEP \
	BFUNC_1(NO_RET, void, SetTop, _settop, int, index)SEP \
	BFUNC_1(NO_RET, void, Pop, _pop, int, n)SEP \
	BFUNC_1(NO_RET, void, Insert, _insert, int, index)SEP \
	BFUNC_1(NO_RET, void, Remove, _remove, int, index)
