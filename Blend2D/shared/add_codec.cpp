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

BLImageCodecCore * GetImageCodec (lua_State * L, int arg, bool * intact_ptr)
{
	return Get<BLImageCodecCore>(L, arg, "blend2d.codec", intact_ptr);
}

static int NewCodec (lua_State * L)
{
	BLImageCodecCore * codec = New<BLImageCodecCore>(L);// codec

	blImageCodecInit(codec);

	if (luaL_newmetatable(L, "blend2d.codec")) // codec, mt
	{
		luaL_Reg codec_funcs[] = {
			{
				"destroy", [](lua_State * L)
				{
					BLImageCodecCore * codec = GetImageCodec(L, 1);

					blImageCodecDestroy(codec);
					Destroy<BLImageCodecCore>(L);

					return 1;
				}
			}, {
				"findByName", [](lua_State * L)
				{
					blImageCodecFindByName(GetImageCodec(L), luaL_checkstring(L, 2), 0U, nullptr);

					return 0;
				}
			}, {
				"__gc", [](lua_State * L)
				{
					bool intact;

					BLImageCodecCore * codec = GetImageCodec(L, 1, &intact);

					if (intact) blImageCodecDestroy(codec);

					return 0;
				}
			}, {
				"__index", Index
			}, {

			},
			{ nullptr, nullptr }
		};

		luaL_register(L, NULL, codec_funcs);
	}

	lua_setmetatable(L, -2);// codec

	return 1;
}

int add_codec (lua_State * L)
{
	lua_newtable(L);// t
	lua_pushcfunction(L, NewCodec);	// t, NewCodec
	lua_setfield(L, -2, "New");	// t = { New = NewCodec }

	return 1;
}