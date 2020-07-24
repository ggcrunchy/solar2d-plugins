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

#define FONTFACE_MNAME "blend2d.fontface"

BLFontFaceCore * GetFontFace (lua_State * L, int arg)
{
	return Get<BLFontFaceCore>(L, arg, FONTFACE_MNAME);
}

static int NewFontFace (lua_State * L)
{
	BLFontFaceCore * font_face = New<BLFontFaceCore>(L);// font_face

	blFontFaceInit(font_face);

	if (luaL_newmetatable(L, FONTFACE_MNAME)) // font_face, mt
	{
		luaL_Reg fontface_funcs[] = {
			{
				"createFromFile", [](lua_State * L)
				{
					BLResult result = blFontFaceCreateFromFile(GetFontFace(L), luaL_checkstring(L, 2), 0);

					if (BL_SUCCESS == result)
					{
						lua_pushboolean(L, 1);	// font_face, filename, true

						return 1;
					}

					else
					{
						lua_pushboolean(L, 0);	// font_face, filename, false
						lua_pushinteger(L, result);	// font_face, filename, false, err

						return 2;
					}
				}
			}, {
				BLEND2D_DESTROY(FontFace)
			}, {
				BLEND2D_GC(FontFace)
			}, {
				"__index", Index
			}, {

			},
			{ nullptr, nullptr }
		};

		luaL_register(L, NULL, fontface_funcs);
	}

	lua_setmetatable(L, -2);// font_face

	return 1;
}

int add_fontface (lua_State * L)
{
	lua_newtable(L);// t
	lua_pushcfunction(L, NewFontFace);	// t, NewFontFace
	lua_setfield(L, -2, "New");	// t = { New = NewFontFace }

	return 1;
}