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
#include "interface.h"
#include "state_data.h"
#include "trampoline.h"
#include "luajit_include/lua.hpp"

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

void StateDataLJ::Close (void)
{
	lua_State * L = Cast(mState);

	lj2_close(L);
}

//
//
//

void * StateDataLJ::Create (void)
{
	lua_State * L = lj2L_newstate();
	
	lj2L_openlibs(L);

	return L;
}

//
//
//

static void lj2L_pushcfunc (lua_State * L, StateData * data, CFunc func)
{
	using LT = Trampoline<lua_State, &lj2_pushcclosure, &lj2_touserdata>;

	LT::Push(L, data, func);
}

//
//
//

#define SEP ,

#define RFUNC_0(RET, _1, _2, FNAME) [](State * state) \
{ \
	RET lj2##FNAME(Cast(state), kREGISTRYINDEX); \
}

#define RFUNC_1(RET, _1, _2, FNAME, AT, A0) [](State * state, AT A0) \
{ \
	RET lj2##FNAME(Cast(state), kREGISTRYINDEX, A0); \
}

#define SFUNC_1(RET, _1, _2, FNAME, AT, A0) [](State * state, StateData * self, AT A0) \
{ \
	RET lj2##FNAME(Cast(state), self, A0); \
}

#define BFUNC_0(RET, _1, _2, FNAME) [](State * state) \
{ \
	RET lj2##FNAME(Cast(state)); \
}

#define BFUNC_1(RET, _1, _2, FNAME, AT, A0) [](State * state, AT A0) \
{ \
	RET lj2##FNAME(Cast(state), A0); \
}

#define BFUNC_2(RET, _1, _2, FNAME, AT1, A1, AT2, A2) [](State * state, AT1 A1, AT2 A2) \
{ \
	RET lj2##FNAME(Cast(state), A1, A2); \
}

#define BFUNC_3(RET, _1, _2, FNAME, AT1, A1, AT2, A2, AT3, A3) [](State * state, AT1 A1, AT2 A2, AT3 A3) \
{ \
	RET lj2##FNAME(Cast(state), A1, A2, A3); \
}

//
//
//

#define lj2_pop lua_pop
#define lj2L_argcheck luaL_argcheck

//
//
//

const Interface InterfaceLJ = {
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

StateDataLJ MakeLJStateData (void * state, StateData * main)
{
	return StateDataLJ(&InterfaceLJ, state, main);
}
