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

#pragma once

#include "CoronaLua.h"
#include "ByteReader.h"
#include "utils/Blob.h"
#include "utils/Byte.h"
#include "utils/Platform.h"
#include "utils/SIMD.h"
#include <utility>

//
#ifdef HAS_ACCELERATE
	#include <vector>
#else
/*
	#ifndef ENABLE_NE10_IMG_ROTATE_RGBA_NEON
		#define ENABLE_NE10_IMG_ROTATE_RGBA_NEON
	#endif
*/
	#include "NE10.h"
#endif

struct ImageOpts {
	int mInStride{0}, mOutStride{0}, mFlags{0}, mX{0}, mY{0};
	bool mAsUserdata{false};

	virtual void AddFields (lua_State * L) {}
};

template<typename T> T * GetOutputBuffer (lua_State * L, BlobXS::State & state, const ImageOpts & opts, int w, int h, int nchannels, bool bZero)
{
	void * data = state.PointToData(L, opts.mX, opts.mY, w, h, opts.mOutStride, bZero, nchannels * sizeof(T));	// ..., ud

	return static_cast<T *>(data);
}

bool ExtractFileArgs (lua_State * L, PathXS::Directories * dirs = nullptr);
bool FileArgsFromOpts (lua_State * L, int * opos = nullptr);

#ifdef __ANDROID__
template<typename G> int AuxAssetFunc (lua_State * L, PathXS::Directories * dirs, G && func) // n.b. need this to avoid RAII clobbering the stack
{
    auto fc = dirs->WithFileContents(L, 1);   // filename, ..., proxy / bytes / nil

    if (!lua_isnil(L, -1))
    {
        ByteReader bytes{L, -1};
        
        lua_replace(L, 1);  // proxy / bytes, ...; this and the next step make it look like a call to Load(), cf. image.cpp
        
        if (fc.mPos) fc.mPos = 1;
        
        return func(L, bytes);
    }

    return 1;
}
#endif

template<typename F,
    #ifdef __ANDROID__
        typename G,
    #endif
        typename T> int WithFilename (lua_State * L, F func,
                                #ifdef __ANDROID__
                                      G asset_func,
                                #endif
                                      T falsy, bool bAbsolute)
{
	std::function<int (lua_State *)> wrapped;
	
	if (bAbsolute)
	{
		if (!PathXS::Directories::AbsolutePathsOK())
		{
			lua_pushliteral(L, "Absolute paths are unsupported");	// ..., path_err

			return LuaXS::ErrorAfterFalse(L);	// ..., false, path_err
		}

		wrapped = [func](lua_State * L) {
			return func(L, luaL_checkstring(L, 1));
		};
	}

	else wrapped = [func
                #ifdef __ANDROID__
                    , asset_func
                #endif
                    ](lua_State * L) {
        PathXS::Directories * dirs = GetPathData(L);
        
    #ifdef __ANDROID__
        if (dirs->UsesResourceDir(L, 2))
        {
            if (dirs->IsDir(L, 2)) lua_remove(L, 2);// filename, ...

            return AuxAssetFunc(L, dirs, asset_func);   // proxy, ...
        }
    #endif
        
        return func(L, dirs->Canonicalize(L, true));
	};

    return LuaXS::PCallWithStackAndUpvaluesThenReturn(L, 1, wrapped, falsy);
}

int Return (lua_State * L, BlobXS::State * state, int res, void * ptr, bool bAsUserdata);
void CheckDims (lua_State * L, int i1, int o1, int & iw, int & ih, int & ow, int & oh);

#define IMAGE_START(i1, o1, otype, oi)	lua_settop(L, oi);					\
										otype opts;							\
										opts.AddFields(L);					\
										BlobXS::State blob{L, -1, "blob"};	\
										int iw, ih, ow, oh, nchannels = 4;	\
										CheckDims(L, i1, o1, iw, ih, ow, oh)
#define IMAGE_END(type, size, zero)	type * input = const_cast<type *>(ByteXS::EnsureN<type>(L, reader, size_t(iw * ih * nchannels), size));	\
									type * output = GetOutputBuffer<type>(L, blob, opts, ow, oh, nchannels, zero)
#define IMAGE_PREP()	if (opts.mInStride == 0) opts.mInStride = iw * 4
#define IMAGE_BUFFER(buf, bytes, w, h, stride)	buf.data = bytes;					\
												buf.width = vImagePixelCount(w);	\
												buf.height = vImagePixelCount(h);	\
												buf.rowBytes = size_t(stride)

#ifdef HAS_ACCELERATE
	typedef unsigned char op_type;

	#define IMAGE_APPLE_CALL(name, flags, ...)	IMAGE_PREP();																											\
												vImage_Buffer vin, vout;																								\
												IMAGE_BUFFER(vin, input, iw, ih, opts.mInStride);																		\
												IMAGE_BUFFER(vout, output, ow, oh, ow * 4);																				\
												int res = 0, size = (int)vImage##name##_ARGB8888(&vin, &vout, nullptr, ##__VA_ARGS__, flags | kvImageGetTempBufferSize);\
												if (size > 0)																											\
												{																														\
													size_t usize = size_t(size);																						\
													std::vector<unsigned char> temp(usize);																				\
													res = (int)vImage##name##_ARGB8888(&vin, &vout, temp.data(), ##__VA_ARGS__, flags) == kvImageNoError;				\
												}
#else
	typedef ne10_uint8_t op_type;

	#define IMAGE_NE10_NAME(name) ne10_img_##name

    #ifndef ENABLE_NE10_IMG_ROTATE_RGBA_NEON
        #define ne10_img_rotate_rgba_neon ne10_img_rotate_rgba_c
    #endif

	#if defined(__ANDROID__) && defined(__ARM_NEON)
        #define IMAGE_FUNC(name) SimdXS::CanUseNeon() ? IMAGE_NE10_NAME(name##_neon) : IMAGE_NE10_NAME(name##_c)
	#elif defined(__ARM_NEON)
        #define IMAGE_FUNC(name) IMAGE_NE10_NAME(name##_neon)
    #else
        #define IMAGE_FUNC(name) IMAGE_NE10_NAME(name##_c)
	#endif

	#define IMAGE_OTHER_CALL(name, ...)	IMAGE_PREP();					\
										int res = 1;					\
										auto rfunc = IMAGE_FUNC(name);	\
										rfunc(__VA_ARGS__)
#endif
