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
// fipMetadataFind class implementation
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


BOOL fipMetadataFind::isValid() const {
	return (_mdhandle != NULL) ? TRUE : FALSE;
}

fipMetadataFind::fipMetadataFind() : _mdhandle(NULL) {
}

fipMetadataFind::~fipMetadataFind() {
	FreeImage_FindCloseMetadata(_mdhandle);
}

BOOL fipMetadataFind::findFirstMetadata(FREE_IMAGE_MDMODEL model, fipImage& image, fipTag& tag) {
	FITAG *firstTag = NULL;
	if(_mdhandle) FreeImage_FindCloseMetadata(_mdhandle);
	_mdhandle = FreeImage_FindFirstMetadata(model, image, &firstTag);
	if(_mdhandle) {
		printf("Tag %s\n", FreeImage_GetTagKey(firstTag));
		tag = FreeImage_CloneTag(firstTag);
		return TRUE;
	}
	return FALSE;
} 

BOOL fipMetadataFind::findNextMetadata(fipTag& tag) {
	FITAG *nextTag = NULL;
	if( FreeImage_FindNextMetadata(_mdhandle, &nextTag) ) {
		tag = FreeImage_CloneTag(nextTag);
		return TRUE;
	}
	return FALSE;
}

luaL_Reg metadata_find_methods[] = {
	{
		"findFirstMetadata", [](lua_State * L)
		{
			lua_settop(L, 3);	// md, model, image

			FREE_IMAGE_MDMODEL model = (FREE_IMAGE_MDMODEL)GetCode(L, FI_EnumList::kMdModel, 2);

			fipTag out;

			BOOL ok = MetadataFind(L, 1).findFirstMetadata(model, Image(L, 3), out);

			lua_pushboolean(L, ok); // md, model, image, ok

			if (ok) out = NewTag(L);// md, model, image, ok[, tag]

			return lua_gettop(L) - 3;
		}
	}, {
		"findNextMetadata", [](lua_State * L)
		{
			lua_settop(L, 1);	// md

			fipTag out;

			BOOL ok = MetadataFind(L, 1).findNextMetadata(out);

			lua_pushboolean(L, ok); // md, ok

			if (ok) out = NewTag(L);// md, ok[, tag]

			return lua_gettop(L) - 1;
		}
	}, {
		"__gc", GC<fipMetadataFind>
	}, {
		"isValid", [](lua_State * L)
		{
			lua_pushboolean(L, MetadataFind(L, 1).isValid());	// md, valid

			return 1;
		}
	},
	{ NULL, NULL }
};

luaL_Reg metadata_find_constructors[] = {
	{
		"NewMetadataFind", [](lua_State * L)
		{
			fipMetadataFind * md = New<fipMetadataFind>(L, "fipMetadataFind", metadata_find_methods); // md

			new (md) fipMetadataFind;

			return 1;
		}
	},
	{ NULL, NULL }
};

int FI_LoadMetadataFind (lua_State * L)
{
	luaL_register(L, NULL, metadata_find_constructors);

	return 0;
}