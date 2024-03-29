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
#include "ByteReader.h"
#include "utils/Blob.h"
#include "utils/LuaEx.h"
#include <vector>

#define CANVAS_ITY_IMPLEMENTATION
#include "canvas_ity.hpp"

//
//
//

#define CANVAS_ITY_METATABLE_NAME "metatable.canvas_ity"

//
//
//

static int sTextureRefs;

//
//
//

struct Texture {
	using Vector = std::vector<unsigned char>;

	Texture (Vector & output, canvas_ity::canvas & owner, lua_State * L) : mOutput{output}, mOwner{owner}, mL{L}
	{
	}
	
	Vector & mOutput;
	canvas_ity::canvas & mOwner;
	lua_State * mL;
};

//
//
//

static canvas_ity::canvas & Get (lua_State * L)
{
	return *(canvas_ity::canvas *)luaL_checkudata(L, 1, CANVAS_ITY_METATABLE_NAME);
}

//
//
//

static unsigned int Texture_GetW (void * context)
{
	return static_cast<Texture *>(context)->mOwner.get_size_x();
}

static unsigned int Texture_GetH (void * context)
{
	return static_cast<Texture *>(context)->mOwner.get_size_y();
}

static const void * Texture_GetData (void * context)
{
	Texture * tex = static_cast<Texture *>(context);
	canvas_ity::canvas & owner = tex->mOwner;
	int stride = owner.get_size_x() * 4, size = stride * owner.get_size_y();

	if (tex->mOutput.size() < size) tex->mOutput.resize(size);

	owner.get_image_data(tex->mOutput.data(), owner.get_size_x(), owner.get_size_y(), stride, 0, 0);

	uint32_t * pixel = reinterpret_cast<uint32_t *>(tex->mOutput.data());
	
	// FastPremult() from https://arxiv.org/pdf/2202.02864v1.pdf
	for (int i = 0, len = owner.get_size_x() * owner.get_size_y(); i < len; ++i, ++pixel)
	{
		uint32_t color = *pixel;
		uint32_t alfa = color >> 24;
		uint32_t rb, ga;

		color |= 0xff000000;

		rb = color & 0x00ff00ff;
		rb *= alfa;
		rb += 0x00800080;
		rb += (rb >> 8) & 0x00ff00ff;
		rb &= 0xff00ff00;
		ga = (color >> 8) & 0x00ff00ff;
		ga *= alfa;
		ga += 0x00800080;
		ga += (ga >> 8) & 0x00ff00ff;
		ga &= 0xff00ff00;

		*pixel = ga | (rb >> 8);
	}
	
	return tex->mOutput.data();
}

static CoronaExternalBitmapFormat Texture_Format (void * context)
{
	return CoronaExternalBitmapFormat::kExternalBitmapFormat_RGBA;
}

static void Texture_Dispose (void * context)
{
	Texture * tex = static_cast<Texture *>(context);

	lua_getref(tex->mL, sTextureRefs); // ..., refs

	for (lua_pushnil(tex->mL); lua_next(tex->mL, -2); lua_pop(tex->mL, 1))
	{
		if (lua_touserdata(tex->mL, -2) == &tex->mOwner)
		{
			lua_pop(tex->mL, 1); // ..., refs, canvas
			lua_pushnil(tex->mL); // ..., refs, canvas, nil
			lua_rawset(tex->mL, -3); // ..., refs = { ..., [canvas] = nil }
			lua_pop(tex->mL, 1); // ...

			break;
		}
	}
	
	delete tex;
}

//
//
//

static int GetTexture (lua_State * L)
{
	lua_getref(L, sTextureRefs); // canvas, refs
	lua_pushvalue(L, 1); // canvas, refs, canvas
	lua_rawget(L, -2); // canvas, refs, texture?
				
	if (lua_isnil(L, -1))
	{
		CoronaExternalTextureCallbacks callbacks = {};

		callbacks.size = sizeof(CoronaExternalTextureCallbacks);
		callbacks.getFormat = Texture_Format;
		callbacks.getHeight = Texture_GetH;
		callbacks.getWidth = Texture_GetW;
		callbacks.onFinalize = Texture_Dispose;
		callbacks.onRequestBitmap = Texture_GetData;

		canvas_ity::canvas & canvas = Get(L);
		Texture * tex = new Texture{*LuaXS::UD<Texture::Vector>(L, lua_upvalueindex(1)), canvas, L};

		if (CoronaExternalPushTexture(L, &callbacks, tex)) // canvas, refs, nil[, texture]
		{
			lua_pushvalue(L, -1); // canvas, refs, nil, texture, texture
			lua_insert(L, -3); // canvas, refs, texture, nil, texture
			lua_pushvalue(L, 1); // canvas, refs, texture, nil, texture, canvas
			lua_replace(L, -3); // canvas, refs, texture, canvas, texture
			lua_rawset(L, -4); // canvas, refs, texture; refs[canvas] = texture
		}
									
		else delete tex; // n.b. nil already on top
	}
					
	return 1;
}

//
//
//

#define ENUM(x) canvas_ity::x
#define NAME(x) #x

//
//
//

canvas_ity::composite_operation GetCompositeOperation (lua_State * L, int arg)
{
    #define COMPOSITE_OPS(OP) OP(source_in), OP(source_copy), OP(source_out), OP(destination_in), \
                                OP(destination_atop), OP(lighter), OP(destination_over), OP(destination_out), \
                                OP(source_atop), OP(source_over), OP(exclusive_or)

    const char * names[] = { COMPOSITE_OPS(NAME), nullptr };
    canvas_ity::composite_operation ops[] = { COMPOSITE_OPS(ENUM) };

    return ops[luaL_checkoption(L, arg, nullptr, names)];
}

canvas_ity::cap_style GetCapStyle (lua_State * L, int arg)
{
    #define CAP_STYLES(OP) OP(butt), OP(square), OP(circle)

    const char * names[] = { CAP_STYLES(NAME), nullptr };
    canvas_ity::cap_style styles[] = { CAP_STYLES(ENUM) };

    return styles[luaL_checkoption(L, arg, nullptr, names)];
}

canvas_ity::join_style GetJoinStyle (lua_State * L, int arg)
{
    #define JOIN_STYLES(OP) OP(miter), OP(bevel), OP(rounded)

    const char * names[] = { JOIN_STYLES(NAME), nullptr };
    canvas_ity::join_style styles[] = { JOIN_STYLES(ENUM) };

    return styles[luaL_checkoption(L, arg, nullptr, names)];
}

canvas_ity::brush_type GetBrushType (lua_State * L, int arg)
{
    #define BRUSH_TYPES(OP) OP(fill_style), OP(stroke_style)

    const char * names[] = { BRUSH_TYPES(NAME), nullptr };
    canvas_ity::brush_type types[] = { BRUSH_TYPES(ENUM) };

    return types[luaL_checkoption(L, arg, nullptr, names)];
}

canvas_ity::repetition_style GetRepetitionStyle (lua_State * L, int arg)
{
    #define REPETITION_STYLES(OP) OP(repeat), OP(repeat_x), OP(repeat_y), OP(no_repeat)

    const char * names[] = { REPETITION_STYLES(NAME), nullptr };
    canvas_ity::repetition_style styles[] = { REPETITION_STYLES(ENUM) };

    return styles[luaL_checkoption(L, arg, nullptr, names)];
}

canvas_ity::align_style GetAlignStyle (lua_State * L, int arg)
{
    #define ALIGN_STYLES(OP) OP(leftward), OP(rightward), OP(center), OP(start), OP(ending)

    const char * names[] = { ALIGN_STYLES(NAME), nullptr };
    canvas_ity::align_style styles[] = { ALIGN_STYLES(ENUM) };

    return styles[luaL_checkoption(L, arg, nullptr, names)];
}

canvas_ity::baseline_style GetBaselineStyle (lua_State * L, int arg)
{
    #define BASELINE_STYLES(OP) OP(alphabetic), OP(top), OP(middle), OP(bottom), OP(hanging), OP(ideographic)

    const char * names[] = { BASELINE_STYLES(NAME), nullptr };
    canvas_ity::baseline_style styles[] = { BASELINE_STYLES(ENUM) };

    return styles[luaL_checkoption(L, arg, nullptr, names)];

}

//
//
//

static float * GetMember (lua_State * L, const char * what)
{
	#define MEMBER(NAME) #NAME, &canvas_ity::canvas::NAME

	struct {
		const char * name;
		float canvas_ity::canvas::*member;
	} values[] = {
		MEMBER(line_dash_offset),
		MEMBER(shadow_offset_x),
		MEMBER(shadow_offset_y)
	};

	for (int i = 0; i < sizeof(values) / sizeof(values[0]); ++i)
	{
		if (strcmp(what, values[i].name) == 0) return &(Get(L).*values[i].member);
	}
	
	return nullptr;
}

//
//
//

#define GET_FOUR_FLOATS(FIRST) FLOAT(FIRST), FLOAT(FIRST + 1), FLOAT(FIRST + 2), FLOAT(FIRST + 3)
#define GET_FIVE_FLOATS(FIRST) GET_FOUR_FLOATS(FIRST), FLOAT(FIRST + 4)
#define GET_SIX_FLOATS(FIRST) GET_FIVE_FLOATS(FIRST), FLOAT(FIRST + 5)

//
//
//

#define CANVAS Get(L)
#define LSIG [](lua_State * L)
#define FLOAT(ARG) LuaXS::Float(L, ARG)
#define NO_ARGS(NAME) #NAME, LSIG { CANVAS.NAME(); return 0; }
#define ONE_FLOAT(NAME) #NAME, LSIG { CANVAS.NAME(FLOAT(2)); return 0; }
#define TWO_FLOATS(NAME) #NAME, LSIG { CANVAS.NAME(FLOAT(2), FLOAT(3)); return 0; }
#define THREE_FLOATS(NAME) #NAME, LSIG { CANVAS.NAME(FLOAT(2), FLOAT(3), FLOAT(4)); return 0; }
#define FOUR_FLOATS(NAME) #NAME, LSIG { CANVAS.NAME(GET_FOUR_FLOATS(2)); return 0; }
#define FIVE_FLOATS(NAME) #NAME, LSIG { CANVAS.NAME(GET_FIVE_FLOATS(2)); return 0; }
#define SIX_FLOATS(NAME) #NAME, LSIG { CANVAS.NAME(GET_SIX_FLOATS(2)); return 0; }

//
//
//

CORONA_EXPORT int luaopen_plugin_canvasity (lua_State* L)
{
	lua_newtable(L); // canvas_ity
	
    luaL_Reg funcs[] = {
        {
            "New", [](lua_State * L)
            {
                int w = luaL_checkint(L, 1), h = luaL_checkint(L, 2);

                LuaXS::NewTyped<canvas_ity::canvas>(L, w, h); // w, h, canvas
                LuaXS::AttachMethods(L, CANVAS_ITY_METATABLE_NAME, [](lua_State * L) {
                    luaL_Reg funcs[] = {
						{
							"add_color_stop", [](lua_State * L)
							{
								Get(L).add_color_stop(GetBrushType(L, 2), GET_FIVE_FLOATS(3));
								
								return 0;
							}
						}, {
							"arc", [](lua_State * L)
							{
								Get(L).arc(GET_FIVE_FLOATS(2), lua_toboolean(L, 7));
								
								return 0;
							}
						}, {
							FIVE_FLOATS(arc_to)
						}, {
							NO_ARGS(begin_path)
						}, {
							SIX_FLOATS(bezier_curve_to)
						}, {
							NO_ARGS(clip)
						}, {
							FOUR_FLOATS(clear_rectangle)
						}, {
							NO_ARGS(close_path)
						}, {
                            "draw_image", [](lua_State * L)
                            {
								ByteReader bytes{L, 2};
								int w = luaL_checkint(L, 3), h = luaL_checkint(L, 4), stride = luaL_checkint(L, 5);
								
								if (bytes.mCount < stride * h) return 0;
								
								Get(L).draw_image(static_cast<const unsigned char *>(bytes.mBytes), w, h, stride, GET_FOUR_FLOATS(6));

                                return 0;
                            }
						}, {
							NO_ARGS(fill)
						}, {
							FOUR_FLOATS(fill_rectangle)
						}, {
							"fill_text", [](lua_State * L)
							{
								Get(L).fill_text(lua_tostring(L, 2), FLOAT(3), FLOAT(4), (float)luaL_optnumber(L, 5, 1.0e30));

								return 0;
							}
						}, {
                            "__gc", LuaXS::TypedGC<canvas_ity::canvas>
                        }, {
                            "get_image_data", [](lua_State * L)
                            {
								bool is_blob = BlobXS::IsBlob(L, 2), has_output = is_blob || lua_istable(L, 2);
								int warg = (has_output || lua_isnil(L, 2)) ? 3 : 2, w = luaL_checkint(L, warg), h = luaL_checkint(L, warg + 1), stride = luaL_checkint(L, warg + 2);
								
								//
								//
								//
								
								std::vector<unsigned char> image;
								unsigned char * data = nullptr;
								
								if (is_blob && BlobXS::GetSize(L, 2) >= stride * h) data = BlobXS::GetData(L, 2);
								
								else if (!lua_isnil(L, 2))
								{
									image.resize(stride * h);
									
									data = image.data();
								}
								
								//
								//
								//
								
								int x = luaL_checkint(L, warg + 3), y = luaL_checkint(L, warg + 4);
								
								Get(L).get_image_data(data, w, h, stride, x, y);
								
								//
								//
								//
								
								if (lua_istable(L, 2))
								{
									lua_getfield(L, 2, "offset"); // canvas, t, w, h, stride, x, y, offset?
									
									int offset = static_cast<int>(luaL_optinteger(L, -1, 0));
									
									for (int row = 0, first = 0; row < h; ++row, first += stride)
									{
										for (int pos = first, i = 0; i < w * 4; ++i)
										{
											lua_pushinteger(L, data[pos++]); // canvas, t, w, h, stride, x, y, offset?, byte
											lua_rawseti(L, 2, pos + offset); // canvas, t = { ..., [pos] = byte, ... }, w, h, stride, x, y, offset?
										}
									}
								}
								
								//
								//
								//
								
								bool output_ok = image.empty() || lua_istable(L, 2);
								
								lua_pushboolean(L, output_ok); // canvas[, blob], w, h, stride, x, y, blob_ok
								
								if (!output_ok) lua_pushlstring(L, reinterpret_cast<char *>(image.data()), image.size()); // canvas[, blob], w, h, stride, x, y, blob_ok[, data]
								
								return output_ok ? 1 : 2;
                            }
						}, {
							"is_point_in_path", [](lua_State * L)
							{
								lua_pushboolean(L, Get(L).is_point_in_path(FLOAT(2), FLOAT(3))); // canvas, x, y, in_path
								
								return 1;
							}
						}, {
							TWO_FLOATS(line_to)
						}, {
							"measure_text", [](lua_State * L)
							{
								lua_pushnumber(L, Get(L).measure_text(luaL_optstring(L, 2, ""))); // canvas, str?, width
								
								return 1;
							}
						}, {
							TWO_FLOATS(move_to)
						}, {
							"__newindex", [](lua_State * L)
							{
								if (LUA_TSTRING != lua_type(L, 2)) return 0;
										
								#define SET_IF(NAME, VALUE) if (strcmp(what, #NAME) == 0) Get(L).NAME = VALUE
								
								const char * what = lua_tostring(L, 2);
								
								//
								//
								//
															
								SET_IF(global_composite_operation, GetCompositeOperation(L, 3));
															
								//
								//
								//
															
								else SET_IF(line_cap, GetCapStyle(L, 3));
															
								//
								//
								//
															
								else SET_IF(line_join, GetJoinStyle(L, 3));
															
								//
								//
								//
															
								else SET_IF(text_align, GetAlignStyle(L, 3));
															
								//
								//
								//
															
								else SET_IF(text_baseline, GetBaselineStyle(L, 3));
															
								//
								//
								//
															
								else
								{
									float * member = GetMember(L, what);
									
									if (member) *member = FLOAT(3);
								}

								//
								//
								//
								
								return 0;
							}	
						}, {
                            "put_image_data", [](lua_State * L)
                            {
								ByteReader bytes{L, 2};
								int w = luaL_checkint(L, 3), h = luaL_checkint(L, 4), stride = luaL_checkint(L, 5);
								
								//if (bytes.mCount < stride * h) return 0;
									// ^^^ TODO: more rigorous
								
								Get(L).put_image_data(static_cast<const unsigned char *>(bytes.mBytes), w, h, stride, luaL_checkint(L, 6), luaL_checkint(L, 7));

								return 0;
                            }
						}, {
							FOUR_FLOATS(quadratic_curve_to)
						}, {
							FOUR_FLOATS(rectangle)
						}, {
							NO_ARGS(reset_bitmap)
						}, {
                            NO_ARGS(restore)
						}, {
							ONE_FLOAT(rotate)
						}, {
                            NO_ARGS(save)
						}, {
							TWO_FLOATS(scale)
						}, {
							"set_color", [](lua_State * L)
							{
								Get(L).set_color(GetBrushType(L, 2), GET_FOUR_FLOATS(3));
								
								return 0;
							}
						}, {
							"set_font", [](lua_State * L)
							{
								ByteReader bytes{L, 2};

								lua_pushboolean(L, Get(L).set_font(static_cast<const unsigned char *>(bytes.mBytes), int(bytes.mCount), FLOAT(3))); // canvas, font, size, ok

								return 1;
							}
						}, {
							ONE_FLOAT(set_global_alpha)
						}, {
							"set_linear_gradient", [](lua_State * L)
							{
								Get(L).set_linear_gradient(GetBrushType(L, 2), GET_FOUR_FLOATS(3));
								
								return 0;
							}
						}, {
							"set_line_dash", [](lua_State * L)
							{
								
								if (!lua_isnoneornil(L, 2))
								{
									std::vector<float> segments;
									
									luaL_checktype(L, 2, LUA_TTABLE);
									
									for (size_t i = 1, n = lua_objlen(L, 2); i <= n; ++i, lua_pop(L, 1))
									{
										lua_rawgeti(L, 2, int(i)); // canvas, segments, len
										
										segments.push_back(FLOAT(-1));
									}
									
									Get(L).set_line_dash(segments.data(), int(segments.size()));
								}
								
								else Get(L).set_line_dash(nullptr, 0);
								
								return 0;
							}
						}, {
							ONE_FLOAT(set_line_width)
						}, {
							ONE_FLOAT(set_miter_limit)
						}, {
							"set_pattern", [](lua_State * L)
							{
								ByteReader bytes{L, 3};
								
								int w = luaL_checkint(L, 4), h = luaL_checkint(L, 5), stride = luaL_checkint(L, 6);
								
								if (bytes.mCount < stride * h) return 0;
								
								Get(L).set_pattern(GetBrushType(L, 2), static_cast<const unsigned char *>(bytes.mBytes), w, h, stride, GetRepetitionStyle(L, 7));
								
								return 0;
							}
						}, {
							"set_radial_gradient", [](lua_State * L)
							{
								Get(L).set_radial_gradient(GetBrushType(L, 2), GET_SIX_FLOATS(3));
								
								return 0;
							}
						}, {
							ONE_FLOAT(set_shadow_blur)
						}, {
							FOUR_FLOATS(set_shadow_color)
						}, {
							SIX_FLOATS(set_transform)
						}, {
							NO_ARGS(stroke)
						}, {
							FOUR_FLOATS(stroke_rectangle)
						}, {
							"stroke_text", [](lua_State * L)
							{
								Get(L).stroke_text(lua_tostring(L, 2), FLOAT(3), FLOAT(4), (float)luaL_optnumber(L, 5, 1.0e30));

								return 0;
							}
						}, {
							SIX_FLOATS(transform)
						}, {
							TWO_FLOATS(translate)
						},
                        { nullptr, nullptr }
                    };

                    luaL_register(L, nullptr, funcs);

					//
					//
					//

					LuaXS::NewTyped<Texture::Vector>(L); // mt, texture_data
					LuaXS::AttachGC(L, LuaXS::TypedGC<Texture::Vector>);

					lua_pushcclosure(L, GetTexture, 1); // mt, GetTexture
					lua_setfield(L, -2, "get_texture"); // mt = { ..., get_texture = GetTexture }

					//
					//
					//

					LuaXS::AttachProperties(L, [](lua_State * L) {
						if (LUA_TSTRING != lua_type(L, 2)) return 0;
									
						#define CASE(NAME) case canvas_ity::NAME: lua_pushliteral(L, #NAME); return 1
									
						const char * what = lua_tostring(L, 2);
		
						//
						//
						//
									
						if (strcmp(what, "global_composite_operation") == 0)
						{
							switch (Get(L).global_composite_operation)
							{
								CASE(source_atop);
								CASE(source_copy);
								CASE(source_in);
								CASE(source_out);
								CASE(source_over);
								CASE(destination_atop);
								CASE(destination_in);
								CASE(destination_out);
								CASE(destination_over);
								CASE(exclusive_or);
								CASE(lighter);
							default:
								return luaL_error(L, "Invalid global composite operation");
							}
						}
									
						//
						//
						//
									
						else if (strcmp(what, "line_cap") == 0)
						{
							switch (Get(L).line_cap)
							{
								CASE(butt);
								CASE(square);
								CASE(circle);
							default:
								return luaL_error(L, "Invalid line cap");
							}
						}
									
						//
						//
						//
									
						else if (strcmp(what, "line_join") == 0)
						{
							switch (Get(L).line_join)
							{
								CASE(miter);
								CASE(bevel);
								CASE(rounded);
							default:
								return luaL_error(L, "Invalid line join");
							}
						}
									
						//
						//
						//
									
						else if (strcmp(what, "text_align") == 0)
						{
							switch (Get(L).text_align)
							{
								CASE(leftward);
								CASE(rightward);
								CASE(center);
							default:
								return luaL_error(L, "Invalid text align");
							}
						}
									
						//
						//
						//
									
						else if (strcmp(what, "text_baseline") == 0)
						{
							switch (Get(L).text_baseline)
							{
								CASE(alphabetic);
								CASE(top);
								CASE(middle);
								CASE(bottom);
								CASE(hanging);
							default:
								return luaL_error(L, "Invalid text baseline");
							}
						}
									
						//
						//
						//
									
						else
						{
							float * member = GetMember(L, what);
			
							if (member)
							{
								lua_pushnumber(L, *member); // canvas, what, value
					
								return 1;
							}
						}

						//
						//
						//
									
						return 0;
					});
                });

				//
				//
				//

                return 1;
            }
        },
        { nullptr, nullptr }
    };

    luaL_register(L, nullptr, funcs);

	//
	//
	//
	
	lua_newtable(L); // canvas_ity, texture_refs
	
	sTextureRefs = lua_ref(L, 1); // canvas_ity; ref = texture_refs
	
	//
	//
	//
	
	return 1;
}
