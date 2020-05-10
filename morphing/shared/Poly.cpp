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
#include "CubicMVCs.h"
#include "PoissonMVCs.h"

Poly * Poly::Instance (lua_State * L)
{
	return Get<Poly>(L);
}

static int NewCubicState (lua_State * L)
{
	CubicState::Instantiate(L, Poly::Instance(L)->Length());

	return 1;
}

static int NewPoissonState (lua_State * L)
{
	PoissonState::Instantiate(L, Poly::Instance(L)->Length());

	return 1;
}

static int PolyLen (lua_State * L)
{
	lua_pushinteger(L, Poly::Instance(L)->Length());

	return 1;
}

static int SetDimsAndScale (lua_State * L)
{
	Poly * poly = Poly::Instance(L);

	poly->mWidth = luaL_optint(L, 2, 0);
	poly->mXScale = luaL_optnumber(L, 3, 0.0);
	poly->mYScale = luaL_optnumber(L, 4, 0.0);

	return 0;
}

luaL_Reg normal_methods[] = {
	{
		"BasicPoissonMVC", [](lua_State * L)
		{
			MorphState ms(L, 3, true);	// poly, p, ..., coords

			basicPoissonMVCs(Poly::Instance(L)->mPoints, ms.mPoint, ms.mCoords->mCoords);

			return 1;
		}
	}, {
		"CubicMVCs", [](lua_State * L)
		{
			MorphState ms(L, 3, false);	// poly, p, ..., coords
			CubicCoords * cc = (CubicCoords *)ms.mCoords;

			cubicMVCs(Poly::Instance(L)->mPoints, ms.mPoint, cc->mCoords, cc->mGNCoords, cc->mGTCoords);

			return 1;
		}
	}, {
		"__gc", GC<Poly>
	}, {
		"__len", &PolyLen
	}, {
		"NewCubicState", &NewCubicState
	}, {
		"NewPoissonState", &NewPoissonState
	}, {
		"PoissonMVCs", [](lua_State * L)
		{
			Poly * poly = Poly::Instance(L);

			MorphState ms(L, 3, true);	// poly, p, ..., coords

			poissonMVCs(poly->mPoints, ms.mPoint, ms.mCoords->mCoords, poly->mCirc);

			return 1;
		}
	}, {
		"SetDimsAndScale", &SetDimsAndScale
	},
	{ NULL, NULL }
};

luaL_Reg holes_methods[] = {
	{
		"BasicPoissonMVC", [](lua_State * L)
		{
			Poly * poly = Poly::Instance(L);
			MorphState ms(L, 3, true);	// poly, p, ..., coords

			basicPoissonMVCs(poly->mPoints, poly->mEdges, ms.mPoint, ms.mCoords->mCoords, poly->mCirc);

			return 1;
		}
	}, {
		"CubicMVCs", [](lua_State * L)
		{
			Poly * poly = Poly::Instance(L);
			MorphState ms(L, 3, false);	// poly, p, ..., coords
			CubicCoords * cc = (CubicCoords *)ms.mCoords;

			cubicMVCs(poly->mPoints, poly->mEdges, ms.mPoint, cc->mCoords, cc->mGNCoords, cc->mGTCoords);

			return 1;
		}
	}, {
		"__gc", GC<Poly>
	}, {
		"__len", &PolyLen
	}, {
		"NewCubicState", &NewCubicState
	}, {
		"NewPoissonState", &NewPoissonState
	}, {
		"PoissonMVCs", [](lua_State * L)
		{
			Poly * poly = Poly::Instance(L);
			MorphState ms(L, 3, true);	// poly, p, ..., coords

			poissonMVCs(poly->mPoints, poly->mEdges, ms.mPoint, ms.mCoords->mCoords, poly->mCirc);

			return 1;
		}
	}, {
		"SetDimsAndScale", &SetDimsAndScale
	},
	{ NULL, NULL }
};