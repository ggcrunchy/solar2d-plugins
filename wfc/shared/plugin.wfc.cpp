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
#include "ByteReader.h"
#include "utils/Blob.h"

#define WFC_IMPLEMENTATION
#include "wfc.h"

#define WFC_METATABLE_NAME "metatable.wfc"

struct wfc_context {
    struct wfc * mWFC{nullptr};
    struct wfc_image * mOutput{nullptr};
    struct wfc_image mInput;
    bool mRanOK{false};
    bool mHasRun{false};
};

static wfc_context * Get (lua_State * L)
{
    return (wfc_context *)luaL_checkudata(L, 1, WFC_METATABLE_NAME);
}

static void AddMetatable (lua_State * L)
{
    if (!luaL_newmetatable(L, WFC_METATABLE_NAME)) return; // ..., mt

    luaL_Reg methods[] = {
        {
            "close", [](lua_State * L)
            {
                wfc_context * WFC = Get(L);
                
                if (WFC->mWFC)
                {
                    wfc_destroy(WFC->mWFC);
                    
                    WFC->mWFC = nullptr;
                    
                    lua_pushnil(L); // wfc, nil
                    lua_setfenv(L, 1); // wfc; wfc.env = nil
                }
                
                return 0;
            }
        }, {
            "destroy_output_image", [](lua_State * L)
            {
                wfc_context * WFC = Get(L);
                
                if (WFC->mOutput)
                {
                    wfc_img_destroy(WFC->mOutput);
                    
                    WFC->mOutput = nullptr;
                }
                
                return 0;
            }
        }, {
            "__gc", [](lua_State * L)
            {
                wfc_context * WFC = Get(L);
               
                if (WFC->mWFC) wfc_destroy(WFC->mWFC);
                if (WFC->mOutput) wfc_img_destroy(WFC->mOutput);
                
                return 0;
            }
        }, {
            "get_output_image", [](lua_State * L)
            {
                wfc_context * WFC = Get(L);
                wfc_image output = {};
                
                if (!WFC->mOutput)
                {
                    if (WFC->mWFC && WFC->mRanOK)
                    {
                        wfc_image image = {};
                        const char * err = nullptr;
                        
                        if (BlobXS::IsBlob(L, 2))
                        {
                            size_t size = BlobXS::GetSize(L, 2);
                            int count = WFC->mWFC->cell_cnt * WFC->mInput.component_cnt;

                            if (BlobXS::IsLocked(L, 2)) err = "cannot send output to locked blob";
                            else if (size < count)
                            {
                                if (!BlobXS::IsResizable(L, 2)) err = "fixed-size blob is too small";
                                else if (!BlobXS::Resize(L, 2, count)) err = "unable to resize blob";
                            }

                            if (!err)
                            {
                                image.data = BlobXS::GetData(L, 2);
                                image.width = WFC->mWFC->output_width;
                                image.height = WFC->mWFC->output_height;
                                image.component_cnt = WFC->mInput.component_cnt;
                            }
                        }

                        wfc_image * result = !err ? wfc_output_image(WFC->mWFC, image.data ? &image : nullptr) : nullptr;

                        if (!result)
                        {
                            lua_pushnil(L); // wfc, nil
                            lua_pushfstring(L, "Failed to create output image (%s)", err ? err : "allocation"); // wfc, nil, error
                            
                            return 2;
                        }

                       if (!image.data) WFC->mOutput = result;

                       output = *result;
                    }
                    
                    else
                    {
                        lua_pushnil(L); // wfc, nil
                        lua_pushstring(L, WFC->mRanOK ? "Unable to create image from closed WFC context" : "WFC has not run successfully"); // wfc, nil, error
                        
                        return 2;
                    }
                }

                if (WFC->mOutput) lua_pushlstring(L, reinterpret_cast<const char *>(output.data), size_t(output.width * output.height * output.component_cnt)); // wfc, output
                else lua_pushboolean(L, 1); // wfc, ok

                lua_pushinteger(L, output.width); // wfc, output / ok, width
                lua_pushinteger(L, output.height); // wfc, output / ok, width, height
                lua_pushinteger(L, output.component_cnt); // wfc, output / ok, width, height, component_count
                
                return 4;
            }
        }, {
            "is_ready", [](lua_State * L)
            {
                wfc_context * WFC = Get(L);
                
                lua_pushboolean(L, WFC->mWFC && WFC->mRanOK); // wfc, ok
                
                return 1;
            }
        }, {
            "run", [](lua_State * L)
            {
                wfc_context * WFC = Get(L);
                
                int max_collapse_count = -1;
                unsigned int seed = time(nullptr);
                
                if (lua_istable(L, 2))
                {
                    lua_getfield(L, 1, "max_collapse_count"); // wfc, options, max_collapse_count
                    lua_getfield(L, 1, "seed"); // wfc, options, max_collapse_count, seed
                    
                    max_collapse_count = luaL_optint(L, -2, max_collapse_count);
                    seed = luaL_optint(L, -1, seed);
                    
                    lua_pop(L, 2); // wfc, options
                }
                
                int ok = false;
                
                if (WFC->mWFC)
                {
                    ok = WFC->mRanOK;
                    
                    if (!ok)
                    {
                        wfc_srand(WFC->mWFC, 0, seed);
                        
                        if (WFC->mHasRun) wfc_init(WFC->mWFC);
                        
                        ok = wfc_run(WFC->mWFC, max_collapse_count);
                        
                        WFC->mRanOK = ok;
                        WFC->mHasRun = true;
                    }
                }
                
                lua_pushboolean(L, ok); // wfc, ok
                
                return 1;
            }
        },
        { nullptr, nullptr }
    };

    luaL_register(L, nullptr, methods);
    lua_pushvalue(L, -1); // ..., mt, mt
    lua_setfield(L, -2, "__index"); // ..., mt; mt.__index = mt
}

static struct wfc_image GetImageDetails (lua_State * L)
{
    struct wfc_image image = {};

    lua_settop(L, 1); // params
    luaL_checktype(L, 1, LUA_TTABLE);
    lua_getfield(L, 1, "component_count"); // params, component_count
    lua_getfield(L, 1, "width"); // params, component_count, width
    lua_getfield(L, 1, "height"); // params, component_count, height
    
    image.height = luaL_checkint(L, -1);
    image.width = luaL_checkint(L, -2);
    image.component_cnt = luaL_checkint(L, -3);
    
    luaL_argcheck(L, -3, image.component_cnt > 0, "Non-positive component count");
    luaL_argcheck(L, -2, image.width > 0, "Non-positive input image width");
    luaL_argcheck(L, -1, image.height > 0, "Non-positive input image height");
    lua_pop(L, 3); // params
    lua_getfield(L, 1, "data"); // params, data?

    ByteReader bytes{ L, -1 };

    if (bytes.mBytes)
    {
        image.data = (unsigned char *)bytes.mBytes;

        luaL_argcheck(L, -1, image.width * image.height * image.component_cnt <= bytes.mCount, "Inadequate data");
    }

    return image;
}

static void LoadModule (lua_State * L)
{
    lua_newtable(L); // wfc

    luaL_Reg funcs[] = {
        {
            "overlapping", [](lua_State * L)
            {
                struct wfc_image image = GetImageDetails(L); // params, data?
                
                if (!image.data)
                {
                    lua_pushliteral(L, "Data missing"); // params, nil, "Data missing"
                    
                    return 2;
                }
                
                wfc_context * context = (wfc_context *)lua_newuserdata(L, sizeof(wfc_context)); // params, data, context
                
                *context = wfc_context{};
                
                int output_width, output_height, tile_width, tile_height;
                
                lua_getfield(L, 1, "output_width"); // params, data, context, output_width
                lua_getfield(L, 1, "output_height"); // params, data, context, output_width, output_height
                lua_getfield(L, 1, "tile_width"); // params, data, context, output_width, output_height, tile_width
                lua_getfield(L, 1, "tile_height"); // params, data, context, output_width, output_height, tile_width, tile_height
                
                tile_height = luaL_checkint(L, -1);
                tile_width = luaL_checkint(L, -2);
                output_height = luaL_checkint(L, -3);
                output_width = luaL_checkint(L, -4);
                
                luaL_argcheck(L, -4, output_width > 0, "Non-positive output image width");
                luaL_argcheck(L, -3, output_height > 0, "Non-positive output image height");
                luaL_argcheck(L, -2, tile_width > 0, "Non-positive tile width");
                luaL_argcheck(L, -1, tile_height > 0, "Non-positive tile height");
                lua_pop(L, 4); // params, data, context
                
                const char * bool_names[] = { "expand_input", "xflip_tiles", "yflip_tiles", "rotate_tiles" };
                
                int bools[4] = {};
                
                for (int i = 0; i < 4; ++i)
                {
                    lua_getfield(L, 1, bool_names[i]); // params, data, context, bool
                    
                    bools[i] = lua_toboolean(L, -1);
                    
                    lua_pop(L, 1); // params, data, context
                }
                
                lua_insert(L, -2); // params, context, data
                lua_setfenv(L, -2); // params, context; context.env = data

                context->mInput = image;
                context->mWFC = wfc_overlapping(output_width, output_height, &context->mInput, tile_width, tile_height, bools[0], bools[1], bools[2], bools[3]);
                
                if (!context->mWFC)
                {
                    lua_pushnil(L); // params, context, nil
                    lua_pushliteral(L, "Failed to create overlapping WFC context"); // params, context, nil, error
                    
                    return 2;
                }
                
                AddMetatable(L); // params, context, wfc_mt

                lua_setmetatable(L, -2); // params, context; context.metatable = wfc_mt
                
                return 1;
            }
        },
        { nullptr, nullptr }
    };
    
    luaL_register( L, nullptr, funcs );
}

CORONA_EXPORT int luaopen_plugin_wfc (lua_State* L)
{
    LoadModule(L); // wfc

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
