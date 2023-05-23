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

#define DPARAMS_METATABLE_NAME "blosc2.dparams"

//
//
//

static ParamsBox<blosc2_dparams> * AuxGet (lua_State * L, int arg)
{
	return LuaXS::CheckUD<ParamsBox<blosc2_dparams>>(L, arg, DPARAMS_METATABLE_NAME);
}

//
//
//

blosc2_dparams * GetDparams (lua_State * L, int arg)
{
	return AuxGet(L, arg)->Get();
}

//
//
//

static void AddMethods (lua_State * L)
{
	LuaXS::AttachMethods(L, DPARAMS_METATABLE_NAME, [](lua_State * L) {
		luaL_Reg funcs[] = {
			{
				"__gc", [](lua_State * L)
				{
					AuxGet(L, 1)->Free();

					return 0;
				}
			}, {
				"__newindex", [](lua_State * L)
				{
					const char * str = lua_tostring(L, 2);

					if (str && strcmp(str, "nthreads") == 0) GetDparams(L)->nthreads = (int16_t)luaL_checkint(L, 3);
					// TODO?: filters

					return 0;
				}
			},
			{ nullptr, nullptr }
		};

		luaL_register(L, nullptr, funcs);

		//
		//
		// 
 
		LuaXS::AttachProperties(L, [](lua_State * L) {
			const char * str = lua_tostring(L, 2);

			if (!str) return 0;

			const blosc2_dparams * dparams = GetDparams(L);

			if (strcmp(str, "nthreads") == 0) lua_pushinteger(L, dparams->nthreads); // dparams, "nthreads", nthreads
			else if (strcmp(str, "schunk") == 0) GetSchunk(L, 1, dparams->schunk); // dparams, "schunk", schunk / nil
			// TODO?: filters
			else return 0;

			return 1;
		});
	});
}

//
//
//

void WrapDparams (lua_State * L, blosc2_dparams * dparams)
{
	LuaXS::NewTyped<ParamsBox<blosc2_dparams>>(L, dparams); // ..., dparams

	AddMethods(L);
}

//
//
//

void AddDparams (lua_State * L)
{
	luaL_Reg funcs[] = {
		{
			"dparams_new", [](lua_State * L)
			{
				LuaXS::NewTyped<ParamsBox<blosc2_dparams>>(L, BLOSC2_DPARAMS_DEFAULTS); // ..., dparams

				AddMethods(L);

				return 1;
			}
		},
		{ nullptr, nullptr }
	};

	luaL_register(L, nullptr, funcs);
}