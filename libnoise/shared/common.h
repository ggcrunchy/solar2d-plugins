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

#pragma once

#include "CoronaGraphics.h"
#include "CoronaLua.h"
#include "utils/LuaEx.h"
#include "noise.h"

//
//
//

noise::module::Module * Module (lua_State * L, int arg = 1);
int AuxGetFromEnvironment (lua_State * L, int index = -1);
void AuxSetInEnvironment (lua_State * L, int index = -1, int other_pos = 3);

//
//
//

#define MT_PREFIX "libnoise."
#define MT_NAME(name) MT_PREFIX #name

#define DO(mod, name) #name, [](lua_State * L)	\
		{						\
			mod(L)->name();	\
								\
			return 0;			\
		}

#define DO_1_ARG(mod, name, what) #name, [](lua_State * L)	\
		{											\
			mod(L)->name(LuaXS::what(L, 2));	\
													\
			return 0;								\
		}

#define DO_2_DOUBLES(mod, name) #name, [](lua_State * L)	\
		{																\
			mod(L)->name(LuaXS::Double(L, 2), LuaXS::Double(L, 3));	\
																		\
			return 0;													\
		}

#define AUX_GET_MODULE(mod, index)	[](lua_State * L)	\
		{																\
			auto * m = mod(L);											\
			return AuxGetFromEnvironment(L, index);	/* m, module? */	\
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
			m->setter(*other);							    \
			AuxSetInEnvironment(L, index); /* m, index */	\
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


//
//
//

void AddModels (lua_State * L);
void AddModuleBase (lua_State * L);
void AddModules (lua_State * L);
void AddProperties (lua_State * L);
void AddUtils (lua_State * L);
