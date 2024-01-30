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

int GetIndexAfterLastSeparator (const char * filename, int len)
{
    for (int i = len - 1; i >= 0; --i)
    {
    #ifdef LUA_WIN
        if (strstr(&filename[i], LUA_DIRSEP))
        {
            return i + sizeof(LUA_DIRSEP) - 1;
        } else
    #endif
        if (filename[i] == '/') return i + 1;
    }

    return 0;
}

//
//
//

int GetExtensionIndex (const char * filename, int len)
{
    for (int i = len - 1; i >= 0; --i)
    {
        if (filename[i] == '.') return i;
    }

    return -1;
}

//
//
//

int Error (lua_State * L, const char * err)
{
    lua_pushnil(L); // ..., nil
    lua_pushstring(L, err); // ..., nil, err

    return 2;
}

//
//
//

void AddMethods (lua_State * L, void (*add)(lua_State *), const char * name)
{
    if (luaL_newmetatable(L, name)) // ..., object, mt
    {
        lua_pushvalue(L, -1); // ..., object, mt, mt
        lua_setfield(L, -2, "__index"); // ..., object, mt; mt.__index = mt

        add(L);
    }

    lua_setmetatable(L, -2); // ..., object; object.metatable = mt
}

//
//
//

void LoadFile (lua_State * L)
{
    lua_pushvalue(L, lua_upvalueindex(1)); // ..., filename, LoadFile
    lua_insert(L, -2); // ..., LoadFile, filename
    lua_getfield(L, 1, "baseDir"); // ..., LoadFile, filename, baseDir
    lua_call(L, 2, 1); // ..., file?
}

//
//
//

static int AuxRegister (lua_State * L)
{
    lua_settop(L, lua_isuserdata(L, 2) ? 2 : 1); // filename[, baseDir]
    luaL_argcheck(L, lua_type(L, 1) == LUA_TSTRING, 1, "Expected string filename");
    lua_pushvalue(L, lua_upvalueindex(2)); // filename[, baseDir], system.pathForFile
    lua_insert(L, 1); // system.pathForFile, filename[, baseDir]
    lua_call(L, lua_gettop(L) - 1, 1); // path?

    if (!lua_isnil(L, -1))
    {
        lua_pushvalue(L, lua_upvalueindex(1)); // path, io.open
        lua_insert(L, -2); // io.open, path
        lua_pushliteral(L, "rb"); // io.open, path, "rb"
        lua_call(L, 2, 1); // fp?

        if (!lua_isnil(L, -1))
        {
            lua_getfield(L, -1, "read"); // fp, fp:read
            lua_pushvalue(L, -2); // fp, fp:read, fp
            lua_pushliteral(L, "*a"); // fp, fp:read, fp, "*a"
            lua_call(L, 2, 1); // fp, contents
            lua_insert(L, -2); // contents, fp
            lua_getfield(L, -1, "close"); // contents, fp, fp:close
            lua_insert(L, -2); // contents, fp:close, fp
            lua_call(L, 1, 0); // contents
        }
    }

    return 1;
}

//
//
//

void RegisterWithLoadFile (lua_State * L, luaL_Reg closures[])
{
    lua_getglobal(L, "io"); // ..., funcs, io
    luaL_argcheck(L, lua_istable(L, -1), -1, "Non-table `io`");
    lua_getglobal(L, "system"); // ..., funcs, io, system
    luaL_argcheck(L, lua_istable(L, -1), -1, "Non-table `system`");
    lua_getfield(L, -2, "open"); // ..., funcs, io, system, io.open
    luaL_argcheck(L, lua_isfunction(L, -1), -1, "Non-function `io.open`");
    lua_getfield(L, -2, "pathForFile"); // ..., funcs, io, system, io.open, system.pathForFile
    luaL_argcheck(L, lua_isfunction(L, -1), -1, "Non-function `system.pathForFile`");
    lua_pushcclosure(L, AuxRegister, 2); // ..., funcs, io, system, LoadFile

    for (int i = 0; closures[i].func; ++i)
    {
        lua_pushvalue(L, -1); // ..., funcs, io, system, LoadFile, LoadFile
        lua_pushcclosure(L, closures[i].func, 1); // ..., funcs, io, system, LoadFile, func
        lua_setfield(L, -5, closures[i].name); // ..., funcs = { ..., [name] = func }, io, system, LoadFile
    }

    lua_pop(L, 3); // ..., funcs
}