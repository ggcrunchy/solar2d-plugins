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

PoissonCoords * PoissonCoords::Instantiate (lua_State * L, size_t n, bool bReserve)
{
	PoissonCoords * pc = New<PoissonCoords>(L);// ..., coords

	bReserve ? pc->mCoords.reserve(n) : pc->mCoords.resize(n);

	if (luaL_newmetatable(L, "morphing.poisson_coords")) // ..., coords, pc_mt
	{
		luaL_Reg poisson_coords_methods[] = {
			{
				"__gc", GC<PoissonCoords>
			}, {
				"GetData", [](lua_State * L)
				{
					PoissonCoords * pc = Get<PoissonCoords>(L);

					bool write_bytes = GetWriteBytes(L);// pc

					Emit(L, pc->mCoords, write_bytes);	// pc, pt

					return 1;
				}
			}, {
				"Map", [](lua_State * L)
				{
					luaL_argcheck(L, CompareMeta(L, "morphing.poisson_state"), 2, "Map expects a Poisson state");

					PoissonCoords * pc = Get<PoissonCoords>(L);
					PoissonState * ps = Get<PoissonState>(L, 2);

					luaL_argcheck(L, pc->mCoords.size() == ps->mPoints.size(), 2, "Coord / size mismatch");

					double x = 0, y = 0;

					for (size_t i = 0; i < pc->mCoords.size(); ++i)
					{
						x += pc->mCoords[i] * ps->mPoints[i].x;
						y += pc->mCoords[i] * ps->mPoints[i].y;
					}

					lua_pushnumber(L, x);	// pc, ps, x
					lua_pushnumber(L, y);	// pc, ps, x, y

					return 2;
				}
			},
			{ NULL, NULL }
		};

		SetMethods(L, poisson_coords_methods);
	}

	lua_setmetatable(L, -2);// ..., coords
	
	return pc;
}