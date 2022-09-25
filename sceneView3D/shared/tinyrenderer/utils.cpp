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

#include "utils/LuaEx.h"
#include "tinyrenderer.h"
#include "geometry.h"

//
//
//

namespace tiny {

//
//
//

Vec3f GetVec3 (lua_State * L, int first)
{
	return Vec3f{LuaXS::Float(L, first), LuaXS::Float(L, first + 1), LuaXS::Float(L, first + 2)};
}

//
//
//

void AddConstructor (lua_State * L, const char * name, lua_CFunction func, int nupvalues)
{
    lua_pushcclosure(L, func, nupvalues); // ..., tinyrenderer, func
    lua_setfield(L, -2, name);  // tinyrenderer = { ..., name = func }
}

//
//
//

int FindInParent (lua_State * L)
{
    lua_getfenv(L, 1);  // object / group, items
    lua_getfield(L, -1, "parent");  // object / group, items, parent / nil

    if (lua_isnil(L, -1)) return -1;

    lua_getfenv(L, -1); // object / group, items, parent, pitems
    
    for (size_t i = 1, n = lua_objlen(L, -1); i <= n; ++i)
    {
        lua_rawgeti(L, -1, int(i)); // object / group, items, parent, pitems, elem
        
        bool bFound = lua_equal(L, 1, -1) != 0;
        
        lua_pop(L, 1);  // object / group, items, parent, pitems
        
        if (bFound) return int(i);
    }

    return luaL_error(L, "Not found");
}

//
//
//

}