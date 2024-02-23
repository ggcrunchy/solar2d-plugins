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

#include "utils.h"

//
//
//

// expose a "L" object in globalThis?
    // then through properties would be able to expose objects
        // if primitive or light userdata (> 5.1 also include light C function), tagged value
        // else, an Object with a Lua reference
            // need to see Lua state during use...
            // could be just a ptr -> object map if maintenance easier
        // calls:
            // function or getmetafield with __call
            // else error
            // would decode one by one and add to stack
            // return values...
                // multiple
                // nil? none?
        // passing functions around...
        // methods

CORONA_EXPORT int luaopen_plugin_wasmthing (lua_State* L)
{
    lua_newtable(L);

    //
    //
    //

    luaL_Reg closures[] = {
        {
            "LoadJS", [](lua_State * L)
            {
                luaL_argcheck(L, lua_istable(L, 1), 1, "Non-table params");
                // if Lua, takes state... so that we can do registry, etc.
                // methods, machinery

                return 0;
            }
        }, {
            "LoadJSWasmPair", LoadJSWasmPair
        }, {
            "LoadWASM", [](lua_State * L)
            {
                luaL_argcheck(L, lua_istable(L, 1), 1, "Non-table params");
                // stripped down version of LoadJSWasmPair
                return 0;
            }
        },
        { nullptr, nullptr }
    };

    //
    //
    //

    RegisterWithLoadFile(L, closures);

    //
    //
    //

	return 1;
}
