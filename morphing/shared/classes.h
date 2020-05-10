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

#include "stdafx.h"
#include "PoissonMVCs.h"

struct Poly {
	pvec mPoints;
	ivec mEdges;
	BaseCircle mCirc;
	int mWidth;
	double mXScale, mYScale;

	static Poly * Instance (lua_State * L);

	Poly (void) : mEdges(), mCirc(), mWidth(0), mXScale(0.0), mYScale(0.0) {}

	size_t Length (void) const
	{
		return mPoints.size();
	}
};

struct PoissonCoords {
	dvec mCoords;

	static PoissonCoords * Instantiate (lua_State * L, size_t n, bool bReserve = false);
};

struct PoissonState {
	pvec mPoints;
	enum Mode { eNormal, eMidpoint, eCatmullRom } mMode;

	PoissonState (void) : mMode(eNormal) {}
	Point2D GetPoint (size_t pos);

	static PoissonState * Instantiate (lua_State * L, size_t n, bool bReserve = false);
};

struct CubicCoords : PoissonCoords {
	dvec mGNCoords, mGTCoords;

	static CubicCoords * Instantiate (lua_State * L, size_t n, bool bReserve = false);
};

struct CubicState : PoissonState {
	pvec mTangent, mNormal; // TODO: Two of each? What if there are holes?

	static CubicState * Instantiate (lua_State * L, size_t n, bool bReserve = false);
};

struct MorphState {
	Point2D mPoint;
	PoissonCoords * mCoords;

	void GetPointFromIndex (lua_State * L);

	MorphState (lua_State * L, int top, bool bJustCoords);
};

extern luaL_Reg holes_methods[];
extern luaL_Reg normal_methods[];