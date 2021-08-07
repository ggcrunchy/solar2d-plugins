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

#include "common.h"
#include "noiseutils/noiseutils.h"

//
//
//

#define lua_tofloat(L, n) (float)lua_tonumber(L, n)

//
//
//

#define UTILS_GETTER(MNAME) static noise::utils::MNAME * MNAME (lua_State * L)	\
{																		\
	return LuaXS::CheckUD<noise::utils::MNAME>(L, 1, MT_NAME(MNAME));	\
}

//
//
//

static noise::uint8 Component (lua_State * L, int arg, bool opt = false)
{
    lua_Number comp = opt ? luaL_optnumber(L, arg, 1.0) : luaL_checknumber(L, arg);

    if (comp < 0.0) comp = 0.0;
    if (comp > 1.0) comp = 1.0;

    return noise::uint8(comp * 255.0);
}

static int SetColorMT (lua_State * L)
{
    luaL_newmetatable(L, MT_NAME(Color));   // color, mt
    lua_setmetatable(L, -2);// color

    return 1;
}

//
//
//

UTILS_GETTER(Color)

static int AddColor (lua_State * L)
{
    if (!lua_isnoneornil(L, 1))
    {
        noise::uint8 r = Component(L, 1), g = Component(L, 2), b = Component(L, 3), a = Component(L, 4, true);

        LuaXS::NewTyped<noise::utils::Color>(L, r, g, b, a);   // r, g, b[, a], color
    }

    else LuaXS::NewTyped<noise::utils::Color>(L);   // color

    return SetColorMT(L);
}

//
//
//

static noise::utils::Color GetColor (lua_State * L, int arg) // kludge to allow some other templates
{
    if (arg > 1)
    {
        lua_pushvalue(L, arg);  // ..., color, ..., color
        lua_insert(L, 1);   // color, ..., color, ...
    }

    const noise::utils::Color & color = *Color(L);

    if (arg > 1) lua_remove(L, 1);  // ..., color, ...

    return color;
}

//
//
//

static int PushColor (lua_State * L, const noise::utils::Color & color)
{
    LuaXS::NewTyped<noise::utils::Color>(L, color);   // ..., color

    return 1;
}

template<typename F>
int AddGradientPoint (lua_State * L, F && getter)
{
    try {
        getter(L)->AddGradientPoint(lua_tonumber(L, 2), GetColor(L, 3));
    } catch (noise::ExceptionInvalidParam &) {
        // TODO: out-of-sequence error...
    }

    return 0;
}

//
//
//

UTILS_GETTER(GradientColor)

static int AddGradientColor (lua_State * L)
{
    auto * color = LuaXS::NewTyped<noise::utils::GradientColor>(L);   // gcolor

    LuaXS::AttachMethods(L, MT_NAME(GradientColor), [](lua_State * L) {
        luaL_Reg funcs[] = {
            {
                "AddGradientPoint", [](lua_State * L)
                {
                    return AddGradientPoint(L, GradientColor);
                }
            }, {
                DO(GradientColor, Clear)
            }, {
                "GetColor", [](lua_State * L)
                {
                    return PushColor(L, GradientColor(L)->GetColor(lua_tonumber(L, 2)));// gcolor, pos, color
                }
            }, {
                "GetBytes", [](lua_State * L)
                {
                    auto * color = GradientColor(L);
                    int n = color->GetGradientPointCount();

                    if (n > 0) lua_pushlstring(L, reinterpret_cast<const char *>(color->GetGradientPointArray()), sizeof(noise::utils::GradientPoint) * n); // gcolor, bytes
                    else lua_pushliteral(L, "");// gcolor, ""

                    return 1;
                }
            }, {
                GET_VALUE(GradientColor, GradientPointCount, integer)
            },
            { nullptr, nullptr }
        };

        luaL_register(L, nullptr, funcs);
    });

    return 1;
}

//
//
//

template<typename T>
T * NewMaybeSized (lua_State * L)
{
    int w = -1, h;

    if (!lua_isnoneornil(L, 1))
    {
        w = LuaXS::Int(L, 1);
        h = LuaXS::Int(L, 2);

        luaL_argcheck(L, w > 0, 1, "Invalid width");
        luaL_argcheck(L, h > 0, 2, "Invalid height");
    }

	return w > 0 ? new T{w, h} : new T;
}

template<typename T>
T * LuaNewMaybeSized (lua_State * L)
{
    int w = -1, h;

    if (!lua_isnoneornil(L, 1))
    {
        w = LuaXS::Int(L, 1);
        h = LuaXS::Int(L, 2);

        luaL_argcheck(L, w > 0, 1, "Invalid width");
        luaL_argcheck(L, h > 0, 2, "Invalid height");
    }

	return w > 0 ? LuaXS::NewTyped<T>(L, w, h) : LuaXS::NewTyped<T>(L);
}

//
//
//

template<typename T>
int PushLengthString (lua_State * L, const T * slab, int n)
{
    lua_pushlstring(L, reinterpret_cast<const char *>(slab), sizeof(T) * n); // ..., bytes

    return 1;
}

template<typename T, T * (*getter)(lua_State *)>
int GetSlabBytes (lua_State * L)
{
    auto * object = getter(L);

    if (lua_isnoneornil(L, 2)) return PushLengthString(L, object->GetSlabPtr(), object->GetStride() * object->GetHeight());   // object, bytes
    else if (!lua_isnoneornil(L, 3)) return PushLengthString(L, object->GetSlabPtr(LuaXS::Int(L, 2) - 1, LuaXS::Int(L, 3) - 1), 1);// object, x, y, bytes
    else return PushLengthString(L, object->GetSlabPtr(LuaXS::Int(L, 2) - 1), object->GetStride());   // object, row, bytes
}

#define GET_BYTES(getter) GetSlabBytes<noise::utils::getter, &getter>

//
//
//

static noise::utils::Image * Image (lua_State * L)
{
    return LuaXS::ExtUD<noise::utils::Image>(L, 1);
}

//
//
//

#define GET_COLOR(getter, name) "Get" #name, [](lua_State * L) \
{                                           \
    PushColor(L, getter(L)->Get##name());   \
                                            \
    return 1;                               \
}

#define GET_COLOR_AT(getter, name) "Get" #name, [](lua_State * L) \
{                                                                                   \
    return PushColor(L, getter(L)->Get##name(LuaXS::Int(L, 2), LuaXS::Int(L, 3)));  \
}

#define WITH_COLOR(getter, name, mname) name, [](lua_State * L) \
{                                       \
    getter(L)->mname(GetColor(L, 2));   \
                                        \
    return 0;                           \
}

#define COLOR_METHOD(getter, name) WITH_COLOR(getter, #name, name)
#define SET_COLOR(getter, name) WITH_COLOR(getter, "Set" #name, Set##name)

#define SET_COLOR_AT(getter, name) "Set" #name, [](lua_State * L) \
{                                                                               \
    getter(L)->Set##name(LuaXS::Int(L, 2), LuaXS::Int(L, 3), GetColor(L, 4));   \
                                                                                \
    return 0;                                                                   \
}

#define SET_SIZE(getter, name) "Set" #name, [](lua_State * L) \
{                                                               \
    getter(L)->Set##name(LuaXS::Int(L, 2), LuaXS::Int(L, 3));   \
                                                                \
    return 0;                                                   \
}

//
//
//

static unsigned int Image_GetW (void * context)
{
	return static_cast<noise::utils::Image *>(context)->GetWidth();
}

static unsigned int Image_GetH (void * context)
{
	return static_cast<noise::utils::Image *>(context)->GetHeight();
}

static const void * Image_GetData (void * context)
{
	noise::utils::Image * image = static_cast<noise::utils::Image *>(context);

	return image->GetConstSlabPtr();
}

static CoronaExternalBitmapFormat Image_Format (void *)
{
	return kExternalBitmapFormat_RGBA;
}

static void Image_Dispose (void * context)
{
	noise::utils::Image * image = static_cast<noise::utils::Image *>(context);

	delete image;
}

//
//
//

static int Image_GetField (lua_State * L, const char * field, void * context)
{
	noise::utils::Image * image = static_cast<noise::utils::Image *>(context);

    luaL_Reg methods[] = {
        {
            COLOR_METHOD(Image, Clear)
        }, {
            GET_COLOR(Image, BorderValue)
        }, {
            "GetBytes", GET_BYTES(Image)
        }, {
            GET_VALUE(Image, Height, integer)
        }, {
            GET_VALUE(Image, Stride, integer)
        }, {
            GET_COLOR_AT(Image, Value)
        }, {
            GET_VALUE(Image, Width, integer)
        }, {
            SET_COLOR(Image, BorderValue)
        }, {
            SET_SIZE(Image, Size)
        }, {
            SET_COLOR_AT(Image, Value)
        },
        { nullptr, nullptr }
    };

    for (int i = 0; methods[i].func; ++i)
    {
        if (strcmp(field, methods[i].name) == 0)
        {
            lua_pushcfunction(L, methods[i].func);  // image, func

            return 1;
        }
    }

	return 0;
}

//
//
//

static int AddImage (lua_State * L)
{
	auto * image = NewMaybeSized<noise::utils::Image>(L);

	CoronaExternalTextureCallbacks callbacks = {};

	callbacks.size = sizeof(CoronaExternalTextureCallbacks);
	callbacks.getFormat = Image_Format;
	callbacks.getHeight = Image_GetH;
	callbacks.getWidth = Image_GetW;
	callbacks.onFinalize = Image_Dispose;
	callbacks.onGetField = Image_GetField;
	callbacks.onRequestBitmap = Image_GetData;

	if (!CoronaExternalPushTexture(L, &callbacks, image))	// [[width, height, ]image]
	{
		delete image;

		return 0;
	}

	return 1;
}

//
//
//

UTILS_GETTER(NoiseMap)

static int AddNoiseMap (lua_State * L)
{
    auto * map = LuaNewMaybeSized<noise::utils::NoiseMap>(L);   // [w, h, ]map

    LuaXS::AttachMethods(L, MT_NAME(NoiseMap), [](lua_State * L) {
        luaL_Reg funcs[] = {
            {
                DO_1_ARG(NoiseMap, Clear, Float)
            }, {
                GET_VALUE(NoiseMap, BorderValue, number)
            }, {
                "GetBytes", GET_BYTES(NoiseMap)
            }, {
                GET_VALUE(NoiseMap, Height, integer)
            }, {
                GET_VALUE(NoiseMap, Stride, integer)
            }, {
                "GetValue", [](lua_State * L)
                {
                    lua_pushnumber(L, NoiseMap(L)->GetValue(LuaXS::Int(L, 2), LuaXS::Int(L, 3)));

                    return 1;
                }
            }, {
                GET_VALUE(NoiseMap, Width, integer)
            }, {
                SET_VALUE(NoiseMap, BorderValue, float)
            }, {
                SET_SIZE(NoiseMap, Size)
            }, {
                "SetValue", [](lua_State * L)
                {
                    NoiseMap(L)->SetValue(LuaXS::Int(L, 2), LuaXS::Int(L, 3), LuaXS::Float(L, 4));

                    return 0;
                }
            },
            { nullptr, nullptr }
        };

        luaL_register(L, nullptr, funcs);
    });

    return 1;
}

//
//
//

#define SET_RESOURCE(getter, other_getter, name, index) "Set" #name, [](lua_State * L) \
{                                                   \
    lua_settop(L, 2); /* target, resource */        \
    auto * target = getter(L);                      \
    lua_insert(L, 1); /* resource, target */        \
	auto * res = other_getter(L);                   \
    target->Set##name(*res);                        \
    lua_insert(L, 1); /* target, resource */        \
	AuxSetInEnvironment(L, index);	/* target */    \
                                                    \
    return 0;                                       \
}

//
//
//

template<typename T, T * (*getter)(lua_State *)>
void NoiseMapBuilder (lua_State * L)
{
    enum { kSourceModuleIndex, kDestNoiseMapIndex };

	luaL_Reg funcs[] = {
		{
            DO(getter, Build)
        }, {
			GET_VALUE(getter, DestHeight, number)
		}, {
			GET_VALUE(getter, DestWidth, number)
		}, {
            "SetBounds", [](lua_State * L)
            {
                getter(L)->SetBounds (LuaXS::Double(L, 2), LuaXS::Double(L, 3), LuaXS::Double(L, 4), LuaXS::Double(L, 5));

                return 0;
            }
        }, {
            SET_RESOURCE(getter, NoiseMap, DestNoiseMap, kDestNoiseMapIndex)
        }, {
			SET_SIZE(getter, DestSize)
		}, {
			SET_MODULE(getter, Source)
		},
		{ nullptr, nullptr }
	};

	luaL_register(L, nullptr, funcs);
}

#define NOISE_MAP_BUILDER(name) NoiseMapBuilder<noise::utils::NoiseMapBuilder##name, &NoiseMapBuilder##name>(L)

template<typename T>
void NewWithEnv (lua_State * L)
{
    LuaXS::NewTyped<T>(L);  // object

    lua_newtable(L);// object, env
    lua_setfenv(L, -2); // object; object.environment = env
}

//
//
//

UTILS_GETTER(NoiseMapBuilderCylinder)

static int AddNoiseMapBuilderCylinder (lua_State * L)
{
    NewWithEnv<noise::utils::NoiseMapBuilderCylinder>(L);   // builder

    LuaXS::AttachMethods(L, MT_NAME(NoiseMapBuilderCylinder), [](lua_State * L) {
        NOISE_MAP_BUILDER(Cylinder);

        luaL_Reg funcs[] = {
            {
                GET_VALUE(NoiseMapBuilderCylinder, LowerAngleBound, number)
            }, {
                GET_VALUE(NoiseMapBuilderCylinder, LowerHeightBound, number)
            }, {
                GET_VALUE(NoiseMapBuilderCylinder, UpperAngleBound, number)
            }, {
                GET_VALUE(NoiseMapBuilderCylinder, UpperHeightBound, number)
            },
            { nullptr, nullptr }
        };

        luaL_register(L, nullptr, funcs);
    });

    return 1;
}

//
//
//

#define ENABLE_DEF_TRUE(getter, name) "Enable" #name, [](lua_State * L) \
    {                                                                           \
        bool is_false = lua_type(L, 1) == LUA_TBOOLEAN && !lua_toboolean(L, 1); \
        getter(L)->Enable##name(!is_false);                                     \
        return 0;                                                               \
    }

//
//
//

UTILS_GETTER(NoiseMapBuilderPlane)

static int AddNoiseMapBuilderPlane (lua_State * L)
{
    NewWithEnv<noise::utils::NoiseMapBuilderPlane>(L);   // builder

    LuaXS::AttachMethods(L, MT_NAME(NoiseMapBuilderPlane), [](lua_State * L) {
        NOISE_MAP_BUILDER(Plane);

        luaL_Reg funcs[] = {
            {
                ENABLE_DEF_TRUE(NoiseMapBuilderPlane, Seamless)
            }, {
                GET_VALUE(NoiseMapBuilderPlane, LowerXBound, number)
            }, {
                GET_VALUE(NoiseMapBuilderPlane, LowerZBound, number)
            }, {
                GET_VALUE(NoiseMapBuilderPlane, UpperXBound, number)
            }, {
                GET_VALUE(NoiseMapBuilderPlane, UpperZBound, number)
            }, {
                PREDICATE(NoiseMapBuilderPlane, SeamlessEnabled)
            },
            { nullptr, nullptr }
        };

        luaL_register(L, nullptr, funcs);
    });

    return 1;
}

//
//
//

UTILS_GETTER(NoiseMapBuilderSphere)

static int AddNoiseMapBuilderSphere (lua_State * L)
{
    NewWithEnv<noise::utils::NoiseMapBuilderSphere>(L);   // builder

    LuaXS::AttachMethods(L, MT_NAME(NoiseMapBuilderSphere), [](lua_State * L) {
        NOISE_MAP_BUILDER(Sphere);

        luaL_Reg funcs[] = {
            {
                GET_VALUE(NoiseMapBuilderSphere, EastLonBound, number)
            }, {
                GET_VALUE(NoiseMapBuilderSphere, NorthLatBound, number)
            }, {
                GET_VALUE(NoiseMapBuilderSphere, SouthLatBound, number)
            }, {
                GET_VALUE(NoiseMapBuilderSphere, WestLonBound, number)
            },
            { nullptr, nullptr }
        };

        luaL_register(L, nullptr, funcs);
    });

    return 1;
}

//
//
//

template<typename T, T * (*getter)(lua_State *)>
void Renderer (lua_State * L)
{
    enum { kDestImageIndex, kSourceNoiseMapIndex };

    luaL_Reg funcs[] = {
        {
            ENABLE_DEF_TRUE(getter, Wrap)
        }, {
            PREDICATE(getter, WrapEnabled)
        }, {
            DO(getter, Render)
        }, {
            SET_RESOURCE(getter, Image, DestImage, kDestImageIndex)
        }, {
            SET_RESOURCE(getter, NoiseMap, SourceNoiseMap, kSourceNoiseMapIndex)
        },
        { nullptr, nullptr }
    };

    luaL_register(L, nullptr, funcs);
}

//
//
//

UTILS_GETTER(RendererImage)

static int AddRendererImage (lua_State * L)
{
    enum { kBackgroundImageIndex = 2 }; // n.b. follows those in Renderer

    NewWithEnv<noise::utils::RendererImage>(L);   // renderer

    LuaXS::AttachMethods(L, MT_NAME(RendererImage), [](lua_State * L) {
        Renderer<noise::utils::RendererImage, &RendererImage>(L);

        luaL_Reg funcs[] = {
            {
                "AddGradientPoint", [](lua_State * L)
                {
                    return AddGradientPoint(L, RendererImage);
                }
            }, {
                DO(RendererImage, BuildGrayscaleGradient)
            }, {
                DO(RendererImage, BuildTerrainGradient)
            }, {
                DO(RendererImage, ClearGradient)
            }, {
                ENABLE_DEF_TRUE(RendererImage, Light)
            }, {
                GET_VALUE(RendererImage, LightAzimuth, number)
            }, {
                GET_VALUE(RendererImage, LightBrightness, number)
            }, {
               GET_COLOR(RendererImage, LightColor)
            }, {
                GET_VALUE(RendererImage, LightContrast, number)
            }, {
                GET_VALUE(RendererImage, LightElev, number)
            }, {
                GET_VALUE(RendererImage, LightIntensity, number)
            }, {
                PREDICATE(RendererImage, LightEnabled)
            }, {
                SET_RESOURCE(RendererImage, Image, BackgroundImage, kBackgroundImageIndex)
            }, {
                SET_VALUE(RendererImage, LightAzimuth, number)
            }, {
                SET_VALUE(RendererImage, LightBrightness, number)
            }, {
                SET_COLOR(RendererImage, LightColor)
            }, {
                SET_VALUE(RendererImage, LightContrast, number)
            }, {
                SET_VALUE(RendererImage, LightElev, number)
            }, {
                SET_VALUE(RendererImage, LightIntensity, number)
            },
            { nullptr, nullptr }
        };

        luaL_register(L, nullptr, funcs);
    });

    return 1;
}

//
//
//

UTILS_GETTER(RendererNormalMap)

static int AddRendererNormalMap (lua_State * L)
{
    NewWithEnv<noise::utils::RendererNormalMap>(L); // renderer

    LuaXS::AttachMethods(L, MT_NAME(RendererNormalMap), [](lua_State * L) {
        Renderer<noise::utils::RendererNormalMap, &RendererNormalMap>(L);

        luaL_Reg funcs[] = {
            {
                GET_VALUE(RendererNormalMap, BumpHeight, number)
            }, {
                SET_VALUE(RendererNormalMap, BumpHeight, number)
            },
            { nullptr, nullptr }
        };

        luaL_register(L, nullptr, funcs);
    });

    return 1;
}

//
//
//

#define ADD_TYPE(name) #name, Add##name

void AddUtils (lua_State * L)
{
	luaL_Reg funcs[] = {
        {
            ADD_TYPE(Color)
        }, {
            ADD_TYPE(GradientColor)
        }, {
            ADD_TYPE(Image)
        }, {
            ADD_TYPE(NoiseMap)
        }, {
            ADD_TYPE(NoiseMapBuilderCylinder)
        }, {
            ADD_TYPE(NoiseMapBuilderPlane)
        }, {
            ADD_TYPE(NoiseMapBuilderSphere)
        }, {
            ADD_TYPE(RendererImage)
        }, {
            ADD_TYPE(RendererNormalMap)
        },
        { nullptr, nullptr }
	};

    luaL_register(L, nullptr, funcs);
}
