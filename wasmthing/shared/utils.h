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

#include "common.h"

//
//
//

int GetIndexAfterLastSeparator (const char * filename, int len);
int GetExtensionIndex (const char * filename, int len);
int Error (lua_State * L, const char * err);
void AddMethods (lua_State * L, void (*add)(lua_State *), const char * name);
void LoadFile (lua_State * L);
void RegisterWithLoadFile (lua_State * L, luaL_Reg closures[]);

//
//
//

template<typename T>
T * Get (lua_State * L, int arg = 1)
{
	return static_cast<T *>(luaL_checkudata(L, arg, T::MetatableName()));
}

//
//
//

template<typename T>
T * Ptr (void * p)
{
	return static_cast<T *>(p);
}

//
//
//

template<typename T>
const T * Ptr (const void * p)
{
	return static_cast<const T *>(p);
}

//
//
//

template<typename T>
T * New (lua_State * L, bool reset = false)
{
	return Ptr<T>(lua_newuserdata(L, sizeof(T))); // ..., object
}