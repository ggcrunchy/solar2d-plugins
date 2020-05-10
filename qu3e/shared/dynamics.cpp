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
#include <type_traits>

template<> inline void LuaXS::PushArg<q3Quaternion> (lua_State * L, q3Quaternion q)
{
    NewQuaternion(L, q);// ..., q
}

template<> inline void LuaXS::PushArg<q3Transform> (lua_State * L, q3Transform xform)
{
    NewTransform(L, xform); // ..., xform
}

template<> inline void LuaXS::PushArg<q3Vec3> (lua_State * L, q3Vec3 v)
{
    NewVector(L, v);// ..., v
}

q3BodyDef & BodyDef (lua_State * L, int arg)
{
    return *LuaXS::CheckUD<q3BodyDef>(L, arg, "qu3e.BodyDef");
}

q3Body ** BodyBox (lua_State * L, int arg)
{
    return LuaXS::CheckUD<q3Body *>(L, arg, "qu3e.Body");
}

static q3Body & Body (lua_State * L, int arg = 1)
{
    q3Body * body = *BodyBox(L, arg);
    
    luaL_argcheck(L, body, arg, "Body has been removed");
    
    return *body;
}

static q3Body & BodySetter (lua_State * L)
{
    luaL_argcheck(L, lua_objlen(L, 1) == sizeof(q3Body *), 1, "Cannot call this method since body is read-only");
    
    return Body(L, 1);
}

q3BoxDef & BoxDef (lua_State * L, int arg = 1)
{
    return *LuaXS::CheckUD<q3BoxDef>(L, arg, "qu3e.BoxDef");
}

const q3Box ** BoxBox (lua_State * L, int arg)
{
    return LuaXS::CheckUD<const q3Box *>(L, arg, "qu3e.Box");
}

static const q3Box & Box (lua_State * L, int arg = 1)
{
    const q3Box * box = *BoxBox(L, arg);
    
    luaL_argcheck(L, box, arg, "Box has been removed");
    
    return *box;
}

template<typename T, T (q3Body::*func)(void) const> int Getter (lua_State * L)
{
    return LuaXS::PushArgAndReturn(L, (Body(L).*func)());
}

template<typename T, const T (q3Body::*func)(void) const> int GetterK (lua_State * L)
{
    T res = (Body(L).*func)();

    return LuaXS::PushArgAndReturn(L, res);
}

template<const q3Vec3 (q3Body::*func)(const q3Vec3 &) const> int GetterRelV (lua_State * L)
{
    q3Vec3 res = (Body(L).*func)(Vector(L, 2));

    return LuaXS::PushArgAndReturn(L, res);
}

template<typename T, void (q3Body::*func)(T)> int Setter (lua_State * L)
{
    (BodySetter(L).*func)(LuaXS::GetArg<T>(L));
    
    return 0;
}

template<void (q3Body::*func)(const q3Vec3 &)> int SetterV (lua_State * L)
{
    (BodySetter(L).*func)(Vector(L, 2));
    
    return 0;
}

#define GETTER(name, type) #name, Getter<type, &q3Body::name>
#define GETTERK(name, type) #name, GetterK<const type, &q3Body::name>
#define GETTERRELV(name) #name, GetterRelV<&q3Body::name>
#define SETTER(name, type) #name, Setter<type, &q3Body::name>
#define SETTERV(name) #name, SetterV<&q3Body::name>

void PutBoxInBox (lua_State * L, const q3Box * box)
{
    LuaXS::NewTyped<const q3Box *>(L, box); // ..., box
    LuaXS::AttachMethods(L, "qu3e.Box", [](lua_State * L) {
        luaL_Reg methods[] = {
            {
                "ComputeAABB", [](lua_State * L)
                {
					Box(L).ComputeAABB(Transform(L, 2), &AABB(L, 3));

                    return 0;
                }
            }, {
                "ComputeMass", [](lua_State * L)
                {
                    q3MassData md;
                    
                    Box(L).ComputeMass(&md);
                    
                    int top = lua_gettop(L);
                    
                    if (top >= 2) Matrix(L, 2) = md.inertia;
                    if (top >= 3) Vector(L, 3) = md.center;
                    
                    return LuaXS::PushArgAndReturn(L, md.mass); // box[, inertia[, center]], mass
                }
            }, {
                "GetID", [](lua_State * L)
                {
                    lua_pushinteger(L, reinterpret_cast<intptr_t>(Box(L).GetUserdata()));
                    
                    return 1;
                }
            }, {
                "Raycast", [](lua_State * L)
                {
                    return LuaXS::PushArgAndReturn(L, Box(L).Raycast(Transform(L, 2), &Raycast(L, 3)));	// box, xform, raycast, hit
                }
            }, {
                "SetID", [](lua_State * L)
                {
                    Box(L).SetUserdata(reinterpret_cast<void *>((intptr_t)lua_tointeger(L, 2)));
                    
                    return 0;
                }
            }, {
                "TestPoint", [](lua_State * L)
                {
                    return LuaXS::PushArgAndReturn(L, Box(L).TestPoint(Transform(L, 2), Vector(L, 3)));
                }
            },
            /*
             void Render( const q3Transform& tx, bool awake, q3Render* render ) const;
             */
            { nullptr, nullptr }
        };
        
        luaL_register(L, nullptr, methods);
    });
}
                         
void PutBodyInBox (lua_State * L, q3Body * body, bool bConst)
{
    LuaXS::NewSizeTypedExtra<q3Body *>(L, bConst ? 1U : 0U, body);  // ..., box
    LuaXS::AttachMethods(L, "qu3e.Body", [](lua_State * L) {
        luaL_Reg methods[] = {
            {
               "ApplyForceAtWorldPoint", [](lua_State * L)
                {
                    BodySetter(L).ApplyForceAtWorldPoint(Vector(L, 2), Vector(L, 3));
                    
                    return 0;
                }
            }, {
                SETTERV(ApplyLinearForce)
            }, {
                SETTERV(ApplyLinearImpulse)
            }, {
                "ApplyLinearImpulseAtWorldPoint", [](lua_State * L)
                {
                    BodySetter(L).ApplyLinearImpulseAtWorldPoint(Vector(L, 2), Vector(L, 3));
                    
                    return 0;
                }
            }, {
               SETTERV(ApplyTorque)
            }, {
                "CanCollide", [](lua_State * L)
                {
                    return LuaXS::PushArgAndReturn(L, Body(L).CanCollide(&Body(L, 2)));
                }
            }, {
                "Dump", [](lua_State * L)
                {
                    return 0;
                }
            }, {
                "GetAngularDamping", [](lua_State * L)
                {
                    return LuaXS::PushArgAndReturn(L, Body(L).GetAngularDamping(LuaXS::Float(L, 2)));
                }
            }, {
                GETTERK(GetAngularVelocity, q3Vec3)
            }, {
                GETTER(GetGravityScale, float)
            }, {
                GETTER(GetInvMass, float)
            }, {
                GETTER(GetLayers, int)
            }, {
                "GetLinearDamping", [](lua_State * L)
                {
                    return LuaXS::PushArgAndReturn(L, Body(L).GetLinearDamping(LuaXS::Float(L, 2)));
                }
            }, {
                GETTERK(GetLinearVelocity, q3Vec3)
            }, {
                GETTERRELV(GetLocalPoint)
            }, {
                GETTERRELV(GetLocalVector)
            }, {
                GETTER(GetMass, float)
            }, {
                GETTERK(GetQuaternion, q3Quaternion)
            }, {
                GETTERK(GetTransform, q3Transform)
            }, {
                GETTERRELV(GetVelocityAtWorldPoint)
            }, {
                GETTERRELV(GetWorldPoint)
            }, {
                GETTERRELV(GetWorldVector)
            }, {
                GETTER(IsAwake, bool)
            }, {
                SETTER(SetAngularDamping, float)
            }, {
                SETTERV(SetAngularVelocity)
            }, {
                SETTER(SetGravityScale, float)
            }, {
                SETTER(SetLayers, int)
            }, {
                SETTER(SetLinearDamping, float)
            }, {
                SETTERV(SetLinearVelocity)
            }, {
                "SetToAwake", [](lua_State * L)
                {
                    BodySetter(L).SetToAwake();
                    
                    return 0;
                }
            }, {
                "SetToSleep", [](lua_State * L)
                {
                    BodySetter(L).SetToSleep();
                    
                    return 0;
                }
            }, {
                "SetTransform", [](lua_State * L)
                {
                    if (!lua_isnil(L, 3)) BodySetter(L).SetTransform(Vector(L, 2), Vector(L, 3), LuaXS::Float(L, 4));
                    else BodySetter(L).SetTransform(Vector(L, 2));
                    
                    return 0;
                }
            },
            { nullptr, nullptr }
        };
        
        luaL_register(L, nullptr, methods);

        luaL_Reg closures[] = {
            {
                "AddBox", [](lua_State * L)
                {
                    const q3Box * box = BodySetter(L).AddBox(BoxDef(L, 2));
                    
                    lua_pushvalue(L, lua_upvalueindex(1));  // body, def, box_to_body
                    
                    PutBoxInBox(L, box);  // body, def, box_to_body, box
                    
                    lua_pushvalue(L, -1);   // body, def, box_to_body, box, box
                    lua_pushvalue(L, 1);// body, def, box_to_body, box, box, body
                    lua_rawset(L, -4);  // body, def, box_to_body = { ..., [box] = body }, box
                    
                    return 1;
                }
            }, {
                "RemoveAllBoxes", [](lua_State * L)
                {
                    q3Body & body = BodySetter(L);
                    
                    lua_pushvalue(L, lua_upvalueindex(1));  // body, box_to_body
                    
                    for (lua_pushnil(L); lua_next(L, -2); lua_pop(L, 1))
                    {
                        if (!lua_equal(L, 1, -1)) continue;
                        
                        *BoxBox(L, -2) = nullptr;
                        
                        lua_pushvalue(L, -2);   // body, box_to_body, box, body, box
                        lua_pushnil(L); // body, box_to_body, box, body, box, nil
                        lua_rawset(L, -5);  // body, box_to_body = { ..., [box] = nil }, box, body
                    }
                    
                    body.RemoveAllBoxes();
                    
                    return 0;
                }
            }, {
                "RemoveBox", [](lua_State * L)
                {
                    q3Body & body = BodySetter(L);
                    const q3Box ** box = BoxBox(L, 2);

                    if (*box)
                    {
                        body.RemoveBox(*box);
                    
                        *box = nullptr;
                    }
                    
                    lua_pushvalue(L, lua_upvalueindex(1));  // body, box, box_to_body
                    lua_pushvalue(L, 2);  // body, box, box_to_body, box
                    lua_pushnil(L); // body, box, box_to_body, box, nil
                    lua_rawset(L, -3);  // body, box, box_to_body = { ..., [box] = nil }
                    
                    return 0;
                }
            },
            { nullptr, nullptr }
        };
        
        LuaXS::NewWeakKeyedTable(L);// body, body_mt, box_to_body
        LuaXS::AddClosures(L, closures, 1); // body, body_mt = { ..., AddBox, RemoveAllBoxes, RemoveBox }
    });
}

#undef GETTER
#undef GETTERK
#undef GETTERRELV
#undef SETTER
#undef SETTERV

#define MEMBER(name) { &q3BodyDef::name, #name }
#define ADD_MEMBERS(members, n1, n2) for (auto && m : members)                      \
{                                                                                   \
    mptrs[index].n1 = m.n2;                                                         \
    lua_pushinteger(L, index++); /* ..., name_to_index, index */                    \
    lua_setfield(L, -2, m.mName); /* ..., name_to_index = { ..., name = index } */  \
}

void open_dynamics (lua_State * L)
{
	luaL_Reg funcs[] = {
		{
			"NewBodyDef", [](lua_State * L)
			{
				LuaXS::NewTyped<q3BodyDef>(L);	// body_def
				LuaXS::AttachMethods(L, "qu3e.BodyDef", [](lua_State * L) {
					union Member {
                        bool (q3BodyDef::*mB);
                        float (q3BodyDef::*mF);
                        q3Vec3 (q3BodyDef::*mV);
                    };

					struct {
						bool (q3BodyDef::*mBool);
						const char * mName;
					} bools[] = {
						MEMBER(active), MEMBER(allowSleep), MEMBER(awake), MEMBER(lockAxisX), MEMBER(lockAxisY), MEMBER(lockAxisZ)
					};

					struct {
						float (q3BodyDef::*mFloat);
						const char * mName;
					} floats[] = {
						MEMBER(angle), MEMBER(angularDamping), MEMBER(gravityScale), MEMBER(linearDamping)
					};

					struct {
						q3Vec3 (q3BodyDef::*mVec);
						const char * mName;
					} vecs[] = {
						MEMBER(angularVelocity), MEMBER(axis), MEMBER(linearVelocity), MEMBER(position)
					};
                    
					enum Offsets : size_t {
						nbools = std::extent<decltype(bools)>::value,
						nfloats = std::extent<decltype(floats)>::value,
						nvecs = std::extent<decltype(vecs)>::value,
						to_vecs = nbools + nfloats,
						to_type = to_vecs + nvecs
					};

                    Member * mptrs = LuaXS::NewArray<Member>(L, to_type);  // body_def, body_def_mt, mptrs
                    int index = 0;

                    lua_createtable(L, to_type + 3U, 0U);   // body_def, body_def_mt, mptrs, name_to_index
                    
                    ADD_MEMBERS(bools, mB, mBool)
                    ADD_MEMBERS(floats, mF, mFloat)
                    ADD_MEMBERS(vecs, mV, mVec)

                    lua_pushinteger(L, index++);// body_def, body_def_mt, mptrs, name_to_index, index
                    lua_setfield(L, -2, "bodyType");// body_def, body_def_mt, mptrs, name_to_index = { ..., bodyType = index }
                    lua_pushinteger(L, index);  // body_def, body_def_mt, mptrs, name_to_index, index
                    lua_setfield(L, -2, "id");  // body_def, body_def_mt, mptrs, name_to_index = { ..., bodyType, id = index }
                    lua_pushinteger(L, index);  // body_def, body_def_mt, mptrs, name_to_index, index
                    lua_setfield(L, -2, "layers");  // body_def, body_def_mt, mptrs, name_to_index = { ..., bodyType, id, layers = index }
                    lua_setfenv(L, -2); // body_def, body_def_mt, mptrs; mptrs.env = name_to_index
                    lua_pushvalue(L, -1);   // body_def, body_def_mt, mptrs, mptrs
                    lua_pushcclosure(L, [](lua_State * L) {
                        lua_pushvalue(L, lua_upvalueindex(1));  // body_def, name, value, mptrs
                        lua_getfenv(L, -1); // body_def, name, value, mptrs, name_to_index
                        lua_pushvalue(L, 2);// body_def, name, value, mptrs, name_to_index, name
                        lua_rawget(L, -2);  // body_def, name, value, mptrs, name_to_index, index?
                        
                        luaL_argcheck(L, !lua_isnil(L, -1), 2, "Invalid property");

                        size_t index = lua_tointeger(L, -1);
                        
                        if (index >= Offsets::to_type)
                        {
                            if (index == Offsets::to_type)
                            {
                                const char * names[] = { "dynamic", "kinematic", "static", nullptr };
                                q3BodyType types[] = { eDynamicBody, eKinematicBody, eStaticBody };

                                BodyDef(L).bodyType = types[luaL_checkoption(L, 3, nullptr, names)];
                            }
                            
                            else if (index == Offsets::to_type + 1U) BodyDef(L).userData = reinterpret_cast<void *>((intptr_t)luaL_checkint(L, 3));
                            else BodyDef(L).layers = luaL_checkint(L, 3);
                        }
                        
                        else
                        {
                            Member & mptr = LuaXS::UD<Member>(L, -3)[LuaXS::Int(L, -1)];
                            
                            if (index >= Offsets::to_vecs) BodyDef(L).*mptr.mV = Vector(L, 3);
                            else if (index >= Offsets::nbools) BodyDef(L).*mptr.mF = LuaXS::Float(L, 3);
                            else BodyDef(L).*mptr.mB = LuaXS::Bool(L, 3);
                        }

                        return 0;
                    }, 1);  // body_def, body_def_mt, mptrs, NewIndex
                    lua_setfield(L, -3, "__newindex");  // body_def, body_def_mt = { ..., __newindex = NewIndex }, mptrs

                    LuaXS::AttachPropertyParams app;
                    
                    app.mUpvalueCount = 1U;
                    
                    LuaXS::AttachProperties(L, [](lua_State * L) {
                        lua_pushvalue(L, lua_upvalueindex(1));  // body, name, mptrs
                        lua_getfenv(L, -1); // body, name, mptrs, name_to_index
                        lua_pushvalue(L, 2);// body, name, mptrs, name_to_index, name
                        lua_rawget(L, -2);  // body, name, mptrs, name_to_index, index?

                        if (!lua_isnil(L, -1))
                        {
                            size_t index = lua_tointeger(L, -1);
                            
                            if (index >= Offsets::to_type)
                            {
                                if (index == Offsets::to_type)
                                {
                                    q3BodyType type = BodyDef(L).bodyType;
                                    
                                    if (type == eStaticBody) lua_pushliteral(L, "static");  // body_def, name, mptrs, name_to_index, index, "static"
                                    else if (type == eKinematicBody) lua_pushliteral(L, "kinematic");   // body_def, name, mptrs, name_to_index, index, "kinematic"
                                    else lua_pushliteral(L, "dynamic"); // body_def, name,  mptrs, name_to_index, index, "dynamic"
                                }
                                
                                else if (index == Offsets::to_type + 1U) lua_pushinteger(L, reinterpret_cast<intptr_t>(BodyDef(L).userData));// body_def, name, mptrs, name_to_index, index, id
                                else lua_pushinteger(L, BodyDef(L).layers);	// body_def, name, mptrs, name_to_index, index, layers
                            }
                            
                            else
                            {
                                Member & mptr = LuaXS::UD<Member>(L, -3)[LuaXS::Int(L, -1)];
								
                                if (index >= Offsets::to_vecs) NewVector(L, BodyDef(L).*mptr.mV);	// body_def, name, value, mptrs, name_to_index, index, vector
                                else if (index >= Offsets::nbools) lua_pushnumber(L, BodyDef(L).*mptr.mF);	// body_def, name, value, mptrs, name_to_index, index, float
                                else LuaXS::PushArg(L, BodyDef(L).*mptr.mB);// body_def, name, value, mptrs, name_to_index, index, bool
                            }
                        }
                        
                        return 1;
                    }, app);// body_def, body_def_mt = { ..., __newindex, __index = IndexWithProps }
				});

				return 1;
			}
        }, {
            "NewBoxDef", [](lua_State * L)
            {
                LuaXS::NewTyped<q3BoxDef>(L);   // box_def
                LuaXS::AttachMethods(L, "qu3e.BoxDef", [](lua_State * L) {
                    luaL_Reg methods[] = {
                        {
                            "Set", [](lua_State * L)
                            {
                                BoxDef(L).Set(Transform(L, 2), Vector(L, 3));
                                
                                return 0;
                            }
                        }, {
                            "SetDensity", [](lua_State * L)
                            {
                                BoxDef(L).SetDensity(LuaXS::Float(L, 2));
                                
                                return 0;
                            }
                        }, {
                            "SetFriction", [](lua_State * L)
                            {
                                BoxDef(L).SetFriction(LuaXS::Float(L, 2));
                                
                                return 0;
                            }
                        }, {
                            "SetRestitution", [](lua_State * L)
                            {
                                BoxDef(L).SetRestitution(LuaXS::Float(L, 2));
                                
                                return 0;
                            }
                        }, {
                            "SetSensor", [](lua_State * L)
                            {
                                BoxDef(L).SetSensor(LuaXS::Bool(L, 2));
                                
                                return 0;
                            }
                        },
                        { nullptr, nullptr }
                    };
                    
                    luaL_register(L, nullptr, methods);
                });
                
                return 1;
            }
        },
		{ nullptr, nullptr }
	};

    luaL_register(L, nullptr, funcs);
}

#undef MEMBER
#undef ADD_MEMBERS
