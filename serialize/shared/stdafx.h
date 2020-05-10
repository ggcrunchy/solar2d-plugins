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

#ifdef _WIN32
#pragma once

#include "targetver.h"

#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers
// Windows Header Files:
#include <windows.h>

#endif

#ifndef STDAFX_H
#define STDAFX_H

#include "CoronaLibrary.h"
#include "CoronaLua.h"

#ifdef __cplusplus
extern "C" {
#endif
    
	/*
	#include "lua.h"
	#include "lualib.h"
	#include "lauxlib.h"
	*/
	int luaopen_pack (lua_State * L);
	int luaopen_marshal (lua_State * L);
	int luaopen_struct (lua_State * L);

	const char * bytes_checklstring (lua_State * L, int arg, size_t * n);

	int is_blob (lua_State * L, int arg, int * resizable, size_t * size);

	void * blob_realloc (lua_State * L, int arg, size_t len);

	typedef struct BlobOrBuffer {
		lua_State * mL;
		luaL_Buffer mBuf;
		size_t mOffset, mSize;
		int mBarg, mResizable;
	} BlobOrBuffer;

	void bob_init (lua_State * L, BlobOrBuffer * bob, int arg);
	void bob_addchar (BlobOrBuffer * bob, char c);
	void bob_addlstring (BlobOrBuffer * bob, const char * s, size_t l);
	void bob_pushresult (BlobOrBuffer * bob);
#ifdef __cplusplus
}
#endif

#endif

// TODO: reference additional headers your program requires here
