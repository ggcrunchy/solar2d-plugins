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

static int SetPoint (lua_State * L)
{
	lua_settop(L, 4);	// coord, index, point, offset

	pvec & points = Get<PoissonState>(L)->mPoints;
	int pos = luaL_checkint(L, 2) - 1, offset = luaL_optint(L, 4, 0);

	luaL_argcheck(L, pos >= 0 && size_t(pos) < points.size(), 2, "Out-of-range point");

	size_t len = lua_objlen(L, 3);

	switch (lua_type(L, 3))
	{
	case LUA_TSTRING:
		GetPointFromString(L, 3, points[pos], len, offset);

		break;
	case LUA_TTABLE:
		GetPointFromTable(L, 3, points[pos], len, offset);

		break;
	default:
		luaL_error(L, "Bad point");
	}

	return 0;
}

CubicState * CubicState::Instantiate (lua_State * L, size_t n, bool bReserve)
{
	CubicState * cs = New<CubicState>(L);

	// TODO: Reserve
	cs->mPoints.resize(n);
	cs->mTangent.resize(2 * n);	// ???
	cs->mNormal.resize(2 * n);	// Ditto

	if (luaL_newmetatable(L, "morphing.cubic_state"))	// ..., cs, cs_mt
	{
		luaL_Reg cubic_state_methods[] = {
			{
				"Clone", [](lua_State * L)
				{
					// TODO!
					return 0;
				}
			}, {
				"__gc", GC<CubicState>
			}, {
				"GetData", [](lua_State * L)
				{
				//	CubicState * cs = Get<CubicState>(L);

					// TODO: structure still uncertain

					return 0;
				}
			}, {
				"SetPoint", &SetPoint
			},
			{ NULL, NULL }
		};

		SetMethods(L, cubic_state_methods);
	}

	lua_setmetatable(L, -2);// ..., cs

	return cs;
}

PoissonState * PoissonState::Instantiate (lua_State * L, size_t n, bool bReserve)
{
	PoissonState * ps = New<PoissonState>(L);

	bReserve ? ps->mPoints.reserve(n) : ps->mPoints.resize(n);

	if (luaL_newmetatable(L, "morphing.poisson_state"))	// ..., ps, ps_mt
	{
		luaL_Reg poisson_state_methods[] = {
			{
				"Clone", [](lua_State * L)
				{
					PoissonState * from = Get<PoissonState>(L);

					*Instantiate(L, from->mPoints.size()) = *from;

					return 1;
				}
			}, {
				"__gc", GC<PoissonState>
			}, {
				"GetData", [](lua_State * L)
				{
					PoissonState * ps = Get<PoissonState>(L);

					bool write_bytes = GetWriteBytes(L);// ps

					Emit(L, ps->mPoints, write_bytes);	// ps, data

					return 1;
				}
			}, {
				"GetMode", [](lua_State * L)
				{
					switch (Get<PoissonState>(L)->mMode)
					{
					case PoissonState::eNormal:
						lua_pushliteral(L, "normal");	// ps, "normal"
						break;
					case PoissonState::eMidpoint:
						lua_pushliteral(L, "midpoint");	// ps, "midpoint"
						break;
					case PoissonState::eCatmullRom:
						lua_pushliteral(L, "catmull_rom");	// ps, "catmull_rom"
						break;
					default:
						luaL_error(L, "Unknown mode");
					}

					return 1;
				}
			}, {
				"GetPoint", [](lua_State * L)
				{
					PoissonState * ps = Get<PoissonState>(L);

					int pos = luaL_checkint(L, 2) - 1;

					luaL_argcheck(L, pos >= 0 && size_t(pos) < ps->mPoints.size(), 2, "Out-of-range point");

					Point2D point = ps->GetPoint(pos);

					lua_pushnumber(L, point.x);	// ps, pos, x
					lua_pushnumber(L, point.y);	// ps, pos, x, y

					return 2;
				}
			}, {
				"Interpolate", [](lua_State * L)
				{
					PoissonState * ps = Get<PoissonState>(L);
					PoissonState * from = Get<PoissonState>(L, 2, "morphing.poisson_state");
					PoissonState * to = Get<PoissonState>(L, 3, "morphing.poisson_state");

					luaL_argcheck(L, ps->mPoints.size() == from->mPoints.size(), 2, "State and 'from' interpolator size mismatch");
					luaL_argcheck(L, ps->mPoints.size() == to->mPoints.size(), 3, "State and 'to' interpolator size mismatch");

					if (lua_istable(L, 4))
					{
						lua_getfield(L, 4, "def");	// ps, from, to, t, def

						double def = luaL_optnumber(L, -1, 0.0);

						for (size_t i = 0; i < ps->mPoints.size(); ++i)
						{
							lua_rawgeti(L, 4, i + 1);	// ps, from, to, t, def, ti

							double t = luaL_optnumber(L, -1, def), s = 1.0 - t;

							ps->mPoints[i] = s * from->GetPoint(i) + t * to->GetPoint(i);

							lua_pop(L, 1);	// ps, from, to, t, def
						}
					}

					else
					{
						double t = luaL_checknumber(L, 4), s = 1.0 - t;

						for (size_t i = 0; i < ps->mPoints.size(); ++i) ps->mPoints[i] = s * from->GetPoint(i) + t * to->GetPoint(i);
					}

					return 0;
				}
			}, {
				"__len", [](lua_State * L)
				{
					lua_pushinteger(L, Get<PoissonState>(L)->mPoints.size());

					return 1;
				}
			}, {
				"SetMode", [](lua_State * L)
				{
					const char * names[] = { "normal", "midpoint", "catmull_rom" };
					Mode modes[] = { PoissonState::eNormal, PoissonState::eMidpoint, PoissonState::eCatmullRom };

					Get<PoissonState>(L)->mMode = modes[luaL_checkoption(L, 2, NULL, names)];
					
					return 0;
				}
			}, {
				"SetPoint", &SetPoint
			},
			{ NULL, NULL }
		};

		SetMethods(L, poisson_state_methods);
	}

	lua_setmetatable(L, -2);// ..., ps

	return ps;
}

Point2D PoissonState::GetPoint (size_t pos)
{
	Point2D point = mPoints[pos];

	if (mMode == PoissonState::eMidpoint)
	{
		size_t n = mPoints.size();
		size_t prev = (size_t(pos) > 0 ? size_t(pos) : n) - 1, next = size_t(pos) + 1 < n ? size_t(pos) + 1 : 0;

		point = 0.5 * (0.5 * (mPoints[prev] + mPoints[next]) + point);
	}

	else if (mMode == PoissonState::eCatmullRom)
	{
		//
	}

	return point;
}