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

#include "Bytes.h"
#include "utils/Byte.h"
#include "utils/LuaEx.h"
#include <algorithm>
#include <cstring>

// Are the bytes distinct from the color key?
bool BytesOpts::Props::IsNotColorKey (const unsigned char * bytes, int n) const
{
	ColorKey key;

	key.mInt = 0U;

	for (int i = 0; i < n; ++i) key.mBytes[i] = bytes[i];

	return mColorKey.mInt != key.mInt;
}

// Constructor
BytesOpts::ColumnIter::ColumnIter (const RowIter & iter) : mRows{iter}, mColumn{iter.mProps.mX1}, mPixel{iter.mData}
{
}

// Constructor
BytesOpts::ColumnIter::ColumnIter (const RowIter & iter, const unsigned char * end) : mRows{iter}, mColumn{iter.mProps.mX1}, mPixel{iter.mData}
{
	size_t count = iter.mProps.GetWidth();

    if (end < mPixel) end = mPixel;
    
	size_t range = (end - mPixel) / iter.mBPP;

	if (range < count) mColumn += count - range;
}

// Advance to the next column
void BytesOpts::ColumnIter::Next (void)
{
	++mColumn;

	mPixel += mRows.mBPP;
}

// Set some bytes in the pixel pointed at by the column
void BytesOpts::ColumnIter::Set (const unsigned char * bytes)
{
	for (int i = 0; i < mRows.mWanted; ++i) mPixel[mRows.mProps.mOffset + i] = bytes[i];
}

// Set some bytes in the pixel pointed at by the column, unless the color key is found 
void BytesOpts::ColumnIter::SetColorKey (const unsigned char * bytes)
{
	if (mRows.mProps.IsNotColorKey(bytes, mRows.mWanted)) Set(bytes);
}

// Set some (grayscale) bytes in the pixel pointed at by the column
void BytesOpts::ColumnIter::SetGray (const unsigned char * bytes)
{
	unsigned char gray = *bytes;

	for (int i = 0; i < 3; ++i) mPixel[i] = gray;
}

// Set some (grayscale) bytes in the pixel pointed at by the column, unless the color key is found
void BytesOpts::ColumnIter::SetGrayColorKey (const unsigned char * bytes)
{
	if (mRows.mProps.IsNotColorKey(bytes, 1)) SetGray(bytes);
}

// Set some (inverse) bytes in the pixel pointed at by the column
void BytesOpts::ColumnIter::SetInverse (const unsigned char * bytes)
{
	for (int i = 0; i < mRows.mWanted; ++i) mPixel[i] = 0xFF - bytes[i];
}

// Set some (inverse) bytes in the pixel pointed at by the column, unless the color key is found 
void BytesOpts::ColumnIter::SetInverseColorKey (const unsigned char * bytes)
{
	if (mRows.mProps.IsNotColorKey(bytes, mRows.mWanted)) SetInverse(bytes);
}

// Constructor
BytesOpts::RowIter::RowIter (const BytesOpts & opts, Bytemap * bmap, int bpp, int wanted) : mProps{opts.mProps}, mBPP{bpp}, mRow{opts.mProps.mY1}
{
	mData = ByteXS::PointToData(bmap->GetData(), mProps.mX1, mProps.mY1, bmap->mW, mBPP, &mStride);
	mWanted = (std::min)(wanted, mBPP);
}

// Constructor
BytesOpts::RowIter::RowIter (const BytesOpts & opts, Bytemap * bmap, int y, int bpp, int wanted) : mProps{opts.mProps}, mBPP{bpp}, mRow{y}
{
	mProps.mY1 = mProps.mY2 = y;

	mData = ByteXS::PointToData(bmap->GetData(), mProps.mX1, y, bmap->mW, mBPP, &mStride);
	mWanted = (std::min)(wanted, mBPP);
}

// Kick off a column iterator
BytesOpts::ColumnIter BytesOpts::RowIter::Columns (void) const
{
	return ColumnIter{*this};
}
		
// Kick off a column iterator and diminish write count
BytesOpts::ColumnIter BytesOpts::RowIter::Columns (const unsigned char * end) const
{
	return ColumnIter{*this, end};
}

// Advance to the next row
void BytesOpts::RowIter::Next (void)
{
	++mRow;

	mData += mStride;
}

// Constructor
BytesOpts::BytesOpts (lua_State * L, int opts, Bytemap * bmap, bool bSetBytes) : mSetBytes{bSetBytes}
{
	// Set appropriate defaults.
	mFormat = bmap->mFormat;
	mProps.mX2 = bmap->mW - 1;
	mProps.mY2 = bmap->mH - 1;
	mWidth = bmap->mW;

	// If no options were provided, use the defaults.
	if (!lua_istable(L, opts)) return;

	// When setting bytes, we might want be assigning from a source narrower than the bytemap,
	// so start as too-thin-to-exist and work up from there.
	if (mSetBytes) mWidth = -1;

	// Do a protected load function to catch any errors.
	mOK = LuaXS::PCall(L, [](lua_State * L)
    {
        LuaXS::UD<BytesOpts>(L, 1)->Load(L, LuaXS::UD<Bytemap>(L, 2), 3);

		return 0;
    }, this, bmap, LuaXS::StackIndex(L, opts));

	// When setting bytes, account for regions that veered outside the bytemap.
	if (mSetBytes)
	{
		if (mWidth != 0) mWidth = (std::max)(mWidth, mProps.GetWidth());
		if (mProps.mX1 < 0) mLeft -= mProps.mX1;
		if (mProps.mY1 < 0) mTop -= mProps.mY1;
	}

	// Clamp the region against the bytemap. If the region is empty, flag it in the width.
	if (mProps.mX1 < bmap->mW && mProps.mY1 < bmap->mH && mProps.mX2 >= 0 && mProps.mY2 >= 0)
	{
		mProps.mX1 = (std::max)(mProps.mX1, 0);
		mProps.mY1 = (std::max)(mProps.mY1, 0);
		mProps.mX2 = (std::min)(mProps.mX2, bmap->mW - 1);
		mProps.mY2 = (std::min)(mProps.mY2, bmap->mH - 1);
	}

	else mWidth = 0;

	// If info is requested, create a table to receive it (or use the one provided).
	if (!mOK) return;

	lua_getfield(L, opts, "get_info");	// ..., info?

	if (lua_toboolean(L, -1))
	{
		mInfoAt = opts;

		if (!lua_istable(L, -1))
		{
			lua_pop(L, 1);	// ...
			lua_newtable(L);// ..., info
		}
	}

	// At this point we have processed any options, so hijack their stack slot.
	lua_replace(L, opts);	// ..., info?, ...
}

// Kick off a row iterator
BytesOpts::RowIter BytesOpts::Rows (Bytemap * bmap) const
{
	return RowIter{*this, bmap, bmap->GetEffectiveBPP(), 0};
}

// Kick off a row iterator, allowing for wanted bits per pixel
BytesOpts::RowIter BytesOpts::Rows (Bytemap * bmap, int y, int wanted) const
{
	return RowIter{*this, bmap, y, bmap->GetEffectiveBPP(), wanted};
}

// Get an optional coordinate
int BytesOpts::GetCoord (lua_State * L, int opts, const char * name, int dim, bool bHigh)
{
	lua_getfield(L, opts, name);// ..., name

	int coord = luaL_optint(L, -1, bHigh ? dim : 1) - 1;

	lua_pop(L, 1);	// ...

	return coord;
}

// Helper to return get / set bytes result, plus any info if requested
int BytesOpts::Return (lua_State * L)
{
	if (!WantsInfo()) return 1;

	lua_pushvalue(L, mInfoAt);	// ..., bytes / ok, info

	return 2;
}

// Add format and rect to info
void BytesOpts::AddFormatAndRect (lua_State * L)
{
	Member ints[] = {
		{ "x1", mProps.mX1 }, { "y1", mProps.mY1 }, { "x2", mProps.mX2 }, { "y2", mProps.mY2 }
	};

	for (auto i : ints) LuaXS::SetField(L, mInfoAt, i.mName, i.mValue + 1);	// ..., info = { name = value }, ...

	switch (mFormat)
	{
	case kExternalBitmapFormat_Mask:
		lua_pushliteral(L, "mask");	// ..., "mask"
		break;
	case kExternalBitmapFormat_RGB:
		lua_pushliteral(L, "rgb");	// ..., "rgb"
		break;
	default:
		lua_pushliteral(L, "rgba");	// ..., "rgba"
	}

	lua_setfield(L, mInfoAt, "format");	// ..., info = { format = format }, ...
}

// Get the optional custom format
void BytesOpts::GetFormat (lua_State * L, int opts)
{
	lua_getfield(L, opts, "format");// ..., format

	if (!lua_isnil(L, -1))
	{
		// When setting bytes, process any special "formats".
		if (mSetBytes)
		{
			lua_pushliteral(L, "inverse");	// ..., format, "inverse"

			mInvert = lua_equal(L, -2, -1) != 0;

			lua_pop(L, 1);	// ..., format

			if (!mInvert && mFormat != kExternalBitmapFormat_Mask)
			{
				lua_pushliteral(L, "grayscale");// ..., format, "grayscale"

				mGray = lua_equal(L, -2, -1) != 0;

				lua_pop(L, 1);	// ..., format
			}
		}

		if (mGray)
		{
			mGray = mFormat != kExternalBitmapFormat_Mask;	// "mask" format has only one channel, so forgo more complex grayscale logic
			mFormat = kExternalBitmapFormat_Mask;
		}

		else if (!mInvert)
		{
			// Begin by servicing any single-component format request. This is done with an explicit search
			// rather than `luaL_checkoption()` since we might be given one of the multi-component formats.
			const char * colors[] = { "red", "green", "blue", "alpha" };
			int offset = -1;

			for (int i = 0; i < 4 && offset < 0; ++i, lua_pop(L, 1))
			{
				lua_pushstring(L, colors[i]);	// ..., format, name

				if (lua_equal(L, -2, -1)) offset = i;
			}

			if (offset >= 0)
			{
				mProps.mOffset = offset;

				// If the bytemap is unable to accommodate the format, interpret it as having no bytes.
				if (offset >= CoronaExternalFormatBPP(mFormat)) mWidth = 0;

				mFormat = kExternalBitmapFormat_Mask;
			}

			// Otherwise, the format must be one of Corona's own.
			else
			{
				const char * names[] = { "mask", "rgb", "rgba", nullptr };
				CoronaExternalBitmapFormat formats[] = { kExternalBitmapFormat_Mask, kExternalBitmapFormat_RGB, kExternalBitmapFormat_RGBA };

				mFormat = formats[luaL_checkoption(L, -1, nullptr, names)];
			}
		}
	}

	lua_pop(L, 1);	// ...
}

// Load body
void BytesOpts::Load (lua_State * L, Bytemap * bmap, int opts)
{
	// Get the rect in the bytemap.
	mProps.mX1 = GetCoord(L, opts, "x1", bmap->mW, false);
	mProps.mY1 = GetCoord(L, opts, "y1", bmap->mH, false);
	mProps.mX2 = GetCoord(L, opts, "x2", bmap->mW, true);
	mProps.mY2 = GetCoord(L, opts, "y2", bmap->mH, true);

	if (mProps.mX1 > mProps.mX2) std::swap(mProps.mX1, mProps.mX2);
	if (mProps.mY1 > mProps.mY2) std::swap(mProps.mY1, mProps.mY2);

	lua_getfield(L, opts, "stride");// ..., stride

	mWidth = luaL_optint(L, -1, mWidth);

	lua_pop(L, 1);	// ...

	// Check for requests to interpret the bytes in a specific format.
	GetFormat(L, opts);

	// If setting bytes, assign any color key (adhering to the format) and check for extrusion.
	if (mSetBytes)
	{
		lua_getfield(L, opts, "colorkey");	// ..., colorkey
		lua_getfield(L, opts, "extrude");	// ..., colorkey, extrude

		if (!lua_isnil(L, -2))
		{
			ByteReader key{L, -2};

			if (key.mBytes && key.mCount > 0)
			{
				memcpy(mProps.mColorKey.mBytes, key.mBytes, (std::min)(int(key.mCount), CoronaExternalFormatBPP(mFormat)));

				mColorKey = true;
			}
		}

		mExtrude = size_t((std::max)(luaL_optint(L, -1, 0), 0));

		lua_pop(L, 2);	// ...
	}

	// When getting bytes, handle any blob target configuration.
	else
	{
		// TODO!
	}
}