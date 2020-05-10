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
#include "utils/Blob.h"
#include "utils/Byte.h"
#include "utils/Compat.h"
#include "utils/LuaEx.h"
#include "utils/Memory.h"
#include "utils/SIMD.h"
#include "utils/Thread.h"
#include "SpotHelper.h"
#include "SpotInterface.h"
#include "spot.hpp"
#include <utility>

#include "src/webp/decode.h"
extern "C" uint8_t* WebPDecodeRGBA(const uint8_t* data, size_t data_size,
                                   int* width, int* height);
ThreadXS::TLS<MemoryXS::ScopedSystem *> tls_ImageMM;

template<void (*func)(float)> int FloatFunc (lua_State * L)
{
	func(LuaXS::Float(L, 1));

	return 0;
}

static int PushWHComp (lua_State * L, int res, int w, int h, int comp)
{
	if (res) return LuaXS::PushMultipleArgsAndReturn(L, true, w, h, comp);	// ..., true, w, h, comp
	
	else return LuaXS::PushMultipleArgsAndReturn(L, false, stbi_failure_reason());	// ..., false, err
}

struct ImageLoadOpts : ImageOpts {
	int mReqComps{0};
    bool mBypassFiltering{false}, mNoFancyUpsampling{false}, mPremultiply{false};

	void AddFields (lua_State * L) override
    {
		LuaXS::Options{L, 2}.Add("req_comp", mReqComps)
							.Add("x", mX)
							.Add("y", mY)
							.Add("out_stride", mOutStride)
                            .Add("bypass_filtering", mBypassFiltering)
                            .Add("no_fancy_upsampling", mNoFancyUpsampling)
                            .Add("premultiply", mPremultiply)
							.Add("as_userdata", mAsUserdata);
	}
};

static inline unsigned char Premult (unsigned char x, unsigned char a)
{
    return static_cast<unsigned char>((static_cast<unsigned short>(x) * static_cast<unsigned short>(a)) >> 8U);
}

template<typename T> static void Premultiply (T *, int) {}

template<> void Premultiply<unsigned char> (unsigned char * res, int n)
{
    for (; n--; res += 4)
    {
        res[0] = Premult(res[0], res[3]);
        res[1] = Premult(res[1], res[3]);
        res[2] = Premult(res[2], res[3]);
    }
}

template<> void Premultiply<spot::pixel> (spot::pixel * res, int n)
{
    while (n--)
    {
        spot::pixel & pixel = *res++;
        
        pixel.r = Premult(pixel.r, pixel.a);
        pixel.g = Premult(pixel.g, pixel.a);
        pixel.b = Premult(pixel.b, pixel.a);
    }
}

template<typename T> static void Premultiply (const T * _RESTRICT, unsigned char * _RESTRICT, int) {}

template<> void Premultiply<unsigned char> (const unsigned char * _RESTRICT res, unsigned char * _RESTRICT out, int n)
{
    for (; n--; res += 4, out += 4)
    {
        out[0] = Premult(res[0], res[3]);
        out[1] = Premult(res[1], res[3]);
        out[2] = Premult(res[2], res[3]);
        out[3] = res[3];
    }
}

template<> void Premultiply<spot::pixel> (const spot::pixel * _RESTRICT res, unsigned char * _RESTRICT out, int n)
{
    for (; n--; out += 4)
    {
        const spot::pixel & pixel = *res++;

        out[0] = Premult(pixel.r, pixel.a);
        out[1] = Premult(pixel.g, pixel.a);
        out[2] = Premult(pixel.b, pixel.a);
        out[3] = pixel.a;
    }
}

template<typename T> static void PushResult (lua_State *, T * res, int, int)
{
	tls_ImageMM->Push(res);	// ..., ud
}

template<> void PushResult<spot::pixel> (lua_State * L, spot::pixel * res, int w, int h)
{
	lua_pushlstring(L, reinterpret_cast<const char *>(res), size_t(w * h * sizeof(spot::pixel)));	// ..., str
}

template<typename T> static void PushData (lua_State * L, T * res, const ImageLoadOpts & opts, int w, int h, bool bShouldPremultiply = false)
{
    BlobXS::State blob{L, -1, "blob"};	// ..., blob?

    unsigned char * out = blob.PointToDataIfBound(L, opts.mX, opts.mY, w, h, opts.mOutStride, sizeof(T));

    if (out)
    {
        if (bShouldPremultiply)
        {
            int stride = opts.mOutStride;
            
            if (!stride) stride = w * 4;

            for (int row = 0; row < h; ++row, out += stride, res += w) Premultiply(res, out, w);
        }

        else if (!spot::ExternalMemory::GetBinding()) blob.LoadFrom(res);
    }

	else
    {
        if (bShouldPremultiply) Premultiply(res, w * h);
        
		lua_pop(L, 1);	// ...

        PushResult(L, res, w, h);	// ..., res_str
	}
}

template<typename T> static int PushWHComp_Data (lua_State * L, T * res, int w, int h, int comp, const ImageLoadOpts & opts = ImageLoadOpts{})
{
    if (res)
	{
		if (comp == -1) PushData(L, reinterpret_cast<spot::texture *>(res)->data(), opts, w, h, opts.mPremultiply); // ..., data
        else PushData(L, res, opts, w, h, comp == 4 && opts.mPremultiply);	// ..., data

        return 1 + LuaXS::PushMultipleArgsAndReturn(L, w, h, comp == -1 ? 4 : comp);// ..., data, w, h, comp
	}																			

	else return LuaXS::PushMultipleArgsAndReturn(L, LuaXS::Nil{}, stbi_failure_reason());	// ..., nil, err
}

template<bool bGetFilename> struct XLoader {
	template<typename F> int operator ()(lua_State * L, F rest)
	{
        bool bIsAbsolute = ExtractFileArgs(L);	// filename[, base_dir]

        return WithFilename(L, [rest](lua_State * L, const char * name) {
            int w, h, frames, * delays;
			stbi_uc * res = stbi_xload(name, &w, &h, &frames, &delays, nullptr);

            return rest(res, w, h, frames, delays);
		},
    #ifdef __ANDROID__
        [rest](lua_State * L, ByteReader & bytes) {
            int w, h, frames, * delays;
            stbi_uc * res = stbi_xload(nullptr, &w, &h, &frames, &delays, &bytes);

            return rest(res, w, h, frames, delays);
        },
    #endif
        LuaXS::Nil{}, bIsAbsolute);
	}
};

template<> struct XLoader<false> {
	template<typename F> int operator ()(lua_State * L, F rest)
	{
		return ByteXS::WithByteReader(L, [rest](lua_State * L, ByteReader & reader) {
			int w, h, frames, * delays;
			stbi_uc * res = stbi_xload(nullptr, &w, &h, &frames, &delays, &reader);

			return rest(res, w, h, frames, delays);
		}, LuaXS::Nil{});
	}
};

template<bool bGetFilename> int XLoad (lua_State * L)
{
	auto bm = tls_ImageMM->Bookmark();

	return XLoader<bGetFilename>{}(L, [L](stbi_uc * res, int w, int h, int frames, int * delays)
	{
		if (res)
		{
			if (frames > 1)
			{
				lua_createtable(L, frames, 0);	// filename, frames

				size_t size = size_t(w * h * 4);
				stbi_uc * p = res;

				for (int i = 1; i <= frames; ++i)
				{
					lua_createtable(L, 0, 2);	// filename, frames, frame
					lua_pushlstring(L, reinterpret_cast<const char *>(p), size);// filename, frames, frame, data
					lua_setfield(L, -2, "image");	// filename, frames, frame = { image = data }

					p += size;
/*
					stbi_uc lob = *p++;
					stbi_uc hib = *p++;
					*/
					lua_pushinteger(L, delays[i - 1]);//((hib << 8) | lob) * 10);// filename, frames, frame, delay
					lua_setfield(L, -2, "delay");	// filename, frames, frame = { image, delay = delay }
					lua_rawseti(L, -2, i);	// filename, frames = { ..., frame }
				}

				STBI_FREE(res);
				STBI_FREE(delays);

				return 1 + LuaXS::PushMultipleArgsAndReturn(L, w, h);	// filename, frames, w, h
			}

			else
			{
				STBI_FREE(delays);

				return PushWHComp_Data(L, res, w, h, 4);
			}
		}

		STBI_FREE(delays);

		return luaL_error(L, stbi_failure_reason());
	});
}

#define STBI_UC() static_cast<const stbi_uc *>(reader.mBytes), int(reader.mCount)

//
template<typename F, typename ... Args> static int Info (lua_State * L, F && func, Args && ... args)
{
	lua_settop(L, 1);	// source

    int w, h, comp, res = func(std::forward<Args>(args)..., &w, &h, &comp);

	return PushWHComp(L, res, w, h, comp);	// source, ok[, w, h, comp]
}

//
template<typename F, typename ... Args> static int IsHDR (lua_State * L, F && func, Args && ... args)
{
    return LuaXS::BoolResult(L, func(std::forward<Args>(args)...));	// source, ok
}

//
template<typename F, typename ... Args> static int Load (lua_State * L, F && func, Args && ... args)
{
    lua_settop(L, 2);	// source, opts
// ^^ TODO: robust against baseDir?
	ImageLoadOpts opts;

    opts.AddFields(L);

    spot::ExternalMemory mem;

    if (opts.mX == 0 && opts.mY == 0 && opts.mInStride == 0)
    {
        BlobXS::State state{L, -1, "blob"};   // source, opts, blob?

        if (state.Bound() && BlobXS::IsResizable(L, -1) && BlobXS::GetAlignment(L, -1) == 0)
        {
            mem.mRGBA = BlobXS::GetVectorN<0U>(L, -1);
            
            if (opts.mBypassFiltering) mem.mFlags |= spot::ExternalMemory::eBypassFiltering;
            if (opts.mNoFancyUpsampling) mem.mFlags |= spot::ExternalMemory::eNoFancyUpsampling;
            if (opts.mPremultiply) mem.mFlags |= spot::ExternalMemory::ePremultiply;
            
            spot::ExternalMemory::Bind(&mem);
        }

        lua_pop(L, 1);  // source, opts
    }
    
    int w, h, comp;

	auto bm = tls_ImageMM->Bookmark();
    auto res = func(std::forward<Args>(args)..., &w, &h, &comp, opts.mReqComps);
    AddLapse(6);

    if (mem.mStatus & spot::ExternalMemory::eWasPremultiplied) opts.mPremultiply = false;
    
    int result = PushWHComp_Data(L, res, w, h, comp, opts);
    
    spot::ExternalMemory::Bind(nullptr);
    
    return result;
}

#ifdef _WIN32
	#include <gl/GL.h>
#elif __ANDROID__
    #include <GLES2/gl2.h>
#else
	#include "TargetConditionals.h"

	#if TARGET_OS_IPHONE
		#include <OpenGLES/ES2/gl.h>
	#else
		#include <OpenGL/gl.h>
	#endif
#endif

static bool TextureFailed (const spot::texture & tex)
{
	return spot::GetLoadResult() == spot::eLoadFailed;
}

static unsigned char * BindTexture (spot::texture & tex, spot::texture && temp, int * x, int * y, int * comp, int req_comp)
{
    AddLapse(0);
    if (TextureFailed(temp)) return nullptr;
    if (!temp.mError.empty()) tls_ImageMM->FailAssert(temp.mError.c_str());
    if (req_comp < 0 || req_comp > 4) tls_ImageMM->FailAssert("Invalid required components");

    tex.swap(temp);

    *x = int(temp.w);
    *y = int(temp.h);
    *comp = req_comp ? req_comp : 4;

    spot::ExternalMemory * mem = spot::ExternalMemory::GetBinding();
    
    if (mem) return mem->mRGBA->data();

    if (req_comp > 0 && req_comp < 4)
    {
        unsigned char * data = reinterpret_cast<unsigned char *>(tex.data()), * read = data, * write = data;

        for (int i = 0, n = int(temp.w * temp.h); i < n; ++i, read += 4, write += req_comp) memcpy(write, read, req_comp);

        return data;
    }
    
    else
    {
        *comp = -1;// Marker for texture

        return reinterpret_cast<unsigned char *>(&tex);
    }
}

static unsigned char * LoadEx (spot::texture & tex, const char * filename, int * x, int * y, int * comp, int req_comp)
{
	spot::SetLoadResult(spot::eLoadNone);
    GetNow();AddLapse(33);
	unsigned char * ptr = stbi_info(filename, x, y, comp) ? nullptr : BindTexture(tex, spot::texture{filename}, x, y, comp, req_comp);
	// ^^ Try Spot if info fails; if Spot itself fails, we might still have a TGA (whose test claims to be flaky)
	// VV If we have info or Spot failed, try a STB load
	return ptr ? ptr : stbi_load(filename, x, y, comp, req_comp);
}

static unsigned char * LoadExFromMemory (spot::texture & tex, const stbi_uc * buffer, int len, int * x, int * y, int * comp, int req_comp)
{
    unsigned char * ptr = stbi_info_from_memory(buffer, len, x, y, comp) ? nullptr : BindTexture(tex, spot::texture{buffer, size_t(len)}, x, y, comp, req_comp);
	
	return ptr ? ptr : stbi_load_from_memory(buffer, len, x, y, comp, req_comp);
}

static luaL_Reg image_funcs[] = {
	{
		"capture", [](lua_State * L)
		{
			int	viewport[4];

			glGetIntegerv(GL_VIEWPORT, viewport);
	
			int w = viewport[2], h = viewport[3];

			std::vector<unsigned char> pixels(size_t(w * h) * 4U);

			glReadPixels(0, 0, w, h, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());

			lua_pushlstring(L, reinterpret_cast<const char *>(pixels.data()), pixels.size());// data
			lua_pushinteger(L, w);	// data, w
			lua_pushinteger(L, h);	// data, w, h

			return 3;
		}
	},
	{
		"hdr_to_ldr_gamma", FloatFunc<&stbi_hdr_to_ldr_gamma>
	}, {
		"hdr_to_ldr_scale", FloatFunc<&stbi_hdr_to_ldr_scale>
	}, {
		"info_from_memory", [](lua_State * L)
		{
			return ByteXS::WithByteReader(L, [](lua_State * L, ByteReader & reader) {
				return Info(L, stbi_info_from_memory, STBI_UC());
			}, false);
		}
	}, {
		"is_hdr_from_memory", [](lua_State * L)
		{
			return ByteXS::WithByteReader(L, [](lua_State * L, ByteReader & reader) {
				return IsHDR(L, stbi_is_hdr_from_memory, STBI_UC());
			}, false);
		}
	}, {
		"ldr_to_hdr_gamma", FloatFunc<&stbi_ldr_to_hdr_gamma>
	}, {
		"ldr_to_hdr_scale", FloatFunc<&stbi_ldr_to_hdr_scale>
	}, {
		"load_from_memory", [](lua_State * L)
		{
			return ByteXS::WithByteReader(L, [](lua_State * L, ByteReader & reader) {
                spot::texture tex;
                
				return Load(L, LoadExFromMemory, tex, STBI_UC());
			}, LuaXS::Nil{});
		}
	}, {
		"loadf_from_memory", [](lua_State * L)
		{
			return ByteXS::WithByteReader(L, [](lua_State * L, ByteReader & reader) {
				return Load(L, stbi_loadf_from_memory, STBI_UC());
			}, LuaXS::Nil{});
		}
	}, {
		"load_image_object_from_memory", [](lua_State * L)
		{
			return ByteXS::WithByteReader(L, [](lua_State * L, ByteReader & reader) {
				auto bm = tls_ImageMM->Bookmark();

				return InstantiateSpotImage(L, spot::image{reader.mBytes, reader.mCount});
			}, LuaXS::Nil{});
		}
	}, {
		"new_color_hlsa", [](lua_State * L)
		{
			return LuaXS::PCallWithStackThenReturn(L, [](lua_State * L) {
				auto bm = tls_ImageMM->Bookmark();
                
                float h = (float)luaL_optnumber(L, 1, 0.0), s = (float)luaL_optnumber(L, 2, 0.0), l = (float)luaL_optnumber(L, 3, 0.0);
				float a = (float)luaL_optnumber(L, 4, 1.0);

				return InstantiateSpotColor(L, spot::color{h, s, l, a});
			}, LuaXS::Nil{});
		}
	}, {
		"new_color_rgba", [](lua_State * L)
		{
			return LuaXS::PCallWithStackThenReturn(L, [](lua_State * L) {
				auto bm = tls_ImageMM->Bookmark();
                
				float r = (float)luaL_optnumber(L, 1, 0.0), g = (float)luaL_optnumber(L, 2, 0.0), b = (float)luaL_optnumber(L, 3, 0.0);
				float a = (float)luaL_optnumber(L, 4, 0.0);

				return InstantiateSpotColor(L, spot::pixel{r, g, b, a});
			}, LuaXS::Nil{});
		}
	}, {
		"new_image_object", [](lua_State * L)
		{
			return LuaXS::PCallWithStackThenReturn(L, [](lua_State * L) {
				auto bm = tls_ImageMM->Bookmark();
                int w = luaL_checkint(L, 1), h = luaL_checkint(L, 2), d = luaL_optint(L, 3, 0);

				luaL_argcheck(L, w > 0, 1, "Invalid width");
				luaL_argcheck(L, h > 0, 2, "Invalid height");
				luaL_argcheck(L, d >= 0, 3, "Invalid depth");

				spot::color filler;

				if (!lua_isnoneornil(L, 4)) filler = *Color(L, 4);

				return InstantiateSpotImage(L, spot::image{size_t(w), size_t(h), size_t(d), filler});
			}, LuaXS::Nil{});
		}
	}, {
		"xload_from_memory", XLoad<false>
	},
	{ nullptr, nullptr }
};

static luaL_Reg image_pd_funcs[] = {
	{
		"info", [](lua_State * L)
		{
			bool bIsAbsolute = ExtractFileArgs(L);	// filename[, base_dir]

			return WithFilename(L, [](lua_State * L, const char * name) {
				return Info(L, stbi_info, name);
			},
        #ifdef __ANDROID__
            [](lua_State * L, ByteReader & reader) {
                return Info(L, stbi_info_from_memory, STBI_UC());
            },
        #endif
            false, bIsAbsolute);
		}
	}, {
		"is_hdr", [](lua_State * L)
		{
			bool bIsAbsolute = ExtractFileArgs(L);	// filename[, base_dir]

			return WithFilename(L, [](lua_State * L, const char * name) {
				return IsHDR(L, stbi_is_hdr, name);
			},
        #ifdef __ANDROID__
            [](lua_State * L, ByteReader & reader) {
                return IsHDR(L, stbi_is_hdr_from_memory, STBI_UC());
            },
        #endif
            false, bIsAbsolute);
		}
	}, {
		"load", [](lua_State * L)
        {AddLapse(99);
			return WithFilename(L, [](lua_State * L, const char * name) {
                spot::texture tex;

				return Load(L, LoadEx, tex, name);
			},
        #ifdef __ANDROID__
            [](lua_State * L, ByteReader & reader) {
                spot::texture tex;
                
                return Load(L, LoadExFromMemory, tex, STBI_UC());
            },
        #endif
            LuaXS::Nil{}, FileArgsFromOpts(L));
        }
	}, {
		"loadf", [](lua_State * L)
		{
			return WithFilename(L, [](lua_State * L, const char * name) {
				return Load(L, stbi_loadf, name);
			},
        #ifdef __ANDROID__
            [](lua_State * L, ByteReader & reader) {
                return Load(L, stbi_loadf_from_memory, STBI_UC());
            },
        #endif
            LuaXS::Nil{}, FileArgsFromOpts(L));
		}
	}, {
		"load_image_object", [](lua_State * L)
		{
			bool bIsAbsolute = ExtractFileArgs(L);	// filename[, base_dir]
			
			return WithFilename(L, [](lua_State * L, const char * name) {
				auto bm = tls_ImageMM->Bookmark();
                
                return InstantiateSpotImage(L, spot::image{name});
            },
        #ifdef __ANDROID__
            [](lua_State * L, ByteReader & reader) {
                auto bm = tls_ImageMM->Bookmark();
                
                return InstantiateSpotImage(L, spot::image{reader.mBytes, reader.mCount});
            },
        #endif
            LuaXS::Nil{}, bIsAbsolute);
		}
	}, {
		"xload", XLoad<true>
	},

    {
        "LogGetNow", [](lua_State * L)
        {
            GetNow();
            return 0;
        }
    }, {
        "LogAddLapse", [](lua_State * L)
        {
            AddLapse(luaL_optint(L, 1, 0));
            return 0;
        }
    }, {
        "LogShowLapses", [](lua_State * L)
        {
            ShowLapses();
            return 0;
        }
    }, {
        "LogEnable", [](lua_State * L)
        {
            Enable(LuaXS::Bool(L, 1));
            return 0;
        }
    }, {
        "LogTry", [](lua_State * L)
        {
            AddLapse(17);
            FILE * fp = fopen(lua_tostring(L, 1), "rb");
            if (fp)
            {
                AddLapse(18);
                fseek(fp, 0L, SEEK_END);
                long size=ftell(fp);
                rewind(fp);
                AddLapse(19);
                uint8_t * BUFFER = new uint8_t[size];
                AddLapse(20);
                fread(BUFFER, sizeof(uint8_t), size, fp);
                fclose(fp);
                AddLapse(21);
                int w, h;
                
                WebPDecoderConfig webp_config;
                WebPInitDecoderConfig(&webp_config);
                
                // B) optional: retrieve the bitstream's features.
                WebPGetFeatures(BUFFER, size_t(size), &webp_config.input);
                
                // C) Adjust 'config', if needed
                //    config.no_fancy_upsampling = 1;
                //    config.output.colorspace = MODE_BGRA;
                webp_config.options.no_fancy_upsampling = 1;
                webp_config.options.bypass_filtering = 1;
                webp_config.options.use_threads = 1;
                webp_config.output.colorspace = MODE_rgbA;
                // etc.
                
                // Note that you can also make config.output point to an externally
                // supplied memory buffer, provided it's big enough to store the decoded
                // picture. Otherwise, config.output will just be used to allocate memory
                // and store the decoded picture.
                w = webp_config.input.width;
                h = webp_config.input.height;
                uint8_t * out = nullptr;
                // D) Decode!
                if (WebPDecode(BUFFER, size_t(size), &webp_config) == VP8_STATUS_OK)
                {
                    // E) Decoded image is now in config.output (and config.output.u.RGBA)
                    
                    // F) Reclaim memory allocated in config's object. It's safe to call
                    // this function even if the memory is external and wasn't allocated
                    // by WebPDecode().
                    //     WebPFreeDecBuffer(&config.output);
                    
                    out = webp_config.output.u.RGBA.rgba;
                }
                
                
            //    uint8_t* out=WebPDecodeRGBA(BUFFER, size, &w, &h);
                AddLapse(22);
                if (out) free(out);
                delete [] BUFFER;
                AddLapse(23);
            }
                return 0;
        }
    },
    
    { nullptr, nullptr }
};

//
int luaopen_image (lua_State * L)
{
	tls_ImageMM = MemoryXS::ScopedSystem::New(L);

	//
	lua_newtable(L);// ..., image
	luaL_register(L, nullptr, image_funcs);

	LuaXS::AddClosures(L, image_pd_funcs, 1, LuaXS::AddParams{-1, lua_upvalueindex(1)});

	InstantiateSpotImage(L, spot::image{});	// ..., image, ud; n.b. this is just to make the metatable and capture the above upvalue there

	lua_pop(L, 1);	// ..., image

	return 1;
}
