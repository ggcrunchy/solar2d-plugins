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
#include "utils/LuaEx.h"
#include "openpnp-capture.h"

#define MINIAUDIO_IMPLEMENTATION
#include "miniaudio.h"

//
//
//

#define AUDIO_NAME(name) "audioRecorder." #name
#define VIDEO_NAME(name) "videoRecorder." #name
//
//
//

static int sPathForFileRef, sDocumentsDirectoryRef, sTemporaryDirectoryRef;

//
//
//

struct AudioRecorder {
    ma_device mDevice;
    ma_encoder mEncoder;
    bool mStopped{false};
};

struct StreamAndSize {
    CapStream mStream; // first for most uses
    size_t mSize; // for captures
};

//
//
//

static CapContext sCapContext;

static CapContext GetCapContext (lua_State * L)
{
    if (!sCapContext)
    {
        sCapContext = Cap_createContext();

        LuaXS::AddCloseLogic(L, [](lua_State * L) {
            Cap_releaseContext(sCapContext);

            sCapContext = nullptr;

            return 0;
        });
    }

    return sCapContext;
}

//
//
//

#define CAP_RESULT_ERROR(NAME) CAPRESULT_##NAME: lua_pushliteral(L, #NAME); break

static int ErrorResult (lua_State * L, CapResult result)
{
    lua_pushboolean(L, CAPRESULT_OK == result); // ..., ok

    switch (result)
    {
    case CAPRESULT_OK:
        return 1;
    case CAP_RESULT_ERROR(ERR);
    case CAP_RESULT_ERROR(DEVICENOTFOUND);
    case CAP_RESULT_ERROR(FORMATNOTSUPPORTED);
    case CAP_RESULT_ERROR(PROPERTYNOTSUPPORTED);
    default:
        lua_pushliteral(L, "UNKNOWN");
    }

    return 2;
}

//
//
//

static int StringResult (lua_State * L, const char * str)
{
    if (str) lua_pushstring(L, str); // ..., str
    else lua_pushnil(L); // ..., nil

    return 1;
}

//
//
//

static StreamAndSize * GetStreamAndSize (lua_State * L)
{
    return LuaXS::CheckUD<StreamAndSize>(L, 1, VIDEO_NAME(Stream));
}

static CapStream GetStream (lua_State * L)
{
    return GetStreamAndSize(L)->mStream;
}

//
//
//

static int GetProperty (lua_State * L)
{
    const char * names[] = { "EXPOSURE", "FOCUS", "ZOOM", "WHITEBALANCE", "GAIN", "BRIGHTNESS", "CONTRAST", "SATURATION", "GAMMA", "HUE", "SHARPNESS", "BACKLIGHTCOMP", "POWERLINEFREQ", nullptr };

    return luaL_checkoption(L, 2, nullptr, names) + 1;
}

//
//
//

void AddScreenRecorder (lua_State * L)
{
	lua_newtable(L); // ..., winmisc, screenRecorder

    //
    //
    //

    sPathForFileRef = sDocumentsDirectoryRef = sTemporaryDirectoryRef = LUA_NOREF;
    sCapContext = nullptr;

    //
    //
    //

    luaL_Reg funcs[] = {
        {
            "getCameraDeviceCount", [](lua_State * L)
            {
                lua_pushinteger(L, Cap_getDeviceCount(GetCapContext(L))); // device_count

                return 1;
            }
        },
        {
            "getCameraDeviceName", [](lua_State * L)
            {
                return StringResult(L, Cap_getDeviceName(GetCapContext(L), lua_tointeger(L, 1) - 1)); // device_id, device_name?
            }
        },
        {
            "getCameraDeviceUniqueID", [](lua_State * L)
            {
                return StringResult(L, Cap_getDeviceUniqueID(GetCapContext(L), lua_tointeger(L, 1) - 1)); // device_id, unique_id?
            }
        },
        {
            "getCameraFormatInfo", [](lua_State * L)
            {
                CapFormatInfo info;
                CapResult result = Cap_getFormatInfo(GetCapContext(L), lua_tointeger(L, 1) - 1, lua_tointeger(L, 2) - 1, &info);

                if (CAPRESULT_OK == result)
                {
                    lua_createtable(L, 0, 5); // device_id, format_id, info
                    lua_pushinteger(L, info.bpp); // device_id, format_id, info, bpp
                    lua_setfield(L, -2, "bpp"); // device_id, format_id, info = { bpp = bpp }
                    lua_pushinteger(L, info.fps); // device_id, format_id, info, fps
                    lua_setfield(L, -2, "fps"); // device_id, format_id, info = { bpp, fps = fps }
                    lua_pushinteger(L, info.height); // device_id, format_id, info, height
                    lua_setfield(L, -2, "height"); // device_id, format_id, info = { bpp, fps, height = height }
                    lua_pushinteger(L, info.width); // device_id, format_id, info, width
                    lua_setfield(L, -2, "width"); // device_id, format_id, info = { bpp, fps, height, width = width }
                    lua_createtable(L, 0, 2); // device_id, format_id, info, fourcc
                    lua_pushinteger(L, info.fourcc); // device_id, format_id, info, fourcc, fourcc_int
                    lua_setfield(L, -2, "int"); // device_id, format_id, info, info, fourcc = { int = fourcc_int }

                    char fourcc[5] = { 0 };

                    for (int i = 0; i < 4; ++i, info.fourcc >>= 8) fourcc[i] = info.fourcc & 0xFF;

                    lua_pushstring(L, fourcc); // device_id, format_id, info, fourcc, fourcc_str
                    lua_setfield(L, -2, "str"); // device_id, format_id, info, fourcc = { int, str = fourcc_str }
                    lua_setfield(L, -2, "fourcc"); // device_id, format_id, info = { bpp, fps, height, width, fourcc = fourcc }

                    return 1;
                }

                return ErrorResult(L, result); // device_id, format_id, info / false[, error]
            }
        },
        {
            "getCameraNumFormats", [](lua_State * L)
            {
                lua_pushinteger(L, Cap_getNumFormats(GetCapContext(L), lua_tointeger(L, 1) - 1)); // device_id, format_count

                return 1;
            }
        },
        {
            "openCameraStream", [](lua_State * L)
            {
                CapStream id = Cap_openStream(GetCapContext(L), lua_tointeger(L, 1) - 1, lua_tointeger(L, 2) - 1);

                if (id >= 0)
                {
                    StreamAndSize * sas = LuaXS::NewTyped<StreamAndSize>(L); // device_id, format_id, stream

                    sas->mStream = id;

                    LuaXS::AttachMethods(L, VIDEO_NAME(Stream), [](lua_State * L) {
                        luaL_Reg funcs[] = {
                            {
                                "captureFrame", [](lua_State * L)
                                {
                                    StreamAndSize * sas = GetStreamAndSize(L);
                                    std::vector<unsigned char> frame_data;
                                    unsigned char * data;
                                    size_t size;

                                    if (BlobXS::IsBlob(L, 2))
                                    {
                                        data = BlobXS::GetData(L, 2);
                                        size = BlobXS::GetSize(L, 2);

                                        lua_settop(L, 2); // stream, blob
                                    }

                                    else
                                    {
                                        frame_data.resize(sas->mSize);

                                        data = frame_data.data();
                                        size = frame_data.size();
                                    }

                                    Cap_captureFrame(GetCapContext(L), sas->mStream, data, size);

                                    if (!frame_data.empty()) lua_pushlstring(L, reinterpret_cast<const char *>(data), size); // stream, frame_data

                                    return 1;
                                }
                            },
                            {
                                "close", [](lua_State * L)
                                {
                                    Cap_closeStream(GetCapContext(L), GetStream(L));

                                    return 0;
                                }
                            },
                            {
                                "__gc", [](lua_State * L)
                                {CoronaLog("A");
                                    Cap_closeStream(GetCapContext(L), GetStream(L));CoronaLog("B");

                                    return 0;
                                }
                            },
                            {
                                "getAutoProperty", [](lua_State * L)
                                {
                                    uint32_t flag;

                                    CapResult result = Cap_getAutoProperty(GetCapContext(L), GetStream(L), GetProperty(L), &flag);

                                    if (CAPRESULT_OK == result)
                                    {
                                        lua_pushboolean(L, flag); // stream, pname, auto

                                        return 1;
                                    }

                                    else return ErrorResult(L, result); // stream, pname, false, error
                                }
                            },
                            {
                                "getProperty", [](lua_State * L)
                                {
                                    int32_t value;

                                    CapResult result = Cap_getProperty(GetCapContext(L), GetStream(L), GetProperty(L), &value);

                                    if (CAPRESULT_OK == result)
                                    {
                                        lua_pushinteger(L, value); // stream, pname, value

                                        return 1;
                                    }

                                    else return ErrorResult(L, result); // stream, pname, false, error
                                }
                            },
                            {
                                "getPropertyLimits", [](lua_State * L)
                                {
                                    int32_t min, max;
                                    int def;

                                    CapResult result = Cap_getPropertyLimits(GetCapContext(L), GetStream(L), GetProperty(L), &min, &max, &def);

                                    if (CAPRESULT_OK == result)
                                    {
                                        lua_pushinteger(L, min); // stream, pname, value, min
                                        lua_pushinteger(L, max); // stream, pname, value, min, max
                                        lua_pushinteger(L, def); // stream, pname, value, min, max, def

                                        return 3;
                                    }

                                    else return ErrorResult(L, result); // stream, pname, false, error
                                }
                            },
                            {
                                "hasNewFrame", [](lua_State * L)
                                {
                                    lua_pushboolean(L, Cap_hasNewFrame(GetCapContext(L), GetStream(L))); // stream, has_new_frame

                                    return 1;
                                }
                            },
                            {
                                "isOpen", [](lua_State * L)
                                {
                                    lua_pushboolean(L, Cap_isOpenStream(GetCapContext(L), GetStream(L))); // stream, is_open

                                    return 1;
                                }
                            },
                            {
                                "setAutoProperty", [](lua_State * L)
                                {
                                    return ErrorResult(L, Cap_setAutoProperty(GetCapContext(L), GetStream(L), GetProperty(L), lua_toboolean(L, 3))); // stream, pname, on_off, ok[, error]
                                }
                            },
                            {
                                "setProperty", [](lua_State * L)
                                {
                                    return ErrorResult(L, Cap_setProperty(GetCapContext(L), GetStream(L), GetProperty(L), lua_tointeger(L, 3))); // stream, pname, value, ok[, error]
                                }
                            },
                            { nullptr, nullptr }
                        };

                        luaL_register(L, nullptr, funcs);
                    });

                    lua_pushvalue(L, -1); // device_id, format_id, stream, stream
                    lua_pushinteger(L, id); // device_id, format_id, stream, stream, stream_id
                    lua_rawset(L, LUA_REGISTRYINDEX); // device_id, format_id, stream; registry[stream] = stream_id
                }

                else lua_pushnil(L); // device_id, format_id, nil

                return 1;
            }
        },
        {
            "startAudioRecording", [](lua_State * L)
            {
                luaL_checktype(L, 1, LUA_TTABLE);
                lua_getfield(L, 1, "format"); // params, format?

                const char * names[] = { "u8", "s16", "s24", "s32", "f32", nullptr };
                ma_format formats[] = { ma_format_u8, ma_format_s16, ma_format_s24, ma_format_s32, ma_format_f32 };

                ma_format enc_format = formats[luaL_checkoption(L, -1, "f32", names)];

                lua_getfield(L, 1, "channels"); // params, format?, channels?
                lua_getfield(L, 1, "sampleRate"); // params, format?, channels?, sampleRate?

                //
                //
                //

                ma_uint32 channels = luaL_optinteger(L, -2, 2);
                ma_uint32 sample_rate = luaL_optinteger(L, -1, 44100);

                lua_getref(L, sPathForFileRef); // params, format?, channels?, sampleRate?, system.pathForFile?

                if (!lua_isfunction(L, -1))
                {
                    CORONA_LOG_WARNING("Non-function `system.pathForFile()`");

                    return 0;
                }

                lua_getfield(L, 1, "filename"); // params, format?, channels?, sampleRate?, system.pathForFile, filename
                luaL_checkstring(L, -1);
                lua_getfield(L, 1, "baseDir"); // params, format?, channels?, sampleRate?, system.pathForFile, filename, baseDir?
                lua_getref(L, sDocumentsDirectoryRef); // params, format?, channels?, sampleRate?, system.pathForFile, filename, baseDir?, system.DocumentsDirectory?
                luaL_argcheck(L, !lua_isnil(L, -1), -1, "Missing `system.DocumentsDirectory` reference");

                if (lua_isnil(L, -2) || lua_equal(L, -2, -1)) lua_remove(L, -2); // params, format?, channels?, sampleRate?, system.pathForFile, filename, system.DocumentsDirectory

                else
                {
                    lua_getref(L, sTemporaryDirectoryRef); // params, format?, channels?, sampleRate?, system.pathForFile, filename, baseDir, system.DocumentsDirectory, system.TemporaryDirectory?
                    luaL_argcheck(L, lua_equal(L, -3, -1), -1, "Missing `system.TemporaryDirectory` reference or invalid `baseDir` provided");
                    lua_replace(L, -3); // params, format?, channels?, sampleRate?, system.pathForFile, filename, system.TemporaryDirectory, system.DocumentsDirectory
                    lua_pop(L, 1); //  params, format?, channels?, sampleRate?, system.pathForFile, filename, system.TemporaryDirectory
                }

                
                lua_call(L, 2, 1); // params, format?, channels?, sampleRate?, file?

                //
                //
                //

                AudioRecorder * recorder = LuaXS::NewTyped<AudioRecorder>(L); // params, format?, channels?, sampleRate?, file, audio_recorder

                // https://miniaud.io/docs/examples/simple_capture.html
                ma_encoder_config encoder_config = ma_encoder_config_init(ma_encoding_format_wav, enc_format, channels, sample_rate);

                if (ma_encoder_init_file(luaL_checkstring(L, -2), &encoder_config, &recorder->mEncoder) != MA_SUCCESS)
                {
                    lua_pushboolean(L, 0); // params, format?, channels?, sampleRate?, file?, audio_recorder, false
                    lua_pushliteral(L, "Failed to initialize output file.");// params, format?, channels?, sampleRate?, file?, audio_recorder, false, err

                    return 2;
                }

                //
                //
                //

                ma_device_config device_config;

                device_config = ma_device_config_init(ma_device_type_capture);
                device_config.capture.format = recorder->mEncoder.config.format;
                device_config.capture.channels = recorder->mEncoder.config.channels;
                device_config.sampleRate = recorder->mEncoder.config.sampleRate;
                device_config.pUserData = &recorder->mEncoder;
                 
                device_config.dataCallback = [](ma_device * device, void *, const void * input, ma_uint32 frame_count)
                {
                    ma_encoder_write_pcm_frames((ma_encoder *)device->pUserData, input, frame_count, nullptr);
                };

                if (ma_device_init(nullptr, &device_config, &recorder->mDevice) != MA_SUCCESS)
                {
                    lua_pushboolean(L, 0); // params, format?, channels?, sampleRate?, file, audio_recorder, false
                    lua_pushliteral(L, "Failed to initialize capture device.");// params, format?, channels?, sampleRate?, file, audio_recorder, false, err

                    return 2;
                }

                //
                //
                //

                if (ma_device_start(&recorder->mDevice) != MA_SUCCESS)
                {
                    ma_device_uninit(&recorder->mDevice);
                   
                    lua_pushboolean(L, 0); // params, format?, channels?, sampleRate?, file, audio_recorder, false
                    lua_pushliteral(L, "Failed to start device.");// params, format?, channels?, sampleRate?, file, audio_recorder, false, err

                    return 2;
                }

                //
                //
                //

                LuaXS::AttachGC(L, AUDIO_NAME(Recording), [](lua_State * L) {
                    AudioRecorder * recorder = LuaXS::CheckUD<AudioRecorder>(L, 1, AUDIO_NAME(Recording));

                    if (!recorder->mStopped)
                    {
                        ma_device_uninit(&recorder->mDevice);
                        ma_encoder_uninit(&recorder->mEncoder);
                    }

                    return 0;
                });

                lua_pushlightuserdata(L, recorder); // params, format?, channels?, sampleRate?, file, audio_recorder, audio_recorder_ptr
                lua_pushvalue(L, -2); // params, format?, channels?, sampleRate?, file, audio_recorder, audio_recorder_ptr, audio_recorder
                lua_rawset(L, LUA_REGISTRYINDEX); // params, format?, channels?, sampleRate?, file, audio_recorder; registry[audio_recorder_ptr] = audio_recorder

                return 1;
            }
        },
        {
            "stopAudioRecording", [](lua_State * L)
            {
                AudioRecorder * recorder = LuaXS::CheckUD<AudioRecorder>(L, 1, AUDIO_NAME(Recording));

                if (!recorder->mStopped)
                {
                    ma_device_uninit(&recorder->mDevice);
                    ma_encoder_uninit(&recorder->mEncoder);

                    recorder->mStopped = true;

                    lua_pushlightuserdata(L, recorder); // audio_recorder, audio_recorder_ptr
                    lua_pushnil(L); // audio_recorder, audio_recorder_ptr, nil
                    lua_rawset(L, LUA_REGISTRYINDEX); // audio_recorder; registry[audio_recorder_ptr = nil
                }

                return 0;
            }
        },
		{ nullptr, nullptr }
	};

	luaL_register(L, nullptr, funcs);
	lua_setfield(L, -2, "screenRecorder"); // ..., winmisc = { ..., screenRecorder = screenRecorder }
    lua_getglobal(L, "system"); // ..., winmisc, system?

    if (!lua_isnil(L, -1))
    {
        lua_getfield(L, -1, "pathForFile"); // ..., winmisc, system, system.pathForFile
        lua_getfield(L, -2, "DocumentsDirectory"); // ..., winmisc, system, system.pathForFile, system.DocumentsDirectory
        lua_getfield(L, -3, "TemporaryDirectory"); // ..., winmisc, system, system.pathForFile, system.DocumentsDirectory, system.TemporaryDirectory

        sTemporaryDirectoryRef = lua_ref(L, 1); // ..., winmisc, system, system.pathForFile, system.DocumentsDirectory; ref = system.TemporaryDirectory
        sDocumentsDirectoryRef = lua_ref(L, 1); // ..., winmisc, system, system.pathForFile; ref = system.DocumentsDirectory
        sPathForFileRef = lua_ref(L, 1); // ..., winmisc, system; ref = system.pathForFile
    }

    lua_pop(L, 1); // ..., winmisc
}