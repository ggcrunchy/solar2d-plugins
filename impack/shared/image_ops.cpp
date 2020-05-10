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
#include "utils/Byte.h"
#include "utils/Platform.h"
#include "utils/Thread.h"
#include "image_utils.h"
#include <algorithm>

#define OPS_ENDTYPED(zero) IMAGE_END(op_type, sizeof(op_type), zero)

static void GetSize (int iw, int ih, int & ow, int & oh, double radian) // adapted from NE10_rotate
{
    double a = sin(radian), b = cos(radian);

    ow = int((iw * fabs(a)) + (iw * fabs(b))) + 1;
    oh = int((iw * fabs(b)) + (ih * fabs(a))) + 1;
}

static luaL_Reg ops_funcs[] = {
	{
		"box_filter", [](lua_State * L)
		{
			return ByteXS::WithByteReader(L, [](lua_State * L, ByteReader & reader)	{	
				IMAGE_START(2, 4, ImageOpts, 6);// input, iw, ih, kw, kh, opts

				luaL_argcheck(L, ow <= iw, 4, "Kernel too wide");
				luaL_argcheck(L, oh <= ih, 5, "Kernel too tall");

				if (opts.mOutStride == 0) opts.mOutStride = iw * 4;

				int kw = ow, kh = oh; // put kernel dims into own values, then duplicate input dims as output

				ow = iw;
				oh = ih;

				OPS_ENDTYPED(false);
			#ifdef IMAGE_APPLE_CALL
				// TODO: could these be options?
				Pixel_8888 bg = { 0 };
				vImage_Flags flags = kvImageBackgroundColorFill;	// TODO: if so, could add copy-in-place, truncate kernel, and background fill options
				vImagePixelCount offx = 0, offy = 0;

				if (kw % 2 == 0) kw += (kw < ow ? +1 : -1);	// Ensure odd-sized kernel dimensions
				if (kh % 2 == 0) kh += (kh < oh ? +1 : -1);

				size_t n = size_t(kw * kh);
				std::vector<int16_t> kernel(n, 1);

				IMAGE_APPLE_CALL(Convolve, flags, offx, offy, kernel.data(), uint32_t(kh), uint32_t(kw), int32_t(n), bg);
			#else
				ne10_size_t source, kernel;

				source.x = ne10_uint32_t(iw);
				source.y = ne10_uint32_t(ih);
				kernel.x = ne10_uint32_t(kw);
				kernel.y = ne10_uint32_t(kh);

				IMAGE_OTHER_CALL(boxfilter_rgba8888, input, output, source, ne10_uint32_t(opts.mInStride), ne10_uint32_t(opts.mOutStride), kernel);
			#endif

				return Return(L, &blob, res, output, opts.mAsUserdata);
			}, LuaXS::Nil{});
		}
	}, {
		"floats_to_unorm8s", [](lua_State * L)
		{
			return ByteXS::WithByteReader(L, [](lua_State * L, ByteReader & reader) {
				IMAGE_START(2, 0, ImageOpts, 4); // input, iw, ih, opts; 0 = no output dims

				LuaXS::Options{L, 4}.Add("channels", nchannels);

				int res = 1, scan_len = iw * nchannels;
				float * input = const_cast<float *>(ByteXS::EnsureN<float>(L, reader, size_t(ih * scan_len)));
				unsigned char * output = GetOutputBuffer<unsigned char>(L, blob, opts, iw, ih, nchannels, false);

				ThreadXS::parallel_for(0, ih, [=](int row) {
					SimdXS::FloatsToUnorm8s(input + row * scan_len, output + row * scan_len, scan_len);
				});

				return Return(L, &blob, res, output, opts.mAsUserdata);
			}, LuaXS::Nil{});
		}
	}, {
		"unorm8s_to_floats", [](lua_State * L)
		{
			return ByteXS::WithByteReader(L, [](lua_State * L, ByteReader & reader) {
				IMAGE_START(2, 0, ImageOpts, 4); // input, iw, ih, opts; 0 = no output dims

				LuaXS::Options{L, 4}.Add("channels", nchannels);

				int res = 1, scan_len = iw * nchannels;
				unsigned char * input = const_cast<unsigned char *>(ByteXS::EnsureN<unsigned char>(L, reader, size_t(ih * scan_len)));
				float * output = GetOutputBuffer<float>(L, blob, opts, iw, ih, nchannels, false);

				ThreadXS::parallel_for(0, ih, [=](int row) {
					SimdXS::Unorm8sToFloats(input + row * scan_len, output + row * scan_len, scan_len);
				});

				return Return(L, &blob, res, output, opts.mAsUserdata);
			}, LuaXS::Nil{});
		}
	}, {
		"rotate", [](lua_State * L)
		{
			return ByteXS::WithByteReader(L, [](lua_State * L, ByteReader & reader)	{
				IMAGE_START(2, 0, ImageOpts, 5); // input, iw, ih, angle, opts; 0 = no output dims

				double angle = luaL_checknumber(L, 4);

				GetSize(iw, ih, ow, oh, angle);

				OPS_ENDTYPED(!PlatformXS::has_accelerate);
			#ifdef IMAGE_APPLE_CALL
				// TODO: could these be options?
				Pixel_8888 bg = { 0 };
				vImage_Flags flags = kvImageBackgroundColorFill;	// TODO: if so, could add background fill options

				opts.mInStride = iw * 4;
				opts.mOutStride = ow * 4;

				IMAGE_APPLE_CALL(Rotate, flags, float(-angle), bg);
			#else
				ne10_uint32_t uow, uoh;	// redundant, but complies with rotate's interface

				IMAGE_OTHER_CALL(rotate_rgba, output, &uow, &uoh, input, ne10_uint32_t(iw), ne10_uint32_t(ih), ne10_uint32_t(angle * 180.0 / NE10_PI + .5));
			#endif

				Return(L, &blob, res, output, opts.mAsUserdata);// input, iw, ih, angle, opts, blob / ud[, str]

				if (res) return 1 + LuaXS::PushMultipleArgsAndReturn(L, ow, oh);// input, iw, ih, angle, opts, blob / ud[, str], ow, oh

				else return 1;
			}, LuaXS::Nil{});
		}
	},
	{ nullptr, nullptr }
};

#undef OPS_ENDTYPED

int luaopen_ops (lua_State * L)
{
	lua_newtable(L);// ..., ops
	luaL_register(L, nullptr, ops_funcs);
	luaopen_resize(L); // Inject "resize" into ops.
	
	return 1;
}