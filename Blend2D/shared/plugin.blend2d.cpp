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

#include "CoronaLua.h"
#include "common.h"

CORONA_PUBLIC CORONA_EXPORT int luaopen_plugin_blend2d (lua_State * L) CORONA_PUBLIC_SUFFIX
{
	lua_newtable(L);// blend2d

    luaL_Reg classes[] = {
		{ "codec", add_codec },
		{ "context", add_context },
		{ "font", add_font },
		{ "fontface", add_fontface },
		{ "glyphbuffer", add_glyphbuffer },
		{ "gradient", add_gradient },
		{ "image", add_image },
		{ "path", add_path },
		{ "pattern", add_pattern },
		{ nullptr, nullptr }
	};

	for (int i = 0; classes[i].func; ++i)
	{
		lua_pushcfunction(L, classes[i].func);	// blend2d, func
		lua_call(L, 0, 1);	// blend2d, class
		lua_setfield(L, -2, classes[i].name);	// blend2d = { ..., name = class }
	}

	return 1;
}
