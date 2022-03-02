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

#include "Depth.h"

DepthEnvironment * InitDepthEnvironment( lua_State * L )
{
	static int sCookie;

	auto env = GetOrNew< DepthEnvironment >( L, &sCookie, true );

	if (env.isNew)
	{
        CoronaCommand command = {};
        
		command.reader = []( const CoronaCommandBuffer *, const unsigned char * data, unsigned int size ) {
            double clear;

            assert( size >= sizeof( double ) );

            memcpy( &clear, data, sizeof( double ) );

        #ifdef OPENGLES
            glClearDepthf
        #else
            glClearDepth
        #endif
            ( clear ); // TODO: is this expensive? if so, avoid when possible...
            glClear( GL_DEPTH_BUFFER_BIT );
		};

		CoronaRendererRegisterCommand( L, &command, &env.object->commandID );
	}

	return env.object;
}
