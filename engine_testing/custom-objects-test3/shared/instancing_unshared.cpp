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
    UnsharedInstancingData * next{NULL};
    UnsharedMemoryResource * resource{NULL};
    float * dataOffset{NULL};
    CoronaGeometryMappingLayout layout;
    BasicInstancingData instances{};
    unsigned long uploadCommand{0};
    unsigned long uploadResourceBlock{0};
    unsigned long resetShaderBoundBlock{0};
    int ref{LUA_NOREF};
    bool initialized{false};
    bool shaderBound{false};
};

static std::vector<int> sCleanupList;

struct DataAndOffset {
    UnsharedInstancingData * data;
    float * offset; // not used, but makes the state distinct
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
UploadResource( const CoronaRenderer * renderer, UnsharedInstancingData * unsharedInstanceData)
{
    UnsharedMemoryResource * buffer = unsharedInstanceData->resource;
    size_t nfloats = (buffer->bakedVectorsCount + unsharedInstanceData->instances.count + 1) * 4;

    CoronaRendererIssueCommand( renderer, unsharedInstanceData->uploadCommand, unsharedInstanceData, nfloats * sizeof( float ) );
    
    unsharedInstanceData->dataOffset += unsharedInstanceData->instances.count * 4;
}

static void
CreateUnsharedBuffer( lua_State * L, unsigned long * uploadCommandID, unsigned long * uploadResourceBlockID, unsigned long * resetShaderBoundBlockID )
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
        
        //
        CoronaStateBlock uploadResourceBlock = {};
        
        uploadResourceBlock.blockSize = sizeof(DataAndOffset);
        
        uploadResourceBlock.stateDirty = []( const CoronaCommandBuffer * commandBuffer, const CoronaRenderer * renderer, const void * newContents, const void *, unsigned int, int restore, void * ) {
            if (!restore)
            {
                const DataAndOffset * dataAndOffset = static_cast<const DataAndOffset *>( newContents );
                
                UploadResource( renderer, dataAndOffset->data );
            }
        };
        
        CoronaRendererRegisterStateBlock( L, &uploadResourceBlock, uploadResourceBlockID );
 
        //
        CoronaStateBlock resetShaderBoundBlock = {};
        
        resetShaderBoundBlock.blockSize = sizeof(UnsharedInstancingData *);
        
        resetShaderBoundBlock.stateDirty = []( const CoronaCommandBuffer * commandBuffer, const CoronaRenderer * renderer, const void *, const void * oldContents, unsigned int, int restore, void * ) {
            if (restore)
            {
                using UnsharedInstancingDataPtr = UnsharedInstancingData *;
                
                UnsharedInstancingData * cur = *static_cast<const UnsharedInstancingDataPtr *>( oldContents );
                
                while (cur)
                {
                    UnsharedInstancingData * next = cur->next;
                    
                    cur->shaderBound = false;
                    cur->next = NULL;
                    
                    cur = next;
                }
            }
        };
        
        CoronaRendererRegisterStateBlock( L, &resetShaderBoundBlock, resetShaderBoundBlockID );

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

        lua_getglobal( L, "Runtime" ); // mt, Runtime
        lua_getfield( L, -1, "addEventListener" ); // mt, Runtime, Runtime:addEventListener
        lua_insert( L, -2 ); // mt, Runtime:addEventListener, Runtime
        lua_pushliteral( L, "lateUpdate" ); // mt, Runtime:addEventListener, Runtime, "lateUpdate"
        lua_pushcfunction( L, [](lua_State * L) {
            for ( int ref : sCleanupList ) lua_unref( L, ref );

            sCleanupList.clear();

            return 0;
        }); // mt, Runtime:addEventListener, Runtime, "lateUpdate", Cleanup
        lua_call( L, 3, 0 ); // mt
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
                            CreateUnsharedBuffer( L, &unsharedInstanceData->uploadCommand, &unsharedInstanceData->uploadResourceBlock, &unsharedInstanceData->resetShaderBoundBlock ); // ..., buffer
                            
                            unsharedInstanceData->resource = (UnsharedMemoryResource *)luaL_checkudata( L, -1, UNSHARED_BUFFER_MT_NAME );
                            unsharedInstanceData->ref = lua_ref( L, 1 ); // ...; ref = buffer
                        }
                        
                        lua_rawgeti( L, LUA_REGISTRYINDEX, unsharedInstanceData->ref ); // resource?
                        
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
                
                CoronaShaderDrawParams drawParams = {};
                
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
                    
                    // Set up and draw any loads.
                    if ( !AddInstanceWriter( renderer, unsharedInstanceData->instances.name ) ) return;

                    unsharedInstanceData->dataOffset = buffer->payload.data() + buffer->bakedVectorsCount * 4;
                    
                    size_t instancesCount = vectorCount - 1;

                    while (instancesCount > 0)
                    {
                        UpdateCount( unsharedInstanceData, instancesCount );

                        DataAndOffset dao = { unsharedInstanceData, unsharedInstanceData->dataOffset };
                        
                        CoronaRendererWriteStateBlock( renderer, unsharedInstanceData->uploadResourceBlock, &dao, sizeof(DataAndOffset) );
                        
                        DrawN( shader, renderer, renderData, unsharedInstanceData );
                    }
                };
                
                callbacks.drawParams = drawParams;
                
                callbacks.prepare = []( const CoronaShader * shader, void * userData, const CoronaRenderData * renderData, int, int, int )
                {
                    UnsharedInstancingData * unsharedInstanceData = GetUnsharedData( userData );

                    unsharedInstanceData->instances.name/*out*/ = NULL;

                    CoronaEffectDetail detail;
                        
                    for (int i = 0; CoronaShaderGetEffectDetail( shader, i, &detail ); ++i)
                    {
                        if (strcmp( detail.name, "supportsInstancing" ) == 0)
                        {
                            unsharedInstanceData->instances.name = detail.value;
                        }
                    }
                };

                callbacks.shaderDetach = []( const CoronaShader * shader, void * userData )
                {
                    UnsharedInstancingData * unsharedInstanceData = GetUnsharedData( userData );
                    
                    if (unsharedInstanceData->resource) sCleanupList.push_back( unsharedInstanceData->ref );
                };

                lua_pushboolean( L, CoronaShaderRegisterEffectDataType( L, "unsharedBuffers", &callbacks ) ); // ..., ok?
                
                return 1;
            }
        },
        { NULL, NULL }
    };

    luaL_register( L, NULL, funcs );
}
