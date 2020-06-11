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

#include "blend2d.h"
#include "CoronaLua.h"
#include "common.h"
#include "utils.h"

#define FONT_MNAME "blend2d.font"

BLFontCore * GetFont (lua_State * L, int arg, bool * intact_ptr)
{
	return Get<BLFontCore>(L, arg, FONT_MNAME, intact_ptr);
}

static int NewFont (lua_State * L)
{
	BLFontCore * font = New<BLFontCore>(L);// font

	blFontInit(font);

	if (luaL_newmetatable(L, FONT_MNAME)) // font, mt
	{
		luaL_Reg font_funcs[] = {
			{
				"createFromFace", [](lua_State * L)
				{
					blFontCreateFromFace(GetFont(L), GetFontFace(L, 2), (float)luaL_checknumber(L, 3));

					return 0;
				}
			}, {
				"destroy", [](lua_State * L)
				{
					BLFontCore * font = GetFont(L);

					blFontDestroy(font);
					Destroy(font);

					return 1;
				}
			}, {
				"__gc", [](lua_State * L)
				{
					bool intact;

					BLFontCore * font = GetFont(L, 1, &intact);

					if (intact) blFontDestroy(font);

					return 0;
				}
			}, {
				"getTextMetrics", [](lua_State * L)
				{
					BLTextMetrics metrics;

					blFontGetTextMetrics(GetFont(L), GetGlyphBuffer(L, 2), &metrics);

					luaL_argcheck(L, lua_istable(L, 3), 3, "Text metrics expects table");

					lua_createtable(L, 0, 4); // font, glyph_buffer, metrics, bbox
					lua_pushnumber(L, metrics.boundingBox.x0); // font, glyph_buffer, metrics, bbox, x0
					lua_pushnumber(L, metrics.boundingBox.y0); // font, glyph_buffer, metrics, bbox, x0, y0
					lua_pushnumber(L, metrics.boundingBox.x1); // font, glyph_buffer, metrics, bbox, x0, y0, x1
					lua_pushnumber(L, metrics.boundingBox.y1); // font, glyph_buffer, metrics, bbox, x0, y0, x1, y1
					lua_setfield(L, -5, "y1"); // font, glyph_buffer, metrics, bbox = { y1 = y1 }, x0, y0, x1
					lua_setfield(L, -4, "x1"); // font, glyph_buffer, metrics, bbox = { x1 = x1, y1 }, x0, y0
					lua_setfield(L, -3, "y0"); // font, glyph_buffer, metrics, bbox = { y0 = y0, x1, y1 }, x0
					lua_setfield(L, -2, "x0"); // font, glyph_buffer, metrics, bbox = { x0 = x0, y0, x1, y1 }
					lua_setfield(L, -2, "boundingBox"); // font, glyph_buffer, metrics = { boundingBox = bbox }

					return 0;
				}
			}, {
				"__index", Index
			}, {
				"metrics", [](lua_State * L)
				{
					BLFontMetrics metrics;

					blFontGetMetrics(GetFont(L), &metrics);

					lua_createtable(L, 0, 3); // font, metrics
					lua_pushnumber(L, metrics.ascent); // font, metrics, ascent
					lua_pushnumber(L, metrics.descent);// font, metrics, ascent, descent
					lua_pushnumber(L, metrics.lineGap);// font, metrics, ascent, descent, lineGap
					lua_setfield(L, -4, "lineGap"); // font, metrics = { lineGap = lineGap }, ascent, descent
					lua_setfield(L, -3, "descent"); // font, metrics = { descent = descent, lineGap }, ascent
					lua_setfield(L, -2, "ascent");	// font, metrics = { ascent = ascent, descent, lineGap }

					return 1;
				}
			}, {
				"shape", [](lua_State * L)
				{
					blFontShape(GetFont(L), GetGlyphBuffer(L, 2));

					return 0;
				}
			},
			{ nullptr, nullptr }
		};

		luaL_register(L, NULL, font_funcs);
	}

	lua_setmetatable(L, -2);// font

	return 1;
}

int add_font (lua_State * L)
{
	lua_newtable(L);// t
	lua_pushcfunction(L, NewFont);	// t, NewFont
	lua_setfield(L, -2, "New");	// t = { New = NewFont }

	return 1;
}