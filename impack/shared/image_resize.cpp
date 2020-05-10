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

#include "impack.h"
#include "ByteReader.h"
#include "utils/Compat.h"
#include "utils/Byte.h"
#include "utils/LuaEx.h"
#include "utils/Memory.h"
#include "utils/Thread.h"

// TODO: Check https://code.google.com/archive/p/imageresampler/

static ThreadXS::TLS<MemoryXS::/*LuaMemory*/ScopedSystem *> tls_ResizeMM;

#define STBIR_ASSERT(cond) if (!(cond)) tls_ResizeMM->FailAssert(#cond)
#define STBIR_MALLOC(sz, u)           tls_ResizeMM->Malloc(sz)
#define STBIR_FREE(p, u)              tls_ResizeMM->Free(p)
#define STB_IMAGE_RESIZE_STATIC
#define STB_IMAGE_RESIZE_IMPLEMENTATION

#include "stb_image_resize.h"
#include "image_utils.h"

static void GetEdge (lua_State * L, stbir_edge & out)
{
	#define LIST() WITH(CLAMP), WITH(REFLECT), WITH(WRAP), WITH(ZERO)
	#define WITH(n) #n

	const char * names[] = { LIST(), nullptr };

	#undef WITH
	#define WITH(n) STBIR_EDGE_##n

	stbir_edge edges[] = { LIST() };

	#undef WITH
	#undef LIST

	out = edges[luaL_checkoption(L, -1, "CLAMP", names)];
}

static void GetFilter (lua_State * L, stbir_filter & out)
{
	#define LIST() WITH(DEFAULT), WITH(BOX), WITH(TRIANGLE), WITH(CUBICBSPLINE), WITH(CATMULLROM), WITH(MITCHELL)
	#define WITH(n) #n

	const char * names[] = { LIST(), nullptr };

	#undef WITH
	#define WITH(n) STBIR_FILTER_##n

	stbir_filter filters[] = { LIST() };

	#undef WITH
	#undef LIST

	out = filters[luaL_checkoption(L, -1, "DEFAULT", names)];
}

static void GetColorspace (lua_State * L, stbir_colorspace & out)
{
	#define LIST() WITH(LINEAR), WITH(SRGB)
	#define WITH(n) #n

	const char * names[] = { LIST(), nullptr };

	#undef WITH
	#define WITH(n) STBIR_COLORSPACE_##n

	stbir_colorspace colorspaces[] = { LIST() };

	#undef WITH
	#undef LIST

	out = colorspaces[luaL_checkoption(L, -1, "LINEAR", names)];
}

static void GetDataType (lua_State * L, stbir_datatype & out)
{
	#define LIST() WITH(UINT8), WITH(UINT16), WITH(UINT32), WITH(FLOAT)
	#define WITH(n) #n

	const char * names[] = { LIST(), nullptr };

	#undef WITH
	#define WITH(n) STBIR_TYPE_##n

	stbir_datatype datatypes[] = { LIST() };

	#undef WITH
	#undef LIST

	out = datatypes[luaL_checkoption(L, -1, "UINT8", names)];
}

struct ResizeOpts : ImageOpts {
	stbir_colorspace mSpace{STBIR_COLORSPACE_LINEAR};
	stbir_datatype mDatatype{STBIR_TYPE_UINT8};
	stbir_edge mHWrap{STBIR_EDGE_CLAMP}, mVWrap{STBIR_EDGE_CLAMP};
	stbir_filter mHFilter{STBIR_FILTER_DEFAULT}, mVFilter{STBIR_FILTER_DEFAULT};
	int mAlpha{STBIR_ALPHA_CHANNEL_NONE};

	void AddFields (lua_State * L) override
	{
		if (!lua_istable(L, 1)) return;

		LuaXS::FlagPair flags[] = {
			{ "ALPHA_PREMULTIPLIED", STBIR_FLAG_ALPHA_PREMULTIPLIED },
			{ "ALPHA_USES_COLORSPACE", STBIR_FLAG_ALPHA_USES_COLORSPACE }
		};

		mFlags = LuaXS::GetFlags(L, 1, "flags", flags);

		//
		lua_getfield(L, 1, "has_alpha");// opts, popts, has_alpha

		if (lua_toboolean(L, -1))
		{
			if (lua_isnumber(L, -1)) mAlpha = lua_tointeger(L, -1);

			else mAlpha = 0;
		}

		lua_pop(L, 1);	// opts, popts

		//
		LuaXS::Options opts{L, 1};

		//
		stbir_edge edge = STBIR_EDGE_CLAMP;

		opts.Call("wrap", GetEdge, edge);

		mHWrap = mVWrap = edge;

		opts.Call("hwrap", GetEdge, mHWrap);
		opts.Call("vwrap", GetEdge, mVWrap);

		//
		stbir_filter filter = STBIR_FILTER_DEFAULT;

		opts.Call("filter", GetFilter, filter);

		mHFilter = mVFilter = filter;

		opts.Call("hfilter", GetFilter, mHFilter);
		opts.Call("vfilter", GetFilter, mVFilter);

		//
		opts.Call("space", GetColorspace, mSpace);
		opts.Call("datatype", GetDataType, mDatatype);
	}
};

static void CheckChannels (lua_State * L, int carg, int & nchan, int & alpha)
{
	nchan = luaL_checkint(L, carg);

	luaL_argcheck(L, nchan > 0 && nchan <= STBIR_MAX_CHANNELS, carg, "Invalid channel count");

	if (alpha != STBIR_ALPHA_CHANNEL_NONE)
	{
		luaL_argcheck(L, alpha >= 0 && alpha <= nchan, carg + 1, "Invalid alpha channel");

		alpha = (alpha ? alpha : nchan) - 1;
	}
}

#define BOOKMARK() auto bm = tls_ResizeMM->Bookmark()//BindTable()
#define RESIZE_START(i1, o1, opts_pos)	IMAGE_START(i1, o1, ResizeOpts, opts_pos)
#define RESIZE_END(type, size)	IMAGE_END(type, size, false);	\
								BOOKMARK()
#define RESIZE_ENDTYPED(type) RESIZE_END(type, sizeof(type))
#define RESIZE_WITHTYPE(i1, o1, carg, opts_pos)	RESIZE_START(i1, o1, opts_pos);					\
												CheckChannels(L, carg, nchannels, opts.mAlpha);	\
												size_t size;									\
												switch (opts.mDatatype)							\
												{												\
													case STBIR_TYPE_UINT8: size = 1; break;		\
													case STBIR_TYPE_UINT16: size = 2; break;	\
													case STBIR_TYPE_UINT32: size = 4; break;	\
													case STBIR_TYPE_FLOAT: size = sizeof(float);\
														break;									\
												}												\
												RESIZE_END(unsigned char, size)
#define RESIZE_CALL(func, ...)	int res = func(input, iw, ih, opts.mInStride, output, ow, oh, opts.mOutStride, __VA_ARGS__);\
								return Return(L, &blob, res, output, opts.mAsUserdata);

template<int (*fff)(const void *, int, int, int, void *, int, int, int, stbir_datatype, int, int, int, stbir_edge, stbir_edge, stbir_filter, stbir_filter, stbir_colorspace, void *, float, float, float, float), void (*check)(lua_State * L, float, float, float, float)> int FullFourFloats (lua_State * L)
{
	return ByteXS::WithByteReader(L, [](lua_State * L, ByteReader & reader)	{
		RESIZE_WITHTYPE(2, 4, 6, 11);	// input, iw, ih, ow, oh, nchannels, f1, f2, f3, f4, opts, out

		float f1 = LuaXS::Float(L, 7), f2 = LuaXS::Float(L, 8), f3 = LuaXS::Float(L, 9), f4 = LuaXS::Float(L, 10);

		check(L, f1, f2, f3, f4);

		RESIZE_CALL(fff, opts.mDatatype, nchannels, opts.mAlpha, opts.mFlags, opts.mHWrap, opts.mVWrap, opts.mHFilter, opts.mVFilter, opts.mSpace, nullptr, f1, f2, f3, f4);// input, iw, ih, ow, oh, nchannels, f1, f2, f3, f4, opts, out[, str]
	}, LuaXS::Nil{});
}

static void CheckRect (lua_State * L, float s0, float t0, float s1, float t1)
{
	luaL_argcheck(L, s0 >= 0 && s0 < 1, 7, "Invalid s0");
	luaL_argcheck(L, t0 >= 0 && t0 < 1, 8, "Invalid t0");
	luaL_argcheck(L, s1 > s0 && s1 <= 1, 9, "Invalid s1");
	luaL_argcheck(L, t1 > t0 && t1 <= 1, 10, "Invalid t1");
	// TODO: Check if order matters
}

static void CheckScalesAndOffsets (lua_State * L, float xscale, float yscale, float xoff, float yoff)
{
	luaL_argcheck(L, xscale <= 1, 7, "Invalid x scale");
	luaL_argcheck(L, yscale <= 1, 8, "Invalid y scale");
	// TODO: Check if < 0 okay
}

static luaL_Reg resize_funcs[] = {
	{
		"resize_custom", [](lua_State * L)
		{
			return ByteXS::WithByteReader(L, [](lua_State * L, ByteReader & reader)	{
				RESIZE_WITHTYPE(2, 4, 6, 7);	// input, iw, ih, ow, oh, nchannels, opts, out
				RESIZE_CALL(stbir_resize, opts.mDatatype, nchannels, opts.mAlpha, opts.mFlags, opts.mHWrap, opts.mVWrap, opts.mHFilter, opts.mVFilter, opts.mSpace, nullptr);	// input, iw, ih, ow, oh, nchannels, opts, out[, str]

				return Return(L, &blob, res, output, opts.mAsUserdata);
			}, LuaXS::Nil{});
		}
	}, {
		"resize_region", FullFourFloats<&stbir_resize_region, &CheckRect>
	}, {
		"resize_rgb", [](lua_State * L)
		{
			return ByteXS::WithByteReader(L, [](lua_State * L, ByteReader & reader)	{
				RESIZE_START(2, 4, 6);
				RESIZE_ENDTYPED(unsigned char);
				RESIZE_CALL(stbir_resize_uint8, 3);	// input, iw, ih, ow, oh, opts, out[, str]

				return Return(L, &blob, res, output, opts.mAsUserdata);
			}, LuaXS::Nil{});
		}
	}, {
		"resize_rgba", [](lua_State * L)
		{
			return ByteXS::WithByteReader(L, [](lua_State * L, ByteReader & reader)	{
				RESIZE_START(2, 4, 6);
				RESIZE_ENDTYPED(op_type);
			#ifdef IMAGE_APPLE_CALL
				IMAGE_APPLE_CALL(Scale, kvImageNoFlags);
			#else
				IMAGE_OTHER_CALL(resize_bilinear_rgba, output, ne10_uint32_t(ow), ne10_uint32_t(oh), input, ne10_uint32_t(iw), ne10_uint32_t(ih), ne10_uint32_t(opts.mInStride));
			#endif

				return Return(L, &blob, res, output, opts.mAsUserdata);
			}, LuaXS::Nil{});
		}
	}, {
		"resize_subpixel", FullFourFloats<&stbir_resize_subpixel, &CheckScalesAndOffsets>
	}, {
		"resize_region", FullFourFloats<&stbir_resize_region, &CheckRect>
	},
	{ nullptr, nullptr }
};

#undef BOOKMARK
#undef RESIZE_START
#undef RESIZE_END
#undef RESIZE_ENDTYPED
#undef RESIZE_ALPHAUNTYPED
#undef RESIZE_ALPHA
#undef RESIZE_WITHTYPE
#undef RESIZE_CALL

int luaopen_resize (lua_State * L)
{
	tls_ResizeMM = MemoryXS::/*LuaMemory*/ScopedSystem::New(L);

//	tls_ResizeMM->PrepDualTables();
	
	// No new table, since resize gets embedded in "ops".

	luaL_register(L, nullptr, resize_funcs);

	return 1;
}
