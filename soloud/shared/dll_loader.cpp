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

#ifndef _WIN32
	#error DLL loader not supported
#endif

#include "common.h"
#include "dll_loader.h"

//
//
//

static void AddOpenmpt (lua_State * L, int & ref)
{
	int top = lua_gettop(L);

	if (TryToAddPlugin(L, "plugin_MemoryLoader")) // ..., MemoryLoader?
	{
		lua_getfield(L, -1, "Mount"); // ..., MemoryLoader, MemoryLoader.Mount
		lua_pushliteral(L, "libopenmpt.zip"); // ..., MemoryLoader, MemoryLoader.Mount, "libopenmpt.zip"
		lua_call(L, 1, 0); // ..., MemoryLoader
		lua_getfield(L, -1, "LoadLibrary"); // ..., MemoryLoader, MemoryLoader.LoadLibrary
		lua_pushliteral(L, "libopenmpt"); // ..., MemoryLoader, MemoryLoader.LoadLibrary, "libopenmpt"
		lua_call(L, 1, 1); // ..., MemoryLoader, Openmpt?

		ref = lua_ref(L, 1); // ...; ref = Openmpt
	}

	lua_settop(L, top); // ...
}

//
//
//

static void * AuxGetProcFromDLL (lua_State * L, int openmpt, const char * name)
{
	if (LUA_REFNIL == openmpt) return nullptr;

	lua_getref(L, openmpt); // ..., Openmpt
	lua_getfield(L, -1, "GetProc"); // ..., Openmpt, Openmpt:GetProc
	lua_insert(L, -2); // Openmpt:GetProc, Openmpt
	lua_pushstring(L, name); // ..., Openmpt:GetProc, Openmpt, name
	lua_call(L, 2, 1); // ..., proc?

	void * proc = nullptr;

	if (!lua_isnil(L, -1)) proc = *LuaXS::UD<void *>(L, -1);

	lua_pop(L, 1); // ...

	return proc;
}

//
//
//

static lua_State * sLuaState;

static int sOpenmptRef;

//
//
//

void * GetProcFromDLL (const char * name)
{
	if (LUA_NOREF == sOpenmptRef) AddOpenmpt(sLuaState, sOpenmptRef);

	return AuxGetProcFromDLL(sLuaState, sOpenmptRef, name);
}

//
//
//

void AddLoader (lua_State * L)
{
	sLuaState = L;
	sOpenmptRef = LUA_NOREF;
}