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

#include "common.h"
#include <atomic>

//
//
//

const void * GetInputMemory (lua_State * L, int arg, int32_t * size)
{
    CoronaMemoryAcquireState state;

    luaL_argcheck(L, !lua_isnoneornil(L, arg) && CoronaMemoryAcquireInterface(L, arg, &state), arg, "Invalid input memory");
    luaL_argcheck(L, state.callbacks->getReadableBytes, arg, "Non-readable bytes");
    
    *size = CORONA_MEMORY_GET(state, ByteCount);

    return CORONA_MEMORY_GET(state, ReadableBytes);
}

//
//
//

Output GetOutputMemory (lua_State * L, int arg)
{
    Output out = {};

    if (!lua_isnoneornil(L, arg))
    {
        CoronaMemoryAcquireState state;

        luaL_argcheck(L, !lua_isnoneornil(L, arg) && CoronaMemoryAcquireInterface(L, arg, &state), arg, "Invalid input memory");
        luaL_argcheck(L, state.callbacks->getWriteableBytes, arg, "Non-writeable bytes");

        out.memory = CORONA_MEMORY_GET(state, WriteableBytes);
        out.size = CORONA_MEMORY_GET(state, ByteCount);
        out.provided = true;
    }

    else
    {
        lua_getfield(L, LUA_REGISTRYINDEX, BLOSC2_SCRATCH); // ..., scratch

        out.memory = lua_touserdata(L, -1);
        out.size = lua_objlen(L, -1);
    }

    return out;
}

//
//
//

int PushOkOrError (lua_State * L, int result)
{
    lua_pushboolean(L, BLOSC2_ERROR_SUCCESS == result); // ..., ok

    if (result != BLOSC2_ERROR_SUCCESS)
    {
        lua_pushstring(L, print_error(result)); // ..., false, error

        return 2;
    }

    return 1;
}

//
//
//

int PushSizeOrError (lua_State * L, int32_t size, const Output & out)
{
    if (size >= 0)
    {
        lua_pushinteger(L, size); // ..., size

        if (size > 0 && !out.provided)
        {
            lua_pushlstring(L, static_cast<char *>(out.memory), size); // ..., size, str

            return 2;
        }

        else return 1;
    }

    else return PushOkOrError(L, size); // ..., false, error
}

//
//
//

#define INT64_METATABLE_NAME "blosc2.int64"

//
//
//

int PushInt64OrError (lua_State * L, int64_t v)
{
    if (v < 0) return PushOkOrError(L, int(v)); // ..., false, error

    else if (v <= (1LL << 52)) lua_pushnumber(L, lua_Number(v)); // ..., v

    else
    {
        LuaXS::NewTyped<int64_t>(L, v); // ..., v

        luaL_newmetatable(L, INT64_METATABLE_NAME); // ..., v, mt
        lua_setmetatable(L, -2); // ..., v; v.metatable = mt
    }

    return 1;
}

//
//
//

int64_t ReadInt64 (lua_State * L, int arg)
{
    if (lua_isnumber(L, arg)) return lua_tointeger(L, arg);
    else return *LuaXS::CheckUD<int64_t>(L, arg, INT64_METATABLE_NAME);
}

//
//
//

#define BLOSC2_WEAK_PAIRING "blosc2.weakpairing"

//
//
//

void WeakKeyPair (lua_State * L, int karg, int varg)
{
    karg = CoronaLuaNormalize(L, karg);
    varg = CoronaLuaNormalize(L, varg);

    lua_getfield(L, LUA_REGISTRYINDEX, BLOSC2_WEAK_PAIRING); // ..., weakpairing
    lua_pushvalue(L, karg); // ..., weakpairing, k
    lua_pushvalue(L, varg); // ..., weakpairing, k, v
    lua_rawset(L, -3); // ..., weakpairing[k] = v
    lua_pop(L, 1); // ...
}

//
//
//

void GetValueFromWeakKey (lua_State * L, int arg)
{
    arg = CoronaLuaNormalize(L, arg);

    lua_getfield(L, LUA_REGISTRYINDEX, BLOSC2_WEAK_PAIRING); // ..., weakpairing
    lua_pushvalue(L, arg); // ..., weakpairing, key
    lua_rawget(L, -2); // ..., weakpairing, value
    lua_remove(L, -2); // ..., value
}

//
//
//

void GetSchunk (lua_State * L, int parg, void * schunk)
{
    if (schunk)
    {
        GetValueFromWeakKey(L, parg); // ..., value?

        luaL_argcheck(L, lua_isuserdata(L, -1), -1, "Bad pairing for schunk");

        if (lua_touserdata(L, -1) != schunk)
        {
            GetContext(L, -1); // ensure type

            lua_pop(L, 1); // ...

            WrapSchunk(L, static_cast<blosc2_schunk *>(schunk)); // ..., schunk
        }
    }
    
    lua_pushnil(L); // ..., nil
}

//
//
//

static void LoadModule (lua_State * L)
{
    lua_newtable(L); // blosc2

    //
    //
    //

    AddCore(L);
    AddContext(L);
    AddArray(L);
    AddCparams(L);
    AddDparams(L);
    AddFrame(L);
    AddMetalayer(L);
    AddSchunk(L);
    // shape, index, stride?
    // nd?

    //
    //
    //

    lua_newuserdata(L, BLOSC2_MIN_SCRATCH_SIZE); // blosc2, scratch
    lua_setfield(L, LUA_REGISTRYINDEX, BLOSC2_SCRATCH); // blosc2; registry[scratch] = scratch

    //
    //
    //

    lua_newtable(L); // blosc2, weakpairing
    lua_createtable(L, 0, 1); // blosc2, weakpairing, mt
    lua_pushliteral(L, "k"); // blosc2, weakpairing, mt, "k"
    lua_setfield(L, -2, "__mode"); // blosc2, weakpairing, mt = { __mode = "k" }
    lua_setmetatable(L, -2); // blosc2, weakpairing
    lua_setfield(L, LUA_REGISTRYINDEX, BLOSC2_WEAK_PAIRING); // blosc2; registry[weakpairing] = weakpairing

    //
    //
    //

    static std::atomic<int> sCount;

    ++sCount;

    LuaXS::AddCloseLogic(L, [](lua_State *) {
        if (0 == --sCount) blosc2_destroy();

        return 0;
    });
}

//
//
//

CORONA_EXPORT int luaopen_plugin_blosc2 (lua_State* L)
{
    blosc2_init();

    LoadModule(L); // blosc2

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
