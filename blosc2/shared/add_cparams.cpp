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

#define CPARAMS_METATABLE_NAME "blosc2.cparams"

//
//
//

static ParamsBox<blosc2_cparams> * AuxGet (lua_State * L, int arg)
{
	return LuaXS::CheckUD<ParamsBox<blosc2_cparams>>(L, arg, CPARAMS_METATABLE_NAME);
}

//
//
//

blosc2_cparams * GetCparams (lua_State * L, int arg)
{
	return AuxGet(L, arg)->Get();
}

//
//
//
#if 0
/**
 * @brief The parameters for creating a context for compression purposes.
 *
 * In parenthesis it is shown the default value used internally when a 0
 * (zero) in the fields of the struct is passed to a function.
 */
typedef struct {
  uint8_t compcode;
  //!< The compressor codec.
  uint8_t compcode_meta;
  //!< The metadata for the compressor codec.
  uint8_t clevel;
  //!< The compression level (5).
  int use_dict;
  //!< Use dicts or not when compressing (only for ZSTD).
  int32_t typesize;
  //!< The type size (8).
  int16_t nthreads;
  //!< The number of threads to use internally (1).
  int32_t blocksize;
  //!< The requested size of the compressed blocks (0 means automatic).
  int32_t splitmode;
  //!< Whether the blocks should be split or not.
  void* schunk;
  //!< The associated schunk, if any (NULL).
  uint8_t filters[BLOSC2_MAX_FILTERS];
  //!< The (sequence of) filters.
  uint8_t filters_meta[BLOSC2_MAX_FILTERS];
  //!< The metadata for filters.
  blosc2_prefilter_fn prefilter;
  //!< The prefilter function.
  blosc2_prefilter_params *preparams;
  //!< The prefilter parameters.
  blosc2_btune *udbtune;
  //!< The user-defined BTune parameters.
  bool instr_codec;
  //!< Whether the codec is instrumented or not
  void *codec_params;
  //!< User defined parameters for the codec
  void *filter_params[BLOSC2_MAX_FILTERS];
  //!< User defined parameters for the filters
} blosc2_cparams;


BLOSC_EXPORT int blosc2_chunk_zeros(blosc2_cparams cparams, int32_t nbytes,
                                    void* dest, int32_t destsize);
BLOSC_EXPORT int blosc2_chunk_nans(blosc2_cparams cparams, int32_t nbytes,
                                   void* dest, int32_t destsize);
BLOSC_EXPORT int blosc2_chunk_repeatval(blosc2_cparams cparams, int32_t nbytes,
                                        void* dest, int32_t destsize, const void* repeatval);
BLOSC_EXPORT int blosc2_chunk_uninit(blosc2_cparams cparams, int32_t nbytes,
                                     void* dest, int32_t destsize);

#endif
static void AddMethods (lua_State * L)
{
	LuaXS::AttachMethods(L, CPARAMS_METATABLE_NAME, [](lua_State * L) {
		luaL_Reg funcs[] = {
			{
				"__gc", [](lua_State * L)
				{
					AuxGet(L, 1)->Free();

					return 0;
				}
			}, {
				"__newindex", [](lua_State * L)
				{
					const char * str = lua_tostring(L, 2);

					if (!str) return 0;

					blosc2_cparams * cparams = GetCparams(L);

					if (strcmp(str, "compcode") == 0)
					{
						const char * names[] = { "BLOSCLZ", "LZ4", "LZ4HC", "ZLIB", "ZSTD", nullptr };
						int codes[] = { BLOSC_BLOSCLZ , BLOSC_LZ4, BLOSC_LZ4HC, BLOSC_ZLIB, BLOSC_ZSTD };

						cparams->compcode = codes[luaL_checkoption(L, 3, nullptr, names)];
					}

					else if (strcmp(str, "clevel") == 0) cparams->clevel = luaL_checkint(L, 3);
					else if (strcmp(str, "nthreads") == 0) cparams->nthreads = (int16_t)luaL_checkint(L, 3);
					else if (strcmp(str, "typesize") == 0) cparams->typesize = luaL_checkint(L, 3);
					
					// TODO: others

					return 0;
				}
			},
			{ nullptr, nullptr }
		};

		luaL_register(L, nullptr, funcs);

		//
		//
		// 
 
		LuaXS::AttachProperties(L, [](lua_State * L) {
			const char * str = lua_tostring(L, 2);

			if (!str) return 0;

			const blosc2_cparams * cparams = GetCparams(L);

			if (strcmp(str, "nthreads") == 0) lua_pushinteger(L, cparams->nthreads); // cparams, "nthreads", nthreads
			else if (strcmp(str, "schunk") == 0) GetSchunk(L, 1, cparams->schunk); // cparams, "schunk", schunk / nil
			// TODO: others
			else return 0;

			return 1;
		});
	});
}

//
//
//

void WrapCparams (lua_State * L, blosc2_cparams * cparams)
{
	LuaXS::NewTyped<ParamsBox<blosc2_cparams>>(L, cparams); // ..., cparams

	AddMethods(L);
}

//
//
//

void AddCparams (lua_State * L)
{
	luaL_Reg funcs[] = {
		{
			"cparams_new", [](lua_State * L)
			{
				LuaXS::NewTyped<ParamsBox<blosc2_cparams>>(L, BLOSC2_CPARAMS_DEFAULTS); // ..., cparams

				AddMethods(L);

				return 1;
			}
		},
		{ nullptr, nullptr }
	};

	luaL_register(L, nullptr, funcs);
}