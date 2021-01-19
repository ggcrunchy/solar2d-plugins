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

//
//
//

static noise::module::Module * Module (lua_State * L, int arg = 1)
{
	luaL_checktype(L, arg, LUA_TUSERDATA);
	lua_getmetatable(L, arg);	// ..., ud, ..., mt
	lua_gettable(L, lua_upvalueindex(1));	// ..., ud, ..., exists?
	luaL_argcheck(L, arg, lua_toboolean(L, -1), "Not a module");
	lua_pop(L, 1);	// ..., ud, ...

	return LuaXS::UD<noise::module::Module>(L, arg);
}

static int AuxGetSourceModule (lua_State * L, int index = -1)
{
	lua_settop(L, 2 - (index >= 0));// mod[, key]

	if (index >= 0) lua_pushinteger(L, index);	// mod, key

	lua_getfenv(L, 1);	// mod, key, env
	lua_insert(L, 2);	// mod, env, key
	lua_gettable(L, 2);	// mod, env, module?

	return 1;
}

static void AuxSetSourceModule (lua_State * L, int index = -1, int other_pos = 3)
{
	lua_settop(L, other_pos - (index >= 0));// mod[, ...[, key]], other

	int key_pos = other_pos - 1;

	if (index >= 0)
	{
		lua_pushinteger(L, index);	// mod[, ...], other, key
		lua_insert(L, key_pos);	// mod[, ...], key, other
	}

	lua_getfenv(L, 1);	// mod[, ...], key, other, env
	lua_insert(L, key_pos);	// mod[, ...], env, key, other
	lua_settable(L, key_pos);	// mod[, ...], env = { ..., [key] = other }
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

typedef void (*AddMethods)(lua_State *);

static void NoOp (lua_State * L) {}

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

#define DO(mod, name) #name, [](lua_State * L)	\
		{						\
			mod(L)->##name();	\
								\
			return 0;			\
		}

#define DO_1_ARG(mod, name, what) #name, [](lua_State * L)	\
		{											\
			mod(L)->##name(LuaXS::##what(L, 2));	\
													\
			return 0;								\
		}

#define DO_2_DOUBLES(mod, name) #name, [](lua_State * L)	\
		{																\
			mod(L)->##name(LuaXS::Double(L, 2), LuaXS::Double(L, 3));	\
																		\
			return 0;													\
		}

#define AUX_GET_MODULE(mod, index)	[](lua_State * L)	\
		{																\
			auto * m = mod(L);											\
			return AuxGetSourceModule(L, index);	/* m, module? */	\
		}

#define GET_MODULE(mod, name) "Get" #name "Module", AUX_GET_MODULE(mod, k##name##ModuleIndex)

#define GET_VALUE(mod, name, what) "Get" #name, [](lua_State * L)	\
		{											\
			lua_push##what(L, mod(L)->Get##name());	\
													\
			return 1;								\
		}

#define PREDICATE(mod, name) "Is" #name, [](lua_State * L)	\
		{											\
			lua_pushboolean(L, mod(L)->Is##name());	\
													\
			return 1;								\
		}

#define AUX_SET_MODULE(mod, setter, index) [](lua_State * L)	\
		{													\
			auto * m = mod(L);								\
			auto * other = Module(L, 2);					\
			m->##setter(*other);							\
			AuxSetSourceModule(L, index); /* m, index */	\
															\
			return 0;										\
		}

#define SET_MODULE(mod, name) "Set" #name "Module", AUX_SET_MODULE(mod, Set##name##Module, k##name##ModuleIndex)

#define SET_VALUE(mod, name, what) "Set" #name, [](lua_State * L)	\
		{											\
			mod(L)->Set##name(lua_to##what(L, 2));	\
													\
			return 0;								\
		}

#define SET_3_DOUBLES(mod, name) "Set" #name, [](lua_State * L)	\
		{																						\
			mod(L)->Set##name(LuaXS::Double(L, 2), LuaXS::Double(L, 3), LuaXS::Double(L, 4));	\
																								\
			return 0;																			\
		}

#define SET_1_OR_3(mod, name) "Set" #name, [](lua_State * L)	\
		{																						\
			auto * m = mod(L);																	\
			if (lua_isnoneornil(L, 3)) m->Set##name(LuaXS::Double(L, 2));						\
			else m->Set##name(LuaXS::Double(L, 2), LuaXS::Double(L, 3), LuaXS::Double(L, 4));	\
																								\
			return 0;																			\
		}

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

#define GETTER(name) static noise::module::##name * name (lua_State * L)	\
{																			\
	return LuaXS::CheckUD<noise::module::##name>(L, 1, "libnoise." #name);	\
}

GETTER(Billow)

static void AddBillow (lua_State * L)
{
	ADD_NOISE_FUNCS(Billow, 3);
}

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

				AuxSetSourceModule(L, kZDisplaceModuleIndex, 4);// displace, x, y, index

				lua_pop(L, 1);	// displace, x, y

				AuxSetSourceModule(L, kYDisplaceModuleIndex, 3);// displace, x, index

				lua_pop(L, 1);	// displace, x

				AuxSetSourceModule(L, kXDisplaceModuleIndex, 2);// displace, index

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
/*
      const Module& GetXDisplaceModule () const
      {
        if (m_pSourceModule == NULL || m_pSourceModule[1] == NULL) {
          throw noise::ExceptionNoModule ();
        }
        return *(m_pSourceModule[1]);
      }

      const Module& GetYDisplaceModule () const
      {
        if (m_pSourceModule == NULL || m_pSourceModule[2] == NULL) {
          throw noise::ExceptionNoModule ();
        }
        return *(m_pSourceModule[2]);
      }

      const Module& GetZDisplaceModule () const
      {
        if (m_pSourceModule == NULL || m_pSourceModule[3] == NULL) {
          throw noise::ExceptionNoModule ();
        }
        return *(m_pSourceModule[3]);
      }

      void SetDisplaceModules (const Module& xDisplaceModule,
        const Module& yDisplaceModule, const Module& zDisplaceModule)
      {
        SetXDisplaceModule (xDisplaceModule);
        SetYDisplaceModule (yDisplaceModule);
        SetZDisplaceModule (zDisplaceModule);
      }

      void SetXDisplaceModule (const Module& xDisplaceModule)
      {
        assert (m_pSourceModule != NULL);
        m_pSourceModule[1] = &xDisplaceModule;
      }

      void SetYDisplaceModule (const Module& yDisplaceModule)
      {
        assert (m_pSourceModule != NULL);
        m_pSourceModule[2] = &yDisplaceModule;
      }

      void SetZDisplaceModule (const Module& zDisplaceModule)
      {
        assert (m_pSourceModule != NULL);
        m_pSourceModule[3] = &zDisplaceModule;
      }
*/
}

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

GETTER(Perlin)

static void AddPerlin (lua_State * L)
{
	ADD_NOISE_FUNCS(Perlin, 3)
}

GETTER(RidgedMulti)

static void AddRidgedMulti (lua_State * L)
{
	ADD_NOISE_FUNCS(RidgedMulti, 2)
}

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

#define PROPERTY(name) #name, noise::module::##name

static void AddProperties (lua_State * L)
{
	lua_newtable(L);// properties

	typedef struct {
		const char * name;
		double value;
	} Double;

	Double doubles[] = {
		PROPERTY(DEFAULT_BILLOW_FREQUENCY),
		PROPERTY(DEFAULT_BILLOW_LACUNARITY),
		PROPERTY(DEFAULT_BILLOW_PERSISTENCE),
		PROPERTY(DEFAULT_CONST_VALUE),
		PROPERTY(DEFAULT_CYLINDERS_FREQUENCY),
		PROPERTY(DEFAULT_EXPONENT),
		PROPERTY(DEFAULT_PERLIN_FREQUENCY),
		PROPERTY(DEFAULT_PERLIN_LACUNARITY),
		PROPERTY(DEFAULT_PERLIN_PERSISTENCE),
		PROPERTY(DEFAULT_RIDGED_FREQUENCY),
		PROPERTY(DEFAULT_RIDGED_LACUNARITY),
		PROPERTY(DEFAULT_ROTATE_X),
		PROPERTY(DEFAULT_ROTATE_Y),
		PROPERTY(DEFAULT_ROTATE_Z),
		PROPERTY(DEFAULT_BIAS),
		PROPERTY(DEFAULT_SCALE),
		PROPERTY(DEFAULT_SCALE_POINT_X),
		PROPERTY(DEFAULT_SCALE_POINT_Y),
		PROPERTY(DEFAULT_SCALE_POINT_Z),
		PROPERTY(DEFAULT_SELECT_EDGE_FALLOFF),
		PROPERTY(DEFAULT_SELECT_LOWER_BOUND),
		PROPERTY(DEFAULT_SELECT_UPPER_BOUND),
		PROPERTY(DEFAULT_SPHERES_FREQUENCY),
		PROPERTY(DEFAULT_TRANSLATE_POINT_X),
		PROPERTY(DEFAULT_TRANSLATE_POINT_Y),
		PROPERTY(DEFAULT_TRANSLATE_POINT_Z),
		PROPERTY(DEFAULT_TURBULENCE_FREQUENCY),
		PROPERTY(DEFAULT_TURBULENCE_POWER),
		PROPERTY(DEFAULT_VORONOI_DISPLACEMENT),
		PROPERTY(DEFAULT_VORONOI_FREQUENCY),
		{ nullptr, 0.0 }
	};

	for (int i = 0; doubles[i].name; ++i)
	{
		lua_pushnumber(L, doubles[i].value);// properties, value
		lua_setfield(L, -2, doubles[i].name);	// properties = { ..., name = value }
	}

	typedef struct {
		const char * name;
		int value;
	} Integer;

	Integer integers[] = {
		PROPERTY(DEFAULT_BILLOW_OCTAVE_COUNT),
		PROPERTY(DEFAULT_BILLOW_SEED),
		PROPERTY(BILLOW_MAX_OCTAVE),
		PROPERTY(DEFAULT_PERLIN_OCTAVE_COUNT),
		PROPERTY(DEFAULT_PERLIN_SEED),
		PROPERTY(PERLIN_MAX_OCTAVE),
		PROPERTY(DEFAULT_RIDGED_OCTAVE_COUNT),
		PROPERTY(DEFAULT_RIDGED_SEED),
		PROPERTY(RIDGED_MAX_OCTAVE),
		PROPERTY(DEFAULT_TURBULENCE_ROUGHNESS),
		PROPERTY(DEFAULT_TURBULENCE_SEED),
		PROPERTY(DEFAULT_VORONOI_SEED),
		{ nullptr, 0 }
	};

	for (int i = 0; integers[i].name; ++i)
	{
		lua_pushinteger(L, integers[i].value);	// properties, value
		lua_setfield(L, -2, integers[i].name);	// properties = { ..., name = value }
	}

	const char * names[] = {
		"DEFAULT_BILLOW_QUALITY", "DEFAULT_PERLIN_QUALITY", "DEFAULT_RIDGED_QUALITY", nullptr
	};

	for (int i = 0; names[i]; ++i)
	{
		lua_pushliteral(L, "STD");	// properties, "STD"
		lua_setfield(L, -2, names[i]);	// properties = { ..., name = "STD" }
	}
}

//
//
//

#define FACTORY(name, adder) Factory<noise::module::##name, adder>
#define MODULE(name) #name, FACTORY(name, NoOp)
#define MODULE_WITH_ADDER(name) #name, FACTORY(name, Add##name)

//
//
//

#define MODEL_GETTER(name) static noise::model::##name * name (lua_State * L)	\
{																			\
	return LuaXS::CheckUD<noise::model::##name>(L, 1, "libnoise." #name);	\
}

#define MODEL_GET_MODULE(model) "GetModule", AUX_GET_MODULE(model, 0)
#define MODEL_SET_MODULE(model) "SetModule", AUX_SET_MODULE(model, SetModule, 0)

#define DO_2_DOUBLES_RETURN_1(model, name)	#name, [](lua_State * L)	\
	{																					\
		lua_pushnumber(L, model(L)->##name(LuaXS::Double(L, 2), LuaXS::Double(L, 3)));	\
																						\
		return 1;																		\
	}

template<typename T>
void NewModel (lua_State * L)
{
	auto * mod = !lua_isnoneornil(L, 1) ? Module(L, 1) : nullptr;

	lua_settop(L, mod ? 1 : 0);	// [module]
	lua_createtable(L, 1, 0);	// [module, ]env

	if (mod)
	{
		lua_insert(L, 1);	// env, module
		lua_rawseti(L, 1, 1);	// env = { module }

		LuaXS::NewTyped<T>(L, *mod);	// env, model
	}

	else LuaXS::NewTyped<T>(L);	// env, model

	lua_insert(L, 1);	// model, env
	lua_setfenv(L, 1);	// model; model.environment = env
}

MODEL_GETTER(Cylinder)

static int AddCylinderModel (lua_State * L)
{
	NewModel<noise::model::Cylinder>(L);	// model

	if (luaL_newmetatable(L, "libnoise.cylinder_model"))	// model, mt
	{
		luaL_Reg funcs[] = {
			{
				MODEL_GET_MODULE(Cylinder)
			}, {
				DO_2_DOUBLES_RETURN_1(Cylinder, GetValue)
			}, {
				MODEL_SET_MODULE(Cylinder)
			},
			{ nullptr, nullptr }
		};

		luaL_register(L, nullptr, funcs);
	}

	return 1;
}

MODEL_GETTER(Line)

static int AddLineModel (lua_State * L)
{
	NewModel<noise::model::Line>(L);// model

	if (luaL_newmetatable(L, "libnoise.line_model"))	// model, mt
	{
		luaL_Reg funcs[] = {
			{
				GET_VALUE(Line, Attenuate, boolean)
			}, {
				MODEL_GET_MODULE(Line)
			}, {
				"GetValue", [](lua_State * L)
				{
					lua_pushnumber(L, Line(L)->GetValue(LuaXS::Double(L, 2)));

					return 1;
				}
			}, {
				SET_VALUE(Line, Attenuate, boolean)
			}, {
				SET_3_DOUBLES(Line, EndPoint)
			}, {
				MODEL_SET_MODULE(Line)
			}, {
				SET_3_DOUBLES(Line, StartPoint)
			},
			{ nullptr, nullptr }
		};

		luaL_register(L, nullptr, funcs);
	}

	return 1;
}

MODEL_GETTER(Plane)

static int AddPlaneModel (lua_State * L)
{
	NewModel<noise::model::Plane>(L);	// model

	if (luaL_newmetatable(L, "libnoise.plane_model"))	// model, mt
	{
		luaL_Reg funcs[] = {			{
				MODEL_GET_MODULE(Plane)
			}, {
				DO_2_DOUBLES_RETURN_1(Plane, GetValue)
			}, {
				MODEL_SET_MODULE(Plane)
			},
			{ nullptr, nullptr }
		};

		luaL_register(L, nullptr, funcs);
	}

	return 1;
}

MODEL_GETTER(Sphere)

static int AddSphereModel (lua_State * L)
{
	NewModel<noise::model::Sphere>(L);	// model

	if (luaL_newmetatable(L, "libnoise.sphere_model"))	// model, mt
	{
		luaL_Reg funcs[] = {			{
				MODEL_GET_MODULE(Sphere)
			}, {
				DO_2_DOUBLES_RETURN_1(Sphere, GetValue)
			}, {
				MODEL_SET_MODULE(Sphere)
			},
			{ nullptr, nullptr }
		};

		luaL_register(L, nullptr, funcs);
	}

	return 1;
}

//
//
//

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

	//
	//
	//

	luaL_Reg module_factories[] = {
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

	for (int i = 0; module_factories[i].func; ++i)
	{
		lua_pushvalue(L, -3);	// module_mt, module_list, libnoise, module_mt
		lua_pushvalue(L, -3);	// module_mt, module_list, libnoise, module_mt, module_list
		lua_pushliteral(L, "libnoise.");// module_mt, module_list, libnoise, module_mt, module_list, "libnoise."
		lua_pushstring(L, module_factories[i].name);// module_mt, module_list, libnoise, module_mt, module_list, "libnoise.", name
		lua_concat(L, 2);	// module_mt, module_list, libnoise, module_mt, module_list, "libnoise." .. name
		lua_pushcclosure(L, module_factories[i].func, 3);	// module_mt, module_list, libnoise, factory
		lua_setfield(L, -2, module_funcs[i].name);	// module_mt, module_list, libnoise = { ..., name = factory }
	}

	//
	//
	//
		
	AddProperties(L);	// module_mt, module_list, libnoise, properties

	lua_pushcclosure(L, [](lua_State * L) {
		lua_settop(L, 1);	// name
		lua_gettable(L, lua_upvalueindex(1));	// value

		return 1;
	}, 1);	// module_mt, module_list, libnoise, GetProperty
	lua_setfield(L, -2, "GetProperty");	// module_mt, module_list, libnoise = { ..., GetProperty = GetProperty }

	//
	//
	//

	luaL_Reg model_funcs[] = {
		{
			"CylinderModel", AddCylinderModel
		}, {
			"LineModel", AddLineModel
		}, {
			"PlaneModel", AddPlaneModel
		}, {
			"SphereModel", AddSphereModel
		},
		{ nullptr, nullptr }
	};

	luaL_register(L, nullptr, model_funcs);

	//
	//
	//

	return 1;
}