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

#if defined(EIGEN_CORE)
	#define PLUGIN_SUFFIX eigencore
	#define PLUGIN_NAME luaopen_plugin_eigencore
	#define WANT_MAP
#elif defined(EIGEN_INT_ONLY)
	#define PLUGIN_SUFFIX eigenint
	#define PLUGIN_NAME luaopen_plugin_eigenint

	#define WANT_INT
	#define WANT_MAP
#elif defined(EIGEN_FLOAT_ONLY)
	#define PLUGIN_SUFFIX eigenfloat
	#define PLUGIN_NAME luaopen_plugin_eigenfloat

	#define WANT_FLOAT
	#define WANT_MAP
#elif defined(EIGEN_DOUBLE_ONLY)
	#define PLUGIN_SUFFIX eigendouble
	#define PLUGIN_NAME luaopen_plugin_eigendouble

	#define WANT_DOUBLE
	#define WANT_MAP
#elif defined(EIGEN_CFLOAT_ONLY)
	#define PLUGIN_SUFFIX eigencfloat
	#define PLUGIN_NAME luaopen_plugin_eigencfloat

	#define WANT_CFLOAT
	#define WANT_MAP
#elif defined(EIGEN_CDOUBLE_ONLY)
	#define PLUGIN_SUFFIX eigencdouble
	#define PLUGIN_NAME luaopen_plugin_eigencdouble

	#define WANT_CDOUBLE
	#define WANT_MAP
#else
	#define PLUGIN_SUFFIX eigen
	#define PLUGIN_NAME luaopen_plugin_eigen

	#define EIGEN_PLUGIN_BASIC
	#define WANT_INT
	#define WANT_FLOAT
	#define WANT_DOUBLE
	#define WANT_CFLOAT
	#define WANT_CDOUBLE
#endif
