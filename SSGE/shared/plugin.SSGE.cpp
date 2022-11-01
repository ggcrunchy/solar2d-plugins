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

#include "CoronaLua.h"
#include "SSGE/include/engine.h"
#include "SSGE/include/camera.h"
#include "SSGE/include/model.h"
#include "SSGE/include/scene.h"
#include "SSGE/include/texture.h"

//
//
//

#define STORE_NAME "SSGE.Store"

//
//
//

void AddToStore (lua_State * L, void * object)
{
	if (!object) object = lua_touserdata(L, -1);

	lua_getfield(L, LUA_REGISTRYINDEX, STORE_NAME); // ..., object, store
	lua_pushlightuserdata(L, object); // ..., object, store, object_ptr
	lua_pushvalue(L, -3); // ..., object, store, object_ptr, object
	lua_rawset(L, -3); // ..., object, store = { ..., [object_ptr] = object }
	lua_pop(L, 1); // ..., object
}

//
//
//

void GetFromStore (lua_State * L, void * object)
{
	lua_getfield(L, LUA_REGISTRYINDEX, STORE_NAME); // ..., store
	lua_pushlightuserdata(L, object); // .., store, object_ptr
	lua_rawget(L, -2); // ..., store, object
	lua_replace(L, -2); // ..., object
}

//
//
//

void RemoveFromStore (lua_State * L, void * object)
{
	if (!object) object = lua_touserdata(L, 1);

	lua_getfield(L, LUA_REGISTRYINDEX, STORE_NAME); // [object, ]..., store
	lua_pushlightuserdata(L, object); // [object, ]..., store, object_ptr
	lua_pushnil(L); // [object, ]..., store, object_ptr, nil
	lua_rawset(L, -3); // [object, ]..., store = { ..., [object_ptr] = nil }
	lua_pop(L, 1); // [object, ]...
}

//
//
//

CORONA_EXPORT int luaopen_plugin_SSGE (lua_State * L)
{
    lua_newtable(L); // SSGE

    Engine::add_engine(L);
    Camera::add_camera(L);
    Model::add_model(L);
    Scene::add_scene(L);
    Texture::add_texture(L);

    //
    //
    //

    lua_newtable(L); // sceneView3D, store
    lua_setfield(L, LUA_REGISTRYINDEX, STORE_NAME); // sceneView3D; registry[name] = store

	return 1;
}
