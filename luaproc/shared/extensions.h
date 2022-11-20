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

#ifndef EXTENSIONS_H
#define EXTENSIONS_H

#include "CoronaLua.h"
#include "CoronaMacros.h"

// Variables
CORONA_EXTERN_C void AddInteger (lua_State * L, lua_Integer i);
CORONA_EXTERN_C void AddNumber (lua_State * L, lua_Number n);
CORONA_EXTERN_C int GetInteger (const char * name, lua_Integer * i);
CORONA_EXTERN_C int GetNumber (const char * name, lua_Number * n);
CORONA_EXTERN_C void RemoveInteger (const char * name);
CORONA_EXTERN_C void RemoveNumber (const char * name);
CORONA_EXTERN_C int UpdateInteger (const char * name, int op, lua_Integer n, lua_Integer * result);
CORONA_EXTERN_C int UpdateNumber (const char * name, int op, lua_Number n, lua_Number * result);

enum { eUpdateAssign, eUpdateAdd, eUpdateSub, eUpdateAnd, eUpdateOr, eUpdateXor };

// Events
CORONA_EXTERN_C int CreateEventX (lua_State * L, int manual_reset, int initial_state);
CORONA_EXTERN_C int DestroyEventX (const char * name);
CORONA_EXTERN_C int SetEventX (const char * name);
CORONA_EXTERN_C int ResetEventX (const char * name);
CORONA_EXTERN_C int WaitForEventX (const char * name, uint64_t ms);
CORONA_EXTERN_C int WaitForMultipleEventsX (const char * names[], uint64_t ms, int all, int * index);

enum { eEventOK, eEventTimeout, eEventError };

// Plugin support
CORONA_EXTERN_C void InitExtensions (lua_State * L);
CORONA_EXTERN_C int AddFunction (const char * name, lua_CFunction func, void * payload);
CORONA_EXTERN_C int PopFunction (const char * name, lua_CFunction * func, void ** payload);

// Miscellaneous
CORONA_EXTERN_C unsigned int EstimateConcurrency (void);
CORONA_EXTERN_C void SleepFor (unsigned int ms);

CORONA_EXTERN_C void ExtDestructors (void);

#endif