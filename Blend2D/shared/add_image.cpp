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

BLImageCore * GetImage (lua_State * L, int arg, bool * intact_ptr)
{
	return Get<BLImageCore>(L, arg, "blend2d.image", intact_ptr);
}

uint32_t GetFormat (lua_State * L, int arg)
{
	const char * names[] = { "NONE", "PRGB32", "XRGB32", "A8", nullptr };
	uint32_t formats[] = { BL_FORMAT_NONE, BL_FORMAT_PRGB32, BL_FORMAT_XRGB32, BL_FORMAT_A8 };

	return formats[luaL_checkoption(L, arg, nullptr, names)];
}

static int NewImage (lua_State * L)
{
	int w = lua_tointeger(L, 1), h = lua_tointeger(L, 2);
	uint32_t format = GetFormat(L, 3);

	BLImageCore * image = New<BLImageCore>(L);	// w, h, format, image

	blImageInitAs(image, w, h, format);

	if (luaL_newmetatable(L, "blend2d.image")) // w, h, format, image, mt
	{
		luaL_Reg image_funcs[] = {
			{
				"destroy", [](lua_State * L)
				{
					BLImageCore * image = GetImage(L, 1);

					blImageDestroy(image);
					Destroy<BLImageCore>(L);

					return 1;
				}
			}, {
				"__gc", [](lua_State * L)
				{
					bool intact;

					BLImageCore * image = GetImage(L, 1, &intact);

					if (intact) blImageDestroy(image);

					return 0;
				}
			}, {
				"__index", Index
			}, {
				"writeToFile", [](lua_State * L)
				{
					blImageWriteToFile(GetImage(L), luaL_checkstring(L, 2), GetImageCodec(L, 3));

					return 0;
				}
			},
			{ nullptr, nullptr }
		};

		luaL_register(L, NULL, image_funcs);
	}

	lua_setmetatable(L, -2);// w, h, format, image

	return 1;
}

int add_image (lua_State * L)
{
	lua_newtable(L);// t
	lua_pushcfunction(L, NewImage);	// t, NewImage
	lua_setfield(L, -2, "New");	// t = { New = NewImage }

	return 1;
}