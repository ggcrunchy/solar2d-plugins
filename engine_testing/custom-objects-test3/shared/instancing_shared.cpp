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

#define SHARED_BUFFER_MT_NAME "shared.memory"

struct ClassConstants {
    unsigned long uploadCommandID;
    unsigned long reuseCommandID;
    unsigned int vectorCount;
};

struct SharedMemoryResource {
    ClassConstants * constants;
    unsigned int timestamp;
    bool isDirty;
    float payload[1];
};

struct TimestampEntry {
    unsigned int timestamp;
    int mod, version;
};

struct SharedInstancingData {
    lua_State * luaState{NULL};
    std::vector<TimestampEntry> stamps;
    SharedMemoryResource * resource{NULL};
    unsigned long offset{0};
    int mod{-1};
    bool initialized{false};
    bool updated{false};
};

struct UploadData {
    SharedMemoryResource * buffer;
    unsigned long * offset;
};

struct ReuseData {
    unsigned long offset;
    unsigned int size;
};

static SharedInstancingData *
GetSharedData( void * userData )
{
    SharedInstancingData * sharedInstanceData = static_cast< SharedInstancingData * >( userData );
    
    if (!sharedInstanceData->initialized)
    {
        new (sharedInstanceData) SharedInstancingData;

        sharedInstanceData->initialized = true;
    }
    
    return sharedInstanceData;
}

void AddShared( lua_State * L)
{
    luaL_Reg funcs[] = {
        {
            "registerSharedDataType", [](lua_State * L)
            {
                CoronaEffectCallbacks callbacks = {};

                callbacks.size = sizeof( CoronaEffectCallbacks );
                callbacks.extraSpace = sizeof( SharedInstancingData );
                
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
                    if (0 == dataIndex)
                    {
                        SharedInstancingData * sharedInstanceData = GetSharedData( userData );
                        SharedMemoryResource * buffer = (SharedMemoryResource *)luaL_checkudata( L, valueIndex, SHARED_BUFFER_MT_NAME );

                        sharedInstanceData->luaState = L; // needed when detaching; assumed to be stable

                        if (sharedInstanceData->resource != buffer)
                        {
                            lua_pushlightuserdata( L, userData ); // ..., buffer, ..., key
                            lua_pushvalue( L, valueIndex ); // ..., buffer, ..., key, buffer
                            lua_rawset( L, LUA_REGISTRYINDEX ); // ..., buffer, ...; registry = { ..., [key] = buffer }
                            
                            sharedInstanceData->resource = buffer;

                            // new buffer, so no variant is in sync
                            for (auto && entry : sharedInstanceData->stamps)
                            {
                                entry.timestamp = 0;
                            }
                            
                            CoronaRendererInvalidate( L );
                        }
                        
                        return 1;
                    }

                    else
                    {
                        lua_pushfstring( L, "setData given bad index: %i", dataIndex ); // ..., err

                        *hadError = true;

                        return 1;
                    }
                };
                
                CoronaShaderDrawParams drawParams = {};
                
                drawParams.ignoreOriginal = true;
                drawParams.before = []( const CoronaShader * shader, void * userData, const CoronaRenderer * renderer, const CoronaRenderData * renderData )
                {
                    SharedInstancingData * sharedInstanceData = GetSharedData( userData );
                    SharedMemoryResource * buffer = sharedInstanceData->resource;

                    if (!buffer)
                    {
                        return;
                    }
                    
                    // Bring buffer up to date on first use.
                    if (buffer->isDirty)
                    {
                        ++buffer->timestamp;
                        
                        buffer->isDirty = false;
                    }
                    
                    // Has this one been seen yet?
                    int variant = -1, version = CoronaShaderGetVersion( renderData, renderer );
                    
                    for (int i = 0; i < sharedInstanceData->stamps.size(); ++i)
                    {
                        const auto & entry = sharedInstanceData->stamps[i];
                        
                        if (entry.mod == sharedInstanceData->mod && entry.version == version)
                        {
                            variant = i;
                            
                            break;
                        }
                    }
                    
                    // If not, remember it. The new entry, of course, is not in sync.
                    if (-1 == variant)
                    {
                        variant = sharedInstanceData->stamps.size();
                        
                        TimestampEntry entry = {};
                        
                        entry.mod = sharedInstanceData->mod;
                        entry.version = version;
                        
                        sharedInstanceData->stamps.push_back( entry );
                    }

                    // Do sync before drawing, if needed.
                    auto & entry = sharedInstanceData->stamps[variant];
                    
                    if (entry.timestamp != buffer->timestamp)
                    {
                        entry.timestamp = buffer->timestamp;
                        
                        sharedInstanceData->offset = 0;
                        sharedInstanceData->updated = true;
                    }
                    
                    // we have a buffer, so draw
                    CoronaShaderRawDraw( shader, renderData, renderer );
                };
                
                callbacks.drawParams = drawParams;
                
                callbacks.prepare = []( const CoronaShader *, void * userData, const CoronaRenderData *, int, int, int mod )
                {
                    SharedInstancingData * sharedInstanceData = GetSharedData( userData );

                    sharedInstanceData->mod = mod;
                };
                
                callbacks.shaderBind = []( const CoronaRenderer * renderer, void * userData )
                {
                    SharedInstancingData * sharedInstanceData = GetSharedData( userData );
                    
                    if (!sharedInstanceData->updated)
                    {
                        return;
                    }
                    
                    SharedMemoryResource * buffer = sharedInstanceData->resource;
                    
                    unsigned int size = buffer->constants->vectorCount * 4 * sizeof( float );
                    
                    // First shader needing to sync?
                    if (!sharedInstanceData->offset)
                    {
                        UploadData data = { buffer, &sharedInstanceData->offset };
                        
                        CoronaRendererIssueCommand( renderer, buffer->constants->uploadCommandID, &data, size );
                        
                        // Restore "no offset" condition after traversing hierarchy.
                        CoronaRendererOpParams params = {};
                        
                        params.u.renderer = renderer;
                        
                        CoronaRendererScheduleEndFrameOp( &params, []( const CoronaRenderer *, void * userData ) {
                            *(bool *)userData = false;
                        }, &sharedInstanceData->updated, NULL );
                    }
                    
                    // Subsequent one, so reuse uploaded contents.
                    else
                    {
                        ReuseData data = { sharedInstanceData->offset, size };
                        
                        CoronaRendererIssueCommand( renderer, buffer->constants->reuseCommandID, &data, sizeof( ReuseData ) );
                    }
                };
                
                callbacks.shaderDetach = []( const CoronaShader * shader, void * userData )
                {
                    SharedInstancingData * sharedInstanceData = GetSharedData( userData );
                    
                    using TimestampVector = std::vector<TimestampEntry>;
                    
                    sharedInstanceData->stamps.~TimestampVector();
                    
                    if (sharedInstanceData->resource)
                    {
                        lua_pushlightuserdata( sharedInstanceData->luaState, userData ); // ..., key
                        lua_pushnil( sharedInstanceData->luaState ); // ..., key, nil
                        lua_rawset( sharedInstanceData->luaState, LUA_REGISTRYINDEX ); // ...; registry = { ..., [key] = nil }
                    }
                };

                lua_pushboolean( L, CoronaShaderRegisterEffectDataType( L, "sharedBuffers", &callbacks ) ); // ..., ok?
                
                return 1;
            }
        }, {
            "newSharedBuffer", [](lua_State * L)
            {
                if (luaL_newmetatable( L, SHARED_BUFFER_MT_NAME )) // mt
                {
                    // All buffers will share a few constants...
                    ClassConstants * classConstants = (ClassConstants *)lua_newuserdata( L, sizeof( ClassConstants ) ); // mt, constants
                    
                    lua_setfield( L, -2, "_constants" ); // mt; mt.__constants = constants
                    
                    // #1: how many vectors Solar says we can expect in the vertex kernel.
                    // This sample just assume the kernel our u_Vectors array claims them
                    // all and any update fill them all. This brute-force approach might
                    // not be suitable in production!
                    lua_getglobal( L, "system" ); // mt, system
                    lua_getfield( L, -1, "getInfo" ); // mt, system, getInfo
                    lua_remove( L, -2 ); // mt, getInfo
                    lua_pushliteral( L, "maxUniformVectorsCount"); // mt, getInfo, "maxUniformVectorsCount"
                    lua_pcall( L, 1, 1, 0 ); // mt, maxCount
                    
                    classConstants->vectorCount = lua_tointeger( L, -1 );
                    
                    lua_pop( L, 1 ); // mt

                    // #2: the ID for issuing an upload command, when the buffer is first
                    // uploaded on a given frame.
                    CoronaCommand uploadCommand = {
                        []( const CoronaCommandBuffer * commandBuffer, const unsigned char * from, unsigned int size )
                        {
                            CoronaWriteUniformParams params = {};
                            
                            params.u.data = from;
                            
                            CoronaCommandBufferWriteNamedUniform( commandBuffer, "u_Vectors", &params, size );
                        },
                        
                        []( const CoronaCommandBuffer * commandBuffer, unsigned char * to, const void * data, unsigned int size )
                        {
                            const UploadData * uploadData = static_cast< const UploadData * >( data );
                            
                            *uploadData->offset = to - CoronaCommandBufferGetBaseAddress( commandBuffer );
                            
                            memcpy( to, uploadData->buffer->payload, size );
                        }
                    };
                    
                    CoronaRendererRegisterCommand( L, &uploadCommand, &classConstants->uploadCommandID );
                    
                    // #3: the ID for issuing a reuse command, once a buffer has already
                    // been uploaded, in order to take advantage of that content.
                    CoronaCommand reuseCommand = {};
                    
                    reuseCommand.reader = []( const CoronaCommandBuffer * commandBuffer, const unsigned char * from, unsigned int )
                    {
                        const ReuseData * reuseData = reinterpret_cast< const ReuseData * >( from );
                        CoronaWriteUniformParams params = {};
                        
                        params.u.offset = reuseData->offset;
                        params.useOffset = true;
                        
                        CoronaCommandBufferWriteNamedUniform( commandBuffer, "u_Vectors", &params, reuseData->size );
                    };
                    
                    CoronaRendererRegisterCommand( L, &reuseCommand, &classConstants->reuseCommandID );
                  
                    // Add some buffer methods.
                    lua_pushvalue( L, -1 ); // mt, mt
                    lua_setfield( L, -2, "__index" ); // mt = { _constants, __index = mt }
                    
                    luaL_Reg methods[] = {
                        {
                            "getVectorCount", []( lua_State * L )
                            {
                                SharedMemoryResource * buffer = (SharedMemoryResource *)luaL_checkudata( L, 1, SHARED_BUFFER_MT_NAME );
                                
                                lua_pushinteger( L, buffer->constants->vectorCount ); // buffer, vectorCount
                                
                                return 1;
                            }
                        },
                        {
                            "setVector", []( lua_State * L )
                            {
                                SharedMemoryResource * buffer = (SharedMemoryResource *)luaL_checkudata( L, 1, SHARED_BUFFER_MT_NAME );
                                int index = luaL_checkint( L, 2 );
                                
                                if (index >= 1 && index <= buffer->constants->vectorCount)
                                {
                                    float * output = buffer->payload + (index - 1) * 4;
                                    float x = luaL_checknumber( L, 3 );
                                    float y = luaL_optnumber( L, 4, 0 );
                                    float z = luaL_optnumber( L, 5, 0 );
                                    float w = luaL_optnumber( L, 6, 0 );
                                    
                                    output[0] = x;
                                    output[1] = y;
                                    output[2] = z;
                                    output[3] = w;
                                    
                                    buffer->isDirty = true;
                                    
                                    CoronaRendererInvalidate( L ); // COULD only do this is buffer in use, with say a ref count...
                                }
                                
                                return 0;
                            }
                        },
                        { NULL, NULL }
                    };
                    
                    luaL_register( L, NULL, methods );
                }
                
                // Make the new buffer.
                lua_getfield( L, -1, "_constants" ); // mt, constants
                
                ClassConstants * constants = (ClassConstants *)lua_touserdata( L, -1 );
                
                size_t bufferSize = sizeof( SharedMemoryResource ) + (constants->vectorCount * 4 - 1) * sizeof( float );
                
                SharedMemoryResource * buffer = (SharedMemoryResource *)lua_newuserdata( L, bufferSize ); // mt, constants, buffer
                
                buffer->constants = constants;
                buffer->timestamp = 0; // no content yet, so all shaders effectively in sync
                buffer->isDirty = false;
                
                lua_insert( L, -3 ); // buffer, mt, constants
                lua_pop( L, 1 ); // buffer, mt
                lua_setmetatable( L, -2 ); // buffer; buffer.metatable = mt

                return 1;
            }
        },
        { NULL, NULL }
    };
    
    luaL_register( L, NULL, funcs );
}
