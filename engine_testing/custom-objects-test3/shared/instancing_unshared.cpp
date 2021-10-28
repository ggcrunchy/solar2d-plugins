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
#include <vector>

#define UNSHARED_BUFFER_MT_NAME "unshared.memory"

struct UnsharedMemoryResource {
    enum {
        kMaxVectorsPerLoad = 8  // up to 7 instances per load
                                // note: this is FAR fewer than even the minimum guaranteed
                                // count available would provide us, but the sample is meant
                                // to demonstrate how to deal with overflow, and it helps if
                                // we can actually get there in a reasonable time :)
    };
    
    unsigned int bakedVectorsCount{0};
    std::vector<float> payload;
};

struct UnsharedInstancingData {
    lua_State * luaState{NULL};
    UnsharedMemoryResource * resource{NULL};
    float * dataOffset{NULL};
    CoronaGeometryMappingLayout layout;
    BasicInstancingData instances{};
    unsigned long uploadCommand{0};
    bool initialized{false};
    bool shaderBound{false};
};

static UnsharedInstancingData *
GetUnsharedData( void * userData )
{
    UnsharedInstancingData * unsharedInstanceData = static_cast< UnsharedInstancingData * >( userData );
    
    if (!unsharedInstanceData->initialized)
    {
        new (unsharedInstanceData) UnsharedInstancingData;

        unsharedInstanceData->initialized = true;
    }
    
    return unsharedInstanceData;
}

static void
UploadResource( const CoronaRenderer * renderer, void * userData )
{
    UnsharedInstancingData * unsharedInstanceData = GetUnsharedData( userData );
    UnsharedMemoryResource * buffer = unsharedInstanceData->resource;
    size_t nfloats = (buffer->bakedVectorsCount + unsharedInstanceData->instances.count + 1) * 4;

    CoronaRendererIssueCommand( renderer, unsharedInstanceData->uploadCommand, unsharedInstanceData, nfloats * sizeof( float ) );
    
    unsharedInstanceData->dataOffset += unsharedInstanceData->instances.count * 4;
}

static void
CreateUnsharedBuffer( lua_State * L, unsigned long * uploadCommandID )
{
    if (luaL_newmetatable( L, UNSHARED_BUFFER_MT_NAME )) // mt
    {
        // Upload command.
        CoronaCommand uploadCommand = {
            []( const CoronaCommandBuffer * commandBuffer, const unsigned char * from, unsigned int size )
            {
                CoronaWriteUniformParams params = {};
                
                params.u.data = from;
                
                CoronaCommandBufferWriteNamedUniform( commandBuffer, "u_Vectors", &params, size );
            },
            
            []( const CoronaCommandBuffer * commandBuffer, unsigned char * to, const void * data, unsigned int size )
            {
                const UnsharedInstancingData * unsharedInstanceData = static_cast< const UnsharedInstancingData * >( data );
                const UnsharedMemoryResource * buffer = unsharedInstanceData->resource;
                
                unsigned int bakedSize = buffer->bakedVectorsCount * sizeof( float ) * 4;
                
                memcpy( to, buffer->payload.data(), bakedSize );
                memcpy( to + bakedSize, unsharedInstanceData->dataOffset, size - bakedSize );
            }
        };
        
        CoronaRendererRegisterCommand( L, &uploadCommand, uploadCommandID );
        
        // Add some buffer methods.
        lua_pushvalue( L, -1 ); // mt, mt
        lua_setfield( L, -2, "__index" ); // mt = { __index = mt }
        
        luaL_Reg methods[] = {
            {
                "appendVector", []( lua_State * L )
                {
                    UnsharedMemoryResource * buffer = (UnsharedMemoryResource *)luaL_checkudata( L, 1, UNSHARED_BUFFER_MT_NAME );
                    
                    float x = luaL_checknumber( L, 2 );
                    float y = luaL_optnumber( L, 3, 0 );
                    float z = luaL_optnumber( L, 4, 0 );
                    float w = luaL_optnumber( L, 5, 0 );
                        
                    buffer->payload.push_back( x );
                    buffer->payload.push_back( y );
                    buffer->payload.push_back( z );
                    buffer->payload.push_back( w );
                        
                    if (buffer->bakedVectorsCount && buffer->payload.size() / 4 - buffer->bakedVectorsCount > 1)
                    {
                        CoronaRendererInvalidate( L );
                    }
                    
                    return 0;
                }
            },
            {
                "bakeConstants", []( lua_State * L )
                {
                    UnsharedMemoryResource * buffer = (UnsharedMemoryResource *)luaL_checkudata( L, 1, UNSHARED_BUFFER_MT_NAME );
                    
                    if (buffer->bakedVectorsCount)
                    {
                        CoronaLog( "Constants already baked!" );
                    }
                    
                    else
                    {
                        // The baked part is always uploaded, versus the subsequent details that
                        // get sent along piecemeal.
                        buffer->bakedVectorsCount = buffer->payload.size() / 4;
                    }
                    
                    return 0;
                }
            },
            {
                "__gc", [](lua_State * L)
                {
                    UnsharedMemoryResource * buffer = (UnsharedMemoryResource *)luaL_checkudata( L, 1, UNSHARED_BUFFER_MT_NAME );
                    
                    buffer->~UnsharedMemoryResource();
                    
                    return 0;
                }
            },
            { NULL, NULL }
        };
        
        luaL_register( L, NULL, methods );
    }
    
    // Make the new buffer.
    UnsharedMemoryResource * buffer = (UnsharedMemoryResource *)lua_newuserdata( L, sizeof( UnsharedMemoryResource ) ); // mt, buffer

    new (buffer) UnsharedMemoryResource;

    lua_insert( L, -2 ); // buffer, mt
    lua_setmetatable( L, -2 ); // buffer; buffer.metatable = mt
}

static void
UpdateCount( UnsharedInstancingData * unsharedInstancingData, size_t & count )
{
    unsharedInstancingData->instances.count = count;
    
    if (unsharedInstancingData->instances.count >= UnsharedMemoryResource::kMaxVectorsPerLoad)
    {
        unsharedInstancingData->instances.count = UnsharedMemoryResource::kMaxVectorsPerLoad - 1;
    }
    
    count -= unsharedInstancingData->instances.count;
}

static void
DrawN( const CoronaShader * shader, const CoronaRenderer * renderer, const CoronaRenderData * renderData, UnsharedInstancingData * unsharedInstancingData )
{
    MultiDraw( shader, renderer, renderData, &unsharedInstancingData->instances );
}

void AddUnshared( lua_State * L)
{
    luaL_Reg funcs[] = {
        {
            "registerUnsharedDataType", [](lua_State * L)
            {
                CoronaEffectCallbacks callbacks = {};

                callbacks.size = sizeof( CoronaEffectCallbacks );
                callbacks.extraSpace = sizeof( UnsharedInstancingData );
                
                callbacks.getDataIndex = []( const char * name )
                {
                    if (0 == strcmp( name, "buffer" ))
                    {
                        return 0;
                    }
                    
                    return -1;
                };
                
                callbacks.getData = []( lua_State * L, int dataIndex, void * userData, int * hadError )
                {
                    if (0 == dataIndex)
                    {
                        UnsharedInstancingData * unsharedInstanceData = GetUnsharedData( userData );
                        
                        if (!unsharedInstanceData->resource)
                        {
                            CreateUnsharedBuffer( L, &unsharedInstanceData->uploadCommand ); // ..., buffer
                            
                            unsharedInstanceData->luaState = L; // needed when detaching; assumed to be stable
                            unsharedInstanceData->resource = (UnsharedMemoryResource *)luaL_checkudata( L, -1, UNSHARED_BUFFER_MT_NAME );
                            
                            lua_pushlightuserdata( L, userData ); // ..., buffer, key
                            lua_insert( L, -2 ); // ..., key, buffer
                            lua_rawset( L, LUA_REGISTRYINDEX ); // ...; registry = { ..., [key] = buffer }
                        }
                        
                        lua_pushlightuserdata( L, userData ); // key
                        lua_rawget( L, LUA_REGISTRYINDEX ); // resource?
                        
                        return 1;
                    }

                    else
                    {
                        lua_pushfstring( L, "getData given bad index: %i", dataIndex ); // ..., err

                        *hadError = true;

                        return 1;
                    }
                };
                
                callbacks.setData = []( lua_State * L, int dataIndex, int valueIndex, void * userData, int *, int * hadError )
                {
                    lua_pushfstring( L, "setData given bad index: %i", dataIndex ); // ..., err
                    
                    *hadError = true;

                    return 1;
                };
                
                CoronaShaderDrawParams drawParams;
                
                drawParams.ignoreOriginal = true;
                drawParams.before = []( const CoronaShader * shader, void * userData, const CoronaRenderer * renderer, const CoronaRenderData * renderData )
                {
                    UnsharedInstancingData * unsharedInstanceData = GetUnsharedData( userData );
                    UnsharedMemoryResource * buffer = unsharedInstanceData->resource;

                    // Do we have enough content to render?
                    if (!buffer)
                    {
                        return;
                    }
                    
                    size_t vectorCount = buffer->payload.size() / 4 - buffer->bakedVectorsCount;
                    
                    if (vectorCount < 2)
                    {
                        return;
                    }
                    
                    // Set up the first load.
                    unsharedInstanceData->dataOffset = buffer->payload.data() + buffer->bakedVectorsCount * 4;
                    
                    size_t instancesCount = vectorCount - 1;

                    UpdateCount( unsharedInstanceData, instancesCount );

                    // Was the shader already bound, i.e. this is not the first object (in a row) being drawn?
                    if (unsharedInstanceData->shaderBound)
                    {
                        CoronaRendererDo( renderer, UploadResource, userData );
                    }

                    // Draw the first / only instance. If this was the first object with this shader,
                    // a shader bind will happen before it renders.
                    DrawN( shader, renderer, renderData, unsharedInstanceData );

                    // Do any remaining loads.
                    while (instancesCount > 0)
                    {
                        UpdateCount( unsharedInstanceData, instancesCount );
                        CoronaRendererDo( renderer, UploadResource, userData );
                        
                        DrawN( shader, renderer, renderData, unsharedInstanceData );
                    }
                };
                
                callbacks.drawParams = drawParams;
                
                callbacks.prepare = []( const CoronaShader * shader, void * userData, const CoronaRenderData * renderData, int, int, int )
                {
                    UnsharedInstancingData * unsharedInstanceData = GetUnsharedData( userData );

                    unsharedInstanceData->instances.out = NULL;

                    CoronaEffectDetail detail;
                        
                    for (int i = 0; CoronaShaderGetEffectDetail( shader, i, &detail ); ++i)
                    {
                        if (strcmp( detail.name, "supportsInstancing" ) == 0)
                        {
                            unsharedInstanceData->instances.out = CoronaGeometryGetMappingFromRenderData( renderData, detail.value, &unsharedInstanceData->instances.dstLayout );
                        }
                    }
                };
                
                callbacks.shaderBind = []( const CoronaRenderer * renderer, void * userData )
                {
                    UnsharedInstancingData * unsharedInstanceData = GetUnsharedData( userData );
                
                    if (unsharedInstanceData->resource)
                    {
                        UploadResource( renderer, userData ); // n.b. using CoronaRendererDo() here will have odd results :)
                    }
                    
                    unsharedInstanceData->shaderBound = true;
                    
                    // Restore "shader not bound" condition after traversing hierarchy.
                    CoronaRendererOpParams params = {};
                    
                    params.u.renderer = renderer;
                    
                    CoronaRendererScheduleEndFrameOp( &params, []( const CoronaRenderer *, void * userData ) {
                        *(bool *)userData = false;
                    }, &unsharedInstanceData->shaderBound, NULL );
                };
                
                callbacks.shaderDetach = []( const CoronaShader * shader, void * userData )
                {
                    UnsharedInstancingData * unsharedInstanceData = GetUnsharedData( userData );
                    
                    if (unsharedInstanceData->resource)
                    {
                        lua_pushlightuserdata( unsharedInstanceData->luaState, userData ); // ..., key
                        lua_pushnil( unsharedInstanceData->luaState ); // ..., key, nil
                        lua_rawset( unsharedInstanceData->luaState, LUA_REGISTRYINDEX ); // ...; registry = { ..., [key] = nil }
                    }
                };

                lua_pushboolean( L, CoronaShaderRegisterEffectDataType( L, "unsharedBuffers", &callbacks ) ); // ..., ok?
                
                return 1;
            }
        },
        { NULL, NULL }
    };

    luaL_register( L, NULL, funcs );
}
