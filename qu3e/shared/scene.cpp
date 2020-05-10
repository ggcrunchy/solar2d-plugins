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
#include "qu3e/dynamics/q3Contact.h"
#include "utils/LuaEx.h"
#include "utils/Path.h"
#include <type_traits>

static const q3ContactConstraint * Contact (lua_State * L)
{
    const q3ContactConstraint ** contact = LuaXS::CheckUD<const q3ContactConstraint *>(L, 1, "qu3e.ContactConstraint");

    luaL_argcheck(L, *contact, 1, "Missing contact constraint");

    return *contact;
}

static q3Scene * Scene (lua_State * L)
{
	return LuaXS::CheckUD<q3Scene>(L, 1, "qu3e.Scene");
}

struct ContactListener : q3ContactListener {
    lua_State * mL;
    int mEventRef, mStateRef;
    const q3ContactConstraint ** mContact;
    const q3Box ** mBox1, ** mBox2;
    q3Body ** mBody1, ** mBody2;
    bool mHasBegin{false}, mHasEnd{false};

    void Call (const q3ContactConstraint * contact)
    {
        lua_getref(mL, mEventRef);  // ..., name, event
        lua_insert(mL, -2); // ..., event, name
        lua_getref(mL, mStateRef);  // ..., event, name, state
        lua_pushvalue(mL, -2);  // ..., event, name, state, name
        lua_rawget(mL, -2); // ..., event, name, state, func
        lua_insert(mL, -4); // ..., func, event, name, state

        const char * fields[] = { "body1", "body2", "box1", "box2", "contact" };
        
        for (const char * name : fields)
        {
            lua_getfield(mL, -1, name); // ..., func, event, name, state, field
            lua_setfield(mL, -4, name); // ..., func, event = { ..., fname = field }, name, state
        }
        
        lua_pop(mL, 1); // ..., func, event, name
        lua_setfield(mL, -2, "name");   // ..., func, event = { ..., name = name }

        *mContact = contact;
        
        if (lua_pcall(mL, 1, 0, 0) != 0) lua_pop(mL, 1);// ...

        *mContact = nullptr;
        *mBody1 = *mBody2 = nullptr;
        *mBox1 = *mBox2 = nullptr;
    }
    
    void BeginContact (const q3ContactConstraint * contact) override
    {
        if (mHasBegin)
        {
            lua_pushliteral(mL, "begin_contact_qu3e");  // ..., "begin_contact_qu3e"
            
            Call(contact);
        }
    }

    virtual void EndContact (const q3ContactConstraint * contact) override
    {
        if (mHasEnd)
        {
            lua_pushliteral(mL, "end_contact_qu3e");// ..., "end_contact_qu3e"
            
            Call(contact);
        }
    }
};

struct QueryCallback : q3QueryCallback {
    lua_State * mL;

    QueryCallback (lua_State * L) : mL{L}
    {
    }

    bool ReportShape (q3Box * box) override
    {
        lua_getfield(mL, -1, "func");   // ..., state, func
        lua_getfield(mL, -2, "box");// ..., state, func, box
        
        const q3Box ** into = BoxBox(mL, -1);
        
        *into = box;
        
        bool bContinue = lua_pcall(mL, 1, 1, 0) == 0 && LuaXS::Bool(mL, -1); // ..., state, res / err
        
        *into = nullptr;
        
        lua_pop(mL, 1); // ...
        
        return bContinue;
    }
};

template<typename T, void (q3Scene::*mfunc)(q3QueryCallback *, T &) const, typename std::remove_const<T>::type & (*func)(lua_State *, int)> int Callback (lua_State * L)
{
    lua_settop(L, 3);   // scene, obj, func
    luaL_argcheck(L, lua_isfunction(L, 3), 3, "Expected function");
    lua_pushvalue(L, lua_upvalueindex(1));  // scene, obj, func, state
    lua_insert(L, 3);   // scene, obj, state, func
    lua_setfield(L, 3, "func"); // scene, obj, state = { box, func = func }
    
    QueryCallback qc{L};
    
    (Scene(L)->*mfunc)(&qc, func(L, 2));
    
    lua_pushnil(L); // scene, obj, state, nil
    lua_setfield(L, 3, "func"); // scene, obj, state = { box, func = nil }
    
    return 0;
}

void open_scene (lua_State * L)
{
	luaL_Reg scene_funcs[] = {
		{
			"NewScene", [](lua_State * L)
			{
				luaL_checktype(L, 1, LUA_TTABLE);
				lua_getfield(L, 1, "dt");	// params, dt
				lua_getfield(L, 1, "gravity");	// params, dt, gravity?
				lua_getfield(L, 1, "iterations");	// params, dt, gravity?, iterations?

				float dt = LuaXS::Float(L, -3);
				q3Scene * scene = !lua_isnil(L, -2) ? LuaXS::NewTyped<q3Scene>(L, dt, Vector(L, -2)) : LuaXS::NewTyped<q3Scene>(L, dt);	// params, dt, gravity?, iterations?, scene

				if (!lua_isnil(L, -1)) scene->SetIterations(LuaXS::Int(L, -1));

				LuaXS::AttachMethods(L, "qu3e.Scene", [](lua_State * L) {
					luaL_Reg methods[] = {
						{
							"__gc", LuaXS::TypedGC<q3Scene>
						}, {
							"GetGravity", [](lua_State * L)
							{
								return NewVector(L, Scene(L)->GetGravity());
							}
						}, {
							"SetAllowSleep", [](lua_State * L)
							{
								Scene(L)->SetAllowSleep(LuaXS::Bool(L, 2));

								return 0;
							}
						}, {
							"SetEnableFriction", [](lua_State * L)
							{
								Scene(L)->SetEnableFriction(LuaXS::Bool(L, 2));

								return 0;
							}
						}, {
							"SetGravity", [](lua_State * L)
							{
								Scene(L)->SetGravity(Vector(L, 2));

								return 0;
							}
						}, {
							"SetIterations", [](lua_State * L)
							{
								Scene(L)->SetIterations(LuaXS::Int(L, 2));

								return 0;
							}
						}, {
							"Step", [](lua_State * L)
							{
								Scene(L)->Step();

								return 0;
							}
						},
						{ nullptr, nullptr }
					};

					luaL_register(L, nullptr, methods);

                    luaL_Reg closures[] = {
                        {
                            "CreateBody", [](lua_State * L)
                            {
                                q3Body * body = Scene(L)->CreateBody(BodyDef(L, 2));
                                
                                lua_pushvalue(L, lua_upvalueindex(1));  // scene, def, body_to_scene
                                
                                PutBodyInBox(L, body);  // scene, def, body_to_scene, body

                                lua_pushvalue(L, -1);   // scene, def, body_to_scene, body, body
                                lua_pushvalue(L, 1);// scene, def, body_to_scene, body, body, scene
                                lua_rawset(L, -4);  // scene, def, body_to_scene = { ..., [body] = scene }, body
                                
                                return 1;
                            }
                        }, {
                            "RemoveAllBodies", [](lua_State * L)
                            {
                                lua_pushvalue(L, lua_upvalueindex(1));  // scene, body_to_scene
                                
                                for (lua_pushnil(L); lua_next(L, -2); lua_pop(L, 1))
                                {
                                    if (!lua_equal(L, 1, -1)) continue;
                                    
                                    q3Body ** box = BodyBox(L, -2);
                                    
                                    if (*box)
                                    {
                                        lua_getfield(L, -2, "RemoveAllBoxes");  // scene, body_to_scene, body, scene, RemoveAllBoxes
                                        lua_pushvalue(L, -3);   // scene, body_to_scene, body, scene, RemoveAllBoxes, body
                                        lua_call(L, 1, 0);  // scene, body_to_scene, body, scene
                                        
                                        *box = nullptr;
                                    }
                                    
                                    lua_pushvalue(L, -2);   // scene, body_to_scene, body, scene, body
                                    lua_pushnil(L); // scene, body_to_scene, body, scene, body, nil
                                    lua_rawset(L, -5);  // scene, body_to_scene = { ..., [body] = nil }, body, scene
                                }
                                
                                Scene(L)->RemoveAllBodies();
                                
                                return 0;
                            }
                        }, {
                            "RemoveBody", [](lua_State * L)
                            {
                                q3Body ** box = BodyBox(L, 2);
                                
                                if (*box)
                                {
                                    lua_getfield(L, 2, "RemoveAllBoxes");   // scene, body, RemoveAllBoxes
                                    lua_pushvalue(L, 2);// scene, body, RemoveAllBoxes, body
                                    lua_call(L, 1, 0);  // scene, body
                                    
                                    Scene(L)->RemoveBody(*box);
                                
                                    *box = nullptr;
                                }
                                
                                lua_pushvalue(L, lua_upvalueindex(1));  // scene, body, body_to_scene
                                lua_pushvalue(L, 2);  // scene, body, body_to_scene, body
                                lua_pushnil(L); // scene, body, body_to_scene, body, nil
                                lua_rawset(L, -3);  // scene, body, body_to_scene = { ..., [body] = nil }
                                
                                return 0;
                            }
                        },
                        { nullptr, nullptr }
                    };
                    
                    LuaXS::NewWeakKeyedTable(L);// scene, scene_mt, body_to_scene
                    LuaXS::AddClosures(L, closures, 1); // scene, scene_mt = { ..., CreateBody, RemoveAllBodies, RemoveBody }
					PathXS::Directories::Instantiate(L);// scene, scene_mt, dirs
                    
					lua_pushcclosure(L, [](lua_State * L) {
						lua_pushvalue(L, lua_upvalueindex(1));	// scene, name[, dir][, mode], dirs

						PathXS::Directories * dirs = LuaXS::UD<PathXS::Directories>(L, -1);

						lua_pop(L, 1);	// scene, name[, dir][, mode]

						const char * path = dirs->Canonicalize(L, false, 2);// scene, name[, mode]
						bool bAppend = false;

						if (lua_gettop(L) > 2)
						{
							lua_pushliteral(L, "append");	// scene, name, mode, "append"

							bAppend = lua_equal(L, 3, -1) != 0;
						}

						FILE * fp = fopen(path, bAppend ? "ab" : "wb");

						if (fp)
						{
							Scene(L)->Dump(fp);

							fclose(fp);
						}

						return LuaXS::PushArgAndReturn(L, fp != nullptr);
					}, 1);	// scene, scene_mt, Dump
					lua_setfield(L, -3, "Dump");// scene, scene_mt = { ..., Dump = Dump }

                    luaL_Reg callback_closures[] = {
                        {
                            "QueryAABB", Callback<const q3AABB, &q3Scene::QueryAABB, &AABB>
                        }, {
                            "QueryPoint", Callback<const q3Vec3, &q3Scene::QueryPoint, &Vector>
                        }, {
                            "RayCast", Callback<q3RaycastData, &q3Scene::RayCast, &Raycast>
                        },
                        { nullptr, nullptr }
                    };
                    
                    lua_createtable(L, 0, 2);   // scene, scene_mt, callback_state
                    
                    PutBoxInBox(L, nullptr);// scene, scene_mt, callback_state, box

                    lua_setfield(L, -2, "box"); // scene, scene_mt, callback_state = { box = box }

                    LuaXS::AddClosures(L, callback_closures, 1);// scene, scene_mt = { ..., QueryAABB, QueryPoint, Raycast }
				});
                
                ContactListener * cl = LuaXS::NewTyped<ContactListener>(L); // scene, scene_mt, listener

                cl->mL = L;
                
                lua_newtable(L);// scene, scene_mt, listener, event
                
                cl->mEventRef = lua_ref(L, 1); // scene, scene_mt, listener; registry.event = event
                
                lua_createtable(L, 0, 6);   // scene, scene_mt, listener, state
                
                PutBodyInBox(L, nullptr, true);// scene, scene_mt, listener, state, body1
                PutBodyInBox(L, nullptr, true);// scene, scene_mt, listener, state, body1, body2
                PutBoxInBox(L, nullptr);// scene, scene_mt, listener, state, body1, body2, box1
                PutBoxInBox(L, nullptr);// scene, scene_mt, listener, state, body1, body2, box1, box2
                
                cl->mBody1 = LuaXS::UD<q3Body *>(L, -4);
                cl->mBody2 = LuaXS::UD<q3Body *>(L, -3);
                cl->mBox1 = LuaXS::UD<const q3Box *>(L, -2);
                cl->mBox2 = LuaXS::UD<const q3Box *>(L, -1);
                
                lua_setfield(L, -5, "box2");// scene, scene_mt, listener, state = { box2 = box2 }, body1, body2, box1
                lua_setfield(L, -4, "box1");// scene, scene_mt, listener, state = { box1 = box1, box2 }, body1, body2
                lua_setfield(L, -3, "body2");   // scene, scene_mt, listener, state = { body2 = body2, box1, box2 }, body1
                lua_setfield(L, -2, "body1");   // scene, scene_mt, listener, state = { body1 = body1, body2, box1, box2 }
                
                cl->mContact = LuaXS::NewTyped<const q3ContactConstraint *>(L, nullptr);// scene, scene_mt, listener, state, cc
                
                LuaXS::AttachMethods(L, "qu3e.ContactConstraint", [](lua_State * L) {
                    luaL_Reg methods[] = {
                        {
                            "GetContactBias", [](lua_State * L)
                            {
                                int index = LuaXS::Int(L, 2) - 1;
                                
                                luaL_argcheck(L, index >= 0 && index <= 7, 2, "Invalid contact index");
                                
                                return LuaXS::PushArgAndReturn(L, Contact(L)->manifold.contacts[index].bias);   // cc, index, bias
                            }
                        }, {
                            "GetContactFeatures", [](lua_State * L)
                            {
                                int index = LuaXS::Int(L, 2) - 1;
                                
                                luaL_argcheck(L, index >= 0 && index <= 7, 2, "Invalid contact index");
                                
                                return LuaXS::PushArgAndReturn(L, Contact(L)->manifold.contacts[index].fp.key);   // cc, index, features
                            }
                        }, {
                            "GetContactImpulses", [](lua_State * L)
                            {
                                int index = LuaXS::Int(L, 2) - 1;
                                
                                luaL_argcheck(L, index >= 0 && index <= 7, 2, "Invalid contact index");
                                
                                const q3Contact & contact = Contact(L)->manifold.contacts[index];
                                
                                return LuaXS::PushMultipleArgsAndReturn(L, contact.normalImpulse, contact.tangentImpulse[0], contact.tangentImpulse[1]);   // cc, index, nimp, timp1, timp2
                            }
                        }, {
                            "GetContactMasses", [](lua_State * L)
                            {
                                int index = LuaXS::Int(L, 2) - 1;
                                
                                luaL_argcheck(L, index >= 0 && index <= 7, 2, "Invalid contact index");
                                
                                const q3Contact & contact = Contact(L)->manifold.contacts[index];
                                
                                return LuaXS::PushMultipleArgsAndReturn(L, contact.normalMass, contact.tangentMass[0], contact.tangentMass[1]);   // cc, index, nmass, tmass1, tmass2
                            }
                        }, {
                            "GetContactPenetration", [](lua_State * L)
                            {
                                int index = LuaXS::Int(L, 2) - 1;
                                
                                luaL_argcheck(L, index >= 0 && index <= 7, 2, "Invalid contact index");
                                
                                return LuaXS::PushArgAndReturn(L, Contact(L)->manifold.contacts[index].penetration);   // cc, index, penetration
                            }
                        }, {
                            "__len", [](lua_State * L)
                            {
                                return LuaXS::PushArgAndReturn(L, Contact(L)->manifold.contactCount);   // cc, count
                            }
                        },
                        { nullptr, nullptr }
                    };
                    
                    luaL_register(L, nullptr, methods);
                    
                    LuaXS::AttachProperties(L, [](lua_State * L) {
                        const q3ContactConstraint * cc = Contact(L);
                        const q3Vec3 * pvec = nullptr;
                        const float * pfloat = nullptr;
                        int flag = 0;
                        
                        if (lua_type(L, 2) == LUA_TSTRING)
                        {
                            const char * str = lua_tostring(L, 2);
                            
                            if (strcmp(str, "colliding") == 0) flag = q3ContactConstraint::eColliding;
                            else if (strcmp(str, "wasColliding") == 0) flag = q3ContactConstraint::eWasColliding;
                            else if (strcmp(str, "friction") == 0) pfloat = &cc->friction;
                            else if (strcmp(str, "restitution") == 0) pfloat = &cc->restitution;
                            else if (strcmp(str, "normal") == 0) pvec = &cc->manifold.normal;
                            else if (strcmp(str, "tangent1") == 0) pvec = &cc->manifold.tangentVectors[0];
                            else if (strcmp(str, "tangent2") == 0) pvec = &cc->manifold.tangentVectors[1];
                        }
                        
                        if (flag) return LuaXS::PushArgAndReturn(L, (cc->m_flags & flag) != 0); // cc, k, flag_set
                        else if (pvec) return NewVector(L, *pvec);  // cc, k, vec
                        else if (pfloat) return LuaXS::PushArgAndReturn(L, *pfloat);// cc, k, float
                        else lua_pushnil(L);// cc, k, nil
                        
                        return 1;
                    });
                });
                
                lua_setfield(L, -2, "contact"); // scene, scene_mt, state = { ..., contact = cc }
                
                cl->mStateRef = lua_ref(L, 1); // scene, scene_mt, listener; registry.state = state
                
                lua_pushcclosure(L, [](lua_State * L) {
                    lua_pushvalue(L, lua_upvalueindex(1));  // scene[, begin, end], listener
                    lua_insert(L, 2);   // scene, listener[, begin, end]
                    lua_settop(L, 4);   // scene, listener, begin?, end?
                    
                    ContactListener * cl = LuaXS::UD<ContactListener>(L, 2);
                    
                    for (int i = 3; i <= 4; ++i) luaL_argcheck(L, lua_isfunction(L, i) || lua_isnil(L, i), i, "Expected function(s) or nil");

                    cl->mHasBegin = !lua_isnil(L, 3);
                    cl->mHasEnd = !lua_isnil(L, 4);
                    
                    Scene(L)->SetContactListener(cl->mHasBegin || cl->mHasEnd ? cl : nullptr);

                    lua_getref(L, cl->mStateRef);   // scene, listener, begin?, end?, state
                    lua_replace(L, 2);  // scene, state, begin?, end?
                    lua_setfield(L, 2, "end_contact_qu3e"); // scene, state = { ..., end_contact_qu3e = end / nil }, begin?
                    lua_setfield(L, 2, "begin_contact_qu3e");   // scene, state = { ..., begin_contact_qu3e = begin / nil, end_contact_qu3e }
                    
                    return 0;
                }, 1);  // scene, scene_mt, SetContactListener
                lua_setfield(L, -2, "SetContactListener");  // scene, scene_mt = { ..., SetContactListener }
                
				return 1;
			}
		},
		{ nullptr, nullptr }
	};

	luaL_register(L, nullptr, scene_funcs);
    /*
     // Render the scene with an interpolated time between the last frame and
     // the current simulation step.
     void Render( q3Render* render ) const;
     */
}
