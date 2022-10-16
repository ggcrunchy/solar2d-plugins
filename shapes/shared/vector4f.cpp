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

#include "common.h"
#include "yocto.h"

//
//
//

#define YOCTO_VECTOR4F_NAME "shapes.yocto.vector4f"

//
//
//

std::vector<yocto::vec4f> & GetVector4f (lua_State * L, int arg)
{
	return BoxOrVector<yocto::vec4f>::Get(L, arg, YOCTO_VECTOR4F_NAME);
}

//
//
//

static int AddMethods (lua_State * L)
{
	LuaXS::AttachMethods(L, YOCTO_VECTOR4F_NAME, [](lua_State * L) {
		luaL_Reg funcs[] = {
			{
				"append", AppendToVector<yocto::vec4f, &GetVector4f>
			}, {
				"__gc", LuaXS::TypedGC<BoxOrVector<yocto::vec4f>>
			}, {
				"make_heightfield", [](lua_State * L)
				{
					return WrapShapeData(L, make_heightfield(LuaXS::GetArg<yocto::vec2i>(L, 2), GetVector4f(L))); // colors, size, field
				}
			}, {
				"update", AppendToVector<yocto::vec4f, &GetVector4f>
			},
			{ nullptr, nullptr }
		};

		luaL_register(L, nullptr, funcs);

		SetComponentCount(L, 4);
	});

	return 1;
}

//
//
//

int WrapVector4f (lua_State * L, std::vector<yocto::vec4f> && sd)
{
	LuaXS::NewTyped<BoxOrVector<yocto::vec4f>>(L)->mVec = std::move(sd); // ..., shape_data
	
	return AddMethods(L);
}

//
//
//

int RefVector4f (lua_State * L, std::vector<yocto::vec4f> * v, int from)
{
	LuaXS::NewTyped<BoxOrVector<yocto::vec4f>>(L, L, from)->mVecPtr = v; // ..., source, ..., shape_data

	return AddMethods(L);
}