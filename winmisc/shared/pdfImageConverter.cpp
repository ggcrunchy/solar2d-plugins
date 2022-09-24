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

//
//
//

void AddPDFImageConverter (lua_State * L)
{
	lua_newtable(L); // ..., winmisc, pdfImageConverter

	luaL_Reg funcs[] = {
		{
			"combinePdf", [](lua_State * L)
			{
				// pdf 1, pdf 2, output pdf
				return 0;
			}
		},
		{
			"getPageTotal", [](lua_State * L)
			{
				// pdf path
				return 0;
			}
		},
		{
			"toImage", [](lua_State * L)
			{
				// pdf path, page number (def 0), output image
				return 0;
			}
		},
		{
			"toPdf", [](lua_State * L)
			{
				// image path, output pdf
				return 0;
			}
		},
		{ nullptr, nullptr }
	};

	luaL_register(L, nullptr, funcs);
	lua_setfield(L, -2, "pdfImageConverter"); // ..., winmisc = { ..., pdfImageConverter = pdfImageConverter }
}