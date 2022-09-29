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

#include "CoronaGraphics.h"
#include "Bytemap.h"
#include "Bytes.h"
#include "utils/Byte.h"
#include "utils/LuaEx.h"
#include "utils/Thread.h"
#include <algorithm>

// Helper to inject a "true" into the result
static int OK (lua_State * L, BytesOpts & opts)
{
	return LuaXS::PushArgAndReturn(L, true) + opts.Return(L);	// bmap, ...[, opts], ok[, info]
}

// Byte setter body
template<void (BytesOpts::ColumnIter::*memfn)(const unsigned char *)> void SetBytesCore (lua_State * L, Bytemap * bmap, const void * bytes, const BytesOpts & opts, size_t n)
{
	auto & props = opts.mProps;
	int bpp = CoronaExternalFormatBPP(opts.mFormat), stride, xend = (std::min)(props.mX2 + 1, props.mX1 + int(n)), y1 = props.mY1;
	auto pdata = ByteXS::PointToData(static_cast<const unsigned char *>(bytes), opts.mLeft, opts.mTop, opts.mWidth, bpp, &stride);
	auto end = ByteXS::PointToData(bmap->mBytes.data(), xend, props.mY2, bmap->mW, bmap->GetEffectiveBPP(), nullptr);

	ThreadXS::parallel_for(y1, props.mY2 + 1, [bmap, bpp, end, opts, pdata, stride, y1](int row)
	{
		auto rows = opts.Rows(bmap, row, bpp);
		auto pbytes = pdata + (row - y1) * stride;

		for (auto cols = rows.Columns(end); !cols.Done(); cols.Next(), pbytes += bpp) (cols.*memfn)(pbytes);
    });
}

// Sets bytes, in the case where padding components are added
static int AuxSetBytes (lua_State * L, Bytemap * bmap, const void * bytes, BytesOpts & opts, size_t n)
{
	if (n > 0)
	{
		if (!opts.mColorKey)
		{
			if (opts.mInvert) SetBytesCore<&BytesOpts::ColumnIter::SetInverse>(L, bmap, bytes, opts, n);
			else if (opts.mGray) SetBytesCore<&BytesOpts::ColumnIter::SetGray>(L, bmap, bytes, opts, n);
			else SetBytesCore<&BytesOpts::ColumnIter::Set>(L, bmap, bytes, opts, n);
		}

		else
		{
			if (opts.mInvert) SetBytesCore<&BytesOpts::ColumnIter::SetInverseColorKey>(L, bmap, bytes, opts, n);
			else if (opts.mGray) SetBytesCore<&BytesOpts::ColumnIter::SetGrayColorKey>(L, bmap, bytes, opts, n);
			else SetBytesCore<&BytesOpts::ColumnIter::SetColorKey>(L, bmap, bytes, opts, n);
		}
	}

	return OK(L, opts);
}

// Bind GetBytes() / SetBytes() region for extrusion
static void SetOpts (lua_State * L, int arg, int x1, int y1, int x2, int y2)
{
	struct {
		const char * mName;
		int mValue;
	} coords[] = {
		{ "x1", x1 }, { "y1", y1 }, { "x2", x2 }, { "y2", y2 }
	};

	for (auto c : coords) LuaXS::SetField(L, arg, c.mName, c.mValue);	// ...; opts = { ..., name = value }
}

// Extrude a given edge
static void DoEdge (lua_State * L, int arg, Bytemap * bmap, int x1, int y1, int x2, int y2, int dx, int dy, size_t extrude, int dist)
{
	SetOpts(L, arg, x1 + 1, y1 + 1, x2 + 1, y2 + 1);

	LuaXS::StackIndex bmi = LuaXS::StackIndex(L, 1), ai = LuaXS::StackIndex(L, arg);
	int top = lua_gettop(L);

	if (LuaXS::PCallN(L, Bytemap_GetBytes, 1, bmi, ai))	// ..., opts, bytes / error
	{
		for (size_t i = 0, n = (std::min)(extrude, size_t(dist)); i < n; ++i)
		{
			x1 += dx;
			y1 += dy;
			x2 += dx;
			y2 += dy;

			SetOpts(L, arg, x1 + 1, y1 + 1, x2 + 1, y2 + 1);

			if (!LuaXS::PCall(L, Bytemap_SetBytes, bmi, LuaXS::StackIndex(L, -1), ai)) lua_pop(L, 1);	// ..., opts, bytes (pops any error)
		}
	}

	lua_settop(L, top);	// ..., opts
}

// Extrude non-border edges
static void Extrude (lua_State * L, Bytemap * bmap, const BytesOpts & opts, int y2)
{
	lua_createtable(L, 0, 4);	// ..., opts

	auto & props = opts.mProps;
	int t = CoronaLuaNormalize(L, -1);

	// Are we at least one pixel into the bytemap from the left...
	if (props.mX1 > 0) DoEdge(L, t, bmap, props.mX1, props.mY1, props.mX1, y2, -1, 0, opts.mExtrude, props.mX1);

	// ...from the right...
	if (props.mX2 < bmap->mW - 1) DoEdge(L, t, bmap, props.mX2, props.mY1, props.mX2, y2, +1, 0, opts.mExtrude, bmap->mW - 1 - props.mX2);

	// ...from above...
	if (props.mY1 > 0) DoEdge(L, t, bmap, props.mX1, props.mY1, props.mX2, props.mY1, 0, -1, opts.mExtrude, props.mY1);

	// ...from below?
	if (y2 < bmap->mH - 1) DoEdge(L, t, bmap, props.mX1, y2, props.mX2, y2, 0, +1, opts.mExtrude, bmap->mH - 1 - y2);

	lua_pop(L, 1);	// ...
}

// Assign bytes to the bytemap
int Bytemap_SetBytes (lua_State * L)
{
	Bytemap * bmap = LuaXS::DualUD<Bytemap>(L, 1, BYTEMAP_NAME);

	if (bmap)
	{
		// If a blob is present, flatten it into normal memory (detaching it). Skip this step
		// when the temporary flag is set, in which case the blob is passed in as the bytes. 
		if (bmap->mBlobRef != LUA_NOREF && !bmap->mTemp && !bmap->Flatten(true)) return LuaXS::ErrorAfterNil(L);// bmap, bytes, ...[, error]

		// Proceed to rest of algorithm.
		ByteReader bytes{L, 2};	// bmap, bytes, ...[, error]

		if (bytes.mBytes)
		{
			BytesOpts opts{L, 3, bmap, true};

            if (opts.mOK)
			{
				// Remember the lower y-coordinate, since the next step might clip it.
				int y2 = opts.mProps.mY2;

				// Get the pixel count: this will either be the bytemap area or the number of
				// (whole) pixels available in the bytes, whichever is lesser. Given this
				// amount, try to pare off any extra rows.
				size_t n = (std::min)(bytes.mCount / CoronaExternalFormatBPP(opts.mFormat), size_t(bmap->mW * bmap->mH));

				if (opts.mWidth != 0) // width = 0 when region is empty
				{
					int to_row = int(n + opts.mWidth - 1) / opts.mWidth - 1;

					opts.mProps.mY2 = (std::min)(opts.mProps.mY1 + to_row, opts.mProps.mY2);
				}

				// If requested, populate relevant info fields.
				if (opts.WantsInfo()) opts.AddFormatAndRect(L);	// bmap, bytes[, opts / info]

				if (opts.mWidth != 0)
				{
					// If the component counts match, we only care what region we want from
					// the bytemap. This is easy when the full rect is requested: blast all
					// bytes into it.
					if (opts.CanCopy(bmap) && !opts.mColorKey) memcpy(bmap->mBytes.data(), bytes.mBytes, n * bmap->GetEffectiveBPP());

					// Otherwise, we must assign pixel by pixel.
					else AuxSetBytes(L, bmap, bytes.mBytes, opts, n);

					// Perform any extrusion.
					if (opts.mExtrude > 0U && !opts.mProps.IsCustomRect(bmap)) Extrude(L, bmap, opts, y2);
				}

				return OK(L, opts);	// bmap, bytes, ok[, info]
			}
		}
	}

	else lua_pushliteral(L, "Missing bytemap");	// ..., error

	return LuaXS::ErrorAfterNil(L);// ..., nil, error
}