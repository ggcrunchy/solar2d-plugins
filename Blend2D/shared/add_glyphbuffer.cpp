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

#define GLYPHBUFFER_MNAME "blend2d.glyphbuffer"

BLGlyphBufferCore * GetGlyphBuffer (lua_State * L, int arg, bool * intact_ptr)
{
	return Get<BLGlyphBufferCore>(L, arg, GLYPHBUFFER_MNAME, intact_ptr);
}

static int NewGlyphBuffer (lua_State * L)
{
	BLGlyphBufferCore * glyph_buffer = New<BLGlyphBufferCore>(L);// glyph_buffer

	blGlyphBufferInit(glyph_buffer);

	if (luaL_newmetatable(L, GLYPHBUFFER_MNAME)) // glyph_buffer, mt
	{
		luaL_Reg glyphbuffer_funcs[] = {
			{
				"destroy", [](lua_State * L)
				{
					BLGlyphBufferCore * glyph_buffer = GetGlyphBuffer(L);

					blGlyphBufferDestroy(glyph_buffer);
					Destroy(glyph_buffer);

					return 1;
				}
			}, {
				"__gc", [](lua_State * L)
				{
					bool intact;

					BLGlyphBufferCore * glyph_buffer = GetGlyphBuffer(L, 1, &intact);

					if (intact) blGlyphBufferDestroy(glyph_buffer);

					return 0;
				}
			}, {
				"__index", Index
			}, {

			},
			{ nullptr, nullptr }
		};

		luaL_register(L, NULL, glyphbuffer_funcs);
	}

	lua_setmetatable(L, -2);// glyph_buffer

	return 1;
}

int add_glyphbuffer (lua_State * L)
{
	lua_newtable(L);// t
	lua_pushcfunction(L, NewGlyphBuffer);	// t, NewGlyphBuffer
	lua_setfield(L, -2, "New");	// t = { New = NewGlyphBuffer }

	return 1;
}