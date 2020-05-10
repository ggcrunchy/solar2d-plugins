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

// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#ifndef STDAFX_H
#define STDAFX_H

#ifdef _WIN32

#include "targetver.h"

#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers
// Windows Header Files:
#include <windows.h>

#endif

#include "CoronaLibrary.h"
#include "CoronaLua.h"
#include "Point.h"
#include <vector>

typedef std::vector<Point2D> pvec;
typedef std::vector<double> dvec;
typedef std::vector<int> ivec;

template<typename T> T * Get (lua_State * L, int arg = 1, const char * name = NULL)
{
	if (name) return (T *)luaL_checkudata(L, arg, name);

	return (T *)lua_touserdata(L, arg);
}

template<typename T> T * New (lua_State * L)
{
	T * ud = (T *)lua_newuserdata(L, sizeof(T));

	new (ud) T;

	return ud;
}

template<typename T> int GC (lua_State * L)
{
	Get<T>(L)->~T();

	return 0;
}

// TODO: reference additional headers your program requires here

#endif
