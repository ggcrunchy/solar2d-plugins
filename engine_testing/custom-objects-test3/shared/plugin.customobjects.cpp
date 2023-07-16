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

static void WriteInstanceValue ( void * dest, const void * context, const CoronaGeometryMappingLayout * layout, unsigned int, unsigned int n )
{
    unsigned char * out = static_cast< unsigned char * >( dest );

    for ( unsigned int i = 0; i < n; ++i )
    {
        memcpy( out, context, sizeof( float ) );

        out += layout->stride;
    }
}

static float sInstance;

bool AddInstanceWriter( const CoronaRenderer * renderer, const char * name )
{
    return CoronaGeometrySetComponentWriter( renderer, name, WriteInstanceValue, &sInstance, false ) != 0;
}

void MultiDraw( const CoronaShader * shader, const CoronaRenderer * renderer, const CoronaRenderData * renderData, BasicInstancingData * instancingData )
{
    if (!instancingData->name) return;

    for (int i = 0; i < instancingData->count; ++i)
    {
        sInstance = float( i );
  
        CoronaShaderRawDraw( shader, renderData, renderer );
    }
}

CORONA_EXPORT int luaopen_plugin_customobjects3( lua_State * L )
{
    lua_newtable(L);// customobjects3

    AddBasic( L );
    AddShared( L );
    AddUnshared( L );

    return 1;
}
