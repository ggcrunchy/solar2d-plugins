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
#include "utils/LuaEx.h"
#include "microsat-cpp.h"
#include <vector>

//
//
//

#define MICROSAT_NAME "microSAT"

//
//
//

static MicroSAT * GetSAT (lua_State * L)
{
    return LuaXS::CheckUD<MicroSAT>(L, 1, MICROSAT_NAME);
}

//
//
//

CORONA_EXPORT int luaopen_plugin_microsat (lua_State* L)
{
    lua_newtable(L); // microsat

    luaL_Reg funcs[] = {
        {
            "New", [](lua_State * L)
            {
                try {
                    MicroSAT * sat = LuaXS::NewTyped<MicroSAT>(L, luaL_checkint(L, 1), luaL_optint(L, 2, 1 << 20)); // nvars[, mem_max], sat

                    LuaXS::AttachMethods(L, MICROSAT_NAME, [](lua_State * L) {
                        luaL_Reg funcs[] = {
                            {
                                "add", [](lua_State * L)
                                {
                                    try {
                                        if (lua_istable(L, 2))
                                        {
                                            std::vector<int> vars;

                                            for (size_t i = 1, n = lua_objlen(L, 2); i <= n; ++i, lua_pop(L, 1))
                                            {
                                                lua_rawgeti(L, 2, int(i)); // sat, vars, v

                                                vars.push_back(luaL_checkint(L, -1));
                                            }
                                             
                                            lua_pushboolean(L, GetSAT(L)->add(vars)); // sat, vars, ok
                                        }

                                        else lua_pushboolean(L, GetSAT(L)->add(lua_toboolean(L, 2))); // sat, var, ok
                                    } catch (const char * err) {
                                        return luaL_error(L, err);
                                    }

                                    return 1;
                                }
                            },
                            {
                                "__gc", LuaXS::TypedGC<MicroSAT>
                            },
                            {
                                "query", [](lua_State * L)
                                {
                                    lua_pushboolean(L, GetSAT(L)->query(luaL_checkint(L, 2))); // sat, var, value

                                    return 1;
                                }
                            },
                            {
                                "solve", [](lua_State * L)
                                {
                                    bool keep_clauses = lua_type(L, 2) != LUA_TBOOLEAN || lua_toboolean(L, 2);

                                    lua_pushboolean(L, GetSAT(L)->solve(keep_clauses)); // sat[, keep_clauses], result

                                    return 1;
                                }
                            },
                            { nullptr, nullptr }
                        };
                    });
                } catch (const char * err) {
                    return luaL_error(L, err);
                }

                return 1;
            }
        },
        { nullptr, nullptr }
    };

    luaL_register(L, nullptr, funcs);

	return 1;
}
