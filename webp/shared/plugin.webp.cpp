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
#include "src/webp/decode.h"
#include <stddef.h>
#include <string.h>

static void SetColorspace (lua_State * L, WEBP_CSP_MODE & mode)
{
    const char * const names[] = { "RGB", "RGBA", "BGR", "BGRA", "ARGB", "rgbA", "bgrA", "Argb", nullptr };
    WEBP_CSP_MODE modes[] = { MODE_RGB, MODE_RGBA, MODE_BGR, MODE_BGRA, MODE_ARGB, MODE_rgbA, MODE_bgrA, MODE_Argb };

    mode = modes[luaL_checkoption(L, -1, nullptr, names)];
}

static void SetOption (lua_State * L, int arg, WebPDecoderConfig & config)
{
    if (lua_isstring(L, arg))
    {
        if (strcmp(lua_tostring(L, arg), "colorspace") == 0) SetColorspace(L, config.output.colorspace);

        else
        {
            const char * const names[] = {
                "bypass_filtering", "flip", "no_fancy_upsampling", "use_cropping", "use_scaling", "use_threads",// booleans: 0-5
                "crop_left", "crop_top", // non-negative: 6-7
                "crop_width", "crop_height", "scaled_width", "scaled_height", // positive: 8-11
                "dithering_strength", "alpha_dithering_strength", // percentage: 12-13
                nullptr
            };

            #define POINT_TO(member) &WebPDecoderOptions::member

            int WebPDecoderOptions::* const members[] = {
                POINT_TO(bypass_filtering), POINT_TO(flip), POINT_TO(no_fancy_upsampling), POINT_TO(use_cropping), POINT_TO(use_scaling), POINT_TO(use_threads),
                POINT_TO(crop_left), POINT_TO(crop_top),
                POINT_TO(crop_width), POINT_TO(crop_height), POINT_TO(scaled_width), POINT_TO(scaled_height),
                POINT_TO(dithering_strength), POINT_TO(alpha_dithering_strength)
            };

            #undef POINT_TO

            int index = luaL_checkoption(L, arg, nullptr, names), & opt = config.options.*members[index];

            if (index <= 5) opt = lua_toboolean(L, -1);

            else
            {
                opt = luaL_checkint(L, -1);

                if (index >= 8 && index <= 11) luaL_argcheck(L, opt > 0, -1, "Width or height must be positive");
                else luaL_argcheck(L, opt >= 0, -1, "Crop corner coordinate or dithering strength must be >= 0");

                luaL_argcheck(L, index < 12 || opt <= 100, -1, "Dithering strength must be <= 100");
            }
        }
    }
}

static int AddError (lua_State * L, const char * error = nullptr)
{
    if (error) lua_pushstring(L, error);// ..., err

    lua_pushboolean(L, 0);  // ..., err, false
    lua_insert(L, -2);  // ..., false, err

    return 2;
}

static bool CheckCode (lua_State * L, VP8StatusCode code)
{
    if (VP8_STATUS_OK == code) return true;

    switch (code)
    {
    case VP8_STATUS_OK:
        return true;
    case VP8_STATUS_OUT_OF_MEMORY:
        lua_pushliteral(L, "Out of memory");
        break;
    case VP8_STATUS_INVALID_PARAM:
        lua_pushliteral(L, "Invalid param");
        break;
    case VP8_STATUS_BITSTREAM_ERROR:
        lua_pushliteral(L, "Bitstream error");
        break;
    case VP8_STATUS_UNSUPPORTED_FEATURE:
        lua_pushliteral(L, "Unsupported feature");
        break;
    case VP8_STATUS_SUSPENDED:
        lua_pushliteral(L, "Suspended");
        break;
    case VP8_STATUS_USER_ABORT:
        lua_pushliteral(L, "User abort");
        break;
    case VP8_STATUS_NOT_ENOUGH_DATA:
        lua_pushliteral(L, "Not enough data");
        break;
    }

    return false;   // ..., err
}

CORONA_EXPORT int luaopen_plugin_webp (lua_State* L)
{
	lua_newtable(L);// webp

    luaL_Reg webp_funcs[] = {
        {
            "GetInfoFromMemory", [](lua_State * L)
            {
                ByteReader bytes{L, 1};

                int w, h, ok = WebPGetInfo(static_cast<const uint8_t *>(bytes.mBytes), bytes.mCount, &w, &h);

                if (ok)
                {
                    lua_pushinteger(L, w);  // bytes, w
                    lua_pushinteger(L, h);  // bytes, w, h
                }

                else lua_pushnil(L); // bytes, nil

                return 1 + ok;
            }
        },
        { nullptr, nullptr }
    };

    luaL_register(L, nullptr, webp_funcs);

    WebPDecoderConfig * config = (WebPDecoderConfig *)lua_newuserdata(L, sizeof(WebPDecoderConfig));// webp, config

    WebPInitDecoderConfig(config);

    config->output.colorspace = WEBP_CSP_MODE::MODE_rgbA;
    config->output.is_external_memory = 1;

    lua_pushvalue(L, -1);   // webp, config, config
    lua_pushcclosure(L, [](lua_State * L) {
        lua_settop(L, 2);   // input, output

        ByteReader input{L, 1}; // input, output[, err]

        if (!input.mBytes) return AddError(L);  // input, output, false, err

        int w, h;

        if (!WebPGetInfo(static_cast<const uint8_t *>(input.mBytes), input.mCount, &w, &h))
        {
            lua_pushliteral(L, "Unable to get info");   // input, output, err

            return AddError(L); // input, output, false, err
        }
        
        ByteReaderOpts opts;

        lua_pushvalue(L, lua_upvalueindex(1));  // input, output, config

        WebPDecoderConfig * config = (WebPDecoderConfig *)lua_touserdata(L, -1);
        size_t ncomps = WebPIsAlphaMode(config->output.colorspace) ? 4U : 3U;

        ByteReaderWriterMultipleSized output{L, 2, {size_t(w), size_t(h), ncomps}, ByteReaderOpts{}.SetGetStrides(true)};   // input, output, config[, err]

        if (!output.mBytes) return AddError(L); // input, output, config, false, err

        int stride = (!output.mStrides.empty() && output.mStrides.front() > 0U) ? int(output.mStrides.front()) : w * ncomps;
        const uint8_t * out = static_cast<const uint8_t *>(output.mBytes);

        config->output.u.RGBA.rgba = const_cast<uint8_t *>(out);
        config->output.u.RGBA.size = output.mCount;
        config->output.u.RGBA.stride = int(output.mStrides.front());

        bool ok = CheckCode(L, WebPDecode(static_cast<const uint8_t *>(input.mBytes), input.mCount, config)); // input, output, config[, err]

        WebPFreeDecBuffer(&config->output);

        if (!ok) return AddError(L);// input, output, config, false, err

        lua_pushboolean(L, 1); // input, output, config, true

        return 1;
    }, 1); // webp, config, LoadWebP
    lua_pushcclosure(L, [](lua_State * L) {
        lua_pushvalue(L, lua_upvalueindex(1));  // LoadWebP

        return 1;
    }, 1);  // webp, config, GetLoader
    lua_setfield(L, -2, "GetLoader");   // webp = { GetLoader = GetLoader }
    lua_pushcclosure(L, [](lua_State * L) {
        lua_pushvalue(L, lua_upvalueindex(1));  // opts / key?[, value?], config

        WebPDecoderConfig * config = (WebPDecoderConfig *)lua_touserdata(L, -1), temp = *config;

        lua_insert(L, 1);   // config, opts / key?[, value?]

        if (lua_istable(L, 2))
        {
            for (lua_pushnil(L); lua_next(L, 2) != 0; lua_pop(L, 1)) SetOption(L, 2, temp);
        }

        else
        {
            lua_settop(L, 3);   // config, key?, value?

            SetOption(L, 2, temp);
        }

        *config = temp;

        return 0;
    }, 1);  // webp, config, UpdateConfig
    lua_setfield(L, -2, "UpdateConfig");// webp = { GetLoader, UpdateConfig = UpdateConfig }

	return 1;
}