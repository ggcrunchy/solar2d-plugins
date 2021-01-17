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
#include "utils/LuaEx.h"
#include "noise.h"
#if 0
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
#endif

static noise::module::Module * Module (lua_State * L, int arg = 1)
{
	luaL_checktype(L, arg, LUA_TUSERDATA);
	lua_getmetatable(L, arg);	// ..., ud, ..., mt
	lua_gettable(L, lua_upvalueindex(1));	// ..., ud, ..., exists?
	luaL_argcheck(L, arg, lua_toboolean(L, -1), "Not a module");
	lua_pop(L, 1);	// ..., ud, ...

	return LuaXS::UD<noise::module::Module>(L, arg);
}

static int AuxGetSourceModule (lua_State * L)
{
	lua_settop(L, 2);	// mod, key
	lua_getfenv(L, 1);	// mod, key, env
	lua_insert(L, 2);	// mod, env, key
	lua_gettable(L, 2);	// mod, env, module?

	return 1;
}

static void AuxSetSourceModule (lua_State * L)
{			
	lua_settop(L, 3);	// mod, key, other
	lua_getfenv(L, 1);	// mod, key, other, env
	lua_insert(L, 2);	// mod, env, key, other
	lua_settable(L, 2);	// mod, env = { ..., [key] = other }
}

static luaL_Reg module_funcs[] = {
	{
		"__gc", LuaXS::TypedGC<noise::module::Module>
	}, {
		"GetSourceModule", [](lua_State * L)
		{
			auto * mod = Module(L);

			luaL_checkint(L, 2);

			return AuxGetSourceModule(L);	// mod, env, source?
		}
	}, {
		"GetSourceModuleCount", [](lua_State * L)
		{
			lua_pushinteger(L, Module(L)->GetSourceModuleCount());	// module, count

			return 1;
		}
	}, {
		"GetValue", [](lua_State * L)
		{
			lua_pushnumber(L, Module(L)->GetValue(LuaXS::Double(L, 2), LuaXS::Double(L, 3), LuaXS::Double(L, 4)));	// module, x, y, z, value

			return 1;
		}
	}, {
		"SetSourceModule", [](lua_State * L)
		{
			auto * mod = Module(L), * other = Module(L, 3);
			int index = luaL_checkint(L, 2);

			luaL_argcheck(L, index >= 1 && index <= mod->GetSourceModuleCount(), 2, "Invalid index");

			mod->SetSourceModule(index - 1, *other);
				
			AuxSetSourceModule(L); // mod, env

			return 0;
		}
	},
	{ nullptr, nullptr }
};

static int NoOp (lua_State * L) { return 0; }

CORONA_EXPORT int luaopen_plugin_libnoise (lua_State* L)
{
	lua_createtable(L, 0, 3);	// module_mt
	lua_newtable(L);// module_mt, module_list
	lua_newtable(L);// module_mt, module_list, libnoise

	for (int i = 0; module_funcs[i].func; ++i)
	{
		lua_pushvalue(L, -2);	// module_mt, module_list, libnoise, module_list
		lua_pushcclosure(L, module_funcs[i].func, 1);	// module_mt, module_list, libnoise, func
		lua_setfield(L, -4, module_funcs[i].name);	// module_mt = { ..., name = func }, module_list, libnoise
	}

	luaL_Reg module_factories[] = {
		{ "abs", NoOp },
		{ "add", NoOp },
		{ "billow", NoOp },
		{ "blend", NoOp },
		{ "cache", NoOp },
		{ "checkerboard", NoOp },
		{ "clamp", NoOp },
		{ "const", NoOp },
		{ "curve", NoOp },
		{ "cylinders", NoOp },
		{ "displace", NoOp },
		{ "exponent", NoOp },
		{ "invert", NoOp },
		{ "max", NoOp },
		{ "min", NoOp },
		{ "multiply", NoOp },
		{ "perlin", NoOp },
		{ "power", NoOp },
		{ "ridgedmulti", NoOp },
		{ "rotatepoint", NoOp },
		{ "scalebias", NoOp },
		{ "scalepoint", NoOp },
		{ "select", NoOp },
		{ "spheres", NoOp },
		{ "terrace", NoOp },
		{ "translatepoint", NoOp },
		{ "turbulence", NoOp },
		{ "voronoi", NoOp },
		{ nullptr, nullptr }
	};

	for (int i = 0; module_factories[i].func; ++i)
	{
		lua_pushvalue(L, -3);	// module_mt, module_list, libnoise, module_mt
		lua_pushvalue(L, -3);	// module_mt, module_list, libnoise, module_mt, module_list
		lua_pushcclosure(L, module_factories[i].func, 2);	// module_mt, module_list, libnoise, factory
		lua_setfield(L, -2, module_funcs[i].name);	// module_mt, module_list, libnoise = { ..., name = factory }
	}

	return 1;
}