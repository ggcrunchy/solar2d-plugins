#include "tinyrenderer.h"
#include "utils/LuaEx.h"

//
//
//

#define GROUP_TYPE "scene3d.tiny.Group"

//
//
//

namespace tiny {

//
//
//

Group & GetGroup (lua_State * L, int arg)
{
    Group * group = LuaXS::CheckUD<Group>(L, arg, GROUP_TYPE);

    luaL_argcheck(L, !group->IsDestroyed(), arg, "Group destroyed");
    
    return *group;
}

//
//
//

void Group::Destroy (void)
{
    for (auto && iter : mObjects) iter->Destroy();
    for (auto && iter : mSubGroups) iter->Destroy();
    
    MarkDestroyed();
}

//
//
//

static void AuxRemoveGroup (Group & group, lua_State * L, int arg)
{
    lua_getfield(L, arg, "Remove");  // ..., group, ..., group:Remove
    
    for (size_t n = lua_objlen(L, arg); n; --n)
    {
        lua_pushvalue(L, -1);   // ..., group, ..., group:Remove, group:Remove
        lua_pushvalue(L, arg);  // ..., group, ..., group:Remove, group:Remove, group
        lua_pushinteger(L, n);  // ..., group, ..., group:Remove, group:Remove, group, n
        lua_call(L, 2, 0);  // ..., group, ..., group:Remove
    }
    
    group.Destroy();
}

void open_group (lua_State * L)
{
    AddConstructor(L, "NewGroup", [](lua_State * L) {
        LuaXS::NewTyped<Group>(L);  // group
        
        lua_newtable(L);// group, items
        lua_setfenv(L, -2); // group; group.env = items
        
        LuaXS::AttachMethods(L, GROUP_TYPE, [](lua_State * L) {
            luaL_Reg methods[] = {
                {
                    "DetachSelf", [](lua_State * L)
                    {
                        GetGroup(L);

                        int index = FindInParent(L);// group, items, parent / nil[, pitems]

                        if (index > 0)
                        {
                            lua_pop(L, 1);  // group, items, parent
                            lua_getfield(L, -1, "Detach");  // group, items, parent, parent:Detach
                            lua_insert(L, -2);  // group, items, parent:Detach, parent
                            lua_pushinteger(L, index);  // group, items, parent:Detach, parent, index
                            lua_call(L, 2, 0);  // group, items
                        }
                        
                        return 0;
                    }
                }, {
                    "Get", [](lua_State * L)
                    {
                        GetGroup(L);
                        
                        lua_settop(L, 2);   // group, index
                        lua_getfenv(L, 1);  // group, index, items
                        lua_insert(L, 2);   // group, items, index
                        lua_rawget(L, 2);   // group, items, item?
                        
                        return 1;
                    }
                }, {
                    "GetObjectCount", [](lua_State * L)
                    {
                        return LuaXS::PushArgAndReturn(L, GetGroup(L).mObjects.size()); // group, count
                    }
                }, {
                    "GetParent", [](lua_State * L)
                    {
                        GetGroup(L);

                        lua_getfenv(L, 1);  // group, items
                        lua_getfield(L, -1, "parent");  // group, items, parent / nil
                        
                        return 1;
                    }
                }, {
                    "GetSubGroupCount", [](lua_State * L)
                    {
                        return LuaXS::PushArgAndReturn(L, GetGroup(L).mSubGroups.size());   // group, count
                    }
                }, {
                    "IsDestroyed", [](lua_State * L)
                    {
                        return LuaXS::PushArgAndReturn(L, LuaXS::CheckUD<Group>(L, 1, GROUP_TYPE)->IsDestroyed());   // group, is_destroyed
                    }
                }, {
                    "__len", [](lua_State * L)
                    {
                        Group & group = GetGroup(L);
                        
                        return LuaXS::PushArgAndReturn(L, group.mObjects.size() + group.mSubGroups.size()); // group, count
                    }
                }, {
                    "Remove", [](lua_State * L)
                    {
                        GetGroup(L);
                        
                        lua_settop(L, 2);   // group, index
                        lua_getfield(L, 1, "Detach");   // group, index, group:Detach
                        lua_insert(L, 2);   // group, group:Detach, index
                        lua_pushvalue(L, 1);// group, group:Detach, index, group
                        lua_insert(L, 3);   // group, group:Detach, group, index
                        lua_call(L, 2, 1);  // group, item
                        
                        if (LuaXS::IsType(L, GROUP_TYPE, 2)) AuxRemoveGroup(GetGroup(L, 2), L, 2);  // group, item, group:Remove
                        else GetObject(L, 2).Destroy();
                        
                        return 0;
                    }
                }, {
                    "RemoveSelf", [](lua_State * L)
                    {
                        Group & group = GetGroup(L);
                        
                        lua_getfield(L, 1, "DetachSelf");   // group, group:DetachSelf
                        lua_pushvalue(L, 1);// group, group:DetachSelf, group
                        lua_call(L, 1, 0);  // group
                        lua_getfenv(L, 1);  // group, items
                        lua_getfield(L, -1, "scene");   // group, items, scene / nil
                        luaL_argcheck(L, lua_isnil(L, 1), 1, "Cannot remove root");

                        AuxRemoveGroup(group, L, 1);// group, items, nil, group:Remove
                        
                        return 0;
                    }
                }, {
                    "SetPosition", [](lua_State * L)
                    {
						GetGroup(L).mXform.SetPosition(L);

                        return 0;
                    }
                }, {
                    "SetRotation", [](lua_State * L)
                    {
						GetGroup(L).mXform.SetRotation(L);

                        return 0;
                    }
                }, {
                    "SetScale", [](lua_State * L)
                    {
						GetGroup(L).mXform.SetScale(L);

                        return 0;
                    }
                },
                { nullptr, nullptr }
            };
            
            luaL_register(L, nullptr, methods);
            lua_newtable(L);// group, group_mt, object_to_parent
            lua_getglobal(L, "table");  // group, group_mt, object_to_parent, table
            lua_pushvalue(L, -2);   // group, group_mt, object_to_parent, table, object_to_parent
            lua_getfield(L, -2, "remove");  // group, group_mt, object_to_parent, table, object_to_parent, table.remove
            lua_pushcclosure(L, [](lua_State * L) {
                Group & group = GetGroup(L);
                int index = LuaXS::Int(L, 2);

                lua_settop(L, 2);   // group, index
                lua_getfenv(L, 1);  // group, index, items
                lua_rawgeti(L, -1, index);  // group, index, items, item?
                luaL_argcheck(L, !lua_isnil(L, -1), 2, "Bad index");

                if (LuaXS::IsType(L, GROUP_TYPE, -1))
                {
                    Group * pgroup = LuaXS::UD<Group>(L, -1);

                    lua_getfenv(L, -1); // group, index, items, item, iitems
                    lua_pushnil(L); // group, index, items, item, iitems, nil
                    lua_setfield(L, -2, "parent");  // group, index, items, item, iitems = { ..., parent = nil }

                    auto iter = std::find(group.mSubGroups.begin(), group.mSubGroups.end(), pgroup);

					group.mSubGroups.erase(iter);
                }
                
                else
                {
                    Object & cobject = GetObject(L, -1);
                    
                    lua_pushvalue(L, -1);   // group, index, items, item, item
                    lua_pushvalue(L, lua_upvalueindex(1));  // group, index, items, item, item, object_to_parent
                    lua_insert(L, -2);  // group, index, items, item, object_to_parent, item
                    lua_pushnil(L); // group, index, items, item, object_to_parent, item, nil
                    lua_rawset(L, -3);  // group, index, items, item, object_to_parent = { ..., [item] = nil }
                    
                    auto iter = std::find(group.mObjects.begin(), group.mObjects.end(), &cobject);

					group.mObjects.erase(iter);
                }
                
                lua_pop(L, 1);  // group, index, items, item
                lua_replace(L, 1);  // item, index, items
                lua_insert(L, 2);   // item, items, index
                lua_pushvalue(L, lua_upvalueindex(2));  // item, items, index, table.remove
                lua_insert(L, 2);   // item, table.remove, items, index
                lua_pushvalue(L, 2);// item, table.remove, items, index
                lua_call(L, 2, 0);  // item; items = { ..., [index] = nil } (items moved down)
                
                return 1;
            }, 2);  // group, group_mt, object_to_parent, table, Detach
            lua_setfield(L, -4, "Detach");  // group, group_mt = { ..., Detach = Detach }, object_to_parent, table
            lua_pop(L, 1);  // group, group_mt, object_to_parent
            lua_pushcclosure(L, [](lua_State * L) {
                Group & group = GetGroup(L);

                luaL_argcheck(L, !lua_equal(L, 1, 2), 2, "Group may not be inserted into itself");
                lua_settop(L, 2);   // group, item
                lua_getfield(L, 2, "Detach");   // group, item, item:Detach
                lua_pushvalue(L, 2);// group, item, item:Detach, item
                lua_call(L, 1, 0);  // group, item
                
                if (LuaXS::IsType(L, GROUP_TYPE, 2))
                {
                    Group * pgroup = LuaXS::UD<Group>(L, 2);
                    
                    lua_getfenv(L, 2);  // group, item, iitems
                    lua_pushvalue(L, 1);// group, item, iitems, group
                    lua_setfield(L, -2, "parent");  // group, item, iitems = { ..., parent = group }
                    lua_pop(L, 1);  // group, item
                    
                    group.mSubGroups.push_back(pgroup);
                }
                
                else
                {
                    Object & cobject = GetObject(L, 2);
                    
                    lua_pushvalue(L, lua_upvalueindex(1));  // group, item, object_to_parent
                    lua_pushvalue(L, 2);// group, item, object_to_parent, item
                    lua_pushvalue(L, 1);// group, item, object_to_parent, item, group
                    lua_rawset(L, -3);  // group, item, object_to_parent = { ..., [item] = group }
                    lua_pop(L, 1);  // group, item
                    
                    group.mObjects.push_back(&cobject);
                }
                
                lua_getfenv(L, 1);  // group, item, items
                lua_insert(L, 2);   // group, items, item
                lua_rawseti(L, -2, int(lua_objlen(L, -2) + 1)); // group, items = { ..., item }
                
                return 0;
            }, 1);  // group, group_mt, Insert
            lua_setfield(L, -2, "Insert");  // group, group_mt = { ..., Detach, Insert = Insert }
        });
        
        return 1;
    });
}

//
//
//

}