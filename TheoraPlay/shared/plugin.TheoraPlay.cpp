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
#include <vector>

struct TheoraPlay {
	THEORAPLAY_Decoder * mDecoder{nullptr};
	const THEORAPLAY_AudioPacket * mAudio{nullptr};
	const THEORAPLAY_VideoFrame * mVideo{nullptr};
	THEORAPLAY_VideoFormat mFormat{THEORAPLAY_VIDFMT_RGBA};
	std::vector<unsigned char> mBlank;
	unsigned int mElapsed{0U}, mFrameMS{UINT_MAX};
	bool mIsPaused{false};

	~TheoraPlay ()
	{
		THEORAPLAY_stopDecode(mDecoder);
		THEORAPLAY_freeAudio(mAudio);
		THEORAPLAY_freeVideo(mVideo);
	}
};


static unsigned int TheoraPlay_GetW (void * context)
{
	TheoraPlay * tp = static_cast<TheoraPlay *>(context);

	// unfortunately for YV12 / IYUV these are 1-channel but not a mask...
	// "rg" would be enough...

	return tp->mVideo ? tp->mVideo->width : 1U;
}

static unsigned int TheoraPlay_GetH (void * context)
{
	TheoraPlay * tp = static_cast<TheoraPlay *>(context);

	if (tp->mVideo)
	{
		unsigned int h = tp->mVideo->height;

		if (tp->mFormat == THEORAPLAY_VIDFMT_YV12 || tp->mFormat == THEORAPLAY_VIDFMT_IYUV) h += h / 2U;

		return h;
	}
	
	else return 1U;
}

static const void * TheoraPlay_GetData (void * context)
{
	TheoraPlay * tp = static_cast<TheoraPlay *>(context);

	if (tp->mVideo) return tp->mVideo->pixels;
		
	else
	{
		tp->mBlank.assign(4U, 0U); // at least one blank pixel

		return tp->mBlank.data();
	}
}

static void TheoraPlay_Cleanup (void * context)
{
	TheoraPlay * tp = static_cast<TheoraPlay *>(context);

	//
}

static CoronaExternalBitmapFormat TheoraPlay_Format (void * context)
{
	TheoraPlay * tp = static_cast<TheoraPlay *>(context);

	// not very accurate... we want "masks" for iyuv / yv12, but without that behavior

	return tp->mFormat == THEORAPLAY_VIDFMT_RGBA ? kExternalBitmapFormat_RGBA : kExternalBitmapFormat_RGB;
}

static void TheoraPlay_Dispose (void * context)
{
	delete static_cast<TheoraPlay *>(context);
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
	TheoraPlay * tp = static_cast<TheoraPlay *>(CoronaExternalGetUserData(L, 1));

	if (!tp->mIsPaused && THEORAPLAY_isDecoding(tp->mDecoder))
	{
		unsigned int step = luaL_checkinteger(L, 2);

		if (!tp->mAudio) tp->mAudio = THEORAPLAY_getAudio(tp->mDecoder);
		if (!tp->mVideo) tp->mVideo = THEORAPLAY_getVideo(tp->mDecoder);

		if ((tp->mAudio || tp->mVideo) && step > 0U) // ??
		{
			unsigned int now = tp->mElapsed + step;

			// audio currently not very functional :P

			if (tp->mVideo)
			{
				if (tp->mFrameMS == UINT_MAX) tp->mFrameMS = (unsigned int)(1000.0 / tp->mVideo->fps);

				while (now >= tp->mVideo->playms + tp->mFrameMS)
				{
					const THEORAPLAY_VideoFrame * next_video = THEORAPLAY_getVideo(tp->mDecoder);

					if (!next_video) break;

					THEORAPLAY_freeVideo(tp->mVideo);

					tp->mVideo = next_video;
				}
			}

			tp->mElapsed = now;
		}
	}
//	if (tp->mVideo) CoronaLog("ms: %u, elapsed: %u", tp->mVideo->playms, tp->mElapsed);
	return 0;
}

static int Pause (lua_State * L)
{
	TheoraPlay * tp = static_cast<TheoraPlay *>(CoronaExternalGetUserData(L, 1));

	tp->mIsPaused = lua_toboolean(L, 2) != 0;

	return 0;
}

static void AvailableAudio (lua_State * L, TheoraPlay * tp)
{
	lua_pushinteger(L, THEORAPLAY_availableAudio(tp->mDecoder));
}

static void AvailableVideo (lua_State * L, TheoraPlay * tp)
{
	lua_pushinteger(L, THEORAPLAY_availableVideo(tp->mDecoder));
}

static void Elapsed (lua_State * L, TheoraPlay * tp)
{
	lua_pushinteger(L, tp->mElapsed);
}

static void HasAudioStream (lua_State * L, TheoraPlay * tp)
{
	lua_pushboolean(L, THEORAPLAY_hasAudioStream(tp->mDecoder));
}

static void HasVideoStream (lua_State * L, TheoraPlay * tp)
{
	lua_pushboolean(L, THEORAPLAY_hasVideoStream(tp->mDecoder));
}

static void IsPaused (lua_State * L, TheoraPlay * tp)
{
	lua_pushboolean(L, tp->mIsPaused ? 1 : 0);
}

static void PushFormat (lua_State * L, TheoraPlay * tp)
{
	switch (tp->mFormat)
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

static int TheoraPlay_GetField (lua_State * L, const char * field, void * context)
{
	int res = 1;

	if (strcmp(field, "Step") == 0) res = PushCachedFunction(L, Step);
	else if (strcmp(field, "Pause") == 0) res = PushCachedFunction(L, Pause);
	else if (strcmp(field, "availableAudio") == 0) AvailableAudio(L, static_cast<TheoraPlay *>(context));
	else if (strcmp(field, "availableVideo") == 0) AvailableVideo(L, static_cast<TheoraPlay *>(context));
	else if (strcmp(field, "elapsed") == 0) Elapsed(L, static_cast<TheoraPlay *>(context));
	else if (strcmp(field, "format") == 0) PushFormat(L, static_cast<TheoraPlay *>(context));
	else if (strcmp(field, "hasAudio") == 0) HasAudioStream(L, static_cast<TheoraPlay *>(context));
	else if (strcmp(field, "hasVideo") == 0) HasVideoStream(L, static_cast<TheoraPlay *>(context));
	else if (strcmp(field, "paused") == 0) IsPaused(L, static_cast<TheoraPlay *>(context));
	else res = 0;

	return res;
}

CORONA_EXPORT int luaopen_plugin_TheoraPlay (lua_State * L)
{
	lua_getglobal(L, "system");	// system
	lua_newtable(L);// system, theora_play
	lua_getfield(L, -2, "pathForFile");	// system, theora_play, system.pathForFile
	lua_pushcclosure(L, [](lua_State * L) {
		unsigned int max_frames = 30;
		THEORAPLAY_VideoFormat vformat = THEORAPLAY_VIDFMT_RGBA;

		if (lua_istable(L, 2))
		{
			lua_getfield(L, 2, "format");	// file, opts, format?
			lua_getfield(L, 2, "maxFrames");// file, opts, format?, maxFrames?
			lua_getfield(L, 2, "baseDir");	// file, opts, format?, maxFrames?, baseDir?

			if (!lua_isnil(L, -3))
			{
				const char * fnames[] = {"YV12", "IYUV", "RGB", "RGBA", nullptr};
				THEORAPLAY_VideoFormat formats[] = {THEORAPLAY_VIDFMT_YV12, THEORAPLAY_VIDFMT_IYUV, THEORAPLAY_VIDFMT_RGB, THEORAPLAY_VIDFMT_RGBA};

				vformat = formats[luaL_checkoption(L, -3, nullptr, fnames)];
			}

			if (!lua_isnil(L, -2)) max_frames = luaL_checkinteger(L, -2);

			if (!lua_isnil(L, -1))
			{
				lua_replace(L, 2);	// file, baseDir, format?, max_frames?
				lua_settop(L, 2);	// file, baseDir
			}

			else lua_settop(L, 1);	// file
		}

		else lua_settop(L, 1);	// file

		lua_pushvalue(L, lua_upvalueindex(1));	// file[, baseDir], system.pathForFile
		lua_insert(L, 1);	// system.pathForFile, file[, baseDir]

		if (lua_pcall(L, lua_gettop(L) - 1, 1, 0) != 0)	// file / err
		{
			lua_pushnil(L);	// err, nil
			lua_insert(L, 1);	// nil, err

			return 2;
		}

		TheoraPlay * tp = new TheoraPlay;

		tp->mDecoder = THEORAPLAY_startDecodeFile(lua_tostring(L, 1), max_frames, vformat);

		if (!tp->mDecoder)
		{
			delete tp;

			lua_pushnil(L);	// file, nil
			lua_pushliteral(L, "Unable to start Theora file decode");	// file, nil, err

			return 2;
		}

		CoronaExternalTextureCallbacks callbacks = {};

		callbacks.size = sizeof(CoronaExternalTextureCallbacks);
		callbacks.getFormat = TheoraPlay_Format;
		callbacks.getHeight = TheoraPlay_GetH;
		callbacks.getWidth = TheoraPlay_GetW;
		callbacks.onFinalize = TheoraPlay_Dispose;
		callbacks.onGetField = TheoraPlay_GetField;
		callbacks.onReleaseBitmap = TheoraPlay_Cleanup;
		callbacks.onRequestBitmap = TheoraPlay_GetData;

		if (!CoronaExternalPushTexture(L, &callbacks, tp))	// file, tp
		{
			delete tp;

			lua_pushnil(L);	// file, tp, nil
			lua_pushliteral(L, "Unable to set up TheoraPlay callbacks");// file, tp, nil, err

			return 2;
		}

		return 1;
	}, 1);	// system, theora_play, DecodeFromFile
	lua_setfield(L, -2, "decodeFromFile");	// system, theora_play = { decodeFromFile = DecodeFromFile }

	return 1;
}
