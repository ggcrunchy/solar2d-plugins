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

// plugin.serialize.cpp : Defines the exported functions for the DLL application.
//

#include "stdafx.h"

static void LoadSubmodules (lua_State * L)
{
	lua_newtable(L);// t, struct
	luaopen_struct(L);	// t, struct := structlib
	lua_setfield(L, -2, "struct");	// t = { struct = structlib }
//	lua_newtable(L);// t, marshallib (N.B. marshal creates own table)
	luaopen_marshal(L);	// t, marshallib := marshallib
	lua_setfield(L, -2, "marshal");	// t = { struct, marshal = marshallib }
	lua_newtable(L);// t, lpacklib
	luaopen_pack(L);// t, lpacklib := lpacklib
	lua_setfield(L, -2, "lpack");	// t = { struct, marshal, lpack = lpacklib }
}

extern "C" {
    
LUALIB_API int luaopen_plugin_serialize (lua_State * L)
{
	luaL_Reg serialize_funcs[] = {
		{
            "Reloader", [](lua_State * L)
            {
                lua_newtable(L);// ..., t
                
                LoadSubmodules(L);
                
                return 1;
            }
		},
		{ NULL, NULL }
	};

	CoronaLibraryNew(L, "serialize", "com.xibalbastudios", 1, 0, serialize_funcs, NULL);
	LoadSubmodules(L);

	return 1;
}
    
}
