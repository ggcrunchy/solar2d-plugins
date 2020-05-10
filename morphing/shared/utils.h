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

#include "stdafx.h"

void Emit (lua_State * L, const dvec & vec, bool bWriteBytes, const char * key = NULL);
void Emit (lua_State * L, const pvec & vec, bool bWriteBytes, const char * key = NULL);
void GetPointFromString (lua_State * L, int arg, Point2D & p, size_t len, int offset);
void GetPointFromTable (lua_State * L, int arg, Point2D & p, size_t len, int offset);
void Populate (lua_State * L, dvec & vec);
void Populate (lua_State * L, pvec & vec);
void SetMethods (lua_State * L, luaL_Reg * methods);

bool CompareMeta (lua_State * L, const char * name);
bool GetWriteBytes (lua_State * L);
