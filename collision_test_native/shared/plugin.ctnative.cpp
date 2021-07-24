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

//
//
//

bool IsType (lua_State * L, int arg, const char * name)
{
	if (!lua_getmetatable(L, arg)) return false; // ..., object, ...[, mt]

	luaL_getmetatable(L, name);	// ..., object, ..., mt, type_mt

	bool matches = lua_equal(L, -2, -1);

	lua_pop(L, 2);	// ..., object, ...

	return matches;
}

//
//
//

CORONA_EXPORT int luaopen_plugin_ctnative (lua_State* L)
{
	lua_newtable(L); // ctnative

	luaL_Reg funcs[] = {
		{
			"AABox2", NewAABox2
		}, {
			"AABox3", NewAABox3
		}, {
			"Plane", NewPlane
		}, {
			"Vector2", NewVec2
		}, {
			"Vector3", NewVec3
		},
		{ nullptr, nullptr }
	};

	luaL_register(L, nullptr, funcs);

	return 1;
}
