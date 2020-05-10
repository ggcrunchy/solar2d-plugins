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
#include "image_utils.h"
#include "ByteReader.h"
#include "utils/Byte.h"
#include "utils/SIMD.h"
#include "utils/Thread.h"
#include <algorithm>
#include <cmath>
#include <cstring>

using namespace std;

#define SDF_IMPLEMENTATION

#include "sdf.h"

template<typename R, typename T = R> inline R Cast (T value) { return static_cast<R>(value); }

#ifndef __ANDROID__ // lrint() broken on CrystaX...
	template<> inline unsigned char Cast<unsigned char, float> (float value) { return static_cast<unsigned char>(lrint(value)); }
#endif

template<typename T = unsigned char> static const T * CorrectBytes (lua_State * L, const ByteReader & reader, int in_stride, int h)
{
	return ByteXS::EnsureN<T>(L, reader, size_t(in_stride * h));
}

template<typename T = unsigned char> static T * GetOutputBuffer (lua_State * L, int w, int h, int out_stride) // TODO: better name! (varies from grabbing a blob, for sure)
{
	T * out = reinterpret_cast<T *>(LuaXS::NewArray<unsigned char>(L, out_stride * h));	// ..., out

	w *= sizeof(T);

	if (out_stride > w)
	{
		int n = out_stride - w;

		for (int i = 0, pos = w; i < h; ++i, pos += out_stride) memset(&out[pos], 0, n);
	}

	return out;
}

enum { kAverage, kLightness, kMax, kMin, kRed, kGreen, kBlue, k601, k709 };

struct GrayState {
	int mMethod;
	bool mIsFloat;
};

static bool GetOpts (lua_State * L, int first, int & w, int & h, int & out_stride, int & in_stride, int scale = 1, GrayState * gs = nullptr)
{
	w = luaL_checkint(L, first), h = luaL_checkint(L, first + 1), in_stride = scale * w, out_stride = w;

	luaL_argcheck(L, w > 0, first, "Invalid width");
	luaL_argcheck(L, h > 0, first + 1, "Invalid height");

	bool bAsUserdata = false;

	if (gs)
	{
		gs->mMethod = kAverage;
		gs->mIsFloat = false;
	}

	LuaXS::Options{L, first + 2}.Add("out_stride", out_stride)
								.Add("in_stride", in_stride)
								.Add("as_userdata", bAsUserdata)
								.ArgCheck(out_stride >= w, "Invalid out stride")
								.ArgCheck(in_stride >= scale * w, "Invalid in stride")
								.Call("gray_method", [gs](lua_State * L)
								{
									const char * names[] = { "average", "lightness", "max", "min", "red", "green", "blue", "601", "709", nullptr };
									int methods[] = { kAverage, kLightness, kMax, kMin, kRed, kGreen, kBlue, k601, k709 };

									if (gs) gs->mMethod = methods[luaL_checkoption(L, -1, "average", names)];
								})
								.Call("is_float", [gs](lua_State * L)
								{
									if (gs) gs->mIsFloat = lua_toboolean(L, -1) != 0;
								});

	if (gs && gs->mIsFloat)
	{
		in_stride *= sizeof(float);
		out_stride *= sizeof(float);
	}

	return bAsUserdata;
}

template<typename T> static int Push (lua_State * L, T * out, bool bAsUserdata)
{
	if (bAsUserdata) ByteXS::AddBytesMetatable(L, IMPACK_BYTES);

	else lua_pushlstring(L, reinterpret_cast<const char *>(out), lua_objlen(L, -1));	// ..., out, str

	return 1;
}
// TODO: Much of this ^^^^ is probably duplicating stuff found elsewhere

template<int bpp, typename T, typename F> void ToGray (const T * rgb, T * out, F gray, int w, int h, int in_stride, int out_stride)
{
	ThreadXS::parallel_for(0U, size_t(h), [=](size_t row)
	{
		auto rgb_line = rgb + row * in_stride;
		auto out_line = out + row * out_stride;

		for (int col = 0; col < w; ++col) out_line[col] = gray(rgb_line + col * bpp);
	});
}

template<typename T> static T GrayAverage (const T * rgb)
{
	return Cast<T>((rgb[0] + rgb[1] + rgb[2]) / 3.0);
}

template<typename T> static T GrayMax (const T * rgb)
{
	return (max)(rgb[0], (max)(rgb[1], rgb[2]));
}

template<typename T> static T GrayMin (const T * rgb)
{
	return (min)(rgb[0], (min)(rgb[1], rgb[2]));
}

template<typename T> static T GrayLightness (const T * rgb)
{
	T gmax = GrayMax(rgb), gmin = GrayMin(rgb);

	return Cast<T>((gmax + gmin) / 2);
}

template<typename T> static T GrayRed (const T * rgb)
{
	return rgb[0];
}

template<typename T> static T GrayGreen (const T * rgb)
{
	return rgb[1];
}

template<typename T> static T GrayBlue (const T * rgb)
{
	return rgb[2];
}

template<typename T> static T Gray601 (const T * rgb)
{
	return Cast<T>(0.299 * rgb[0] + 0.587 * rgb[1] + 0.114 * rgb[2]);
}

template<typename T> static T Gray709 (const T * rgb)
{
	return Cast<T>(0.2126 * rgb[0] + 0.7152 * rgb[1] + 0.0722 * rgb[2]);
}

#define CALL_GRAY(name) case k##name: ToGray<bpp, T>(rgb, out, Gray##name<T>, w, h, in_stride, out_stride); break

//
template<int bpp, typename T> int GrayFromRGB (lua_State * L, const ByteReader & reader, int w, int h, int in_stride, int out_stride, int method, bool bAsUserdata)
{
	auto rgb = CorrectBytes<T>(L, reader, in_stride, h);
	T * out = GetOutputBuffer<T>(L, w, h, out_stride);	// radius, img, w, h, opts, out

	switch (method)
	{
	CALL_GRAY(Average);
	CALL_GRAY(Lightness);
	CALL_GRAY(Max);
	CALL_GRAY(Min);
	CALL_GRAY(Red);
	CALL_GRAY(Green);
	CALL_GRAY(Blue);
	CALL_GRAY(601);
	CALL_GRAY(709);
	}

	return Push(L, out, bAsUserdata);
}

#undef CALL_GRAY

static luaL_Reg grayscale_funcs[] = {
	{
		"build_distance_field", [](lua_State * L)
		{
			return ByteXS::WithByteReader(L, [](lua_State * L, ByteReader & reader) {
				float r = LuaXS::Float(L, 2);

				luaL_argcheck(L, r > 0, 2, "Non-positive radius");

				int w, h, out_stride, in_stride;

				bool bAsUserdata = GetOpts(L, 3, w, h, out_stride, in_stride);	// img, radius, w, h, opts
				unsigned char * temp = LuaXS::NewArray<unsigned char>(L, 3 * sizeof(float) * w * h);// img, radius, w, h, opts, temp
				unsigned char * out = GetOutputBuffer(L, w, h, out_stride);	// img, radius, w, h, opts, temp, out

				sdfBuildDistanceFieldNoAlloc(out, out_stride, r, CorrectBytes(L, reader, in_stride, h), w, h, in_stride, temp);

				lua_remove(L, -2);	// img, radius, w, h, opts, out

				return Push(L, out, bAsUserdata);	// img, radius, w, h, opts, out[, str]
			}, LuaXS::Nil{});
		}
	}, {
		"coverage_to_distance_field", [](lua_State * L)
		{
			return ByteXS::WithByteReader(L, [](lua_State * L, ByteReader & reader) {
				int w, h, out_stride, in_stride;

				bool bAsUserdata = GetOpts(L, 2, w, h, out_stride, in_stride);	// img, w, h, opts
				unsigned char * out = GetOutputBuffer(L, w, h, out_stride);	// radius, img, w, h, opts, out

				sdfCoverageToDistanceField(out, out_stride, CorrectBytes(L, reader, in_stride, h), w, h, in_stride);

				return Push(L, out, bAsUserdata);	// img, w, h, opts, out[, str]
			}, LuaXS::Nil{});
		}
	}, {
		"rgb_to_gray", [](lua_State * L)
		{
			return ByteXS::WithByteReader(L, [](lua_State * L, ByteReader & reader) {
				int w, h, out_stride, in_stride;
				GrayState gs;

				bool bAsUserdata = GetOpts(L, 2, w, h, out_stride, in_stride, 3, &gs);	// img, w, h, opts

				if (!gs.mIsFloat) return GrayFromRGB<3, unsigned char>(L, reader, w, h, in_stride, out_stride, gs.mMethod, bAsUserdata);

				else return GrayFromRGB<3, float>(L, reader, w, h, in_stride, out_stride, gs.mMethod, bAsUserdata);
			}, LuaXS::Nil{});
		}
	}, {
		"rgba_to_gray", [](lua_State * L)
		{
			return ByteXS::WithByteReader(L, [](lua_State * L, ByteReader & reader) {
				int w, h, out_stride, in_stride;
				GrayState gs;

				bool bAsUserdata = GetOpts(L, 2, w, h, out_stride, in_stride, 4, &gs);	// img, w, h, opts

				if (!gs.mIsFloat) return GrayFromRGB<4, unsigned char>(L, reader, w, h, in_stride, out_stride, gs.mMethod, bAsUserdata);

				else return GrayFromRGB<4, float>(L, reader, w, h, in_stride, out_stride, gs.mMethod, bAsUserdata);
			}, LuaXS::Nil{});
		}
	},
	{ nullptr, nullptr }
};

int luaopen_grayscale (lua_State * L)
{
	lua_newtable(L);// ..., grayscale
	luaL_register(L, nullptr, grayscale_funcs);

	return 1;
}