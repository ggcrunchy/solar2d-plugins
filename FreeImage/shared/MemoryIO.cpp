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
#include "classes.h"
#include "enums.h"

// ==========================================================
// fipMemoryIO class implementation
//
// Design and implementation by
// - Hervé Drolon (drolon@infonie.fr)
//
// This file is part of FreeImage 3
//
// COVERED CODE IS PROVIDED UNDER THIS LICENSE ON AN "AS IS" BASIS, WITHOUT WARRANTY
// OF ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING, WITHOUT LIMITATION, WARRANTIES
// THAT THE COVERED CODE IS FREE OF DEFECTS, MERCHANTABLE, FIT FOR A PARTICULAR PURPOSE
// OR NON-INFRINGING. THE ENTIRE RISK AS TO THE QUALITY AND PERFORMANCE OF THE COVERED
// CODE IS WITH YOU. SHOULD ANY COVERED CODE PROVE DEFECTIVE IN ANY RESPECT, YOU (NOT
// THE INITIAL DEVELOPER OR ANY OTHER CONTRIBUTOR) ASSUME THE COST OF ANY NECESSARY
// SERVICING, REPAIR OR CORRECTION. THIS DISCLAIMER OF WARRANTY CONSTITUTES AN ESSENTIAL
// PART OF THIS LICENSE. NO USE OF ANY COVERED CODE IS AUTHORIZED HEREUNDER EXCEPT UNDER
// THIS DISCLAIMER.
//
// Use at your own risk!
// ==========================================================


#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>

fipMemoryIO::fipMemoryIO(BYTE *data, DWORD size_in_bytes) {
	_hmem = FreeImage_OpenMemory(data, size_in_bytes);
}

fipMemoryIO::~fipMemoryIO() { 
	if(_hmem != NULL) {
		FreeImage_CloseMemory(_hmem);
	}
}

void fipMemoryIO::close() { 
	if(_hmem != NULL) {
		FreeImage_CloseMemory(_hmem);
		_hmem = NULL;
	}
}

BOOL fipMemoryIO::isValid() const {
	return (_hmem != NULL);
}

FREE_IMAGE_FORMAT fipMemoryIO::getFileType() const {
	if(_hmem != NULL) {
		return FreeImage_GetFileTypeFromMemory(_hmem, 0);
	}

	return FIF_UNKNOWN;
}

FIBITMAP* fipMemoryIO::load(FREE_IMAGE_FORMAT fif, int flags) const {
	return FreeImage_LoadFromMemory(fif, _hmem, flags);
}

FIMULTIBITMAP* fipMemoryIO::loadMultiPage(FREE_IMAGE_FORMAT fif, int flags) const {
	return FreeImage_LoadMultiBitmapFromMemory(fif, _hmem, flags);
}

BOOL fipMemoryIO::save(FREE_IMAGE_FORMAT fif, FIBITMAP *dib, int flags) {
	return FreeImage_SaveToMemory(fif, dib, _hmem, flags);
}

BOOL fipMemoryIO::saveMultiPage(FREE_IMAGE_FORMAT fif, FIMULTIBITMAP *bitmap, int flags) {
	return FreeImage_SaveMultiBitmapToMemory(fif, bitmap, _hmem, flags);
}

unsigned fipMemoryIO::read(void *buffer, unsigned size, unsigned count) const {
	return FreeImage_ReadMemory(buffer, size, count, _hmem);
}

unsigned fipMemoryIO::write(const void *buffer, unsigned size, unsigned count) {
	return FreeImage_WriteMemory(buffer, size, count, _hmem);
}

long fipMemoryIO::tell() const {
	return FreeImage_TellMemory(_hmem);
}

BOOL fipMemoryIO::seek(long offset, int origin) {
	return FreeImage_SeekMemory(_hmem, offset, origin);
}

BOOL fipMemoryIO::acquire(BYTE **data, DWORD *size_in_bytes) {
	return FreeImage_AcquireMemory(_hmem, data, size_in_bytes);
}

luaL_Reg memory_io_methods[] = {
	{
		"acquire", [](lua_State * L)
		{
			lua_settop(L, 2);	// mio, as_table

			BYTE * data;
			DWORD size;
			BOOL ok = MemoryIO(L, 1).acquire(&data, &size);

			lua_pushboolean(L, ok);	// mio, as_table, ok

			if (ok)
			{
				if (lua_toboolean(L, 2))
				{
					lua_createtable(L, size, 0);// mio, as_table, ok, t

					for (size_t i = 1; i <= size; ++i)
					{
						lua_pushinteger(L, data[i - 1]);// mio, as_table, ok, t, byte
						lua_rawseti(L, 4, i);	// mio, as_table, ok, t
					}
				}

				else lua_pushlstring(L, (char *)data, size);	// mio, as_table, ok, data
			}

			return lua_gettop(L) - 2;
		}
	}, {
		"close", [](lua_State * L)
		{
			MemoryIO(L, 1).close();

			return 0;
		}
	}, {
		"gc", GC<fipMemoryIO>
	}, {
		"getFileType", [](lua_State * L)
		{
			lua_pushinteger(L, MemoryIO(L, 1).getFileType());	// mio, ftype

			GetName(L, FI_EnumList::kFormat, -1);

			return 1;
		}
	}, {
		"isValid", [](lua_State * L)
		{
			lua_pushboolean(L, MemoryIO(L, 1).isValid());	// mio, ok

			return 1;
		}
	}, {
		"load", [](lua_State * L)
		{
			FREE_IMAGE_FORMAT fif = (FREE_IMAGE_FORMAT)GetCode(L, FI_EnumList::kFormat, 2);

			lua_pushlightuserdata(L, MemoryIO(L, 1).load(fif, luaL_optint(L, 3, 0)));	// mio, fif, flags

			// TODO: light userdata?

			return 1;
		}
	}, {
		"read", [](lua_State * L)
		{
			unsigned size = lua_tointeger(L, 2), count = lua_tointeger(L, 3);

			void * data = lua_newuserdata(L, size * count);	// size, count, data

			unsigned nread = MemoryIO(L, 1).read(data, size, count);

			lua_pushlstring(L, (char *)data, size * nread);	// size, count, data, bytes

			return 1;
		}
	}, {
		"save", [](lua_State * L)
		{
			FREE_IMAGE_FORMAT fif = (FREE_IMAGE_FORMAT)GetCode(L, FI_EnumList::kFormat, 2);

			lua_pushboolean(L, MemoryIO(L, 1).save(fif, (FIBITMAP *)lua_touserdata(L, 3), luaL_optint(L, 4, 0)));	// mio, fif, dib[, flags], ok

			return 1;
		}
	}, {
		"seek", [](lua_State * L)
		{
			lua_pushboolean(L, MemoryIO(L, 1).seek(lua_tointeger(L, 2), lua_tointeger(L, 3)));	// mio, offset, origin

			return 1;
		}
	}, {
		"tell", [](lua_State * L)
		{
			lua_pushinteger(L, MemoryIO(L, 1).tell());	// mio, offset

			return 1;
		}
	}, {
		"write", [](lua_State * L)
		{
			unsigned size = lua_tointeger(L, 2), count = lua_tointeger(L, 3), len = lua_objlen(L, 1);

			if (size * count > len) count = len / size;

			lua_pushinteger(L, MemoryIO(L, 1).write(lua_tostring(L, 1), size, count));	// mio, size, count, written

			return 1;
		}
	},
	// TODO: multipage?
	{ NULL, NULL }
};

luaL_Reg memory_io_constructors[] = {
	{
		"NewMemoryIO", [](lua_State * L)
		{
			lua_settop(L, 2);	// data, size_in_bytes

			fipMemoryIO * mio = New<fipMemoryIO>(L, "fipMemoryIO", memory_io_methods); // data, size_in_bytes, mio

			new (mio) fipMemoryIO((BYTE *)lua_touserdata(L, 1), luaL_optinteger(L, 2, 0UL));

			return 1;
		}
	},
	{ NULL, NULL }
};

int FI_LoadMemoryIO (lua_State * L)
{
	luaL_register(L, NULL, memory_io_constructors);

	return 0;
}