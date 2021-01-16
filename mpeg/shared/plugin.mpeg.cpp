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

#include "CoronaGraphics.h"
#include "CoronaLua.h"
#include "ByteReader.h"
#include "jo_file.h"
#include <vector>

void jo_write_mpeg (/*FILE *fp*/JO_File * fp, const /*void*/unsigned char * rgbx, int width, int height, int fps);

#define PL_MPEG_IMPLEMENTATION
#include "pl_mpeg.h"

struct Encoder {
	int mWidth, mHeight, mFPS;
	bool mOpen{false};
	JO_File mFile;

	Encoder (lua_State * L, const char * filename, const char * mode = "wb") : mFile{L, filename, mode}
	{
		mOpen = !!mFile.mFP;
	}

	~Encoder ()
	{
		if (mOpen) mFile.Close();
	}
};

struct Reader {
    std::vector<unsigned char> mRGB;
	plm_video_t * mVideo{nullptr};
    plm_frame_t * mFrame{nullptr};
    bool mPaused{false};

    Reader (const char * filename)
    {
        plm_buffer_t * buffer = plm_buffer_create_with_filename(filename);

        if (buffer)
        {
			mVideo = plm_video_create_with_buffer(buffer, 1);

            int w = plm_video_get_width(mVideo), h = plm_video_get_height(mVideo);

            mRGB.resize(size_t(w * h * 3));
        }
    }

    ~Reader ()
    {
        if (mVideo) plm_video_destroy(mVideo);
    }
};

static unsigned int MPEG_GetW (void * context)
{
    plm_video_t * video = static_cast<Reader *>(context)->mVideo;

	return video ? plm_video_get_width(video) : 0U;
}

static unsigned int MPEG_GetH (void * context)
{
	plm_video_t * video = static_cast<Reader *>(context)->mVideo;

	return video ? plm_video_get_height(video) : 0U;
}

static const void * MPEG_GetData (void * context)
{
    Reader * reader = static_cast<Reader *>(context);

	if (reader && reader->mVideo)
    {
        if (reader->mFrame)
        {
			int w = plm_video_get_width(reader->mVideo);

            plm_frame_to_rgb(reader->mFrame, reader->mRGB.data(), w * 3);

            reader->mFrame = nullptr;
        }

        return reader->mRGB.data();
    }

    else return nullptr;
}

static void MPEG_Cleanup (void * context)
{
}

static CoronaExternalBitmapFormat MPEG_Format (void * context)
{
    return kExternalBitmapFormat_RGB;
}

static void MPEG_Dispose (void * context)
{
	static_cast<Reader *>(context)->~Reader();
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
	Reader * player = static_cast<Reader *>(CoronaExternalGetUserData(L, 1));

	if (!player->mPaused)
	{
		lua_Number target_time = plm_video_get_time(player->mVideo) + lua_tonumber(L, 2);

		while (plm_video_get_time(player->mVideo) < target_time)
		{
			player->mFrame = plm_video_decode(player->mVideo);

			if (!player->mFrame) break;
		}

	}

	return 0;
}

static int MPEG_GetField (lua_State * L, const char * field, void * context)
{
	int res = 1;

	if (strcmp(field, "Step") == 0) res = PushCachedFunction(L, Step);
 /*
	else if (strcmp(field, "Pause") == 0) res = PushCachedFunction(L, Pause);
	else if (strcmp(field, "availableAudio") == 0) AvailableAudio(L, static_cast<TheoraReader *>(context));
	else if (strcmp(field, "availableVideo") == 0) AvailableVideo(L, static_cast<TheoraReader *>(context));
	else if (strcmp(field, "elapsed") == 0) Elapsed(L, static_cast<TheoraReader *>(context));
	else if (strcmp(field, "format") == 0) PushFormat(L, static_cast<TheoraReader *>(context));
	else if (strcmp(field, "hasAudio") == 0) HasAudioStream(L, static_cast<TheoraReader *>(context));
	else if (strcmp(field, "hasVideo") == 0) HasVideoStream(L, static_cast<TheoraReader *>(context));
	else if (strcmp(field, "paused") == 0) IsPaused(L, static_cast<TheoraReader *>(context)); */
	else res = 0;

	return res;
}

static Encoder * Writer (lua_State * L)
{
	return (Encoder *)luaL_checkudata(L, 1, "mpeg.encoder");
}

const unsigned char * GetBytes (const ByteReader & reader, std::vector<unsigned char> & fallback, int width, int height)
{
	size_t needed = size_t(width * height * 4);

	if (reader.mCount >= needed) return static_cast<const unsigned char *>(reader.mBytes);

	else
	{
		fallback.resize(needed);

		memcpy(fallback.data(), reader.mBytes, reader.mCount);
		memset(fallback.data() + reader.mCount, 0, reader.mCount - needed);

		return fallback.data();
	}
}

CORONA_EXPORT int luaopen_plugin_mpeg (lua_State* L)
{
	lua_getglobal(L, "system");	// system
	lua_newtable(L);// system, mpeg
	lua_getfield(L, -2, "pathForFile");	// system, mpeg, system.pathForFile
	lua_pushcclosure(L, [](lua_State * L) {
		int count = 1 + !lua_isnoneornil(L, 2);

		lua_settop(L, count);// file[, baseDir]
		lua_pushvalue(L, lua_upvalueindex(1));	// file[, baseDir], system.pathForFile
		lua_insert(L, 1);	// system.pathForFile, file[, baseDir]

		if (lua_pcall(L, count, 1, 0) != 0)	// file / err
		{
			lua_pushnil(L);	// err, nil
			lua_insert(L, 1);	// nil, err

			return 2;
		}

		const char * filename = luaL_checkstring(L, 1);

		Reader * reader = new Reader{filename};

		if (!reader->mVideo)
		{
			delete reader;

			lua_pushnil(L);	// file, nil
			lua_pushliteral(L, "Unable to start MPEG file decode");	// file, nil, err

			return 2;
		}

		CoronaExternalTextureCallbacks callbacks = {};

		callbacks.size = sizeof(CoronaExternalTextureCallbacks);
		callbacks.getFormat = MPEG_Format;
		callbacks.getHeight = MPEG_GetH;
		callbacks.getWidth = MPEG_GetW;
		callbacks.onFinalize = MPEG_Dispose;
		callbacks.onGetField = MPEG_GetField;
		callbacks.onReleaseBitmap = MPEG_Cleanup;
		callbacks.onRequestBitmap = MPEG_GetData;

		if (!CoronaExternalPushTexture(L, &callbacks, reader))	// file, reader
		{
			delete reader;

			lua_pushnil(L);	// file, reader, nil
			lua_pushliteral(L, "Unable to set up MPEG callbacks");	// file, reader, nil, err

			return 2;
		}

		return 1;
	}, 1);	// system, mpeg, DecodeFromFile
	lua_setfield(L, -2, "decodeFromFile");	// system, mpeg = { decodeFromFile = DecodeFromFile }
	lua_getfield(L, -2, "pathForFile");	// system, mpeg, pathForFile
	lua_getfield(L, -3, "DocumentsDirectory");	// system, mpeg, pathForFile, DocumentsDirectory
	lua_pushcclosure(L, [](lua_State * L) {
		luaL_checktype(L, 1, LUA_TTABLE);
		lua_getfield(L, 1, "filename");	// params, filename
		luaL_checktype(L, -1, LUA_TSTRING);
		lua_insert(L, 1);	// filename, params
		lua_getfield(L, 2, "width");// filename, params, width
		lua_getfield(L, 2, "height");	// filename, params, width, height
		lua_getfield(L, 2, "fps");	// filename, params, width, height, fps, append
		lua_getfield(L, 2, "append");	// filename, params, width, height, fps, append
		lua_getfield(L, 2, "baseDir");	// filename, params, width, height, fps, append, baseDir

		int w = luaL_optint(L, -5, -1), h = luaL_optint(L, -4, -1), fps = luaL_optint(L, -3, 60);
		bool append = lua_toboolean(L, -2) != 0;

		if (lua_isnil(L, -1)) lua_pushvalue(L, lua_upvalueindex(2));// filename, params, width, height, fps, append, nil, DocumentsDirectory

		lua_replace(L, 2);	// filename, baseDir, width, height, fps, append[, nil]
		lua_settop(L, 2);	// file, baseDir
		lua_pushvalue(L, lua_upvalueindex(1));	// file, baseDir, system.pathForFile
		lua_insert(L, 1);	// system.pathForFile, file, baseDir

		if (lua_pcall(L, 2, 1, 0) != 0)	// file / err
		{
			lua_pushnil(L);	// err, nil
			lua_insert(L, 1);	// nil, err

			return 2;
		}

		const char * file = luaL_checkstring(L, 1);
		FILE * fp = fopen(file, "rb");

		if (fp) fclose(fp);
		
		else append = false;

		if (!append)
		{
			luaL_argcheck(L, w > 0, 1, "Invalid width");
			luaL_argcheck(L, h > 0, 1, "Invalid height");
			luaL_argcheck(L, 24 == fps || 25 == fps || 30 == fps || 50 == fps || 60 == fps, 1, "Invalid fps");
		}

		Encoder * writer = (Encoder *)lua_newuserdata(L, sizeof(Encoder));	// file, encoder

		new (writer) Encoder{L, file, append ? "ab" : "wb"};

		if (!writer->mOpen)
		{
			writer->~Encoder();

			lua_pushnil(L);	// file, encoder, nil
			lua_pushliteral(L, "Unable to open file for writing or appending");
		}

		else if (append)
		{
			fp = writer->mFile.mFP;

			fseek(fp, 4, SEEK_SET); // sequence header

            // 12 bits for width, height
            int8_t n1, n2, n3;

            fread(&n1, 1, 1, fp);
            fread(&n2, 1, 1, fp);
            fread(&n3, 1, 1, fp);

            w = (int(n1) << 4) | (int(n2 & 0xF0) >> 4);
            h = (int(n2 & 0xF) << 8) | int(n3 & 0xFF);

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

			fseek(fp, SEEK_END, 0);
		}

		writer->mWidth = w;
		writer->mHeight = h;
		writer->mFPS = fps;

		if (luaL_newmetatable(L, "mpeg.encoder"))	// file, encoder, meta
		{
			luaL_Reg funcs[] = {
				{
					"commit", [](lua_State * L)
					{
						Encoder * encoder = Writer(L);

						if (encoder->mOpen)
						{
							encoder->mFile.Close();

							encoder->mOpen = false;
						}

						return 0;
					}
				}, {
					"__gc", [](lua_State * L)
					{
						Writer(L)->~Encoder();

						return 0;
					}
				}, {
					"newFrame", [](lua_State * L)
					{
						Encoder * writer = Writer(L);

						ByteReader reader{L, 2};
						std::vector<unsigned char> fallback;

						const unsigned char * bytes = GetBytes(reader, fallback, writer->mWidth, writer->mHeight);

						jo_write_mpeg(&writer->mFile, bytes, writer->mWidth, writer->mHeight, writer->mFPS);
 
						return 0;
					}
				},
				{ nullptr, nullptr }
			};

			luaL_register(L, nullptr, funcs);
			lua_pushvalue(L, -1);	// file, encoder, meta, meta
			lua_setfield(L, -2, "__index");	// file, encoder, meta = { ..., __index = meta }
		}

		lua_setmetatable(L, -2);// path, params, encoder; encoder.metatable = meta

		return 1;
	}, 2);	// system, mpeg, NewEncoder
	lua_setfield(L, -2, "newEncoder");	// system, mpeg = { decodeFromFile, newEncoder = NewEncoder }

	return 1;
}
