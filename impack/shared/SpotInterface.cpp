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

#include "impack.h"
#include "ByteReader.h"
#include "utils/Blob.h"
#include "utils/LuaEx.h"
#include "SpotInterface.h"
#include "image_utils.h"
#include <utility>

spot::color * Color (lua_State * L, int arg)
{
	return LuaXS::CheckUD<spot::color>(L, arg, "impack.spot.color");
}

spot::image * Image (lua_State * L, int arg)
{
	return LuaXS::CheckUD<spot::image>(L, arg, "impack.spot.image");
}

int InstantiateSpotColor (lua_State * L, spot::color && color)
{
	LuaXS::NewTyped<spot::color>(L, std::move(color));	// ..., ud

	LuaXS::AttachMethods(L, "impack.spot.color", [](lua_State * L)
	{
		luaL_Reg spot_color_methods[] = {
			{
				"__add", [](lua_State * L)
				{
					if (lua_isnumber(L, 2)) return InstantiateSpotColor(L, *Color(L, 1) + LuaXS::Float(L, 2));

					else return InstantiateSpotColor(L, *Color(L, 1) + *Color(L, 2));
				}
			}, {
				"add_mutate", [](lua_State * L)
				{
					return LuaXS::PCallWithStackThenReturn(L, [](lua_State * L) {
						if (lua_isnumber(L, 2)) *Color(L, 1) += LuaXS::Float(L, 2);

						else *Color(L, 1) += *Color(L, 2);

						return LuaXS::PushArgAndReturn(L, true);
					}, false);
				}
			}, {
				"clamp", [](lua_State * L)
				{
					return LuaXS::PCallWithStackThenReturn(L, [](lua_State * L) {
						return InstantiateSpotColor(L, Color(L, 1)->clamp());
					}, LuaXS::Nil{});
				}
			}, {
				"clone", [](lua_State * L)
				{
					return LuaXS::PCallWithStackThenReturn(L, [](lua_State * L) {
						return InstantiateSpotColor(L, spot::color{*Color(L, 1)});
					}, LuaXS::Nil{});
				}
			}, {
				"__div", [](lua_State * L)
				{
					if (lua_isnumber(L, 2)) return InstantiateSpotColor(L, *Color(L, 1) / LuaXS::Float(L, 2));

					else return InstantiateSpotColor(L, *Color(L, 1) / *Color(L, 2));
				}
			}, {
				"div_mutate", [](lua_State * L)
				{
					return LuaXS::PCallWithStackThenReturn(L, [](lua_State * L) {
						if (lua_isnumber(L, 2)) *Color(L, 1) /= LuaXS::Float(L, 2);

						else *Color(L, 1) /= *Color(L, 2);

						return LuaXS::PushArgAndReturn(L, true);
					}, false);
				}
			}, {
				"__gc", LuaXS::TypedGC<spot::color>
			}, {
				"__mul", [](lua_State * L)
				{
					if (lua_isnumber(L, 2)) return InstantiateSpotColor(L, *Color(L, 1) * LuaXS::Float(L, 2));

					else return InstantiateSpotColor(L, *Color(L, 1) * *Color(L, 2));
				}
			}, {
				"mul_mutate", [](lua_State * L)
				{
					return LuaXS::PCallWithStackThenReturn(L, [](lua_State * L) {
						if (lua_isnumber(L, 2)) *Color(L, 1) *= LuaXS::Float(L, 2);

						else *Color(L, 1) *= *Color(L, 2);

						return LuaXS::PushArgAndReturn(L, true);
					}, false);
				}
			}, {
				"premultiply", [](lua_State * L)
				{
					return LuaXS::PCallWithStackThenReturn(L, [](lua_State * L) {
						return InstantiateSpotColor(L, Color(L, 1)->premultiply());
					}, LuaXS::Nil{});
				}
			}, {
				"__sub", [](lua_State * L)
				{
					if (lua_isnumber(L, 2)) return InstantiateSpotColor(L, *Color(L, 1) - LuaXS::Float(L, 2));

					else return InstantiateSpotColor(L, *Color(L, 1) - *Color(L, 2));
				}
			}, {
				"sub_mutate", [](lua_State * L)
				{
					return LuaXS::PCallWithStackThenReturn(L, [](lua_State * L) {
						if (lua_isnumber(L, 2)) *Color(L, 1) -= LuaXS::Float(L, 2);

						else *Color(L, 1) -= *Color(L, 2);

						return LuaXS::PushArgAndReturn(L, true);
					}, false);
				}
			}, {
				"to_rgba", [](lua_State * L)
				{
					return LuaXS::PCallWithStackThenReturn(L, [](lua_State * L) {
						return InstantiateSpotColor(L, Color(L, 1)->to_rgba());
					}, LuaXS::Nil{});
				}
			}, {
				"unpremultiply", [](lua_State * L)
				{
					return LuaXS::PCallWithStackThenReturn(L, [](lua_State * L) {
						return InstantiateSpotColor(L, Color(L, 1)->unpremultiply());
					}, LuaXS::Nil{});
				}
			},
			{ nullptr, nullptr }
		};

		luaL_register(L, nullptr, spot_color_methods);

		LuaXS::AttachProperties(L, [](lua_State * L)
		{
			if (lua_type(L, 2) == LUA_TSTRING)
			{
				spot::color * color = Color(L, 1);
				std::string what = lua_tostring(L, 2);

				if (what == "h") lua_pushnumber(L, color->h);	// color, key, h
				else if (what == "s") lua_pushnumber(L, color->s);	// color, key, s
				else if (what == "l") lua_pushnumber(L, color->l);	// color, key, l
				else if (what == "a") lua_pushnumber(L, color->a);	// color, key, a
			}

			lua_settop(L, 3);	// color, key, value / nil

			return 1;
		});	// ..., meta = { ..., __index = GetProperties() -> index }

		LuaXS::SetField(L, -1, "__newindex", [](lua_State * L)
		{
			if (lua_type(L, 2) == LUA_TSTRING)
			{
				spot::color * color = Color(L, 1);
				std::string what = lua_tostring(L, 2);
				float * value = nullptr;

				if (what == "h") value = &color->h;
				else if (what == "s") value = &color->s;
				else if (what == "l") value = &color->l;
				else if (what == "a") value = &color->a;

				if (value) *value = LuaXS::Float(L, 3);
			}

			return 0;
		});	// ..., meta = { ..., __index, __setindex = SetIndex }
	});

	return 1;
}

enum { eBMP, eDDS, eTGA, ePNG, eJPG, ePUG, eWEBP, eKTX, ePVR, eCCZ, ePKM };

static void GetParams (lua_State * L, int opts, unsigned int & quality, unsigned int & ncomps)
{
	lua_getfield(L, opts, "quality");	// ..., quality?

	if (!lua_isnil(L, -1))
	{
		int q = luaL_checkint(L, -1);

		luaL_argcheck(L, q > 0 && q <= 100, -1, "Invalid quality");

		quality = (unsigned int)q;
	}

	lua_getfield(L, opts, "channels");	// ..., quality?, channels?

	if (!lua_isnil(L, -1))
	{
		int n = luaL_checkint(L, -1);

		luaL_argcheck(L, n >= 1 && n <= 4, -1, "Invalid channel count");

		if (n) ncomps = size_t(n);
	}
}

static spot::pixel WriteRGB (const unsigned char * bytes)
{
	return spot::pixel(bytes[0], bytes[1], bytes[2], 255.f);
}

static spot::pixel WriteRGBA (const unsigned char * bytes)
{
	return spot::pixel(bytes[0], bytes[1], bytes[2], bytes[3]);
}

size_t WriteImageBytes (lua_State * L, spot::image & image, size_t x, size_t y, int rarg, int n, bool bAlpha)
{
	ByteReader bytes{L, rarg};

	if (!bytes.mBytes) return 0;

	auto data = static_cast<const unsigned char *>(bytes.mBytes);
	auto writer = bAlpha ? WriteRGBA : WriteRGB;
	size_t nwritten = 0U, bpp = bAlpha ? 4U : 3U, count = bytes.mCount / bpp;

	if (n > 0) count = (std::min)(size_t(n), count);

	while (y < image.h)
	{
		image.at(x, y) = writer(data);

		data += bpp;

		if (++nwritten == count) break;

		else if (++x == image.w)
		{
			x = 0U;

			++y;
		}
	}

	return nwritten;
}

struct Filename {
    PathXS::Directories * mDirs;
    const char * mName;
    bool mInAssets{false};

    Filename (lua_State * L)
    {
        mDirs = LuaXS::UD<PathXS::Directories>(L, -1);
    }
};

static Filename GetFilename (lua_State * L)
{
    luaL_getmetafield(L, 1, "impack.dirs"); // image, ..., dirs

    Filename out{L};

    lua_pop(L, 1);  // image, ...

	bool bIsAbsolute = ExtractFileArgs(L, out.mDirs);   // image, filename[, base_dir]

#ifdef __ANDROID__
    if (out.mDirs->UsesResourceDir(L, 3)) out.mInAssets = true;
#endif

	out.mName = (bIsAbsolute || out.mInAssets) ? luaL_checkstring(L, 2) : out.mDirs->Canonicalize(L, true, 2);	// image, filename[, base_dir]

    return out;
}

int InstantiateSpotImage (lua_State * L, spot::image && image)
{
	if (!image.mError.empty()) luaL_error(L, image.mError.c_str());

	LuaXS::NewTyped<spot::image>(L, std::move(image));	// ..., ud

	LuaXS::AttachMethods(L, "impack.spot.image", [](lua_State * L)
	{
        lua_pushvalue(L, lua_upvalueindex(1));  // ..., ud, meta, dirs
        lua_setfield(L, -2, "impack.dirs"); // ..., ud, meta = { ..., [impack.dirs] = dirs }

		luaL_Reg spot_image_methods[] = {
			{
				"__add", [](lua_State * L)
				{
					return LuaXS::PCallWithStackThenReturn(L, [](lua_State * L) {
						return InstantiateSpotImage(L, *Image(L, 1) + *Color(L, 2));
					}, LuaXS::Nil{});
				}
			}, {
				"add_mutate", [](lua_State * L)
				{
					return LuaXS::PCallWithStackThenReturn(L, [](lua_State * L) {
						*Image(L, 1) += *Color(L, 2);

						return LuaXS::PushArgAndReturn(L, true);
					}, false);
				}
			}, {
				"blank", [](lua_State * L)
				{
					return LuaXS::PCallWithStackThenReturn(L, [](lua_State * L) {
						return InstantiateSpotImage(L, Image(L, 1)->blank());
					}, LuaXS::Nil{});
				}
			}, {
				"bleed", [](lua_State * L)
				{
					return LuaXS::PCallWithStackThenReturn(L, [](lua_State * L) {
						return InstantiateSpotImage(L, Image(L, 1)->bleed());
					}, LuaXS::Nil{});
				}
			}, {
				"checkered", [](lua_State * L)
				{
					return LuaXS::PCallWithStackThenReturn(L, [](lua_State * L) {
						return InstantiateSpotImage(L, Image(L, 1)->checkered());
					}, LuaXS::Nil{});
				}
			}, {
				"clamp", [](lua_State * L)
				{
					return LuaXS::PCallWithStackThenReturn(L, [](lua_State * L) {
						return InstantiateSpotImage(L, Image(L, 1)->clamp());
					}, LuaXS::Nil{});
				}
			}, {
				"clear", [](lua_State * L)
				{
					*Image(L, 1) = spot::image{};

					return 0;
				}
			}, {
				"copy", [](lua_State * L)
				{
					return LuaXS::PCallWithStackThenReturn(L, [](lua_State * L) {
						spot::image * image = Image(L, 1);
						int x = luaL_checkint(L, 2) - 1, y = luaL_checkint(L, 3) - 1, w = -1, h = -1;

						luaL_argcheck(L, x >= 0 && size_t(x) < image->w, 2, "Invalid x");
						luaL_argcheck(L, y >= 0 && size_t(y) < image->h, 3, "Invalid y");
						
						if (!lua_isnil(L, 4))
						{
							w = luaL_checkint(L, 4);

							luaL_argcheck(L, w > 0 && size_t(x + w) <= image->w, 4, "Invalid width");
						}

						if (!lua_isnil(L, 5))
						{
							h = luaL_checkint(L, 5);

							luaL_argcheck(L, h > 0 && size_t(y + h) <= image->h, 5, "Invalid height");
						}

						return InstantiateSpotImage(L, image->copy(size_t(x), size_t(y), size_t(w), size_t(h)));
					}, LuaXS::Nil{});
				}
			}, {
				"crop", [](lua_State * L)
				{
					return LuaXS::PCallWithStackThenReturn(L, [](lua_State * L) {
						spot::image * image = Image(L, 1);
						int l = luaL_checkint(L, 2) - 1, r = luaL_checkint(L, 3) - 1, t = luaL_checkint(L, 4) - 1, b = luaL_checkint(L, 5) - 1;

						luaL_argcheck(L, l >= 0, 2, "Invalid left");
						luaL_argcheck(L, r >= 0, 3, "Invalid right");
						luaL_argcheck(L, t >= 0, 4, "Invalid top");
						luaL_argcheck(L, b >= 0, 5, "Invalid bottom");
						luaL_argcheck(L, size_t(l + r) <= image->w, 2, "Left + right > width");
						luaL_argcheck(L, size_t(t + b) <= image->h, 4, "Top + bottom >= height");

						return InstantiateSpotImage(L, image->crop(size_t(l), size_t(r), size_t(t), size_t(b)));
					}, LuaXS::Nil{});
				}
			}, {
				"__div", [](lua_State * L)
				{
					return LuaXS::PCallWithStackThenReturn(L, [](lua_State * L) {
						return InstantiateSpotImage(L, *Image(L, 1) / *Color(L, 2));
					}, LuaXS::Nil{});
				}
			}, {
				"div_mutate", [](lua_State * L)
				{
					return LuaXS::PCallWithStackThenReturn(L, [](lua_State * L) {
						*Image(L, 1) /= *Color(L, 2);

						return LuaXS::PushArgAndReturn(L, true);
					}, false);
				}
			}, {
				"encode", [](lua_State * L)
				{
					return LuaXS::PCallWithStackThenReturn(L, [](lua_State * L) {
						lua_settop(L, 3);	// image, format, opts

						spot::image * image = Image(L, 1);
						const char * names[] = { "PNG", "JPG", "PUG", "WEBP", "KTX", "PVR", "CCZ", "PKM", nullptr };
						int formats[] = { ePNG, eJPG, ePUG, eWEBP, eKTX, ePVR, eCCZ, ePKM };
						unsigned int quality = spot::SPOT_DEFAULT_QUALITY, ncomps = 4U;
						std::string encoded;

						if (lua_istable(L, 3)) GetParams(L, 3, quality, ncomps);

						switch (formats[luaL_checkoption(L, 2, nullptr, names)])
						{
						case ePNG:
							encoded = image->encode_as_png(ncomps);
							break;
						case eJPG:
							encoded = image->encode_as_jpg(quality);
							break;
						case ePUG:
							encoded = image->encode_as_pug(quality);
							break;
						case eWEBP:
							encoded = image->encode_as_webp(quality);
							break;
						case eKTX:
							encoded = image->encode_as_ktx(quality);
							break;
						case ePVR:
							encoded = image->encode_as_pvr(quality);
							break;
						case eCCZ:
							encoded = image->encode_as_ccz(quality);
							break;
						case ePKM:
							encoded = image->encode_as_pkm(quality);
							break;
						}

						lua_pushlstring(L, encoded.c_str(), encoded.length());

						return 1;
					}, LuaXS::Nil{});
				}
			}, {
				"flip_h", [](lua_State * L)
				{
					return LuaXS::PCallWithStackThenReturn(L, [](lua_State * L) {
						return InstantiateSpotImage(L, Image(L, 1)->flip_h());
					}, LuaXS::Nil{});
				}
			}, {
				"flip_w", [](lua_State * L)
				{
					return LuaXS::PCallWithStackThenReturn(L, [](lua_State * L) {
						return InstantiateSpotImage(L, Image(L, 1)->flip_w());
					}, LuaXS::Nil{});
				}
			}, {
				"get", [](lua_State * L)
				{
					return LuaXS::PCallWithStackThenReturn(L, [](lua_State * L) {
						spot::color color;
						spot::image * image = Image(L, 1);
						int x = luaL_checkint(L, 2) - 1;

						luaL_argcheck(L, x >= 0 && size_t(x) < image->w, 2, "Invalid x");

						if (!lua_isnil(L, 3))
						{
							int y = luaL_checkint(L, 3) - 1;

							luaL_argcheck(L, y >= 0 && size_t(y) < image->h, 3, "Invalid y");

							if (!lua_isnil(L, 4))
							{
								int z = luaL_checkint(L, 4) - 1;

								luaL_argcheck(L, z >= 0 && size_t(z) < image->d, 4, "Invalid z");

								color = image->at(size_t(x), size_t(y), size_t(z));
							}

							else color = image->at(size_t(x), size_t(y));
						}

						else color = image->at(size_t(x));

						return InstantiateSpotColor(L, std::move(color));
					}, LuaXS::Nil{});
				}
			}, {
				"getf", [](lua_State * L)
				{
					return LuaXS::PCallWithStackThenReturn(L, [](lua_State * L) {
						spot::color color;
						spot::image * image = Image(L, 1);
						float x = LuaXS::Float(L, 2);

						luaL_argcheck(L, x >= 0.0f && x <= 1.0f, 2, "Invalid x");

						if (!lua_isnil(L, 3))
						{
							float y = LuaXS::Float(L, 3);

							luaL_argcheck(L, y >= 0.0f && y <= 1.0f, 3, "Invalid y");

							if (!lua_isnil(L, 4))
							{
								float z = LuaXS::Float(L, 4);

								luaL_argcheck(L, z >= 0.0f && z <= 1.0f, 4, "Invalid z");

								color = image->atf(x, y, z);
							}

							else color = image->atf(x, y);
						}

						else color = image->atf(x);

						return InstantiateSpotColor(L, std::move(color));
					}, LuaXS::Nil{});
				}
			}, {
				"__gc", LuaXS::TypedGC<spot::image>
			}, {
				"glow", [](lua_State * L)
				{
					return LuaXS::PCallWithStackThenReturn(L, [](lua_State * L) {
						return InstantiateSpotImage(L, Image(L, 1)->glow());
					}, LuaXS::Nil{});
				}
			}, {
				"load", [](lua_State * L)
				{
					return LuaXS::PCallWithStackThenReturn(L, [](lua_State * L) {
						spot::image * image = Image(L, 1);
                        Filename filename = GetFilename(L);	// image, filename[, base_dir]
						
                    #ifdef __ANDROID__
                        if (filename.mInAssets)
                        {
                            auto fc = filename.mDirs->WithFileContents(L, 2);   // image, filename[, base_dir], proxy / contents / nil
                            
                            ByteReader reader{L, -1};

                            if (!image->load(reader.mBytes, reader.mCount)) luaL_error(L, image->mError.c_str());
                        } else
                    #endif

						if (!image->load(filename.mName)) luaL_error(L, image->mError.c_str());

						return LuaXS::PushArgAndReturn(L, true);
					}, false);
				}
			}, {
				"load_from_memory", [](lua_State * L)
				{
					return LuaXS::PCallWithStackThenReturn(L, [](lua_State * L) {
						spot::image * image = Image(L, 1);
						ByteReader reader{L, 2};

						if (!image->load(reader.mBytes, reader.mCount)) luaL_error(L, image->mError.c_str());

						return LuaXS::PushArgAndReturn(L, true);
					}, false);
				}
			}, {
				"load_hdr", [](lua_State * L)
				{
					return LuaXS::PCallWithStackThenReturn(L, [](lua_State * L) {
						spot::image * image = Image(L, 1);
                        Filename filename = GetFilename(L);	// image, filename[, base_dir]

                    #ifdef __ANDROID__
                        if (filename.mInAssets)
                        {
                            auto fc = filename.mDirs->WithFileContents(L, 2);   // image, filename[, base_dir], proxy / contents / nil

                            ByteReader reader{L, -1};
                            
                            if (!image->load_hdr(reader.mBytes, reader.mCount)) luaL_error(L, image->mError.c_str());
                        } else
                    #endif

                        if (!image->load_hdr(filename.mName)) luaL_error(L, image->mError.c_str());

						return LuaXS::PushArgAndReturn(L, true);
					}, false);
				}
			}, {
				"load_hdr_from_memory", [](lua_State * L)
				{
					return LuaXS::PCallWithStackThenReturn(L, [](lua_State * L) {

						spot::image * image = Image(L, 1);
						ByteReader reader{L, 2};

						if (!image->load_hdr(reader.mBytes, reader.mCount)) luaL_error(L, image->mError.c_str());

						return LuaXS::PushArgAndReturn(L, true);
					}, false);
				}
			}, {
				"loaded", [](lua_State * L)
				{
					return LuaXS::PushArgAndReturn(L, LuaXS::UD<spot::image>(L, 1)->loaded());
				}
			}, {
				"__mul", [](lua_State * L)
				{
					return LuaXS::PCallWithStackThenReturn(L, [](lua_State * L) {
						return InstantiateSpotImage(L, *Image(L, 1) * *Color(L, 2));
					}, LuaXS::Nil{});
				}
			}, {
				"mul_mutate", [](lua_State * L)
				{
					return LuaXS::PCallWithStackThenReturn(L, [](lua_State * L) {
						*Image(L, 1) *= *Color(L, 2);

						return LuaXS::PushArgAndReturn(L, true);
					}, false);
				}
			}, {
				"paste", [](lua_State * L)
				{
					return LuaXS::PCallWithStackThenReturn(L, [](lua_State * L) {
						spot::image * image = Image(L, 1);
						int x = luaL_checkint(L, 2) - 1, y = luaL_checkint(L, 3) - 1;

						luaL_argcheck(L, x >= 0 && size_t(x) < image->w, 2, "Invalid x");
						luaL_argcheck(L, y >= 0 && size_t(y) < image->h, 3, "Invalid y");

						return InstantiateSpotImage(L, image->paste(x, y, *Image(L, 4)));
					}, LuaXS::Nil{});
				}
			}, {
				"premultiply", [](lua_State * L)
				{
					return LuaXS::PCallWithStackThenReturn(L, [](lua_State * L) {
						spot::image * image = Image(L, 1);
						spot::rect<spot::color> pic{image->w, image->h};

						pic.resize(0);

						pic.delay = image->delay;
						pic.space = image->space;

						for (const auto &it : *image) pic.push_back(it.premultiply());

						return InstantiateSpotImage(L, pic);
					}, LuaXS::Nil{});
				}
			}, {
				"premultiply_mutate", [](lua_State * L)
				{
					return LuaXS::PCallWithStackThenReturn(L, [](lua_State * L) {
						spot::image * image = Image(L, 1);

						for (auto &it : *image) it = it.premultiply();

						return LuaXS::PushArgAndReturn(L, true);
					}, false);
				}
			}, {
				"rotate_left", [](lua_State * L)
				{
					return LuaXS::PCallWithStackThenReturn(L, [](lua_State * L) {
						return InstantiateSpotImage(L, Image(L, 1)->rotate_left());
					}, LuaXS::Nil{});
				}
			}, {
				"rgba", [](lua_State * L)
				{
					return LuaXS::PCallWithStackThenReturn(L, [](lua_State * L) {
						auto res = Image(L, 1)->rgba();

						lua_pushlstring(L, reinterpret_cast<const char *>(res.data()), res.size());

						return 1;
					}, LuaXS::Nil{});
				}
			}, {
				"rotate_right", [](lua_State * L)
				{
					return LuaXS::PCallWithStackThenReturn(L, [](lua_State * L) {
						return InstantiateSpotImage(L, Image(L, 1)->rotate_right());
					}, LuaXS::Nil{});
				}
			}, {
				"save", [](lua_State * L)
				{
					return LuaXS::PCallWithStackThenReturn(L, [](lua_State * L) {
						lua_settop(L, 3);	// image, filename, opts

						spot::image * image = Image(L, 1);
						int format = -1;
						unsigned int quality = spot::SPOT_DEFAULT_QUALITY, ncomps = 4U;
						bool bOK;

                        if (!lua_istable(L, 3)) lua_pop(L, 1);  // image, filename
                        
                        else
						{
                            lua_getfield(L, 3, "format");	// image, filename, opts, format?
							
							if (!lua_isnil(L, 4))
							{
								const char * names[] = { "BMP", "DDS", "TGA", "PNG", "JPG", "PUG", "WEBP", "KTX", "PVR", "CCZ", "PKM", nullptr };
								int formats[] = { eBMP, eDDS, eTGA, ePNG, eJPG, ePUG, eWEBP, eKTX, ePVR, eCCZ, ePKM };
						
								format = formats[luaL_checkoption(L, 4, nullptr, names)];
							}

							GetParams(L, 3, quality, ncomps);   // image, filename, opts, format?, quality?, channels?
                            
                            lua_getfield(L, 3, "baseDir");  // image, filename, opts, format?, quality?, channels?, base_dir?
                            lua_replace(L, 3);  // image, filename, base_dir?, format?, quality?, channels?
						}

                        luaL_getmetafield(L, 1, "impack.dirs"); // image, filename, [base_dir, ]..., dirs

                        PathXS::Directories * dirs = LuaXS::UD<PathXS::Directories>(L, -1);
                        
                        lua_pop(L, 1);  // image, filename, [base_dir, ]...

                        const char * filename = dirs->Canonicalize(L, false, 2);

						if (format == -1 || format != ePNG || ncomps == 4U) bOK = image->save(filename, quality);

						else
						{
							switch (format)
							{
							case eBMP:
								bOK = image->save_as_bmp(filename);
								break;
							case eDDS:
								bOK = image->save_as_dds(filename);
								break;
							case eTGA:
								bOK = image->save_as_tga(filename);
								break;
							case ePNG:
								bOK = image->save_as_png(filename, ncomps);
								break;
							case eJPG:
								bOK = image->save_as_jpg(filename, quality);
								break;
							case ePUG:
								bOK = image->save_as_pug(filename, quality);
								break;
							case eWEBP:
								bOK = image->save_as_webp(filename, quality);
								break;
							case eKTX:
								bOK = image->save_as_ktx(filename, quality);
								break;
							case ePVR:
								bOK = image->save_as_pvr(filename, quality);
								break;
							case eCCZ:
								bOK = image->save_as_ccz(filename, quality);
								break;
							case ePKM:
								bOK = image->save_as_pkm(filename, quality);
								break;
							}
						}

						if (!bOK) luaL_error(L, image->mError.c_str());

						return LuaXS::PushArgAndReturn(L, true);
					}, false);
				}
			}, {
				"set", [](lua_State * L)
				{
					return LuaXS::PCallWithStackThenReturn(L, [](lua_State * L) {
						spot::image * image = Image(L, 1);
						int x = luaL_checkint(L, 2) - 1;

						luaL_argcheck(L, x >= 0 && size_t(x) < image->w, 2, "Invalid x");

						if (lua_isnumber(L, 3))
						{
							int y = luaL_checkint(L, 3) - 1;

							luaL_argcheck(L, y >= 0 && size_t(y) < image->h, 3, "Invalid y");

							if (lua_isnumber(L, 4))
							{
								int z = luaL_checkint(L, 4) - 1;

								luaL_argcheck(L, z >= 0 && size_t(z) < image->d, 4, "Invalid z");

								image->at(size_t(x), size_t(y), size_t(z)) = *Color(L, 5);
							}

							else image->at(size_t(x), size_t(y)) = *Color(L, 4);
						}

						else image->at(size_t(x)) = *Color(L, 3);

						return LuaXS::PushArgAndReturn(L, true);
					}, false);
				}
			}, {
				"setf", [](lua_State * L)
				{
					return LuaXS::PCallWithStackThenReturn(L, [](lua_State * L) {
						spot::image * image = Image(L, 1);
						float x = LuaXS::Float(L, 2);

						luaL_argcheck(L, x >= 0.0f && x <= 1.0f, 2, "Invalid x");

						if (lua_isnumber(L, 3))
						{
							float y = LuaXS::Float(L, 3);

							luaL_argcheck(L, y >= 0.0f && y <= 1.0f, 3, "Invalid y");

							if (lua_isnumber(L, 4))
							{
								float z = LuaXS::Float(L, 4);

								luaL_argcheck(L, z >= 0.0f && z <= 1.0f, 4, "Invalid z");

								image->atf(x, y, z) = *Color(L, 5);
							}

							else image->atf(x, y) = *Color(L, 4);
						}

						else image->atf(x) = *Color(L, 3);

						return LuaXS::PushArgAndReturn(L, true);
					}, false);
				}
			}, {
				"set_from_bytes_rgb", [](lua_State * L)
				{
					return LuaXS::PCallWithStackThenReturn(L, [](lua_State * L) {
						spot::image * image = Image(L, 1);
						int x = luaL_checkint(L, 2) - 1, y = luaL_checkint(L, 3) - 1;

						luaL_argcheck(L, x >= 0 && size_t(x) < image->w, 2, "Invalid x");
						luaL_argcheck(L, y >= 0 && size_t(y) < image->h, 3, "Invalid y");

						size_t nwritten = WriteImageBytes(L, *image, size_t(x), size_t(y), 4, luaL_optint(L, 5, -1), false);

						return LuaXS::PushMultipleArgsAndReturn(L, true, nwritten);
					}, false);
				}
			}, {
				"set_from_bytes_rgba", [](lua_State * L)
				{
					return LuaXS::PCallWithStackThenReturn(L, [](lua_State * L) {
						spot::image * image = Image(L, 1);
						int x = luaL_checkint(L, 2) - 1, y = luaL_checkint(L, 3) - 1;

						luaL_argcheck(L, x >= 0 && size_t(x) < image->w, 2, "Invalid x");
						luaL_argcheck(L, y >= 0 && size_t(y) < image->h, 3, "Invalid y");

						size_t nwritten = WriteImageBytes(L, *image, size_t(x), size_t(y), 4, luaL_optint(L, 5, -1), true);

						return LuaXS::PushMultipleArgsAndReturn(L, true, nwritten);
					}, false);
				}
			}, {
				"set_reader_mode", [](lua_State * L)
				{
					lua_pushliteral(L, "raw");	// spot_image, mode, "raw"

					int use_raw = lua_equal(L, 2, -1);

					lua_settop(L, 1);	// spot_image
					luaL_getmetafield(L, 1, "raw_mode");// spot_image, raw_mode
					lua_insert(L, 1);	// raw_mode, spot_image
					lua_pushboolean(L, use_raw);// raw_mode, spot_image, is_raw
					lua_rawset(L, 1);	// raw_mode[spot_image] = is_raw

					return 0;
				}
			}, {
				"__sub", [](lua_State * L)
				{
					return LuaXS::PCallWithStackThenReturn(L, [](lua_State * L) {
						return InstantiateSpotImage(L, *Image(L, 1) - *Color(L, 2));
					}, LuaXS::Nil{});
				}
			}, {
				"sub_mutate", [](lua_State * L)
				{
					return LuaXS::PCallWithStackThenReturn(L, [](lua_State * L) {
						*Image(L, 1) -= *Color(L, 2);

						return LuaXS::PushArgAndReturn(L, true);
					}, false);
				}
			}, {
				"to_hsla", [](lua_State * L)
				{
					return LuaXS::PCallWithStackThenReturn(L, [](lua_State * L) {
						return InstantiateSpotImage(L, Image(L, 1)->to_hsla());
					}, LuaXS::Nil{});
				}
			}, {
				"to_rgba", [](lua_State * L)
				{
					return LuaXS::PCallWithStackThenReturn(L, [](lua_State * L) {
						return InstantiateSpotImage(L, Image(L, 1)->to_rgba());
					}, LuaXS::Nil{});
				}
			}, {
				"unpremultiply", [](lua_State * L)
				{
					return LuaXS::PCallWithStackThenReturn(L, [](lua_State * L) {
						spot::image * image = Image(L, 1);
						spot::rect<spot::color> pic{image->w, image->h};

						pic.resize(0);

						pic.delay = image->delay;
						pic.space = image->space;

						for (const auto &it : *image) pic.push_back(it.unpremultiply());

						return InstantiateSpotImage(L, pic);
					}, LuaXS::Nil{});
				}
			}, {
				"unpremultiply_mutate", [](lua_State * L)
				{
					return LuaXS::PCallWithStackThenReturn(L, [](lua_State * L) {
						spot::image * image = Image(L, 1);

						for (auto &it : *image) it = it.unpremultiply();

						return LuaXS::PushArgAndReturn(L, true);
					}, false);
				}
			}, {
				"write_to_blob", [](lua_State * L)
				{
					return LuaXS::PCallWithStackThenReturn(L, [](lua_State * L) {
						BlobXS::State state{L, 2};

						spot::image * image = Image(L, 1);
						int h = image->h ? image->h : 1, d = image->d ? image->d : 1;

						unsigned char * pdata = state.PointToDataIfBound(L, 0, 0, image->w, h, image->w * 4, 4);

						if (!pdata) return luaL_error(L, "Expected blob (large enough or resizable)");

						auto rgba = image->rgba();

						if (state.mNoResize || BlobXS::GetAlignment(L, 2) != 0U) memcpy(pdata, rgba.data(), rgba.size() * 4U);

						else
						{
							std::vector<unsigned char> * pvec = BlobXS::GetVectorN<0U>(L, 2);

							pvec->swap(rgba);
						}

						return LuaXS::PushArgAndReturn(L, true);
					}, LuaXS::Nil{});
				}
			},
			{ nullptr, nullptr }
		};

		luaL_register(L, nullptr, spot_image_methods);

		LuaXS::AttachProperties(L, [](lua_State * L)
		{
			if (lua_type(L, 2) == LUA_TSTRING)
			{
				spot::image * image = Image(L, 1);
				std::string what = lua_tostring(L, 2);

				if (what == "width") lua_pushnumber(L, image->w);	// image, key, width
				else if (what == "height") lua_pushnumber(L, image->h);	// image, key, height
				else if (what == "depth") lua_pushnumber(L, image->d);	// image, key, depth
				else if (what == "space") lua_pushstring(L, image->space == spot::SPACE_RGBA ? "RGBA" : "HSLA");// image, key, space
			}

			lua_settop(L, 3);	// image, key, value / nil

			return 1;
		});	// ..., meta = { ..., __index = GetProperties() -> index }

		ByteReaderFunc * func = ByteReader::Register(L);

		func->mGetBytes = [](lua_State * L, ByteReader & reader, int arg, void *)
		{
			luaL_getmetafield(L, arg, "raw_mode");	// ..., raw_mode
			lua_pushvalue(L, arg);	// ..., raw_mode, spot_image
			lua_rawget(L, -2);	// ..., raw_mode, is_raw

			bool bIsRaw = lua_toboolean(L, -1) != 0;

			lua_pop(L, 2);	// ...

			if (bIsRaw) VectorReader(L, reader, arg, nullptr);

			else
			{
				arg = CoronaLuaNormalize(L, arg);

				if (!LuaXS::PCallN(L, [](lua_State * L)
				{
					auto rgba = Image(L, 1)->rgba();

					lua_pushlstring(L, reinterpret_cast<const char *>(rgba.data()), rgba.size());	// image, rgba

					return 1;
				}, 1, LuaXS::StackIndex(L, arg))) return false; // ..., raw_mode, is_raw, rgba / err

				reader.mBytes = lua_tostring(L, -1);
				reader.mCount = lua_objlen(L, -1);
			}

			return true;
		};
		func->mContext = nullptr;

		LuaXS::NewWeakKeyedTable(L);// ..., mt, raw_mode

		lua_setfield(L, -2, "raw_mode");// ..., mt = { raw_mode = raw_mode }

		LuaXS::SetField(L, -1, "__bytes", func);// ..., mt = { ..., raw_mode, __bytes = reader_func }
		LuaXS::SetField(L, -1, "__metatable", "spot.image");// ..., mt = { ..., raw_mode, __bytes, __metatable = "blob" }
	});

	return 1;
}
