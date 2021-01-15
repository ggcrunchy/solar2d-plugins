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
#include "CoronaGraphics.h"
#include "theoraplay.h"
#include "video_encoder.h"
#include <vector>

struct TheoraReader {
	THEORAPLAY_Decoder * mDecoder{nullptr};
	const THEORAPLAY_AudioPacket * mAudio{nullptr};
	const THEORAPLAY_VideoFrame * mVideo{nullptr};
	THEORAPLAY_VideoFormat mFormat{THEORAPLAY_VIDFMT_RGBA};
	unsigned char mBlank[4] = {};
	unsigned int mElapsed{0U}, mFrameMS{UINT_MAX};
	bool mIsPaused{false};

	~TheoraReader ()
	{
		THEORAPLAY_stopDecode(mDecoder);
		THEORAPLAY_freeAudio(mAudio);
		THEORAPLAY_freeVideo(mVideo);
	}
};

struct TheoraWriter : public VideoEncoder {
	int mWidth, mHeight;

	TheoraWriter (int width, int height) : VideoEncoder{ width, height }, mWidth{width}, mHeight{height}
	{
	}

	virtual ~TheoraWriter ()
	{
	}
};

static unsigned int Theora_GetW (void * context)
{
	TheoraReader * player = static_cast<TheoraReader *>(context);

	// unfortunately for YV12 / IYUV these are 1-channel but not a mask...
	// "rg" would be enough...

	return player->mVideo ? player->mVideo->width : 1U;
}

static unsigned int Theora_GetH (void * context)
{
	TheoraReader * player = static_cast<TheoraReader *>(context);

	if (player->mVideo)
	{
		unsigned int h = player->mVideo->height;

		if (player->mFormat == THEORAPLAY_VIDFMT_YV12 || player->mFormat == THEORAPLAY_VIDFMT_IYUV) h += h / 2U;

		return h;
	}
	
	else return 1U;
}

static const void * Theora_GetData (void * context)
{
	TheoraReader * player = static_cast<TheoraReader *>(context);

	if (player->mVideo) return player->mVideo->pixels;
		
	else return player->mBlank; // at least one blank pixel
}

static void Theora_Cleanup (void * context)
{
	TheoraReader * player = static_cast<TheoraReader *>(context);

	//
}

static CoronaExternalBitmapFormat Theora_Format (void * context)
{
	TheoraReader * player = static_cast<TheoraReader *>(context);

	// not very accurate... we want "masks" for iyuv / yv12, but without that behavior

	return player->mFormat == THEORAPLAY_VIDFMT_RGBA ? kExternalBitmapFormat_RGBA : kExternalBitmapFormat_RGB;
}

static void Theora_Dispose (void * context)
{
	delete static_cast<TheoraReader *>(context);
}

static int PushCachedFunction ( lua_State * L, lua_CFunction f )
{
	// check cache for the funciton, cache key is function address
	lua_pushlightuserdata(L, (void *)f);
	lua_gettable(L, LUA_REGISTRYINDEX);

	// cahce miss
	if (!lua_iscfunction(L, -1))
	{
		lua_pop(L, 1); // pop nil on top of stack

		// create c function closure on top of stack
		lua_pushcfunction(L, f);

		// push cache key
		lua_pushlightuserdata(L, (void *)f);
		// copy function to be on top of stack as well
		lua_pushvalue(L, -2);
		lua_settable(L, LUA_REGISTRYINDEX);

		// now original function is on top of stack, and cache key and function is in cache
	}

	return 1;
}

static int Step (lua_State * L)
{
	TheoraReader * player = static_cast<TheoraReader *>(CoronaExternalGetUserData(L, 1));

	if (!player->mIsPaused && THEORAPLAY_isDecoding(player->mDecoder))
	{
		unsigned int step = luaL_checkinteger(L, 2);

		if (!player->mAudio) player->mAudio = THEORAPLAY_getAudio(player->mDecoder);
		if (!player->mVideo) player->mVideo = THEORAPLAY_getVideo(player->mDecoder);

		if ((player->mAudio || player->mVideo) && step > 0U) // ??
		{
			unsigned int now = player->mElapsed + step;

			// audio currently not very functional :P

			if (player->mVideo)
			{
				if (player->mFrameMS == UINT_MAX) player->mFrameMS = (unsigned int)(1000.0 / player->mVideo->fps);

				while (now >= player->mVideo->playms + player->mFrameMS)
				{
					const THEORAPLAY_VideoFrame * next_video = THEORAPLAY_getVideo(player->mDecoder);

					if (!next_video) break;

					THEORAPLAY_freeVideo(player->mVideo);

					player->mVideo = next_video;
				}
			}

			player->mElapsed = now;
		}
	}
//	if (player->mVideo) CoronaLog("ms: %u, elapsed: %u", player->mVideo->playms, player->mElapsed);
	return 0;
}

static int Pause (lua_State * L)
{
	TheoraReader * player = static_cast<TheoraReader *>(CoronaExternalGetUserData(L, 1));

	player->mIsPaused = lua_toboolean(L, 2) != 0;

	return 0;
}

static void AvailableAudio (lua_State * L, TheoraReader * player)
{
	lua_pushinteger(L, THEORAPLAY_availableAudio(player->mDecoder));
}

static void AvailableVideo (lua_State * L, TheoraReader * player)
{
	lua_pushinteger(L, THEORAPLAY_availableVideo(player->mDecoder));
}

static void Elapsed (lua_State * L, TheoraReader * player)
{
	lua_pushinteger(L, player->mElapsed);
}

static void HasAudioStream (lua_State * L, TheoraReader * player)
{
	lua_pushboolean(L, THEORAPLAY_hasAudioStream(player->mDecoder));
}

static void HasVideoStream (lua_State * L, TheoraReader * player)
{
	lua_pushboolean(L, THEORAPLAY_hasVideoStream(player->mDecoder));
}

static void IsPaused (lua_State * L, TheoraReader * player)
{
	lua_pushboolean(L, player->mIsPaused ? 1 : 0);
}

static void PushFormat (lua_State * L, TheoraReader * player)
{
	switch (player->mFormat)
	{
	case THEORAPLAY_VIDFMT_IYUV:
		lua_pushliteral(L, "IYUV");
		break;
	case THEORAPLAY_VIDFMT_YV12:
		lua_pushliteral(L, "YV12");
		break;
	case THEORAPLAY_VIDFMT_RGB:
		lua_pushliteral(L, "RGB");
		break;
	default:
		lua_pushliteral(L, "RGBA");
		break;
	}
}

static int Theora_GetField (lua_State * L, const char * field, void * context)
{
	int res = 1;

	if (strcmp(field, "Step") == 0) res = PushCachedFunction(L, Step);
	else if (strcmp(field, "Pause") == 0) res = PushCachedFunction(L, Pause);
	else if (strcmp(field, "availableAudio") == 0) AvailableAudio(L, static_cast<TheoraReader *>(context));
	else if (strcmp(field, "availableVideo") == 0) AvailableVideo(L, static_cast<TheoraReader *>(context));
	else if (strcmp(field, "elapsed") == 0) Elapsed(L, static_cast<TheoraReader *>(context));
	else if (strcmp(field, "format") == 0) PushFormat(L, static_cast<TheoraReader *>(context));
	else if (strcmp(field, "hasAudio") == 0) HasAudioStream(L, static_cast<TheoraReader *>(context));
	else if (strcmp(field, "hasVideo") == 0) HasVideoStream(L, static_cast<TheoraReader *>(context));
	else if (strcmp(field, "paused") == 0) IsPaused(L, static_cast<TheoraReader *>(context));
	else res = 0;

	return res;
}

static bool ResolveFile (lua_State * L)
{
	int has_base_dir = 0;

	if (lua_istable(L, 2))
	{
		lua_getfield(L, 2, "baseDir");	// file, opts, ..., baseDir?

		has_base_dir = !lua_isnil(L, -1);

		if (has_base_dir) lua_replace(L, 2);// file, baseDir, ...
	}

	int count = has_base_dir + 1;

	lua_settop(L, count);	// file[, baseDir]
	lua_pushvalue(L, lua_upvalueindex(1));	// file[, baseDir], system.pathForFile
	lua_insert(L, 1);	// system.pathForFile, file[, baseDir]

	if (lua_pcall(L, count, 1, 0) != 0)	// file / err
	{
		lua_pushnil(L);	// err, nil
		lua_insert(L, 1);	// nil, err

		return false;
	}

	return true;
}

static TheoraWriter * Writer (lua_State * L)
{
	return (TheoraWriter *)luaL_checkudata(L, 1, "theora.writer");
}

const unsigned char * GetBytes (lua_State * L, std::vector<unsigned char> & fallback, int width, int height)
{
	const char * str = lua_tostring(L, 2);
	size_t needed = size_t(width * height * 3);
	size_t size = lua_objlen(L, 2);

	if (size >= needed) return reinterpret_cast<const unsigned char *>(str);

	else
	{
		fallback.resize(needed);

		memcpy(fallback.data(), str, size);
		memset(fallback.data() + size, 0, size - needed);

		return fallback.data();
	}
}

CORONA_EXPORT int luaopen_plugin_theora (lua_State * L)
{
	lua_getglobal(L, "system");	// system
	lua_newtable(L);// system, theora
	lua_getfield(L, -2, "pathForFile");	// system, theora, system.pathForFile
	lua_pushcclosure(L, [](lua_State * L) {
		unsigned int max_frames = 30;
		THEORAPLAY_VideoFormat vformat = THEORAPLAY_VIDFMT_RGBA;

		if (lua_istable(L, 2))
		{
			lua_getfield(L, 2, "format");	// file, opts, format?
			lua_getfield(L, 2, "maxFrames");// file, opts, format?, maxFrames?

			if (!lua_isnil(L, -3))
			{
				const char * fnames[] = {"YV12", "IYUV", "RGB", "RGBA", nullptr};
				THEORAPLAY_VideoFormat formats[] = {THEORAPLAY_VIDFMT_YV12, THEORAPLAY_VIDFMT_IYUV, THEORAPLAY_VIDFMT_RGB, THEORAPLAY_VIDFMT_RGBA};

				vformat = formats[luaL_checkoption(L, -2, nullptr, fnames)];
			}

			if (!lua_isnil(L, -2)) max_frames = luaL_checkinteger(L, -1);
		}

		if (!ResolveFile(L)) return 2; // file / nil[, err]

		TheoraReader * player = new TheoraReader;

		player->mDecoder = THEORAPLAY_startDecodeFile(lua_tostring(L, 1), max_frames, vformat);

		if (!player->mDecoder)
		{
			delete player;

			lua_pushnil(L);	// file, nil
			lua_pushliteral(L, "Unable to start Theora file decode");	// file, nil, err

			return 2;
		}

		CoronaExternalTextureCallbacks callbacks = {};

		callbacks.size = sizeof(CoronaExternalTextureCallbacks);
		callbacks.getFormat = Theora_Format;
		callbacks.getHeight = Theora_GetH;
		callbacks.getWidth = Theora_GetW;
		callbacks.onFinalize = Theora_Dispose;
		callbacks.onGetField = Theora_GetField;
		callbacks.onReleaseBitmap = Theora_Cleanup;
		callbacks.onRequestBitmap = Theora_GetData;

		if (!CoronaExternalPushTexture(L, &callbacks, player))	// file, player
		{
			delete player;

			lua_pushnil(L);	// file, tp, nil
			lua_pushliteral(L, "Unable to set up TheoraPlay callbacks");// file, tp, nil, err

			return 2;
		}

		return 1;
	}, 1);	// system, theora, DecodeFromFile
	lua_setfield(L, -2, "decodeFromFile");	// system, theora = { decodeFromFile = DecodeFromFile }
	lua_getfield(L, -2, "pathForFile");	// system, theora, system.pathForFile
	lua_pushcclosure(L, [](lua_State * L) {
		luaL_checktype(L, 1, LUA_TTABLE);
		lua_getfield(L, 1, "filename");	// params, filename
		luaL_checktype(L, -1, LUA_TSTRING);
		lua_insert(L, 1);	// filename, params

		lua_getfield(L, 2, "width");// filename, params, width
		lua_getfield(L, 2, "height");	// filename, params, width, height
		lua_getfield(L, 2, "frameRate");// filename, params, width, height, frameRate?
		lua_getfield(L, 2, "keyFrameInterval");	// filename, params, width, height, frameRate?, keyFrameInterval?
		lua_getfield(L, 2, "quality");	// filename, params, width, height, frameRate?, keyFrameInterval?, quality?

		int width = luaL_checkint(L, -5), height = luaL_checkint(L, -4);
		int frameRate = luaL_optint(L, -3, -1), keyFrameInterval = luaL_optint(L, -2, -1), quality = luaL_optint(L, -1, -1);

		if (!ResolveFile(L)) return 2; // file / nil[, err]

		const char * file = luaL_checkstring(L, 1);

		TheoraWriter * writer = (TheoraWriter *)lua_newuserdata(L, sizeof(TheoraWriter));	// file, writer

		new (writer) TheoraWriter{width, height};

		writer->setOutputFile(file);

		if (frameRate > 0) writer->setFrameRate(frameRate);
		if (keyFrameInterval > 0) writer->setKeyFrameInterval(keyFrameInterval);
		if (quality > 0) writer->setQuality(quality);

		if (luaL_newmetatable(L, "theora.writer"))	// file, writer, meta
		{
			luaL_Reg funcs[] = {
				{
					"commit", [](lua_State * L)
					{
						Writer(L)->end();

						return 0;
					}
				}, {
					"dupFrame", [](lua_State * L)
					{
						luaL_checkstring(L, 2);

						int time = luaL_checkint(L, 3), ok = 1;

						try {
							std::vector<unsigned char> fallback;

							TheoraWriter * writer = Writer(L);
							const unsigned char * bytes = GetBytes(L, fallback, writer->mWidth, writer->mHeight);

							writer->dupFrame(bytes, time);
						} catch (const char * error) {
							lua_pushstring(L, error);	// writer, data, time, error

							ok = 0;
						}

						if (!ok) return lua_error(L);

						return 0;
					}
				}, {
					"__gc", [](lua_State * L)
					{
						Writer(L)->~TheoraWriter();

						return 0;
					}
				}, {
					"newFrame", [](lua_State * L)
					{
						luaL_checkstring(L, 2);

						int ok = 1;

						try {
							std::vector<unsigned char> fallback;

							TheoraWriter * writer = Writer(L);
							const unsigned char * bytes = GetBytes(L, fallback, writer->mWidth, writer->mHeight);

							writer->newFrame(bytes);
						} catch (const char * error) {
							lua_pushstring(L, error);	// writer, data, error

							ok = 0;
						}

						if (!ok) return lua_error(L);

						return 0;
					}
				},
				{ nullptr, nullptr }
			};

			luaL_register(L, nullptr, funcs);
			lua_pushvalue(L, -1);	// file, writer, meta, meta
			lua_setfield(L, -2, "__index");	// file, writer, meta = { ..., __index = meta }
		}

		lua_setmetatable(L, -2);// path, params, writer; writer.metatable = meta

		return 1;
	}, 1);	// system, theora, meta, NewWriter
	lua_setfield(L, -2, "newWriter"); // system, theora = { ..., newWriter = NewWriter }

	return 1;
}
