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

#include "CoronaLua.h"
#include "common.h"
#include "vector2d.h"

#define VEC2_METATABLE "ctnative.vec2"

const Vec2 & GetConstVec2 (lua_State * L, int arg)
{
	return *static_cast<const Vec2 *>(luaL_checkudata(L, arg, VEC2_METATABLE));
}

Vec2 & GetVec2 (lua_State * L, int arg)
{
	return *static_cast<Vec2 *>(luaL_checkudata(L, arg, VEC2_METATABLE));
}

static Vec2 MakeVec (lua_State * L, int arg1)
{
	return Vec2{ luaL_checknumber(L, arg1), luaL_checknumber(L, arg1 + 1) };
}

int AuxNewVec2 (lua_State * L, const Vec2 & v)
{
	memcpy(lua_newuserdata(L, sizeof(Vec2)), &v, sizeof(Vec2)); // [x, [y, ]]vec

	if (luaL_newmetatable(L, VEC2_METATABLE)) // vec, vec_mt
	{
		luaL_Reg methods[] = {
			{
				"__add", [](lua_State * L)
				{
					return AuxNewVec2(L, GetConstVec2(L, 1) + GetConstVec2(L, 2));
				}
			},
			{
				"Add", [](lua_State * L)
				{
					GetVec2(L) += GetConstVec2(L, 2);

					return 0;
				}
			},
			{
				"AddScaled", [](lua_State * L)
				{
					GetVec2(L) += GetConstVec2(L, 2) * luaL_checknumber(L, 3);

					return 0;
				}
			},
			{
				"AddScaledXY", [](lua_State * L)
				{
					GetVec2(L) += MakeVec(L, 2) * luaL_checknumber(L, 4);

					return 0;
				}
			},
			{
				"AddXY", [](lua_State * L)
				{
					GetVec2(L) += MakeVec(L, 2);

					return 0;
				}
			},
			{
				"AssignTo", [](lua_State * L)
				{
					const Vec2 & v = GetConstVec2(L);

					lua_pushnumber(L, v.x); // v, object, x
					lua_setfield(L, 2, "x"); // v, object; object.x = x
					lua_pushnumber(L, v.y); // v, object, y
					lua_setfield(L, 2, "y"); // v, object; object.y = y

					return 0;
				}
			},
			{
				"Component", [](lua_State * L)
				{
					// TODO: does this mean v should be understood as normalized?

					return 0;
				}
			},
			{
				"Determinant", [](lua_State * L)
				{
					lua_pushnumber(L, GetConstVec2(L).Determinant(GetConstVec2(L, 2))); // v, w, det

					return 1;
				}
			},
			{
				"__div", [](lua_State * L)
				{
					return AuxNewVec2(L, GetConstVec2(L, 1) * (1.0 / luaL_checknumber(L, 2)));
				}
			},
			{
				"DotProduct", [](lua_State * L)
				{
					lua_pushnumber(L, GetConstVec2(L).DotProduct(GetConstVec2(L, 2))); // v, w, dot

					return 1;
				}
			},
			{
				"__index", [](lua_State * L)
				{
					if (lua_isstring(L, 2))
					{
						const char * str = lua_tostring(L, 2);

						if (('x' == *str || 'y' == *str) && !str[1])
						{
							lua_pushnumber(L, GetConstVec2(L).arr[*str - 'x']); // t, k, x / y

							return 1;
						}
					}

					lua_getmetatable(L, 1); // t, k, mt
					lua_replace(L, 1); // mt, k
					lua_rawget(L, 1); // mt, value?

					return 1;
				}
			},
			{
				"IsAlmostEqualTo", [](lua_State * L)
				{
					lua_pushboolean(L, Vec2::AlmostZeroSquared((GetConstVec2(L) - GetConstVec2(L, 2)).LengthSquared())); // v, almost_equal

					return 1;
				}
			},
			{
				"IsAlmostOrthogonalTo", [](lua_State * L)
				{
					lua_pushboolean(L, Vec2::AlmostZero(GetConstVec2(L).DotProduct(GetConstVec2(L, 2)))); // v, almost_orthogonal

					return 1;
				}
			},
			{
				"IsAlmostZero", [](lua_State * L)
				{
					lua_pushboolean(L, Vec2::AlmostZeroSquared(GetConstVec2(L).LengthSquared())); // v, almost_zero

					return 1;
				}
			},
			{
				"Length", [](lua_State * L)
				{
					lua_pushnumber(L, GetConstVec2(L).Length()); // v, length

					return 1;
				}
			},
			{
				"LengthSquared", [](lua_State * L)
				{
					lua_pushnumber(L, GetConstVec2(L).LengthSquared()); // v, length_squared

					return 1;
				}
			},
			{
				"__mul", [](lua_State * L)
				{
					int ni = lua_isnumber(L, 1); // 1 if in stack position 1, else 0
					const Vec2 & v = GetConstVec2(L, 1 + ni); // thus this will be 2 or 1
					lua_Number n = luaL_checknumber(L, 2 - ni); // while this is 1 or 2

					return AuxNewVec2(L, v * n);
				}
			},
			{
				"Negate", [](lua_State * L)
				{
					Vec2 & v = GetVec2(L);

					v = -v;

					return 0;
				}
			},
			{
				"__newindex", [](lua_State * L)
				{
					if (lua_isstring(L, 2))
					{
						const char * str = lua_tostring(L, 2);

						if (('x' == *str || 'y' == *str) && !str[1]) GetVec2(L).arr[*str - 'x'] = luaL_checknumber(L, 3);
					}

					return 0;
				}
			},
			{
				"Normalize", [](lua_State * L)
				{
					GetVec2(L).Normalize();

					return 0;
				}
			},
			{
				"ScaleBy", [](lua_State * L)
				{
					GetVec2(L) *= luaL_checknumber(L, 2);

					return 0;
				}
			},
			{
				"ScaleToLength", [](lua_State * L)
				{
					Vec2 & v = GetVec2(L);

					v.Normalize();

					v *= luaL_checknumber(L, 2);

					return 0;
				}
			},
			{
				"Set", [](lua_State * L)
				{
					GetVec2(L) = GetConstVec2(L, 2);

					return 0;
				}
			},
			{
				"SetDifference", [](lua_State * L)
				{
					GetVec2(L) = GetConstVec2(L, 2) - GetConstVec2(L, 3);

					return 0;
				}
			},
			{
				"SetDifferenceVectorXY", [](lua_State * L)
				{
					GetVec2(L) = GetConstVec2(L, 2) - MakeVec(L, 3);

					return 0;
				}
			},
			{
				"SetDifferenceXYVector", [](lua_State * L)
				{
					GetVec2(L) = MakeVec(L, 2) - GetConstVec2(L, 4);

					return 0;
				}
			},
			{
				"SetFrom", [](lua_State * L)
				{
					const char * xkey = luaL_optstring(L, 3, "x");
					const char * ykey = luaL_optstring(L, 4, "y");

					lua_getfield(L, 2, xkey); // v, object[, xkey[, ykey]], x
					lua_getfield(L, 2, ykey); // v, object[, xkey[, ykey]], x, y

					lua_Number x = luaL_checknumber(L, -2);
					lua_Number y = luaL_checknumber(L, -1);
					Vec2 & v = GetVec2(L);

					v.x = x;
					v.y = y;

					return 0;
				}
			},
			{
				"SetNormalized", [](lua_State * L)
				{
					Vec2 & v = GetVec2(L);

					v = GetConstVec2(L, 2);

					v.Normalize();

					return 0;
				}
			},
			{
				"SetNormalizedXY", [](lua_State * L)
				{
					Vec2 & v = GetVec2(L);

					v = MakeVec(L, 2);

					v.Normalize();

					return 0;
				}
			},
			{
				"SetPerp", [](lua_State * L)
				{
					const Vec2 & w = GetConstVec2(L, 2);

					GetVec2(L) = { -w.y, +w.x };

					return 0;
				}
			},
			{
				"SetProjectionOfPointOntoRay", [](lua_State * L)
				{
					Vec2 & v = GetVec2(L);
					const Vec2 & origin = GetConstVec2(L, 3);
					const Vec2 to_point = GetConstVec2(L, 2) - origin, ray = GetConstVec2(L, 4);
					lua_Number denominator = ray.LengthSquared();

					if (!Vec2::AlmostZeroSquared(denominator)) v = ray * (to_point.DotProduct(ray) / denominator);
					else v = {};

					v += origin;

					return 0;
				}
			},
			{
				"SetProjectionOfPointOntoSegment", [](lua_State * L)
				{
					Vec2 & v = GetVec2(L);
					const Vec2 & p1 = GetConstVec2(L, 3);
					const Vec2 to_point = GetConstVec2(L, 2) - p1, segment = GetConstVec2(L, 4) - p1;
					lua_Number denominator = segment.LengthSquared();

					if (!Vec2::AlmostZeroSquared(denominator)) v = segment * (to_point.DotProduct(segment) / denominator);
					else v = {};

					v += p1;

					return 0;
				}
			},
			{
				"SetProjectionOntoVector", [](lua_State * L)
				{
					Vec2 & v = GetVec2(L);
					const Vec2 & w = GetConstVec2(L, 2);
					lua_Number denominator = w.LengthSquared();

					if (!Vec2::AlmostZeroSquared(denominator)) v = w * (v.DotProduct(w) / denominator);
					else v = {};

					return 0;
				}
			},
			{
				"SetScaled", [](lua_State * L)
				{
					GetVec2(L) = GetConstVec2(L, 2) * luaL_checknumber(L, 3);

					return 0;
				}
			},
			{
				"SetScaledXY", [](lua_State * L)
				{
					GetVec2(L) = MakeVec(L, 2) * luaL_checknumber(L, 4);

					return 0;
				}
			},
			{
				"SetSum", [](lua_State * L)
				{
					GetVec2(L) = GetConstVec2(L, 2) + GetConstVec2(L, 3);

					return 0;
				}
			},
			{
				"SetXY", [](lua_State * L)
				{
					GetVec2(L) = MakeVec(L, 2);

					return 0;
				}
			},
			{
				"SetZero", [](lua_State * L)
				{
					GetVec2(L) = {};

					return 0;
				}
			},
			{
				"__sub", [](lua_State * L)
				{
					return AuxNewVec2(L, GetConstVec2(L, 1) - GetConstVec2(L, 2));
				}
			},
			{
				"Sub", [](lua_State * L)
				{
					GetVec2(L) -= GetConstVec2(L, 2);

					return 0;
				}
			},
			{
				"SubScaled", [](lua_State * L)
				{
					GetVec2(L) -= GetConstVec2(L, 2) * luaL_checknumber(L, 3);

					return 0;
				}
			},
			{
				"SubScaledXY", [](lua_State * L)
				{
					GetVec2(L) -= MakeVec(L, 2) * luaL_checknumber(L, 4);

					return 0;
				}
			},
			{
				"SubXY", [](lua_State * L)
				{
					GetVec2(L) -= MakeVec(L, 2);

					return 0;
				}
			},
			{
				"__unm", [](lua_State * L)
				{
					return AuxNewVec2(L, -GetConstVec2(L));
				}
			},
			{ nullptr, nullptr }
		};

		luaL_register(L, nullptr, methods);
	}

	lua_setmetatable(L, -2); // vec

	return 1;
}

int NewVec2 (lua_State * L)
{
	return AuxNewVec2(L, { luaL_optnumber(L, 1, 0.0), luaL_optnumber(L, 2, 0.0) });
}