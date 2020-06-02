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

template<typename T> T * New (lua_State * L)
{
	T * object = (T *)lua_newuserdata(L, sizeof(T) + sizeof(bool)); // object

	memset(object, 0, sizeof(T));

	*reinterpret_cast<bool *>(&object[1]) = true;

	return object;
}

template<typename T> T * Get (lua_State * L, int arg, const char * name, bool * intact_ptr = nullptr)
{
	T * object = static_cast<T *>(luaL_checkudata(L, arg, name));

	bool intact = *reinterpret_cast<bool *>(&object[1]);

	if (intact_ptr) *intact_ptr = intact;
	else luaL_argcheck(L, intact, arg, "Object has been destroyed");

	return object;
}

template<typename T> bool Is (lua_State * L, int arg, const char * name)
{
	lua_getmetatable(L, arg);	// ..., mt1
	luaL_getmetatable(L, name);	// ..., mt2

	bool ok = false;

	if (lua_equal(L, -1, -2) && !lua_isnil(L, -1)) // has the same metatable and both are tables?
	{
		T * object = static_cast<T *>(lua_touserdata(L, arg));

		ok = *reinterpret_cast<bool *>(&object[1]);
	}

	lua_pop(L, 2);	// ...

	return ok;
}

template<typename T> void Destroy (T * object)
{
	*reinterpret_cast<bool *>(&object[1]) = false;
}