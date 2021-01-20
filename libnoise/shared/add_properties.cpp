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

//
//
//

#define PROPERTY(name) #name, noise::module::##name

//
//
//

void AddProperties (lua_State * L)
{
	lua_newtable(L);// properties

	//
	//
	//

	typedef struct {
		const char * name;
		double value;
	} Double;

	Double doubles[] = {
		PROPERTY(DEFAULT_BILLOW_FREQUENCY),
		PROPERTY(DEFAULT_BILLOW_LACUNARITY),
		PROPERTY(DEFAULT_BILLOW_PERSISTENCE),
		PROPERTY(DEFAULT_CONST_VALUE),
		PROPERTY(DEFAULT_CYLINDERS_FREQUENCY),
		PROPERTY(DEFAULT_EXPONENT),
		PROPERTY(DEFAULT_PERLIN_FREQUENCY),
		PROPERTY(DEFAULT_PERLIN_LACUNARITY),
		PROPERTY(DEFAULT_PERLIN_PERSISTENCE),
		PROPERTY(DEFAULT_RIDGED_FREQUENCY),
		PROPERTY(DEFAULT_RIDGED_LACUNARITY),
		PROPERTY(DEFAULT_ROTATE_X),
		PROPERTY(DEFAULT_ROTATE_Y),
		PROPERTY(DEFAULT_ROTATE_Z),
		PROPERTY(DEFAULT_BIAS),
		PROPERTY(DEFAULT_SCALE),
		PROPERTY(DEFAULT_SCALE_POINT_X),
		PROPERTY(DEFAULT_SCALE_POINT_Y),
		PROPERTY(DEFAULT_SCALE_POINT_Z),
		PROPERTY(DEFAULT_SELECT_EDGE_FALLOFF),
		PROPERTY(DEFAULT_SELECT_LOWER_BOUND),
		PROPERTY(DEFAULT_SELECT_UPPER_BOUND),
		PROPERTY(DEFAULT_SPHERES_FREQUENCY),
		PROPERTY(DEFAULT_TRANSLATE_POINT_X),
		PROPERTY(DEFAULT_TRANSLATE_POINT_Y),
		PROPERTY(DEFAULT_TRANSLATE_POINT_Z),
		PROPERTY(DEFAULT_TURBULENCE_FREQUENCY),
		PROPERTY(DEFAULT_TURBULENCE_POWER),
		PROPERTY(DEFAULT_VORONOI_DISPLACEMENT),
		PROPERTY(DEFAULT_VORONOI_FREQUENCY),
		{ nullptr, 0.0 }
	};

	for (int i = 0; doubles[i].name; ++i)
	{
		lua_pushnumber(L, doubles[i].value);// properties, value
		lua_setfield(L, -2, doubles[i].name);	// properties = { ..., name = value }
	}

	//
	//
	//

	typedef struct {
		const char * name;
		int value;
	} Integer;

	Integer integers[] = {
		PROPERTY(DEFAULT_BILLOW_OCTAVE_COUNT),
		PROPERTY(DEFAULT_BILLOW_SEED),
		PROPERTY(BILLOW_MAX_OCTAVE),
		PROPERTY(DEFAULT_PERLIN_OCTAVE_COUNT),
		PROPERTY(DEFAULT_PERLIN_SEED),
		PROPERTY(PERLIN_MAX_OCTAVE),
		PROPERTY(DEFAULT_RIDGED_OCTAVE_COUNT),
		PROPERTY(DEFAULT_RIDGED_SEED),
		PROPERTY(RIDGED_MAX_OCTAVE),
		PROPERTY(DEFAULT_TURBULENCE_ROUGHNESS),
		PROPERTY(DEFAULT_TURBULENCE_SEED),
		PROPERTY(DEFAULT_VORONOI_SEED),
		{ nullptr, 0 }
	};

	for (int i = 0; integers[i].name; ++i)
	{
		lua_pushinteger(L, integers[i].value);	// properties, value
		lua_setfield(L, -2, integers[i].name);	// properties = { ..., name = value }
	}

	//
	//
	//

	const char * names[] = {
		"DEFAULT_BILLOW_QUALITY", "DEFAULT_PERLIN_QUALITY", "DEFAULT_RIDGED_QUALITY", nullptr
	};

	for (int i = 0; names[i]; ++i)
	{
		lua_pushliteral(L, "STD");	// properties, "STD"
		lua_setfield(L, -2, names[i]);	// properties = { ..., name = "STD" }
	}

	//
	//
	//

	lua_pushcclosure(L, [](lua_State * L) {
		lua_settop(L, 1);	// name
		lua_gettable(L, lua_upvalueindex(1));	// value

		return 1;
	}, 1);	// libnoise, GetProperty
	lua_setfield(L, -2, "GetProperty");	// libnoise = { ..., GetProperty = GetProperty }
}