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

#include "CoronaLua.h"
#include "CoronaMacros.h"
#include "CoronaPublicTypes.h"
#include "CoronaGraphics.h"
#include "CoronaObjects.h"
#include <cassert>
#include <cstring>

#ifdef __APPLE__
    #include "TargetConditionals.h"

    #if TARGET_IPHONE
        #include <OpenGLES/ES2/gl.h>
        #include <OpenGLES/ES2/glext.h>

        #define OPENGLES
    #else
        #include <OpenGL/gl.h>
        #include <OpenGL/glext.h>
    #endif
#elif defined(_WIN32) // TODO: or Switch...
    #define WIN32_LEAN_AND_MEAN

    #include <GL/glew.h>
#else
    #include <GLES2/gl2.h>
    #include <GLES2/gl2ext.h>

    #define OPENGLES
#endif

void EarlyOutPredicate( const CoronaDisplayObject *, void *, int * result );
void AddToParamsList( CoronaObjectParamsHeader & head, CoronaObjectParamsHeader * params, unsigned short method );
void DisableCullAndHitTest( CoronaObjectParamsHeader & head );
void DummyArgs( lua_State * L );
int FindName( lua_State * L, int valueIndex, const char * list[] );
bool FindFunc( lua_State * L, int valueIndex, int * func );

void AddUtils( lua_State * L );

struct ScopeMessagePayload {
	const CoronaRenderer * renderer;
	unsigned int drawSessionID;
};

template<typename T> struct Boxed {
	T * object;
	bool isNew;
};

template<typename T> Boxed< T >
GetOrNew( lua_State * L, void * cookie, bool construct = false )
{
	Boxed< T > boxed;

	lua_pushlightuserdata( L, cookie ); // ..., cookie
	lua_rawget( L, LUA_REGISTRYINDEX ); // ..., object?

	if (!lua_isnil( L, -1 ))
	{
		boxed.object = (T *)lua_touserdata( L, -1 );
		boxed.isNew = false;
	}

	else
	{
		lua_pushlightuserdata( L, cookie ); // ..., nil, cookie

		boxed.object = (T *)lua_newuserdata( L, sizeof( T ) ); // ..., nil, cookie, object
		boxed.isNew = true;

		if (construct)
		{
			new ( boxed.object ) T;
		}

		else
		{
			memset( boxed.object, 0, sizeof( T ) );
		}

		lua_rawset( L, LUA_REGISTRYINDEX ); // ..., nil; registry = { ..., [cookie] = object }
	}

	lua_pop( L, 1 ); // ...

	return boxed;
}
