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
#include "CoronaMemory.h"
#include "blake3.h"

#define BLAKE3_METATABLE_NAME "metatable.blake3"

static blake3_hasher * Get (lua_State * L)
{
    return (blake3_hasher *)luaL_checkudata(L, 1, BLAKE3_METATABLE_NAME);
}

static int AddMetatable (lua_State * L)
{
    if (luaL_newmetatable(L, BLAKE3_METATABLE_NAME)) // ..., hasher, mt
    {
        luaL_Reg methods[] = {
            {
                "finalize", [](lua_State * L)
                {
                    blake3_hasher * blake = Get(L);
                
                    CoronaMemoryAcquireState state;

                    if (!lua_isnoneornil(L, 2) && CoronaMemoryAcquireInterface(L, 2, &state) && state.callbacks->getWriteableBytes)
                    {
                        blake3_hasher_finalize(Get(L), (uint8_t *)CORONA_MEMORY_GET(state, WriteableBytes), CORONA_MEMORY_GET(state, ByteCount));

                        return 0;
                    }
                
                    else
                    {
                        uint8_t out[BLAKE3_OUT_LEN];

                        blake3_hasher_finalize(Get(L), out, BLAKE3_OUT_LEN);

                        lua_pushlstring(L, reinterpret_cast<char *>(out), BLAKE3_OUT_LEN);

                        return 1;
                    }
                }
            }, {
                "finalize_seek", [](lua_State * L)
                {
                    blake3_hasher * blake = Get(L);
                
                    CoronaMemoryAcquireState state;

                    if (!lua_isnoneornil(L, 3) && CoronaMemoryAcquireInterface(L, 3, &state) && state.callbacks->getWriteableBytes)
                    {
                        blake3_hasher_finalize_seek(Get(L), lua_tointeger(L, 2), (uint8_t *)CORONA_MEMORY_GET(state, WriteableBytes), CORONA_MEMORY_GET(state, ByteCount));

                        return 0;
                    }

                    else
                    {
                        uint8_t out[BLAKE3_OUT_LEN];

                        blake3_hasher_finalize_seek(Get(L), lua_tointeger(L, 2), out, BLAKE3_OUT_LEN);

                        lua_pushlstring(L, reinterpret_cast<char *>(out), BLAKE3_OUT_LEN);
                
                        return 1;
                    }
                }
            }, {
                "reset", [](lua_State * L)
                {
                    blake3_hasher_reset(Get(L));
                
                    return 0;
                }
            }, {
                "update", [](lua_State * L)
                {
                    CoronaMemoryAcquireState state;

                    if (CoronaMemoryAcquireInterface(L, 2, &state) && state.callbacks->getReadableBytes)
                    {
                        blake3_hasher_update(Get(L), CORONA_MEMORY_GET(state, ReadableBytes), CORONA_MEMORY_GET(state, ByteCount));
                    }
                
                    return 0;
                }
            },
            { nullptr, nullptr }
        };

        luaL_register(L, nullptr, methods);
        lua_pushvalue(L, -1); // ..., hasher, mt, mt
        lua_setfield(L, -2, "__index"); // ..., hasher, mt; mt.__index = mt
    }

    lua_setmetatable(L, -2); // ..., hasher; hasher.metatable = mt

    return 1;
}

static blake3_hasher * NewHasher (lua_State * L)
{
    return (blake3_hasher *)lua_newuserdata(L, sizeof(blake3_hasher)); // ..., hasher
}

static void LoadModule (lua_State * L)
{
    lua_newtable(L); // blake3

    luaL_Reg funcs[] = {
        {
            "NewHasher", [](lua_State * L)
            {
                blake3_hasher * hasher = NewHasher(L); // hasher

                blake3_hasher_init(hasher);
                
                return AddMetatable(L);
            }
        }, {
            "NewHasher_DeriveKey", [](lua_State * L)
            {
                const char * context = luaL_checkstring(L, 1); // context
                blake3_hasher * hasher = NewHasher(L); // context, hasher
                
                blake3_hasher_init_derive_key(hasher, context);

                return AddMetatable(L);
            }
        }, {
            "NewHasher_DeriveKeyRaw", [](lua_State * L)
            {
                CoronaMemoryAcquireState state;

                if (CoronaMemoryAcquireInterface(L, 1, &state) && state.callbacks->getReadableBytes)
                {
                    blake3_hasher * hasher = NewHasher(L); // context, hasher

                    blake3_hasher_init_derive_key_raw(hasher, CORONA_MEMORY_GET(state, ReadableBytes), CORONA_MEMORY_GET(state, ByteCount));
                
                    return AddMetatable(L);
                }

                lua_pushnil(L); // [key, ]nil

                return 1;
            }
        }, {
            "NewHasher_Keyed", [](lua_State * L)
            {
                CoronaMemoryAcquireState state;

                if (CoronaMemoryAcquireInterface(L, 1, &state) && state.callbacks->getReadableBytes)
                {
                    blake3_hasher * hasher = NewHasher(L); // key, hasher
                    size_t count = CORONA_MEMORY_GET(state, ByteCount);

                    if (count >= BLAKE3_KEY_LEN) blake3_hasher_init_keyed(hasher, (uint8_t*)CORONA_MEMORY_GET(state, ReadableBytes));

                    else
                    {
                        uint8_t key[BLAKE3_KEY_LEN] = {};

                        memcpy(key, CORONA_MEMORY_GET(state, ReadableBytes), count);

                        blake3_hasher_init_keyed(hasher, key);
                    }

                    return AddMetatable(L);
                }

                lua_pushnil(L); // [key, ]nil
                
                return 1;
            }
        },
        { nullptr, nullptr }
    };
    
    luaL_register( L, nullptr, funcs );
}

CORONA_EXPORT int luaopen_plugin_blake3 (lua_State* L)
{
    LoadModule(L); // blake3

    luaL_Reg funcs[] = {
        {
            "Reloader", [](lua_State * L)
            {
                LoadModule(L);

                return 1;
            }
        },
        { nullptr, nullptr }
    };

    luaL_register(L, nullptr, funcs);

	return 1;
}
