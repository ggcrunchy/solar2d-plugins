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
#include "CoronaLog.h"
#include "utils/LuaEx.h"
#include "soloud.h"

//
//
//

#define MT_PREFIX "soloud."
#define MT_NAME(name) MT_PREFIX #name

//
//
//

struct Options {
	bool mWantPan{false};
	bool mWantPaused{false};
	bool mWantVelocity{false};
	bool mWantVolume{false};
	bool mWantCallback{false};

	void Get (lua_State * L, int opts);

	float mPan, mVolume, mVelX, mVelY, mVelZ;
	bool mPaused, mGotCallback;
};

//
//
//

struct FloatBuffer {
	~FloatBuffer()
	{
		if (mOwnsData) delete mData;
	}

	unsigned int Size (lua_State * L, int arg);

	float * mData{nullptr};
	size_t mSize{0U};
	bool mCanWrite{false}; // n.b. if not owned
	bool mOwnsData{true};
	bool mSourceGone{false};
};

//
//
//

void add_audiosources (lua_State * L);
void add_floatbuffer (lua_State * L);
void add_core (lua_State * L);
void add_filters (lua_State * L);

//
//
//

SoLoud::AudioSource * GetAudioSource (lua_State * L, int arg = 1);
SoLoud::Soloud * GetCore (lua_State * L, int arg = 1);

SoLoud::Filter * GetFilter (lua_State * L, int arg);

FloatBuffer * GetFloatBuffer (lua_State * L, int arg = 1);
FloatBuffer * NewFloatBuffer (lua_State * L);

//
//
//

int PushHandle (lua_State * L, unsigned int handle);
unsigned int GetHandle (lua_State * L, int arg);

//
//
//

void PrepareOnComplete (lua_State * L, unsigned int id);

//
//
//

unsigned int GetAttenuationModel (lua_State * L, int arg);
unsigned int GetResampler (lua_State * L, int arg);
const char * GetResamplerString (lua_State * L, unsigned int resampler);
const char ** GetWaveformModelList (void);
int GetWaveform (lua_State * L, int arg, const char * def = nullptr);

//
//
//

int HasMetatable (lua_State * L, int arg, const char * name);

//
//
//

int Result (lua_State * L, SoLoud::result err);
int GetIndexEnum (lua_State * L, int arg, const char * const names[], const char * def = nullptr);
float OptFloat (lua_State * L, int arg, float def);

//
//
//

void SetFilterRefToEnvironment (lua_State * L, int arg, int index);
void RemoveEnvironment (lua_State * L, int arg = 1);

//
//
//

void AddToStore (lua_State * L, void * object = nullptr);
void GetFromStore (lua_State * L, void * object);
void RemoveFromStore (lua_State * L, void * object = nullptr);

//
//
//

void AddBasics (lua_State * L);
bool TryToAddPlugin (lua_State * L, const char * name);

//
//
//

void PushPluginModule (lua_State * L);
void PushFilterParameters (lua_State * L);