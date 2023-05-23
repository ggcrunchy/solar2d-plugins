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

//
//
//

void AddCore (lua_State * L)
{
/*
BLOSC_EXPORT const char* blosc1_get_compressor(void);
BLOSC_EXPORT int blosc1_set_compressor(const char* compname);
BLOSC_EXPORT void blosc2_set_delta(int dodelta);

want these ???
BLOSC_EXPORT int blosc2_compcode_to_compname(int compcode, const char** compname);
BLOSC_EXPORT int blosc2_compname_to_compcode(const char* compname)


BLOSC_EXPORT const char* blosc2_list_compressors(void);
BLOSC_EXPORT const char* blosc2_get_version_string(void);
BLOSC_EXPORT int blosc2_get_complib_info(const char* compname, char** complib,
                                         char** version);

BLOSC_EXPORT void blosc1_cbuffer_sizes(const void* cbuffer, size_t* nbytes,
                                       size_t* cbytes, size_t* blocksize);
BLOSC_EXPORT int blosc2_cbuffer_sizes(const void* cbuffer, int32_t* nbytes,
                                      int32_t* cbytes, int32_t* blocksize);
BLOSC_EXPORT int blosc1_cbuffer_validate(const void* cbuffer, size_t cbytes,
                                         size_t* nbytes);
BLOSC_EXPORT void blosc1_cbuffer_metainfo(const void* cbuffer, size_t* typesize,
                                          int* flags);
BLOSC_EXPORT void blosc2_cbuffer_versions(const void* cbuffer, int* version,
                                          int* versionlz);
BLOSC_EXPORT const char* blosc2_cbuffer_complib(const void* cbuffer);
*/
    luaL_Reg funcs[] = {
        {
            "compress", [](lua_State * L) // todo: revise to in, out, opts?
            {
                int clevel = 5, has_params = lua_istable(L, 1), shuffle = BLOSC_SHUFFLE;
                size_t typesize = 8;

                if (has_params)
                {
                    lua_getfield(L, 1, "src"); // params, source?
                    luaL_argcheck(L, !lua_isnil(L, -1), -1, "Expected memory provider under `src`");
                }
 
                int32_t srcsize;

                const void * src = GetInputMemory(L, has_params ? -1 : 1, &srcsize);

                if (has_params)
                {
                    lua_getfield(L, 1, "clevel"); // params, source, clevel
                    lua_getfield(L, 1, "shuffle"); // params, source, clevel, shuffle
                    lua_getfield(L, 1, "typesize"); // params, clevel, shuffle, typesize
                    lua_getfield(L, 1, "dst"); // name, clevel, shuffle, typesize, source, dest?

                    const char * names[] = { "NOSHUFFLE", "SHUFFLE", "BITSHUFFLE", nullptr };
                    const int values[] = { BLOSC_NOSHUFFLE, BLOSC_SHUFFLE, BLOSC_BITSHUFFLE };

                    clevel = luaL_optint(L, -4, clevel);
                    shuffle = values[luaL_checkoption(L, -3, "SHUFFLE", names)];
                    typesize = luaL_optinteger(L, -2, typesize);
                }

                else lua_settop(L, 2); // source, dest?
                
                Output dst = GetOutputMemory(L, -1); // params / source[, clevel, shuffle, typesize], dest?[, scratch]
                
                return PushSizeOrError(L, blosc2_compress(clevel, shuffle, typesize, src, srcsize, dst.memory, dst.size), dst); // name, clevel, shuffle, typesize, source, dest?[, scratch], size / false[, str / error]
            }
        }, {
            "decompress", [](lua_State * L)
            {
                int32_t srcsize;

                const void * src = GetInputMemory(L, 1, &srcsize);
                Output dst = GetOutputMemory(L, 2); // source[, dest][, scratch]
                
                return PushSizeOrError(L, blosc2_decompress(src, srcsize, dst.memory, dst.size), dst);// source[, dest][, scratch], size / false[, str / error]
            }
        }, {
            "getitem", [](lua_State * L)
            {
                int32_t srcsize;

                const void * src = GetInputMemory(L, 1, &srcsize);
                Output dst = GetOutputMemory(L, 4); // source, start, nitems[, dest][, scratch]
                
                return PushSizeOrError(L, blosc2_getitem(src, srcsize, luaL_checkint(L, 2), luaL_checkint(L, 3), dst.memory, dst.size), dst); // source, start, nitems[, dest][, scratch], size / false[, str / error]
            }
        }, {
            "get_nthreads", [](lua_State * L)
            {
                lua_pushinteger(L, blosc2_get_nthreads()); // nthreads

                return 1;
            }
        }, {
            "set_nthreads", [](lua_State * L)
            {
                lua_pushinteger(L, blosc2_set_nthreads(luaL_checkint(L, 1))); // nthreads, old_nthreads

                return 1;
            }
        }, {
            "set_scratch_size", [](lua_State * L)
            {
                int size = luaL_checkint(L, 1);

                if (size < BLOSC2_MIN_SCRATCH_SIZE) size = BLOSC2_MIN_SCRATCH_SIZE;

                lua_newuserdata(L, size); // size, scratch
                lua_setfield(L, LUA_REGISTRYINDEX, BLOSC2_SCRATCH); // size; registry[scratch] = scratch

                return 0;
            }
        },
        { nullptr, nullptr }
    };

    luaL_register(L, nullptr, funcs);
}