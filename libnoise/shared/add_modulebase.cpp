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

noise::module::Module * Module (lua_State * L, int arg)
{
	luaL_checktype(L, arg, LUA_TUSERDATA);
	lua_getmetatable(L, arg);	// ..., ud, ..., mt
	lua_getfield(L, LUA_REGISTRYINDEX, MT_NAME(modules));	// ..., ud, ..., mt, module_list
	lua_insert(L, -2);	// ..., ud, ..., module_list, mt
	lua_gettable(L, -2);// ..., ud, ..., module_list, exists?
	luaL_argcheck(L, lua_toboolean(L, -1), arg, "Not a module");
	lua_pop(L, 2);	// ..., ud, ...

	return LuaXS::UD<noise::module::Module>(L, arg);
}

//
//
//

void AddModuleBase (lua_State * L)
{
	lua_createtable(L, 0, 3);	// mt
	lua_pushvalue(L, -1);	// mt, mt
	lua_setfield(L, -2, "__index");	// mt = { __index = mt }

	luaL_Reg funcs[] = {
		{
			"__gc", LuaXS::TypedGC<noise::module::Module>
		}, {
			"GetSourceModule", [](lua_State * L)
			{
				auto * mod = Module(L);

				luaL_checkint(L, 2);

				return AuxGetFromEnvironment(L);// mod, env, source?
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
				
				AuxSetInEnvironment(L);	// mod, env

				return 0;
			}
		},
		{ nullptr, nullptr }
	};

	luaL_register(L, nullptr, funcs);
}