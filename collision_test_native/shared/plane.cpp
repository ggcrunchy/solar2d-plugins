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

//
//
//

struct Plane {
	Vec3 position{}, normal{};
	lua_Number distance;

	void SetNormal (const Vec3 & n)
	{
		normal = n;
		distance = position.DotProduct(normal);
	}

	void SetPosition (const Vec3 & p)
	{
		position = p;
		distance = position.DotProduct(normal);
	}
};

//
//
//

#define PLANE_METATABLE "ctnative.plane"

static const Plane & GetConstPlane (lua_State * L, int arg = 1)
{
	return *static_cast<const Plane *>(luaL_checkudata(L, arg, PLANE_METATABLE));
}

static Plane & GetPlane (lua_State * L, int arg = 1)
{
	return *static_cast<Plane *>(luaL_checkudata(L, arg, PLANE_METATABLE));
}

static Vec3 & GetOutVector (lua_State * L, int top)
{
	lua_settop(L, top); // ..., v?

	if (!IsVec3(L, top))
	{
		AuxNewVec3(L, {}); // ..., other, v

		lua_replace(L, top); // ..., v
	}

	return GetVec3(L, top);
}

//
//
//

static int AuxNewPlane (lua_State * L, const Plane & p)
{
	memcpy(lua_newuserdata(L, sizeof(Plane)), &p, sizeof(Plane)); // [..., ]plane

	if (luaL_newmetatable(L, PLANE_METATABLE)) // ..., plane, plane_mt
	{
		luaL_Reg methods[] = {
			{
				"GetDistance", [](lua_State * L)
				{
					lua_pushnumber(L, GetPlane(L).distance); // plane, distance

					return 1;
				}
			}, {
				"GetIntersectionWith", [](lua_State * L)		
				{
					// (p(t) - P) . n = 0
					// (p + t * V) . n = P . n = d
					// d - p . n = t * (V . n)
					// t = (d - p . n) / (V . n)

					const Plane & plane = GetConstPlane(L);
					const Vec3 & v = GetConstVec3(L, 3);
					lua_Number denominator = v.DotProduct(plane.normal);

					if (abs(denominator) > 1e-12)
					{
						const Vec3 & p = GetConstVec3(L, 2);
						lua_Number t = (plane.distance - p.DotProduct(plane.normal)) / denominator;

						GetOutVector(L, 4) = p + v * t; // plane, p, v, intersection

						lua_pushnumber(L, t); // plane, p, v, intersection, t

						return 2;
					}

					else
					{
						lua_pushboolean(L, 0); // plane, p, v[, out], false

						return 1;
					}
				}
			}, {
				"GetNormal", [](lua_State * L)
				{
					GetOutVector(L, 2) = GetPlane(L).normal; // plane, normal

					return 1;
				}
			}, {
				"GetPosition", [](lua_State * L)
				{
					GetOutVector(L, 2) = GetPlane(L).position; // plane, position

					return 1;
				}
			}, {
				"GetProjectionOfPoint", [](lua_State * L)
				{
					const Plane & plane = GetPlane(L);
					const Vec3 & pos = GetConstVec3(L, 2);
					Vec3 npos;

					npos.SetProjectionOfPointOntoRay(pos, plane.position, plane.normal);
					
					GetOutVector(L, 3) = pos - (npos - plane.position);

					return 1;
				}
			}, {
				"__index", [](lua_State * L)
				{
					if (lua_isstring(L, 2))
					{
						const char * str = lua_tostring(L, 2);

						if (strcmp(str, "position") == 0) return AuxNewVec3(L, GetPlane(L).position); // t, k, position
						else if (strcmp(str, "normal") == 0) return AuxNewVec3(L, GetPlane(L).normal); // t, k, normal
						else if (strcmp(str, "distance") == 0)
						{
							lua_pushnumber(L, GetPlane(L).distance); // t, k, distance

							return 1;
						}
					}

					lua_getmetatable(L, 1); // t, k, mt
					lua_replace(L, 1); // mt, k
					lua_rawget(L, 1); // mt, value?

					return 1;
				}
			}, {
				"__newindex", [](lua_State * L)
				{
					if (lua_isstring(L, 2))
					{
						const char * str = lua_tostring(L, 2);
						
						if (strcmp(str, "position") == 0) GetPlane(L).position = GetConstVec3(L, 3);
						else if (strcmp(str, "normal") == 0) GetPlane(L).normal = GetConstVec3(L, 3);
					}

					return 0;
				}
			}, {
				"SetNormal", [](lua_State * L)
				{
					GetPlane(L).SetNormal(GetConstVec3(L, 2));

					return 0;
				}
			}, {
				"SetPosition", [](lua_State * L)
				{
					GetPlane(L).SetPosition(GetConstVec3(L, 2));

					return 0;
				}
			},
			{ nullptr, nullptr }
		};

		luaL_register(L, nullptr, methods);
	}

	lua_setmetatable(L, -2); // ..., plane

	return 1;
}

//
//
//

int NewPlane (lua_State * L)
{
	Plane plane = {};

	if (IsVec3(L, 1)) plane.SetPosition(GetConstVec3(L, 1));
	if (IsVec3(L, 2)) plane.SetNormal(GetConstVec3(L, 2));

	return AuxNewPlane(L, plane);
}