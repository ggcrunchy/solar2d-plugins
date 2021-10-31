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

#pragma once

#include "CoronaGraphics.h"
#include "CoronaObjects.h"
#include "CoronaLog.h"
#include "CoronaLua.h"

#include <string>
#include <vector>

#include "utils.h"

struct DepthSettings {
	int func{GL_LESS}, cullFace{GL_BACK}, frontFace{GL_CCW};
	double near{0.}, far{1.};
	bool cullFaceEnabled{false}, enabled{false}, mask{true};
};

struct DepthInfo {
	double clear{1.};
	CoronaMatrix4x4 projectionMatrix, viewMatrix;
	DepthSettings settings;
};

struct DepthEnvironment {
    static void EndFrame( const CoronaRenderer * renderer, void * userData );
    
	DepthInfo current, working;
	std::vector<DepthInfo> stack;
    unsigned long clearOpID{0};
    unsigned long commandID{0};
	unsigned int id;
	double clear{1.}; // TODO: should add some way to set this, too...
	bool anySinceClear{false};
	bool hasSetID{false};
};

DepthEnvironment * InitDepthEnvironment( lua_State * L );

int DepthClearObject( lua_State * L );
int DepthStateObject( lua_State * L );

void AddDepthFuncs( lua_State * L );
