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

#include "CoronaLibrary.h"
#include "CoronaGraphics.h"
#include "CoronaLua.h"
#include "utils/Blob.h"
#include "utils/LuaEx.h"
#include "ByteReader.h"
#include <vector>

// TODO: CHIPS_ASSERT() to throw Lua error
#define CHIPS_ASSERT(x) if (!(x)) printf("%s\n", #x)

#define CHIPS_IMPL
#undef _SP // Windows uses this too

#include "chips/m6502.h"
#include "chips/m6526.h"
#include "chips/m6569.h"
#include "chips/m6581.h"
#include "chips/z80.h"
#include "chips/am40010.h"
#include "chips/ay38910.h"
#include "chips/beeper.h"
#include "chips/clk.h"
#include "chips/fdd.h"
#include "chips/fdd_cpc.h"
#include "chips/i8255.h"
#include "chips/kbd.h"
#include "chips/mc6845.h"
#include "chips/mem.h"
#include "chips/upd765.h"
#include "systems/cpc.h"
#include "systems/c64.h"
#include "systems/zx.h"

struct Screen {
	enum InstanceType { eCPC, eC64, eZX };

	Screen (InstanceType type, size_t size) : mType{type}, mPixels(size)
	{
	}

	template<typename T> T * Instance (void) { return reinterpret_cast<T *>(mInstance.data()); }

	std::vector<unsigned char> mPixels;
	std::vector<unsigned char> mInstance;
	InstanceType mType;
};

Screen * GetScreen (lua_State * L)
{
	return static_cast<Screen *>(CoronaExternalGetUserData(L, 1));
}

static int GetBytes (lua_State * L)
{
	auto & bytes = GetScreen(L)->mPixels;

	lua_pushlstring(L, reinterpret_cast<const char *>(bytes.data()), bytes.size());	// screen, mask

	return 1;
}

static cpc_t * WrapCPC (lua_State * L, cpc_t * cpc = nullptr);
static c64_t * WrapC64 (lua_State * L, c64_t * c64 = nullptr);
static zx_t * WrapZX (lua_State * L, zx_t * zx = nullptr);

static int Screen_GetField (lua_State * L, const char * field, void * context)
{
	int res = 1;

	if (strcmp(field, "GetBytes") == 0) lua_pushcfunction(L, GetBytes);
	else if (strcmp(field, "instance") == 0)
	{
		Screen * screen = static_cast<Screen *>(context);

		switch (screen->mType)
		{
		case Screen::eCPC:
			WrapCPC(L, screen->template Instance<cpc_t>());
			break;
		case Screen::eC64:
			WrapC64(L, screen->template Instance<c64_t>());
			break;
		case Screen::eZX:
			WrapZX(L, screen->template Instance<zx_t>());
			break;
		}

		res = 1;
	}
	else if (strcmp(field, "instance_type") == 0)
	{
		switch (static_cast<Screen *>(context)->mType)
		{
		case Screen::eCPC:
			lua_pushliteral(L, "CPC");
			break;
		case Screen::eC64:
			lua_pushliteral(L, "C64");
			break;
		case Screen::eZX:
			lua_pushliteral(L, "ZX");
			break;
		}

		res = 1;
	}
	else res = 0;

	return res;
}

static unsigned int Screen_GetW (void * context)
{
	Screen * screen = static_cast<Screen *>(context);
	bool loaded = !screen->mInstance.empty();

	switch (screen->mType)
	{
	case Screen::eCPC:
		return (unsigned int)(loaded ? cpc_display_width(screen->template Instance<cpc_t>()) : cpc_std_display_width());
	case Screen::eC64:
		return (unsigned int)(loaded ? c64_display_width(screen->template Instance<c64_t>()) : c64_std_display_width());
	case Screen::eZX:
		return (unsigned int)(loaded ? zx_display_width(screen->template Instance<zx_t>()) : zx_std_display_width());
	default:
		return 0U;
	}
}

static unsigned int Screen_GetH (void * context)
{
	Screen * screen = static_cast<Screen *>(context);
	bool loaded = !screen->mInstance.empty();

	switch (screen->mType)
	{
	case Screen::eCPC:
		return (unsigned int)(loaded ? cpc_display_height(screen->template Instance<cpc_t>()) : cpc_std_display_height());
	case Screen::eC64:
		return (unsigned int)(loaded ? c64_display_height(screen->template Instance<c64_t>()) : c64_std_display_height());
	case Screen::eZX:
		return (unsigned int)(loaded ? zx_display_height(screen->template Instance<zx_t>()) : zx_std_display_height());
	default:
		return 0U;
	}
}

static const void * Screen_GetData (void * context)
{
	return static_cast<Screen *>(context)->mPixels.data();
}

static void Screen_Cleanup (void * context)
{
	//
}

static CoronaExternalBitmapFormat Screen_Format (void * context)
{
	return CoronaExternalBitmapFormat::kExternalBitmapFormat_RGBA;
}

static void Screen_Dispose (void * context)
{
	Screen * screen = static_cast<Screen *>(context);

	switch (screen->mType)
	{
	case Screen::eCPC:
		cpc_discard(screen->template Instance<cpc_t>());
		break;
	case Screen::eC64:
		c64_discard(screen->template Instance<c64_t>());
		break;
	case Screen::eZX:
		zx_discard(screen->template Instance<zx_t>());
		break;
	default:
		break;
	}

	delete screen;
}

static Screen * NewScreen (lua_State * L, Screen::InstanceType type, int size)
{
	Screen * screen = new Screen{type, size_t(size)};

	CoronaExternalTextureCallbacks callbacks = {};

	callbacks.size = sizeof(CoronaExternalTextureCallbacks);
	callbacks.getFormat = Screen_Format;
	callbacks.getHeight = Screen_GetH;
	callbacks.getWidth = Screen_GetW;
	callbacks.onFinalize = Screen_Dispose;
	callbacks.onGetField = Screen_GetField;
	callbacks.onReleaseBitmap = Screen_Cleanup;
	callbacks.onRequestBitmap = Screen_GetData;

	if (CoronaExternalPushTexture(L, &callbacks, screen)) return screen;	// ..., screen
	
	else
	{
		delete screen;

		return nullptr;
	}
}

template<typename T> bool & InUseFlag (T * ptr)
{
	return *reinterpret_cast<bool *>(ptr + 1);
}

template<typename T> T * Get (lua_State * L, const char * name, bool may_discard)
{
	T * v{nullptr};

	if (lua_objlen(L, 1) > sizeof(T *)) // embedded objects far larger than pointers
	{
		v = LuaXS::CheckUD<T>(L, 1, name);
		bool present = InUseFlag(v);

		luaL_argcheck(L, present || may_discard, 1, "Instance has been discarded");

		if (!present && may_discard) return nullptr;
	}

	else if (!may_discard) v = *LuaXS::CheckUD<T *>(L, 1, name); // reject boxes when discards allowed

	return v;
}

static cpc_t * GetCPC (lua_State * L, bool may_be_null = false)
{
	return Get<cpc_t>(L, "chips.cpc", may_be_null);
}

static cpc_joystick_type_t ToCPCJoystickType (lua_State * L, const char * def, int arg)
{
	const char * names[] = { "ANALOG", "DIGITAL", "NONE", nullptr };
	cpc_joystick_type_t types[] = { CPC_JOYSTICK_ANALOG, CPC_JOYSTICK_DIGITAL, CPC_JOYSTICK_NONE };

	return types[luaL_checkoption(L, arg, def, names)];
}

template<typename T> T * BoxOrAlloc (lua_State * L, T * v)
{
	if (v) *LuaXS::NewTyped<T *>(L) = v; // ..., box

	else
	{
		v = LuaXS::NewSizeTypedExtra<T>(L, sizeof(bool));	// ..., v
		InUseFlag(v) = true;
	}

	return v;
}

static cpc_t * WrapCPC (lua_State * L, cpc_t * cpc)
{
	cpc = BoxOrAlloc(L, cpc);	// ..., box / cpc

	LuaXS::AttachMethods(L, "chips.cpc", [](lua_State * L) {
		luaL_Reg cpc_methods[] = {
			{
				"discard", [](lua_State * L)
				{
					cpc_t * cpc = GetCPC(L, true);

					luaL_argcheck(L, cpc, 1, "Attempt to discard external or already-discarded CPC instance");

					cpc_discard(cpc);

					InUseFlag(cpc) = false; // known to be box

					return 0;
				}
			}, {
				"display_height", [](lua_State * L)
				{
					return LuaXS::PushArgAndReturn(L, cpc_display_height(GetCPC(L))); // cpc, height
				}
			}, {
				"display_width", [](lua_State * L)
				{
					return LuaXS::PushArgAndReturn(L, cpc_display_width(GetCPC(L))); // cpc, width
				}
			}, {
				"enable_video_debugging", [](lua_State * L)
				{
					cpc_enable_video_debugging(GetCPC(L), LuaXS::Bool(L, 2));

					return 0;
				}
			}, {
				"exec", [](lua_State * L)
				{
					cpc_exec(GetCPC(L), LuaXS::Uint(L, 2));

					return 0;
				}
			}, {
				"__gc", [](lua_State * L)
				{
					cpc_t * cpc = GetCPC(L, true);

					if (cpc) cpc_discard(cpc); // would be box

					return 0;
				}
			}, {
				"insert_disc", [](lua_State * L)
				{
					ByteReader bytes{L, 2};

					return LuaXS::PushArgAndReturn(L, cpc_insert_disc(GetCPC(L), static_cast<const uint8_t *>(bytes.mBytes), int(bytes.mCount))); // cpc, disc, ok
				}
			}, {
				"insert_tape", [](lua_State * L)
				{
					ByteReader bytes{L, 2};

					return LuaXS::PushArgAndReturn(L, cpc_insert_tape(GetCPC(L), static_cast<const uint8_t *>(bytes.mBytes), int(bytes.mCount))); // cpc, tape, ok
				}
			}, {
				"joystick", [](lua_State * L)
				{
					cpc_joystick(GetCPC(L), static_cast<uint8_t>(LuaXS::Uint(L, 2)));

					return 0;
				}
			}, {
				"joystick_type", [](lua_State * L)
				{
					cpc_joystick_type_t type = cpc_joystick_type(GetCPC(L));

					switch (type)
					{
					case CPC_JOYSTICK_ANALOG:
						lua_pushliteral(L, "ANALOG"); // cpc, "ANALOG"
						break;
					case CPC_JOYSTICK_DIGITAL:
						lua_pushliteral(L, "DIGITAL");// cpc, "DIGITAL"
						break;
					default:
						lua_pushliteral(L, "NONE");	// cpc, "NONE"
						break;
					}

					return 1;
				}
			}, {
				"key_down", [](lua_State * L)
				{
					cpc_key_down(GetCPC(L), LuaXS::Int(L, 2));

					return 0;
				}
			}, {
				"key_up", [](lua_State * L)
				{
					cpc_key_up(GetCPC(L), LuaXS::Int(L, 2));

					return 0;
				}
			}, {
				"quickload", [](lua_State * L)
				{
					ByteReader bytes{L, 2};

					return LuaXS::PushArgAndReturn(L, cpc_quickload(GetCPC(L), static_cast<const uint8_t *>(bytes.mBytes), int(bytes.mCount))); // cpc, bytes, ok
				}
			}, {
				"remove_disc", [](lua_State * L)
				{
					cpc_remove_disc(GetCPC(L));

					return 0;
				}
			}, {
				"remove_tape", [](lua_State * L)
				{
					cpc_remove_tape(GetCPC(L));

					return 0;
				}
			}, {
				"reset", [](lua_State * L)
				{
					cpc_reset(GetCPC(L));

					return 0;
				}
			}, {
				"set_joystick_type", [](lua_State * L)
				{
					cpc_set_joystick_type(GetCPC(L), ToCPCJoystickType(L, nullptr, 2));

					return 0;
				}
			}, {
				"video_debugging_enabled", [](lua_State * L)
				{
					return LuaXS::PushArgAndReturn(L, cpc_video_debugging_enabled(GetCPC(L))); // cpc, video_debugging_enabled
				}
			}, 
			{ nullptr, nullptr }
		};

		luaL_register(L, nullptr, cpc_methods);
	});

	return cpc;
}

static void * GetPixelBuffer (lua_State * L, int size)
{
	void * data{nullptr};

	lua_getfield(L, 1, "pixel_buffer");	// params, ..., pixel_buffer?

	if (!lua_isnil(L, -1))
	{
		ByteReader pixels{L, -1};	// params, ..., pixel_buffer[, err]

		if (!pixels.mBytes) lua_error(L);

		luaL_argcheck(L, int(pixels.mCount) >= size, 1, "Expected pixel buffer >= display size * 4");
		luaL_argcheck(L, BlobXS::IsBlob(L, -1), 1, "Pixel buffer must be a blob");
		luaL_argcheck(L, !BlobXS::IsResizable(L, -1), 1, "Pixel buffer may not be resizable");

		data = BlobXS::GetData(L, -1);
	}

	lua_pop(L, 1);	// params, ...

	return data;
}

static ByteReader GetBytes (lua_State * L, const char * name)
{
	lua_getfield(L, 1, name);	// params, ..., bytes?

	if (lua_isnil(L, -1)) // replace nil with empty string to stifle ByteReader errors on missing bytes
	{
		lua_pop(L, 1);	// params, ...
		lua_pushliteral(L, "");	// params, ..., ""
	}

	ByteReader bytes{L, -1};// params, ..., bytes[, err]

	if (!bytes.mBytes) lua_error(L);

	return bytes;
}

luaL_Reg cpc_funcs[] = {
	{
		"init", [](lua_State * L)
		{
			cpc_desc_t desc;

			memset(&desc, 0, sizeof(cpc_desc_t));

			desc.audio_cb = nullptr; // TODO!

			luaL_checktype(L, 1, LUA_TTABLE);
			lua_getfield(L, 1, "type"); // params, type
			lua_getfield(L, 1, "joystick_type");// params, type, joystick_type

			const char * tnames[] = { "464", "6128", "KCCOMPACT", nullptr };
			cpc_type_t ttypes[] = { CPC_TYPE_464, CPC_TYPE_6128, CPC_TYPE_KCCOMPACT };

			desc.type = ttypes[luaL_checkoption(L, -2, "6128", tnames)];
			desc.joystick_type = ToCPCJoystickType(L, "NONE", -1);

			ByteReader rom_os = GetBytes(L, "rom_os");	// params, type, joystick_type, rom_os
			ByteReader rom_basic = GetBytes(L, "rom_basic");// params, type, joystick_type, rom_os, rom_basic
			ByteReader rom_amsdos = GetBytes(L, "rom_amsdos");	// params, type, joystick_type, rom_os, rom_basic, rom_amsdos

			luaL_argcheck(L, rom_os.mCount >= 0x4000, 1, "Expected 16KB OS ROM");
			luaL_argcheck(L, rom_basic.mCount >= 0x4000, 1, "Expected 16KB BASIC ROM");

			switch (desc.type)
			{
			case CPC_TYPE_464:
				desc.rom_464_basic = rom_basic.mBytes;
				desc.rom_464_os = rom_os.mBytes;
				desc.rom_464_basic_size = 0x4000;
				desc.rom_464_os_size = 0x4000;
				break;
			case CPC_TYPE_6128:
				luaL_argcheck(L, rom_amsdos.mCount >= 0x4000, 1, "Expected 16KB AMSDOS ROM");

				desc.rom_6128_basic = rom_basic.mBytes;
				desc.rom_6128_os = rom_os.mBytes;
				desc.rom_6128_amsdos = rom_amsdos.mBytes;
				desc.rom_6128_basic_size = 0x4000;
				desc.rom_6128_os_size = 0x4000;
				desc.rom_6128_amsdos_size = 0x4000;
				break;
			case CPC_TYPE_KCCOMPACT:
				desc.rom_kcc_basic = rom_basic.mBytes;
				desc.rom_kcc_os = rom_os.mBytes;
				desc.rom_kcc_basic_size = 0x4000;
				desc.rom_kcc_os_size = 0x4000;
				break;
			}

			desc.pixel_buffer_size = cpc_max_display_size();
			desc.pixel_buffer = GetPixelBuffer(L, desc.pixel_buffer_size);

			if (desc.pixel_buffer) cpc_init(WrapCPC(L), &desc);	// params, type, joystick_type, rom_os, rom_basic, rom_amsdos, cpc

			else
			{
				Screen * screen = NewScreen(L, Screen::eCPC, desc.pixel_buffer_size); // params, type, joystick_type, rom_os, rom_basic, rom_amsdos[, screen]

				if (screen)
				{
					screen->mInstance.resize(sizeof(cpc_t));

					cpc_t * cpc = screen->template Instance<cpc_t>();

					desc.pixel_buffer = screen->mPixels.data();

					cpc_init(cpc, &desc);
				}

				else lua_pushnil(L);// params, type, joystick_type, rom_os, rom_basic, rom_amsdos, nil
			}

			return 1;
		}
	}, {
		"max_display_size", [](lua_State * L)
		{
			return LuaXS::PushArgAndReturn(L, cpc_max_display_size());	// max_display_size
		}
	}, {
		"std_display_height", [](lua_State * L)
		{
			return LuaXS::PushArgAndReturn(L, cpc_std_display_height()); // height
		}
	}, {
		"std_display_width", [](lua_State * L)
		{
			return LuaXS::PushArgAndReturn(L, cpc_std_display_height()); // width
		}
	},
	{ nullptr, nullptr }
};

static c64_t * GetC64 (lua_State * L, bool may_be_null = false)
{
	return Get<c64_t>(L, "chips.c64", may_be_null);
}

static c64_joystick_type_t ToC64JoystickType (lua_State * L, const char * def, int arg)
{
	const char * names[] = { "DIGITAL_1", "DIGITAL_2", "NONE", nullptr };
	c64_joystick_type_t types[] = { C64_JOYSTICKTYPE_DIGITAL_1, C64_JOYSTICKTYPE_DIGITAL_2, C64_JOYSTICKTYPE_NONE };

	return types[luaL_checkoption(L, arg, def, names)];
}

static c64_t * WrapC64 (lua_State * L, c64_t * c64)
{
	c64 = BoxOrAlloc(L, c64);	// ..., box / c64

	LuaXS::AttachMethods(L, "chips.c64", [](lua_State * L) {
		luaL_Reg cpc_methods[] = {
			{
				"discard", [](lua_State * L)
				{
					c64_t * c64 = GetC64(L, true);

					luaL_argcheck(L, c64, 1, "Attempt to discard external or already-discarded C64 instance");

					c64_discard(c64);

					InUseFlag(c64) = false; // known to be box

					return 0;
				}
			}, {
				"display_height", [](lua_State * L)
				{
					return LuaXS::PushArgAndReturn(L, c64_display_height(GetC64(L))); // c64, height
				}
			}, {
				"display_width", [](lua_State * L)
				{
					return LuaXS::PushArgAndReturn(L, c64_display_width(GetC64(L))); // c64, width
				}
			}, {
				"exec", [](lua_State * L)
				{
					c64_exec(GetC64(L), LuaXS::Uint(L, 2));

					return 0;
				}
			}, {
				"__gc", [](lua_State * L)
				{
					c64_t * c64 = GetC64(L, true);

					if (c64) c64_discard(c64); // would be box

					return 0;
				}
			}, {
				"insert_tape", [](lua_State * L)
				{
					ByteReader bytes{L, 2};

					return LuaXS::PushArgAndReturn(L, c64_insert_tape(GetC64(L), static_cast<const uint8_t *>(bytes.mBytes), int(bytes.mCount))); // c64, tape, ok
				}
			}, {
				"joystick", [](lua_State * L)
				{
					c64_joystick(GetC64(L), static_cast<uint8_t>(LuaXS::Uint(L, 2)), static_cast<uint8_t>(LuaXS::Uint(L, 3)));

					return 0;
				}
			}, {
				"joystick_type", [](lua_State * L)
				{
					c64_joystick_type_t type = c64_joystick_type(GetC64(L));

					switch (type)
					{
					case C64_JOYSTICKTYPE_DIGITAL_1:
						lua_pushliteral(L, "DIGITAL_1");// c64, "DIGITAL_1"
						break;
					case C64_JOYSTICKTYPE_DIGITAL_2:
						lua_pushliteral(L, "DIGITAL_2");// c64, "DIGITAL_2"
						break;
					default:
						lua_pushliteral(L, "NONE");	// c64, "NONE"
						break;
					}

					return 1;
				}
			}, {
				"key_down", [](lua_State * L)
				{
					c64_key_down(GetC64(L), LuaXS::Int(L, 2));

					return 0;
				}
			}, {
				"key_up", [](lua_State * L)
				{
					c64_key_up(GetC64(L), LuaXS::Int(L, 2));

					return 0;
				}
			}, {
				"quickload", [](lua_State * L)
				{
					ByteReader bytes{L, 2};

					return LuaXS::PushArgAndReturn(L, c64_quickload(GetC64(L), static_cast<const uint8_t *>(bytes.mBytes), int(bytes.mCount))); // c64, bytes, ok
				}
			}, {
				"remove_tape", [](lua_State * L)
				{
					c64_remove_tape(GetC64(L));

					return 0;
				}
			}, {
				"reset", [](lua_State * L)
				{
					c64_reset(GetC64(L));

					return 0;
				}
			}, {
				"set_joystick_type", [](lua_State * L)
				{
					c64_set_joystick_type(GetC64(L), ToC64JoystickType(L, nullptr, 2));

					return 0;
				}
			}, {
				"start_tape", [](lua_State * L)
				{
					c64_start_tape(GetC64(L));

					return 0;
				}
			}, {
				"stop_tape", [](lua_State * L)
				{
					c64_stop_tape(GetC64(L));

					return 0;
				}
			}, 
			{ nullptr, nullptr }
		};

		luaL_register(L, nullptr, cpc_methods);
	});

	return c64;
}

luaL_Reg c64_funcs[] = {
	{
		"init", [](lua_State * L)
		{
			c64_desc_t desc;

			memset(&desc, 0, sizeof(c64_desc_t));

			desc.audio_cb = nullptr; // TODO!

			luaL_checktype(L, 1, LUA_TTABLE);
			lua_getfield(L, 1, "joystick_type");// params, joystick_type

			desc.joystick_type = ToC64JoystickType(L, "NONE", -1);

			ByteReader rom_char = GetBytes(L, "rom_char");	// params, joystick_type, rom_char
			ByteReader rom_basic = GetBytes(L, "rom_basic");// params, joystick_type, rom_char, rom_basic
			ByteReader rom_kernal = GetBytes(L, "rom_kernal");	// params, joystick_type, rom_char, rom_basic, rom_kernal

			luaL_argcheck(L, rom_char.mCount >= 0x1000, 1, "Expected 4KB character ROM dump");
			luaL_argcheck(L, rom_basic.mCount >= 0x2000, 1, "Expected 8KB BASIC dump");
			luaL_argcheck(L, rom_kernal.mCount >= 0x2000, 1, "Expected 8KB KERNAL dump");

			desc.rom_char = rom_char.mBytes;
			desc.rom_basic = rom_basic.mBytes;
			desc.rom_kernal = rom_kernal.mBytes;
			desc.rom_char_size = 0x1000;
			desc.rom_basic_size = 0x2000;
			desc.rom_kernal_size = 0x2000;

			desc.pixel_buffer_size = c64_max_display_size();
			desc.pixel_buffer = GetPixelBuffer(L, desc.pixel_buffer_size);

			if (desc.pixel_buffer) c64_init(WrapC64(L), &desc);	// params, joystick_type, rom_char, rom_basic, rom_kernal, c64

			else
			{
				Screen * screen = NewScreen(L, Screen::eC64, desc.pixel_buffer_size); // params, joystick_type, rom_char, rom_basic, rom_kernal[, screen]

				if (screen)
				{
					screen->mInstance.resize(sizeof(c64_t));

					c64_t * c64 = screen->template Instance<c64_t>();

					desc.pixel_buffer = screen->mPixels.data();

					c64_init(c64, &desc);
				}

				else lua_pushnil(L);// params, joystick_type, rom_char, rom_basic, rom_kernal, nil
			}

			return 1;
		}
	}, {
		"max_display_size", [](lua_State * L)
		{
			return LuaXS::PushArgAndReturn(L, c64_max_display_size());	// max_display_size
		}
	}, {
		"std_display_height", [](lua_State * L)
		{
			return LuaXS::PushArgAndReturn(L, c64_std_display_height()); // height
		}
	}, {
		"std_display_width", [](lua_State * L)
		{
			return LuaXS::PushArgAndReturn(L, c64_std_display_height()); // width
		}
	},
	{ nullptr, nullptr }
};

static zx_t * GetZX (lua_State * L, bool may_be_null = false)
{
	return Get<zx_t>(L, "chips.zx", may_be_null);
}

static zx_joystick_type_t ToZXJoystickType (lua_State * L, const char * def, int arg)
{
	const char * names[] = { "KEMPSTON", "SINCLAIR_1", "SINCLAIR_2", "NONE", nullptr };
	zx_joystick_type_t types[] = { ZX_JOYSTICKTYPE_KEMPSTON, ZX_JOYSTICKTYPE_SINCLAIR_1, ZX_JOYSTICKTYPE_SINCLAIR_2, ZX_JOYSTICKTYPE_NONE };

	return types[luaL_checkoption(L, arg, def, names)];
}

static zx_t * WrapZX (lua_State * L, zx_t * zx)
{
	zx = BoxOrAlloc(L, zx);	// ..., box / zx

	LuaXS::AttachMethods(L, "chips.zx", [](lua_State * L) {
		luaL_Reg cpc_methods[] = {
			{
				"discard", [](lua_State * L)
				{
					zx_t * zx = GetZX(L, true);

					luaL_argcheck(L, zx, 1, "Attempt to discard external or already-discarded ZX Spectrum instance");

					zx_discard(zx);

					InUseFlag(zx) = false; // known to be box

					return 0;
				}
			}, {
				"display_height", [](lua_State * L)
				{
					return LuaXS::PushArgAndReturn(L, zx_display_height(GetZX(L)));	// zx, height
				}
			}, {
				"display_width", [](lua_State * L)
				{
					return LuaXS::PushArgAndReturn(L, zx_display_width(GetZX(L))); // zx, width
				}
			}, {
				"exec", [](lua_State * L)
				{
					zx_exec(GetZX(L), LuaXS::Uint(L, 2));

					return 0;
				}
			}, {
				"__gc", [](lua_State * L)
				{
					zx_t * zx = GetZX(L, true);

					if (zx) zx_discard(zx); // would be box

					return 0;
				}
			}, {
				"joystick", [](lua_State * L)
				{
					zx_joystick(GetZX(L), static_cast<uint8_t>(LuaXS::Uint(L, 2)));

					return 0;
				}
			}, {
				"joystick_type", [](lua_State * L)
				{
					zx_joystick_type_t type = zx_joystick_type(GetZX(L));

					switch (type)
					{
					case ZX_JOYSTICKTYPE_KEMPSTON:
						lua_pushliteral(L, "KEMPSTON");	// zx, "KEMPSTON"
						break;
					case ZX_JOYSTICKTYPE_SINCLAIR_1:
						lua_pushliteral(L, "SINCLAIR_1");	// zx, "SINCLAIR_1"
						break;
					case ZX_JOYSTICKTYPE_SINCLAIR_2:
						lua_pushliteral(L, "SINCLAIR_2");	// zx, "SINCLAIR_2"
						break;
					default:
						lua_pushliteral(L, "NONE");	// zx, "NONE"
						break;
					}

					return 1;
				}
			}, {
				"key_down", [](lua_State * L)
				{
					zx_key_down(GetZX(L), LuaXS::Int(L, 2));

					return 0;
				}
			}, {
				"key_up", [](lua_State * L)
				{
					zx_key_up(GetZX(L), LuaXS::Int(L, 2));

					return 0;
				}
			}, {
				"quickload", [](lua_State * L)
				{
					ByteReader bytes{L, 2};

					return LuaXS::PushArgAndReturn(L, zx_quickload(GetZX(L), static_cast<const uint8_t *>(bytes.mBytes), int(bytes.mCount))); // zx, bytes, ok
				}
			}, {
				"reset", [](lua_State * L)
				{
					zx_reset(GetZX(L));

					return 0;
				}
			}, {
				"set_joystick_type", [](lua_State * L)
				{
					zx_set_joystick_type(GetZX(L), ToZXJoystickType(L, nullptr, 2));

					return 0;
				}
			}, 
			{ nullptr, nullptr }
		};

		luaL_register(L, nullptr, cpc_methods);
	});

	return zx;
}

luaL_Reg zx_funcs[] = {
	{
		"init", [](lua_State * L)
		{
			zx_desc_t desc;

			memset(&desc, 0, sizeof(zx_desc_t));

			desc.audio_cb = nullptr; // TODO!

			luaL_checktype(L, 1, LUA_TTABLE);
			lua_getfield(L, 1, "type"); // params, type
			lua_getfield(L, 1, "joystick_type");// params, type, joystick_type

			const char * tnames[] = { "48K", "128", nullptr };
			zx_type_t ttypes[] = { ZX_TYPE_48K, ZX_TYPE_128 };

			desc.type = ttypes[luaL_checkoption(L, -2, "48K", tnames)];
			desc.joystick_type = ToZXJoystickType(L, "NONE", -1);

			ByteReader rom_zx48k = GetBytes(L, "rom_zx48k");// params, type, joystick_type, rom_zx48k
			ByteReader rom_zx128_0 = GetBytes(L, "rom_zx128_0");// params, type, joystick_type, rom_zx48k, rom_zx128_0
			ByteReader rom_zx128_1 = GetBytes(L, "rom_zx128_1");// params, type, joystick_type, rom_zx48k, rom_zx128_0, rom_zx128_1

			if (desc.type == ZX_TYPE_48K) luaL_argcheck(L, rom_zx48k.mCount >= 0x4000, 1, "Expected 16KB 48K ROM");

			else
			{
				luaL_argcheck(L, rom_zx128_0.mCount >= 0x4000, 1, "Expected 16KB 128K ROM #0");
				luaL_argcheck(L, rom_zx128_1.mCount >= 0x4000, 1, "Expected 16KB 128K ROM #1");
			}

			switch (desc.type)
			{
			case ZX_TYPE_48K:
				desc.rom_zx48k = rom_zx48k.mBytes;
				desc.rom_zx48k_size = 0x4000;
				break;
			case ZX_TYPE_128:
				desc.rom_zx128_0 = rom_zx128_0.mBytes;
				desc.rom_zx128_1 = rom_zx128_1.mBytes;
				desc.rom_zx128_0_size = 0x4000;
				desc.rom_zx128_1_size = 0x4000;
				break;
			}

			desc.pixel_buffer_size = zx_max_display_size();
			desc.pixel_buffer = GetPixelBuffer(L, desc.pixel_buffer_size);

			if (desc.pixel_buffer) zx_init(WrapZX(L), &desc);	// params, type, joystick_type, rom_zx48k, rom_zx128_0, rom_zx128_1, zx

			else
			{
				Screen * screen = NewScreen(L, Screen::eZX, zx_max_display_size());	// params, type, joystick_type, rom_zx48k, rom_zx128_0, rom_zx128_1[, screen]

				if (screen)
				{
					screen->mInstance.resize(sizeof(zx_t));

					zx_t * zx = screen->template Instance<zx_t>();

					desc.pixel_buffer = screen->mPixels.data();

					zx_init(zx, &desc);
				}

				else lua_pushnil(L);// params, type, joystick_type, rom_zx48k, rom_zx128_0, rom_zx128_1, nil
			}

			return 1;
		}
	}, {
		"max_display_size", [](lua_State * L)
		{
			return LuaXS::PushArgAndReturn(L, zx_max_display_size());	// max_display_size
		}
	}, {
		"std_display_height", [](lua_State * L)
		{
			return LuaXS::PushArgAndReturn(L, zx_std_display_height()); // height
		}
	}, {
		"std_display_width", [](lua_State * L)
		{
			return LuaXS::PushArgAndReturn(L, zx_std_display_height()); // width
		}
	},
	{ nullptr, nullptr }
};

static void AddFlag (lua_State * L, const char * name, int flag)
{
	lua_pushinteger(L, flag);	// ..., t, flag
	lua_setfield(L, -2, name);	// ..., t = { ..., name = flag }
}

CORONA_EXPORT int luaopen_plugin_chips (lua_State * L)
{
	luaL_reg funcs[] = {
		{ nullptr, nullptr }
	};

	CoronaLibraryNew(L, "chips", "com.xibalbastudios", 1, 0, funcs, nullptr);	// chips

	lua_newtable(L);// chips, cpc
	luaL_register(L, nullptr, cpc_funcs);

	#define ADD_CPC_FLAG(name) AddFlag(L, #name, CPC_##name)

	ADD_CPC_FLAG(JOYSTICK_UP);
	ADD_CPC_FLAG(JOYSTICK_DOWN);
	ADD_CPC_FLAG(JOYSTICK_LEFT);
	ADD_CPC_FLAG(JOYSTICK_RIGHT);
	ADD_CPC_FLAG(JOYSTICK_BTN0);
	ADD_CPC_FLAG(JOYSTICK_BTN1);

	lua_setfield(L, -2, "cpc");	// chips = { ..., cpc = cpc }
	lua_newtable(L);// chips, c64
	luaL_register(L, nullptr, c64_funcs);

	#define ADD_C64_FLAG(name) AddFlag(L, #name, C64_##name)

	ADD_C64_FLAG(JOYSTICK_UP);
	ADD_C64_FLAG(JOYSTICK_DOWN);
	ADD_C64_FLAG(JOYSTICK_LEFT);
	ADD_C64_FLAG(JOYSTICK_RIGHT);
	ADD_C64_FLAG(JOYSTICK_BTN);

	lua_setfield(L, -2, "c64");	// chips = { ..., cpc, c64 = c64 }
	lua_newtable(L);// chips, zx
	luaL_register(L, nullptr, zx_funcs);

	#define ADD_ZX_FLAG(name) AddFlag(L, #name, ZX_##name)

	ADD_ZX_FLAG(JOYSTICK_RIGHT);
	ADD_ZX_FLAG(JOYSTICK_LEFT);
	ADD_ZX_FLAG(JOYSTICK_DOWN);
	ADD_ZX_FLAG(JOYSTICK_UP);
	ADD_ZX_FLAG(JOYSTICK_BTN);

	lua_setfield(L, -2, "zx");	// chips = { ..., cpc, c64, zx = zx }

	return 1;
}