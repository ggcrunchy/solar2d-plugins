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

void MultiDraw( const CoronaShader * shader, const CoronaRenderer * renderer, const CoronaRenderData * renderData, BasicInstancingData * instancingData )
{
    CoronaShaderRawDraw( shader, renderData, renderer ); // first / only instance (could also leave original intact)

    // now do any others
    if (instancingData->out && instancingData->count > 1U)
    {
        CoronaGeometryMappingLayout srcLayout;

        srcLayout.count = 1U;
        srcLayout.offset = 0U;
        srcLayout.stride = 0U;
        srcLayout.type = kAttributeType_Float;
        srcLayout.size = sizeof( float );

        for (size_t i = 1; i < instancingData->count; ++i)
        {
            const float instance = float( i );
  
            CoronaGeometryCopyData( instancingData->out, &instancingData->dstLayout, &instance, &srcLayout ); // update instance index
            CoronaShaderRawDraw( shader, renderData, renderer ); // instance #2 and up
        }

        const float zero = 0.f;

        CoronaGeometryCopyData( instancingData->out, &instancingData->dstLayout, &zero, &srcLayout ); // restore instance index to 0
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
