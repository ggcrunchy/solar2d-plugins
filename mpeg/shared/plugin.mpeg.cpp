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
#include "jo_file.h"

void jo_write_mpeg (/*FILE *fp*/JO_File * fp, /*const void*/unsigned char * rgbx, int width, int height, int fps);

#define PL_MPEG_IMPLEMENTATION
#include "pl_mpeg.h"

CORONA_EXPORT int luaopen_plugin_mpeg (lua_State* L)
{
	luaL_Reg mpeg_funcs[] = {
		{
			"Write", [](lua_State * L)
			{
/*
                    const int i1 = bGetFilename ? 2 : 1;

                    EnsureDims<bGetFilename>(L);// [filename, w, h, ]frames[, opts]
                    PathXS::WriteAux waux{L, i1, bGetFilename ? GetPathData(L) : nullptr};

                    int fps = 30;
                    bool bAppend = false;

                    LuaXS::Options{L, i1 + 3}   .Add("append", bAppend)
                                                .Add("fps", fps);

                    WithExtantFile<bGetFilename>{}.Do(waux.mFilename, bAppend, [&waux, &fps](FILE * fp) {
                        fseek(fp, 4, SEEK_SET); // sequence header

                        // 12 bits for width, height
                        int8_t n1, n2, n3;

                        fread(&n1, 1, 1, fp);
                        fread(&n2, 1, 1, fp);
                        fread(&n3, 1, 1, fp);

                        waux.mW = (int(n1) << 4) | (int(n2 & 0xF0) >> 4);
                        waux.mH = (int(n2 & 0xF) << 8) | int(n3 & 0xFF);

                        fps = 0;

                        fread(&fps, 1, 1, fp);

                        switch (fps)
                        {
                            case 0x12:
                                fps = 24;
                                break;
                            case 0x13:
                                fps = 25;
                                break;
                            case 0x15:
                                fps = 30;
                                break;
                            case 0x16:
                                fps = 50;
                                break;
                            default:
                                fps = 60;
                                break;
                        }
                    });

                    lua_settop(L, i1 + 2);	// [filename, ]w, h, frames
                    luaL_checktype(L, i1 + 2, LUA_TTABLE);

                    size_t n = lua_objlen(L, i1 + 2), ntotal = size_t(waux.mW * waux.mH * 4);

                    std::vector<unsigned char> mpeg(n * ntotal);

                    for (auto elem : LuaXS::Range(L, i1 + 2))
                    {
                        ByteReader bytes{L, -1};

                        auto ucbytes = waux.GetBytes(L, bytes, waux.mW * 4);// [filename, ]w, h, frames, bytes / err

                        memcpy(mpeg.data() + (elem - 1) * ntotal, ucbytes, ntotal);
                    }

                    //
                    JO_File file{L, bGetFilename ? waux.mFilename : nullptr, bAppend ? "ab" : "wb"};

                    if (bGetFilename && !file.mFP) luaL_error(L, "Error: Could not WriteMPEG to %s", waux.mFilename);	// filename, w, h, frames, false, err

                    //
                    for (size_t i = 0; i < n; ++i) jo_write_mpeg(&file, mpeg.data() + i * ntotal, waux.mW, waux.mH, fps);

                    file.Close();	// [filename, ]w, h, frames, true / memory
                    */

                    return 1;
			}
		},
		{ nullptr, nullptr }
	};

	lua_newtable(L);// mpeg
	luaL_register(L, NULL, mpeg_funcs);

	return 1;
}
