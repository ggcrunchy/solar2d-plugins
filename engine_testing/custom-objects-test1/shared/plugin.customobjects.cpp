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

#include "CoronaObjects.h"
#include "CoronaLog.h"
#include "CoronaLua.h"

#include <string>

static void
AddToParamsList( CoronaObjectParamsHeader & head, CoronaObjectParamsHeader * params, unsigned short method )
{
    params->next = head.next;
    params->method = method;
    head.next = params;
}


void
DummyArgs( lua_State * L )
{
    lua_settop( L, lua_istable( L, 1 ) ); // [group]
    lua_pushinteger( L, 0 ); // [group, ]x
    lua_pushinteger( L, 0 ); // [group, ]x, y
    lua_pushinteger( L, 1 ); // [group, ]x, y, w
    lua_pushinteger( L, 1 ); // [group, ]x, y, w, h
}

static void
NoOp( const CoronaDisplayObject *, void *, int * )
{
}

struct ScopeMessagePayload {
    const CoronaRenderer * rendererHandle;
    unsigned int drawSessionID;
};

CORONA_EXPORT int luaopen_plugin_customobjects1 (lua_State * L)
{
    lua_newtable(L);// customobjects1

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

                    drawParams.before = []( const CoronaDisplayObject * object, void * userData, const CoronaRenderer * rendererHandle )
                    {
                        const CoronaGroupObject * groupObject = reinterpret_cast< const CoronaGroupObject * >( object );
                        ScopeMessagePayload payload = { rendererHandle, sScopeDrawSessionID };

                        for (int i = 0, n = CoronaGroupObjectGetNumChildren( groupObject ); i < n; ++i)
                        {
                            CoronaObjectSendMessage( CoronaGroupObjectGetChild( groupObject, i ), "willDraw", &payload, sizeof( ScopeMessagePayload ) );
                        }
                    };

                    drawParams.after = []( const CoronaDisplayObject * object, void * userData, const CoronaRenderer * rendererHandle )
                    {
                        const CoronaGroupObject * groupObject = reinterpret_cast< const CoronaGroupObject * >( object );
                        ScopeMessagePayload payload = { rendererHandle, sScopeDrawSessionID++ };

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
            "newPrintObject", []( lua_State * L )
            {
                DummyArgs(L);
                
                struct PrintObjectState {
                    std::string fDidDraw, fWillDraw, fDraw;
                };

                static CoronaObjectParams sParams;

                if (!sParams.useRef)
                {
                    CoronaObjectParamsHeader paramsList = {};

                    // Customize canCull method:
                    CoronaObjectBooleanResultParams canCullParams = {};
                    
                    canCullParams.before = NoOp;
                    
                    // Customize canHitTest method:
                    CoronaObjectBooleanResultParams canHitTestParams = {};
                    
                    canHitTestParams.before = NoOp;
                    
                    // Customize draw method:
                    CoronaObjectDrawParams drawParams = {};

                    drawParams.ignoreOriginal = true;
                    drawParams.after = []( const CoronaDisplayObject * object, void * userData, const CoronaRenderer * rendererHandle )
                    {
                        PrintObjectState * state = static_cast< PrintObjectState * >( userData );
                        
                        if (!state->fDraw.empty())
                        {
                            CoronaLog( state->fDraw.c_str() );
                        }
                    };

                    drawParams.header.method = kAugmentedMethod_Draw;
                    
                    // Customize message method:
                    CoronaObjectOnMessageParams onMessageParams = {};

                    onMessageParams.action = []( const CoronaDisplayObject *, void * userData, const char * message, const void * data, unsigned int size )
                    {
                        if (strcmp( message, "willDraw" ) == 0 || strcmp( message, "didDraw" ) == 0)
                        {
                            PrintObjectState * state = static_cast< PrintObjectState * >( userData );
                            
                            if ('w' == message[0])
                            {
                                if (!state->fWillDraw.empty())
                                {
                                    CoronaLog( state->fWillDraw.c_str() );
                                }
                            }

                            else
                            {
                                if (!state->fDidDraw.empty())
                                {
                                    CoronaLog( state->fDidDraw.c_str() );
                                }
                            }
                        }
                    };
                    
                    // Customize SetValue method:
                    CoronaObjectSetValueParams setValueParams = {};

                    setValueParams.before = []( const CoronaDisplayObject *, void * userData, lua_State * L, const char key[], int valueIndex, int * result )
                    {
                        if ((
                             strcmp( key, "didDrawMessage" ) == 0 ||
                             strcmp( key, "willDrawMessage" ) == 0 ||
                             strcmp( key, "drawMessage" ) == 0
                        ) && lua_isstring( L, valueIndex ) && lua_objlen( L, valueIndex ) > 0)
                        {
                            PrintObjectState * state = static_cast< PrintObjectState * >( userData );
                            const char * message = lua_tostring( L, valueIndex );
                            
                            *result = true;
                            
                            if ('r' == key[1]) // "draw"
                            {
                                state->fDraw = message;
                            }
                            
                            else if ('w' == key[0]) // "will"
                            {
                                state->fWillDraw = message;
                            }
                            
                            else // "did"
                            {
                                state->fDidDraw = message;
                            }
                        }
                    };
                    
                    // Customize onCreate method:
                    CoronaObjectOnCreateParams onCreateParams = {};

                    onCreateParams.action = []( const CoronaDisplayObject *, void ** userData )
                    {
                        *userData = new PrintObjectState;
                    };
                    
                    // Customize onFinalize method:
                    CoronaObjectOnFinalizeParams onFinalizeParams = {};

                    onFinalizeParams.action = []( const CoronaDisplayObject *, void * userData )
                    {
                        delete static_cast< PrintObjectState * >( userData );
                    };
                    
                    // Register customizations.
                    AddToParamsList( paramsList, &canCullParams.header, kAugmentedMethod_CanCull );
                    AddToParamsList( paramsList, &canHitTestParams.header, kAugmentedMethod_CanHitTest );
                    AddToParamsList( paramsList, &drawParams.header, kAugmentedMethod_Draw );
                    AddToParamsList( paramsList, &onMessageParams.header, kAugmentedMethod_OnMessage );
                    AddToParamsList( paramsList, &setValueParams.header, kAugmentedMethod_SetValue );
                    AddToParamsList( paramsList, &onCreateParams.header, kAugmentedMethod_OnCreate );
                    AddToParamsList( paramsList, &onFinalizeParams.header, kAugmentedMethod_OnFinalize );
                    
                    sParams.useRef = true;
                    sParams.u.ref = CoronaObjectsBuildMethodStream( L, &paramsList );
                }
                
                // Make object and add state.
                return CoronaObjectsPushRect( L, nullptr, &sParams );
            }
        },
        { nullptr, nullptr }
    };

    luaL_register(L, nullptr, funcs);

    return 1;
}
