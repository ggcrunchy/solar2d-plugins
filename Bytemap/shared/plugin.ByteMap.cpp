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

#include "CoronaAssert.h"
#include "CoronaGraphics.h"
#include "CoronaLua.h"
#include "ByteReader.h"
#include "Bytemap.h"
#include "Bytes.h"
#include "utils/Blob.h"
#include "utils/LuaEx.h"
#include "utils/Path.h"
#include "utils/Platform.h"
#include "utils/Thread.h"

#define STB_IMAGE_IMPLEMENTATION
#define STBI_NO_STDIO
#define STBI_ONLY_JPEG
#define STBI_ONLY_PNG

#include "stb_image.h"

static unsigned int Bytemap_GetW (void * context)
{
	return static_cast<Bytemap *>(context)->mW;
}

static unsigned int Bytemap_GetH (void * context)
{
	return static_cast<Bytemap *>(context)->mH;
}

static const void * Bytemap_GetData (void * context)
{
	Bytemap * bmap = static_cast<Bytemap *>(context);

	bmap->CheckSufficientMemory();

	return bmap->GetData();
}

static void Bytemap_Cleanup (void * context)
{
	return static_cast<Bytemap *>(context)->ResolveMemory();
}

static CoronaExternalBitmapFormat Bytemap_Format (void * context)
{
	return static_cast<Bytemap *>(context)->GetRepFormat();
}

static void Bytemap_Dispose (void * context)
{
	Bytemap * bmap = static_cast<Bytemap *>(context);

	delete bmap;
}

unsigned char * Bytemap::GetData (void)
{
	if (mTemp || mBlobRef == LUA_NOREF) return mBytes.data();

	else
	{
		PushBlob();	// ..., blob

		unsigned char * data = BlobXS::GetData(mL, -1);

		lua_pop(mL, 1);	// ...

		return data;
	}
}

bool Bytemap::Flatten (bool bDetach)
{
	lua_pushcfunction(mL, Bytemap_SetBytes);// ..., SetBytes
	lua_pushvalue(mL, 1);	// ..., SetBytes, bmap

	PushBlob();	// ..., SetBytes, bmap, blob
	
	if (bDetach) DetachBlob();

	InitializeBytes();

	return lua_pcall(mL, 2, 0, 0) == 0;
}

void Bytemap::DetachBlob (int * ref)
{
	if (!ref) ref = &mBlobRef;

	lua_unref(mL, *ref);// ...; registry = { ..., [*ref] = nil }

	*ref = LUA_NOREF;
}

void Bytemap::InitializeBytes (void)
{
	size_t sz = mW * mH * CoronaExternalFormatBPP(mFormat);

	if (mFormat != mRepFormat) sz += mW * mH;

	mBytes.assign(sz, 0);

	if (mFormat != mRepFormat)
	{
		const unsigned char fixup[4] = { 0, 0, 0, 255 };
		
		int bpp = CoronaExternalFormatBPP(mFormat), true_bpp = GetEffectiveBPP();

		for (size_t offset = bpp; offset < sz; offset += true_bpp)
		{
			for (int i = 0; i < true_bpp - bpp; ++i) mBytes[offset + i] = fixup[3 - i];
		}
	}
}

void Bytemap::CheckSufficientMemory (void)
{
	if (mBlobRef == LUA_NOREF) return;

	PushBlob();	// ..., blob

	// If the representative format differs from the expected one, flattening is almost
	// certainly the right choice, since no end user should need to know about this,
	// apart from the leaky performance characteristics. This obviates the need to
	// compare the sizes using the representative format in the subsequent clause.
	mTemp = mFormat != mRepFormat || BlobXS::GetSize(mL, -1) < size_t(mW * mH * CoronaExternalFormatBPP(mFormat));

	if (mTemp) Flatten();

	lua_pop(mL, 1);	// ...
}

void Bytemap::PushBlob (void)
{
	lua_getref(mL, mBlobRef);	// ..., blob?
}

void Bytemap::ResolveMemory (void)
{
	if (mTemp) mBytes.clear();

	mTemp = false;
}

static int PushCachedFunction( lua_State *L, lua_CFunction f )
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

static int Bytemap_GetBlob (lua_State * L)
{
	Bytemap * bmap = LuaXS::DualUD<Bytemap>(L, 1, "BytemapXS");

	if (bmap) lua_getref(L, bmap->mBlobRef);// bmap, blob?

	else lua_pushnil(L);// bmap, nil

	return 1;
}

static void PushFormat (lua_State * L, Bytemap * bmap)
{
	switch (bmap->mFormat)
	{
	case kExternalBitmapFormat_Mask:
		lua_pushliteral(L, "mask");
		break;
	case kExternalBitmapFormat_RGB:
		lua_pushliteral(L, "rgb");
		break;
	default:
		lua_pushliteral(L, "rgba");
		break;
	}
}

static int Bytemap_GetField (lua_State * L, const char * field, void * context)
{
	int res = 1;
	
	if (strcmp(field, "SetBytes") == 0) res = PushCachedFunction(L, Bytemap_SetBytes);
	else if (strcmp(field, "GetBytes") == 0) res = PushCachedFunction(L, Bytemap_GetBytes);
	else if (strcmp(field, "BindBlob") == 0) res = PushCachedFunction(L, Bytemap_BindBlob);
	else if (strcmp(field, "Deallocate") == 0) res = PushCachedFunction(L, Bytemap_Deallocate);
	else if (strcmp(field, "GetBlob") == 0) res = PushCachedFunction(L, Bytemap_GetBlob);
	else if (strcmp(field, "format") == 0) PushFormat(L, static_cast<Bytemap *>(context));
	else res = 0;

	return res;
}

Bytemap::Bytemap (lua_State * L, int w, int h, CoronaExternalBitmapFormat format, CoronaExternalBitmapFormat rep_format) : mL{L}, mW{w}, mH{h}, mFormat{format}, mRepFormat{rep_format}
{
	if (rep_format == kExternalBitmapFormat_Undefined) rep_format = format;

	InitializeBytes();
}

Bytemap::~Bytemap (void)
{
	DetachBlob();

	lua_unref(mL, mDummyRef);
}

static bool NewBytemap (lua_State * L, int w, int h, CoronaExternalBitmapFormat format)
{
	lua_getfield(L, 1, "is_non_external");	// ..., is_non_external

	int is_non_external = lua_toboolean(L, -1);

	lua_pop(L, 1);	// ...

	// In main state, create an external userdata, unless requested otherwise.
	if (LuaXS::IsMainState(L) && !is_non_external)
	{
		if (w > 0 && h > 0)
		{
			CoronaExternalBitmapFormat rep_format = format;

			if (w % 4 != 0 && CoronaExternalFormatBPP(format) == 3) rep_format = kExternalBitmapFormat_RGBA;

			Bytemap * bmap = new Bytemap{L, w, h, format, rep_format};

			// set up callbacks
			CoronaExternalTextureCallbacks callbacks = {};

			callbacks.size = sizeof(CoronaExternalTextureCallbacks);
			callbacks.getFormat = Bytemap_Format;
			callbacks.getHeight = Bytemap_GetH;
			callbacks.getWidth = Bytemap_GetW;
			callbacks.onFinalize = Bytemap_Dispose;
			callbacks.onGetField = Bytemap_GetField;
			callbacks.onReleaseBitmap = Bytemap_Cleanup;
			callbacks.onRequestBitmap = Bytemap_GetData;

			if (!CoronaExternalPushTexture(L, &callbacks, bmap))	// params[, bmap]
			{
				delete bmap;

				return false;
			}

			return true;
		}

		else return false;
	}

	// Otherwise, make a plain userdata.
	else
	{
		LuaXS::NewTyped<Bytemap>(L, L, w, h, format);	// params, bmap
		LuaXS::AttachMethods(L, "BytemapXS", [](lua_State * L)
		{
			luaL_Reg methods[] = {
				{
					"BindBlob", Bytemap_BindBlob
				}, {
					"Deallocate", Bytemap_Deallocate
				}, {
					"__gc", LuaXS::TypedGC<Bytemap>
				}, {
					"GetBlob", Bytemap_GetBlob
				}, {
					"GetBytes", Bytemap_GetBytes
				}, {
					"invalidate", LuaXS::NoOp
				}, {
					"releaseSelf", LuaXS::NoOp
				}, {
					"SetBytes", Bytemap_SetBytes
				},
				{ nullptr, nullptr }
			};

			luaL_register(L, nullptr, methods);

			LuaXS::AttachProperties(L, [](lua_State * L)
			{
				if (lua_type(L, 2) == LUA_TSTRING)
				{
					Bytemap * bmap = LuaXS::UD<Bytemap>(L, 1);
					const char * what = lua_tostring(L, 2);

					if (strcmp(what, "width") == 0) lua_pushinteger(L, bmap->mW);	// bmap, key, w
					else if (strcmp(what, "height") == 0) lua_pushinteger(L, bmap->mH);	// bmap, key, h
					else if (strcmp(what, "format") == 0) PushFormat(L, bmap);	// bmap, key, format
					else if (strcmp(what, "filename") == 0 || strcmp(what, "baseDir") == 0) luaL_error(L, "`%s` is not a valid property for non-main state bytemaps", what);
				}

				lua_settop(L, 3);	// bmap, key, value / nil

				return 1;
			});	// ..., meta = { ..., __index = GetProperties() -> index }
		});

		return true;
	}
}

static CoronaExternalBitmapFormat GetFormat (lua_State * L, int arg)
{
	CoronaExternalBitmapFormat formats[] = { kExternalBitmapFormat_Mask, kExternalBitmapFormat_RGB, kExternalBitmapFormat_RGBA };
	const char * names[] = { "mask", "rgb", "rgba", nullptr };

	return formats[luaL_checkoption(L, arg, "rgba", names)];
}

CORONA_EXPORT int luaopen_plugin_Bytemap (lua_State * L)
{
	lua_newtable(L);// bytemap

	luaL_Reg bytemap_funcs[] = {
		{
			"newTexture", [](lua_State * L)
			{
				luaL_argcheck(L, lua_istable(L, 1), 1, "Non-table params for newTexture");
				lua_getfield(L, 1, "width");// params, w
				lua_getfield(L, 1, "height");	// params, w, h
				lua_getfield(L, 1, "format");	// params, w, h, format?

				return LuaXS::ResultOrNil(L, NewBytemap(L, LuaXS::Int(L, -3), LuaXS::Int(L, -2), GetFormat(L, -1)));// params, w, h, format?, bmap / nil
			}
		},
		{ nullptr, nullptr }
	};
	
	luaL_register(L, nullptr, bytemap_funcs);

	struct ByteProxy {
		unsigned char * mBytes;
		size_t mSize;
	};

	ByteReaderFunc * func = ByteReader::Register(L);

	func->mGetBytes = [](lua_State * L, ByteReader & reader, int arg, void *)
	{
		ByteProxy * proxy = LuaXS::UD<ByteProxy>(L, arg);

		reader.mBytes = proxy->mBytes;
		reader.mCount = proxy->mSize;

		return true;
	};

	lua_pushlightuserdata(L, func);	// bytemap, func

	ByteXS::BytesMetatableOpts opts;

	opts.mMore = [](lua_State * L)
	{
		lua_pushvalue(L, -3);	// ..., func, proxy, mt, func
		lua_setfield(L, -2, "__bytes");	// ..., func, proxy, mt = { ..., __bytes = func }
	};

	lua_newuserdata(L, sizeof(ByteProxy));	// bytemap, func, proxy
					
	ByteXS::AddBytesMetatable(L, "Bytemap.proxy", &opts);

	PathXS::Directories::Instantiate(L);// bytemap, func, proxy, dirs

	lua_pushcclosure(L, [](lua_State * L) {
		luaL_argcheck(L, lua_istable(L, 1), 1, "Non-table params for loadTexture");
		lua_pushvalue(L, lua_upvalueindex(2));	// params, dirs

		PathXS::Directories * dirs = LuaXS::UD<PathXS::Directories>(L, -1);

		lua_getfield(L, 1, "format");	// params, dirs, format?

        CoronaExternalBitmapFormat format = GetFormat(L, -1);

		lua_getfield(L, 1, "is_absolute");	// params, dirs, format?, is_absolute
		lua_getfield(L, 1, "filename");	// params, dirs, format?, is_absolute, filename
        lua_getfield(L, 1, "baseDir");  // params, dirs, format?, is_absolute, filename, base_dir

        return LuaXS::ResultOrNil(L, dirs->WithFileContentsDo(L, -2, -3, [L, format](ByteReader & bytes) {
            int w, h, comp, ncomps = 4;

            if (kExternalBitmapFormat_Mask == format) ncomps = 1;
            else if (kExternalBitmapFormat_RGB == format) ncomps = 3;

            unsigned char * uc = stbi_load_from_memory(static_cast<const unsigned char *>(bytes.mBytes), int(bytes.mCount), &w, &h, &comp, ncomps);
            bool bOK = false;

            if (uc)
            {
                bOK = NewBytemap(L, w, h, format);	// params, dirs, format?, is_absolute, filename[, bmap]

                if (bOK)
                {
                    lua_pushcfunction(L, Bytemap_SetBytes);	// params, dirs, format?, is_absolute, filename, bmap, SetBytes
                    lua_pushvalue(L, -2);	// params, dirs, format?, is_absolute, filename, bmap, SetBytes, bmap
                    lua_pushvalue(L, lua_upvalueindex(1));	// params, dirs, format?, is_absolute, filename, bmap, SetBytes, bmap, proxy

                    ByteProxy * proxy = LuaXS::UD<ByteProxy>(L, -1);

                    proxy->mBytes = uc;
                    proxy->mSize = size_t(w * h * ncomps);

                    lua_pcall(L, 2, 0, 0);	// params, dirs, format?, is_absolute, filename, bmap
                }
            }

            STBI_FREE(uc);

            return bOK;
        }));// params, format?, is_absolute, filename, bmap / nil
	}, 2);	// bytemap, func, loadTexture
	lua_setfield(L, -3, "loadTexture");	// bytemap = { newTexture, loadTexture = loadTexture }, func
	lua_pop(L, 1);	// bytemap

	return 1;
}
