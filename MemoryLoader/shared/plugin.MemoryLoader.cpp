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

#include "dll_loader.h"
#include "ByteReader.h"
#include "utils/LuaEx.h"

// This module is adapted from Lua's own require() implementation.

//
//
//

#define MEMORY_LOADER_LIB_NAME "MemoryLoader.Lib"

//
//
//

#define LIBPREFIX "LOADLIB_ML:"

//
//
//

static HCUSTOMMODULE * GetCustomModule (lua_State * L)
{
    return LuaXS::CheckUD<HCUSTOMMODULE>(L, 1, MEMORY_LOADER_LIB_NAME);
}

//
//
//

template<int kListIndex> void AddLib (lua_State * L, HCUSTOMMODULE mod)
{
    *LuaXS::NewTyped<HCUSTOMMODULE>(L) = mod; // filename, zip_name / callback, zpath / store, lib

    LuaXS::AttachMethods(L, MEMORY_LOADER_LIB_NAME, [](lua_State * L) {
        luaL_Reg funcs[] = {
            {
                "__gc", [](lua_State * L)
                {
                    HCUSTOMMODULE * lib = GetCustomModule(L);

                    if (*lib) RecordList::UnloadDLL(*lib);

                    *lib = nullptr;

                    return 0;
                }
            },
            { nullptr, nullptr }
        };

        luaL_register(L, nullptr, funcs);

        //
        //
        //

        LuaXS::NewWeakKeyedTable(L); // ..., mt, procs_to_lib; n.b. will do double-duty as "GetLuaFunction" cookie
        
        lua_pushvalue(L, -1); // ..., mt, procs_to_lib, procs_to_lib
        lua_pushvalue(L, kListIndex); // ..., mt, procs_to_lib, procs_to_lib, list
        lua_pushcclosure(L, [](lua_State * L) {
            HCUSTOMMODULE * lib = GetCustomModule(L);
            const char * name = luaL_checkstring(L, 2);

            if (*lib)
            {
                RecordList * list = LuaXS::UD<RecordList>(L, lua_upvalueindex(2));
                RecordList::Proc proc = list->GetProcFromDLL(*lib, name);

                if (proc)
                {
                    if (lua_equal(L, 3, lua_upvalueindex(1))) lua_pushcfunction(L, (lua_CFunction)proc); // lib, name, cookie, func

                    else
                    {
                        *LuaXS::NewTyped<RecordList::Proc>(L) = proc; // lib, name, proc

                        lua_pushvalue(L, -1); // lib, name, proc, proc
                        lua_pushvalue(L, 1); // lib, name, proc, proc, lib
                        lua_rawset(L, lua_upvalueindex(1)); //; lib, name, proc; procs_to_lib[proc] = lib
                    }

                    return 1;
                }
            }

            lua_pushnil(L); // lib, name, nil

            return 1;
        }, 2); // ..., mt, procs_to_lib, GetProc; GetProc.upvalues = { procs_to_lib, list }
        lua_pushvalue(L, -1); // ..., mt, procs_to_lib, GetProc, GetProc
        lua_setfield(L, -4, "GetProc"); // ..., mt = { __gc, GetProc = GetProc }, procs_to_lib, GetProc

        //
        //
        //

        lua_pushcclosure(L, [](lua_State * L) {
            lua_settop(L, 2); // lib, name
            lua_pushvalue(L, lua_upvalueindex(2)); // lib, name, GetProc
            lua_insert(L, 1); // GetProc, lib, name
            lua_pushvalue(L, lua_upvalueindex(1)); // GetProc, lib, name, cookie
            lua_call(L, 3, 1); // func?

            return 1;
        }, 2); // mt, GetLuaFunction; GetLuaFunction.upvalues = { cookie, GetProc }
        lua_setfield(L, -2, "GetLuaFunction"); // ..., mt = { __gc, GetProc, GetLuaFunction = GetLuaFunction }
    });
}

//
//
//

CORONA_EXPORT int luaopen_plugin_MemoryLoader (lua_State* L)
{
    lua_newtable(L); // MemoryLoader
    lua_getglobal(L, "system"); // MemoryLoader, system
    luaL_checktype(L, -1, LUA_TTABLE);
    lua_getfield(L, -1, "pathForFile"); // MemoryLoader, system, system.pathForFile
    luaL_checktype(L, -1, LUA_TFUNCTION);
    lua_remove(L, -2); // MemoryLoader, system.pathForFile
    lua_newtable(L); // MemoryLoader, system.pathForFile, mounts

    LuaXS::NewTyped<RecordList>(L); // MemoryLoader, system.pathForFile, mounts, list
    LuaXS::AttachGC(L, LuaXS::TypedGC<RecordList>);

    //
    //
    //

    lua_pushcclosure(L, [](lua_State * L) {
        size_t len = lua_objlen(L, lua_upvalueindex(2));

        //
        //
        //

        if (lua_type(L, 2) == LUA_TBOOLEAN) // append or remove?
        {
            if (lua_toboolean(L, 2)) // append?
            {
                const char * name = lua_tostring(L, 1);

                if (name)
                {
                    lua_pushvalue(L, lua_upvalueindex(1)); // zip_name, true, system.pathForFile
                    lua_pushvalue(L, 1); // zip_name, true, system.pathForFile, zip_name
                    lua_call(L, 1, 1); // zip_name, true, zpath?
                }

                if (!lua_isnil(L, -1))
                {
                    lua_settop(L, 1); // zip_name / callback
                    lua_rawseti(L, lua_upvalueindex(2), int(len) + 1); // (empty); mounts[#mounts + 1] = zip_name / callback
                }
            
                else CORONA_LOG_WARNING("No zip found with name '%s'", name);
            }

            //
            //
            //

            else // remove
            {
                for (size_t i = len; i > 0; --i, lua_pop(L, 1))
                {
                    lua_rawgeti(L, lua_upvalueindex(2), int(i)); // zip_name / callback, false, zip_name2 / callback2

                    if (!lua_equal(L, 1, -1)) continue;

                    if (i < len)
                    {
                        lua_rawgeti(L, lua_upvalueindex(2), int(len)); // zip_name / callback, false, zip_name2 / callback2, last
                        lua_rawseti(L, lua_upvalueindex(2), int(i)); // zip_name / callback, false, zip_name2 / callback2; mounts = { ..., last, ... }
                    }
                        
                    lua_pushnil(L); // zip_name / callback, false, zip_name2 / callback2, nil
                    lua_rawseti(L, lua_upvalueindex(2), int(len)); // zip_name / callback, false, zip_name2 / callback2; mounts = { ..., nil }
                }
            }

            return 0;
        }

        //
        //
        //

        else
        {
            const int kListIndex = lua_upvalueindex(3);

            const char * name = lua_pushfstring(L, "%s.dll", luaL_checkstring(L, 1)); // dll_name, dll_name .. ".dll"
            
            lua_remove(L, 1); // dll_name*

            RecordList * list = LuaXS::UD<RecordList>(L, kListIndex);
            HCUSTOMMODULE mod = list->LoadCached(name);

            for (size_t i = 1; !mod && i <= len; ++i, lua_pop(L, 2))
            {
                lua_rawgeti(L, lua_upvalueindex(2), int(i)); // dll_name*, zip_name / callback
        
                if (!lua_isfunction(L, -1))
                {
                    lua_pushvalue(L, lua_upvalueindex(1)); // dll_name*, zip_name, system.pathForFile
                    lua_pushvalue(L, -2); // dll_name*, zip_name, system.pathForFile, zip_name
                    lua_call(L, 1, 1); // dll_name*, zip_name, zpath?

                    mod = list->LoadDLL(name, nullptr, lua_tostring(L, -1));
                }

                else
                {
                    lua_newtable(L); // dll_name*, callback, store

                    mod = list->LoadDLL(name, L, nullptr);
                }
            }

            if (!mod) mod = RecordList::LoadFallback(name);

            if (mod) AddLib<kListIndex>(L, mod);
            else lua_pushnil(L); // dll_name*, zip_name, zpath, nil

            return 1;
        }
    }, 3); // MemoryLoader, WithMounts; WithMounts.upvalues = { system.pathForFile, mounts, list }

    //
    //
    //

    luaL_Reg funcs[] = {
        {
            "Loader", [](lua_State * L)
            {
                const char * modname = luaL_checkstring(L, 1);

                //
                //
                //

                lua_pushfstring(L, "%s%s", LIBPREFIX, modname); // modname, key
                lua_rawget(L, LUA_REGISTRYINDEX); // modname, lib?

                bool has_lib = !lua_isnil(L, -1);

                if (!has_lib)
                {
                    lua_pushvalue(L, lua_upvalueindex(1)); // modname, nil, WithMounts
                    lua_pushvalue(L, 1); // modname, nil, WithMounts, modname
                    lua_call(L, 1, 1); // modname, nil, lib / nil

                    if (!lua_isnil(L, -1))
                    {
                        lua_pushfstring(L, "%s%s", LIBPREFIX, modname); // modname, nil, lib, key
                        lua_pushvalue(L, -2); // modname, nil, lib, key, lib
                        lua_rawset(L, LUA_REGISTRYINDEX); // modname, nil, lib; registry[key] = lib

                        has_lib = true;
                    }
                }

                //
                //
                //

                if (has_lib)
                {
                    lua_getfield(L, -1, "GetLuaFunction"); // modname, nil, lib, lib:GetLuaFunction
                    lua_insert(L, -2); // modname, nil, lib:GetLuaFunction, lib
                    lua_pushfstring(L, "luaopen_%s", modname); // modname, nil, lib:GetLuaFunction, lib, luaopen_plugin_MODNAME
                    lua_pushliteral(L, "lua"); // modname, nil, lib:GetLuaFunction, lib, luaopen_plugin_MODNAME
                    lua_pcall(L, 2, 1, 0); // modname, nil, proc / err
                }

                else lua_pop(L, 1); // modname, nil

                //
                //
                //

                return 1;
            }
        }, {
            "LoadLibrary", [](lua_State * L)
            {
                lua_settop(L, 1); // modname   
                lua_pushvalue(L, lua_upvalueindex(1)); // modname, WithMounts
                lua_insert(L, 1); // WithMounts, modname
                lua_call(L, 1, 1); // lib / nil

                return 1;
            }
        }, {
            "Mount", [](lua_State * L)
            {
                luaL_argcheck(L, lua_type(L, 1) == LUA_TSTRING || lua_isfunction(L, 1), 1, "Mount() expects filename or callback");
                lua_settop(L, 1); // zip_name / callback   
                lua_pushvalue(L, lua_upvalueindex(1)); // zip_name / callback, WithMounts
                lua_insert(L, 1); // WithMounts, zip_name / callback
                lua_pushboolean(L, 1); // WithMounts, zip_name / callback, true
                lua_call(L, 2, 0); // (empty)

                return 0;
            }
        }, {
            "Unmount", [](lua_State * L)
            {
                lua_settop(L, 1); // zip_name / callback   
                lua_pushvalue(L, lua_upvalueindex(1)); // zip_name / callback, WithMounts
                lua_insert(L, 1); // WithMounts, zip_name / callback
                lua_pushboolean(L, 0); // WithMounts, zip_name / callback, false
                lua_call(L, 2, 0); // (empty)

                return 0;
            }
        },
        { nullptr, nullptr }
    };

    //
    //
    //

    LuaXS::AddClosures(L, funcs, 1); // MemoryLoader; per-closure upvalue = WithMounts

    //
    //
    //

	return 1;
}