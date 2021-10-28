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

#include "CoronaGraphics.h"
#include "CoronaObjects.h"
#include "CoronaLog.h"
#include "CoronaLua.h"

#include <string>
#include <vector>

#ifdef __APPLE__
    #include "TargetConditionals.h"

    #if TARGET_IPHONE
        #include <OpenGLES/ES2/gl.h>
        #include <OpenGLES/ES2/glext.h>
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
#endif

static void
EarlyOutPredicate( const CoronaDisplayObject *, void *, int * result )
{
    *result = false;
}

static void
AddToParamsList( CoronaObjectParamsHeader & head, CoronaObjectParamsHeader * params, unsigned short method )
{
    params->next = head.next;
    params->method = method;
    head.next = params;
}

static void
DisableCullAndHitTest( CoronaObjectParamsHeader & head )
{
    static CoronaObjectBooleanResultParams canCull, canHitTest;

    canCull.before = canHitTest.before = EarlyOutPredicate;

    AddToParamsList( head, &canCull.header, kAugmentedMethod_CanCull );
    AddToParamsList( head, &canHitTest.header, kAugmentedMethod_CanHitTest );
}

static void
DummyArgs( lua_State * L )
{
    lua_settop( L, lua_istable( L, 1 ) ); // [group]
    lua_pushinteger( L, 0 ); // [group, ]x
    lua_pushinteger( L, 0 ); // [group, ]x, y
    lua_pushinteger( L, 1 ); // [group, ]x, y, w
    lua_pushinteger( L, 1 ); // [group, ]x, y, w, h
}

static void
CopyWriter( const CoronaCommandBuffer *, unsigned char * out, const void * data, unsigned int size )
{
    memcpy( out, data, size );
}

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

struct ColorMaskSettings {
    bool red{true}, green{true}, blue{true}, alpha{true};
};

struct SharedColorMaskData {
    unsigned long endFrameOp = {};
    unsigned long command = {};
    CoronaRendererOp op;
    CoronaObjectParams params;
    ColorMaskSettings current, working;
    std::vector< ColorMaskSettings > stack;
    unsigned int id;
    bool hasSetID{false};
};

struct InstancedColorMaskData {
    SharedColorMaskData * shared;
    ColorMaskSettings settings;
    unsigned char hasRed : 1;
    unsigned char hasGreen : 1;
    unsigned char hasBlue : 1;
    unsigned char hasAlpha : 1;
};

static void
RegisterRendererLogic( lua_State * L, SharedColorMaskData * sharedData )
{
    CoronaCommand command = {
        [](const CoronaCommandBuffer *, const unsigned char * data, unsigned int size) {
            ColorMaskSettings mask;

            assert( size >= sizeof( ColorMaskSettings ) );

            memcpy( &mask, data, sizeof( ColorMaskSettings ) );

            glColorMask( mask.red, mask.green, mask.blue, mask.alpha );
        }, CopyWriter
    };

    CoronaRendererRegisterCommand( L, &command, &sharedData->command );
}

static void
UpdateColorMask( const CoronaRenderer * renderer, void * userData )
{
    SharedColorMaskData * _this = static_cast< SharedColorMaskData * >( userData );

    CoronaRendererIssueCommand( renderer, _this->command, &_this->working, sizeof( ColorMaskSettings ) );
    
    CoronaRendererOpParams params = { renderer, 0 };
    
    if (!_this->endFrameOp)
    {
        CoronaRendererScheduleEndFrameOp( &params, [](const CoronaRenderer * renderer, void * userData) {
            SharedColorMaskData * _this = static_cast< SharedColorMaskData * >( userData );
            ColorMaskSettings defSettings; // TODO: configurable?

            if (memcmp( &_this->current, &defSettings, sizeof( ColorMaskSettings ) ) != 0)
            {
                CoronaRendererIssueCommand( renderer, _this->command, &defSettings, sizeof( ColorMaskSettings ) );
            }
            
            _this->current = _this->working = defSettings;
            _this->endFrameOp = 0UL;
        }, _this, &_this->endFrameOp );
    }
            
    _this->current = _this->working;
}

static CoronaObjectDrawParams
DrawParams()
{
    CoronaObjectDrawParams drawParams = {};

    drawParams.ignoreOriginal = true;
    drawParams.after = []( const CoronaDisplayObject *, void * userData, const CoronaRenderer * renderer )
    {
        InstancedColorMaskData * _this = static_cast< InstancedColorMaskData * >( userData );
        SharedColorMaskData * shared = _this->shared;

        if (_this->hasRed)
        {
            shared->working.red = _this->settings.red;
        }

        if (_this->hasGreen)
        {
            shared->working.green = _this->settings.green;
        }

        if (_this->hasBlue)
        {
            shared->working.blue = _this->settings.blue;
        }

        if (_this->hasAlpha)
        {
            shared->working.alpha = _this->settings.alpha;
        }

        if (memcmp( &shared->current, &shared->working, sizeof( ColorMaskSettings ) ) != 0)
        {
            CoronaRendererDo( renderer, UpdateColorMask, shared );
        }
    };

    return drawParams;
}

static void
ClearValue( InstancedColorMaskData * _this, int index )
{
    switch (index)
    {
    case 0:
        _this->hasRed = false;

        break;
    case 1:
        _this->hasGreen = false;

        break;
    case 2:
        _this->hasBlue = false;

        break;
    case 3:
        _this->hasAlpha = false;

        break;
    default:
        assert( !"Not reached" );
    }
}

static void
SetValue( InstancedColorMaskData * _this, int index, bool enable )
{
    switch (index)
    {
    case 0:
        _this->settings.red = enable;
        _this->hasRed = true;

        break;
    case 1:
        _this->settings.green = enable;
        _this->hasGreen = true;

        break;
    case 2:
        _this->settings.blue = enable;
        _this->hasBlue = true;

        break;
    case 3:
        _this->settings.alpha = enable;
        _this->hasAlpha = true;

        break;
    default:
        assert( !"Not reached" );
    }
}

static CoronaObjectSetValueParams
SetValueParams()
{
    CoronaObjectSetValueParams setValueParams = {};

    setValueParams.before = []( const CoronaDisplayObject * object, void * userData, lua_State * L, const char key[], int valueIndex, int * result )
    {
        InstancedColorMaskData * _this = static_cast< InstancedColorMaskData * >( userData );

        if (strcmp( key, "red" ) == 0 || strcmp( key, "green" ) == 0 || strcmp( key, "blue" ) == 0 || strcmp( key, "alpha" ) == 0)
        {
            int index = key[1] != 'l' ? key[0] < 'r' : 2 + (key[0] < 'b');

            *result = true;

            if (lua_isnil( L, valueIndex ))
            {
                ClearValue( _this, index );
            }

            else
            {
                SetValue( _this, index, lua_toboolean( L, valueIndex ) );
            }

            CoronaObjectInvalidate( object );
        }
    };

    return setValueParams;
}

static void
PushColorMask( SharedColorMaskData * shared, const ScopeMessagePayload & payload )
{
    shared->stack.push_back( shared->working );

    shared->id = payload.drawSessionID;
    shared->hasSetID = true;
}

static void
PopColorMask( SharedColorMaskData * shared, const ScopeMessagePayload & payload )
{
    shared->hasSetID = false;

    if (!shared->stack.empty())
    {
        shared->working = shared->stack.back();

        shared->stack.pop_back();

        if (memcmp( &shared->working, &shared->current, sizeof( ColorMaskSettings ) ) != 0)
        {
            CoronaRendererDo( payload.renderer, UpdateColorMask, shared );
        }
    }

    else
    {
        CoronaLog( "Unbalanced 'PopColorMask' " );
    }
}

static CoronaObjectOnMessageParams
OnMessageParams()
{
    CoronaObjectOnMessageParams onMessageParams = {};

    onMessageParams.action = []( const CoronaDisplayObject *, void * userData, const char * message, const void * data, unsigned int size )
    {
        InstancedColorMaskData * _this = static_cast< InstancedColorMaskData * >( userData );
        SharedColorMaskData * shared = _this->shared;

        if (strcmp( message, "willDraw" ) == 0 || strcmp( message, "didDraw" ) == 0)
        {
            if (size >= sizeof( ScopeMessagePayload ) )
            {
                ScopeMessagePayload payload = *static_cast< const ScopeMessagePayload * >( data );

                if ('w' == message[0] && !shared->hasSetID)
                {
                    PushColorMask( shared, payload );
                }

                else if (shared->hasSetID && payload.drawSessionID == shared->id)
                {
                    PopColorMask( shared, payload );
                }
            }
                
            else
            {
                CoronaLog( "'%s' message's payload too small", message );
            }
        }
    };

    return onMessageParams;
}

static void
PopulateSharedData( lua_State * L, SharedColorMaskData * sharedData )
{
    RegisterRendererLogic( L, sharedData );

    CoronaObjectParamsHeader paramsList = {};

    DisableCullAndHitTest( paramsList );

    CoronaObjectDrawParams drawParams = DrawParams();

    AddToParamsList( paramsList, &drawParams.header, kAugmentedMethod_Draw );

    CoronaObjectSetValueParams setValueParams = SetValueParams();

    AddToParamsList( paramsList, &setValueParams.header, kAugmentedMethod_SetValue );

    CoronaObjectOnMessageParams onMessageParams = OnMessageParams();

    AddToParamsList( paramsList, &onMessageParams.header, kAugmentedMethod_OnMessage );

    CoronaObjectOnFinalizeParams onFinalizeParams = {};

    onFinalizeParams.action = []( const CoronaDisplayObject *, void * userData )
    {
        delete static_cast< InstancedColorMaskData * >( userData );
    };

    AddToParamsList( paramsList, &onFinalizeParams.header, kAugmentedMethod_OnFinalize );

    sharedData->params.useRef = true;
    sharedData->params.u.ref = CoronaObjectsBuildMethodStream( L, paramsList.next );
}

CORONA_EXPORT int luaopen_plugin_customobjects2( lua_State * L )
{
    lua_newtable(L);// customobjects2

    luaL_Reg funcs[] = {
        {
            "newScopedGroupObject", []( lua_State * L )
            {
                static unsigned int sScopeDrawSessionID;
                static CoronaObjectParams sParams;

                if (!sParams.useRef)
                {
                    CoronaObjectParamsHeader paramsList = {};

                    // Customize draw method:
                    CoronaObjectDrawParams drawParams = {};

                    drawParams.before = []( const CoronaDisplayObject * object, void * userData, const CoronaRenderer * renderer )
                    {
                        const CoronaGroupObject * groupObject = reinterpret_cast< const CoronaGroupObject * >( object );
                        ScopeMessagePayload payload = { renderer, sScopeDrawSessionID };

                        for (int i = 0, n = CoronaGroupObjectGetNumChildren( groupObject ); i < n; ++i)
                        {
                            CoronaObjectSendMessage( CoronaGroupObjectGetChild( groupObject, i ), "willDraw", &payload, sizeof( ScopeMessagePayload ) );
                        }
                    };

                    drawParams.after = []( const CoronaDisplayObject * object, void * userData, const CoronaRenderer * renderer )
                    {
                        const CoronaGroupObject * groupObject = reinterpret_cast< const CoronaGroupObject * >( object );
                        ScopeMessagePayload payload = { renderer, sScopeDrawSessionID++ };

                        for (int i = CoronaGroupObjectGetNumChildren( groupObject ); i; --i)
                        {
                            CoronaObjectSendMessage( CoronaGroupObjectGetChild( groupObject, i - 1 ), "didDraw", &payload, sizeof( ScopeMessagePayload ) );
                        }
                    };
                    
                    // Register customizations.
                    AddToParamsList( paramsList, &drawParams.header, kAugmentedMethod_Draw );
                    
                    sParams.useRef = true;
                    sParams.u.ref = CoronaObjectsBuildMethodStream( L, &paramsList );
                }

                return CoronaObjectsPushGroup( L, nullptr, &sParams ); // ...[, scopeGroup]
            },
        },
        {
            "newColorMaskObject", []( lua_State * L )
            {
                DummyArgs( L ); // [group, ]x, y, w, h

                static int sCookie;

                auto sharedColorMaskData = GetOrNew< SharedColorMaskData >( L, &sCookie, true );

                if (sharedColorMaskData.isNew)
                {
                    PopulateSharedData( L, sharedColorMaskData.object );
                }

                InstancedColorMaskData * colorMaskData = new InstancedColorMaskData;

                memset( colorMaskData, 0, sizeof( InstancedColorMaskData ) );

                colorMaskData->shared = sharedColorMaskData.object;

                return CoronaObjectsPushRect( L, colorMaskData, &sharedColorMaskData.object->params );
            }
        },
        { nullptr, nullptr }
    };

    luaL_register(L, nullptr, funcs);

    return 1;
}
