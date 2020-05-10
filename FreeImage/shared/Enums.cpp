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
#include "enums.h"

struct EnumEntry {
	const char * name;
	int code;
};

#define ENUM_FIC(n) { #n, FIC_##n }
#define ENUM_FICC(n) { #n, FICC_##n }
#define ENUM_FID(n) { #n, FID_##n }
#define ENUM_FIDT(n) { #n, FIDT_##n }
#define ENUM_FIF(n) { #n, FIF_##n }
#define ENUM_FILTER(n) { #n, FILTER_##n }
#define ENUM_FIMD(n) { #n, FIMD_##n }
#define ENUM_FIQ(n) { #n, FIQ_##n }
#define ENUM_FIT(n) { #n, FIT_##n }
#define ENUM_FITMO(n) { #n, FITMO_##n }

static int EnumRef[kNumEnums];

template<size_t n> void AddEnums (lua_State * L, int index, EnumEntry (&arr)[n])
{
	lua_rawgeti(L, LUA_REGISTRYINDEX, EnumRef[index]);	// ..., enums

    for (auto entry : arr)
    {
		lua_pushstring(L, entry.name);	// ..., enums, name
		lua_pushinteger(L, entry.code);	// ..., enums, name, code
		lua_pushvalue(L, -1);	// ..., enums, name, code, code
		lua_pushvalue(L, -3);	// ..., enums, name, code, code, name
		lua_rawset(L, -5);	// ..., enums = { ..., code = name }, name, code
		lua_rawset(L, -3);	// ..., enums = { ..., code = name, name = code }
	}

	lua_pop(L, 1);	// ...
}

int FI_LoadEnums (lua_State * L)
{
	for (int i = 0; i < kNumEnums; ++i)
	{
		lua_newtable(L);	// ..., enum_group

		EnumRef[i] = luaL_ref(L, LUA_REGISTRYINDEX);	// ...
	}

	EnumEntry FI_color_channel[] = {
		ENUM_FICC(RGB),
		ENUM_FICC(RED), ENUM_FICC(GREEN), ENUM_FICC(BLUE), ENUM_FICC(ALPHA), ENUM_FICC(BLACK),
		ENUM_FICC(REAL), ENUM_FICC(IMAG), ENUM_FICC(MAG), ENUM_FICC(PHASE)
	};

	AddEnums(L, kColorChannel, FI_color_channel);

	EnumEntry FI_color_type[] = {
		ENUM_FIC(MINISWHITE), ENUM_FIC(MINISBLACK),
		ENUM_FIC(RGB), ENUM_FIC(PALETTE), ENUM_FIC(RGBALPHA), ENUM_FIC(CMYK)
	};

	AddEnums(L, kColorType, FI_color_type);

	EnumEntry FI_dither[] = {
		ENUM_FID(FS),
		ENUM_FID(BAYER4x4), ENUM_FID(BAYER8x8), ENUM_FID(BAYER16x16),
		ENUM_FID(CLUSTER6x6), ENUM_FID(CLUSTER8x8), ENUM_FID(CLUSTER16x16)
	};

	AddEnums(L, kDither, FI_dither);

	EnumEntry FI_filter[] = {
		ENUM_FILTER(BOX), ENUM_FILTER(BICUBIC), ENUM_FILTER(BILINEAR), ENUM_FILTER(BSPLINE),
		ENUM_FILTER(CATMULLROM), ENUM_FILTER(LANCZOS3)
	};

	AddEnums(L, kFilter, FI_filter);

	EnumEntry FI_format[] = {
		ENUM_FIF(UNKNOWN),
		ENUM_FIF(BMP), ENUM_FIF(ICO), ENUM_FIF(JPEG), ENUM_FIF(JNG), ENUM_FIF(KOALA), ENUM_FIF(LBM), ENUM_FIF(IFF),
		ENUM_FIF(MNG), ENUM_FIF(PBM), ENUM_FIF(PBMRAW), ENUM_FIF(PCD), ENUM_FIF(PCX), ENUM_FIF(PGM), ENUM_FIF(PGMRAW),
		ENUM_FIF(PNG), ENUM_FIF(PPM), ENUM_FIF(PPMRAW), ENUM_FIF(RAS), ENUM_FIF(TARGA), ENUM_FIF(TIFF), ENUM_FIF(WBMP),
		ENUM_FIF(PSD), ENUM_FIF(CUT), ENUM_FIF(XBM), ENUM_FIF(XPM), ENUM_FIF(DDS), ENUM_FIF(GIF), ENUM_FIF(HDR),
		ENUM_FIF(FAXG3), ENUM_FIF(SGI), ENUM_FIF(EXR), ENUM_FIF(J2K), ENUM_FIF(JP2), ENUM_FIF(PFM), ENUM_FIF(PICT),
		ENUM_FIF(RAW), ENUM_FIF(WEBP), ENUM_FIF(JXR)
	};

	AddEnums(L, kFormat, FI_format);

	EnumEntry FI_md_model[] = {
		ENUM_FIMD(NODATA), ENUM_FIMD(COMMENTS),
		ENUM_FIMD(EXIF_MAIN), ENUM_FIMD(EXIF_EXIF), ENUM_FIMD(EXIF_GPS), ENUM_FIMD(EXIF_MAKERNOTE), ENUM_FIMD(EXIF_INTEROP),
		ENUM_FIMD(IPTC), ENUM_FIMD(XMP), ENUM_FIMD(GEOTIFF), ENUM_FIMD(ANIMATION), ENUM_FIMD(CUSTOM), ENUM_FIMD(EXIF_RAW)
	};

	AddEnums(L, kMdModel, FI_md_model);

	EnumEntry FI_md_type[] = {
		ENUM_FIDT(NOTYPE), ENUM_FIDT(UNDEFINED), ENUM_FIDT(ASCII),
		ENUM_FIDT(BYTE), ENUM_FIDT(SHORT), ENUM_FIDT(LONG), ENUM_FIDT(RATIONAL),
		ENUM_FIDT(SBYTE), ENUM_FIDT(SSHORT), ENUM_FIDT(SLONG), ENUM_FIDT(SRATIONAL),
		ENUM_FIDT(FLOAT), ENUM_FIDT(DOUBLE),
		ENUM_FIDT(IFD), ENUM_FIDT(PALETTE),
		ENUM_FIDT(LONG8), ENUM_FIDT(SLONG8), ENUM_FIDT(IFD8)
	};

	AddEnums(L, kMdType, FI_md_type);

	EnumEntry FI_quantize[] = { ENUM_FIQ(WUQUANT), ENUM_FIQ(NNQUANT), ENUM_FIQ(LFPQUANT) };

	AddEnums(L, kQuantize, FI_quantize);

	EnumEntry FI_tmo[] = { ENUM_FITMO(DRAGO03), ENUM_FITMO(REINHARD05), ENUM_FITMO(FATTAL02) };

	AddEnums(L, kTMO, FI_tmo);

	EnumEntry FI_type[] = {
		ENUM_FIT(UNKNOWN),
		ENUM_FIT(BITMAP), ENUM_FIT(UINT16), ENUM_FIT(INT16), ENUM_FIT(UINT32), ENUM_FIT(INT32),
		ENUM_FIT(FLOAT), ENUM_FIT(DOUBLE), ENUM_FIT(COMPLEX),
		ENUM_FIT(RGB16), ENUM_FIT(RGBA16), ENUM_FIT(RGBF), ENUM_FIT(RGBAF)
	};

	AddEnums(L, kType, FI_type);

	return 0;
}

#undef ENUM_FIC
#undef ENUM_FICC
#undef ENUM_FID
#undef ENUM_FIDT
#undef ENUM_FIF
#undef ENUM_FILTER
#undef ENUM_FIMD
#undef ENUM_FIQ
#undef ENUM_FIT
#undef ENUM_FITMO

static int AuxGet (lua_State * L, FI_EnumList list, int index)
{
	index = CoronaLuaNormalize(L, index);

	lua_rawgeti(L, LUA_REGISTRYINDEX, EnumRef[list]);	// ..., enums
	lua_pushvalue(L, index);// ..., enums, key
	lua_rawget(L, -2);	// ..., enums, value
	lua_replace(L, index);	// ..., enums
	lua_pop(L, 1);	// ...

	return index;
}

int GetCode (lua_State * L, FI_EnumList list, int name_index)
{
	return luaL_checkint(L, AuxGet(L, list, name_index));
}

const char * GetName (lua_State * L, FI_EnumList list, int code_index)
{
	return luaL_checkstring(L, AuxGet(L, list, code_index));
}

int GetCode_Def (lua_State * L, FI_EnumList list, int name_index, int def)
{
	int index = AuxGet(L, list, name_index);

	if (lua_isnil(L, index))
	{
		lua_pushinteger(L, def);// ..., def
		lua_insert(L, index);	// ...
	}

	return lua_tointeger(L, index);
}

const char * GetName_Def (lua_State * L, FI_EnumList list, int code_index, const char * def)
{
	int index = AuxGet(L, list, code_index);

	if (lua_isnil(L, index))
	{
		lua_pushstring(L, def);	// ..., def
		lua_insert(L, index);	// ...
	}

	return lua_tostring(L, index);
}