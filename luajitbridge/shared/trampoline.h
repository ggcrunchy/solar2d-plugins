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

#include "state_data.h"

//
//
//

template<
	typename T,
	void (*PushCClosure)(T *, int (*)(T *), int),
	void * (*ToUserdata)(T *, int)
> struct Trampoline {
	struct CFuncBox { CFunc func; };

	/* ----- */

	// cf. lua.h:

	static const int kGLOBALSINDEX = -10002;

	#define UPVALUE(i) (kGLOBALSINDEX - (i))

	/* ----- */

	static int Shell (T * L)
	{
		StateData * self = (StateData *)ToUserdata(L, UPVALUE(1)), temp(*self);

		/* ----- */

		// During collection, L and self->mState seem to diverge. Mindful of
		// this, assume L to be correct there, but leave the state intact.
		
		if (self->mState != (State *)L)
		{
			self = &temp;
			self->mState = (State *)L;
		} 

		/* ----- */

		CFuncBox * fbox = (CFuncBox *)self->ToUserdata(UPVALUE(2));

		return fbox->func(self);
	}

	/* ----- */

	static void Push (T * L, StateData * self, CFunc func)
	{
		self->PushLightUserdata(self); // ..., self
		self->Make<CFuncBox>()->func = func; // ..., self, box

		PushCClosure(L, Shell, 2); // ..., Shell
	}
};
