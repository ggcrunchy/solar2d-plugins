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

void MorphState::GetPointFromIndex (lua_State * L)
{
	int index = luaL_checkint(L, 2);

	Poly * poly = Poly::Instance(L);

	if (poly->mWidth == 0) luaL_error(L, "Poly width of 0, cannot process index");

	mPoint.x = poly->mXScale * (index % poly->mWidth);
	mPoint.y = poly->mYScale * (index / poly->mWidth);
}

MorphState::MorphState (lua_State * L, int top, bool bJustCoords) : mPoint(), mCoords(0)
{
	lua_settop(L, top);	// poly[, edges], p, offset

	int n = Get<Poly>(L)->Length(), offset = luaL_optint(L, top, 0);

	//
	int pi = top - 1;

	size_t len = lua_objlen(L, pi);

	switch (lua_type(L, pi))
	{
	case LUA_TSTRING:
		GetPointFromString(L, pi, mPoint, len, offset);

		break;
	case LUA_TTABLE:
		GetPointFromTable(L, pi, mPoint, len, offset);

		break;
	case LUA_TNUMBER:
		GetPointFromIndex(L);

		break;
	default:
		luaL_error(L, "Bad point");
	}

	//
	if (bJustCoords) mCoords = PoissonCoords::Instantiate(L, n); // poly[, edges], p, offset, coords, mt

	else mCoords = CubicCoords::Instantiate(L, n); // poly[, edges], p, offset, coords, mt
}