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

#include "CoronaLua.h"
#include "qu3e/q3.h"

q3Mat3 & Matrix (lua_State * L, int arg = 1);
q3Quaternion & Quaternion (lua_State * L, int arg = 1);
q3Transform & Transform (lua_State * L, int arg = 1);
q3Vec3 & Vector (lua_State * L, int arg = 1);

void AddVectorRef (lua_State * L, q3Vec3 * vec);

q3AABB & AABB (lua_State * L, int arg = 1);
q3HalfSpace & HalfSpace (lua_State * L, int arg = 1);
q3RaycastData & Raycast (lua_State * L, int arg = 1);

q3BodyDef & BodyDef (lua_State * L, int arg = 1);

q3Body ** BodyBox (lua_State * L, int arg);
const q3Box ** BoxBox (lua_State * L, int arg);

void PutBodyInBox (lua_State * L, q3Body * body, bool bConst = false);
void PutBoxInBox (lua_State * L, const q3Box * box);

int NewMatrix (lua_State * L, const q3Mat3 & mat);
int NewQuaternion (lua_State * L, const q3Quaternion & quat);
int NewTransform (lua_State * L, const q3Transform & xform);
int NewVector (lua_State * L, const q3Vec3 & vec);

void open_common (lua_State * L);
void open_dynamics (lua_State * L);
void open_math (lua_State * L);
void open_scene (lua_State * L);
