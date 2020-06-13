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

int Index (lua_State * L);

uint32_t CheckUint32 (lua_State * L, int arg);

template<typename T> struct Box {
	T mItem;
	bool mValid;
};

template<typename T> T * New (lua_State * L)
{
	Box<T> * box = (Box<T> *)lua_newuserdata(L, sizeof(Box<T>)); // object

	memset(&box->mItem, 0, sizeof(T));

	box->mValid = true;

	return &box->mItem;
}

template<typename T> T * Get (lua_State * L, int arg, const char * name)
{
	int targ = INDEX_DESTROY == arg ? 1 : arg;
	Box<T> * box = static_cast<Box<T> *>(luaL_checkudata(L, targ, name));

	luaL_argcheck(L, box->mValid || INDEX_DESTROY == arg, targ, "Object has been destroyed");

	return box->mValid ? &box->mItem : nullptr;
}

template<typename T> bool Is (lua_State * L, int arg, const char * name)
{
	if (!lua_getmetatable(L, arg)) return false;// ...[, mt1]

	luaL_getmetatable(L, name);	// ..., mt1, mt2

	bool ok = !!lua_equal(L, -1, -2);

	lua_pop(L, 2);	// ...

	return ok && static_cast<Box<T> *>(lua_touserdata(L, arg))->mValid;
}

template<typename T, T * (*get)(lua_State *, int), uint32_t (*destroy)(T *)> int Destroy (lua_State * L)
{
	T * object = get(L, 1);

	destroy(object);

	reinterpret_cast<Box<T> *>(object)->mValid = false;

	return 0;
}

#define BLEND2D_DESTROY(type) "destroy", Destroy<BL##type##Core, &Get##type, &bl##type##Destroy>

template<typename T, T * (*get)(lua_State *, int), uint32_t (*destroy)(T *)> int GC (lua_State * L)
{
	T * object = get(L, INDEX_DESTROY);

	if (object) destroy(object);

	return 0;
}

#define BLEND2D_GC(type) "__gc", GC<BL##type##Core, &Get##type, &bl##type##Destroy>
