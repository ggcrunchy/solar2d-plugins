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

#define YOCTO_VECTOR1F_NAME "shapes.yocto.vector1f"

//
//
//

std::vector<float> & GetVector1f (lua_State * L, int arg)
{
	return BoxOrVector<float>::Get(L, arg, YOCTO_VECTOR1F_NAME);
}

//
//
//

static int AddMethods (lua_State * L)
{
	LuaXS::AttachMethods(L, YOCTO_VECTOR1F_NAME, [](lua_State * L) {
		luaL_Reg funcs[] = {
			{
				"append", AppendToVector<float, &GetVector1f>
			}, {
				"__gc", LuaXS::TypedGC<BoxOrVector<float>>
			}, {
				"get", GetValue<float, &GetVector1f>
			}, {
				"__len", GetLength<float, &GetVector1f>
			}, {
				"make_heightfield", [](lua_State * L)
				{
					return WrapShapeData(L, make_heightfield(LuaXS::GetArg<yocto::vec2i>(L, 2), GetVector1f(L))); // heights, size, field
				}
			}, {
				"update", AppendToVector<float, &GetVector1f>
			},
			{ nullptr, nullptr }
		};

		luaL_register(L, nullptr, funcs);

		SetComponentCount(L, 1);
	});

	return 1;
}

//
//
//

int WrapVector1f (lua_State * L, std::vector<float> && sd)
{
	LuaXS::NewTyped<BoxOrVector<float>>(L)->mVec = std::move(sd); // ..., shape_data
	
	return AddMethods(L);
}

//
//
//

int RefVector1f (lua_State * L, std::vector<float> * v, int from)
{
	LuaXS::NewTyped<BoxOrVector<float>>(L, L, from)->mVecPtr = v; // ..., source, ..., shape_data

	return AddMethods(L);
}