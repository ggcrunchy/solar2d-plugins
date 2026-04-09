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
#include "common.h"
#include "interface.h"
#include "state_data.h"

//
//
//

MAKE_KEY_GETTER(WeakValuedLookup);

//
//
//

static int GetBoxedInfo (lua_State * L)
{
	StateData * main = (StateData *)lua_touserdata(L, lua_upvalueindex(1));
	const BoxedGCObject * obj = main->GetBoxedGCObject(1);
	
	/* ----- */

	if (obj)
	{
		PushWeakValuedData(main, obj->mOwner); // object, owner

		/* ----- */

		obj->mOwner->GetRef(obj->mObjectRef); // OWNER: ..., object

		int type = obj->mOwner->Type(-1);
		
		obj->mOwner->Pop(1); // OWNER: ...

		/* ----- */
	
		const int LUA_TCDATA = LUA_TTHREAD + 2; // cf. lj_obj.h in LuaJIT
		
		if (type == LUA_TCDATA) lua_pushliteral(L, "cdata"); // object, owner, "cdata"
		else lua_pushstring(L, lua_typename(L, type)); // object, owner, typename

		/* ----- */
			
		return 2;
	}

	/* ----- */

	else
	{
		lua_pushnil(L); // obj, nil
		
		return 1;
	}
}

//
//
//

static int NewState (lua_State * L)
{
	StateData * main = (StateData *)lua_touserdata(L, lua_upvalueindex(1));

	/* ----- */
	
	StateDataLJ::New(main); // bridge, sd_lj

	/* ----- */

	lua_createtable(L, 0, 1); // bridge, sd_lj, env
	lua_pushvalue(L, lua_upvalueindex(1)); // bridge, sd_lj, env, main
	lua_setfield(L, -2, "main_state"); // bridge, sd_lj, env = { main_state = env }
	lua_setfenv(L, -2); // bridge, sd_lj; sd_lj.environment = env

	/* ----- */
	
	return 1;
}

//
//
//

static int Reloader (lua_State * L)
{
	lua_newtable(L); // bridge
    
	/* ----- */
	
	StateData temp = MakeLuaStateData(L), * sd_lua = temp.Make<StateData>(); // ..., sd_lua

	*sd_lua = temp;
	
	/* ----- */
    
    const luaL_Reg closures[] = {
		{ "GetBoxedInfo", GetBoxedInfo },
		{ "NewState", NewState }
	};
    
    for (const luaL_Reg & reg : closures)
    {
		lua_pushvalue(L, -1); // bridge, sd_lua, sd_lua
		lua_pushcclosure(L, reg.func, 1); // bridge, sd_lua, closure
		lua_setfield(L, -3, reg.name); // bridge = { ..., [name] = closure }, sd_lua
    }
	
	/* ----- */
	
	lua_pop(L, 1); // bridge
	
	/* ----- */

    if (!sd_lua->InstallPrelude()) return lua_error(L); // bridge, proxy_mt / error

	/* ----- */
	
	lua_pushlightuserdata(L, KEY_GETTER(WeakValuedLookup)()); // bridge, wvl_key
	lua_newtable(L); // bridge, wvl_key, wvl
	lua_createtable(L, 0, 1); // bridge, wwl_key, wvl, weak_mt
	lua_pushliteral(L, "v"); // bridge, wwl_key, wvl, weak_mt, "v"
	lua_setfield(L, -2, "__mode"); // bridge, wvl_key, wvl, weak_mt = { __mode = "v" }
	lua_setmetatable(L, -2); // bridge, wvl_key, wvl; wvl.metatable = weak_mt
	lua_rawset(L, LUA_REGISTRYINDEX); // bridge; registry[wvl_key] = wvl

	/* ----- */

	return 1;
}

//
//
//

CORONA_EXPORT int luaopen_plugin_luajitbridge (lua_State * L)
{
	Reloader(L); // bridge

	/* ----- */

	lua_pushcfunction(L, Reloader); // bridge, reloader
	lua_setfield(L, -2, "Reloader"); // bridge = { ..., Reloader = reloader }

	/* ----- */

	return 1;
}
