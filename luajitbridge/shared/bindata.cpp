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
#include "state_data.h"

//
//
//

#define INCBIN_PREFIX
#define INCBIN_SILENCE_BITCODE_WARNING

#include "incbin.h"

//
//
//

#ifdef _MSC_VER
	INCBIN(Prelude, "prelude.lua");
#else
	INCTXT(Prelude, "prelude.lua");
#endif

//
//
//

#ifdef _MSC_VER
	#include "data.c"

	//
	//
	//

	bool StateData::LoadCode (void)
	{
		PushLstring((const char *)PreludeData, PreludeSize); // ..., code
		
		const char * code = ToLstring(-1, nullptr);
		bool ok = LoadString(code) == 0; // ..., code, func / error

		Remove(-2); // ..., func / error
		
		return ok;
	}
#else
	bool StateData::LoadCode (void)
	{
		return LoadString(PreludeData) == 0; // ..., func / err
	}
#endif
