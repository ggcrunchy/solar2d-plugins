#include "CoronaLua.h"
#include "utils/LuaEx.h"
#include "capture3d.h"
#include "geometry.h"

Vec3f GetVec3 (lua_State * L, int first)
{
	return Vec3f{LuaXS::Float(L, first), LuaXS::Float(L, first + 1), LuaXS::Float(L, first + 2)};
}

void AddConstructor (lua_State * L, const char * name, lua_CFunction func, int nupvalues)
{
    lua_pushcclosure(L, func, nupvalues); // capture3d, func
    lua_setfield(L, -2, name);  // capture3d = { ..., name = func }
}

int FindInParent (lua_State * L)
{
    lua_getfenv(L, 1);  // object / group, items
    lua_getfield(L, -1, "parent");  // object / group, items, parent / nil

    if (lua_isnil(L, -1)) return -1;

    lua_getfenv(L, -1); // object / group, items, parent, pitems
    
    for (size_t i = 1, n = lua_objlen(L, -1); i <= n; ++i)
    {
        lua_rawgeti(L, -1, int(i)); // object / group, items, parent, pitems, elem
        
        bool bFound = lua_equal(L, 1, -1) != 0;
        
        lua_pop(L, 1);  // object / group, items, parent, pitems
        
        if (bFound) return int(i);
    }

    return luaL_error(L, "Not found");
}

CORONA_EXPORT int luaopen_plugin_capture3d (lua_State * L)
{
	lua_newtable(L);// capture3d

    open_group(L);
    open_model(L);
    open_object(L);
    open_scene(L);
    open_texture(L);

	return 1;
}
