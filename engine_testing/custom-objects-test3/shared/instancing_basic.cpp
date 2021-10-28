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

static int
GetDataIndex( const char * name )
{
    if (strcmp( name, "instanceCount" ) == 0)
    {
        return 0;
    }

    return -1;
}

static int
GetData( lua_State * L, int dataIndex, void * userData, int * hadError )
{
    if (0 == dataIndex)
    {
        BasicInstancingData * _this = static_cast< BasicInstancingData * >( userData );

        lua_pushinteger( L, _this->count ); // ..., count

        return 1;
    }

    else
    {
        lua_pushfstring( L, "getData given bad index: %i", dataIndex ); // ..., err

        *hadError = true;

        return 1;
    }
}

static int
SetData( lua_State * L, int dataIndex, int valueIndex, void * userData, int *, int * hadError )
{
    int instanceCount = lua_tointeger( L, valueIndex );

    if (0 == dataIndex && instanceCount >= 0)
    {
        BasicInstancingData * _this = static_cast< BasicInstancingData * >( userData );

        _this->count = instanceCount;

        return 1;
    }

    else
    {
        if (dataIndex)
        {
            lua_pushfstring( L, "setData given bad index: %i", dataIndex ); // ..., err
        }

        else
        {
            lua_pushfstring( L, "Invalid instance count: %i", instanceCount ); // ..., err
        }

        *hadError = true;

        return 1;
    }
}

static CoronaShaderDrawParams
DrawParams()
{
    CoronaShaderDrawParams drawParams = {};

    drawParams.ignoreOriginal = true;
    drawParams.after = []( const CoronaShader * shader, void * userData, const CoronaRenderer * renderer, const CoronaRenderData * renderData )
    {
        BasicInstancingData * _this = static_cast< BasicInstancingData * >( userData );
        
        MultiDraw( shader, renderer, renderData, _this );
    };

    return drawParams;
}

static void
Prepare( const CoronaShader * shader, void * userData, const CoronaRenderData * renderData, int w, int h, int mod )
{
    BasicInstancingData * _this = static_cast< BasicInstancingData * >( userData );

    _this->out = NULL;

    CoronaEffectDetail detail;
        
    for (int i = 0; CoronaShaderGetEffectDetail( shader, i, &detail ); ++i)
    {
        if (strcmp( detail.name, "supportsInstancing" ) == 0)
        {
            _this->out = CoronaGeometryGetMappingFromRenderData( renderData, detail.value, &_this->dstLayout );
        }
    }
}

void AddBasic( lua_State * L)
{
    luaL_Reg funcs[] = {
        {
            "registerBasicInstancingDataType", [](lua_State * L)
            {
                CoronaEffectCallbacks callbacks = {};

                callbacks.size = sizeof( CoronaEffectCallbacks );
                callbacks.extraSpace = sizeof( BasicInstancingData );
                callbacks.getDataIndex = GetDataIndex;
                callbacks.getData = GetData;
                callbacks.setData = SetData;
                callbacks.drawParams = DrawParams();
                callbacks.prepare = Prepare;

                lua_pushboolean( L, CoronaShaderRegisterEffectDataType( L, "basicInstances", &callbacks ) ); // ..., ok?
                
                return 1;
            }
        },
        { NULL, NULL }
    };
    
    luaL_register( L, NULL, funcs );
}
