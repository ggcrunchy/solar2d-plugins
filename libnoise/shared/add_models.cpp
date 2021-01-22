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

#define MODEL_GETTER(name) static noise::model::##name * name (lua_State * L)	\
{																		\
	return LuaXS::CheckUD<noise::model::##name>(L, 1, MT_NAME(name));	\
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

//
//
//

MODEL_GETTER(Cylinder)

static int AddCylinderModel (lua_State * L)
{
	NewModel<noise::model::Cylinder>(L);	// model

	LuaXS::AttachMethods(L, MT_NAME(cylinder_model), [](lua_State * L) {
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
	});

	return 1;
}

//
//
//

MODEL_GETTER(Line)

static int AddLineModel (lua_State * L)
{
	NewModel<noise::model::Line>(L);// model

	LuaXS::AttachMethods(L, MT_NAME(line_model), [](lua_State * L) {
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
	});

	return 1;
}

//
//
//

MODEL_GETTER(Plane)

static int AddPlaneModel (lua_State * L)
{
	NewModel<noise::model::Plane>(L);	// model

	LuaXS::AttachMethods(L, MT_NAME(plane_model), [](lua_State * L) {
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
	});

	return 1;
}

//
//
//

MODEL_GETTER(Sphere)

static int AddSphereModel (lua_State * L)
{
	NewModel<noise::model::Sphere>(L);	// model

	LuaXS::AttachMethods(L, MT_NAME(sphere_model), [](lua_State * L) {
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
	});

	return 1;
}

//
//
//

void AddModels (lua_State * L)
{
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
}