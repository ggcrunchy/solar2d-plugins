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
#include "utils/Blob.h"
#include "utils/Byte.h"
#include "utils/LuaEx.h"

// Gets bytes, in the case where padding components are added
static int GetAugmentedBytes (lua_State * L, Bytemap * bmap, BytesOpts & opts, int nwanted, int true_bpp)
{
	{
		ByteXS::ByteWriter writer{L, opts.mBlobRow, opts.mOutStride};

		// Go through the rows, accumulating columns one by one inside the requested range. In
		// each instance, pad the column with some suitable default bytes.
		unsigned char pad[4] = { 0, 0, 0, 255 };

		for (auto rows = opts.Rows(bmap); !rows.Done(); rows.Next(), writer.NextLine())
		{
			for (auto cols = rows.Columns(); !cols.Done(); cols.Next())
			{
				writer.AddBytes(cols, true_bpp); // n.b. no offset since these only apply to masks (one byte, thus never augmented)
				writer.AddBytes(pad + true_bpp, nwanted - true_bpp);
			}
		}

		// Return the amalgamated fragment data, along with any info.

		// ~ByteWriter(): bmap[, opts / info], bytes
	}

	return opts.Return(L);	// ..., bytes[, info]
}

// Get bytes, in the case where components are being dropped
static int GetReducedBytes (lua_State * L, Bytemap * bmap, BytesOpts & opts, int nwanted)
{
	{
		ByteXS::ByteWriter writer{L, opts.mBlobRow, opts.mOutStride};

		// Go through the rows, accumulating columns one by one inside the requested range.
		for (auto rows = opts.Rows(bmap); !rows.Done(); rows.Next(), writer.NextLine())
		{
			for (auto cols = rows.Columns(); !cols.Done(); cols.Next()) writer.AddBytes(cols + opts.mProps.mOffset, nwanted);
		}

		// Return the amalgamated fragment data, along with any info.

		// ~ByteWriter(): bmap[, opts / info], bytes
	}

	return opts.Return(L);	// ..., bytes[, info]
}

// RAII-based helper to use temporary memory
struct TempGuard {
	Bytemap & mRef;	// Bytemap being guarded

	TempGuard (Bytemap & bmap) : mRef{bmap}
	{
		mRef.CheckSufficientMemory(); 
	}

	~TempGuard (void)
	{
		mRef.ResolveMemory();
	}
};

// Extract bytes from the bytemap
int Bytemap_GetBytes (lua_State * L)
{
	Bytemap * bmap = LuaXS::DualUD<Bytemap>(L, 1, BYTEMAP_NAME);

	if (bmap)
	{
		BytesOpts opts{L, 2, bmap, false};

		if (opts.mOK)
		{
			TempGuard guard{*bmap};

			// If requested, populate relevant info fields.
			if (opts.WantsInfo()) opts.AddFormatAndRect(L);	// bmap[, opts / info], ...

			// In the case of an empty region, return a zero-length byte stream.
			if (opts.mWidth == 0)
			{
				lua_pushliteral(L, "");	// bmap[, opts / info], ..., ""

				return opts.Return(L);	// bmap, ""[, info]
			}

			// If the component counts differ (either by request or because of representation
			// quirks), either augment or reduce the data accordingly, returning it.
			int nwanted = CoronaExternalFormatBPP(opts.mFormat), true_bpp = bmap->GetEffectiveBPP();

			if (true_bpp > nwanted) return GetReducedBytes(L, bmap, opts, nwanted);	// bmap, bytes[, info]
			if (true_bpp < nwanted) return GetAugmentedBytes(L, bmap, opts, nwanted, true_bpp);	// bmap, bytes[, info]

			// Since the component counts match, we now only care what region we want from the
			// bytemap. This is easy when the full rect is requested: return all bytes intact.
			{
				ByteXS::ByteWriter writer{L, opts.mBlobRow, opts.mOutStride};

				if (opts.CanCopy(bmap)) writer.AddBytes(bmap->GetData(), true_bpp * bmap->mW * bmap->mH);

				// Otherwise, we must accumulate a line at a time.
				else
				{
					// For each row, get all columns in the requested range. At the end, merge them.
					int w = opts.mProps.GetWidth() * true_bpp;

					for (auto rows = opts.Rows(bmap); !rows.Done(); rows.Next(), writer.NextLine()) writer.AddBytes(rows, w);
				}

				// Return the bytes. If requested, append some info.

				// ~ByteWriter(): bmap[, opts / info], bytes
			}

			return opts.Return(L);	// bitmap, bytes[, info]
		}
	}

	else lua_pushliteral(L, "Missing bytemap");	// ..., error

	return LuaXS::ErrorAfterNil(L);	// ..., error, nil
}