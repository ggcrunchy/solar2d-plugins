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

#include "classes.h"
#include "utils.h"

CubicCoords * CubicCoords::Instantiate (lua_State * L, size_t n, bool bReserve)
{
	CubicCoords * cc = New<CubicCoords>(L);	// ..., coords

	bReserve ? cc->mCoords.reserve(n) : cc->mCoords.resize(n);
	bReserve ? cc->mGNCoords.reserve(2 * n) : cc->mGNCoords.resize(2 * n);
	bReserve ? cc->mGNCoords.reserve(2 * n) : cc->mGTCoords.resize(2 * n);

	if (luaL_newmetatable(L, "morphing.cubic_coords")) // ..., coords, cc_mt
	{
		luaL_Reg cubic_coords_methods[] = {
			{
				"__gc", GC<CubicCoords>
			}, {
				"GetData", [](lua_State * L)
				{
					CubicCoords * cc = Get<CubicCoords>(L);

					bool write_bytes = GetWriteBytes(L);// cc

					lua_createtable(L, 0, 3);	// cc, ct

					Emit(L, cc->mCoords, write_bytes, "vcoords");	// cc, ct = { vcoords = vcoords }
					Emit(L, cc->mGNCoords, write_bytes, "gn_coords");	// cc, ct = { vcoords, gn_coords = gn_coords }
					Emit(L, cc->mGTCoords, write_bytes, "gt_coords");	// cc, ct = { vcoords, gn_coords, gt_coords = gt_coords }

					return 1;
				}
			}, {
				"Map", [](lua_State * L)
				{
					luaL_argcheck(L, CompareMeta(L, "morphing.cubic_state"), 2, "Map expects a cubic state");

					CubicCoords * cc = Get<CubicCoords>(L);
					CubicState * cs = Get<CubicState>(L, 2);

					// TODO! (slightly unsure about tangents and normals, mostly)
					// Key line: f[v] = Sum[i]{a_i[v]f_i} + Sum[i]{b_i^s[v]f_i^s} + Sum[i]{c_i^s[v]h_i^s}
					// f_i: Value
					// f_i^s: Derivative, sign s
					// h_i^s: Normal, sign s

					return 2;
				}
			},
			{ NULL, NULL }
		};

		SetMethods(L, cubic_coords_methods);
	}
	
	lua_setmetatable(L, -2);// ..., coords

	return cc;
}