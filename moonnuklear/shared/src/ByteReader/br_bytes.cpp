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
#include "ByteReader.h"

CORONA_EXTERN_C {
	#include "ByteReader/br_bytes.h"
}

//
//
//

CORONA_EXTERN_C void * BytesNew (lua_State * L, size_t size)
{
	void * bytes = lua_newuserdata(L, size); // ..., bytes

	if (luaL_newmetatable(L, "moonnuklear.bytes")) // ..., bytes, bytes_mt
	{
		ByteReaderFunc * func = ByteReader::Register(L); // ..., bytes, bytes_mt

		func->mGetBytes = [](lua_State * L, ByteReader & reader, int arg, void *)
		{
			reader.mBytes = lua_touserdata(L, arg);
			reader.mCount = lua_objlen(L, arg);

			return false;
		};
		func->mContext = nullptr;

		lua_pushlightuserdata(L, func); // ..., bytes, bytes_mt, reader
		lua_setfield(L, -2, "__bytes"); // ..., bytes, bytes_mt = { ..., __bytes = reader }
	}

	lua_setmetatable(L, -2); // ..., bytes; bytes.metatable = bytes_mt

	return bytes;
}