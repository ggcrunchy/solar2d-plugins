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

#define WITH_ALIGNMENT(align, action)	case align:	{			\
										const size_t VA = align;\
										action } break
#define WITH_ALIGNMENT_DO(align, action)	switch (align)					\
											{								\
											WITH_ALIGNMENT(0U, action);		\
											WITH_ALIGNMENT(4U, action);		\
											WITH_ALIGNMENT(8U, action);		\
											WITH_ALIGNMENT(16U, action);	\
											WITH_ALIGNMENT(32U, action);	\
											WITH_ALIGNMENT(64U, action);	\
											WITH_ALIGNMENT(128U, action);	\
											WITH_ALIGNMENT(256U, action);	\
											WITH_ALIGNMENT(512U, action);	\
											WITH_ALIGNMENT(1024U, action);	\
											}