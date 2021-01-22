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

int AuxGetFromEnvironment (lua_State * L, int index)
{
	lua_settop(L, 2 - (index >= 0));// object[, key]

	if (index >= 0) lua_pushinteger(L, index);	// object, key

	lua_getfenv(L, 1);	// object, key, env
	lua_insert(L, 2);	// object, env, key
	lua_gettable(L, 2);	// object, env, module?

	return 1;
}

void AuxSetInEnvironment (lua_State * L, int index, int other_pos)
{
	lua_settop(L, other_pos - (index >= 0));// object[, ...[, key]], other

	int key_pos = other_pos - 1;

	if (index >= 0)
	{
		lua_pushinteger(L, index);	// object[, ...], other, key
		lua_insert(L, key_pos);	// object[, ...], key, other
	}

	lua_getfenv(L, 1);	// object[, ...], key, other, env
	lua_insert(L, key_pos);	// object[, ...], env, key, other
	lua_settable(L, key_pos);	// object[, ...], env = { ..., [key] = other }
}

//
//
//

CORONA_EXPORT int luaopen_plugin_libnoise (lua_State* L)
{
	lua_newtable(L);// libnoise
	lua_newtable(L);// libnoise, module_list
	lua_setfield(L, LUA_REGISTRYINDEX, MT_NAME(modules)); // libnoise; registry.modules = module_list

	AddModuleBase(L);	// libnoise, module_mt
	AddModules(L);// libnoise
	AddProperties(L);
	AddModels(L);
	AddUtils(L);

	return 1;
}