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
#include "common.h"
#include "functions.h"
#include "interface.h"
#include "trampoline.h"

//
//
//

struct State;

//
//
//

static inline lua_State * Cast (State * state)
{
	return (lua_State *)state;
}

//
//
//

static void luaL_pushcfunc (lua_State * L, StateData * data, CFunc func)
{
	using LT = Trampoline<lua_State, &lua_pushcclosure, &lua_touserdata>;

	LT::Push(L, data, func);
}

//
//
//

#define SEP ,

#define RFUNC_0(RET, _1, _2, FNAME) [](State * state) { \
	RET lua##FNAME(Cast(state), kREGISTRYINDEX); \
}

#define RFUNC_1(RET, _1, _2, FNAME, T, A) [](State * state, T A) { \
	RET lua##FNAME(Cast(state), kREGISTRYINDEX, A); \
}

#define SFUNC_1(RET, _1, _2, FNAME, AT, A0) [](State * state, StateData * self, AT A0) \
{ \
	RET lua##FNAME(Cast(state), self, A0); \
}

#define BFUNC_0(RET, _1, _2, FNAME) [](State * state) { \
	RET lua##FNAME(Cast(state)); \
}

#define BFUNC_1(RET, _1, _2, FNAME, T, A) [](State * state, T A) { \
	RET lua##FNAME(Cast(state), A); \
}

#define BFUNC_2(RET, _1, _2, FNAME, AT1, A1, AT2, A2) [](State * state, AT1 A1, AT2 A2) { \
	RET lua##FNAME(Cast(state), A1, A2); \
}

#define BFUNC_3(RET, _1, _2, FNAME, AT1, A1, AT2, A2, AT3, A3) [](State * state, AT1 A1, AT2 A2, AT3 A3) { \
	RET lua##FNAME(Cast(state), A1, A2, A3); \
}

//
//
//

const Interface InterfaceLua = {
	WITH_FUNCTIONS_DO()
};

//
//
//

#undef SEP
#undef RFUNC_0
#undef RFUNC_1
#undef SFUNC_1
#undef BFUNC_0
#undef BFUNC_1
#undef BFUNC_2
#undef BFUNC_3

//
//
//

StateData MakeLuaStateData (void * state)
{
	return StateData(&InterfaceLua, state);
}

//
//
//

void PushWeakValuedData (StateData * main, StateData * ptr)
{
	main->PushLightUserdata(KEY_GETTER(WeakValuedLookup)()); // ..., wvl_key
	main->RawGet(LUA_REGISTRYINDEX); // ..., weak_valued_lookup
	main->PushLightUserdata(ptr); // ..., weak_valued_lookup, ptr
	main->RawGet(-2); // ..., weak_valued_lookup, strong
	main->Remove(-2); // ..., strong
}
