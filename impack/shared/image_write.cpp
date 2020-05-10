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
#include "jo_gif.h"
#include "ByteReader.h"
#include "utils/Byte.h"
#include "utils/LuaEx.h"
#include "utils/Memory.h"
#include "utils/Thread.h"
#include "SpotHelper.h"
#include "spot.hpp"

ThreadXS::TLS<MemoryXS::ScopedSystem *> tls_WriteMM;

// GIF writer from http://www.jonolick.com/home/gif-writer
// MPEG writer from http://www.jonolick.com/home/mpeg-video-writer

#define BOOKMARK() auto bm = tls_WriteMM->Bookmark()

static void Write (void * context, void * data, int)
{
	tls_WriteMM->Push(data, *static_cast<bool *>(context));	// ..., data
}

struct GifData {
	std::vector<unsigned char> mData{};
	short mDelay{0};
	bool mLocalPalette{false};
};

//
template<typename F> int WithUpvalue (lua_State * L, F func)
{
	return LuaXS::PCallWithStackAndUpvaluesThenReturn(L, 1, func, LuaXS::Nil{});
}

//
template<typename F> int WithoutUpvalue (lua_State * L, F func)
{
	return LuaXS::PCallWithStackThenReturn(L, func, false);
}

//
template<typename T = bool> static T False (void) { return false; }

template<> LuaXS::Nil False<LuaXS::Nil> (void) { return LuaXS::Nil{}; }

//
template<bool bGetFilename, typename F> int WriteMultiframe (lua_State * L, F func)
{
	return LuaXS::PCallWithStackAndUpvaluesThenReturn(L, bGetFilename ? 1 : 0, func, False<typename std::conditional<bGetFilename, LuaXS::Nil, bool>::type>());
}

//
template<bool bGetFilename> struct WithExtantFile {
    template<typename F> void Do (const char * name, bool bAppend, F && func)
    {
        if (!bAppend) return;

        FILE * fp = fopen(name, "rb");

        if (fp)
        {
            func(fp);
            fclose(fp);
        }
    }
};

template<> struct WithExtantFile<false> {
    template<typename F> void Do (const char *, bool, F &&) {}
};

template<bool bGetFilename> void EnsureDims (lua_State * L)
{
    if (lua_type(L, 2) == LUA_TNUMBER) return;

    for (int i = 0; i < 2; ++i)
    {
        lua_pushinteger(L, 0);  // filename, ..., 0
        lua_insert(L, 2);  // filename, 0, ...
    }
}

template<> void EnsureDims<false> (lua_State *) {}

//
template<bool bGetFilename> int WriteGIF (lua_State * L)
{
	return WriteMultiframe<bGetFilename>(L, [](lua_State * L) {
		const int i1 = bGetFilename ? 2 : 1;

        EnsureDims<bGetFilename>(L);// [filename, w, h, ]frames[, opts]
		PathXS::WriteAux waux{L, i1, bGetFilename ? GetPathData(L) : nullptr};

		BOOKMARK();

		int repeat = 0, pal = 8;

		LuaXS::Options{L, i1 + 3}   .Add("repeat", repeat)
									.Add("palette_depth", pal)
									.ArgCheck(pal >= 2 && pal <= 8, "Invalid palette size");
/*
 TODO: backing off for now; has trailing byte that complicates append
        WithExtantFile<bGetFilename>{}.Do(waux.mFilename, bAppend, [&waux, &pal,&bDid](FILE * fp) {
            fseek(fp, 6, SEEK_SET); // "GIF89a"

            waux.mW = waux.mH = 0;
            fread(&waux.mW, 2, 1, fp);
            fread(&waux.mH, 2, 1, fp);

            pal = int(log2(pal_size));

            fread(&pal_size, 1, 1, fp);
        });
*/
        luaL_checktype(L, i1 + 2, LUA_TTABLE);

		size_t n = lua_objlen(L, i1 + 2), ntotal = size_t(waux.mW * waux.mH * 4);

		std::vector<GifData> frames(n);

		for (auto elem : LuaXS::Range(L, i1 + 2))
		{
			auto & frame = frames[elem - 1];

			//
			LuaXS::Options{L, -1}	.Add("delay", frame.mDelay)
									.Add("has_local_palette", frame.mLocalPalette)
									.Replace("image"); // [filename, ]w, h, frames, bytes

			// Get the bytes, augmenting them at least to the per-frame size if necessary. If we
			// get any bytes at all, store the frame-sized content, possibly padded with zeroes.
			ByteReader bytes{L, -1};

			auto ucbytes = waux.GetBytes(L, bytes, waux.mW * 4U);	// [filename, ]w, h, frames, bytes / err

			frame.mData.assign(ucbytes, ucbytes + ntotal);
		}

		//
        JO_FileAlloc file{L, bGetFilename ? waux.mFilename : nullptr, *tls_WriteMM};

		if (bGetFilename && !file.mFP) luaL_error(L, "Error: Could not WriteGif to %s", waux.mFilename);// filename, w, h, frames, false, err

		jo_gif_t gif = jo_gif_start(&file, short(waux.mW), short(waux.mH), short(repeat), (1 << pal) - 1);

		//
		for (size_t i = 0; i < frames.size(); ++i) jo_gif_frame(&gif, frames[i].mData.data(), frames[i].mDelay, frames[i].mLocalPalette);

		jo_gif_end(&gif);	// [filename, ]w, h, frames, true / memory

		return 1;
	});
}

//
template<bool bGetFilename> int WriteMPEG (lua_State * L)
{
	return WriteMultiframe<bGetFilename>(L, [](lua_State * L) {
		const int i1 = bGetFilename ? 2 : 1;

        EnsureDims<bGetFilename>(L);// [filename, w, h, ]frames[, opts]
        PathXS::WriteAux waux{L, i1, bGetFilename ? GetPathData(L) : nullptr};

		int fps = 30;
        bool bAppend = false;
	
		LuaXS::Options{L, i1 + 3}   .Add("append", bAppend)
                                    .Add("fps", fps);

        WithExtantFile<bGetFilename>{}.Do(waux.mFilename, bAppend, [&waux, &fps](FILE * fp) {
            fseek(fp, 4, SEEK_SET); // sequence header

            // 12 bits for width, height
            int8_t n1, n2, n3;
            
            fread(&n1, 1, 1, fp);
            fread(&n2, 1, 1, fp);
            fread(&n3, 1, 1, fp);
            
            waux.mW = (int(n1) << 4) | (int(n2 & 0xF0) >> 4);
            waux.mH = (int(n2 & 0xF) << 8) | int(n3 & 0xFF);
            
            fps = 0;
            
            fread(&fps, 1, 1, fp);
            
            switch (fps)
            {
                case 0x12:
                    fps = 24;
                    break;
                case 0x13:
                    fps = 25;
                    break;
                case 0x15:
                    fps = 30;
                    break;
                case 0x16:
                    fps = 50;
                    break;
                default:
                    fps = 60;
                    break;
            }
        });

		lua_settop(L, i1 + 2);	// [filename, ]w, h, frames
		luaL_checktype(L, i1 + 2, LUA_TTABLE);

		size_t n = lua_objlen(L, i1 + 2), ntotal = size_t(waux.mW * waux.mH * 4);

		std::vector<unsigned char> mpeg(n * ntotal);

		for (auto elem : LuaXS::Range(L, i1 + 2))
		{
			ByteReader bytes{L, -1};

			auto ucbytes = waux.GetBytes(L, bytes, waux.mW * 4);// [filename, ]w, h, frames, bytes / err

			memcpy(mpeg.data() + (elem - 1) * ntotal, ucbytes, ntotal);
		}

		//
        JO_File file{L, bGetFilename ? waux.mFilename : nullptr, bAppend ? "ab" : "wb"};

		if (bGetFilename && !file.mFP) luaL_error(L, "Error: Could not WriteMPEG to %s", waux.mFilename);	// filename, w, h, frames, false, err

		//
		for (size_t i = 0; i < n; ++i) jo_write_mpeg(&file, mpeg.data() + i * ntotal, waux.mW, waux.mH, fps);

		file.Close();	// [filename, ]w, h, frames, true / memory

		return 1;
	});
}

static luaL_Reg write_funcs[] = {
	{
		"bmp", [](lua_State * L)
		{
			return WithUpvalue(L, [](lua_State * L) {
				PathXS::WriteData<> wd{L, GetPathData(L)};

				BOOKMARK();

				return LuaXS::BoolResult(L, stbi_write_bmp(wd.mFilename, wd.mW, wd.mH, wd.mComp, wd.mData));// filename, w, h, comp, data, ok
			});
		}
	}, {
		"bmp_to_memory", [](lua_State * L)
		{
			return WithoutUpvalue(L, [](lua_State * L) {
				PathXS::WriteData<> wd{L};

				BOOKMARK();

				return LuaXS::ResultOrNil(L, stbi_write_bmp_to_func(&Write, &wd.mAsUserdata, wd.mW, wd.mH, wd.mComp, wd.mData));// w, h, comp, data[, bmp]
			});
		}
	}, {
		"gif", WriteGIF<true>
	}, {
		"gif_to_memory", WriteGIF<false>
	}, {
		"hdr", [](lua_State * L)
		{
			return WithUpvalue(L, [](lua_State * L) {
				PathXS::WriteData<float> wd{L, GetPathData(L)};

				BOOKMARK();

				return LuaXS::BoolResult(L, stbi_write_hdr(wd.mFilename, wd.mW, wd.mH, wd.mComp, wd.mData));// filename, w, h, comp, data, ok
			});
		}
	}, {
		"hdr_to_memory", [](lua_State * L)
		{
			return WithoutUpvalue(L, [](lua_State * L) {
				PathXS::WriteData<float> wd{L};

				BOOKMARK();

				return LuaXS::ResultOrNil(L, stbi_write_hdr_to_func(&Write, &wd.mAsUserdata, wd.mW, wd.mH, wd.mComp, wd.mData));// w, h, comp, data[, hdr]
			});
		}
	}, {
		"jpg", [](lua_State * L)
		{
			return WithUpvalue(L, [](lua_State * L) {
				PathXS::WriteData<> wd{L, GetPathData(L), PathXS::WriteData<>::Quality};

				luaL_argcheck(L, wd.mExtra > 0 && wd.mExtra <= 100, 6, "Invalid quality");

				JO_File file{L, wd.mFilename};

				if (!file.mFP) luaL_error(L, "Error: Could not write JPG to %s", wd.mFilename);	// filename, w, h, comp, data[, opts], false, err

				bool bOK = jo_write_jpg(&file, wd.mData, wd.mW, wd.mH, wd.mComp, wd.mExtra);

				file.Close();

				return LuaXS::BoolResult(L, bOK);	// filename, w, h, comp, data[, opts], ok
			});
		}
	}, {
		"jpg_to_memory", [](lua_State * L)
		{
			return WithoutUpvalue(L, [](lua_State * L) {
				PathXS::WriteData<> wd{L, nullptr, PathXS::WriteData<>::Quality};

				luaL_argcheck(L, wd.mExtra > 0 && wd.mExtra <= 100, 6, "Invalid quality");

				JO_File file{L, nullptr};

				bool bOK = jo_write_jpg(&file, wd.mData, wd.mW, wd.mH, wd.mComp, wd.mExtra);

				if (bOK) file.Close();	// w, h, comp, data[, opts][, jpg]

				return LuaXS::ResultOrNil(L, bOK);	// w, h, comp, data[, opts], jpg / nil
			});
		}
	}, {
		"mpeg", WriteMPEG<true>
	}, {
		"mpeg_to_memory", WriteMPEG<false>
	}, {
		"png", [](lua_State * L)
		{
			return WithUpvalue(L, [](lua_State * L) {
				PathXS::WriteData<> wd{L, GetPathData(L), PathXS::WriteData<>::Stride};

				BOOKMARK();

				return LuaXS::BoolResult(L, stbi_write_png(wd.mFilename, wd.mW, wd.mH, wd.mComp, wd.mData, wd.mExtra));	// filename, w, h, comp, data[, opts], ok
			});
		}
	}, {
		"png_to_memory", [](lua_State * L)
		{
			return WithoutUpvalue(L, [](lua_State * L) {
				PathXS::WriteData<> wd{L, nullptr, PathXS::WriteData<>::Stride};

				BOOKMARK();

				return LuaXS::ResultOrNil(L, stbi_write_png_to_func(&Write, &wd.mAsUserdata, wd.mW, wd.mH, wd.mComp, wd.mData, wd.mExtra));	// w, h, comp, data[, opts][, png]
			});
		}
	}, {
		"tga", [](lua_State * L)
		{
			return WithUpvalue(L, [](lua_State * L) {
				PathXS::WriteData<> wd{L, GetPathData(L)};

				BOOKMARK();

				return LuaXS::BoolResult(L, stbi_write_tga(wd.mFilename, wd.mW, wd.mH, wd.mComp, wd.mData));// filename, w, h, comp, data, ok
			});
		}
	}, {
		"tga_to_memory", [](lua_State * L)
		{
			return WithoutUpvalue(L, [](lua_State * L) {
				PathXS::WriteData<> wd{L};

				BOOKMARK();

				return LuaXS::ResultOrNil(L, stbi_write_tga_to_func(&Write, &wd.mAsUserdata, wd.mW, wd.mH, wd.mComp, wd.mData));// w, h, comp, data[, tga]
			});
		}
	},
	{ nullptr, nullptr }
};

#undef BOOKMARK

int luaopen_write (lua_State * L)
{
	tls_WriteMM = MemoryXS::ScopedSystem::New(L);

	lua_newtable(L);// ..., write

	LuaXS::AddClosures(L, write_funcs, 1, LuaXS::AddParams{-1, lua_upvalueindex(1)});	// ..., write

	return 1;
}
