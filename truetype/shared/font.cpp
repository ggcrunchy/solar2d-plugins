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

#include "truetype.h"
#include "ByteReader.h"
#include "utils/Blob.h"
#include "utils/Byte.h"
#include "utils/LuaEx.h"
#include "stb_truetype.h"
#include <utility>
#include <vector>

//
struct FontInfo {
	stbtt_fontinfo mInfo;

	int GlyphIndex (lua_State * L, int arg)
	{
		int index = luaL_checkint(L, arg) - 1;

		luaL_argcheck(L, index >= 0 && index < mInfo.numGlyphs, arg, "Invalid glyph index");

		return index;
	}
};

//
static FontInfo * GetFont (lua_State * L, int arg = 1)
{
	FontInfo * font = arg == 1 ? LuaXS::UD<FontInfo>(L, 1) : LuaXS::CheckUD<FontInfo>(L, arg, "truetype.fontinfo");

	font->mInfo.userdata = L;

	return font;
}

stbtt_fontinfo * GetFontInfo (lua_State * L, int arg)
{
	return &GetFont(L, arg)->mInfo;
}

static float Scale (lua_State * L, int arg)
{
	return LuaXS::Float(L, arg);// TODO: checking
}

static float Shift (lua_State * L, int arg)
{
	return LuaXS::Float(L, arg);	// TODO: checking
}

int Codepoint (lua_State * L, int arg)
{
	return luaL_checkint(L, arg);	// TODO: checking
}

static int Dim (lua_State * L, int arg)
{
	int dim = luaL_checkint(L, arg);

	luaL_argcheck(L, dim >= 0, arg, "Invalid dimension");

	return dim;
}

struct VertexCount {
	stbtt_vertex * mEntry;
	int * mCount, mSize;

	VertexCount (lua_State * L, stbtt_vertex * verts, int pos = -1)
	{
		mSize = static_cast<int>(LuaXS::ArrayN<stbtt_vertex>(L, pos));
		mEntry = verts + mSize - 1;
		mCount = reinterpret_cast<int *>(&mEntry->x); // n.b. assumes sane layout
	}

	VertexCount (lua_State * L, int pos = -1) : VertexCount{L, LuaXS::UD<stbtt_vertex>(L, pos), pos}
	{
	}

	int Size (void) const
	{
		return mEntry->type == 0 ? *mCount : mSize;
	}
};

static int NewShape (lua_State * L, stbtt_vertex * vertices, int n)
{
	Free(vertices, L, true); // ..., verts

	lua_newtable(L);// ..., verts, t
	lua_setfenv(L, -2);	// ..., verts

	VertexCount vc{L, vertices};

	if (n < vc.mSize) // array larger than necessary, so repurpose last slot as a count
	{
		vc.mEntry->type = 0;
		*vc.mCount = n;
	}

	LuaXS::AttachMethods(L, "truetype.ttshape", [](lua_State * L)
	{
		luaL_Reg vert_funcs[] = {
			"GetVertex", [](lua_State * L)
			{
				int vi = luaL_checkint(L, 2) - 1, n = VertexCount{L, 1}.Size();

				luaL_argcheck(L, vi >= 0 && vi < n, 2, "Invalid vertex index");

				stbtt_vertex * vert = LuaXS::UD<stbtt_vertex>(L, 1) + vi;

				switch (vert->type)
				{
				case STBTT_vmove:
				case STBTT_vline:
					return LuaXS::PushMultipleArgsAndReturn(L, vert->type == STBTT_vmove ? "move_to" : "line_to", vert->x, vert->y);// verts, index, "move_to" / "line_to", x, y
				case STBTT_vcurve:
					return LuaXS::PushMultipleArgsAndReturn(L, "curve_to", vert->x, vert->y, vert->cx, vert->cy);	// verts, index, "curve_to", x, y, cx, cy
				case STBTT_vcubic:
					return LuaXS::PushMultipleArgsAndReturn(L, "cubic_to", vert->x, vert->y, vert->cx, vert->cy, vert->cx1, vert->cy1);	// verts, index, "curve_to", x, y, cx, cy, cx1, cy1
				default:
					return luaL_error(L, "Unknown vertex type", vert->type);
				}
			}, {
				"__len", [](lua_State * L)
				{
					lua_settop(L, 1);	// verts
					
					return LuaXS::PushArgAndReturn(L, VertexCount{L}.Size());
				}
			}, {
				"Rasterize", [](lua_State * L)
				{
					lua_settop(L, 10);	// verts, w, h, xscale, yscale, xshift, yshift, xoff, yoff, opts

					stbtt__bitmap result = { 0 };

					result.w = Dim(L, 2);
					result.h = Dim(L, 3);
					int invert = 1, x = 0, y = 0, n = VertexCount{L, 1}.Size();
					bool bAsUserdata = false;
					float flatness = .35f;

					LuaXS::Options{L, 10}	.Add("flatness", flatness)
											.Add("invert", invert)
											.Add("as_userdata", bAsUserdata)
											.Add("stride", result.stride);

					BlobXS::State blob{L, 10, "blob"};

					result.pixels = blob.PointToData(L, x, y, result.w, result.h, result.stride);	// verts, w, h, xscale, yscale, xshift, yshift, xoff, yoff, opts, pixels

					stbtt_Rasterize(&result, flatness, LuaXS::UD<stbtt_vertex>(L, 1), n, Scale(L, 2), Scale(L, 3), Shift(L, 4), Shift(L, 5), luaL_checkint(L, 6), luaL_checkint(L, 7), invert, L);
	
					return blob.PushData(L, result.pixels, TRUETYPE_BYTES, bAsUserdata);// verts, w, h, xscale, yscale, xshift, yshift, xoff, yoff, opts, pixels[, str]
				}
			},
			{ nullptr, nullptr }
		};

		luaL_register(L, nullptr, vert_funcs);
	});

	return 1;
}

//
template<typename T> typename std::conditional<std::is_same<T, FontInfo *>::value, const stbtt_fontinfo *, T>::type PassThrough (T arg) { return arg; }

template<> inline const stbtt_fontinfo * PassThrough<FontInfo *> (FontInfo * arg) { return &arg->mInfo; }

template<typename F, typename A1, typename ... Args> auto FontFunc (F func, A1 && arg1, Args && ... args) -> decltype(func(PassThrough(std::forward<A1>(arg1)), args...))
{
	return func(PassThrough(std::forward<A1>(arg1)), args...);
}

template<typename F, typename ... Args> int FontBox (lua_State * L, F func, Args && ... args)
{
	int i1, i2, i3, i4;

	FontFunc(func, std::forward<Args>(args)..., &i1, &i2, &i3, &i4);

	return LuaXS::PushMultipleArgsAndReturn(L, i1, i2, i3, i4);
}

template<typename F, typename ... Args> int FontBoxCond (lua_State * L, F func, Args && ... args)
{
	int i1, i2, i3, i4, ok = FontFunc(func, std::forward<Args>(args)..., &i1, &i2, &i3, &i4);

	lua_pushboolean(L, ok);

	return 1 + (ok ? LuaXS::PushMultipleArgsAndReturn(L, i1, i2, i3, i4) : 0);
}

template<typename F, typename ... Args> int FontMake (lua_State * L, F func, Args && ... args)
{
	lua_settop(L, sizeof...(args) + 7); // font, blob, w, h, xscale, yscale, ..., opts

	FontInfo * font = GetFont(L); // font, blob, w, h, xscale, yscale, ..., opts
	int x = 0, y = 0, stride = 0, w = Dim(L, 3), h = Dim(L, 4);

	BlobXS::State blob{L, 2};

	LuaXS::Options{L, -1}	.Add("stride", stride)
							.Add("x", x)
							.Add("y", y);

	unsigned char * out = blob.PointToDataIfBound(L, x, y, w, h, stride);
							
	if (out) FontFunc(func, font, out, w, h, stride, Scale(L, 4), Scale(L, 5), std::forward<Args>(args)...);

	return LuaXS::PushArgAndReturn(L, out != nullptr); // font, blob, w, h, xscale, yscale, ..., opts, ok
}

template<typename F, typename ... Args> int FontPush (lua_State * L, F func, Args && ... args)
{
	lua_settop(L, sizeof...(args) + 2); // font, ..., how
	lua_pushliteral(L, "as_bytes"); // font, ..., how, "as_bytes"

	FontInfo * font = GetFont(L); // font, ..., how, "as_bytes"

	int i1, i2, i3, i4;
	
	unsigned char * out = FontFunc(func, font, Scale(L, 2), Scale(L, 3), std::forward<Args>(args)..., &i1, &i2, &i3, &i4);

	if (!out) return LuaXS::PushArgAndReturn(L, LuaXS::Nil{}); // font, ..., how, "as_bytes", nil

	Push(L, out, lua_equal(L, -3, -2) != 0); // font, ..., how, "as_bytes", bitmap

	return 1 + LuaXS::PushMultipleArgsAndReturn(L, i1, i2, i3, i4); // font, ..., how, "as_bytes", bitmap, w, h, xoff, yoff
}

template<typename F, typename ... Args> int FontPush1 (lua_State * L, F func, Args && ... args) // one-scale variant
{
	lua_settop(L, sizeof...(args) + 2); // font, ..., how
	lua_pushliteral(L, "as_bytes"); // font, ..., how, "as_bytes"

	FontInfo * font = GetFont(L); // font, ..., how, "as_bytes"

	int i1, i2, i3, i4;
	
	unsigned char * out = FontFunc(func, font, Scale(L, 2), std::forward<Args>(args)..., &i1, &i2, &i3, &i4);

	if (!out) return LuaXS::PushArgAndReturn(L, LuaXS::Nil{}); // font, ..., how, "as_bytes", nil

	Push(L, out, lua_equal(L, -3, -2) != 0); // font, ..., how, "as_bytes", bitmap

	return 1 + LuaXS::PushMultipleArgsAndReturn(L, i1, i2, i3, i4); // font, ..., how, "as_bytes", bitmap, w, h, xoff, yoff
}

//
int NewFont (lua_State * L)
{
	ByteReader bytes{L, 1};

	if (!bytes.mBytes) return LuaXS::PushArgAndReturn(L, LuaXS::Nil{});	// bytes[, offset], nil

	int offset = luaL_optint(L, 2, 0);
	size_t extra = lua_isstring(L, 1) ? 0U : bytes.mCount;
	FontInfo * font = LuaXS::NewSizeTyped<FontInfo>(L, sizeof(stbtt_fontinfo) + extra);	// bytes[, offset], font
	const unsigned char * data;

	if (extra)
	{
		memcpy(&font[1], bytes.mBytes, bytes.mCount);

		data = reinterpret_cast<const unsigned char *>(&font[1]);
	}

	else
	{
		lua_createtable(L, 0, 1); // bytes[, offset], font, env
		lua_pushvalue(L, 1);// bytes[, offset], font, env, bytes
		lua_setfield(L, -2, "data"); // bytes[, offset], font, env = { data = bytes }
		lua_setfenv(L, -2); // bytes[, offset], font; font.env = env

		data = reinterpret_cast<const unsigned char *>(lua_tostring(L, 1));
	}

	if (!stbtt_InitFont(&font->mInfo, data, offset)) return LuaXS::PushArgAndReturn(L, LuaXS::Nil{}); // bytes[, offset], font, nil

	LuaXS::AttachMethods(L, "truetype.fontinfo", [](lua_State * L)
	{
		luaL_reg font_methods[] = {
			{
				"FindGlyphIndex", [](lua_State * L)
				{
					return LuaXS::PushArgAndReturn(L, FontFunc(stbtt_FindGlyphIndex, GetFont(L), Codepoint(L, 2)) + 1);	// font, codepoint, index
				}
			}, {
				"GetCodepointBitmap", [](lua_State * L)
				{
					return FontPush(L, stbtt_GetCodepointBitmap, Codepoint(L, 4));	// font, xscale, yscale, codepoint, memory, out[, str]
				}
			}, {
				"GetCodepointBitmapBox", [](lua_State * L)
				{
					return FontBox(L, stbtt_GetCodepointBitmapBox, GetFont(L, 1), Codepoint(L, 2), Scale(L, 3), Scale(L, 4));	// font, codepoint, xscale, yscale, x0, y0, x1, y1
				}
			}, {
				"GetCodepointBitmapBoxSubpixel", [](lua_State * L)
				{
					return FontBox(L, stbtt_GetCodepointBitmapBoxSubpixel, GetFont(L, 1), Codepoint(L, 2), Scale(L, 3), Scale(L, 4), Shift(L, 5), Shift(L, 6));// font, codepoint, xscale, yscale, xshift, yshift, x0, y0, x1, y1
				}
			}, {
				"GetCodepointBitmapSubpixel", [](lua_State * L)
				{
					return FontPush(L, stbtt_GetCodepointBitmapSubpixel, Shift(L, 4), Shift(L, 5), Codepoint(L, 6));// font, xscale, yscale, xshift, yshift, codepoint, opts, memory, out[, str][, w, h, xoff, yoff]
				}
			}, {
				"GetCodepointBox", [](lua_State * L)
				{
					return FontBoxCond(L, stbtt_GetCodepointBox, GetFont(L), Codepoint(L, 2));	// font, codepoint, ok[, x0, y0, x1, y1]
				}
			}, {
				"GetCodepointHMetrics", [](lua_State * L)
				{
					int advance_width, left_side_bearing;

					FontFunc(stbtt_GetCodepointHMetrics, GetFont(L), luaL_checkint(L, 2), &advance_width, &left_side_bearing);

					return LuaXS::PushMultipleArgsAndReturn(L, advance_width, left_side_bearing);	// font, codepoint, advance_width, left_side_bearing
				}
			}, {
				"GetCodepointKernAdvance", [](lua_State * L)
				{
					return LuaXS::PushArgAndReturn(L, FontFunc(stbtt_GetCodepointKernAdvance, GetFont(L), luaL_checkint(L, 2), luaL_checkint(L, 3)));	// font, ch1, ch2, advance
				}
			}, {
				"GetCodepointSDF", [](lua_State * L)
				{
					return FontPush1(L, stbtt_GetCodepointSDF, Codepoint(L, 3), luaL_checkint(L, 4), (unsigned char)luaL_checkint(L, 5), LuaXS::Float(L, 6)); // font, scale, codepoint, padding, onedge_value, pixel_dist_scale, memory, out[, str]
				}
			}, {
				"GetCodepointShape", [](lua_State * L)
				{
					stbtt_vertex * vertices;
					int n = FontFunc(stbtt_GetCodepointShape, GetFont(L), Codepoint(L, 2), &vertices);

					if (!n) return LuaXS::PushArgAndReturn(L, LuaXS::Nil{});// font, codepoint, nil

					return NewShape(L, vertices, n);	// font, codepoint, shape
				}
			}, {
				"GetCodepointSVG", [](lua_State * L)
				{
					const char *svg;
					int n = FontFunc(stbtt_GetCodepointSVG, GetFont(L), Codepoint(L, 2), &svg);

					if (!n) return LuaXS::PushArgAndReturn(L, LuaXS::Nil{}); // font, codepoint, nil

					lua_pushlstring(L, svg, n); // font, codepoint, svg

					return 1;
				}
			}, {
				"GetFontBoundingBox", [](lua_State * L)
				{
					return FontBox(L, stbtt_GetFontBoundingBox, GetFont(L));// font, x0, y0, x1, y1
				}
			}, {
				"GetFontNameString", [](lua_State * L)
				{
					luaL_checktype(L, 2, LUA_TTABLE);
					lua_getfield(L, 2, "platform"); // font, params, platform
					lua_getfield(L, 2, "encoding"); // font, params, platform, encoding
					lua_getfield(L, 2, "language"); // font, params, platform, encoding, language
					lua_getfield(L, 2, "nameID"); // font, params, platform, encoding, language, name_id

					const char * platform_names[] = { "UNICODE", "MAC", "ISO", "MICROSOFT", nullptr };
					int platforms[] = { STBTT_PLATFORM_ID_UNICODE, STBTT_PLATFORM_ID_MAC, STBTT_PLATFORM_ID_ISO, STBTT_PLATFORM_ID_MICROSOFT };
					int pid = platforms[luaL_checkoption(L, -4, nullptr, platform_names)], eid = 0, lid = 0, nid = luaL_checkint(L, -1);

					switch (pid)
					{
					case STBTT_PLATFORM_ID_UNICODE:
						{
							const char * encoding_names[] = {
								"UNICODE_1_0", "UNICODE_1_1",
								"ISO_10646",
								"UNICODE_2_0_BMP", "UNICODE_2_0_FULL",
								nullptr
							};
							int encodings[] = {
								STBTT_UNICODE_EID_UNICODE_1_0, STBTT_UNICODE_EID_UNICODE_1_1,
								STBTT_UNICODE_EID_ISO_10646,
								STBTT_UNICODE_EID_UNICODE_2_0_BMP, STBTT_UNICODE_EID_UNICODE_2_0_FULL
							};

							eid = encodings[luaL_checkoption(L, -3, nullptr, encoding_names)];
						}
						break;
					case STBTT_PLATFORM_ID_MAC:
						{
							const char * encoding_names[] = {
								"ROMAN", "ARABIC", "JAPANESE", "HEBREW",
								"CHINESE_TRAD", "GREEK", "KOREAN", "RUSSIAN",
								nullptr
							};
							int encodings[] = {
								STBTT_MAC_EID_ROMAN, STBTT_MAC_EID_ARABIC, STBTT_MAC_EID_JAPANESE, STBTT_MAC_EID_HEBREW,
								STBTT_MAC_EID_CHINESE_TRAD, STBTT_MAC_EID_GREEK, STBTT_MAC_EID_KOREAN, STBTT_MAC_EID_RUSSIAN
							};

							eid = encodings[luaL_checkoption(L, -3, nullptr, encoding_names)];

							const char * language_names[] = {
								"ENGLISH", "JAPANESE", "ARABIC", "KOREAN",
								"DUTCH", "RUSSIAN", "FRENCH", "SPANISH",
								"GERMAN", "SWEDISH", "HEBREW", "SIMPLIFIED",
								"ITALIAN", "CHINESE_TRAD",
								nullptr
							};
							int languages[] = {
								STBTT_MAC_LANG_ENGLISH, STBTT_MAC_LANG_JAPANESE, STBTT_MAC_LANG_ARABIC, STBTT_MAC_LANG_KOREAN,
								STBTT_MAC_LANG_DUTCH, STBTT_MAC_LANG_RUSSIAN, STBTT_MAC_LANG_FRENCH, STBTT_MAC_LANG_SPANISH,
								STBTT_MAC_LANG_GERMAN, STBTT_MAC_LANG_SWEDISH, STBTT_MAC_LANG_HEBREW, STBTT_MAC_LANG_CHINESE_SIMPLIFIED,
								STBTT_MAC_LANG_ITALIAN, STBTT_MAC_LANG_CHINESE_TRAD
							};

							lid = languages[luaL_checkoption(L, -3, nullptr, language_names)];
						}
						break;
					case STBTT_PLATFORM_ID_MICROSOFT:
						{
							const char * encoding_names[] = {
								"SYMBOL", "BMP", "SHIFTJIS", "UNICODE_FULL",
								nullptr
							};
							int encodings[] = {
								STBTT_MS_EID_SYMBOL, STBTT_MS_EID_UNICODE_BMP, STBTT_MS_EID_SHIFTJIS, STBTT_MS_EID_UNICODE_FULL
							};

							eid = encodings[luaL_checkoption(L, -3, nullptr, encoding_names)];

							const char * language_names[] = {
								"ENGLISH", "ITALIAN", "CHINESE", "JAPANESE",
								"DUTCH", "KOREAN", "FRENCH", "RUSSIAN",
								"GERMAN", "SPANISH", "HEBREW", "SWEDISH",
								nullptr
							};
							int languages[] = {
								STBTT_MS_LANG_ENGLISH, STBTT_MS_LANG_ITALIAN, STBTT_MS_LANG_CHINESE, STBTT_MS_LANG_JAPANESE,
								STBTT_MS_LANG_DUTCH, STBTT_MS_LANG_KOREAN, STBTT_MS_LANG_FRENCH, STBTT_MS_LANG_RUSSIAN,
								STBTT_MS_LANG_GERMAN, STBTT_MS_LANG_SPANISH, STBTT_MS_LANG_HEBREW, STBTT_MS_LANG_SWEDISH
							};

							lid = languages[luaL_checkoption(L, -3, nullptr, language_names)];
						}
						break;
					default:
						break;
					}

					int length;
					const char * str = FontFunc(stbtt_GetFontNameString, GetFont(L), &length, pid, eid, lid, nid);

					if (!str) return LuaXS::PushArgAndReturn(L, LuaXS::Nil{}); // font, params, platform, encoding, language, name_id, nil

					lua_pushlstring(L, str, length); // font, params, platform, encoding, language, name_id, str

					return 1;
				}
			}, {
				"GetFontVMetrics", [](lua_State * L)
				{
					int ascent, descent, line_gap;

					FontFunc(stbtt_GetFontVMetrics, GetFont(L), &ascent, &descent, &line_gap);

					return LuaXS::PushMultipleArgsAndReturn(L, ascent, descent, line_gap);	// font, ascent, descent, line_gap
				}
			}, {
				"GetFontVMetricsOS2", [](lua_State * L)
				{
					int ascent, descent, line_gap;

					FontFunc(stbtt_GetFontVMetricsOS2, GetFont(L), &ascent, &descent, &line_gap);

					return LuaXS::PushMultipleArgsAndReturn(L, ascent, descent, line_gap);	// font, ascent, descent, line_gap
				}
			}, {
				"GetGlyphBitmap", [](lua_State * L)
				{
					FontInfo * font = GetFont(L);

					return FontPush(L, stbtt_GetGlyphBitmap, font->GlyphIndex(L, 4));	// font, xscale, yscale, gi, opts, memory, out[, str][, w, h, xoff, yoff]
				}
			}, {
				"GetGlyphBitmapBox", [](lua_State * L)
				{
					FontInfo * font = GetFont(L);

					return FontBox(L, stbtt_GetGlyphBitmapBox, font, font->GlyphIndex(L, 2), Scale(L, 3), Scale(L, 4));	// font, gi, xscale, yscale, x0, y0, x1, y1
				}
			}, {
				"GetGlyphBitmapBoxSubpixel", [](lua_State * L)
				{
					FontInfo * font = GetFont(L);

					return FontBox(L, stbtt_GetGlyphBitmapBoxSubpixel, font, font->GlyphIndex(L, 2), Scale(L, 3), Scale(L, 4), Shift(L, 5), Shift(L, 6));	// font, gi, xscale, yscale, xshift, yshift, x0, y0, x1, y1
				}
			}, {
				"GetGlyphBitmapSubpixel", [](lua_State * L)
				{
					FontInfo * font = GetFont(L);

					return FontPush(L, stbtt_GetGlyphBitmapSubpixel, Shift(L, 4), Shift(L, 5), font->GlyphIndex(L, 6));	// font, xscale, yscale, xshift, yshift, gi, opts, memory, out[, str][, w, h, xoff, yoff]
				}
			}, {
				"GetGlyphBox", [](lua_State * L)
				{
					FontInfo * font = GetFont(L);

					return FontBoxCond(L, stbtt_GetGlyphBox, font, font->GlyphIndex(L, 2));	// font, gi, ok[, x0, y0, x1, y1]
				}
			}, {
				"GetGlyphHMetrics", [](lua_State * L)
				{
					FontInfo * font = GetFont(L);
					int advance_width, left_side_bearing;

					FontFunc(stbtt_GetGlyphHMetrics, font, font->GlyphIndex(L, 2), &advance_width, &left_side_bearing);

					return LuaXS::PushMultipleArgsAndReturn(L, advance_width, left_side_bearing);	// font, gi, advance_width, left_side_bearing
				}
			}, {
				"GetGlyphKernAdvance", [](lua_State * L)
				{
					FontInfo * font = GetFont(L);

					return LuaXS::PushArgAndReturn(L, FontFunc(stbtt_GetGlyphKernAdvance, font, font->GlyphIndex(L, 2), font->GlyphIndex(L, 3)));	// font, g1, g2, advance
				}
			}, {
				"GetGlyphSDF", [](lua_State * L)
				{
					FontInfo * font = GetFont(L);

					return FontPush1(L, stbtt_GetCodepointSDF, font->GlyphIndex(L, 3), luaL_checkint(L, 4), (unsigned char)luaL_checkint(L, 5), LuaXS::Float(L, 6)); // font, scale, gi, padding, onedge_value, pixel_dist_scale, memory, out[, str]
				}
			}, {
				"GetGlyphShape", [](lua_State * L)
				{
					auto font = GetFont(L); // font, glyph
					stbtt_vertex * vertices;
					int n = FontFunc(stbtt_GetGlyphShape, font, font->GlyphIndex(L, 2), &vertices);
						
					if (!n) return LuaXS::PushArgAndReturn(L, LuaXS::Nil{});// font, gi, nil

					return NewShape(L, vertices, n);// font, gi, shape
				}
			}, {
				"GetGlyphSVG", [](lua_State * L)
				{
					FontInfo * font = GetFont(L);
					const char *svg;
					int n = FontFunc(stbtt_GetCodepointSVG, font, font->GlyphIndex(L, 2), &svg);

					if (!n) return LuaXS::PushArgAndReturn(L, LuaXS::Nil{}); // font, gi, nil

					lua_pushlstring(L, svg, n); // font, gi, svg

					return 1;
				}
			}, {
				"GetKerningTable", [](lua_State * L)
				{
					FontInfo * font = GetFont(L);
					int length = !lua_isnoneornil(L, 2) ? luaL_checkint(L, 2) : stbtt_GetKerningTableLength(&font->mInfo);
					std::vector<stbtt_kerningentry> table(length);

					int n = FontFunc(stbtt_GetKerningTable, font, table.data(), length);

					if (!n) return LuaXS::PushArgAndReturn(L, LuaXS::Nil{}); // font, nil

					lua_createtable(L, n, 0); // font, table

					for (int i = 0; i < n; ++i)
					{
						lua_createtable(L, 0, 3); // font, table, kern
						lua_pushinteger(L, table[i].glyph1); // font, table, kern, glyph1
						lua_pushinteger(L, table[i].glyph2); // font, table, kern, glyph1, glyph2
						lua_pushinteger(L, table[i].advance); // font, table, kern, glyph1, glyph2, advance
						lua_setfield(L, -4, "advance"); // font, table, kern = { advance = advance }, glyph1, glyph2
						lua_setfield(L, -3, "glyph2"); // font, table, kern = { advance, glyph2 = glyph2 }, glyph1
						lua_setfield(L, -2, "glyph1"); // font, table, kern = { advance, glyph2, glyph1 = glyph1 }
						lua_rawseti(L, -2, i + 1); // font, table = { ..., kern }
					}

					return 1;
				}
			}, {
				"GetKerningTableLength", [](lua_State * L)
				{
					return LuaXS::PushArgAndReturn(L, FontFunc(stbtt_GetKerningTableLength, GetFont(L))); // font, length
				}
			}, {
				"IsGlyphEmpty", [](lua_State * L)
				{
					FontInfo * font = GetFont(L);

					return LuaXS::PushArgAndReturn(L, FontFunc(stbtt_IsGlyphEmpty, font, font->GlyphIndex(L, 2)));	// font, index, is_empty
				}
			}, {
				"MakeCodepointBitmap", [](lua_State * L)
				{
					return FontMake(L, stbtt_MakeCodepointBitmap, Codepoint(L, 7));	// font, blob, ow, oh, xscale, yscale, codepoint, opts, memory, out[, str]
				}
			}, {
				"MakeCodepointBitmapSubpixel", [](lua_State * L)
				{
					return FontMake(L, stbtt_MakeCodepointBitmapSubpixel, Shift(L, 7), Shift(L, 8), Codepoint(L, 9));	// font, blob, ow, oh, xscale, yscale, xshift, yshift, codepoint, opts, memory, out[, str]
				}
			}, {
				"MakeCodepointBitmapSubpixelPrefilter", [](lua_State * L)
				{
					float subx, suby;

					int result = FontMake(L, stbtt_MakeCodepointBitmapSubpixelPrefilter, Shift(L, 7), Shift(L, 8), luaL_checkint(L, 9), luaL_checkint(L, 10), &subx, &suby, Codepoint(L, 11)); // font, blob, ow, oh, xscale, yscale, xshift, yshift, xoversample, yoversample, codepoint, opts, memory, out[, str]

					lua_pushnumber(L, subx); // font, blob, ow, oh, xscale, yscale, xshift, yshift, xoversample, yoversample, codepoint, opts, memory, out[, str], subx
					lua_pushnumber(L, suby); // font, blob, ow, oh, xscale, yscale, xshift, yshift, xoversample, yoversample, codepoint, opts, memory, out[, str], subx, suby

					return result + 2;
				}
			}, {
				"MakeGlyphBitmap", [](lua_State * L)
				{
					return FontMake(L, stbtt_MakeGlyphBitmap, GetFont(L)->GlyphIndex(L, 7));// font, blob, ow, oh, xscale, yscale, gi, opts, memory, out[, str]
				}
			}, {
				"MakeGlyphBitmapSubpixel", [](lua_State * L)
				{
					return FontMake(L, stbtt_MakeGlyphBitmapSubpixel, Shift(L, 7), Shift(L, 8), GetFont(L)->GlyphIndex(L, 9));// font, blob, ow, oh, xscale, yscale, xshift, yshift, gi, opts, memory, out[, str]
				}
			}, {
				"MakeGlyphBitmapSubpixelPrefilter", [](lua_State * L)
				{
					float subx, suby;

					int result = FontMake(L, stbtt_MakeGlyphBitmapSubpixelPrefilter, Shift(L, 7), Shift(L, 8), luaL_checkint(L, 9), luaL_checkint(L, 10), &subx, &suby,  GetFont(L)->GlyphIndex(L, 11)); // font, blob, ow, oh, xscale, yscale, xshift, yshift, xoversample, yoversample, gi, opts, memory, out[, str]

					lua_pushnumber(L, subx); // font, blob, ow, oh, xscale, yscale, xshift, yshift, xoversample, yoversample, gi, opts, memory, out[, str], subx
					lua_pushnumber(L, suby); // font, blob, ow, oh, xscale, yscale, xshift, yshift, xoversample, yoversample, gi, opts, memory, out[, str], subx, suby

					return result + 2;
				}
			}, {
				"ScaleForMappingEmToPixels", [](lua_State * L)
				{
					return LuaXS::PushArgAndReturn(L, FontFunc(stbtt_ScaleForMappingEmToPixels, GetFont(L), LuaXS::Float(L, 2)));	// font, pixels, scale
				}
			}, {
				"ScaleForPixelHeight", [](lua_State * L)
				{
					return LuaXS::PushArgAndReturn(L, FontFunc(stbtt_ScaleForPixelHeight, GetFont(L), LuaXS::Float(L, 2)));	// font, pixels, scale
				}
			},
			{ nullptr, nullptr }
		};

		luaL_register(L, nullptr, font_methods);
	});

	return 1;
}