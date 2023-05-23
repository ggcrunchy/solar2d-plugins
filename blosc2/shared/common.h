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

//
//
//

#define HAVE_ZLIB_NG
#define HAVE_ZSTD

#include "blosc2.h"
#include "CoronaLua.h"
#include "CoronaMemory.h"
#include "utils/LuaEx.h"

//
//
//

#define BLOSC2_SCRATCH "blosc2.scratch"
#define BLOSC2_MIN_SCRATCH_SIZE 4096

//
//
//

struct Output {
    void * memory;
    int32_t size;
    bool provided;
};

const void * GetInputMemory (lua_State * L, int arg, int32_t * size);
Output GetOutputMemory (lua_State * L, int arg);
int PushOkOrError (lua_State * L, int result);
int PushSizeOrError (lua_State * L, int32_t size, const Output & out);

//
//
//

int PushInt64OrError (lua_State * L, int64_t v);
int64_t ReadInt64 (lua_State * L, int arg);

//
//
//

template<typename T>
class ParamsBox {
public:
    ParamsBox (T * params) : mMustFree(true)
    {
        mRef = params;
    }

    ParamsBox (const T & params) : mMustFree(false)
    {
        mOwned = params;
    }

    //
    //
    //

    T * Get (void) { return mMustFree ? mRef : &mOwned; }

    //
    //
    //

    void Free (void)
    {
        if (!mMustFree) return;
        
        free(mRef);

        mRef = nullptr;
    }

    //
    //
    //

private:
	bool mMustFree;
    union {
        T * mRef;
        T mOwned;
    };
};

//
//
//

blosc2_cparams * GetCparams (lua_State * L, int arg = 1);
blosc2_dparams * GetDparams (lua_State * L, int arg = 1);
blosc2_context * GetContext (lua_State * L, int arg = 1);

void WrapCparams (lua_State * L, blosc2_cparams * cparams);
void WrapDparams (lua_State * L, blosc2_dparams * dparams);
void WrapSchunk (lua_State * L, blosc2_schunk * schunk);

//
//
//

void WeakKeyPair (lua_State * L, int karg, int varg);
void GetValueFromWeakKey (lua_State * L, int arg);
void GetSchunk (lua_State * L, int parg, void * schunk);

//
//
//

// static char *print_error(int rc);

void AddCore (lua_State * L);
void AddContext (lua_State * L);
void AddArray (lua_State * L);
void AddCparams(lua_State * L);
void AddDparams (lua_State * L);
void AddFrame (lua_State * L);
void AddMetalayer (lua_State * L);
void AddSchunk (lua_State * L);