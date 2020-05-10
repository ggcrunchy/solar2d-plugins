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

//
static luaL_Reg truetype_funcs[] = {
	{
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

MemoryXS::LuaMemory * truetype_GetMemory (void)
{
	return tls_truetypeMM;
}

//
CORONA_EXPORT int luaopen_plugin_truetype (lua_State * L)
{
	//
	tls_truetypeMM = MemoryXS::LuaMemory::New(L);

	//
	lua_newtable(L);// truetype
	luaL_register(L, nullptr, truetype_funcs);

    PathXS::Directories::Instantiate(L);// truetype, dirs
    
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
