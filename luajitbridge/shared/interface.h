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

//
//
//

struct StateData;

//
//
//

struct Interface {

	#define SEP ;
	#define RFUNC_0(_1, RT, NAME, _2) RT (*NAME)(State * state)
	#define RFUNC_1(_1, RT, NAME, _2, T, A) RT (*NAME)(State * state, T A)
	#define SFUNC_1(_1, RT, NAME, _2, T, A) RT (*NAME)(State * state, StateData * self, T A)
	#define BFUNC_0 RFUNC_0
	#define BFUNC_1 RFUNC_1
	#define BFUNC_2(_1, RT, NAME, _2, T1, A1, T2, A2) RT (*NAME)(State * state, T1 A1, T2 A2)
	#define BFUNC_3(_1, RT, NAME, _2, T1, A1, T2, A2, T3, A3) RT (*NAME)(State * state, T1 A1, T2 A2, T3 A3)
	
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

};
