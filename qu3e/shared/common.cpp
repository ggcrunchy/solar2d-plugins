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

#include "qu3e.h"
#include "utils/LuaEx.h"

q3AABB & AABB (lua_State * L, int arg)
{
	return *LuaXS::CheckUD<q3AABB>(L, arg, "qu3e.AABB");
}

q3HalfSpace & HalfSpace (lua_State * L, int arg)
{
	return *LuaXS::CheckUD<q3HalfSpace>(L, arg, "qu3e.HalfSpace");
}

q3RaycastData & Raycast (lua_State * L, int arg)
{
	return *LuaXS::CheckUD<q3RaycastData>(L, arg, "qu3e.RaycastData");
}

static int AuxAABB (lua_State * L)
{
	LuaXS::NewTyped<q3AABB>(L);	// ..., aabb
	LuaXS::AttachMethods(L, "qu3e.AABB", [](lua_State * L) {
		luaL_Reg methods[] = {
			{
				"Contains", [](lua_State * L)
				{
					if (LuaXS::IsType(L, "qu3e.AABB", 2)) return LuaXS::PushArgAndReturn(L, AABB(L).Contains(AABB(L, 2)));	// aabb1, aabb2, contained
					else return LuaXS::PushArgAndReturn(L, AABB(L).Contains(Vector(L, 2)));	// aabb, vec, contained
				}
			}, {
				"__newindex", [](lua_State * L)
				{
					const char * str = luaL_checkstring(L, 2);

					if (strcmp(str, "max") == 0) AABB(L).max = Vector(L, 3);
					else if (strcmp(str, "min") == 0) AABB(L).min = Vector(L, 3);
					else return luaL_error(L, "Invalid member");

					return 0;
				}
			}, {
				"SurfaceArea", [](lua_State * L)
				{
					return LuaXS::PushArgAndReturn(L, AABB(L).SurfaceArea());	// aabb, area
				}
			},
			{ nullptr, nullptr }
		};

		luaL_register(L, nullptr, methods);

		LuaXS::NewWeakKeyedTable(L);// aabb, aabb_mt, vec_to_aabb

		LuaXS::AttachPropertyParams app;

		app.mUpvalueCount = 1U;

		LuaXS::AttachProperties(L, [](lua_State * L) {
			q3Vec3 * pvec = nullptr;

			if (lua_type(L, 2) == LUA_TSTRING)
			{
				const char * str = lua_tostring(L, 2);
                
				if (strcmp(str, "max") == 0) pvec = &AABB(L).max;
				else if (strcmp(str, "min") == 0) pvec = &AABB(L).min;
			}
            
			if (pvec) AddVectorRef(L, pvec);// aabb, k, vec
			else lua_pushnil(L);// aabb, k, nil

			return 1;
		}, app);// aabb, aabb_mt
	});

	return 1;
}

void open_common (lua_State * L)
{
	luaL_Reg funcs[] = {
		{
			"Combine", [](lua_State * L)
			{
				lua_settop(L, 2);	// aabb1, aabb2

				AuxAABB(L);	// aabb1, aabb2, comb

				AABB(L, 3) = q3Combine(AABB(L, 1), AABB(L, 2));

				return 1;
			}
		}, {
			"NewAABB", AuxAABB
		}, {
			"NewHalfSpace", [](lua_State * L)
			{
				q3HalfSpace * hs = LuaXS::NewTyped<q3HalfSpace>(L);	// [normal, distance, ]halfspace

				if (lua_gettop(L) > 1) *hs = q3HalfSpace{ Vector(L, 1), LuaXS::Float(L, 2) };

				LuaXS::AttachMethods(L, "qu3e.HalfSpace", [](lua_State * L) {
					luaL_Reg methods[] = {
						{
							"Distance", [](lua_State * L)
							{
								return LuaXS::PushArgAndReturn(L, HalfSpace(L).Distance(Vector(L, 2)));	// hs, p, distance
							}
						}, {
							"__newindex", [](lua_State * L)
							{
								const char * str = luaL_checkstring(L, 2);

								if (strcmp(str, "normal") == 0) HalfSpace(L).normal = Vector(L, 3);
								else if (strcmp(str, "distance") == 0) HalfSpace(L).distance = LuaXS::Float(L, 3);
								else return luaL_error(L, "Invalid member");

								return 0;
							}
						}, {
							"Origin", [](lua_State * L)
							{
								return NewVector(L, HalfSpace(L).Origin());	// hs, origin
							}
						}, {
							"Projected", [](lua_State * L)
							{
								return NewVector(L, HalfSpace(L).Projected(Vector(L, 2)));	// hs, v, proj
							}
						}, {
							"Set", [](lua_State * L)
							{
								if (lua_gettop(L) >= 4) HalfSpace(L).Set(Vector(L, 2), Vector(L, 3), Vector(L, 4));
								else HalfSpace(L).Set(Vector(L, 2), Vector(L, 3));

								return 0;
							}
						},
						{ nullptr, nullptr }
					};

					luaL_register(L, nullptr, methods);

					LuaXS::AttachProperties(L, [](lua_State * L) {
						if (lua_type(L, 2) == LUA_TSTRING)
						{
							const char * str = lua_tostring(L, 2);
                
							if (strcmp(str, "normal") == 0) return NewVector(L, HalfSpace(L).normal);	// hs, key, normal
							else if (strcmp(str, "distance") == 0) return LuaXS::PushArgAndReturn(L, HalfSpace(L).distance);// hs, key, distance
						}
            
						return LuaXS::PushArgAndReturn(L, LuaXS::Nil{});// hs, k, nil
					});
				});

				return 1;
			}
		}, {
			"NewRaycastData", [](lua_State * L)
			{
				LuaXS::NewTyped<q3RaycastData>(L);	// raycast
				LuaXS::AttachMethods(L, "qu3e.RaycastData", [](lua_State * L) {
					luaL_Reg methods[] = {
						{
							"GetImpactPoint", [](lua_State * L)
							{
								return NewVector(L, Raycast(L).GetImpactPoint());	// raycast, impact
							}
						}, {
							"__newindex", [](lua_State * L)
							{
								const char * str = luaL_checkstring(L, 2);
								q3Vec3 * pvec = nullptr;
								float * pfloat = nullptr;

								if (strcmp(str, "start") == 0) pvec = &Raycast(L).start;
								else if (strcmp(str, "dir") == 0) pvec = &Raycast(L).dir;
								else if (strcmp(str, "normal") == 0) pvec = &Raycast(L).normal;
								else if (strcmp(str, "t") == 0) pfloat = &Raycast(L).t;
								else if (strcmp(str, "toi") == 0) pfloat = &Raycast(L).toi;

								luaL_argcheck(L, pvec || pfloat, 2, "Invalid member");

								if (pvec) *pvec = Vector(L, 3);
								else *pfloat = LuaXS::Float(L, 3);

								return 0;
							}
						}, {
							"Set", [](lua_State * L)
							{
								Raycast(L).Set(Vector(L, 2), Vector(L, 3), LuaXS::Float(L, 4));

								return 0;
							}
						},
						{ nullptr, nullptr }
					};

					luaL_register(L, nullptr, methods);

					LuaXS::AttachProperties(L, [](lua_State * L) {
						if (lua_type(L, 2) == LUA_TSTRING)
						{
							const char * str = lua_tostring(L, 2);
							q3Vec3 * pvec = nullptr;
							float * pfloat = nullptr;

							if (strcmp(str, "start") == 0) pvec = &Raycast(L).start;
							else if (strcmp(str, "dir") == 0) pvec = &Raycast(L).dir;
							else if (strcmp(str, "normal") == 0) pvec = &Raycast(L).normal;
							else if (strcmp(str, "t") == 0) pfloat = &Raycast(L).t;
							else if (strcmp(str, "toi") == 0) pfloat = &Raycast(L).toi;
                
							if (pvec) NewVector(L, *pvec);	// raycast, key, v
							else if (pfloat) return LuaXS::PushArgAndReturn(L, *pfloat);// raycast, key, float
						}
            
						return LuaXS::PushArgAndReturn(L, LuaXS::Nil{});// raycast, k, nil
					});
				});

				return 1;
			}
		},
		{ nullptr, nullptr }
	};

	luaL_register(L, nullptr, funcs);
}
