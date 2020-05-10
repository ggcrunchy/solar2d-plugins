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
// fipTag class implementation
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

#include <string.h>

fipTag::fipTag() {
	_tag = FreeImage_CreateTag();
}

fipTag::~fipTag() {
	FreeImage_DeleteTag(_tag);
}

BOOL fipTag::setKeyValue(const char *key, const char *value) {
	if(_tag) {
		FreeImage_DeleteTag(_tag);
		_tag = NULL;
	}
	// create a tag
	_tag = FreeImage_CreateTag();
	if(_tag) {
		BOOL bSuccess = TRUE;
		// fill the tag
		DWORD tag_length = (DWORD)(strlen(value) + 1);
		bSuccess &= FreeImage_SetTagKey(_tag, key);
		bSuccess &= FreeImage_SetTagLength(_tag, tag_length);
		bSuccess &= FreeImage_SetTagCount(_tag, tag_length);
		bSuccess &= FreeImage_SetTagType(_tag, FIDT_ASCII);
		bSuccess &= FreeImage_SetTagValue(_tag, value);
		return bSuccess;
	}
	return FALSE;
}

fipTag::fipTag(const fipTag& tag) {
	_tag = FreeImage_CloneTag(tag._tag);
}

fipTag& fipTag::operator=(const fipTag& tag) {
	if(this != &tag) {
		if(_tag) FreeImage_DeleteTag(_tag);
		_tag = FreeImage_CloneTag(tag._tag);
	}
	return *this;
}

fipTag& fipTag::operator=(FITAG *tag) {
	if(_tag) FreeImage_DeleteTag(_tag);
	_tag = tag;
	return *this;
}

BOOL fipTag::isValid() const {
	return (_tag != NULL) ? TRUE : FALSE;
}

const char* fipTag::getKey() const {
	return FreeImage_GetTagKey(_tag);
}

const char* fipTag::getDescription() const {
	return FreeImage_GetTagDescription(_tag);
}

WORD fipTag::getID() const {
	return FreeImage_GetTagID(_tag);
}

FREE_IMAGE_MDTYPE fipTag::getType() const {
	return FreeImage_GetTagType(_tag);
}

DWORD fipTag::getCount() const {
	return FreeImage_GetTagCount(_tag);
}

DWORD fipTag::getLength() const {
	return FreeImage_GetTagLength(_tag);
}

const void* fipTag::getValue() const {
	return FreeImage_GetTagValue(_tag);
}

BOOL fipTag::setKey(const char *key) {
	return FreeImage_SetTagKey(_tag, key);
}

BOOL fipTag::setDescription(const char *description) {
	return FreeImage_SetTagDescription(_tag, description);
}

BOOL fipTag::setID(WORD id) {
	return FreeImage_SetTagID(_tag, id);
}

BOOL fipTag::setType(FREE_IMAGE_MDTYPE type) {
	return FreeImage_SetTagType(_tag, type);
}

BOOL fipTag::setCount(DWORD count) {
	return FreeImage_SetTagCount(_tag, count);
}

BOOL fipTag::setLength(DWORD length) {
	return FreeImage_SetTagLength(_tag, length);
}

BOOL fipTag::setValue(const void *value) {
	return FreeImage_SetTagValue(_tag, value);
}

const char* fipTag::toString(FREE_IMAGE_MDMODEL model, char *Make) const {
	return FreeImage_TagToString(model, _tag, Make);
}

luaL_Reg tag_methods[] = {
	{
		"assign", [](lua_State * L)
		{
			Tag(L, 1) = Tag(L, 2);

			return 0;
		}
	}, {
		"clone", [](lua_State * L)
		{
			fipTag & new_tag = NewTag(L);	// tag, new_tag

			new_tag = Tag(L, 1);

			return 1;
		}
	}, {
		"__gc", GC<fipTag>
	}, {
		"getCount", [](lua_State * L)
		{
			lua_pushinteger(L, Tag(L, 1).getCount());	// tag, count

			return 1;
		}
	}, {
		"getDescription", [](lua_State * L)
		{
			lua_pushstring(L, Tag(L, 1).getDescription());	// tag, desc

			return 1;
		}
	}, {
		"getID", [](lua_State * L)
		{
			lua_pushinteger(L, Tag(L, 1).getID());	// tag, id

			return 1;
		}
	}, {
		"getKey", [](lua_State * L)
		{
			lua_pushstring(L, Tag(L, 1).getKey());	// tag, key

			return 1;
		}
	}, {
		"getLength", [](lua_State * L)
		{
			lua_pushinteger(L, Tag(L, 1).getLength());	// tag, length

			return 1;
		}
	}, {
		"getType", [](lua_State * L)
		{
			lua_pushinteger(L, Tag(L, 1).getType());// tag, type

			GetName(L, FI_EnumList::kMdType, 2);

			return 1;
		}
	}, {
		"getValue", [](lua_State * L)
		{
			lua_pushlightuserdata(L, (void *)Tag(L, 1).getValue());	// tag, value

			return 1;
		}
	}, {
		"isValid", [](lua_State * L)
		{
			lua_pushboolean(L, Tag(L, 1).isValid());	// tag, valid

			return 1;
		}
	}, {
		"setCount", [](lua_State * L)
		{
			lua_pushboolean(L, Tag(L, 1).setCount(lua_tointeger(L, 2)));// tag, count, ok

			return 1;
		}
	}, {
		"setDescription", [](lua_State * L)
		{
			lua_pushboolean(L, Tag(L, 1).setDescription(lua_tostring(L, 2)));	// tag, desc, ok

			return 1;
		}
	}, {
		"setID", [](lua_State * L)
		{
			lua_pushboolean(L, Tag(L, 1).setID(lua_tointeger(L, 2)));	// tag, id, ok

			return 1;
		}
	}, {
		"setKey", [](lua_State * L)
		{
			lua_pushboolean(L, Tag(L, 1).setKey(lua_tostring(L, 2)));	// tag, key, ok

			return 1;
		}
	}, {
		"setLength", [](lua_State * L)
		{
			lua_pushboolean(L, Tag(L, 1).setLength(lua_tointeger(L, 2)));	// tag, length, ok

			return 1;
		}
	}, {
		"setKeyValue", [](lua_State * L)
		{
			lua_pushboolean(L, Tag(L, 1).setKeyValue(lua_tostring(L, 2), lua_tostring(L, 3)));	// tag, key, value, ok

			return 1;
		}
	}, {
		"setType", [](lua_State * L)
		{
			FREE_IMAGE_MDTYPE mdtype = (FREE_IMAGE_MDTYPE)GetCode(L, FI_EnumList::kMdType, 2);

			lua_pushboolean(L, Tag(L, 1).setType(mdtype));	// tag, type, ok

			return 1;
		}
	}, {
		"setValue", [](lua_State * L)
		{
			BOOL ok = FALSE;

			switch (lua_type(L, 2))
			{
			case LUA_TSTRING:
				ok = Tag(L, 1).setValue(lua_tostring(L, 2));
				break;
			case LUA_TLIGHTUSERDATA:
			case LUA_TUSERDATA:
				ok = Tag(L, 1).setValue(lua_touserdata(L, 2));
				break;
			}

			lua_pushboolean(L, ok);	// tag, ok

			return 1;
		}
	},
	{ NULL, NULL }
};

fipTag & NewTag (lua_State * L)
{
	fipTag * tag = New<fipTag>(L, "fipTag", tag_methods); // tag

	new (tag) fipTag;

	return *tag;
}

int FI_LoadTag (lua_State * L)
{
	return 0;
}