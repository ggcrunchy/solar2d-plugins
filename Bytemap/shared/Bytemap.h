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

#ifndef BYTEMAP_H
#define BYTEMAP_H

#include "CoronaGraphics.h"
#include "CoronaLua.h"
#include <vector>

struct Bytemap
{
	std::vector<unsigned char> mBytes{};// Normal data (or temporary, when using unsuitable bytemaps)
	CoronaExternalBitmapFormat mFormat;	// Public format
	lua_State * mL;	// Used to look up bytemap when submitting pixels
	int mBlobRef{LUA_NOREF};// If a blob is installed, its lookup reference
	int mDummyRef{LUA_NOREF};	// Lazy dummy blob, used to effect deallocation
	int mW, mH;	// Bytemap dimensions
	bool mTemp{false};	// Using `mBytes` in temporary manner?

	Bytemap (lua_State * L, int w, int h, CoronaExternalBitmapFormat format, CoronaExternalBitmapFormat rep_format = kExternalBitmapFormat_Undefined);
	~Bytemap (void);

	CoronaExternalBitmapFormat mRepFormat;	// Representation format (workaround for issues with non-multiple-of-4 sizes)

	CoronaExternalBitmapFormat GetRepFormat (void) const { return mRepFormat; }

	unsigned char * GetData (void);

	int GetEffectiveBPP (void) const { return CoronaExternalFormatBPP(GetRepFormat()); }

	bool Flatten (bool bDetach = false);

	void DetachBlob (int * ref = nullptr);
	void InitializeBytes (bool bZeroBytes = true);
	void CheckSufficientMemory (void);
	void PushBlob (void);
	void ResolveMemory (void);
};

#endif