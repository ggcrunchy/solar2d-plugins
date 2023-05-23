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

struct SharedDepthStateData {
    unsigned long commandID{0};
    unsigned long blockID{0};
	CoronaObjectParams params;
	DepthEnvironment * env;
};

struct InstancedDepthStateData {
	SharedDepthStateData * shared;
	DepthSettings settings;
	unsigned short hasFunc : 1;
    unsigned short hasCullFace : 1;
    unsigned short hasFrontFace : 1;
    unsigned short hasNear : 1;
    unsigned short hasFar : 1;
    unsigned short hasMask : 1;
    unsigned short hasCullFaceEnabled : 1;
    unsigned short hasEnabled : 1;
};

static void
RegisterRendererLogic( lua_State * L, SharedDepthStateData * sharedData )
{
    CoronaCommand command = {};
    
    command.reader = []( const CoronaCommandBuffer *, const unsigned char * data, unsigned int size ) {
        DepthSettings settings[2];

        assert( size >= sizeof( settings ) );

        memcpy( settings, data, sizeof( settings ) );

        const DepthSettings & current = settings[0], & working = settings[1];

        if (current.func != working.func)
        {
            glDepthFunc( working.func );
        }

        if (current.cullFace != working.cullFace)
        {
            glCullFace( working.cullFace );
        }

        if (current.frontFace != working.frontFace)
        {
            glFrontFace( working.frontFace );
        }

        if (current.near != working.near || current.far != working.far)
        {
        #ifdef OPENGLES
            glDepthRangef
        #else
            glDepthRange
        #endif
            ( working.near, working.far );
        }

        if (current.mask != working.mask)
        {
            glDepthMask( working.mask );
        }

        if (current.cullFaceEnabled != working.cullFaceEnabled)
        {
            (working.cullFaceEnabled ? glEnable : glDisable)( GL_CULL_FACE );
        }

        if (current.enabled != working.enabled)
        {
            (working.enabled ? glEnable : glDisable)( GL_DEPTH_TEST );
        }
    };

	CoronaRendererRegisterCommand( L, &command, &sharedData->commandID );

    DepthSettings defSettings;
    CoronaStateBlock depth_state = {};
    
    depth_state.defaultContents = &defSettings;
    depth_state.blockSize = sizeof(DepthSettings);
    depth_state.userData = &sharedData->commandID;
    
    depth_state.stateDirty = []( const CoronaCommandBuffer * commandBuffer, const CoronaRenderer * renderer, const void * newContents, const void * oldContents, unsigned int, int, void * data ) {
        const DepthSettings * oldSettings = static_cast< const DepthSettings * >( oldContents );
        const DepthSettings * newSettings = static_cast< const DepthSettings * >( newContents );
        
        DepthSettings settings[] = { *oldSettings, *newSettings };
        
        CoronaRendererIssueCommand( renderer, *(unsigned long *)data, settings, sizeof(settings) );
    };
    
    CoronaRendererRegisterStateBlock( L, &depth_state, &sharedData->blockID );
	
	GLboolean test, write_mask, cull_face;
	GLfloat depth_clear, depth_range[2];
	GLint depth_func, cull_face_mode;

	glGetBooleanv(GL_DEPTH_TEST, &test);
	glGetBooleanv(GL_DEPTH_WRITEMASK, &write_mask);
	glGetBooleanv(GL_CULL_FACE, &cull_face);
	glGetFloatv(GL_DEPTH_CLEAR_VALUE, &depth_clear);
	glGetFloatv(GL_DEPTH_RANGE, depth_range);
	glGetIntegerv(GL_DEPTH_FUNC, &depth_func);
	glGetIntegerv(GL_CULL_FACE_MODE, &cull_face_mode);

	lua_pushboolean( L, !!test );
	lua_setglobal( L, "DEPTH_TEST" );
	lua_pushboolean( L, !!write_mask );
	lua_setglobal( L, "DEPTH_WRITEMASK" );
	lua_pushboolean( L, !!cull_face );
	lua_setglobal( L, "CULL_FACE" );
	lua_pushnumber( L, depth_clear );
	lua_setglobal( L, "DEPTH_CLEAR_VALUE" );
	lua_pushnumber( L, depth_range[0] );
	lua_setglobal( L, "DEPTH_RANGE0" );
	lua_pushnumber( L, depth_range[1] );
	lua_setglobal( L, "DEPTH_RANGE1" );

	const char * cf_modes[] = { "FRONT", "BACK", "FRONT_AND_BACK", nullptr };
	int modes[] = { GL_FRONT, GL_BACK, GL_FRONT_AND_BACK };
	
	for (int i = 0; cf_modes[i]; ++i)
	{
		if (modes[i] == cull_face_mode)
		{
			lua_pushstring( L, cf_modes[i] );
			lua_setglobal( L, "CULL_FACE_MODE" );
			
			break;
		}
	}
	
	const char * d_funcs[] = { "NEVER", "LESS", "EQUAL", "LEQUAL", "GREATER", "NOTEQUAL", "GEQUAL", "ALWAYS", nullptr };
	int funcs[] = { GL_NEVER, GL_LESS, GL_EQUAL, GL_LEQUAL, GL_GREATER, GL_NOTEQUAL, GL_GEQUAL, GL_ALWAYS };
	
	for (int i = 0; d_funcs[i]; ++i)
	{
		if (funcs[i] == depth_func)
		{
			lua_pushstring( L, d_funcs[i] );
			lua_setglobal( L, "DEPTH_FUNC" );
			
			break;
		}
	}	
/*
	GL_DEPTH_CLEAR_VALUE - 1 (float))
	GL_DEPTH_FUNC - value (GL_LESS)
	GL_DEPTH_RANGE - 0, 1 (float
	GL_DEPTH_TEST - boolean (false)
	GL_DEPTH_WRITEMASK - boolean (true)
	GL_CULL_FACE - boolean (false)
	GL_CULL_FACE_MODE - value, GL_FRONT,GL_BACK,GL_FRONT_AND_BACK (back)*/
}

static CoronaObjectDrawParams
DrawParams()
{
	CoronaObjectDrawParams drawParams = {};

	drawParams.ignoreOriginal = true;
	drawParams.after = []( const CoronaDisplayObject *, void * userData, const CoronaRenderer * renderer )
	{
		InstancedDepthStateData * _this = static_cast< InstancedDepthStateData * >( userData );
		DepthEnvironment * env = _this->shared->env;

		if (_this->hasFunc)
		{
			env->current.settings.func = _this->settings.func;
		}

		if (_this->hasCullFace)
		{
			env->current.settings.cullFace = _this->settings.cullFace;
		}

		if (_this->hasFrontFace)
		{
			env->current.settings.frontFace = _this->settings.frontFace;
		}

		if (_this->hasNear)
		{
			env->current.settings.near = _this->settings.near;
		}

		if (_this->hasFar)
		{
			env->current.settings.far = _this->settings.far;
		}

		if (_this->hasMask)
		{
			env->current.settings.mask = _this->settings.mask;
		}
        
        if (_this->hasCullFaceEnabled)
        {
            env->current.settings.cullFaceEnabled = _this->settings.cullFaceEnabled;
        }

		if (_this->hasEnabled)
		{
			env->current.settings.enabled = _this->settings.enabled;
		}

        CoronaRendererWriteStateBlock( renderer, _this->shared->blockID, &env->current.settings, sizeof(DepthSettings) );
	};

	return drawParams;
}

static void
ClearBoolean( lua_State * L, InstancedDepthStateData * _this, int index )
{
	switch (index)
	{
	case 0:
		_this->hasCullFaceEnabled = false;

		break;
	case 1:
		_this->hasEnabled = false;

		break;
	case 2:
		_this->hasMask = false;

		break;
	default:
        luaL_error( L, "Attempt to clear unknown boolean property" );
	}
}

static void
SetBoolean( lua_State * L, InstancedDepthStateData * _this, int index, int valueIndex )
{
	bool value = lua_toboolean( L, valueIndex );

	switch (index)
	{
	case 0:
		_this->settings.cullFaceEnabled = value;
		_this->hasCullFaceEnabled = true;

		break;
	case 1:
		_this->settings.enabled = value;
		_this->hasEnabled = true;

		break;
	case 2:
		_this->settings.mask = value;
		_this->hasMask = true;

		break;
	default:
        luaL_error( L, "Attempt to set unknown boolean property" );
	}
}

static void
UpdateBoolean( lua_State * L, InstancedDepthStateData * _this, const char key[], int valueIndex )
{
	int index = 'm' == key[0] ? 2 : 'e' == key[0];

	if (lua_isnil( L, valueIndex ))
	{
		ClearBoolean( L, _this, index );
	}

	else
	{
		SetBoolean( L, _this, index, valueIndex );
	}
}

static const char *
UpdateFunc( lua_State * L, InstancedDepthStateData * _this, int valueIndex )
{
	if (lua_isnil( L, valueIndex ))
	{
		_this->hasFunc = false;
	}

	else if (lua_isstring( L, valueIndex ))
	{
		if (FindFunc( L, valueIndex, &_this->settings.func ))
		{
			_this->hasFunc = true;
		}

		else
		{
			CoronaLuaWarning( L, "'%s' is not a supported depth function", lua_tostring( L, valueIndex ) );
		}
	}

	else
	{
		return "string";
	}

	return NULL;
}

static void
ClearFaceValue( lua_State * L, InstancedDepthStateData * _this, int opIndex )
{
	switch (opIndex)
	{
	case 0:
		_this->hasCullFace = false;

		break;
	case 1:
		_this->hasFrontFace = false;

		break;
	default:
        luaL_error( L, "Attempt to clear unknown face property" );
	}
}

static void
SetFaceValue( lua_State * L, InstancedDepthStateData * _this, int opIndex, int valueIndex )
{
	const char * cullFaceNames[] = { "front", "back", "frontAndBack", NULL }, * frontFaceNames[] = { "cw", "ccw", NULL }, ** names = NULL;

	switch (opIndex)
	{
	case 0:
		names = cullFaceNames;

		break;
	case 1:
		names = frontFaceNames;

		break;
	default:
        luaL_error( L, "Attempt to set unknown face property" );
	}

	int resultIndex = FindName( L, valueIndex, names );

	if (names[resultIndex])
	{
		switch (opIndex)
		{
		case 0:
			{
				const GLenum cullFaceValues[] = { GL_FRONT, GL_BACK, GL_FRONT_AND_BACK };

				_this->settings.cullFace = cullFaceValues[resultIndex];
			}

			_this->hasCullFace = true;

			break;
		case 1:
			{
				const GLenum frontFaceValues[] = { GL_CW, GL_CCW };

				_this->settings.frontFace = frontFaceValues[resultIndex];
			}

			_this->hasFrontFace = true;

			break;
		default:
            luaL_error( L, "Attempt to assign unknown face property value" );
		}
	}

	else
	{
		CoronaLuaWarning( L, "'%s' is not a supported face value", lua_tostring( L, valueIndex ) );
	}
}

static const char *
UpdateFaceOp( lua_State * L, InstancedDepthStateData * _this, const char key[], int valueIndex )
{
	int opIndex = 'f' == key[0];

	if (lua_isnil( L, valueIndex ))
	{
		ClearFaceValue( L, _this, opIndex );
	}

	else if (lua_isstring( L, valueIndex ))
	{
		SetFaceValue( L, _this, opIndex, valueIndex );
	}

	else
	{
		return "string";
	}

	return NULL;
}

static void
ClearConstantValue( lua_State * L, InstancedDepthStateData * _this, int index )
{
	switch (index)
	{
	case 0:
		_this->hasNear = false;

		break;
	case 1:
		_this->hasFar = false;

		break;
	default:
        luaL_error( L, "Attempt to clear unknown constant property" );
	}
}

static void
SetConstantValue( lua_State * L, InstancedDepthStateData * _this, int index, int valueIndex )
{
	switch (index)
	{
	case 0:
		_this->settings.near = lua_tonumber( L, valueIndex );
		_this->hasNear = true;

		break;
	case 1:
		_this->settings.far = lua_tonumber( L, valueIndex );
		_this->hasFar = true;

		break;
	default:
        luaL_error( L, "Attempt to set unknown constant property" );
	}
}

static const char *
UpdateConstant( lua_State * L, InstancedDepthStateData * _this, const char key[], int valueIndex )
{
	int index = 'f' == key[0];

	if (lua_isnil( L, valueIndex ))
	{
		ClearConstantValue( L, _this, index );
	}

	else if (lua_isnumber( L, valueIndex ))
	{
		SetConstantValue( L, _this, index, valueIndex );
	}

	else
	{
		return "number";
	}

	return NULL;
}

static CoronaObjectSetValueParams
SetValueParams()
{
	CoronaObjectSetValueParams setValueParams = {};

	setValueParams.before = []( const CoronaDisplayObject *, void * userData, lua_State * L, const char key[], int valueIndex, int * result )
	{
		InstancedDepthStateData * _this = static_cast< InstancedDepthStateData * >( userData );
		const char * expected = NULL;

		*result = true;

		if (strcmp( key, "cullFaceEnabled" ) == 0 || strcmp( key, "enabled" ) == 0 || strcmp( key, "mask" ) == 0)
		{
			UpdateBoolean( L, _this, key, valueIndex );
		}

		else if (strcmp( key, "func" ) == 0)
		{
			expected = UpdateFunc( L, _this, valueIndex );
		}

		else if (strcmp( key, "cullFace" ) == 0 || strcmp( key, "frontFace" ) == 0)
		{
			expected = UpdateFaceOp( L, _this, key, valueIndex );
		}

		else if (strcmp( key, "near" ) == 0 || strcmp( key, "far" ) == 0)
		{
			expected = UpdateConstant( L, _this, key, valueIndex );
		}

		else
		{
			*result = false;
		}

		if (expected)
		{
			CoronaLuaWarning( L, "Expected %s or nil for '%s', got %s", expected, key, luaL_typename( L, valueIndex ) );
		}
	};

	return setValueParams;
}

static void
PushDepthState( DepthEnvironment * env, const ScopeMessagePayload & payload )
{
	env->stack.push_back( env->current );

	env->id = payload.drawSessionID;
	env->hasSetID = true;
}

static void
PopDepthState( InstancedDepthStateData * _this, DepthEnvironment * env, const ScopeMessagePayload & payload )
{
	env->hasSetID = false;

	if (!env->stack.empty())
	{
		env->current = env->stack.back();

		env->stack.pop_back();

        CoronaRendererWriteStateBlock( payload.renderer, _this->shared->blockID, &env->current.settings, sizeof(DepthSettings) );
	}

	else
	{
		CoronaLog( "Unbalanced 'didDraw' " );
	}
}

static CoronaObjectOnMessageParams
OnMessageParams()
{
	CoronaObjectOnMessageParams onMessageParams = {};

	onMessageParams.action = []( const CoronaDisplayObject *, void * userData, const char * message, const void * data, unsigned int size )
	{
		InstancedDepthStateData * _this = static_cast< InstancedDepthStateData * >( userData );
		DepthEnvironment * env = _this->shared->env;

		if (strcmp( message, "willDraw" ) == 0 || strcmp( message, "didDraw" ) == 0)
		{
			if (size >= sizeof( ScopeMessagePayload ) )
			{
				ScopeMessagePayload payload = *static_cast< const ScopeMessagePayload * >( data );

				if ('w' == message[0] && !env->hasSetID)
				{
					PushDepthState( env, payload );
				}

				else if (env->hasSetID && payload.drawSessionID == env->id)
				{
					PopDepthState( _this, env, payload );
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
PopulateSharedData( lua_State * L, SharedDepthStateData * sharedData )
{
	DepthEnvironment * env = InitDepthEnvironment( L );

	sharedData->env = env;

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
		delete static_cast< InstancedDepthStateData * >( userData );
	};

	AddToParamsList( paramsList, &onFinalizeParams.header, kAugmentedMethod_OnFinalize );

	sharedData->params.useRef = true;
	sharedData->params.u.ref = CoronaObjectsBuildMethodStream( L, paramsList.next );
}

int
DepthStateObject( lua_State * L )
{
	DummyArgs( L ); // [group, ]x, y, w, h

	static int sCookie;

	auto sharedStateData = GetOrNew< SharedDepthStateData >(L, &sCookie );

	if (sharedStateData.isNew)
	{
		PopulateSharedData( L, sharedStateData.object );
	}

	InstancedDepthStateData * stateData = new InstancedDepthStateData;

	memset( stateData, 0, sizeof( InstancedDepthStateData ) );

	stateData->shared = sharedStateData.object;

	return CoronaObjectsPushRect( L, stateData, &sharedStateData.object->params );
}
