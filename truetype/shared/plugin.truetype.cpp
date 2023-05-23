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

#include "truetype.h"
#include "ByteReader.h"
#include "utils/LuaEx.h"
#include "utils/Path.h"

//
//
//

#define TRUETYPE_STORE "xs.truetype.store"

//
//
//

static lua_State * GetStateAndStore (void * context)
{
    lua_State * L = static_cast<lua_State *>(context);

    lua_pushliteral(L, TRUETYPE_STORE); // ..., key
    lua_rawget(L, LUA_REGISTRYINDEX); // ..., store

    return L;
}

//
//
//

void * Alloc (size_t size, void * context)
{
    lua_State * L = GetStateAndStore(context); // ..., store
    void * data = lua_newuserdata(L, size); // ..., store, data

    lua_pushlightuserdata(L, data); // ..., store, data, data_ptr
    lua_insert(L, -2); // ..., store, data_ptr, data
    lua_rawset(L, -3); // ..., store = { ..., [data_ptr] = data }
    lua_pop(L, 1); // ...

    return data;
}

void Free (void * data, void * context, bool leave_on_stack)
{
    lua_State * L = GetStateAndStore(context); // ..., store

    if (leave_on_stack)
    {
        lua_pushlightuserdata(L, data); // ..., store, data_ptr
        lua_rawget(L, -2); // ..., store, data?
        lua_insert(L, -2); // ..., data?, store
    }

    lua_pushlightuserdata(L, data); // ...[, data?], store, data_ptr
    lua_pushnil(L); // ...[, data?], store, data_ptr, nil
    lua_rawset(L, -3); // ...[, data?], store
    lua_pop(L, 1); // ...[, data?]
}

void Push (lua_State * L, void * ptr, bool bAsUserdata)
{
    Free(ptr, L, true); // ..., data

	if (bAsUserdata)
	{
		ByteXS::AddBytesMetatable(L, TRUETYPE_BYTES);
	}

	else
	{
		lua_pushlstring(L, static_cast<const char *>(ptr), lua_objlen(L, -1)); // ..., data, bytes
        lua_remove(L, -2); // ..., bytes
	}
}

//
//
//

// TODO: consider just doing a mutex-backed list with thread::id / Lua state combo and __gc cleanup
// #define STBTT_assert(cond) if (!(cond)) luaL_error(tls_L, #cond)
#define STBTT_malloc(x, u) Alloc(x, u)
#define STBTT_free(x, u) Free(x, u)
#define STB_RECT_PACK_IMPLEMENTATION
#define STB_TRUETYPE_IMPLEMENTATION

#include "stb_rect_pack.h"
#include "stb_truetype.h"

//
//
//

static int GetOffset (lua_State * L, const ByteReader & reader)
{
    return stbtt_GetFontOffsetForIndex(static_cast<const unsigned char *>(reader.mBytes), luaL_optint(L, 2, 0));
}

static luaL_Reg truetype_funcs[] = {
    {
        "CompareUTF8toUTF16_bigendian", [](lua_State * L)
        {
            return LuaXS::PushArgAndReturn(L, !!stbtt_CompareUTF8toUTF16_bigendian(luaL_checkstring(L, 1), lua_objlen(L, 1), luaL_checkstring(L, 2), lua_objlen(L, 2))); // str1, str2, match
        }
    }, {
		"GetFontOffsetForIndex", [](lua_State * L)
		{
			ByteReader reader{L, 1};

			return LuaXS::PushArgAndReturn(L, GetOffset(L, reader));// bytes[, index], offset
		}
	}, {
		"InitFont", NewFont
    }, {
		"PackBegin", NewPacking
    }, {
		"PointSize", PointSize
	},
	{ nullptr, nullptr }
};

//
//
//

CORONA_EXPORT int luaopen_plugin_truetype (lua_State * L)
{
    lua_pushliteral(L, TRUETYPE_STORE); // key
    lua_newtable(L); // key, store
    lua_rawset(L, LUA_REGISTRYINDEX); // registry = { ..., [key] = store }
	lua_newtable(L); // truetype
	luaL_register(L, nullptr, truetype_funcs);

    PathXS::Directories::Instantiate(L); // truetype, dirs
    
    lua_pushcclosure(L, [](lua_State * L) {
        lua_settop(L, 2);   // params / filename, base_dir?
        lua_pushvalue(L, lua_upvalueindex(1));  // params / filename, base_dir?, dirs
        
        PathXS::Directories * dirs = LuaXS::UD<PathXS::Directories>(L, -1);
        
        if (lua_istable(L, 1))
        {
            lua_getfield(L, 1, "index");   // params, base_dir?, dirs, index?
            lua_replace(L, 2);  // params, index?, dirs; NewFont looks for offset in arg #2
            lua_getfield(L, 1, "is_absolute");    // params, index?, dirs, is_absolute
            lua_getfield(L, 1, "filename");    // params, index?, dirs, is_absolute, filename
            lua_getfield(L, 1, "baseDir");  // params, index?, dirs, is_absolute, filename, base_dir
        }
        
        else
        {
            lua_pushnil(L); // filename, base_dir?, dirs, nil
            lua_pushvalue(L, 1);// filename, base_dir?, dirs, nil, filename
            lua_pushvalue(L, dirs->IsDir(L, 2) ? 2 : -2);   // filename, base_dir?, dirs, nil, filename, base_dir / nil
            lua_pushnil(L); // filename, base_dir?, dirs, nil, filename, base_dir / nil, nil
            lua_replace(L, 2);  // filename, nil, dirs, nil, filename, base_dir / nil; NewFont looks for offset in arg #2
        }
        
        return LuaXS::ResultOrNil(L, dirs->WithFileContentsDo(L, -2, -3, [L](ByteReader & bytes) {
            lua_pushvalue(L, -1);   // filename, index?, ..., font_data, font_data
            lua_replace(L, 1);  // font_data, index?, ..., font_data; NewFont looks for data in arg #1

            if (!lua_isnil(L, 2))
            {
                lua_pushinteger(L, GetOffset(L, bytes));// font_data, index, ..., font_data, offset
                lua_replace(L, 2);  // font_data, offset, ..., font_data
            }
            
            NewFont(L); // font_data, offset?, ..., font_data, font / nil

            return !lua_isnil(L, -1);
        }));// params / filename / font_data, dirs, is_absolute, filename, base_dir / nil, font / nil[, nil]
    }, 1);  // truetype, LoadFont
    lua_setfield(L, -2, "LoadFont");// truetype = { ..., LoadFont = LoadFont }
        
    return 1;
}
