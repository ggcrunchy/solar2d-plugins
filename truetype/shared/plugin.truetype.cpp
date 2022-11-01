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
#include "utils/Memory.h"
#include "utils/Path.h"
#include "utils/Thread.h"

ThreadXS::TLS<MemoryXS::LuaMemory *> tls_truetypeMM;

//
#define STBTT_assert(cond) if (!(cond)) tls_truetypeMM->FailAssert(#cond)
#define STBTT_malloc(x, u) ((void)(u), tls_truetypeMM->Malloc(x))
#define STBTT_free(x, u)   ((void)(u), tls_truetypeMM->Free(x))
#define STB_RECT_PACK_IMPLEMENTATION
#define STB_TRUETYPE_IMPLEMENTATION

#include "stb_rect_pack.h"
#include "stb_truetype.h"


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

PathXS::Directories * PrepareData (lua_State * L, const char * key1, const char * key2 = nullptr)
{
    lua_pushvalue(L, lua_upvalueindex(1)); // params / filename[, base_dir?], ..., dirs
        
    PathXS::Directories * dirs = LuaXS::UD<PathXS::Directories>(L, -1);

    if (lua_istable(L, 1))
    {
        lua_getfield(L, 1, key1); // params, ..., dirs, arg1?
        lua_insert(L, 2); // params, arg1?, ..., dirs

        if (key2)
        {
            lua_getfield(L, 1, key2); // params, arg1?, ..., dirs, arg2?
            lua_replace(L, 3); // params, arg1?, arg2?, ..., dirs
        }

        lua_getfield(L, 1, "from_memory"); // params, arg1?[, arg2?], ..., dirs, from_memory?

        if (!lua_isnil(L, -1))
        {
            lua_pushboolean(L, 0); // params, arg1?[, arg2?], ..., dirs, from_memory, false
            lua_insert(L, -3); // params, arg1?[, arg2?], ..., false, dirs, from_memory
        }

        else
        {
            lua_getfield(L, 1, "is_absolute"); // params, arg1?[, arg2?], ..., dirs, is_absolute
            lua_getfield(L, 1, "filename"); // params, arg1?[, arg2?], ..., dirs, is_absolute, filename
            lua_getfield(L, 1, "baseDir"); // params, arg1?[, arg2?], ..., dirs, is_absolute, filename, base_dir
        }
    }
        
    else
    {
        bool is_dir = dirs->IsDir(L, 2);

        lua_pushnil(L); // filename, base_dir?, ..., dirs, nil
        lua_pushvalue(L, 1); // filename, base_dir?, ..., dirs, nil, filename
        lua_pushvalue(L, is_dir ? 2 : -2); // filename, base_dir?, ..., dirs, nil, filename, base_dir / nil

        if (is_dir) lua_remove(L, 2); // filename, ..., dirs, nil, filename, base_dir / nil

        int top = lua_gettop(L) - 3, wanted_top = key2 ? 3 : 2;

        while (top < wanted_top)
        {
            lua_pushnil(L); // filename, ..., dirs, nil, filename, base_dir / nil, nil
            lua_insert(L, ++top); // filename, ..., nil, ..., dirs, nil, filename, base_dir / nil
        }
    }

    return dirs;
}

//
//
//

MemoryXS::LuaMemory * truetype_GetMemory (void)
{
	return tls_truetypeMM;
}

//
//
//

static int GetMatchFlag (lua_State * L, int arg)
{
    const char * names[] = { "DONTCARE", "BOLD", "ITALIC", "UNDERSCORE", "NONE", nullptr };
    int flags[] = { STBTT_MACSTYLE_DONTCARE, STBTT_MACSTYLE_BOLD, STBTT_MACSTYLE_ITALIC, STBTT_MACSTYLE_UNDERSCORE, STBTT_MACSTYLE_NONE };

    return flags[luaL_checkoption(L, arg, nullptr, names)];
}

//
//
//

CORONA_EXPORT int luaopen_plugin_truetype (lua_State * L)
{
	tls_truetypeMM = MemoryXS::LuaMemory::New(L);

	lua_newtable(L); // truetype
	luaL_register(L, nullptr, truetype_funcs);

    PathXS::Directories::Instantiate(L); // truetype, dirs
    
    luaL_Reg closures[] = {
        {
            "FindMatchingFont", [](lua_State * L)
            {
                PathXS::Directories * dirs = PrepareData(L, "name", "flags"); // params / filename, name, flags, dirs, is_absolute, filename, base_dir? 
                int offset = -1;

                dirs->WithFileContentsDo(L, -2, -3, [L, &offset](ByteReader & bytes) {
                    offset = stbtt_FindMatchingFont(static_cast<const unsigned char *>(bytes.mBytes), luaL_checkstring(L, 2), GetMatchFlag(L, 3));

                    return true;
                });

                return LuaXS::PushArgAndReturn(L, offset); // params / filename, name, flags, dirs, is_absolute, filename, base_dir?, offset
            }
        }, {
            "GetFontOffsetForIndex", [](lua_State * L)
		    {
                PathXS::Directories * dirs = PrepareData(L, "index"); // params / filename, index?, dirs, is_absolute, filename, base_dir? 
                int offset = -1;

                dirs->WithFileContentsDo(L, -2, -3, [L, &offset](ByteReader & bytes) {
                    offset = GetOffset(L, bytes);

                    return true;
                });

			    return LuaXS::PushArgAndReturn(L, offset); // params / filename, index?, dirs, is_absolute, filename, base_dir?, offset
            }
        }, {
            "LoadFont", [](lua_State * L)
            {
                PathXS::Directories * dirs = PrepareData(L, "index"); // params / filename, index?, dirs, is_absolute, filename, base_dir?
        
                return LuaXS::ResultOrNil(L, dirs->WithFileContentsDo(L, -2, -3, [L](ByteReader & bytes) {
                    lua_pushvalue(L, -1); // params / filename, index?, ..., font_data, font_data
                    lua_replace(L, 1); // font_data, index?, ..., font_data; NewFont looks for data in arg #1

                    if (!lua_isnil(L, 2))
                    {
                        lua_pushinteger(L, GetOffset(L, bytes)); // font_data, index, ..., font_data, offset
                        lua_replace(L, 2); // font_data, offset, ..., font_data
                    }
            
                    NewFont(L); // font_data, offset?, ..., font_data, font / nil

                    return !lua_isnil(L, -1);
                })); // params / filename / font_data, dirs, is_absolute, filename, base_dir / nil, font / nil[, nil]
            }
        },
        { nullptr, nullptr }
    };

    LuaXS::AddClosures(L, closures, 1); // truetype
        
    return 1;
}
