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

#define PATTERN_MNAME "blend2d.pattern"

BLPatternCore * GetPattern (lua_State * L, int arg)
{
	return Get<BLPatternCore>(L, arg, PATTERN_MNAME);
}

bool IsPattern (lua_State * L, int arg)
{
	return Is<BLPatternCore>(L, arg, PATTERN_MNAME);
}

static int NewPattern (lua_State * L)
{
	BLImageCore * texture = GetImage(L, 1);
	BLPatternCore * pattern = New<BLPatternCore>(L);// texture, pattern

	blPatternInitAs(pattern, texture, nullptr, BL_EXTEND_MODE_REPEAT, nullptr);

	if (luaL_newmetatable(L, PATTERN_MNAME)) // texture, pattern, mt
	{
		luaL_Reg pattern_funcs[] = {
			{
				BLEND2D_DESTROY(Pattern)
			}, {
				BLEND2D_GC(Pattern)
			}, {
				"__index", Index
			}, {

			},
			{ nullptr, nullptr }
		};

		luaL_register(L, NULL, pattern_funcs);
	}

	lua_setmetatable(L, -2);// texture, pattern

	return 1;
}

int add_pattern (lua_State * L)
{
	lua_newtable(L);// t
	lua_pushcfunction(L, NewPattern);	// t, NewPattern
	lua_setfield(L, -2, "New");	// t = { New = NewPattern }

	return 1;
}