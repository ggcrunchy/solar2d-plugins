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

#include "CoronaLua.h"
#include "common.h"
#include "interface.h"
#include "state_data.h"

//
//
//

#define BRIDGE_METATABLE_NAME "metatable.luajitbridge"

//
//
//

StateData::StateData (const Interface * ifx, void * state) : mIFX(ifx), mMain(nullptr)
{
	mState = static_cast<State *>(state);
}

//
//
//

static const luaL_Reg BridgeMethods[] = {
	{
		"Do", [](lua_State * L)
		{
			StateDataLJ * dst = (StateDataLJ *)luaL_checkudata(L, 1, BRIDGE_METATABLE_NAME);
			
			/* ----- */
				
			const char * code = luaL_checkstring(L, 2);
			int ret = dst->LoadString(code); // LJ: ..., func / error
	
			/* ----- */
						
			if (ret == 0)
			{
				int nargs = lua_gettop(L) - 2;

				ret = dst->mMain->BridgeCall(dst, nargs, LUA_MULTRET);
			}

			/* ----- */
			
			bool ok = ret >= 0;

			return ok ? ret : dst->ForwardError(dst->mMain);
		}
	}, {
		"__gc", [](lua_State * L)
		{
			StateDataLJ * sd = (StateDataLJ *)luaL_checkudata(L, 1, BRIDGE_METATABLE_NAME);

			sd->Close();

			return 0;
		}
	},
	{ nullptr, nullptr }
};

//
//
//

void StateDataLJ::New (StateData * main)
{
	StateDataLJ * sd_lj = main->Make<StateDataLJ>(); // ..., sd
	
	*sd_lj = MakeLJStateData(StateDataLJ::Create(), main);

	/* ----- */

	// During development, the prelude might end up broken, so attach the
	// metatable (with __gc) first as a sanity measure.

	if (main->NewMetatable(BRIDGE_METATABLE_NAME)) // ..., sd, mt
	{
		main->PushString("__index"); // ..., sd, mt, "__index"
		main->PushValue(-2); // ..., sd, mt, "__index", mt
		main->RawSet(-3); // ..., sd, mt = { __index = mt }
		
		/* ----- */

		luaL_register((lua_State *)main->mState, nullptr, BridgeMethods);
	}

	main->SetMetatable(-2); // ..., sd; sd.metatable = mt

	/* ----- */

	bool ok = sd_lj->InstallPrelude(); // LJ: (empty) / error
	
	/* ----- */
	
	if (ok)
	{
		main->PushLightUserdata(KEY_GETTER(WeakValuedLookup)()); // ..., sd, wvl_key
		main->RawGet(LUA_REGISTRYINDEX); // ..., sd, weak_valued_lookup
		main->PushLightUserdata(sd_lj); // ..., sd, weak_valued_lookup, sd_ptr
		main->PushValue(-3); // ..., sd, weak_valued_lookup, sd_ptr, sd
		main->RawSet(-3); // ..., sd, weak_valued_lookup = { ..., [sd_ptr] = sd }
		main->Pop(1); // ..., sd
	}

	/* ----- */

	else sd_lj->ForwardError(main); // LJ: (empty); THIS: ..., sd[, error]
}

//
//
//

void BoxedGCObject::DropRefs (void)
{
	mOwner->Unref(mObjectRef);

	/* ----- */

	if (!mOwner->IsMain()) mOwner->mMain->Unref(mOwnerRef);
}

//
//
//

bool StateData::IsMain (void) const
{
	return !mMain;
}

//
//
//

bool StateData::Owns (const BoxedGCObject * obj) const
{
	return obj && obj->mOwner == this;
}
