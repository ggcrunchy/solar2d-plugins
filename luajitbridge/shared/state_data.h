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

//
//
//

#include "functions.h"
#include "interface.h"

//
//
//

struct BoxedGCObject;
struct State;

//
//
//

struct StateData {
	StateData (const Interface * ifx, void * state);
	
	/* ----- */
	
	const Interface * mIFX;
	StateData * mMain;
	State * mState;
	
	/* ----- */
	
	void CopyStringTo (StateData * dst, int index);
	const BoxedGCObject * GetBoxedGCObject (int index);
	void PushGCObject (StateData * dst, int index);
	void PushToOtherStack (StateData * dst, int n);
	void PushShimFunction (const char * name, size_t len);
	int BridgeCall (StateData * dst, int nargs, int nrets);
	int ForwardError (StateData * dst);
	bool LoadCode (void);
	bool InstallPrelude (void);
	void AddMetatable (void);
	
	/* ----- */
	
	bool IsMain (void) const;
	bool Owns (const BoxedGCObject * obj) const;
	
	/* ----- */
	
	#define SEP
	
	#define RFUNC_0(RET, RT, NAME, _1) inline RT NAME (void) \
	{ \
		RET mIFX->NAME(mState); \
	}
	
	#define RFUNC_1(RET, RT, NAME, _1, T, A) inline RT NAME (T A) \
	{ \
		RET mIFX->NAME(mState, A); \
	}
	
	#define SFUNC_1(RET, RT, NAME, _1, T, A) inline RT NAME (T A) \
	{ \
		RET mIFX->NAME(mState, this, A); \
	}
	
	#define BFUNC_0 RFUNC_0
	
	#define BFUNC_1 RFUNC_1
	
	#define BFUNC_2(RET, RT, NAME, _1, T1, A1, T2, A2) inline RT NAME (T1 A1, T2 A2) \
	{ \
		RET mIFX->NAME(mState, A1, A2); \
	}
	
	#define BFUNC_3(RET, RT, NAME, _1, T1, A1, T2, A2, T3, A3) inline RT NAME (T1 A1, T2 A2, T3 A3) \
	{ \
		RET mIFX->NAME(mState, A1, A2, A3); \
	}
	
	/* ----- */
	
	WITH_FUNCTIONS_DO();
	
	/* ----- */
	
	#undef SEP
	#undef RFUNC_0
	#undef RFUNC_1
	#undef SFUNC_1
	#undef BFUNC_0
	#undef BFUNC_1
	#undef BFUNC_2
	#undef BFUNC_3
	
	/* ----- */

	template<typename T> inline T * Make (void)
	{
		return (T *)NewUserdata(sizeof(T));
	}
};

//
//
//

struct StateDataLJ : StateData {
	StateDataLJ (const Interface * ifx, void * state, StateData * main) : StateData(ifx, state)
	{
		mMain = main;
	}
	
	/* ----- */
	
	void Close (void);

	/* ----- */
	
	static void * Create (void);
	static void New (StateData * main);
};

//
//
//

StateData MakeLuaStateData (void * state);
StateDataLJ MakeLJStateData (void * state, StateData * main);

//
//
//

void PushWeakValuedData (StateData * main, StateData * ptr);

//
//
//

struct BoxedGCObject {
	StateData * mOwner;
	int mObjectRef;
	int mOwnerRef;

	/* ----- */
	
	void DropRefs (void);
	void ResolveOwner (void);

	/* ----- */
	
	static BoxedGCObject * New (StateData * dst);
};
