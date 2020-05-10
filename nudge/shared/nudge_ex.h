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

#pragma once

#include "nudge.h"
#include "utils/LuaEx.h"
#include "utils/Memory.h"
#include "ByteReader.h"
#include <algorithm>

#if defined(_WIN32) || defined(__ANDROID__)
    const size_t Align = 4U;
#else
    const size_t Align = 8U;
#endif

template<typename T> struct Name {
    static const char * Get (void) { return ""; }
};

#define NUDGE_NAME(type) template<> inline const char * Name<nudge::type>::Get (void) { return "nudge." #type; }

#ifdef _WIN32
    #define NUDGE_VECTOR(type) MemoryXS::AlignedVectorN<type, Align>::vector_type
#else
    #define NUDGE_VECTOR(type) typename MemoryXS::AlignedVectorN<type, Align>::vector_type
#endif

struct Arena;

NUDGE_NAME(Arena)

struct Arena {
    nudge::Arena mObject;
    NUDGE_VECTOR(unsigned char) mStorage;
    int mCount{0}, mRef{LUA_NOREF};
    
    static Arena & GetWrapper (lua_State * L, int arg = 1)
    {
        return *LuaXS::CheckUD<Arena>(L, arg, Name<nudge::Arena>::Get());
    }
    
    static nudge::Arena & Get (lua_State * L, int arg = 1)
    {
        return GetWrapper(L, arg).mObject;
    }
    
    static Arena * GetRef (lua_State * L, int arg)
    {
        if (LuaXS::IsType(L, Name<nudge::Arena>::Get()))
        {
            Arena * arena = LuaXS::UD<Arena>(L, arg);
            
            if (arena->mCount++ == 0)
            {
                lua_pushvalue(L, arg);  // ..., arena
                
                arena->mRef = lua_ref(L, 1);
            }
            
            return arena;
        }
        
        return nullptr;
    }
    
    void * Align (lua_State * L, uintptr_t alignment) // adapted from nudge tests
    {
        uintptr_t data = (uintptr_t)mObject.data;
        uintptr_t end = data + mObject.size;
        uintptr_t mask = alignment - 1;
        
        data = (data + mask) & ~mask;
        
        mObject.data = (void *)data;
        mObject.size = end - data;
        
        if ((intptr_t)mObject.size < 0) luaL_error(L, "Out of memory.");
        
        return mObject.data;
    }
    
    template<typename T> T * Allocate (lua_State * L, int n, uintptr_t alignment) // ditto
    {
        void * data = Align(L, alignment);
        uintptr_t size = uintptr_t(n * sizeof(T));
        
        mObject.data = (void *)((uintptr_t)data + size);
        mObject.size -= size;
        
        if ((intptr_t)mObject.size < 0) luaL_error(L, "Out of memory.");
        
        return static_cast<T *>(data);
    }
    
    void Release (lua_State * L)
    {
        if (--mCount == 0)
        {
            lua_unref(L, mRef);
            
            mRef = LUA_NOREF;
        }
    }
    
    Arena (int n)
    {
        mStorage.resize(n);
        
        mObject.data = mStorage.data();
        mObject.size = n;
    }
};

template<typename T> struct TypeFromArena;
template<typename T> struct TypeWithVectors;

struct InitOpts {
    int mCount{0}, mCount2{0};
};

template<typename T> struct TypeBase {
    T mObject;
    TypeWithVectors<T> * mTWV;
    
    static TypeBase<T> & GetWrapper (lua_State * L, int arg = 1)
    {
        return *LuaXS::CheckUD<TypeBase<T>>(L, arg, Name<T>::Get());
    }
    
    static T & Get (lua_State * L, int arg = 1)
    {
        return GetWrapper(L, arg).mObject;
    }
    
    void CommonInit (const InitOpts & opts);
    void FromArena (lua_State * L, const InitOpts & opts, TypeFromArena<T> & tfa);
    void WithVectors (lua_State * L, const InitOpts & opts, bool bReserve);
    
    void Init (lua_State * L, const InitOpts & opts, bool bReserve)
    {
        mTWV = dynamic_cast<TypeWithVectors<T> *>(this);
        
        if (mTWV) WithVectors(L, opts, bReserve);
        else FromArena(L, opts, *static_cast<TypeFromArena<T> *>(this));
        
        CommonInit(opts);
    }
    
    virtual ~TypeBase (void) {}
};

template<typename T> struct TypeFromArena : TypeBase<T> {
    lua_State * mL;
    Arena & mArena;
    
    TypeFromArena (lua_State * L, Arena & arena) : mL{L}, mArena{arena}
    {
    }
    
    ~TypeFromArena (void) override
    {
        mArena.Release(mL);
    }
};

#define NUDGE_STRUCT_BEGIN(type)    NUDGE_NAME(type)                                                                \
                                    template<> struct TypeWithVectors<nudge::type> : TypeBase<nudge::type> {
#define NUDGE_STRUCT_END(type)  };                                  \
                                using type = TypeBase<nudge::type>

#define NUDGE_COMMON_INIT(type) template<> inline void TypeBase<nudge::type>::CommonInit (const InitOpts & opts)
#define NUDGE_HAS_CAPACITY(type) NUDGE_COMMON_INIT(type) { mObject.capacity = opts.mCount; }
#define NUDGE_COUNT_ONLY(type) NUDGE_COMMON_INIT(type) { mObject.count = 0; }

#define NUDGE_FROM_ARENA(type) template<> inline void TypeBase<nudge::type>::FromArena (lua_State * L, const InitOpts & opts, TypeFromArena<nudge::type> & tfa)
#define NUDGE_WITH_VECTORS(type) template<> inline void TypeBase<nudge::type>::WithVectors (lua_State *, const InitOpts & opts, bool bReserve)

NUDGE_STRUCT_BEGIN(ActiveBodies);
    NUDGE_VECTOR(uint16_t) mIndices;
NUDGE_STRUCT_END(ActiveBodies);

template<typename T> void SetFromArena (lua_State * L, T *& arr, Arena & arena, int n)
{
    arr = arena.Allocate<T>(L, n, Align);
}

template<typename T> void SetFromVector (T *& arr, typename MemoryXS::AlignedVectorN<T, Align>::vector_type & v, int n, bool bReserve = false)
{
    bReserve ? v.reserve(n) : v.resize(n);
    
    arr = v.data();
}

NUDGE_HAS_CAPACITY(ActiveBodies)

NUDGE_FROM_ARENA(ActiveBodies)
{
    SetFromArena(L, mObject.indices, tfa.mArena, opts.mCount);
}

NUDGE_WITH_VECTORS(ActiveBodies)
{
    SetFromVector(mObject.indices, mTWV->mIndices, opts.mCount);
}

NUDGE_STRUCT_BEGIN(BodyConnections);
    NUDGE_VECTOR(nudge::BodyPair) mData;
NUDGE_STRUCT_END(BodyConnections);

NUDGE_COUNT_ONLY(BodyConnections)

NUDGE_FROM_ARENA(BodyConnections)
{
    SetFromArena(L, mObject.data, tfa.mArena, opts.mCount);
}

NUDGE_WITH_VECTORS(BodyConnections)
{
    SetFromVector(mObject.data, mTWV->mData, opts.mCount);
}

/*
 struct BodyConnections {
     BodyPair* data;
     uint32_t count;
 };
 */

NUDGE_STRUCT_BEGIN(BodyData);
    NUDGE_VECTOR(nudge::Transform) mTransforms;
    NUDGE_VECTOR(nudge::BodyProperties) mProperties;
    NUDGE_VECTOR(nudge::BodyMomentum) mMomentum;
    NUDGE_VECTOR(uint8_t) mIdleCounters;
NUDGE_STRUCT_END(BodyData);

NUDGE_COUNT_ONLY(BodyData)

NUDGE_FROM_ARENA(BodyData)
{
    SetFromArena(L, mObject.transforms, tfa.mArena, opts.mCount);
    SetFromArena(L, mObject.properties, tfa.mArena, opts.mCount);
    SetFromArena(L, mObject.momentum, tfa.mArena, opts.mCount);
}

NUDGE_WITH_VECTORS(BodyData)
{
    SetFromVector(mObject.transforms, mTWV->mTransforms, opts.mCount, true);
    SetFromVector(mObject.properties, mTWV->mProperties, opts.mCount, true);
    SetFromVector(mObject.momentum, mTWV->mMomentum, opts.mCount, true);
}

/*
 struct BodyData {
     Transform* transforms;
     BodyProperties* properties;
     BodyMomentum* momentum;
     uint8_t* idle_counters;
     uint32_t count;
 };
 */

NUDGE_STRUCT_BEGIN(ColliderData);
    struct {
        NUDGE_VECTOR(uint16_t) mTags;
        NUDGE_VECTOR(nudge::BoxCollider) mData;
        NUDGE_VECTOR(nudge::Transform) mTransforms;
    } boxes;
    struct {
        NUDGE_VECTOR(uint16_t) mTags;
        NUDGE_VECTOR(nudge::SphereCollider) mData;
        NUDGE_VECTOR(nudge::Transform) mTransforms;
    } spheres;
NUDGE_STRUCT_END(ColliderData);

NUDGE_COMMON_INIT(ColliderData)
{
    mObject.boxes.count = 0;
    mObject.spheres.count = 0;
}

NUDGE_FROM_ARENA(ColliderData)
{
    SetFromArena(L, mObject.boxes.tags, tfa.mArena, opts.mCount);
    SetFromArena(L, mObject.boxes.data, tfa.mArena, opts.mCount);
    SetFromArena(L, mObject.boxes.transforms, tfa.mArena, opts.mCount);
    SetFromArena(L, mObject.spheres.tags, tfa.mArena, opts.mCount2);
    SetFromArena(L, mObject.spheres.data, tfa.mArena, opts.mCount2);
    SetFromArena(L, mObject.spheres.transforms, tfa.mArena, opts.mCount2);
}

NUDGE_WITH_VECTORS(ColliderData)
{
    SetFromVector(mObject.boxes.tags, mTWV->boxes.mTags, opts.mCount, true);
    SetFromVector(mObject.boxes.data, mTWV->boxes.mData, opts.mCount, true);
    SetFromVector(mObject.boxes.transforms, mTWV->boxes.mTransforms, opts.mCount, true);
    SetFromVector(mObject.spheres.tags, mTWV->spheres.mTags, opts.mCount2, true);
    SetFromVector(mObject.spheres.data, mTWV->spheres.mData, opts.mCount2, true);
    SetFromVector(mObject.spheres.transforms, mTWV->spheres.mTransforms, opts.mCount2, true);
}

NUDGE_STRUCT_BEGIN(ContactCache);
    NUDGE_VECTOR(uint64_t) mTags;
    NUDGE_VECTOR(nudge::CachedContactImpulse) mData;
NUDGE_STRUCT_END(ContactCache);

NUDGE_HAS_CAPACITY(ContactCache)

NUDGE_FROM_ARENA(ContactCache)
{
    SetFromArena(L, mObject.tags, tfa.mArena, opts.mCount);
    SetFromArena(L, mObject.data, tfa.mArena, opts.mCount);
}

NUDGE_WITH_VECTORS(ContactCache)
{
    SetFromVector(mObject.tags, mTWV->mTags, opts.mCount);
    SetFromVector(mObject.data, mTWV->mData, opts.mCount);
}

NUDGE_STRUCT_BEGIN(ContactData);
    NUDGE_VECTOR(nudge::Contact) mData;
    NUDGE_VECTOR(nudge::BodyPair) mBodies;
    NUDGE_VECTOR(uint64_t) mTags;
    NUDGE_VECTOR(uint32_t) mSleepingPairs;
NUDGE_STRUCT_END(ContactData);

NUDGE_HAS_CAPACITY(ContactData)

NUDGE_FROM_ARENA(ContactData)
{
    SetFromArena(L, mObject.data, tfa.mArena, opts.mCount);
    SetFromArena(L, mObject.bodies, tfa.mArena, opts.mCount);
    SetFromArena(L, mObject.tags, tfa.mArena, opts.mCount);
    SetFromArena(L, mObject.sleeping_pairs, tfa.mArena, opts.mCount);
}

NUDGE_WITH_VECTORS(ContactData)
{
    SetFromVector(mObject.data, mTWV->mData, opts.mCount);
    SetFromVector(mObject.bodies, mTWV->mBodies, opts.mCount);
    SetFromVector(mObject.tags, mTWV->mTags, opts.mCount);
    SetFromVector(mObject.sleeping_pairs, mTWV->mSleepingPairs, opts.mCount);
}

template<typename T, bool bCheck> T ** GetBox (lua_State * L, int arg, const char * name)
{
    T ** box = LuaXS::CheckUD<T *>(L, arg, name);
    
    luaL_argcheck(L, !bCheck || *box, arg, "No item in box");
    
    return box;
}

#define NUDGE_BOX_GETTER(type)  NUDGE_NAME(type)                                                                \
                                template<bool bCheck = true> nudge::type ** type (lua_State * L, int arg = 1) { \
                                    return GetBox<nudge::type, true>(L, arg, Name<nudge::type>::Get());         \
}

NUDGE_BOX_GETTER(ContactConstraintData)
NUDGE_BOX_GETTER(ContactImpulseData)

template<typename T, size_t N> void SetArray (lua_State * L, T arr[N], int arg)
{
    ByteReader reader{L, arg};
    size_t n;
    
    if (reader.mBytes)
    {
        n = (std::min)(LuaXS::ArrayN<T>(L, arg), N);
        
        memcpy(arr, reader.mBytes, n * sizeof(T));
    }
    
    else
    {
        n = lua_objlen(L, arg);
        
        for (int i = 1; i <= int(n); ++i, lua_pop(L, 1))
        {
            lua_rawgeti(L, arg, i); // ..., t, ..., value
            
            arr[i - 1] = LuaXS::GetArg<T>(L, -1);
        }
    }
    
    memset(arr + n, 0, (N - n) * sizeof(T));
}

void ColliderDataMethods (lua_State * L);
