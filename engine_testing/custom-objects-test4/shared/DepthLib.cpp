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
#include <math.h>

#define MATRIX_MT_NAME "graphics.matrix"

static CoronaObjectParams *
PopulateSharedState( lua_State * L )
{
    static CoronaObjectParams sParams;
    
    if (!sParams.useRef)
    {
        CoronaObjectParamsHeader paramsList = {};

        DisableCullAndHitTest( paramsList );

        CoronaObjectOnCreateParams onCreateParams = {};

        onCreateParams.action = []( const CoronaDisplayObject * object, void ** )
        {
            CoronaObjectSetHasDummyStageBounds( object, true );
        };

        AddToParamsList( paramsList, &onCreateParams.header, kAugmentedMethod_OnCreate );

        sParams.useRef = true;
        sParams.u.ref = CoronaObjectsBuildMethodStream( L, paramsList.next );
    }
    
    return &sParams;
}

static int
TransformableMesh( lua_State * L )
{
    CoronaObjectParams * params = PopulateSharedState( L );

	return CoronaObjectsPushMesh( L, NULL, params );
}

static int
TransformablePolygon( lua_State * L )
{
    CoronaObjectParams * params = PopulateSharedState( L );

	return CoronaObjectsPushPolygon( L, NULL, params );
}

static int NewMatrix( lua_State * L )
{
    lua_newuserdata( L, sizeof( CoronaMatrix4x4 ) ); // matrix
    
    if (luaL_newmetatable( L, MATRIX_MT_NAME )) // matrix, mt
    {
        lua_pushvalue( L, -1 ); // mt, mt
        lua_setfield( L, -2, "__index" ); // mt; mt = { __index = mt }
        
        luaL_Reg methods[] = {
            {
                "getAsArray", []( lua_State * L )
                {
                    CoronaMatrix4x4 * matrix = (CoronaMatrix4x4 *)luaL_checkudata( L, 1, MATRIX_MT_NAME );
                    
                    lua_createtable( L, 16, 0 ); // matrix, arr
                    
                    for (int i = 1; i <= 16; ++i)
                    {
                        lua_pushnumber( L, (*matrix)[i - 1] ); // matrix, arr, comp
                        lua_rawseti( L, -2, i ); // ..., arr = { ..., comp }
                    }
                    
                    return 1;
                }
            }, {
                "populateFromEulerAngles", []( lua_State * L )
                {
                    CoronaMatrix4x4 * matrix = (CoronaMatrix4x4 *)luaL_checkudata( L, 1, MATRIX_MT_NAME );
                    
                    const char * angleNames[] = { "yaw", "pitch", "roll" };
                    float eulerAngles[3];
                    
                    luaL_checktype( L, 2, LUA_TTABLE );

                    for (int i = 0; i < 3; ++i)
                    {
                        lua_getfield( L, 2, angleNames[i] ); // matrix, args, value
                        
                        eulerAngles[i] = luaL_optnumber( L, -1, 0 ) * (3.141592653589793 / 180.);
                        
                        lua_pop( L, 1 ); // matrix, args
                    }
                    
                    CoronaMatrix4x4 rotations[3] = {}, & X = rotations[0], & Y = rotations[1], & Z = rotations[2];
                    float c1 = cos( eulerAngles[2] ), s1 = sin( eulerAngles[2] );
                    float c2 = cos( eulerAngles[0] ), s2 = sin( eulerAngles[0] );
                    float c3 = cos( eulerAngles[1] ), s3 = sin( eulerAngles[1] );
                    int i1, i2, i3;

                /*
                 |  0  1  2  3 |
                 |  4  5  6  7 |
                 |  8  9 10 11 |
                 | 12 13 14 15 |
                */

                    X[15] = Y[15] = Z[15] = 1.f;
                    X[0] = Y[5] = Z[10] = 1.f;

                    X[5] = X[10] = c1;
                    X[6] = -s1;
                    X[9] = +s1;

                    Y[0] = Y[10] = c2;
                    Y[2] = +s2;
                    Y[8] = -s2;

                    Z[0] = Z[5] = c3;
                    Z[1] = -s3;
                    Z[4] = +s3;
                    
                    // ordered to usefully default when zeroed out:
                    enum EulerAnglesConvention { kZYX, kXYZ, kXZY, kYXZ, kYZX, kZXY };
                    
                    lua_getfield( L, 2, "convention" ); // matrix, args, convention?
                    
                    const char * conventionNames[] = { "zyx", "xyz", "xzy", "yxz", "yzx", "zxy", NULL };
                    int index = luaL_checkoption( L, -1, "zyx", conventionNames );
                    
                    switch ((EulerAnglesConvention)index)
                    {
                    case kXYZ:
                        i1 = 0; i2 = 1; i3 = 2;

                        break;
                    case kXZY:
                        i1 = 0; i2 = 2; i3 = 1;

                        break;
                    case kYXZ:
                        i1 = 1; i2 = 0; i3 = 2;

                        break;
                    case kYZX:
                        i1 = 1; i2 = 2; i3 = 0;

                        break;
                    case kZXY:
                        i1 = 2; i2 = 0; i3 = 1;

                        break;
                    case kZYX:
                        i1 = 2; i2 = 1; i3 = 0;

                        break;
                    }

                    CoronaMatrix4x4 m23;

                    CoronaMultiplyMatrix4x4( rotations[i2], rotations[i3], m23 );
                    CoronaMultiplyMatrix4x4( rotations[i1], m23, *matrix );
                    
                    return 0;
                }
            }, {
                "populateIdentity", []( lua_State * L )
                {
                    CoronaMatrix4x4 * matrix = (CoronaMatrix4x4 *)luaL_checkudata( L, 1, MATRIX_MT_NAME );
                    
                    memset( *matrix, 0, sizeof( CoronaMatrix4x4 ) );

                    (*matrix)[0] = (*matrix)[5] = (*matrix)[10] = (*matrix)[15] = 1.f;
                    
                    return 0;
                }
            }, {
                "populatePerspective", []( lua_State * L )
                {
                    CoronaMatrix4x4 * matrix = (CoronaMatrix4x4 *)luaL_checkudata( L, 1, MATRIX_MT_NAME );
                    
                    const char * names[] = { "fovy", "aspectRatio", "zNear", "zFar" };
                    float args[4] = {};

                    luaL_checktype( L, 2, LUA_TTABLE );
                    
                    for (int i = 0; i < 4; ++i)
                    {
                        lua_getfield( L, 2, names[i] ); // matrix, args, value

                        args[i] = luaL_checknumber( L, -1 );

                        lua_pop( L, 1 ); // matrix, args
                    }
                    
                    CoronaCreatePerspectiveMatrix( args[0], args[1], args[2], args[3], *matrix );
                    
                    return 0;
                }
            }, {
                "populateView", []( lua_State * L )
                {
                    CoronaMatrix4x4 * matrix = (CoronaMatrix4x4 *)luaL_checkudata( L, 1, MATRIX_MT_NAME );
                    
                    const char * names[] = { "eye", "center", "up" };
                    CoronaVector3 vecs[3];

                    for (int i = 0; i < 3; ++i)
                    {
                        lua_getfield( L, 2, names[i] ); // matrix, args, vec
                        luaL_checktype( L, -1, LUA_TTABLE );

                        for (int j = 1; j <= 3; ++j)
                        {
                            lua_rawgeti( L, -1, j ); // matrix, args, vec, comp

                            vecs[i][j - 1] = luaL_checknumber( L, -1 );

                            lua_pop( L, 1 ); // matrix, args, vec
                        }
                        
                        lua_pop( L, 1 ); // matrix, args
                    }
                    
                    CoronaCreateViewMatrix( vecs[0], vecs[1], vecs[2], *matrix );
                    
                    return 0;
                }
            }, {
                "setProduct", []( lua_State * L )
                {
                    CoronaMatrix4x4 * matrix = (CoronaMatrix4x4 *)luaL_checkudata( L, 1, MATRIX_MT_NAME );
                    CoronaMatrix4x4 * m1 = (CoronaMatrix4x4 *)luaL_checkudata( L, 2, MATRIX_MT_NAME );
                    CoronaMatrix4x4 * m2 = (CoronaMatrix4x4 *)luaL_checkudata( L, 3, MATRIX_MT_NAME );
                    
                    CoronaMultiplyMatrix4x4( *m1, *m2, *matrix );
                    
                    return 0;
                }
            },
            { NULL, NULL }
        };
        
        luaL_register( L, NULL, methods );
    }
    
    lua_setmetatable( L, -2 ); // matrix; matrix.metatable = mt
    
    return 1;
}

void AddDepthFuncs( lua_State * L )
{
	luaL_Reg funcs[] = {
		{ "newDepthClearObject", DepthClearObject },
		{ "newDepthStateObject", DepthStateObject },
        { "newMatrix", NewMatrix },
		{ "newTransformableMesh", TransformableMesh },
		{ "newTransformablePolygon", TransformablePolygon },
		{ NULL, NULL }
	};

	luaL_register( L, NULL, funcs );
}
