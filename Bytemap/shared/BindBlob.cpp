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

#include "CoronaGraphics.h"
#include "Bytes.h"
#include "utils/Blob.h"
#include "utils/LuaEx.h"

// Bind a blob to the bytemap (or unbind)
int Bytemap_BindBlob (lua_State * L)
{
	Bytemap * bmap = LuaXS::DualUD<Bytemap>(L, 1, BYTEMAP_NAME);

	if (bmap)
	{
		int old_ref = bmap->mBlobRef;

		// If a blob is provided, register it. If no blob was already bound, the bytemap
		// is switching memory modes, so release the normal memory.
		if (BlobXS::IsBlob(L, 2))
		{
			lua_settop(L, 2);	// bmap, blob

			if (old_ref == LUA_NOREF) bmap->mBytes.clear();

			bmap->mBlobRef = lua_ref(L, 1);	// bmap; registry = { ..., [ref] = blob }
		}

		// When a previous blob was bound, detach and return it. If no new blob was bound,
		// the bytemap reverts to normal memory.
		if (old_ref != LUA_NOREF)
		{
			if (bmap->mBlobRef != old_ref)
			{
				lua_getref(L, old_ref);	// bmap[, nil], old_blob

				bmap->DetachBlob(&old_ref);
			}

			else
			{
				bmap->InitializeBytes();
				bmap->PushBlob();	// bmap[, nil], old_blob
				bmap->DetachBlob();
			}

			return 1;
		}
	}

	// Return nil on failure or if no blob was previously bound.
	return LuaXS::PushArgAndReturn(L, LuaXS::Nil{});// bmap[, blob?], nil
}

// Deallocate bytemap memory
int Bytemap_Deallocate (lua_State * L)
{
	Bytemap * bmap = LuaXS::DualUD<Bytemap>(L, 1, BYTEMAP_NAME);

	if (bmap)
	{
		lua_settop(L, 1);	// bmap

		if (bmap->mDummyRef == LUA_NOREF)
		{
			BlobXS::NewBlob(L, 0U);	// bmap, dummy

			lua_pushvalue(L, -1);	// bmap, dummy, dummy

			bmap->mDummyRef = lua_ref(L, 1);// bmap, dummy; registry = { [dummy_ref] = dummy }
		}

		else lua_getref(L, bmap->mDummyRef);// bmap, dummy
	}

	return Bytemap_BindBlob(L);	// bmap, old_blob?
}