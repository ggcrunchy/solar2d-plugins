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
#include "vector3d.h"

#define VEC3_METATABLE "ctnative.vec3"

const Vec3 & GetConstVec3 (lua_State * L, int arg)
{
	return *static_cast<const Vec3 *>(luaL_checkudata(L, arg, VEC3_METATABLE));
}

Vec3 & GetVec3 (lua_State * L, int arg)
{
	return *static_cast<Vec3 *>(luaL_checkudata(L, arg, VEC3_METATABLE));
}

static Vec3 MakeVec (lua_State * L, int arg1)
{
	return Vec3{ luaL_checknumber(L, arg1), luaL_checknumber(L, arg1 + 1), luaL_checknumber(L, arg1 + 2) };
}

int AuxNewVec3 (lua_State * L, const Vec3 & v)
{
	memcpy(lua_newuserdata(L, sizeof(Vec3)), &v, sizeof(Vec3)); // [x, [y, ]]vec

	if (luaL_newmetatable(L, VEC3_METATABLE)) // vec, vec_mt
	{
		luaL_Reg methods[] = {
			{
				"__add", [](lua_State * L)
				{
					return AuxNewVec3(L, GetConstVec3(L, 1) + GetConstVec3(L, 2));
				}
			},
			{
				"Add", [](lua_State * L)
				{
					GetVec3(L) += GetConstVec3(L, 2);

					return 0;
				}
			},
			{
				"AddScaled", [](lua_State * L)
				{
					GetVec3(L) += GetConstVec3(L, 2) * luaL_checknumber(L, 3);

					return 0;
				}
			},
			{
				"AddScaledXYZ", [](lua_State * L)
				{
					GetVec3(L) += MakeVec(L, 2) * luaL_checknumber(L, 5);

					return 0;
				}
			},
			{
				"AddXYZ", [](lua_State * L)
				{
					GetVec3(L) += MakeVec(L, 2);

					return 0;
				}
			},
			{
				"AssignTo", [](lua_State * L)
				{
					const Vec3 & v = GetConstVec3(L);

					lua_pushnumber(L, v.x); // v, object, x
					lua_setfield(L, 2, "x"); // v, object; object.x = x
					lua_pushnumber(L, v.y); // v, object, y
					lua_setfield(L, 2, "y"); // v, object; object.y = y
					lua_pushnumber(L, v.z); // v, object, z
					lua_setfield(L, 2, "z"); // v, object; object.z = z

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
					lua_pushnumber(L, GetConstVec3(L).Determinant(GetConstVec3(L, 2))); // v, w, det

					return 1;
				}
			},
			{
				"__div", [](lua_State * L)
				{
					return AuxNewVec3(L, GetConstVec3(L, 1) * (1.0 / luaL_checknumber(L, 2)));
				}
			},
			{
				"DotProduct", [](lua_State * L)
				{
					lua_pushnumber(L, GetConstVec3(L).DotProduct(GetConstVec3(L, 2))); // v, w, dot

					return 1;
				}
			},
			{
				"__index", [](lua_State * L)
				{
					if (lua_isstring(L, 2))
					{
						const char * str = lua_tostring(L, 2);

						if (('x' == *str || 'y' == *str || 'z' == *str) && !str[1])
						{
							lua_pushnumber(L, GetConstVec3(L).arr[*str - 'x']); // t, k, x / y / z

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
					lua_pushboolean(L, Vec3::AlmostZeroSquared((GetConstVec3(L) - GetConstVec3(L, 2)).LengthSquared())); // v, almost_equal

					return 1;
				}
			},
			{
				"IsAlmostOrthogonalTo", [](lua_State * L)
				{
					lua_pushboolean(L, Vec3::AlmostZero(GetConstVec3(L).DotProduct(GetConstVec3(L, 2)))); // v, almost_orthogonal

					return 1;
				}
			},
			{
				"IsAlmostZero", [](lua_State * L)
				{
					lua_pushboolean(L, Vec3::AlmostZeroSquared(GetConstVec3(L).LengthSquared())); // v, almost_zero

					return 1;
				}
			},
			{
				"Length", [](lua_State * L)
				{
					lua_pushnumber(L, GetConstVec3(L).Length()); // v, length

					return 1;
				}
			},
			{
				"LengthSquared", [](lua_State * L)
				{
					lua_pushnumber(L, GetConstVec3(L).LengthSquared()); // v, length_squared

					return 1;
				}
			},
			{
				"__mul", [](lua_State * L)
				{
					int ni = lua_isnumber(L, 1); // 1 if in stack position 1, else 0
					const Vec3 & v = GetConstVec3(L, 1 + ni); // thus this will be 2 or 1
					lua_Number n = luaL_checknumber(L, 2 - ni); // while this is 1 or 2

					return AuxNewVec3(L, v * n);
				}
			},
			{
				"Negate", [](lua_State * L)
				{
					Vec3 & v = GetVec3(L);

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

						if (('x' == *str || 'y' == *str || 'z' == *str) && !str[1]) GetVec3(L).arr[*str - 'x'] = luaL_checknumber(L, 3);
					}

					return 0;
				}
			},
			{
				"Normalize", [](lua_State * L)
				{
					GetVec3(L).Normalize();

					return 0;
				}
			},
			{
				"ScaleBy", [](lua_State * L)
				{
					GetVec3(L) *= luaL_checknumber(L, 2);

					return 0;
				}
			},
			{
				"ScaleToLength", [](lua_State * L)
				{
					Vec3 & v = GetVec3(L);

					v.Normalize();

					v *= luaL_checknumber(L, 2);

					return 0;
				}
			},
			{
				"Set", [](lua_State * L)
				{
					GetVec3(L) = GetConstVec3(L, 2);

					return 0;
				}
			},
			{
				"SetDifference", [](lua_State * L)
				{
					GetVec3(L) = GetConstVec3(L, 2) - GetConstVec3(L, 3);

					return 0;
				}
			},
			{
				"SetDifferenceVectorXYZ", [](lua_State * L)
				{
					GetVec3(L) = GetConstVec3(L, 2) - MakeVec(L, 3);

					return 0;
				}
			},
			{
				"SetDifferenceXYVector", [](lua_State * L)
				{
					GetVec3(L) = MakeVec(L, 2) - GetConstVec3(L, 4);

					return 0;
				}
			},
			{
				"SetFrom", [](lua_State * L)
				{
					const char * xkey = luaL_optstring(L, 3, "x");
					const char * ykey = luaL_optstring(L, 4, "y");
					const char * zkey = luaL_optstring(L, 5, "z");

					lua_getfield(L, 2, xkey); // v, object[, xkey[, ykey]], x
					lua_getfield(L, 2, ykey); // v, object[, xkey[, ykey]], x, y
					lua_getfield(L, 2, zkey); // v, object[, xkey[, ykey[, zkey]]], x, y, z

					lua_Number x = luaL_checknumber(L, -3);
					lua_Number y = luaL_checknumber(L, -2);
					lua_Number z = luaL_checknumber(L, -1);
					Vec3 & v = GetVec3(L);

					v.x = x;
					v.y = y;
					v.z = z;

					return 0;
				}
			},
			{
				"SetNormalized", [](lua_State * L)
				{
					Vec3 & v = GetVec3(L);

					v = GetConstVec3(L, 2);

					v.Normalize();

					return 0;
				}
			},
			{
				"SetNormalizedXYZ", [](lua_State * L)
				{
					Vec3 & v = GetVec3(L);

					v = MakeVec(L, 2);

					v.Normalize();

					return 0;
				}
			},
			{
				"SetProjectionOfPointOntoRay", [](lua_State * L)
				{
					Vec3 & v = GetVec3(L);
					const Vec3 & origin = GetConstVec3(L, 3);
					const Vec3 to_point = GetConstVec3(L, 2) - origin, ray = GetConstVec3(L, 4);
					lua_Number denominator = ray.LengthSquared();

					if (!Vec3::AlmostZeroSquared(denominator)) v = ray * (to_point.DotProduct(ray) / denominator);
					else v = {};

					v += origin;

					return 0;
				}
			},
			{
				"SetProjectionOfPointOntoSegment", [](lua_State * L)
				{
					Vec3 & v = GetVec3(L);
					const Vec3 & p1 = GetConstVec3(L, 3);
					const Vec3 to_point = GetConstVec3(L, 2) - p1, segment = GetConstVec3(L, 4) - p1;
					lua_Number denominator = segment.LengthSquared();

					if (!Vec3::AlmostZeroSquared(denominator)) v = segment * (to_point.DotProduct(segment) / denominator);
					else v = {};

					v += p1;

					return 0;
				}
			},
			{
				"SetProjectionOntoVector", [](lua_State * L)
				{
					Vec3 & v = GetVec3(L);
					const Vec3 & w = GetConstVec3(L, 2);
					lua_Number denominator = w.LengthSquared();

					if (!Vec3::AlmostZeroSquared(denominator)) v = w * (v.DotProduct(w) / denominator);
					else v = {};

					return 0;
				}
			},
			{
				"SetScaled", [](lua_State * L)
				{
					GetVec3(L) = GetConstVec3(L, 2) * luaL_checknumber(L, 3);

					return 0;
				}
			},
			{
				"SetScaledXYZ", [](lua_State * L)
				{
					GetVec3(L) = MakeVec(L, 2) * luaL_checknumber(L, 5);

					return 0;
				}
			},
			{
				"SetSum", [](lua_State * L)
				{
					GetVec3(L) = GetConstVec3(L, 2) + GetConstVec3(L, 3);

					return 0;
				}
			},
			{
				"SetXY", [](lua_State * L)
				{
					GetVec3(L) = MakeVec(L, 2);

					return 0;
				}
			},
			{
				"SetZero", [](lua_State * L)
				{
					GetVec3(L) = {};

					return 0;
				}
			},
			{
				"__sub", [](lua_State * L)
				{
					return AuxNewVec3(L, GetConstVec3(L, 1) - GetConstVec3(L, 2));
				}
			},
			{
				"Sub", [](lua_State * L)
				{
					GetVec3(L) -= GetConstVec3(L, 2);

					return 0;
				}
			},
			{
				"SubScaled", [](lua_State * L)
				{
					GetVec3(L) -= GetConstVec3(L, 2) * luaL_checknumber(L, 3);

					return 0;
				}
			},
			{
				"SubScaledXYZ", [](lua_State * L)
				{
					GetVec3(L) -= MakeVec(L, 2) * luaL_checknumber(L, 5);

					return 0;
				}
			},
			{
				"SubXYZ", [](lua_State * L)
				{
					GetVec3(L) -= MakeVec(L, 2);

					return 0;
				}
			},
			{
				"__unm", [](lua_State * L)
				{
					return AuxNewVec3(L, -GetConstVec3(L));
				}
			},
			{ nullptr, nullptr }
		};

		luaL_register(L, nullptr, methods);
	}

	lua_setmetatable(L, -2); // vec

	return 1;
}

int NewVec3 (lua_State * L)
{
	return AuxNewVec3(L, { luaL_optnumber(L, 1, 0.0), luaL_optnumber(L, 2, 0.0), luaL_optnumber(L, 3, 0.0) });
}