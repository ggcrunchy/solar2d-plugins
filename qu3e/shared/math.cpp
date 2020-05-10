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
#include "ByteReader.h"

q3Mat3 & Matrix (lua_State * L, int arg)
{
    if (LuaXS::IsType(L, "qu3e.MatrixRef", arg)) return **LuaXS::UD<q3Mat3 *>(L, arg);
    
    return *LuaXS::CheckUD<q3Mat3>(L, arg, "qu3e.Matrix");
}

q3Quaternion & Quaternion (lua_State * L, int arg)
{
    return *LuaXS::CheckUD<q3Quaternion>(L, arg, "qu3e.Quaternion");
}

q3Transform & Transform (lua_State * L, int arg)
{
    return *LuaXS::CheckUD<q3Transform>(L, arg, "qu3e.Transform");
}

q3Vec3 & Vector (lua_State * L, int arg)
{
    luaL_argcheck(L, LuaXS::IsType(L, "qu3e.Vector", arg), arg, "Expected vector");

    static_assert(sizeof(q3Vec3) != sizeof(q3Vec3 *), "Sizes match, unable to disambiguate");

    if (lua_objlen(L, arg) == sizeof(q3Vec3)) return *LuaXS::UD<q3Vec3>(L, arg);
    else return **LuaXS::UD<q3Vec3 *>(L, arg);
}

template<typename T> void AddRef (lua_State * L, T * ref, lua_CFunction attach)
{
    LuaXS::NewTyped<T *>(L, ref);   // from, ..., ref

    attach(L);
    
    lua_pushvalue(L, lua_upvalueindex(1));  // from, ..., ref, refs_to_source
    lua_pushvalue(L, -2);   // from, ..., ref, refs_to_source, ref
    lua_pushvalue(L, 1);// from, ..., ref, refs_to_source, ref, from
    lua_rawset(L, -3);  // from, ..., ref, refs_to_source = { ..., [ref] = from }
    lua_pop(L, 1);  // from, ..., ref}
}

static q3Vec3 * GetVectorMember (lua_State * L)
{
    int type = lua_type(L, 2);

    if (type == LUA_TNUMBER)
    {
        int index = LuaXS::Int(L, 2) - 1;
        
        luaL_argcheck(L, index >= 0 && index <= 2, 2, "Invalid index");
        
        return &Matrix(L)[index];
    }
    
    else if (type == LUA_TSTRING)
    {
        const char * str = lua_tostring(L, 2);
        
        if (strcmp(str, "ex") == 0) return &Matrix(L).ex;
        else if (strcmp(str, "ey") == 0) return &Matrix(L).ey;
        else if (strcmp(str, "ez") == 0) return &Matrix(L).ez;
    }

    return nullptr;
}

static int AuxNewVector (lua_State * L);

void AddVectorRef (lua_State * L, q3Vec3 * vec)
{
	AddRef(L, vec, AuxNewVector);// source, ..., vec
}

static int AuxNewMatrix (lua_State * L)
{          
    LuaXS::AttachMethods(L, "qu3e.Matrix", [](lua_State * L) {
        luaL_Reg methods[] = {
            {
                "__add", [](lua_State * L)
                {
                    return NewMatrix(L, Matrix(L, 1) + Matrix(L, 2));   // m1, m2, mout
                }
            }, {
                "AddMutate", [](lua_State * L)
                {
                    Matrix(L) += (Matrix(L, 2));
                    
                    lua_settop(L, 1);   // m1
                    
                    return 1;
                }
            }, {
                "Assign", [](lua_State * L)
                {
                    Matrix(L) = Matrix(L, 2);
                    
                    lua_settop(L, 1);   // m1
                    
                    return 1;
                }
            }, {
                "Column0", [](lua_State * L)
                {
                    return NewVector(L, Matrix(L).Column0());
                }
            }, {
                "Column1", [](lua_State * L)
                {
                    return NewVector(L, Matrix(L).Column1());
                }
            }, {
                "Column2", [](lua_State * L)
                {
                    return NewVector(L, Matrix(L).Column2());
                }
            }, {
                "__mul", [](lua_State * L)
                {
                    if (lua_type(L, 1) == LUA_TNUMBER) NewMatrix(L, Matrix(L, 2) * LuaXS::Float(L, 1)); // scale, m, mout
                    else if (lua_type(L, 2) == LUA_TNUMBER) NewMatrix(L, Matrix(L) * LuaXS::Float(L, 2));   // m, scale, mout
                    else
                    {
                        q3Mat3 & lhs = Matrix(L);
                        
                        if (LuaXS::IsType(L, "qu3e.Matrix", 2)) NewMatrix(L, lhs * Matrix(L, 2));// m1, m2, mout
                        else NewVector(L, lhs * Vector(L, 2));  // m, v, vout
                    }

                    return 1;
                }
            }, {
                "MulMutate", [](lua_State * L)
                {
                    if (lua_type(L, 2) == LUA_TNUMBER) Matrix(L) *= LuaXS::Float(L, 2);
                    else Matrix(L) *= Matrix(L, 2);
                    
                    lua_settop(L, 1);   // m1
                    
                    return 1;
                }
            }, {
                "__newindex", [](lua_State * L)
                {
                    q3Vec3 * pvec = GetVectorMember(L);
                    
                    luaL_argcheck(L, pvec, 2, "Invalid member");
                    
                    *pvec = Vector(L, 3);
                    
                    return 0;
                }
            }, {
                "Set", [](lua_State * L)
                {
                    if (lua_type(L, 1) == LUA_TNUMBER) Matrix(L).Set(LuaXS::Float(L, 2), LuaXS::Float(L, 3), LuaXS::Float(L, 4),
                                                                     LuaXS::Float(L, 5), LuaXS::Float(L, 6), LuaXS::Float(L, 7),
                                                                     LuaXS::Float(L, 8), LuaXS::Float(L, 9), LuaXS::Float(L, 10));
                    else Matrix(L).Set(Vector(L, 2), LuaXS::Float(L, 3));
                    
                    return 0;
                }
            }, {
                "SetRows", [](lua_State * L)
                {
                    Matrix(L).SetRows(Vector(L, 2), Vector(L, 3), Vector(L, 4));
                    
                    return 0;
                }
            }, {
                "__sub", [](lua_State * L)
                {
                    return NewMatrix(L, Matrix(L, 1) - Matrix(L, 2));   // m1, m2, mout
                }
            }, {
                "SubMutate", [](lua_State * L)
                {
                    Matrix(L) -= (Matrix(L, 2));
                    
                    lua_settop(L, 1);   // m1
                    
                    return 1;
                }
            },
            { nullptr, nullptr }
        };
                    
        luaL_register(L, nullptr, methods);

        LuaXS::NewWeakKeyedTable(L);// matrix, matrix_mt, vrefs_to_matrix

        LuaXS::AttachPropertyParams app;
        
        app.mUpvalueCount = 1U;

        LuaXS::AttachProperties(L, [](lua_State * L) {
            q3Vec3 * pvec = GetVectorMember(L);
            
            if (pvec) AddVectorRef(L, pvec);// mat, index, vref
            else lua_pushnil(L);// mat, k, nil
            
            return 1;
        }, app);
    });
                
    return 1;
}

int NewMatrix (lua_State * L, const q3Mat3 & mat)
{
	LuaXS::NewTyped<q3Mat3>(L, mat);// ..., mat

	return AuxNewMatrix(L);
}

static int AuxNewQuaternion (lua_State * L)
{
    LuaXS::AttachMethods(L, "qu3e.Quaternion", [](lua_State * L) {
        luaL_Reg methods[] = {
            {
                "Integrate", [](lua_State * L)
                {
                    Quaternion(L).Integrate(Vector(L, 2), LuaXS::Float(L, 3));
                    
                    return 0;
                }
            }, {
                "__mul", [](lua_State * L)
                {
                    return NewQuaternion(L, Quaternion(L, 1) * Quaternion(L, 2));	// quat1, quat2, qout
                }
            }, {
                "MulMutate", [](lua_State * L)
                {
                    Quaternion(L) *= Quaternion(L, 2);
                    
                    lua_settop(L, 1);   // q1
                    
                    return 1;
                }
            }, {
                "__newindex", [](lua_State * L)
                {
                    if (lua_type(L, 2) == LUA_TNUMBER)
                    {
                        int index = LuaXS::Int(L, 2) - 1;
                        
                        luaL_argcheck(L, index >= 0 && index <= 3, 2, "Invalid index");
                        
                        Quaternion(L).v[index] = LuaXS::Float(L, 3);
                    }
                    
                    else
                    {
                        const char * str = lua_tostring(L, 2);
                        
                        if (strcmp(str, "x") == 0) Quaternion(L).x = LuaXS::Float(L, 3);
                        else if (strcmp(str, "y") == 0) Quaternion(L).y = LuaXS::Float(L, 3);
                        else if (strcmp(str, "z") == 0) Quaternion(L).z = LuaXS::Float(L, 3);
                        else if (strcmp(str, "w") == 0) Quaternion(L).w = LuaXS::Float(L, 3);
                        else return luaL_error(L, "Invalid member");
                    }
                    
                    return 0;
                }
            }, {
                "Set", [](lua_State * L)
                {
                    Quaternion(L).Set(Vector(L, 2), LuaXS::Float(L, 3));
                    
                    return 0;
                }
            }, {
                "ToAxisAngle", [](lua_State * L)
                {
                    q3Vec3 axis;
                    float angle;
                    
                    Quaternion(L).ToAxisAngle(&axis, &angle);
                    
                    NewVector(L, axis); // quat, axis
                    
                    return 1 + LuaXS::PushArgAndReturn(L, angle);   // quat, axis, angle
                }
            }, {
                "ToMat", [](lua_State * L)
                {
                    return NewMatrix(L, Quaternion(L).ToMat3());
                }
            },
            { nullptr, nullptr }
        };
        
        luaL_register(L, nullptr, methods);

        LuaXS::AttachProperties(L, [](lua_State * L) {
            int type = lua_type(L, 2);
            float * pfloat = nullptr;
            
            if (type == LUA_TNUMBER)
            {
                int index = LuaXS::Int(L, 2) - 1;
                
                luaL_argcheck(L, index >= 0 && index <= 3, 2, "Invalid index");
                
                pfloat = &Quaternion(L).v[index];
            }
            
            else if (type == LUA_TSTRING)
            {
                const char * str = lua_tostring(L, 2);
                
                if (strcmp(str, "x") == 0) pfloat = &Quaternion(L).x;
                else if (strcmp(str, "y") == 0) pfloat = &Quaternion(L).y;
                else if (strcmp(str, "z") == 0) pfloat = &Quaternion(L).z;
                else if (strcmp(str, "w") == 0) pfloat = &Quaternion(L).w;
            }
            
            if (pfloat) lua_pushnumber(L, *pfloat); // quat, key, float
            else lua_pushnil(L);// quat, key, nil
            
            return 1;
        });

        ByteReaderFunc * func = ByteReader::Register(L);

        func->mGetBytes = [](lua_State * L, ByteReader & reader, int arg, void *)
        {
            q3Quaternion & quat = Quaternion(L, arg);
            
            reader.mBytes = quat.v;
            reader.mCount = 4U * sizeof(float);
            
            return true;
        };
        
        lua_pushlightuserdata(L, func); // quat, quat_mt, func
        lua_setfield(L, -2, "__bytes"); // quat, quat_mt = { ..., __bytes = func }
    });

	return 1;
}

int NewQuaternion (lua_State * L, const q3Quaternion & quat)
{
	LuaXS::NewTyped<q3Quaternion>(L, quat);	// ..., quat

	return AuxNewQuaternion(L);
}

static int AuxNewTransform (lua_State * L)
{
    LuaXS::AttachMethods(L, "qu3e.Transform", [](lua_State * L) {
        luaL_Reg methods[] = {
            {
                "__setindex", [](lua_State * L)
                {
                    const char * str = luaL_checkstring(L, 2);
                    
                    if (strcmp(str, "position") == 0) Transform(L).position = Vector(L, 3);
                    else if (strcmp(str, "rotation") == 0) Transform(L).rotation = Matrix(L, 3);
                    else luaL_error(L, "Invalid transform property");
                    
                    return 0;
                }
            },
            { nullptr, nullptr }
        };

        luaL_register(L, nullptr, methods);
        
        LuaXS::NewWeakKeyedTable(L);// xform, xform_mt, refs_to_xform
        
        LuaXS::AttachPropertyParams app;
        
        app.mUpvalueCount = 1U;
        
        LuaXS::AttachProperties(L, [](lua_State * L) {
            const char * str = luaL_checkstring(L, 2);

            if (strcmp(str, "position") == 0) AddVectorRef(L, &Transform(L).position);  // xform, "position", pos
            else if (strcmp(str, "rotation") == 0) AddRef(L, &Transform(L).rotation, AuxNewMatrix); // xform, "rotation", rot
            else lua_pushnil(L);// xform, k, nil
            
            return 1;
        }, app);
    });

    return 1;
}

int NewTransform (lua_State * L, const q3Transform & xform)
{
    LuaXS::NewTyped<q3Transform>(L, xform); // ..., xform
    
    return AuxNewTransform(L);
}

static int AuxNewVector (lua_State * L)
{
	LuaXS::AttachMethods(L, "qu3e.Vector", [](lua_State * L) {
		luaL_Reg methods[] = {
			{
				"__add", [](lua_State * L)
				{
					return NewVector(L, Vector(L, 1) + Vector(L, 2));	// vec1, vec2, vout
				}
			}, {
				"AddMutate", [](lua_State * L)
				{
					Vector(L) += Vector(L, 2);

					lua_settop(L, 1);	// vec1

					return 1;
				}
			}, {
				"__div", [](lua_State * L)
				{
					return NewVector(L, Vector(L) / LuaXS::Float(L, 2));// vec, scale, vout
				}
			}, {
				"DivMutate", [](lua_State * L)
				{
					Vector(L) /= LuaXS::Float(L, 2);

					lua_settop(L, 1);	// vec1

					return 1;
				}
			}, {
				"__mul", [](lua_State * L)
				{
					if (lua_type(L, 1) == LUA_TNUMBER) return NewVector(L, Vector(L, 2) * LuaXS::Float(L, 1));	// scale, vec, vout
					else return NewVector(L, Vector(L) * LuaXS::Float(L, 2));	// vec, scale, vout
				}
			}, {
				"MulMutate", [](lua_State * L)
				{
					Vector(L) *= LuaXS::Float(L, 2);

					lua_settop(L, 1);	// vec1

					return 1;
				}
			}, {
                "__newindex", [](lua_State * L)
                {
                    if (lua_type(L, 2) == LUA_TNUMBER)
                    {
                        int index = LuaXS::Int(L, 2) - 1;
                        
                        luaL_argcheck(L, index >= 0 && index <= 2, 2, "Invalid index");
                        
                        Vector(L).v[index] = LuaXS::Float(L, 3);
                    }
                    
                    else
                    {
                        const char * str = luaL_checkstring(L, 2);
                        
                        if (strcmp(str, "x") == 0) Vector(L).x = LuaXS::Float(L, 3);
                        else if (strcmp(str, "y") == 0) Vector(L).y = LuaXS::Float(L, 3);
                        else if (strcmp(str, "z") == 0) Vector(L).z = LuaXS::Float(L, 3);
                        else return luaL_error(L, "Invalid member");
                    }
                    
                    return 0;
                }
            }, {
				"Set", [](lua_State * L)
				{
					Vector(L).Set(LuaXS::Float(L, 2), LuaXS::Float(L, 3), LuaXS::Float(L, 4));

					return 0;
				}
			}, {
				"SetAll", [](lua_State * L)
				{
					Vector(L).SetAll(LuaXS::Float(L, 2));

					return 0;
				}
			}, {
				"__sub", [](lua_State * L)
				{
					return NewVector(L, Vector(L, 1) - Vector(L, 2));	// vec1, vec2, vout
				}
			}, {
				"SubMutate", [](lua_State * L)
				{
					Vector(L) -= Vector(L, 2);

					lua_settop(L, 1);	// vec1

					return 1;
				}
			}, {
				"__unm", [](lua_State * L)
				{
					return NewVector(L, -Vector(L));// v, vout
				}
			},
			{ nullptr, nullptr }
		};

		luaL_register(L, nullptr, methods);

        LuaXS::AttachProperties(L, [](lua_State * L) {
            int type = lua_type(L, 2);
            float * pfloat = nullptr;
            
            if (type == LUA_TNUMBER)
            {
                int index = LuaXS::Int(L, 2) - 1;
                
                luaL_argcheck(L, index >= 0 && index <= 2, 2, "Invalid index");
                
                pfloat = &Vector(L).v[index];
            }
            
            else if (type == LUA_TSTRING)
            {
                const char * str = lua_tostring(L, 2);
                
                if (strcmp(str, "x") == 0) pfloat = &Vector(L).x;
                else if (strcmp(str, "y") == 0) pfloat = &Vector(L).y;
                else if (strcmp(str, "z") == 0) pfloat = &Vector(L).z;
            }
            
            if (pfloat) lua_pushnumber(L, *pfloat); // vec, key, float
            else lua_pushnil(L);// vec, key, nil
            
            return 1;
        });

        ByteReaderFunc * func = ByteReader::Register(L);

        func->mGetBytes = [](lua_State * L, ByteReader & reader, int arg, void *)
        {
            q3Vec3 & vec = Vector(L, arg);
            
            reader.mBytes = vec.v;
            reader.mCount = 3U * sizeof(float);
            
            return true;
        };
        
        lua_pushlightuserdata(L, func); // vec, vec_mt, func
        lua_setfield(L, -2, "__bytes"); // vec, vec_mt = { ..., __bytes = func }
	});

	return 1;
}

int NewVector (lua_State * L, const q3Vec3 & vec)
{
	LuaXS::NewTyped<q3Vec3>(L, vec);// ..., vec

	return AuxNewVector(L);
}

void open_math (lua_State * L)
{
    luaL_Reg math_funcs[] = {
        {
            "NewMatrix", [](lua_State * L)
            {
                q3Mat3 * mat = LuaXS::NewTyped<q3Mat3>(L); // ..., mat

                if (lua_type(L, 1) == LUA_TNUMBER) *mat = q3Mat3{
                                                        LuaXS::Float(L, 1), LuaXS::Float(L, 2), LuaXS::Float(L, 3),
                                                        LuaXS::Float(L, 4), LuaXS::Float(L, 5), LuaXS::Float(L, 6),
                                                        LuaXS::Float(L, 7), LuaXS::Float(L, 8), LuaXS::Float(L, 9)
                                                    };
                else if (lua_gettop(L) > 1) *mat = q3Mat3{ Vector(L, 1), Vector(L, 2), Vector(L, 3) };

				return AuxNewMatrix(L);
            }
        }, {
            "NewQuaternion", [](lua_State * L)
            {
                q3Quaternion * quat = LuaXS::NewTyped<q3Quaternion>(L); // ..., quat

                if (lua_type(L, 1) == LUA_TNUMBER) *quat = q3Quaternion{ LuaXS::Float(L, 1), LuaXS::Float(L, 2), LuaXS::Float(L, 3), LuaXS::Float(L, 4) };
                else if (lua_gettop(L) > 1) *quat = q3Quaternion{ Vector(L, 1), LuaXS::Float(L, 2) };

                return AuxNewQuaternion(L);
            }
        }, {
            "NewTransform", [](lua_State * L)
            {
                int top = lua_gettop(L);
                
                q3Transform * xform = LuaXS::NewTyped<q3Transform>(L);  // ..., xform
                
                if (top >= 1) xform->position = Vector(L, 1);
                if (top >= 2) xform->rotation = Matrix(L, 2);
                
                return AuxNewTransform(L);
            }
        }, {
            "NewVector", [](lua_State * L)
            {
                q3Vec3 * vec = LuaXS::NewTyped<q3Vec3>(L);  // ..., vec

                if (lua_gettop(L) > 1) *vec = q3Vec3{ LuaXS::Float(L, 1), LuaXS::Float(L, 2), LuaXS::Float(L, 3) };
                
                return AuxNewVector(L);
            }
        },
        { nullptr, nullptr }
    };

    luaL_register(L, nullptr, math_funcs);
}
