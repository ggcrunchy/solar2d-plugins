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

struct SharedDepthClearData {
	CoronaObjectParams params;
	DepthEnvironment * env;
};

struct InstancedDepthClearData {
	SharedDepthClearData * shared;
	double clear;
	bool hasClear;
};

static CoronaObjectDrawParams
DrawParams()
{
	CoronaObjectDrawParams drawParams = {};

	drawParams.ignoreOriginal = true;
	drawParams.after = []( const CoronaDisplayObject *, void * userData, const CoronaRenderer * renderer )
	{
		InstancedDepthClearData * _this = static_cast< InstancedDepthClearData * >( userData );
		SharedDepthClearData * shared = _this->shared;

		if (_this->hasClear)
		{
			shared->env->current.clear = _this->clear;
		}

		CoronaRendererDo( renderer, []( const CoronaRenderer * renderer, void * userData ) {
			DepthEnvironment * _this = static_cast< DepthEnvironment * >( userData );

            CoronaRendererIssueCommand( renderer, _this->commandID, &_this->current.clear, sizeof( double ) ); // TODO: could pass in current and working, compare...

			_this->anySinceClear = false; // TODO: MIGHT be usable to avoid unnecessary clears
		}, shared->env );
	};

	return drawParams;
}

static CoronaObjectSetValueParams
SetValueParams()
{
	CoronaObjectSetValueParams setValueParams = {};

	setValueParams.before = []( const CoronaDisplayObject *, void * userData, lua_State * L, const char key[], int valueIndex, int * result )
	{
		if (strcmp( key, "value" ) == 0)
		{
			InstancedDepthClearData * _this = static_cast< InstancedDepthClearData * >( userData );

			if (lua_isnil( L, valueIndex ))
			{
				_this->hasClear = false;
			}

			else if (lua_isnumber( L, valueIndex ))
			{
				_this->clear = lua_tonumber( L, valueIndex ); // TODO: could validate
				_this->hasClear = true;
			}

			else
			{
				CoronaLuaWarning( L, "Expected number or nil for 'value', got %s", luaL_typename( L, valueIndex ) );
			}

			*result = true;
		}
	};

	return setValueParams;
}

static void
PopulateSharedData( lua_State * L, SharedDepthClearData * sharedData )
{
	DepthEnvironment * env = InitDepthEnvironment( L );

	sharedData->env = env;

	CoronaObjectParamsHeader paramsList = {};

	DisableCullAndHitTest( paramsList );

	CoronaObjectDrawParams drawParams = DrawParams();

	AddToParamsList( paramsList, &drawParams.header, kAugmentedMethod_Draw );

	CoronaObjectSetValueParams setValueParams = SetValueParams();

	AddToParamsList( paramsList, &setValueParams.header, kAugmentedMethod_SetValue );

	CoronaObjectOnFinalizeParams finalizeParams = {};

	finalizeParams.action = []( const CoronaDisplayObject *, void * userData )
	{
		delete static_cast< InstancedDepthClearData * >( userData );
	};

	AddToParamsList( paramsList, &finalizeParams.header, kAugmentedMethod_OnFinalize );

	sharedData->params.useRef = true;
	sharedData->params.u.ref = CoronaObjectsBuildMethodStream( L, paramsList.next );
}

int
DepthClearObject( lua_State * L )
{
	DummyArgs( L ); // [group, ]x, y, w, h

	static int sCookie;

	auto sharedClearData = GetOrNew< SharedDepthClearData >(L, &sCookie );

	if (sharedClearData.isNew)
	{
		PopulateSharedData( L, sharedClearData.object );
	}

	InstancedDepthClearData * clearData = new InstancedDepthClearData;

	memset( clearData, 0, sizeof( InstancedDepthClearData ) );

	clearData->shared = sharedClearData.object;

	return CoronaObjectsPushRect( L, clearData, &sharedClearData.object->params );
}
