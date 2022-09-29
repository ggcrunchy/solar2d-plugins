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

#ifndef BYTES_H
#define BYTES_H

#include "CoronaLua.h"
#include "CoronaGraphics.h"
#include "Bytemap.h"
#include "ByteReader.h"

// State for various options used by non-default getBytes() and setBytes() calls
struct BytesOpts {
	struct Props {
		int mX1{0}, mY1{0}, mX2, mY2;	// Region to get or set
		int mOffset{0};	// Offset used to read off color-specific bytes
		union ColorKey {
			unsigned int mInt;	// Integer representation of color key...
			unsigned char mBytes[4];// ...and byte representation
		} mColorKey;

		int GetWidth (void) const { return mX2 - mX1 + 1; }

		bool IsCustomRect (Bytemap * bmap) const { return mX1 > 0 || mY1 > 0 || mX2 < bmap->mW - 1 || mY2 < bmap->mH - 1; }
		bool IsNotColorKey (const unsigned char * bytes, int n) const;
	};

	Props mProps;	// Properties common to options and iterators
	bool mOK{true};	// Loading succeeded?
	bool mColorKey{false}, mGray{false}, mInvert{false}, mSetBytes;	// Has color key? Is grayscale? Is inverted? Setting bytes?
	CoronaExternalBitmapFormat mFormat;	// Format of data to get or set
	unsigned char * mBlobRow{nullptr};	// Start of row in blob data, if any
	size_t mOutStride{0U};	// Blob stride
	size_t mExtrude{0U};// How many pixels to extrude in each direction
	int mLeft{0}, mTop{0}, mWidth;	// Assignment offset, when setting bytes; data width
	int mInfoAt{-1};// Stack location of info table

	struct Member {
		const char * mName;	// Field name, in Lua table
		int mValue;	// Value of opts member
	};

	class RowIter;

	class ColumnIter {
		const RowIter & mRows;	// Rows to which these columns belong
		int mColumn;// Current column
		unsigned char * mPixel;	// Data pointed to by column

	public:
		bool Done (void) const { return mColumn > mRows.mProps.mX2; }
		operator const unsigned char * (void) const { return mPixel; }
		void Next (void);
		void Set (const unsigned char * bytes);
		void SetColorKey (const unsigned char * bytes);
		void SetGray (const unsigned char * bytes);
		void SetGrayColorKey (const unsigned char * bytes);
		void SetInverse (const unsigned char * bytes);
		void SetInverseColorKey (const unsigned char * bytes);

		ColumnIter (const RowIter & iter);
		ColumnIter (const RowIter & iter, const unsigned char * end);
	};

	class RowIter {
		Props mProps;	// Properties used on this row and its columns
		int mBPP, mStride;	// Bits-per-pixel and stride of bytemap
		int mRow;	// Current row
		int mWanted;// Wanted bits-per-pixel
		unsigned char * mData;	// Data pointed to by row

		friend class ColumnIter;

	public:
		bool Done (void) const { return mRow > mProps.mY2; }
		operator const unsigned char * (void) const { return mData; }

		RowIter (const BytesOpts & opts, Bytemap * bmap, int bpp, int wanted);
		RowIter (const BytesOpts & opts, Bytemap * bmap, int y, int bpp, int nwanted);

		ColumnIter Columns (void) const;
		ColumnIter Columns (const unsigned char * end) const;

		void Next (void);
	};

	BytesOpts (lua_State * L, int opts, Bytemap * bmap, bool bSetBytes);

	RowIter Rows (Bytemap * bmap) const;
	RowIter Rows (Bytemap * bmap, int y, int wanted) const;

	bool CanCopy (Bytemap * bmap) const { return mFormat == bmap->GetRepFormat() && !mProps.IsCustomRect(bmap); }
	bool WantsInfo (void) const { return mInfoAt > 0; }

	int GetCoord (lua_State * L, int opts, const char * name, int dim, bool bHigh);
	int Return (lua_State * L);

	void AddFormatAndRect (lua_State * L);
	void GetFormat (lua_State * L, int opts);
	void Load (lua_State * L, Bytemap * bmap, int opts);
};

//
//
//

struct ByteProxy {
	unsigned char * mBytes;
	size_t mSize;
};

//
//
//

int Bytemap_BindBlob (lua_State * L);
int Bytemap_Deallocate (lua_State * L);
int Bytemap_GetBytes (lua_State * L);
int Bytemap_MakeSeamless (lua_State * L);
int Bytemap_SetBytes (lua_State * L);

#endif