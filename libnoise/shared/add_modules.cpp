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

//
//
//

typedef void (*AddMethods)(lua_State *);

template<typename T, AddMethods add>
int Factory (lua_State * L)
{
	T * mod = static_cast<T *>(lua_newuserdata(L, sizeof(T)));	// module

	if (mod->GetSourceModuleCount() > 0)
	{
		lua_newtable(L);// module, env
		lua_setfenv(L, -2);	// module; module.environment = env
	}

	if (luaL_newmetatable(L, lua_tostring(L, lua_upvalueindex(3))))	// module, mt
	{
		add(L);
	}

	lua_setmetatable(L, -2);// module; module.metatable = mt

	return 1;
}

//
//
//

static int QualityName (lua_State * L, noise::NoiseQuality quality)
{
	switch (quality)
	{
	case noise::QUALITY_FAST:
		lua_pushliteral(L, "FAST");
		break;
	case noise::QUALITY_STD:
		lua_pushliteral(L, "STD");
		break;
	case noise::QUALITY_BEST:
		lua_pushliteral(L, "BEST");
		break;
	default:
		luaL_error(L, "Invalid noise quality");
	}

	return 1;
}

static noise::NoiseQuality Quality (lua_State * L, int arg)
{
	const char * names[] = { "FAST", "STD", "BEST", nullptr };
	noise::NoiseQuality qualities[] = { noise::QUALITY_FAST, noise::QUALITY_STD, noise::QUALITY_BEST };

	return qualities[luaL_checkoption(L, arg, "STD", names)];
}

//
//
//

template<typename T, T * (*getter)(lua_State *)>
void Noise1 (lua_State * L)
{
	luaL_Reg funcs[] = {
		{
			GET_VALUE(getter, Frequency, number)
		}, {
			GET_VALUE(getter, Seed, integer)
		}, {
			SET_VALUE(getter, Frequency, number)
		}, {
			SET_VALUE(getter, Seed, integer)
		},
		{ nullptr, nullptr }
	};

	luaL_register(L, nullptr, funcs);
}

template<typename T, T * (*getter)(lua_State *)>
void Noise2 (lua_State * L)
{
	Noise1<T, getter>(L);

	luaL_Reg funcs[] = {
		{
			GET_VALUE(getter, Lacunarity, number)
		}, {
			"GetNoiseQuality", [](lua_State * L)
			{
				return QualityName(L, getter(L)->GetNoiseQuality());
			}
		}, {
			GET_VALUE(getter, OctaveCount, integer)
		}, {
			SET_VALUE(getter, Lacunarity, number)
		}, {
			"SetNoiseQuality", [](lua_State * L)
			{
				getter(L)->SetNoiseQuality(Quality(L, 2));

				return 0;
			}
		}, {
			SET_VALUE(getter, OctaveCount, integer)
		},
		{ nullptr, nullptr }
	};

	luaL_register(L, nullptr, funcs);
}

template<typename T, T * (*getter)(lua_State *)>
void Noise3 (lua_State * L)
{
	Noise2<T, getter>(L);

	luaL_Reg funcs[] = {
		{
			GET_VALUE(getter, Persistence, number)
		}, {
			SET_VALUE(getter, Persistence, number)
		},
		{ nullptr, nullptr }
	};

	luaL_register(L, nullptr, funcs);
}

#define ADD_NOISE_FUNCS(name, level) Noise##level<noise::module::##name, name>(L);

//
//
//

#define GETTER(name) static noise::module::##name * name (lua_State * L)	\
{																		\
	return LuaXS::CheckUD<noise::module::##name>(L, 1, MT_NAME(name));	\
}

//
//
//

GETTER(Billow)

static void AddBillow (lua_State * L)
{
	ADD_NOISE_FUNCS(Billow, 3);
}

//
//
//

GETTER(Blend)

static void AddBlend (lua_State * L)
{
	enum { kControlModuleIndex = 2 };	// cf. blend.h

	luaL_Reg funcs[] = {
		{
			GET_MODULE(Blend, Control)
		}, {
			SET_MODULE(Blend, Control)
		}
	};

	luaL_register(L, nullptr, funcs);
}

//
//
//

GETTER(Clamp)

static void AddClamp (lua_State * L)
{
	luaL_Reg funcs[] = {
		{
			GET_VALUE(Clamp, LowerBound, number)
		}, {
			GET_VALUE(Clamp, UpperBound, number)
		}, {
			DO_2_DOUBLES(Clamp, SetBounds)
		}
	};

	luaL_register(L, nullptr, funcs);
}

//
//
//

GETTER(Const)

static void AddConst (lua_State * L)
{
	luaL_Reg funcs[] = {
		{
			GET_VALUE(Const, ConstValue, number)
		}, {
			SET_VALUE(Const, ConstValue, number)
		}
	};

	luaL_register(L, nullptr, funcs);
}

//
//
//

GETTER(Curve)

static void AddCurve (lua_State * L)
{
	luaL_Reg funcs[] = {
		{
			DO_2_DOUBLES(Curve, AddControlPoint)
		}, {
			DO(Curve, ClearAllControlPoints)
		}, {
			"GetControlPointArray", [](lua_State * L)
			{
				noise::module::Curve * curve = Curve(L);
				auto cp = curve->GetControlPointArray();
				int n = curve->GetControlPointCount();

				lua_createtable(L, n, 0);	// curve, array

				for (int i = 0; i < n; ++i)
				{
					lua_createtable(L, 0, 2);	// curve, array, point
					lua_pushnumber(L, cp[i].inputValue);// curve, array, point, inputValue
					lua_setfield(L, -2, "inputValue");	// curve, array, point = { inputValue = inputValue }
					lua_pushnumber(L, cp[i].outputValue);	// curve, array, point, outputValue
					lua_setfield(L, -2, "outputValue");	// curve, array, point = { inputValue, outputValue = outputValue }
					lua_rawseti(L, -2, i + 1);	// curve, array = { ..., point }
				}

				return 1;
			}
		}, {
			GET_VALUE(Curve, ControlPointCount, integer)
		},
		{ nullptr, nullptr }
	};

	luaL_register(L, nullptr, funcs);
}

//
//
//

GETTER(Cylinders)

static void AddCylinders (lua_State * L)
{
	luaL_Reg funcs[] = {
		{
			GET_VALUE(Cylinders, Frequency, number)
		}, {
			SET_VALUE(Cylinders, Frequency, number)
		}
	};

	luaL_register(L, nullptr, funcs);
}

//
//
//

GETTER(Displace)

static void AddDisplace (lua_State * L)
{
	enum { kXDisplaceModuleIndex = 1, kYDisplaceModuleIndex, kZDisplaceModuleIndex };	// cf. displace.h

	luaL_Reg funcs[] = {
		{
			GET_MODULE(Displace, XDisplace)
		}, {
			GET_MODULE(Displace, YDisplace)
		}, {
			GET_MODULE(Displace, ZDisplace)
		}, {
			"SetDisplaceModules", [](lua_State * L)
			{
				auto * displace = Displace(L);
				auto * x = Module(L, 2), * y = Module(L, 3), * z = Module(L, 4);

				displace->SetDisplaceModules(*x, *y, *z);

				AuxSetInEnvironment(L, kZDisplaceModuleIndex, 4);	// displace, x, y, index

				lua_pop(L, 1);	// displace, x, y

				AuxSetInEnvironment(L, kYDisplaceModuleIndex, 3);	// displace, x, index

				lua_pop(L, 1);	// displace, x

				AuxSetInEnvironment(L, kXDisplaceModuleIndex, 2);	// displace, index

				return 0;
			}
		}, {
			SET_MODULE(Displace, XDisplace)
		}, {
			SET_MODULE(Displace, YDisplace)
		}, {
			SET_MODULE(Displace, ZDisplace)
		},
		{ nullptr, nullptr }
	};

	luaL_register(L, nullptr, funcs);
}

//
//
//

GETTER(Exponent)

static void AddExponent (lua_State * L)
{
	luaL_Reg funcs[] = {
		{
			GET_VALUE(Exponent, Exponent, number)
		}, {
			SET_VALUE(Exponent, Exponent, number)
		}
	};

	luaL_register(L, nullptr, funcs);
}

//
//
//

GETTER(Perlin)

static void AddPerlin (lua_State * L)
{
	ADD_NOISE_FUNCS(Perlin, 3)
}

//
//
//

GETTER(RidgedMulti)

static void AddRidgedMulti (lua_State * L)
{
	ADD_NOISE_FUNCS(RidgedMulti, 2)
}

//
//
//

GETTER(RotatePoint)

static void AddRotatePoint (lua_State * L)
{
	luaL_Reg funcs[] = {
		{
			GET_VALUE(RotatePoint, XAngle, number)
		}, {
			GET_VALUE(RotatePoint, YAngle, number)
		}, {
			GET_VALUE(RotatePoint, ZAngle, number)
		}, {
			SET_3_DOUBLES(RotatePoint, Angles)
		}, {
			SET_VALUE(RotatePoint, XAngle, number)
		}, {
			SET_VALUE(RotatePoint, YAngle, number)
		}, {
			SET_VALUE(RotatePoint, ZAngle, number)
		}
	};

	luaL_register(L, nullptr, funcs);
}

//
//
//

GETTER(ScaleBias)

static void AddScaleBias (lua_State * L)
{
	luaL_Reg funcs[] = {
		{
			GET_VALUE(ScaleBias, Bias, number)
		}, {
			GET_VALUE(ScaleBias, Scale, number)
		}, {
			SET_VALUE(ScaleBias, Bias, number)
		}, {
			SET_VALUE(ScaleBias, Scale, number)
		}
	};

	luaL_register(L, nullptr, funcs);
}

//
//
//

GETTER(ScalePoint)

static void AddScalePoint (lua_State * L)
{
	luaL_Reg funcs[] = {
		{
			GET_VALUE(ScalePoint, XScale, number)
		}, {
			GET_VALUE(ScalePoint, YScale, number)
		}, {
			GET_VALUE(ScalePoint, ZScale, number)
		}, {
			SET_1_OR_3(ScalePoint, Scale)
		}, {
			SET_VALUE(ScalePoint, XScale, number)
		}, {
			SET_VALUE(ScalePoint, YScale, number)
		}, {
			SET_VALUE(ScalePoint, ZScale, number)
		}
	};

	luaL_register(L, nullptr, funcs);
}

GETTER(Select)

static void AddSelect (lua_State * L)
{
	enum { kControlModuleIndex = 2 };	// cf. select.h

	luaL_Reg funcs[] = {
		{
 			GET_MODULE(Select, Control)
		}, {
			GET_VALUE(Select, EdgeFalloff, number)
		}, {
			GET_VALUE(Select, LowerBound, number)
		}, {
			GET_VALUE(Select, UpperBound, number)
		}, {
			DO_2_DOUBLES(Select, SetBounds)
		}, {
			SET_MODULE(Select, Control)
		}, {
			SET_VALUE(Select, EdgeFalloff, number)
		},
		{ nullptr, nullptr }
	};
}

//
//
//

GETTER(Spheres)

static void AddSpheres (lua_State * L)
{
	luaL_Reg funcs[] = {
		{
			GET_VALUE(Spheres, Frequency, number)
		}, {
			SET_VALUE(Spheres, Frequency, number)
		}
	};

	luaL_register(L, nullptr, funcs);
}

//
//
//

GETTER(Terrace)

static void AddTerrace (lua_State * L)
{
	luaL_Reg funcs[] = {
		{
			DO_1_ARG(Terrace, AddControlPoint, Double)
		}, {
			DO(Terrace, ClearAllControlPoints)
		}, {
			"GetControlPointArray", [](lua_State * L)
			{
				noise::module::Terrace * terrace = Terrace(L);
				auto * arr = terrace->GetControlPointArray();
				int n = terrace->GetControlPointCount();

				lua_createtable(L, n, 0);	// terrace, array

				for (int i = 0; i < n; ++i)
				{
					lua_pushnumber(L, arr[i]);// terrace, array, value
					lua_rawseti(L, -2, i + 1);	// terrace, array = { ..., value }
				}

				return 1;
			}
		}, {
			GET_VALUE(Terrace, ControlPointCount, integer)
		}, {
			DO_1_ARG(Terrace, InvertTerraces, Bool)
		}, {
			PREDICATE(Terrace, TerracesInverted)
		}, {
			DO_1_ARG(Terrace, MakeControlPoints, Int)
		},
		{ nullptr, nullptr }
	};

	luaL_register(L, nullptr, funcs);
}

//
//
//

GETTER(TranslatePoint)

static void AddTranslatePoint (lua_State * L)
{
	luaL_Reg funcs[] = {
		{
			GET_VALUE(TranslatePoint, XTranslation, number)
		}, {
			GET_VALUE(TranslatePoint, YTranslation, number)
		}, {
			GET_VALUE(TranslatePoint, ZTranslation, number)
		}, {
			SET_1_OR_3(TranslatePoint, Translation)
		}, {
			SET_VALUE(TranslatePoint, XTranslation, number)
		}, {
			SET_VALUE(TranslatePoint, YTranslation, number)
		}, {
			SET_VALUE(TranslatePoint, ZTranslation, number)
		}
	};

	luaL_register(L, nullptr, funcs);
}

//
//
//

GETTER(Turbulence)

static void AddTurbulence (lua_State * L)
{
	ADD_NOISE_FUNCS(Turbulence, 1)

	luaL_Reg funcs[] = {
		{
			GET_VALUE(Turbulence, Power, number)
		}, {
			GET_VALUE(Turbulence, RoughnessCount, integer)
		}, {
			SET_VALUE(Turbulence, Frequency, number)
		}, {
			SET_VALUE(Turbulence, Roughness, integer)
		}
	};

	luaL_register(L, nullptr, funcs);
}

//
//
//

GETTER(Voronoi)

static void AddVoronoi (lua_State * L)
{
	luaL_Reg funcs[] = {
		{
			DO_1_ARG(Voronoi, EnableDistance, Bool)
		}, {
			GET_VALUE(Voronoi, Displacement, number)
		}, {
			GET_VALUE(Voronoi, Frequency, number)
		}, {
			GET_VALUE(Voronoi, Seed, integer)
		}, {
			PREDICATE(Voronoi, DistanceEnabled)
		}, {
			SET_VALUE(Voronoi, Displacement, number)
		}, {
			SET_VALUE(Voronoi, Frequency, number)
		}, {
			SET_VALUE(Voronoi, Seed, integer)
		}
	};

	luaL_register(L, nullptr, funcs);
}

//
//
//

static void NoOp (lua_State * L) {}

#define FACTORY(name, adder) Factory<noise::module::##name, adder>
#define MODULE(name) #name, FACTORY(name, NoOp)
#define MODULE_WITH_ADDER(name) #name, FACTORY(name, Add##name)

//
//
//

void AddModules (lua_State * L)
{

	luaL_Reg factories[] = {
		MODULE(Abs),
		MODULE(Add),
		MODULE_WITH_ADDER(Billow),
		MODULE_WITH_ADDER(Blend),
		MODULE(Cache),
		MODULE(Checkerboard),
		MODULE_WITH_ADDER(Clamp),
		MODULE_WITH_ADDER(Const),
		MODULE_WITH_ADDER(Curve),
		MODULE_WITH_ADDER(Cylinders),
		MODULE_WITH_ADDER(Displace),
		MODULE_WITH_ADDER(Exponent),
		MODULE(Invert),
		MODULE(Max),
		MODULE(Min),
		MODULE(Multiply),
		MODULE_WITH_ADDER(Perlin),
		MODULE(Power),
		MODULE_WITH_ADDER(RidgedMulti),
		MODULE_WITH_ADDER(RotatePoint),
		MODULE_WITH_ADDER(ScaleBias),
		MODULE_WITH_ADDER(ScalePoint),
		MODULE_WITH_ADDER(Select),
		MODULE_WITH_ADDER(Spheres),
		MODULE_WITH_ADDER(Terrace),
		MODULE_WITH_ADDER(TranslatePoint),
		MODULE_WITH_ADDER(Turbulence),
		MODULE_WITH_ADDER(Voronoi),
		{ nullptr, nullptr }
	};

	for (int i = 0; factories[i].func; ++i)
	{
		lua_pushvalue(L, -2);	// libnoise, mt, list, mt
		lua_pushvalue(L, -2);	// libnoise, mt, list, mt, list
		lua_pushliteral(L, MT_PREFIX);	// libnoise, mt, list, mt, list, prefix
		lua_pushstring(L, factories[i].name);// libnoise, mt, list, mt, list, prefix, name
		lua_concat(L, 2);	// libnoise, mt, list, mt, list, prefix .. name
		lua_pushcclosure(L, factories[i].func, 3);	// libnoise, mt, list, factory
		lua_setfield(L, -4, factories[i].name);	// libnoise = { ..., name = factory }, mt, list
	}

	lua_pop(L, 2);	// libnoise
}