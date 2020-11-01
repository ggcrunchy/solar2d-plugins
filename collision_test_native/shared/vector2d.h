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

#include <algorithm>

#pragma once

struct Vec2 {
	union {
		struct {
			lua_Number x, y;
		};
		lua_Number arr[2];
	};

	Vec2 operator + (const Vec2 & rhs) const { return { x + rhs.x, y + rhs.y }; }
	Vec2 operator - (const Vec2 & rhs) const { return { x - rhs.x, y - rhs.y }; }
	Vec2 operator - () const { return { -x, -y }; }
	Vec2 operator * (lua_Number n) const { return { x * n, y * n }; }
	bool operator < (const Vec2 & rhs) const { return x < rhs.x && y < rhs.y; }
	bool operator > (const Vec2 & rhs) const { return x > rhs.x && y > rhs.y; }

	Vec2 & operator += (const Vec2 & rhs) { *this = *this + rhs; return *this; }
	Vec2 & operator -= (const Vec2 & rhs) { *this = *this - rhs; return *this; }
	Vec2 & operator *= (lua_Number n) { *this = *this * n; return *this; }

	Vec2 Max (const Vec2 & rhs) const { return { std::max(x, rhs.x), std::max(y, rhs.y) }; }
	Vec2 Min (const Vec2 & rhs) const { return { std::min(x, rhs.x), std::min(y, rhs.y) }; }

	lua_Number Determinant (const Vec2 & rhs) const { return x * rhs.y - y * rhs.x; }
	lua_Number DotProduct (const Vec2 & rhs) const { return x * rhs.x + y * rhs.y; }
	lua_Number Length () const { return sqrt(LengthSquared()); }
	lua_Number LengthSquared () const { return x * x + y * y; }

	void Normalize ()
	{
		lua_Number length = Length();

		if (!AlmostZero(length)) *this *= 1.0 / length;
	}

	static bool AlmostZeroSquared (lua_Number squared_length) { return squared_length < 1e-12; }
	static bool AlmostZero (lua_Number length) { return AlmostZeroSquared(length * length); }
};

const Vec2 & GetConstVec2 (lua_State * L, int arg = 1);
Vec2 & GetVec2 (lua_State * L, int arg = 1);

int AuxNewVec2 (lua_State * L, const Vec2 & v);
