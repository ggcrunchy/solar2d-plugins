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

#include "MemoryBlob.h"
#include "utils/Byte.h"

bool BlobStateImpl::Fit (lua_State * L, int x, int y, int w, int h)
{
	if (!mIsBlob || mStride == 0) return false;
	if (x < 0 || y < 0 || w <= 0 || h <= 0) return false;
	if (x + w > mW || y + h > mH) return false;
	if (size_t(mH * mStride) > mLength)
    {
		if (x != 0) return false; // growing horizontally will mess up calculations, vertically is fine

		mLength = mH * mStride;

		BlobXS::Resize(L, mArg, mLength);

		mData = BlobXS::GetData(L, mArg);
	}

	mData = ByteXS::PointToData(mData, x, y, mBPP, mStride);

	return true;
}

bool BlobStateImpl::InterpretAs (lua_State * L, int w, int h, int bpp, int stride)
{
	if (!mIsBlob || w <= 0 || h <= 0 || bpp <= 0) return false;

	int wlen = w * bpp;

	if (!stride) stride = wlen;

	else if (stride < wlen) return false;

	if (size_t(h * stride) > mLength && (!BlobXS::IsResizable(L, mArg) || BlobXS::IsLocked(L, mArg))) return false;

	mW = w;
	mH = h;
	mBPP = bpp;
	mStride = stride;

	return true;
}

void BlobStateImpl::CopyTo (void * ptr)
{
	unsigned char * from = mData, * out = static_cast<unsigned char *>(ptr);

	size_t count = size_t(mW * mBPP);

	for (int row = 0; row < mH; ++row, from += mStride, out += count) memcpy(out, from, count);
}

void BlobStateImpl::LoadFrom (void * ptr)
{
	unsigned char * from = static_cast<unsigned char *>(ptr), * out = mData;

	size_t count = size_t(mW * mBPP);

	for (int row = 0; row < mH; ++row, from += count, out += mStride) memcpy(out, from, count);
}

void BlobStateImpl::Zero (void)
{
	unsigned char * out = mData;

	for (int row = 0; row < mH; ++row, out += mStride) memset(out, 0, mW * mBPP);
}

void BlobStateImpl::Bind (lua_State * L, int arg)
{
	mArg = CoronaLuaNormalize(L, arg);
    mIsBlob = BlobXS::IsBlob(L, arg);
	mLength = BlobXS::GetSize(L, arg);
	mData = BlobXS::GetData(L, arg);
}

void BlobStateImpl::Initialize (lua_State * L, int arg, const char * key, bool bLeave)
{
	if (key && lua_istable(L, arg))
	{
		lua_getfield(L, arg, key);	// ..., arg, ..., blob?

		Bind(L, -1);

		if (!bLeave) lua_pop(L, 1);	// ...
	}

	else Bind(L, arg);
}

bool BlobStateImpl::Initialize (lua_State * L, int arg, const char * req_key, const char * opt_key, bool bLeave)
{
	luaL_checktype(L, arg, LUA_TTABLE);
	luaL_argcheck(L, req_key || opt_key, arg, "Expected at least one key");

	int top = lua_gettop(L);

	if (req_key)
	{
		lua_getfield(L, arg, req_key);	// ..., arg, ..., blob?

		if (!lua_isnil(L, -1))
		{
			Bind(L, -1);

			if (!bLeave) lua_pop(L, 1);	// ...

			return true;
		}
	}

	if (opt_key)
	{
		lua_settop(L, top);	// ...

		lua_getfield(L, arg, opt_key);	// ..., arg, ..., blob?

		Bind(L, -1);

		if (!bLeave) lua_pop(L, 1);	// ...
	}

	return false;
}
