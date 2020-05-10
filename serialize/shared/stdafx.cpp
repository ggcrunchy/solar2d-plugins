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

// stdafx.cpp : source file that includes just the standard includes
// plugin.serialize.pch will be the pre-compiled header
// stdafx.obj will contain the pre-compiled type information

#include "stdafx.h"
#include "ByteReader.h"
#include "utils/Blob.h"

const char * bytes_checklstring (lua_State * L, int arg, size_t * n)
{
	ByteReader bytes{L, arg};	// ..., bytes, ...[, err]

	if (!bytes.mBytes) lua_error(L);

	*n = bytes.mCount;

	return static_cast<const char *>(bytes.mBytes);
}

int is_blob (lua_State * L, int arg, int * resizable, size_t * size)
{
	int blob = BlobXS::IsBlob(L, arg) && !BlobXS::IsLocked(L, arg);

	if (blob)
	{
		if (resizable) *resizable = BlobXS::IsResizable(L, arg);
		if (size) *size = BlobXS::GetSize(L, arg);
	}

	return blob;
}

void * blob_realloc (lua_State * L, int arg, size_t len)
{
	if (len != 0U) try {
		BlobXS::Resize(L, arg, len);
	} catch (std::bad_alloc &) {
		return nullptr;
	}

	return BlobXS::GetData(L, arg);
}

void bob_init (lua_State * L, BlobOrBuffer * bob, int arg)
{
	int blob = is_blob(L, arg, &bob->mResizable, &bob->mSize);

	if (blob)
	{
		bob->mBarg = arg;
		bob->mOffset = 0U;
	}

	else
	{
		luaL_buffinit(L, &bob->mBuf);

		bob->mBarg = 0;
	}
}

static const char * GetData (BlobOrBuffer * bob)
{
	return reinterpret_cast<const char *>(BlobXS::GetData(bob->mL, bob->mBarg));
}

static const char * Resize (BlobOrBuffer * bob, size_t add)
{
	const char * data = nullptr;

	if (bob->mBarg)
	{
		if (bob->mResizable)
		{
			data = static_cast<const char *>(blob_realloc(bob->mL, bob->mBarg, bob->mSize + add));

			if (!data) luaL_error(bob->mL, "Resize failed: out of memory");

			bob->mSize += add;
		}

		data = GetData(bob);

		if (!bob->mResizable && bob->mOffset + add > bob->mSize) // no more room in fixed-size blob?
		{
			luaL_buffinit(bob->mL, &bob->mBuf);

			if (bob->mOffset > 0U) luaL_addlstring(&bob->mBuf, data, bob->mOffset);

			bob->mBarg = 0;
		}
	}

	return data;
}

void bob_addchar (BlobOrBuffer * bob, char c)
{
	Resize(bob, sizeof(char));

	if (bob->mBarg) BlobXS::GetData(bob->mL, bob->mBarg)[bob->mOffset++] = static_cast<unsigned char>(c);

	else luaL_addchar(&bob->mBuf, c);
}

void bob_addlstring (BlobOrBuffer * bob, const char * s, size_t l)
{
	Resize(bob, l);

	if (bob->mBarg)
	{
		unsigned char * out = BlobXS::GetData(bob->mL, bob->mBarg);

		memcpy(out + bob->mOffset, s, l);

		bob->mOffset += l;
	}

	else luaL_addlstring(&bob->mBuf, s, l);
}

void bob_pushresult (BlobOrBuffer * bob)
{
	if (!bob->mBarg) luaL_pushresult(&bob->mBuf);	// ..., str

	else if (bob->mBarg != -1 && bob->mBarg != lua_gettop(bob->mL)) lua_pushvalue(bob->mL, bob->mBarg);	// ..., blob, ...[, blob]
}

// TODO: reference any additional headers you need in STDAFX.H
// and not in this file
