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

#include "utils.h"

static int
ScopeGroupObject( lua_State * L )
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
            const CoronaDisplayObject * child = reinterpret_cast< const CoronaDisplayObject * >( CoronaObjectGetAvailableSlot() );
            const CoronaGroupObject * groupObject = reinterpret_cast< const CoronaGroupObject * >( object );
            ScopeMessagePayload payload = { renderer, sScopeDrawSessionID };

            for (int i = 0, n = CoronaGroupObjectGetNumChildren( groupObject ); i < n; ++i)
            {
                CoronaGroupObjectGetChild( groupObject, i, child );
                CoronaObjectSendMessage( child, "willDraw", &payload, sizeof( ScopeMessagePayload ) );
            }
        };

        drawParams.after = []( const CoronaDisplayObject * object, void * userData, const CoronaRenderer * renderer )
        {
            const CoronaDisplayObject * child = reinterpret_cast< const CoronaDisplayObject * >( CoronaObjectGetAvailableSlot() );
            const CoronaGroupObject * groupObject = reinterpret_cast< const CoronaGroupObject * >( object );
            ScopeMessagePayload payload = { renderer, sScopeDrawSessionID++ };

            for (int i = CoronaGroupObjectGetNumChildren( groupObject ); i; --i)
            {
                CoronaGroupObjectGetChild( groupObject, i - 1, child );
                CoronaObjectSendMessage( child, "didDraw", &payload, sizeof( ScopeMessagePayload ) );
            }
        };
        
        // Register customizations.
        AddToParamsList( paramsList, &drawParams.header, kAugmentedMethod_Draw );
        
        sParams.useRef = true;
        sParams.u.ref = CoronaObjectsBuildMethodStream( L, &paramsList );
    }

    return CoronaObjectsPushGroup( L, nullptr, &sParams ); // ...[, scopeGroup]
}

void
EarlyOutPredicate( const CoronaDisplayObject *, void *, int * result )
{
	*result = false;
}

void
AddToParamsList( CoronaObjectParamsHeader & head, CoronaObjectParamsHeader * params, unsigned short method )
{
	params->next = head.next;
	params->method = method;
	head.next = params;
}

void
DisableCullAndHitTest( CoronaObjectParamsHeader & head )
{
	static CoronaObjectBooleanResultParams canCull, canHitTest;

	canCull.before = canHitTest.before = EarlyOutPredicate;

	AddToParamsList( head, &canCull.header, kAugmentedMethod_CanCull );
	AddToParamsList( head, &canHitTest.header, kAugmentedMethod_CanHitTest );
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

int
FindName( lua_State * L, int valueIndex, const char * list[] )
{
	const char * name = lua_tostring( L, valueIndex );
	int index = 0;

	while (list[index] && strcmp( list[index], name ) != 0 )
	{
		++index;
	}

	return index;
}

bool FindFunc( lua_State * L, int valueIndex, int * func )
{
	const char * names[] = { "never", "less", "equal", "greater", "greaterThanOrEqual", "lessThanOrEqual", "notEqual", "always", NULL };
	int index = FindName( L, valueIndex, names );

	if (names[index])
	{
		const GLenum funcs[] = { GL_NEVER, GL_LESS, GL_EQUAL, GL_GREATER, GL_GEQUAL, GL_LEQUAL, GL_NOTEQUAL, GL_ALWAYS };

		*func = funcs[index];

		return true;
	}

	return false;
}

void
AddUtils( lua_State * L)
{
	luaL_Reg funcs[] = {
		{ "newScopeGroupObject", ScopeGroupObject },
		{ NULL, NULL }
	};

	luaL_register( L, NULL, funcs );
}
