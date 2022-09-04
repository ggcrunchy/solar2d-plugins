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
#include "ByteReader.h"
#include "soloud_fft.h"

//
//
//

static int AuxValues (lua_State * L)
{
	FloatBuffer * buffer = GetFloatBuffer(L);
	size_t pos = luaL_checkint(L, 2) + 1;
					
	if (pos >= 1 && pos <= buffer->mSize)
	{
		lua_pushinteger(L, pos); // buffer, pos, pos + 1
		lua_pushnumber(L, buffer->mData[pos - 1]); // buffer, pos, pos + 1, value

		return 2;
	}

	else return 0;
}

//
//
//

#define N_VALUES_MERGE(pos, step) (((pos) & 0x00FFFFFFF) << 8) | (step & 0xFF)
#define N_VALUES_GET_POS(merged) merged >> 8
#define N_VALUES_GET_STEP(merged) merged & 0xFF

static int AuxNValues (lua_State * L)
{
	FloatBuffer * buffer = GetFloatBuffer(L);
	size_t pos, step;

	if (luaL_checknumber(L, 2) < 0) // first value?
	{
		pos = 0;
		step = size_t(-lua_tonumber(L, 2));
	}

	else
	{
		size_t merged = lua_tointeger(L, 2);

		pos = N_VALUES_GET_POS(merged);
		step = N_VALUES_GET_STEP(merged);
	}
					
	if (pos >= 0 && pos < buffer->mSize)
	{
		size_t merged = N_VALUES_MERGE(pos + step, step);

		lua_pushinteger(L, merged); // buffer, pos, merged

		if (pos + step > buffer->mSize) step = buffer->mSize - pos; // avoid going over

		for (size_t i = 0; i < step; ++i)
		{
			lua_pushnumber(L, buffer->mData[pos + i]); // buffer, pos, merged, ..., value
		}

		return int(1 + step);
	}

	else return 0;
}

//
//
//

FloatBuffer * GetFloatBuffer (lua_State * L, int arg)
{
	FloatBuffer * buffer = LuaXS::CheckUD<FloatBuffer>(L, arg, MT_NAME(FloatBuffer));

	luaL_argcheck(L, !buffer->mSourceGone, arg, "Buffer source has disappeared");

	return buffer;
}

FloatBuffer * NewFloatBuffer (lua_State * L)
{
	FloatBuffer * buffer = LuaXS::NewTyped<FloatBuffer>(L); // buffer

	LuaXS::AttachMethods(L, MT_NAME(FloatBuffer), [](lua_State * L) {
		luaL_Reg funcs[] = {
			{
				"dup", [](lua_State * L)
				{
					FloatBuffer * buffer = GetFloatBuffer(L);
					FloatBuffer * dup = NewFloatBuffer(L); // buffer, dup

					dup->mData = new float [buffer->mSize];
					dup->mSize = buffer->mSize;

					memcpy(dup->mData, buffer->mData, buffer->mSize * sizeof(float));

					return 1;
				}
			}, {
				"fft", [](lua_State * L)
				{
					FloatBuffer * buffer = GetFloatBuffer(L);

					luaL_argcheck(L, buffer->mOwnsData, 1, "Attempt to fft() unowned data");

					SoLoud::FFT::fft(buffer->mData, buffer->mSize);

					return 0;
				}
			}, {
				"fft256", [](lua_State * L)
				{
					FloatBuffer * buffer = GetFloatBuffer(L);
					
					luaL_argcheck(L, buffer->mSize >= 256, 1, "Too little data for fft256()");
					luaL_argcheck(L, buffer->mOwnsData, 1, "Attempt to fft256() unowned data");

					SoLoud::FFT::fft256(buffer->mData);

					return 0;
				}
			}, {
				"fft1024", [](lua_State * L)
				{
					
					FloatBuffer * buffer = GetFloatBuffer(L);
					
					luaL_argcheck(L, buffer->mSize >= 1024, 1, "Too little data for fft1024()");
					luaL_argcheck(L, buffer->mOwnsData, 1, "Attempt to fft1024() unowned data");

					SoLoud::FFT::fft1024(buffer->mData);

					return 0;
				}
			}, {
				"__gc", LuaXS::TypedGC<FloatBuffer>
			}, {
				"getAt", [](lua_State * L)
				{
					FloatBuffer * buffer = GetFloatBuffer(L);
					size_t pos = luaL_checkint(L, 2);
					
					if (pos >= 1 && pos <= buffer->mSize) lua_pushnumber(L, buffer->mData[pos - 1]); // buffer, pos, value
					else lua_pushnumber(L, 0); // buffer, pos, 0

					return 1;
				}
			}, {
				"ifft", [](lua_State * L)
				{
					FloatBuffer * buffer = GetFloatBuffer(L);

					luaL_argcheck(L, buffer->mOwnsData, 1, "Attempt to ifft() unowned data");

					SoLoud::FFT::ifft(buffer->mData, buffer->mSize);

					return 0;
				}
			}, {
				"ifft256", [](lua_State * L)
				{
					FloatBuffer * buffer = GetFloatBuffer(L);
					
					luaL_argcheck(L, buffer->mSize >= 256, 1, "Too little data for ifft256()");
					luaL_argcheck(L, buffer->mOwnsData, 1, "Attempt to ifft256() unowned data");

					SoLoud::FFT::ifft256(buffer->mData);

					return 0;
				}
			}, {
				"__len", [](lua_State * L)
				{
					lua_pushinteger(L, GetFloatBuffer(L)->mSize); // buffer, size

					return 1;
				}
			}, {
				"setAt", [](lua_State * L)
				{
					FloatBuffer * buffer = GetFloatBuffer(L);
					size_t pos = luaL_checkint(L, 2);
					float v = LuaXS::Float(L, 3);
					
					luaL_argcheck(L, pos >= 1 && pos <= buffer->mSize, 2, "Invalid index to setAt()");
					luaL_argcheck(L, buffer->mOwnsData || buffer->mCanWrite, 1, "Attempt to setAt() unowned data");

					buffer->mData[pos - 1] = v;

					return 0;
				}
			}, {
				"values", [](lua_State * L)
				{
					int step = luaL_optinteger(L, 2, 0);

					luaL_argcheck(L, step >= 0 && step < 16, 2, "Step must be >= 0 and less than 16");
					lua_settop(L, 1); // buffer

					FloatBuffer * buffer = GetFloatBuffer(L);

					if (0 == step)
					{					
						lua_pushcfunction(L, AuxValues); // buffer, AuxValues
						lua_insert(L, 1); // AuxValues, buffer
						lua_pushinteger(L, 0); // AuxValues, buffer, 0
					}

					else
					{
						lua_pushcfunction(L, AuxNValues); // buffer, AuxNValues
						lua_insert(L, 1); // AuxNValues, buffer
						lua_pushnumber(L, -step); // AuxValues, buffer, step
					}

					return 3;
				}
			}, {
				"zero", [](lua_State * L)
				{
					FloatBuffer * buffer = GetFloatBuffer(L);

					luaL_argcheck(L, buffer->mOwnsData, 1, "Attempt to zero() unowned data");

					memset(buffer->mData, 0, buffer->mSize * sizeof(float));

					return 0;
				}
			},
			{ nullptr, nullptr }
		};

		luaL_register(L, nullptr, funcs);

		//
		//
		//
            
        ByteReaderFunc * func = ByteReader::Register(L);
            
        func->mGetBytes = [](lua_State * L, ByteReader & reader, int arg, void *) {
            FloatBuffer * buffer = GetFloatBuffer(L, arg);
                
            if (buffer->mData)
            {	
				reader.mBytes = buffer->mData;
				reader.mCount = buffer->mSize * sizeof(float);
                
				return true;
			}

			else
			{
                reader.mBytes = nullptr;
                reader.mCount = 0U;
                    
                return false;
            }
        };
            
        lua_pushlightuserdata(L, func); // mt, func
        lua_setfield(L, -2, "__bytes"); // mt = { ..., __bytes = func }
	});

	return buffer;
}

//
//
//

void add_floatbuffer (lua_State * L)
{
	lua_pushcfunction(L, [](lua_State * L) {
		size_t size = LuaXS::Uint(L, 1);

		luaL_argcheck(L, size > 0, 1, "Invalid size");

		FloatBuffer * buffer = NewFloatBuffer(L); // size, buffer

		buffer->mData = new float[size];
		buffer->mSize = size;

		return 1;
	}); // soloud, CreateFloatBuffer
	lua_setfield(L, -2, "createFloatBuffer"); // soloud = { ..., createFloatBuffer = CreateFloatBuffer }
}