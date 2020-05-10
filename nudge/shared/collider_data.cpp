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

#include "nudge_ex.h"

template<typename T, T (nudge::ColliderData::*mColl)> struct CountName {
    static const char * Get (void) { return ""; }
};

template<> const char * CountName<decltype(nudge::ColliderData::boxes), &nudge::ColliderData::boxes>::Get (void) { return "box_counts"; }
template<> const char * CountName<decltype(nudge::ColliderData::spheres), &nudge::ColliderData::spheres>::Get (void) { return "sphere_counts"; }

#define COLLIDER_UPDATE_BIND(method, field) method< decltype(nudge::ColliderData::field), &nudge::ColliderData::field,   \
                                                    decltype(TypeWithVectors<nudge::ColliderData>::field), &TypeWithVectors<nudge::ColliderData>::field>
#define COLLIDER_UPDATE_DECLARE() template<typename T, T (nudge::ColliderData::*mColl1), typename U, U (TypeWithVectors<nudge::ColliderData>::*mColl2)>

/*
 struct ColliderData {
     struct {
         uint16_t* tags;
         BoxCollider* data;
         Transform* transforms;
         uint32_t count;
     } boxes;
 
     struct {
         uint16_t* tags;
         SphereCollider* data;
         Transform* transforms;
         uint32_t count;
     } spheres;
 }; */

COLLIDER_UPDATE_DECLARE() int Add (lua_State * L)
{
    ColliderData & cd = ColliderData::GetWrapper(L);
    auto & coll1 = cd.mObject.*mColl1;
    int n = luaL_optint(L, 2, 1);
    
    if (cd.mTWV)
    {
        auto & coll2 = cd.mTWV->*mColl2;
        size_t size = coll2.mData.size();
        
        coll2.mData.resize(size + n);
        coll2.mTags.resize(size + n);
        coll2.mTransforms.resize(size + n);
        
        coll1.data = coll2.mData.data();
        coll1.tags = coll2.mTags.data();
        coll1.transforms = coll2.mTransforms.data();
        
        coll1.count += uint32_t(n);
    }
    
    else
    {
        luaL_getmetafield(L, 1, CountName<T, mColl1>::Get());  // cd[, n], count
        
        uint32_t size = LuaXS::Uint(L, -1);
        
        coll1.count = (std::min)(coll1.count + uint32_t(n), size);
        n = int(size - coll1.count);
    }
    
    return LuaXS::PushArgAndReturn(L, n);   // cd[, n][, count], nadded
}

COLLIDER_UPDATE_DECLARE() int Pop (lua_State * L)
{
    ColliderData & cd = ColliderData::GetWrapper(L);
    auto & coll1 = cd.mObject.*mColl1;
    int n = luaL_optint(L, 2, 1);
    
    n = (std::min)(n, int(coll1.count));
    
    if (cd.mTWV)
    {
        auto & coll2 = cd.mTWV->*mColl2;
        size_t size = coll2.mData.size();
        
        coll2.mData.resize(size - n);
        coll2.mTags.resize(size - n);
        coll2.mTransforms.resize(size - n);
        
        coll1.data = coll2.mData.data();
        coll1.tags = coll2.mTags.data();
        coll1.transforms = coll2.mTransforms.data();
    }
    
    coll1.count -= uint32_t(n);
    
    return LuaXS::PushArgAndReturn(L, n);   // cd[, n], npopped
}

template<typename T, T (nudge::ColliderData::*mCollection)> int SetTransforms (lua_State * L)
{
    ColliderData & cd = ColliderData::GetWrapper(L);
    auto & collection = cd.mObject.*mCollection;
    int index = LuaXS::Int(L, 2) - 1, nmax = luaL_optint(L, 4, 0);
    size_t n = 1U;
    
    luaL_argcheck(L, index >= 0 && uint32_t(index) < collection.count, 2, "Invalid index");
    
    ByteReader reader{L, 3};
    
    if (reader.mBytes) n = LuaXS::ArrayN<nudge::Transform>(L, 3);
    else
    {
        luaL_checktype(L, 3, LUA_TTABLE);
        lua_getfield(L, 3, "is_array"); // cd, index, transforms[, n], is_array
        
        n = lua_toboolean(L, -1) ? 1U : lua_objlen(L, 3);
    }
    
    if (nmax) n = (std::min)(n, size_t(nmax));
    
    n = (std::min)(n, size_t(collection.count));
    
    if (reader.mBytes) memcpy(&collection.transforms[index], reader.mBytes, n * sizeof(nudge::Transform));
    else if (n)
    {
        std::vector<nudge::Transform> xforms(n);
        
        if (lua_toboolean(L, -1)) lua_rawgeti(L, 3, 1);  // cd, index, transforms[, n], false, transforms[1]
        else lua_pushvalue(L, 3);  // cd, index, transforms[, n], true, transforms
        
        for (int i = 1; i <= int(n); ++i)
        {
            nudge::Transform & xform = xforms[i - 1];
            
            lua_getfield(L, -1, "position");// cd, index, transforms[, n], is_array, xform, position
            lua_getfield(L, -2, "body");// cd, index, transforms[, n], is_array, xform, position, body
            lua_getfield(L, -3, "rotation");// cd, index, transforms[, n], is_array, xform, position, body, rotation
            
            SetArray<float, 3>(L, xform.position, -3);
            SetArray<float, 4>(L, xform.rotation, -1);
            
            xform.body = uint32_t(LuaXS::Int(L, -2));
            
            lua_pop(L, 4);  // cd, index, transforms[, n], is_array
            lua_rawgeti(L, 3, i + 1);   // cd, index, transforms[, n], true, transforms[i + 1] / nil
        }
        
        memcpy(&collection.transforms[index], xforms.data(), n * sizeof(nudge::Transform));
    }
    
    return LuaXS::PushArgAndReturn(L, n);   // cd, index, transforms[, n][, is_array, nil], nset
}

static int SetTags (lua_State * L, uint16_t * tags, uint32_t count)
{
    ColliderData & cd = ColliderData::GetWrapper(L);
    int index = LuaXS::Int(L, 2) - 1, nmax = luaL_optint(L, 4, 0);
    size_t n = 1U;
    
    luaL_argcheck(L, index >= 0 && uint32_t(index) < count, 2, "Invalid index");
    
    ByteReader reader{L, 3};
    
    if (reader.mBytes) n = LuaXS::ArrayN<nudge::Transform>(L, 3);
    else if (lua_istable(L, 3)) n = lua_objlen(L, 3);
    
    if (nmax) n = (std::min)(n, size_t(nmax));
    
    n = (std::min)(n, size_t(count));
    
    if (reader.mBytes) memcpy(&tags[index], reader.mBytes, n * sizeof(uint16_t));
    else if (n)
    {
		if (!lua_istable(L, 3)) tags[index] = (uint16_t)LuaXS::Uint(L, 3);
		else
		{
			std::vector<uint16_t> tagv(n);
        
			for (int i = 1; i <= int(n); ++i, lua_pop(L, 1))
			{
				lua_rawgeti(L, 3, i);	// cd, index, tags[, n], tag

				tagv[i - 1] = (uint16_t)LuaXS::Uint(L, -1);
			}
        
			memcpy(&tags[index], tagv.data(), n * sizeof(uint16_t));
		}
    }
    
    return LuaXS::PushArgAndReturn(L, n);   // cd, index, tags[, n], nset
}

void ColliderDataMethods (lua_State * L)
{
    luaL_Reg methods[] = {
        {
            "AddBoxes", COLLIDER_UPDATE_BIND(Add, boxes)
        }, {
            "AddSpheres", COLLIDER_UPDATE_BIND(Add, spheres)
        }, {
            "BoxCount", [](lua_State * L)
            {
                return LuaXS::PushArgAndReturn(L, ColliderData::Get(L).boxes.count);// cd, nboxes
            }
        }, {
            "__gc", LuaXS::TypedGC<ColliderData>
        }, {
            "PopBoxes", COLLIDER_UPDATE_BIND(Pop, boxes)
        }, {
            "PopSpheres", COLLIDER_UPDATE_BIND(Pop, spheres)
        }, {
            "SetBoxColliders", [](lua_State * L)
            {
				/*
				struct BoxCollider {
					float size[3];
					float unused;
				};

				// bytes - assumed to be 4 floats
				// table
					// triples - assumed to be 3 floats
					// else SetArray<float, 3>
				*/
                return 1;
            }
        }, {
            "SetBoxTags", [](lua_State * L)
            {
				nudge::ColliderData & cd = ColliderData::Get(L);

                return SetTags(L, cd.boxes.tags, cd.boxes.count);
            }
        }, {
            "SetBoxTransforms", SetTransforms<decltype(nudge::ColliderData::boxes), &nudge::ColliderData::boxes>
        }, {
            "SetSphereColliders", [](lua_State * L)
            {
				/*
				struct SphereCollider {
					float radius;
				};
				// bytes or array of floats
				*/

                return 1;
            }
        }, {
            "SetSphereTags", [](lua_State * L)
            {
				nudge::ColliderData & cd = ColliderData::Get(L);

                return SetTags(L, cd.spheres.tags, cd.spheres.count);
            }
        }, {
            "SetSphereTransforms", SetTransforms<decltype(nudge::ColliderData::spheres), &nudge::ColliderData::spheres>
        }, {
            "SphereCount", [](lua_State * L)
            {
                return LuaXS::PushArgAndReturn(L, ColliderData::Get(L).spheres.count);  // cd, nspheres
            }
        },
        { nullptr, nullptr }
    };
    
    luaL_register(L, nullptr, methods);
    
    LuaXS::NewWeakKeyedTable(L);// cd, cd_meta, box_counts
    LuaXS::NewWeakKeyedTable(L);// cd, cd_meta, box_counts, sphere_counts
    
    lua_setfield(L, -3, "sphere_counts");   // cd, cd_meta = { ..., sphere_counts = sphere_counts }, box_counts
    lua_setfield(L, -2, "box_counts");  // cd, cd_meta = { ..., box_counts = box_counts, sphere_counts }
}
