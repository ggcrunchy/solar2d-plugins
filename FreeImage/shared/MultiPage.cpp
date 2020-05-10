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
// fipMultiPage class implementation
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


fipMultiPage::fipMultiPage(BOOL keep_cache_in_memory) : _mpage(NULL), _bMemoryCache(keep_cache_in_memory) {
}

fipMultiPage::~fipMultiPage() {
	if(_mpage) {
		// close the stream
		close(0);
	}
}

BOOL fipMultiPage::isValid() const {
	return (NULL != _mpage) ? TRUE : FALSE;
}

BOOL fipMultiPage::open(const char* lpszPathName, BOOL create_new, BOOL read_only, int flags) {
	// try to guess the file format from the filename
	FREE_IMAGE_FORMAT fif = FreeImage_GetFIFFromFilename(lpszPathName);

	// open the stream
	_mpage = FreeImage_OpenMultiBitmap(fif, lpszPathName, create_new, read_only, _bMemoryCache, flags);

	return (NULL != _mpage ) ? TRUE : FALSE;
}

BOOL fipMultiPage::open(fipMemoryIO& memIO, int flags) {
	// try to guess the file format from the memory handle
	FREE_IMAGE_FORMAT fif = memIO.getFileType();

	// open the stream
	_mpage = memIO.loadMultiPage(fif, flags);

	return (NULL != _mpage ) ? TRUE : FALSE;
}

BOOL fipMultiPage::open(FreeImageIO *io, fi_handle handle, int flags) {
	// try to guess the file format from the handle
	FREE_IMAGE_FORMAT fif = FreeImage_GetFileTypeFromHandle(io, handle, 0);

	// open the stream
	_mpage = FreeImage_OpenMultiBitmapFromHandle(fif, io, handle, flags);

	return (NULL != _mpage ) ? TRUE : FALSE;
}

BOOL fipMultiPage::close(int flags) {
	BOOL bSuccess = FALSE;
	if(_mpage) {
		// close the stream
		bSuccess = FreeImage_CloseMultiBitmap(_mpage, flags);
		_mpage = NULL;
	}

	return bSuccess;
}

BOOL fipMultiPage::saveToHandle(FREE_IMAGE_FORMAT fif, FreeImageIO *io, fi_handle handle, int flags) const {
	BOOL bSuccess = FALSE;
	if(_mpage) {
		bSuccess = FreeImage_SaveMultiBitmapToHandle(fif, _mpage, io, handle, flags);
	}

	return bSuccess;
}

BOOL fipMultiPage::saveToMemory(FREE_IMAGE_FORMAT fif, fipMemoryIO& memIO, int flags) const {
	BOOL bSuccess = FALSE;
	if(_mpage) {
		bSuccess = memIO.saveMultiPage(fif, _mpage, flags);
	}

	return bSuccess;
}

int fipMultiPage::getPageCount() const {
	return _mpage ? FreeImage_GetPageCount(_mpage) : 0;
}

void fipMultiPage::appendPage(fipImage& image) {
	if(_mpage) {
		FreeImage_AppendPage(_mpage, image);
	}
}

void fipMultiPage::insertPage(int page, fipImage& image) {
	if(_mpage) {
		FreeImage_InsertPage(_mpage, page, image);
	}
}

void fipMultiPage::deletePage(int page) {
	if(_mpage) {
		FreeImage_DeletePage(_mpage, page);
	}
}

BOOL fipMultiPage::movePage(int target, int source) {
	return _mpage ? FreeImage_MovePage(_mpage, target, source) : FALSE;
}

FIBITMAP* fipMultiPage::lockPage(int page) {
	return _mpage ? FreeImage_LockPage(_mpage, page) : NULL;
}

void fipMultiPage::unlockPage(fipImage& image, BOOL changed) {
	if(_mpage) {
		FreeImage_UnlockPage(_mpage, image, changed);
		// clear the image so that it becomes invalid.
		// this is possible because of the friend declaration
		image._dib = NULL;
		image._bHasChanged = FALSE;
	}
}

BOOL fipMultiPage::getLockedPageNumbers(int *pages, int *count) const {
	return _mpage ? FreeImage_GetLockedPageNumbers(_mpage, pages, count) : FALSE;
}

luaL_Reg multi_page_methods[] = {
	{
		"appendPage", [](lua_State * L)
		{
			MultiPage(L, 1).appendPage(Image(L, 2));

			return 0;
		}
	}, {
		"close", [](lua_State * L)
		{
			lua_pushboolean(L, MultiPage(L, 1).close(luaL_optint(L, 2, 0)));// mp[, flags], ok
			// TODO: Save flags
			return 1;
		}
	}, {
		"deletePage", [](lua_State * L)
		{
			MultiPage(L, 1).deletePage(lua_tointeger(L, 2));

			return 0;
		}
	}, {
		"__gc", GC<fipMultiPage>
	}, {
		"getLockedPageNumbers", [](lua_State * L)
		{
			lua_settop(L, 1);	// mp

			fipMultiPage & mp = MultiPage(L, 1);

			int count;

			BOOL ok = mp.getLockedPageNumbers(NULL, &count);

			if (ok)
			{
				int * pages = (int *)lua_newuserdata(L, count * sizeof(int));	// mp, pages

				ok = mp.getLockedPageNumbers(pages, &count);

				if (ok)
				{
					lua_createtable(L, count, 0);	// mp, pages, t

					for (int i = 0; i < count; ++i)
					{
						lua_pushinteger(L, pages[i]);	// mp, pages, t, page
						lua_rawseti(L, 3, i + 1);	// mp, pages, t = { ..., page }
					}
				}
			}

			lua_pushboolean(L, ok);	// mp[, pages, t], ok

			if (ok) lua_insert(L, 3);	// mp, pages, ok, t

			return ok ? 2 : 1;
		}
	}, {
		"getPageCount", [](lua_State * L)
		{
			lua_pushinteger(L, MultiPage(L, 1).getPageCount());	// mp, count

			return 1;
		}
	}, {
		"insertPage", [](lua_State * L)
		{
			MultiPage(L, 1).insertPage(lua_tointeger(L, 2), Image(L, 3));

			return 0;
		}
	}, {
		"isValid", [](lua_State * L)
		{
			lua_pushboolean(L, MultiPage(L, 1).isValid());	// mp, valid

			return 1;
		}
	}, {
		"lockPage", [](lua_State * L)
		{
			lua_pushlightuserdata(L, MultiPage(L, 1).lockPage(lua_tointeger(L, 2)));// mp, page, data

			return 1;
		}
	}, {
		"movePage", [](lua_State * L)
		{
			lua_pushboolean(L, MultiPage(L, 1).movePage(lua_tointeger(L, 2), lua_tointeger(L, 3))); // mp, target, source, ok

			return 1;
		}
	}, {
		"open", [](lua_State * L)
		{
			fipMultiPage & mp = MultiPage(L, 1);

			BOOL ok;

			if (lua_isstring(L, 2))
			{
				const char * name = lua_tostring(L, 2);

				ok = mp.open(name, lua_toboolean(L, 3), lua_toboolean(L, 4), GetLoadFlags(L, fipImage::identifyFIF(name), 5));
			}

			else
			{
				fipMemoryIO & mio = MemoryIO(L, 2);

				ok = mp.open(mio, GetLoadFlags(L, mio.getFileType(), 2));
			}

			lua_pushboolean(L, ok);	// mp, (ARGS), ok

			return 1;
		}
	}, {
		"saveToMemory", [](lua_State * L)
		{
			FREE_IMAGE_FORMAT fif = (FREE_IMAGE_FORMAT)GetCode(L, FI_EnumList::kFormat, 2);

			lua_pushboolean(L, MultiPage(L, 1).saveToMemory(fif, MemoryIO(L, 3), GetSaveFlags(L, fif, 4)));	// mp, mem[, flags], ok

			return 1;
		}
	}, {
		"unlockPage", [](lua_State * L)
		{
			MultiPage(L, 1).unlockPage(Image(L, 2), lua_toboolean(L, 3));

			return 0;
		}
	},
	{ NULL, NULL }
};

luaL_Reg multi_page_constructors[] = {
	{
		"NewMultiPage", [](lua_State * L)
		{
			lua_settop(L, 1);	// cache_in_memory

			fipMultiPage * mp = New<fipMultiPage>(L, "fipMultiPage", multi_page_methods); // cache_in_memory, mp

			new (mp) fipMultiPage(lua_toboolean(L, 1));

			return 1;
		}
	},
	{ NULL, NULL }
};

int FI_LoadMultiPage (lua_State * L)
{
	luaL_register(L, NULL, multi_page_constructors);

	return 0;
}