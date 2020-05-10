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

#include "nudge.h"
#include "nudge_ex.h"
#include "utils/LuaEx.h"

ThreadXS::TLS<lua_State *> sLua;

struct Config {
    int * mCount1{nullptr}, * mCount2{nullptr};
    int mScale;
    
    Config (int scale = 1) : mScale{scale}
    {
    }
};

struct ConfigEx {
    Config mConfig;
    int mN1, mN2;
    
    ConfigEx (int scale = 1) : mConfig{scale}
    {
        mConfig.mCount1 = &mN1;
        mConfig.mCount2 = &mN2;
    }
};

static void AddCountRef (lua_State * L, const char * key, int n)
{
    luaL_getmetafield(L, -1, key);  // ..., object, object_to_count
    lua_pushvalue(L, -2);   // ..., object, object_to_count, object
    lua_pushinteger(L, n);  // ... object, object_to_count, object, n
    lua_rawset(L, -3);  // ..., object, object_to_count = { ..., [object] = n }
    lua_pop(L, 1);  // ..., object
}

template<typename T, bool bReserve = false> TypeBase<T> * New (lua_State * L, const Config & config = Config{})
{
    InitOpts opts;
    Arena * arena = nullptr;
    
    if (lua_istable(L, 1))
    {
        lua_getfield(L, 1, "arena");// params, arena?
        lua_getfield(L, 1, "count");// params, arena?, count?
        lua_getfield(L, 1, "count1");   // params, arena?, count?, count1?
        lua_getfield(L, 1, "count2");   // params, arena?, count?, count1?, count2?
        
        arena = Arena::GetRef(L, -4);
        
        opts.mCount = config.mScale * (!lua_isnil(L, -2) ? LuaXS::Int(L, -2) : luaL_optint(L, -3, 0));
        opts.mCount2 = luaL_optint(L, -1, 0);
    }
    
    TypeBase<T> * result;
    
    if (arena)
    {
        result = LuaXS::NewTyped<TypeFromArena<T>>(L, L, *arena);  // params, arena, count?, count1?, count2?, object
        
        if (config.mCount1) *config.mCount1 = opts.mCount;
        if (config.mCount2) *config.mCount2 = opts.mCount2;
    }
    
    else result = LuaXS::NewTyped<TypeWithVectors<T>>(L);   // [params, nil, count?, count1?, count2?, ]object
    
    result->Init(L, opts, bReserve);
    
    return result;
}

CORONA_EXPORT int luaopen_plugin_nudge (lua_State * L)
{
    sLua = L;
    
	lua_newtable(L);// nudge

    luaL_Reg nfuncs[] = {
        {
            "NewActiveBodies", [](lua_State * L)
            {
                New<nudge::ActiveBodies>(L, Config{64});   // [params, ]ab
                LuaXS::AttachMethods(L, Name<nudge::ActiveBodies>::Get(), [](lua_State * L) {
                    luaL_Reg methods[] = {
                        {
                            "Advance", [](lua_State * L)
                            {
                                nudge::advance(ActiveBodies::Get(L), BodyData::Get(L, 2), LuaXS::Float(L, 3));
                                
                                return 0;
                            }
                        }, {
                            "Collide", [](lua_State * L)
                            {
                                nudge::collide(&ActiveBodies::Get(L),
                                               &ContactData::Get(L, 2), BodyData::Get(L, 3), ColliderData::Get(L, 4),
                                               BodyConnections::Get(L, 5),
                                               Arena::Get(L, 6));
                                
                                return 0;
                            }
                        }, {
                            "__gc", LuaXS::TypedGC<ActiveBodies>
                        },
                        { nullptr, nullptr }
                    };
                    
                    luaL_register(L, nullptr, methods);
                });
                
                return 1;
            }
        }, {
            "NewArena", [](lua_State * L)
            {
                LuaXS::NewTyped<Arena>(L, LuaXS::Int(L, 1));   // n, arena
                LuaXS::AttachMethods(L, Name<nudge::Arena>::Get(), [](lua_State * L) {
                    luaL_Reg methods[] = {
                        {
                            "__gc", LuaXS::TypedGC<Arena>
                        }, {
                            "Reset", [](lua_State * L)
                            {
                                Arena & arena = Arena::GetWrapper(L);
                                
                                luaL_argcheck(L, arena.mCount == 0, 1, "Arena has allocated complex objects");
                                
                                arena.mObject.data = arena.mStorage.data();
                                arena.mObject.size = arena.mStorage.size();
                                
                                return 0;
                            }
                        },
                        { nullptr, nullptr }
                    };
                    
                    luaL_register(L, nullptr, methods);
                });

                return 1;
            }
        }, {
            "NewBodyConnections", [](lua_State * L)
            {
                New<nudge::BodyConnections>(L);   // [params, ]bc
                LuaXS::AttachMethods(L, Name<nudge::BodyConnections>::Get(), [](lua_State * L) {
                    luaL_Reg methods[] = {
                        {
                            "__gc", LuaXS::TypedGC<BodyConnections>
                        },
                        { nullptr, nullptr }
                    };
                    
                    luaL_register(L, nullptr, methods);
                });

                return 1;
            }
        }, {
            "NewBodyData", [](lua_State * L)
            {
                New<nudge::BodyData>(L);   // [params, ]bd
                LuaXS::AttachMethods(L, Name<nudge::BodyData>::Get(), [](lua_State * L) {
                    luaL_Reg methods[] = {
                        {
                            "__gc", LuaXS::TypedGC<BodyData>
                        },
                        { nullptr, nullptr }
                    };
                    
                    luaL_register(L, nullptr, methods);
                });

                return 1;
            }
        }, {
            "NewColliderData", [](lua_State * L)
            {
                ConfigEx ex;
                
                ColliderData * cd = New<nudge::ColliderData>(L, ex.mConfig);   // [params, ]cd

                LuaXS::AttachMethods(L, Name<nudge::ColliderData>::Get(), ColliderDataMethods);

                if (!cd->mTWV)
                {
                    AddCountRef(L, "box_counts", ex.mN1);
                    AddCountRef(L, "sphere_counts", ex.mN2);
                }
                
                return 1;
            }
        }, {
            "NewContactCache", [](lua_State * L)
            {
                New<nudge::ContactCache>(L, Config{64});   // [params, ]cc
                LuaXS::AttachMethods(L, Name<nudge::ContactCache>::Get(), [](lua_State * L) {
                    luaL_Reg methods[] = {
                        {
                            "__gc", LuaXS::TypedGC<ContactCache>
                        }, {
                            "WriteCachedImpulses", [](lua_State * L)
                            {
                                nudge::write_cached_impulses(&ContactCache::Get(L), ContactData::Get(L, 2), *ContactImpulseData(L, 3));
                                
                                return 0;
                            }
                        },
                        { nullptr, nullptr }
                    };
                    
                    luaL_register(L, nullptr, methods);
                });

                return 1;
            }
        }, {
            "NewContactConstraintData", [](lua_State * L)
            {
                LuaXS::NewTyped<nudge::ContactConstraintData *>(L, nullptr);   // ccd
                LuaXS::AttachMethods(L, Name<nudge::ContactConstraintData>::Get(), [](lua_State * L) {
                    luaL_Reg methods[] = {
                        {
                            "ApplyImpulses", [](lua_State * L)
                            {
                                nudge::apply_impulses(*ContactConstraintData(L), BodyData::Get(L, 2));
                                
                                return 0;
                            }
                        }, {
                            "Setup", [](lua_State * L)
                            {
                                nudge::ContactConstraintData ** box = ContactConstraintData<false>(L);
                                
                                *box = nullptr;
                                *box = nudge::setup_contact_constraints(ActiveBodies::Get(L, 2),
                                                                        ContactData::Get(L, 3), BodyData::Get(L, 4),
                                                                        *ContactImpulseData(L, 5),
                                                                        &Arena::Get(L, 6));
                                
                                return 0;
                            }
                        }, {
                            "UpdateCachedImpulses", [](lua_State * L)
                            {
                                nudge::update_cached_impulses(*ContactConstraintData(L), *ContactImpulseData(L, 2));
                                
                                return 0;
                            }
                        },
                        { nullptr, nullptr }
                    };
                    
                    luaL_register(L, nullptr, methods);
                });

                return 1;
            }
        }, {
            "NewContactData", [](lua_State * L)
            {
                New<nudge::ContactData>(L, Config{64});   // [params, ]cd
                LuaXS::AttachMethods(L, Name<nudge::ContactData>::Get(), [](lua_State * L) {
                    luaL_Reg methods[] = {
                        {
                            "__gc", LuaXS::TypedGC<ContactData>
                        },
                        { nullptr, nullptr }
                    };
                    
                    luaL_register(L, nullptr, methods);
                });

                return 1;
            }
        }, {
            "NewContactImpulseData", [](lua_State * L)
            {
                LuaXS::NewTyped<nudge::ContactImpulseData *>(L);   // cid
                LuaXS::AttachMethods(L, Name<nudge::ContactImpulseData>::Get(), [](lua_State * L) {
                    luaL_Reg methods[] = {
                        {
                            "ReadCachedImpulses", [](lua_State * L)
                            {
                                nudge::ContactImpulseData ** box = ContactImpulseData<false>(L);
                                
                                *box = nullptr;
                                *box = nudge::read_cached_impulses(ContactCache::Get(L, 2), ContactData::Get(L, 3), &Arena::Get(L, 4));
                                
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

    luaL_register(L, nullptr, nfuncs);

	return 1;
}
