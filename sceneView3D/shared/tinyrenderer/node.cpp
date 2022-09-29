#include "tinyrenderer.h"

//
//
//

#define NODE_TYPE "sceneView3D.tiny.Node"

//
//
//

namespace tiny {

//
//
//

Node & GetNode (lua_State * L, int arg)
{
    Node * node = LuaXS::CheckUD<Node>(L, arg, NODE_TYPE);

    luaL_argcheck(L, !node->IsDestroyed(), arg, "Node destroyed");
    
    return *node;
}

//
//
//

void add_node (lua_State * L)
{
    lua_pushcfunction(L, [](lua_State * L) {
        LuaXS::NewTyped<Node>(L); // node
        LuaXS::AttachMethods(L, NODE_TYPE, [](lua_State * L) {
            luaL_Reg methods[] = {
                {
                    "Destroy", [](lua_State * L)
                    {
                        Node & node = GetNode(L);
                        
                        if (node.mIsSceneRoot) CORONA_LOG_WARNING("Cannot destroy scene root");

                        else
                        {
                            for (size_t i = 0; i < node.mObjects.size(); ++i) node.mObjects[i]->mParent = nullptr;
                            for (size_t i = 0; i < node.mSubnodes.size(); ++i) node.mSubnodes[i]->mParent = nullptr;

                            node.MarkDestroyed();
                        }

                        return 0;
                    }
                }, {
                    "DetachObject", [](lua_State * L)
                    {
                        Node & node = GetNode(L);
                        int index = LuaXS::Int(L, 2);

                        if (index >= 1 && index <= node.mObjects.size())
                        {
                            node.mObjects[index - 1]->mParent = nullptr;

                            node.mObjects.erase(node.mObjects.begin() + index - 1);
                        }

                        else CORONA_LOG_WARNING("Invalid DetachObject() index");

                        return 0;
                    }
                }, {
                    "DetachSelf", [](lua_State * L)
                    {
                        Node & node = GetNode(L);
						Node * parent = static_cast<Node *>(node.mParent);

						if (parent)
						{
							auto iter = std::find(parent->mSubnodes.begin(), parent->mSubnodes.end(), &node);

							if (iter != parent->mSubnodes.end()) parent->mSubnodes.erase(iter);
						}

						node.mParent = nullptr;

                        return 0;
                    }
                }, {
                    "DetachSubnode", [](lua_State * L)
                    {
                        Node & node = GetNode(L);
                        int index = LuaXS::Int(L, 2);

                        if (index >= 1 && index <= node.mSubnodes.size())
                        {
                            node.mSubnodes[index - 1]->mParent = nullptr;

                            node.mSubnodes.erase(node.mSubnodes.begin() + index - 1);
                        }

                        else CORONA_LOG_WARNING("Invalid DetachSubnode() index");

                        return 0;
                    }
                }, {
                    "GetObject", [](lua_State * L)
                    {
                        Node & node = GetNode(L);
                        int index = LuaXS::Int(L, 2);

                        if (index >= 1 && index <= node.mObjects.size()) GetFromStore(L, node.mObjects[index - 1]); // node, index, object
                        else lua_pushnil(L); // node, index, nil
                        
                        return 1;
                    }
                }, {
                    "GetSubnode", [](lua_State * L)
                    {
                        Node & node = GetNode(L);
                        int index = LuaXS::Int(L, 2);

                        if (index >= 1 && index <= node.mSubnodes.size()) GetFromStore(L, node.mSubnodes[index - 1]); // node, index, subnode
                        else lua_pushnil(L); // node, index, nil
                        
                        return 1;
                    }
                }, {
                    "GetObjectCount", [](lua_State * L)
                    {
                        return LuaXS::PushArgAndReturn(L, GetNode(L).mObjects.size()); // node, count
                    }
                }, {
                    "GetParent", [](lua_State * L)
                    {
                        GetFromStore(L, GetNode(L).mParent); // node, parent
                        
                        return 1;
                    }
                }, {
                    "GetSubnodeCount", [](lua_State * L)
                    {
                        return LuaXS::PushArgAndReturn(L, GetNode(L).mSubnodes.size()); // node, count
                    }
                }, {
                    "Insert", [](lua_State * L)
                    {
                        Node & node = GetNode(L);

                        if (LuaXS::IsType(L, NODE_TYPE, 2) && GetNode(L, 2).mIsSceneRoot) CORONA_LOG_WARNING("Cannot insert scene root");

                        else
                        {
                            luaL_argcheck(L, !lua_equal(L, 1, 2), 2, "Node may not be inserted into itself");
                            lua_getfield(L, 2, "DetachSelf"); // node, item, item:DetachSelf
                            lua_pushvalue(L, 2); // node, item, item:DetachSelf, item
                            lua_call(L, 1, 0); // node, item

                            if (LuaXS::IsType(L, NODE_TYPE, 2)) node.mSubnodes.push_back(LuaXS::UD<Node>(L, 2));
                            else node.mObjects.push_back(&GetObject(L, 2));
                        }

                        return 0;
                    }
                }, {
                    "IsDestroyed", [](lua_State * L)
                    {
                        return LuaXS::PushArgAndReturn(L, LuaXS::CheckUD<Node>(L, 1, NODE_TYPE)->IsDestroyed()); // node, is_destroyed
                    }
                }, {
                    "__len", [](lua_State * L)
                    {
                        Node & node = GetNode(L);
                        
                        return LuaXS::PushArgAndReturn(L, node.mObjects.size() + node.mSubnodes.size()); // node, count
                    }
                }, {
                    "SetPosition", [](lua_State * L)
                    {
						GetNode(L).SetPosition(L);

                        return 0;
                    }
                }, {
                    "SetRotation", [](lua_State * L)
                    {
						GetNode(L).SetRotation(L);

                        return 0;
                    }
                }, {
                    "SetScale", [](lua_State * L)
                    {
						GetNode(L).SetScale(L);

                        return 0;
                    }
                },
                { nullptr, nullptr }
            };
            
            luaL_register(L, nullptr, methods);
        });

        AddToStore(L);
        
        return 1;
    }); // ..., tinyrenderer, NewNode
    lua_setfield(L, -2, "NewNode"); // ..., tinyrenderer = { ..., NewNode = NewNode }
}

//
//
//

}