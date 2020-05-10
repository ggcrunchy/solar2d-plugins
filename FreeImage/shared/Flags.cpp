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

#include "stdafx.h"
#include "classes.h"
#include <cstring>

struct FlagEntry {
	const char * name;
	int flag;
};

#define FLAG_BMP(n) { #n, BMP_##n }
#define FLAG_EXR(n) { #n, EXR_##n }
#define FLAG_GIF(n) { #n, GIF_##n }
#define FLAG_ICO(n) { #n, ICO_##n }
#define FLAG_JPEG(n) { #n, JPEG_##n }
#define FLAG_PCD(n) { #n, PCD_##n }
#define FLAG_PNG(n) { #n, PNG_##n }
#define FLAG_PNM(n) { #n, PNM_##n }
#define FLAG_PSD(n) { #n, PSD_##n }
#define FLAG_RAW(n) { #n, RAW_##n }
#define FLAG_TARGA(n) { #n, TARGA_##n }
#define FLAG_TIFF(n) { #n, TIFF_##n }
#define FLAG_WEBP(n) { #n, WEBP_##n }
#define FLAG_JXR(n) { #n, JXR_##n }

static int FlagRef;

template<size_t n> void AddFlags (lua_State * L, FREE_IMAGE_FORMAT fif, FlagEntry (&arr)[n])
{
	lua_rawgeti(L, LUA_REGISTRYINDEX, FlagRef);	// ..., flags
	lua_createtable(L, n, 0);	// ..., flags, type_flags

    for (auto entry : arr)
    {
		lua_pushstring(L, entry.name);	// ..., flags, type_flags, name
		lua_pushinteger(L, entry.flag);	// ..., flags, type_flags, name, flag
		lua_rawset(L, -3);	// ..., flags, type_flags = { ..., name = flag }
	}

	lua_rawseti(L, -2, fif);// ..., flags = { ..., fif = type_flags }
	lua_pop(L, 1);	// ...
}

static void GetFlagTable (lua_State * L, FREE_IMAGE_FORMAT fif)
{
	lua_rawgeti(L, LUA_REGISTRYINDEX, FlagRef);	// ..., flags
	lua_rawgeti(L, -1, fif);// ..., flags, ft

	if (lua_isnil(L, -1))	// flagless type: give it a dummy table to simplify some operations
	{
		lua_newtable(L);// ..., flags, nil, dummy
		lua_pushvalue(L, -1);	// ..., flags, nil, dummy, dummy
		lua_rawseti(L, -4, fif);// ..., flags = { fif = dummy }, nil, dummy
		lua_insert(L, -2);	// ..., flags, dummy
	}

	lua_insert(L, -2);	// ..., ft
}

static int FindFlag (lua_State * L, int index, bool bLoading)
{
	index = CoronaLuaNormalize(L, index);

	int ft = CoronaLuaNormalize(L, -2), flag;

	lua_pushvalue(L, index);// ft[, name], name
	lua_rawget(L, ft);	// ft[, name], flag

	if (!lua_isnil(L, -1)) flag = lua_tointeger(L, -1);

	else
	{
		const char * str = lua_tostring(L, index);

		if (strcmp(str, "DEFAULT") == 0) flag = 0;
		else if (strcmp(str, "LOAD_NOPIXELS") == 0) flag = bLoading ? FIF_LOAD_NOPIXELS : 0; // conflicts with JPEG save flag...
		else luaL_error(L, "%s is not one of the format's options", str);
	}

	lua_pop(L, 2);	// ...

	return flag;
}

static int GetFlags (lua_State * L, FREE_IMAGE_FORMAT fif, int index, bool bLoading)
{
	index = CoronaLuaNormalize(L, index);

	if (lua_isstring(L, index))
	{
		GetFlagTable(L, fif);	// ..., ft

		return FindFlag(L, index, bLoading);	// ...
	}

	else if (lua_istable(L, index))
	{
		int flags = 0, n = lua_objlen(L, index);

		GetFlagTable(L, fif);	// ..., ft

		for (int i = 1; i < n; ++i)
		{
			lua_rawgeti(L, index, i);	// ..., ft, opt

			if (!lua_isstring(L, -1)) luaL_error(L, "Entry %i is not a string", i);

			flags |= FindFlag(L, -1, bLoading);	// ..., ft
		}

		lua_pop(L, 1);	// ...

		return flags;
	}

	return 0;
}

int GetLoadFlags (lua_State * L, FREE_IMAGE_FORMAT fif, int index)
{
	return GetFlags(L, fif, index, true);
}

int GetSaveFlags (lua_State * L, FREE_IMAGE_FORMAT fif, int index)
{
	return GetFlags(L, fif, index, false);
}

int FI_LoadFlags (lua_State * L)
{
	lua_newtable(L);// ..., flags

	FlagRef = luaL_ref(L, LUA_REGISTRYINDEX);	// ...

	FlagEntry bmp_flags[] = {
		FLAG_BMP(SAVE_RLE)
	};

	AddFlags(L, FIF_BMP, bmp_flags);

	FlagEntry exr_flags[] = {
		FLAG_EXR(FLOAT), FLAG_EXR(NONE), FLAG_EXR(ZIP), FLAG_EXR(PIZ), FLAG_EXR(PXR24), FLAG_EXR(B44), FLAG_EXR(LC)
	};

	AddFlags(L, FIF_EXR, exr_flags);

	FlagEntry gif_flags[] = {
		FLAG_GIF(LOAD256), FLAG_GIF(PLAYBACK)
	};

	AddFlags(L, FIF_GIF, gif_flags);

	FlagEntry ico_flags[] = {
		FLAG_ICO(MAKEALPHA)
	};

	AddFlags(L, FIF_ICO, ico_flags);

	FlagEntry jpeg_flags[] = {
		FLAG_JPEG(FAST), FLAG_JPEG(ACCURATE), FLAG_JPEG(CMYK), FLAG_JPEG(EXIFROTATE), FLAG_JPEG(GREYSCALE),
		FLAG_JPEG(QUALITYSUPERB), FLAG_JPEG(QUALITYGOOD), FLAG_JPEG(QUALITYNORMAL), FLAG_JPEG(QUALITYAVERAGE), FLAG_JPEG(QUALITYBAD),
		FLAG_JPEG(PROGRESSIVE), FLAG_JPEG(SUBSAMPLING_411), FLAG_JPEG(SUBSAMPLING_420), FLAG_JPEG(SUBSAMPLING_422), FLAG_JPEG(SUBSAMPLING_444),
		FLAG_JPEG(OPTIMIZE), FLAG_JPEG(BASELINE)
	};

	AddFlags(L, FIF_JPEG, jpeg_flags);

	FlagEntry pcd_flags[] = {
		FLAG_PCD(BASE), FLAG_PCD(BASEDIV4), FLAG_PCD(BASEDIV16)
	};

	AddFlags(L, FIF_PCD, pcd_flags);

	FlagEntry png_flags[] = {
		FLAG_PNG(IGNOREGAMMA),
		FLAG_PNG(Z_BEST_SPEED), FLAG_PNG(Z_DEFAULT_COMPRESSION), FLAG_PNG(Z_BEST_COMPRESSION), FLAG_PNG(Z_NO_COMPRESSION), FLAG_PNG(INTERLACED)
	};

	AddFlags(L, FIF_PNG, png_flags);

	FlagEntry pnm_flags[] = {
		FLAG_PNM(SAVE_RAW), FLAG_PNM(SAVE_ASCII)
	};

	AddFlags(L, FIF_PBM, pnm_flags);
	AddFlags(L, FIF_PBMRAW, pnm_flags);
	AddFlags(L, FIF_PGM, pnm_flags);
	AddFlags(L, FIF_PGMRAW, pnm_flags);
	AddFlags(L, FIF_PPM, pnm_flags);
	AddFlags(L, FIF_PPMRAW, pnm_flags);

	FlagEntry psd_flags[] = {
		FLAG_PSD(CMYK), FLAG_PSD(LAB)
	};

	AddFlags(L, FIF_PSD, psd_flags);

	FlagEntry raw_flags[] = {
		FLAG_RAW(PREVIEW), FLAG_RAW(DISPLAY), FLAG_RAW(HALFSIZE), FLAG_RAW(UNPROCESSED)
	};

	AddFlags(L, FIF_RAW, raw_flags);

	FlagEntry targa_flags[] = {
		FLAG_TARGA(LOAD_RGB888), FLAG_TARGA(SAVE_RLE)
	};

	AddFlags(L, FIF_TARGA, targa_flags);

	FlagEntry tiff_flags[] = {
		FLAG_TIFF(CMYK),
		FLAG_TIFF(PACKBITS), FLAG_TIFF(DEFLATE), FLAG_TIFF(ADOBE_DEFLATE), FLAG_TIFF(NONE),
		FLAG_TIFF(CCITTFAX3), FLAG_TIFF(CCITTFAX4),
		FLAG_TIFF(LZW), FLAG_TIFF(JPEG), FLAG_TIFF(LOGLUV)
	};

	AddFlags(L, FIF_TIFF, tiff_flags);

	FlagEntry webp_flags[] = {
		FLAG_WEBP(LOSSLESS)
	};

	AddFlags(L, FIF_WEBP, webp_flags);

	FlagEntry jxr_flags[] = {
		FLAG_JXR(LOSSLESS), FLAG_JXR(PROGRESSIVE)
	};

	AddFlags(L, FIF_JXR, jxr_flags);

	return 0;
}

#undef FLAG_BMP
#undef FLAG_EXR
#undef FLAG_GIF
#undef FLAG_ICO
#undef FLAG_JPEG
#undef FLAG_PCD
#undef FLAG_PNG
#undef FLAG_PNM
#undef FLAG_PSD
#undef FLAG_RAW
#undef FLAG_TARGA
#undef FLAG_TIFF
#undef FLAG_WEBP
#undef FLAG_JXR