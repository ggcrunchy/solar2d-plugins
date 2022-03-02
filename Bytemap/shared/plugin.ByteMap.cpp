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

void Bytemap::InitializeBytes (std::vector<unsigned char> * bytes)
{
	if (!bytes)
	{
		size_t sz = mW * mH * CoronaExternalFormatBPP(mFormat);

		if (mFormat != mRepFormat) sz += mW * mH; // n.b. at the moment this means adding a dummy alpha component

		mBytes.assign(sz, 0);

		int bpp = CoronaExternalFormatBPP(mFormat), true_bpp = GetEffectiveBPP();

		if (true_bpp > bpp)
		{
			const unsigned char fixup[4] = {0, 0, 0, 255};

			for (size_t offset = bpp; offset < sz; offset += true_bpp)
			{
				for (int i = 0; i < true_bpp - bpp; ++i) mBytes[offset + i] = fixup[3 - i];
			}
		}
	}

	else mBytes.swap(*bytes);
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
}

Bytemap::~Bytemap (void)
{
	DetachBlob();

	lua_unref(mL, mDummyRef);
}

static bool AttachCallbacks (lua_State * L, Bytemap * bmap)
{
	CoronaExternalTextureCallbacks callbacks = {};

	callbacks.size = sizeof(CoronaExternalTextureCallbacks);
	callbacks.getFormat = Bytemap_Format;
	callbacks.getHeight = Bytemap_GetH;
	callbacks.getWidth = Bytemap_GetW;
	callbacks.onFinalize = Bytemap_Dispose;
	callbacks.onGetField = Bytemap_GetField;
	callbacks.onReleaseBitmap = Bytemap_Cleanup;
	callbacks.onRequestBitmap = Bytemap_GetData;

	if (!CoronaExternalPushTexture(L, &callbacks, bmap))	// ...[, bmap]
	{
		delete bmap;

		return false;
	}

	return true;
}

static CoronaExternalBitmapFormat ResolveFormat (int w, int h, CoronaExternalBitmapFormat format)
{
	if (w > 0 && h > 0)
	{
		CoronaExternalBitmapFormat rep_format = format;

		if (w % 4 != 0 && CoronaExternalFormatBPP(format) == 3) rep_format = kExternalBitmapFormat_RGBA;

		return rep_format;
	}

	else return kExternalBitmapFormat_Undefined;
}

static bool NewBytemap (lua_State * L, int w, int h, CoronaExternalBitmapFormat format, std::vector<unsigned char> * bytes = nullptr)
{
	lua_getfield(L, 1, "is_non_external");	// ..., is_non_external

	int is_non_external = lua_toboolean(L, -1);

	lua_pop(L, 1);	// ...

	// In main state, create an external userdata, unless requested otherwise.
	if (LuaXS::IsMainState(L) && !is_non_external)
	{
		CoronaExternalBitmapFormat rep_format = ResolveFormat(w, h, format);

		if (rep_format != kExternalBitmapFormat_Undefined)
		{
			Bytemap * bmap = new Bytemap{L, w, h, format, rep_format};

			bmap->InitializeBytes(bytes);

			return AttachCallbacks(L, bmap);	// params[, bmap]
		}

		else return false;
	}

	// Otherwise, make a plain userdata.
	else
	{
		Bytemap * bmap = LuaXS::NewTyped<Bytemap>(L, L, w, h, format);	// params, bmap

		bmap->InitializeBytes(bytes);

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

struct ByteProxy {
	unsigned char * mBytes;
	size_t mSize;
};

static void * RegisterByteProxyReader (lua_State * L)
{
	ByteReaderFunc * func = ByteReader::Register(L);

	func->mGetBytes = [](lua_State * L, ByteReader & reader, int arg, void *)
	{
		ByteProxy * proxy = LuaXS::UD<ByteProxy>(L, arg);

		reader.mBytes = proxy->mBytes;
		reader.mCount = proxy->mSize;

		return true;
	};

	return func;
}

struct BytemapRef {
	std::vector<unsigned char> * mBytes{nullptr};
	std::vector<int> mLoaders;
	int mW, mH, mComps;
};

static void * RegisterBytemapReaderWriter (lua_State * L)
{
	ByteReaderFunc * func = ByteReader::Register(L);

	func->mEnsureSize = [](lua_State * L, ByteReader & reader, int arg, void *, const std::vector<size_t> & sizes)
	{
		BytemapRef * bref = LuaXS::UD<BytemapRef>(L, arg);

		if (!bref->mBytes) return false;
		if (sizes.size() < 2U) return false;

		const size_t * psizes = sizes.data();

		if (!psizes[0] || !psizes[1]) return false;

		bref->mW = int(psizes[0]);
		bref->mH = int(psizes[1]);

		int ncomps = bref->mComps;

		if (ncomps > 1)
		{
			if (3 == bref->mComps && ResolveFormat(bref->mW, bref->mH, kExternalBitmapFormat_RGB) != kExternalBitmapFormat_RGB) ncomps = 4;

			bref->mBytes->resize(bref->mW * bref->mH * ncomps);
		}

		else
		{
			bref->mW = ((bref->mW + 6) + 3) & ~3;
			bref->mH = ((bref->mH + 6) + 3) & ~3;
			bref->mBytes->assign(size_t(bref->mW * bref->mH), 0U);
		}

		reader.mNumComponents = ncomps;

		return true;
	};

	func->mGetBytes = [](lua_State * L, ByteReader & reader, int arg, void *)
	{
		BytemapRef * bref = LuaXS::UD<BytemapRef>(L, arg);

		if (!bref->mBytes) return false;

		unsigned char * data = bref->mBytes->data();

		if (1U == reader.mNumComponents) data += (bref->mW + 1) * 3;// 3 rows, then 3 columns

		reader.mBytes = data;
		reader.mCount = bref->mBytes->size();

		return true;
	};

	func->mGetStrides = [](lua_State * L, ByteReader & reader, int arg, void *)
	{
		BytemapRef * bref = LuaXS::UD<BytemapRef>(L, arg);

		if (!bref->mBytes) return false;

		if (1U == reader.mNumComponents) reader.mStrides.push_back(size_t(bref->mW));

		return true;
	};

	return func;
}

template<typename T> void NewByteSource (lua_State * L, const char * mname, void * reader_key)
{
	ByteXS::BytesMetatableOpts opts;

	opts.mContext = reader_key;
	opts.mMore = [](lua_State * L, void * context)
	{
		lua_pushlightuserdata(L, context);	// ..., mt, key
		lua_setfield(L, -2, "__bytes");	// ..., mt = { ..., __bytes = key }
	};

	LuaXS::NewTyped<T>(L);	// ..., source

	ByteXS::AddBytesMetatable(L, mname, &opts);
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

	NewByteSource<ByteProxy>(L, "Bytemap.proxy", RegisterByteProxyReader(L));	// bytemap, proxy
	PathXS::Directories::Instantiate(L);// bytemap, proxy, dirs

	NewByteSource<BytemapRef>(L, "Bytemap.ptr", RegisterBytemapReaderWriter(L));// bytemap, proxy, dirs, bmap_ref

	lua_pushvalue(L, -1);	// bytemap, proxy, dirs, bmap_ref, bmap_ref
	lua_pushcclosure(L, [](lua_State * L) {
		luaL_argcheck(L, lua_isfunction(L, 1) || luaL_getmetafield(L, 1, "__call"), 1, "Non-callable loader");	// func[, call]
		lua_pushvalue(L, 1);// func[, call], func

		int ref = lua_ref(L, 1);// func[, call]

		lua_pushvalue(L, lua_upvalueindex(1));	// func[, call], bmap_ref

		LuaXS::UD<BytemapRef>(L, -1)->mLoaders.push_back(ref);

		return 0;
	}, 1); // bytemap, proxy, dirs, bmap_ref, addLoader
	lua_setfield(L, -5, "addLoader");	// bytemap = { newTexture, addLoader = addLoader }
	lua_pushcclosure(L, [](lua_State * L) {
		luaL_argcheck(L, lua_istable(L, 1), 1, "Non-table params for loadTexture");
		lua_pushvalue(L, lua_upvalueindex(2));	// params, dirs

		PathXS::Directories * dirs = LuaXS::UD<PathXS::Directories>(L, -1);

		lua_getfield(L, 1, "format");	// params, dirs, format?

        CoronaExternalBitmapFormat format = GetFormat(L, -1);

		lua_getfield(L, 1, "no_premultiplied_alpha");	// params, dirs, format?, no_premultiplied_alpha

		bool bNoPremultipliedAlpha = lua_toboolean(L, -1) != 0;

		lua_pop(L, 1);	// params, dirs, format?
		lua_getfield(L, 1, "from_memory"); // params, dirs, format?, from_memory?

		if (lua_isstring(L, -1))
		{
			lua_pushlightuserdata(L, dirs);	// params, dirs, format?, from_memory, dirs
			lua_insert(L, -2); // params, dirs, format?, dirs, from_memory
			lua_pushnil(L); // params, dirs, format?, dirs, from_memory, nil
		}

		else
		{
			lua_pop(L, 1); // params, dirs, format?
			lua_getfield(L, 1, "is_absolute");	// params, dirs, format?, is_absolute
			lua_getfield(L, 1, "filename");	// params, dirs, format?, is_absolute, filename
			lua_getfield(L, 1, "baseDir");  // params, dirs, format?, is_absolute, filename, base_dir
			luaL_argcheck(L, lua_isstring(L, -2), -2, "Expected filename");
		}

        return LuaXS::ResultOrNil(L, dirs->WithFileContentsDo(L, -2, -3, [L, format, bNoPremultipliedAlpha](ByteReader & bytes) {
            int w, h, comp, ncomps = 4;

            if (kExternalBitmapFormat_Mask == format) ncomps = 1;
            else if (kExternalBitmapFormat_RGB == format) ncomps = 3;

            unsigned char * uc = stbi_load_from_memory(static_cast<const unsigned char *>(bytes.mBytes), int(bytes.mCount), &w, &h, &comp, ncomps);
            bool bOK = false, used_stb = uc != nullptr;

            if (used_stb)
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

					if (4 == ncomps && !bNoPremultipliedAlpha)
					{
						for (int i = 0; i < proxy->mSize; i += 4)
						{
							unsigned int alpha = uc[i + 3];

							for (int j = 0; j < 3; ++j) uc[i + j] = (unsigned char)((alpha * uc[i + j]) / 255);
						}
					}

                    lua_pcall(L, 2, 0, 0);	// params, dirs, format?, is_absolute, filename, bmap
                }
            }

            STBI_FREE(uc);

			if (!used_stb)
			{
				lua_pushvalue(L, lua_upvalueindex(3));	// params, dirs, format?, is_absolute, filename, ..., bmap_ref
				lua_getfield(L, 1, "want_loader_errors");	// params, dirs, format?, is_absolute, filename, ..., bmap_ref, want_loader_errors

				bool want_loader_errors = lua_toboolean(L, -1) != 0;

				lua_pop(L, 1);	// params, dirs, format?, is_absolute, filename, ..., bmap_ref

				std::vector<unsigned char> out;

				BytemapRef * bref = LuaXS::UD<BytemapRef>(L, -1);

				bref->mBytes = &out;
				bref->mComps = ncomps;

				for (auto && loader : bref->mLoaders)
				{
					lua_getref(L, loader);	// params, dirs, format?, is_absolute, filename, ..., bmap_ref, loader
					lua_pushvalue(L, bytes.mPos);	// params, dirs, format?, is_absolute, filename, ..., bmap_ref, loader, bytes
					lua_pushvalue(L, -3);	// params, dirs, format?, is_absolute, filename, ..., bmap_ref, loader, bytes, bmap_ref

					bOK = lua_pcall(L, 2, 0, 0) == 0;	// params, dirs, format?, is_absolute, filename, ..., bmap_ref[, err]

					if (!bOK)
					{
						if (want_loader_errors) CoronaLuaWarning(L, "error = %s", lua_isstring(L, -1) ? lua_tostring(L, -1) : "ERROR");

						lua_pop(L, 1);	// params, dirs, format?, is_absolute, filename, ..., bmap_ref
					}

					else
					{
						bOK = NewBytemap(L, bref->mW, bref->mH, format, bref->mBytes);	// params, dirs, format?, is_absolute, filename, ..., bmap_ref[, bmap]

						break;
					}
				}

				bref->mBytes = nullptr;
			}

            return bOK;
        }));// params, format?, is_absolute, filename, bmap / nil
	}, 3);	// bytemap, loadTexture
	lua_setfield(L, -2, "loadTexture");	// bytemap = { newTexture, addLoader, loadTexture = loadTexture }

	return 1;
}
