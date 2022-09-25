#include "tinyrenderer.h"
#include "utils/LuaEx.h"

//
//
//

#define TEXTURE_TYPE "scene3d.tiny.Texture"
#define FLOAT_TEXTURE_TYPE "scene3d.tiny.FloatTexture"

//
//
//

namespace tiny {

//
//
//
	
Texture<unsigned char> & GetTexture (lua_State * L, int arg)
{
    return *LuaXS::CheckUD<Texture<unsigned char>>(L, arg, TEXTURE_TYPE);
}

//
//
//

Texture<float> & GetFloatTexture (lua_State * L, int arg)
{
    return *LuaXS::CheckUD<Texture<float>>(L, arg, FLOAT_TEXTURE_TYPE);
}

//
//
//

void open_texture (lua_State * L)
{
    AddConstructor(L, "NewTexture", [](lua_State * L)
	{
		LuaXS::NewTyped<Texture<unsigned char>>(L);	// texture
		LuaXS::AttachMethods(L, TEXTURE_TYPE, [](lua_State * L) {
			luaL_Reg methods[] = {
				{
					"Bind", [](lua_State * L)
					{
						int comps = luaL_optint(L, 5, 3);
                                
						GetTexture(L).Bind(L, 2, LuaXS::Int(L, 3), LuaXS::Int(L, 4), comps, &Texture<unsigned char>::AssignOrRef);

						return 0;
					}
				}, {
					"__gc", LuaXS::TypedGC<Texture<unsigned char>>
				},
				{ nullptr, nullptr }
			};

			luaL_register(L, nullptr, methods);
		});

		return 1;
	});

	AddConstructor(L, "NewFloatTexture", [](lua_State * L)
	{
		LuaXS::NewTyped<Texture<float>>(L);	// texture
		LuaXS::AttachMethods(L, FLOAT_TEXTURE_TYPE, [](lua_State * L) {
			luaL_Reg methods[] = {
				{
					"Bind", [](lua_State * L)
					{
						int w = LuaXS::Int(L, 3), h = LuaXS::Int(L, 4), comps = luaL_optint(L, 5, 3);
                            
						luaL_argcheck(L, comps >= 3, 5, "Not enough components for normal map");
         
						GetFloatTexture(L).Bind(L, 2, w, h, comps, [](lua_State *, Texture<float> & tex, const ByteReader & reader, size_t n) {
							auto from = static_cast<const unsigned char *>(reader.mBytes);
                                
							for (size_t i = 0; i < n; ++i) tex.mPixels[i] = 2.0f * (float(from[i]) / 255.0f) - 1.0f;
						});

						return 0;
					}
				}, {
					"__gc", LuaXS::TypedGC<Texture<float>>
				},
				{ nullptr, nullptr }
			};

			luaL_register(L, nullptr, methods);
		});

		return 1;
	});
}

//
//
//

}
