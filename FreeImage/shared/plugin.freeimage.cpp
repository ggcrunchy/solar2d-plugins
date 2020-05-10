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

// plugin.freeimage.cpp : Defines the exported functions for the DLL application.
//

#include "stdafx.h"
#include "classes.h"

#define FIP_GETTER(name) fip##name & name (lua_State * L, int index) { return Check<fip##name>(L, "fip"#name, index); }

#ifdef _WIN32
	fipImage & Image (lua_State * L, int index) { return Check<fipImage>(L, "fipImage", "fipWinImage", index); }
	FIP_GETTER(WinImage)
#else
	FIP_GETTER(Image)
#endif

FIP_GETTER(MemoryIO)
FIP_GETTER(MetadataFind)
FIP_GETTER(MultiPage)
FIP_GETTER(Tag)

static bool AuxEq (lua_State * L, const char * name)
{
	luaL_getmetatable(L, name);	// ..., object_mt, named_mt

	return lua_equal(L, -2, -1) != 0;
}

bool IsType (lua_State * L, const char * name, int index)
{
	if (!lua_getmetatable(L, index)) return false;	// ..., object_mt

	bool eq = AuxEq(L, name); // ..., object_mt, named_mt

	lua_pop(L, 2);	// ...

	return eq;
}

bool IsType (lua_State * L, const char * name, const char * alt, int index)
{
	if (!lua_getmetatable(L, index)) return false;	// ..., object_mt

	bool eq = AuxEq(L, name);	// object_mt, named_mt

	if (!eq) 
	{
		lua_pop(L, 1);	// object_mt

		eq = AuxEq(L, alt);	// object_mt, alt_mt
	}

	lua_pop(L, 2);	// ...

	return eq;
}

luaL_Reg no_funcs[] = { { NULL, NULL } };

extern "C" {
    
LUALIB_API int luaopen_plugin_freeimage (lua_State * L)
{
	CoronaLibraryNew(L, "freeimage", "com.xibalbastudios", 1, 0, no_funcs, NULL);	// freeimage

	lua_CFunction funcs[] = {
		&FI_LoadEnums, &FI_LoadFlags,
		&FI_LoadImage, &FI_LoadMemoryIO, &FI_LoadMetadataFind, &FI_LoadMultiPage, &FI_LoadTag 
#ifdef _WIN32
		, &FI_LoadWinImage
#endif
    };
    
    for (auto func : funcs)
    {
		lua_pushcfunction(L, func); // freeimage, func
		lua_pushvalue(L, -2);	// freeimage, func, freeimage
		lua_pcall(L, 1, 0, 0);	// freeimage
	}

	return 1;
}

}