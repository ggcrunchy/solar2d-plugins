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
#include "CoronaLua.h"
#include "utils/LuaEx.h"

PathXS::Directories * GetPathData (lua_State * L)
{
	lua_pushvalue(L, lua_upvalueindex(1));	// ..., dirs

	PathXS::Directories * dirs = LuaXS::UD<PathXS::Directories>(L, -1);

	lua_pop(L, 1);	// ...

	return dirs;
}

template<int N> static int CPOT (void)
{
	if (N > 0) return 1 + CPOT<N / 2>();

	else return 0;
}

CORONA_EXPORT int luaopen_plugin_impack (lua_State * L)
{
    lua_newtable(L);    // impack

	PathXS::Directories::Instantiate(L);	// impack, pd

	luaL_Reg libs[] = {
		{ "grayscale", luaopen_grayscale },
		{ "image", luaopen_image },
		{ "ops", luaopen_ops },
		{ "write", luaopen_write },
		{ nullptr, nullptr }
	};

	LuaXS::LoadClosureLibs(L, libs, 1);	// impack

	return 1;
}
