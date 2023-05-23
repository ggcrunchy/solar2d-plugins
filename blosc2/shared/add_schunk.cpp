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

#define SCHUNK_METATABLE_NAME "blosc.schunk"

//
//
//

static blosc2_schunk * Get (lua_State * L)
{
    return *LuaXS::CheckUD<blosc2_schunk *>(L, 1, SCHUNK_METATABLE_NAME);
}

//
//
//

static blosc2_storage GetStorage (lua_State * L, int arg)
{
    blosc2_storage storage = BLOSC2_STORAGE_DEFAULTS;

    if (lua_istable(L, arg))
    {
        arg = CoronaLuaNormalize(L, arg);

        lua_getfield(L, arg, "cparams"); // ..., cparams
        lua_getfield(L, arg, "dparams"); // ..., cparams, dparams
        lua_getfield(L, arg, "urlpath"); // ..., cparams, dparams, urlpath
        lua_getfield(L, arg, "contiguous"); // ..., cparams, dparams, urlpath, contiguous

        if (!lua_isnil(L, -4)) storage.cparams = GetCparams(L, -4);
        if (!lua_isnil(L, -3)) storage.dparams = GetDparams(L, -3);
        if (!lua_isnil(L, -2)) storage.urlpath = const_cast<char *>(luaL_checkstring(L, -2));

        storage.contiguous = lua_toboolean(L, -1);

        lua_pop(L, 4); // ...
    }

    return storage;
}





#if 0
/**
 * @brief This struct is the standard container for Blosc 2 compressed data.
 *
 * This is essentially a container for Blosc 1 chunks of compressed data,
 * and it allows to overcome the 32-bit limitation in Blosc 1. Optionally,
 * a #blosc2_frame can be attached so as to store the compressed chunks contiguously.
 */
typedef struct blosc2_schunk {
  uint8_t version;
  uint8_t compcode;
  //!< The default compressor. Each chunk can override this.
  uint8_t compcode_meta;
  //!< The default compressor metadata. Each chunk can override this.
  uint8_t clevel;
  //!< The compression level and other compress params.
  uint8_t splitmode;
  //!< The split mode.
  int32_t typesize;
  //!< The type size.
  int32_t blocksize;
  //!< The requested size of the compressed blocks (0; meaning automatic).
  int32_t chunksize;
  //!< Size of each chunk. 0 if not a fixed chunksize.
  uint8_t filters[BLOSC2_MAX_FILTERS];
  //!< The (sequence of) filters.  8-bit per filter.
  uint8_t filters_meta[BLOSC2_MAX_FILTERS];
  //!< Metadata for filters. 8-bit per meta-slot.
  int64_t nchunks;
  //!< Number of chunks in super-chunk.
  int64_t current_nchunk;
  //!< The current chunk that is being accessed
  int64_t nbytes;
  //!< The data size (uncompressed).
  int64_t cbytes;
  //!< The data size + chunks header size (compressed).
  uint8_t** data;
  //!< Pointer to chunk data pointers buffer.
  size_t data_len;
  //!< Length of the chunk data pointers buffer.
  blosc2_storage* storage;
  //!< Pointer to storage info.
  blosc2_frame* frame;
  //!< Pointer to frame used as store for chunks.
  //!<uint8_t* ctx;
  //!< Context for the thread holder. NULL if not acquired.
  blosc2_context* cctx;
  //!< Context for compression
  blosc2_context* dctx;
  //!< Context for decompression.
  struct blosc2_metalayer *metalayers[BLOSC2_MAX_METALAYERS];
  //!< The array of metalayers.
  uint16_t nmetalayers;
  //!< The number of metalayers in the super-chunk
  struct blosc2_metalayer *vlmetalayers[BLOSC2_MAX_VLMETALAYERS];
  //<! The array of variable-length metalayers.
  int16_t nvlmetalayers;
  //!< The number of variable-length metalayers.
  blosc2_btune *udbtune;
  //<! Struct for BTune
  int8_t ndim;
  //<! The ndim (mainly for ZFP usage)
  int64_t *blockshape;
  //<! The blockshape (mainly for ZFP usage)
} blosc2_schunk;







BLOSC_EXPORT blosc2_schunk* blosc2_schunk_copy(blosc2_schunk *schunk, blosc2_storage *storage);
BLOSC_EXPORT blosc2_schunk* blosc2_schunk_from_buffer(uint8_t *cframe, int64_t len, bool copy);
BLOSC_EXPORT int64_t blosc2_schunk_to_buffer(blosc2_schunk* schunk, uint8_t** cframe, bool* needs_free);
BLOSC_EXPORT int64_t blosc2_schunk_to_file(blosc2_schunk* schunk, const char* urlpath);
BLOSC_EXPORT int64_t blosc2_schunk_append_file(blosc2_schunk* schunk, const char* urlpath);
BLOSC_EXPORT int64_t blosc2_schunk_append_chunk(blosc2_schunk *schunk, uint8_t *chunk, bool copy);
BLOSC_EXPORT int64_t blosc2_schunk_update_chunk(blosc2_schunk *schunk, int64_t nchunk, uint8_t *chunk, bool copy);
BLOSC_EXPORT int64_t blosc2_schunk_insert_chunk(blosc2_schunk *schunk, int64_t nchunk, uint8_t *chunk, bool copy);
BLOSC_EXPORT int64_t blosc2_schunk_delete_chunk(blosc2_schunk *schunk, int64_t nchunk);

BLOSC_EXPORT int blosc2_schunk_decompress_chunk(blosc2_schunk *schunk, int64_t nchunk, void *dest, int32_t nbytes);
BLOSC_EXPORT int blosc2_schunk_get_chunk(blosc2_schunk *schunk, int64_t nchunk, uint8_t **chunk,
                                         bool *needs_free);
BLOSC_EXPORT int blosc2_schunk_get_lazychunk(blosc2_schunk *schunk, int64_t nchunk, uint8_t **chunk,
                                             bool *needs_free);
BLOSC_EXPORT int blosc2_schunk_get_slice_buffer(blosc2_schunk *schunk, int64_t start, int64_t stop, void *buffer);
BLOSC_EXPORT int blosc2_schunk_set_slice_buffer(blosc2_schunk *schunk, int64_t start, int64_t stop, void *buffer);
BLOSC_EXPORT int blosc2_schunk_reorder_offsets(blosc2_schunk *schunk, int64_t *offsets_order);
BLOSC_EXPORT int64_t blosc2_schunk_frame_len(blosc2_schunk* schunk);
BLOSC_EXPORT int64_t blosc2_schunk_fill_special(blosc2_schunk* schunk, int64_t nitems,
                                            int special_value, int32_t chunksize);


/*********************************************************************
  Functions related with fixed-length metalayers.
*********************************************************************/

static inline int blosc2_meta_exists(blosc2_schunk *schunk, const char *name);


BLOSC_EXPORT int blosc2_meta_add(blosc2_schunk *schunk, const char *name, uint8_t *content,
                                 int32_t content_len);
BLOSC_EXPORT int blosc2_meta_update(blosc2_schunk *schunk, const char *name, uint8_t *content,
                                    int32_t content_len);

static inline void swap_store(void *dest, const void *pa, int size);
static inline int blosc2_meta_get(blosc2_schunk *schunk, const char *name, uint8_t **content,
                                  int32_t *content_len);


/*********************************************************************
  Variable-length metalayers functions.
*********************************************************************/

BLOSC_EXPORT int blosc2_vlmeta_exists(blosc2_schunk *schunk, const char *name);
BLOSC_EXPORT int blosc2_vlmeta_add(blosc2_schunk *schunk, const char *name,
                                   uint8_t *content, int32_t content_len,
                                   blosc2_cparams *cparams);
BLOSC_EXPORT int blosc2_vlmeta_update(blosc2_schunk *schunk, const char *name,
                                      uint8_t *content, int32_t content_len,
                                      blosc2_cparams *cparams);
BLOSC_EXPORT int blosc2_vlmeta_get(blosc2_schunk *schunk, const char *name,
                                   uint8_t **content, int32_t *content_len);
BLOSC_EXPORT int blosc2_vlmeta_delete(blosc2_schunk *schunk, const char *name);
BLOSC_EXPORT int blosc2_vlmeta_get_names(blosc2_schunk *schunk, char **names);

#endif

//
//
//

static void Methods (lua_State * L)
{
	luaL_Reg funcs[] = {
        {
            "append_buffer", [](lua_State * L)
            {
                int32_t srcsize;

                const void * src = GetInputMemory(L, 2, &srcsize);

                return PushInt64OrError(L, blosc2_schunk_append_buffer(Get(L), src, srcsize)); // schunk, bytes, size / false[, error]
            }
        }, {
            "avoid_cframe_free", [](lua_State * L)
            {
                blosc2_schunk_avoid_cframe_free(Get(L), lua_toboolean(L, 2));

                return 0;
            }
        }, {
            "__gc", [](lua_State * L)
            {
                blosc2_schunk_free(Get(L));

                return 0;
            }
        }, {
            "get_cparams", [](lua_State * L)
            {
                blosc2_cparams * cparams;

                blosc2_schunk_get_cparams(Get(L), &cparams);

                LuaXS::NewTyped<ParamsBox<blosc2_cparams>>(L, cparams); // schunk, cparams

                WeakKeyPair(L, -1, 1);

                return 1;
            }
        }, {
            "get_dparams", [](lua_State * L)
            {
                blosc2_dparams * dparams;

                blosc2_schunk_get_dparams(Get(L), &dparams);

                LuaXS::NewTyped<ParamsBox<blosc2_dparams>>(L, dparams); // schunk, dparams

                WeakKeyPair(L, -1, 1);

                return 1;
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

        const blosc2_schunk * schunk = Get(L);

        if (strcmp(str, "nbytes") == 0) PushInt64OrError(L, schunk->nbytes); // schunk, "nbytes", nbytes
        else if (strcmp(str, "cbytes") == 0) PushInt64OrError(L, schunk->cbytes); // schunk, "cbytes", cbytes
        // TODO? many more...
		else return 0;

		return 1;
	});
}

//
//
//

void WrapSchunk (lua_State * L, blosc2_schunk * schunk)
{
    LuaXS::NewTyped<blosc2_schunk *>(L, schunk); // ..., schunk
	LuaXS::AttachMethods(L, SCHUNK_METATABLE_NAME, Methods);
}

//
//
//

void AddSchunk (lua_State * L)
{
	luaL_Reg funcs[] = {
		{
			"schunk_new", [](lua_State * L)
			{
                blosc2_storage storage = GetStorage(L, 1);
                blosc2_schunk * schunk = blosc2_schunk_new(&storage);

                if (schunk) WrapSchunk(L, schunk); // [storage, ]schunk
                else lua_pushnil(L); // [storage, ]nil

				return 1;
			}
		}, {
            "schunk_open", [](lua_State * L)
            {
                blosc2_schunk * schunk = blosc2_schunk_open(luaL_checkstring(L, 1));

                if (schunk) WrapSchunk(L, schunk); // urlpath, schunk
                else lua_pushnil(L); // urlpath, nil

				return 1;
            }
        }, {
            "schunk_open_offset", [](lua_State * L)
            {
                blosc2_schunk * schunk = blosc2_schunk_open_offset(luaL_checkstring(L, 1), ReadInt64(L, 2));

                if (schunk) WrapSchunk(L, schunk); // urlpath, offset, schunk
                else lua_pushnil(L); // urlpath, offset, nil

				return 1;
            }
        },
		{ nullptr, nullptr }
	};

	luaL_register(L, nullptr, funcs);
}