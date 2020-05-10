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

#include "MemoryBlob.h"
#include "Macros.h"
#include "utils/LuaEx.h"
#include "utils/Memory.h"
#include <algorithm>

void * BlobProps::GetKey (void)
{
	return GetBlobPimpl(); // something relatively unique to this module
}

static bool AuxIsBlob (lua_State * L, int arg)
{
	if (lua_type(L, arg) != LUA_TUSERDATA) return false;

	lua_pushvalue(L, arg);	// ..., blob?
	lua_pushlightuserdata(L, BlobProps::GetKey());	// ..., blob?, props_key
	lua_rawget(L, LUA_REGISTRYINDEX);	// ..., blob?, props

	if (lua_istable(L, -1))
	{
		lua_insert(L, -2);	// ..., props, blob?
		lua_rawget(L, -2);	// ..., props, prop?

		return lua_type(L, -1) == LUA_TUSERDATA;
	}

	return false;
}

bool BlobProps::HasUpdatePrivileges (void * key)
{
	if (key == mKey) return true;

	auto last = mWhitelist.end();

	return std::find(mWhitelist.begin(), last, key) != last;
}

BlobPropViewer::BlobPropViewer (lua_State * L, int arg, bool bLeaveTop) : mL{L}
{
	mArg = CoronaLuaNormalize(L, arg);

	if (!bLeaveTop) mTop = lua_gettop(L);
	if (AuxIsBlob(L, arg)) mProps = LuaXS::UD<BlobProps>(L, -1);// ...[, props, prop]
}

BlobPropViewer::~BlobPropViewer (void)
{
	if (mTop >= 0) lua_settop(mL, mTop);
}

bool MemoryBlob::IsBlob (lua_State * L, int arg, const char * type)
{
	if (type && !LuaXS::IsType(L, type, arg)) return false;

	int top = lua_gettop(L);
	bool bIsBlob = AuxIsBlob(L, arg);

	lua_settop(L, top);	// ...

	return bIsBlob;
}

bool MemoryBlob::IsLocked (lua_State * L, int arg, void * key)
{
	BlobPropViewer bpv{L, arg};

	if (!bpv.mProps) return false;
	
	return !bpv.mProps->HasUpdatePrivileges(key);
}

bool MemoryBlob::IsResizable (lua_State * L, int arg)
{
	 return GetVector(L, arg) != nullptr;
}

bool MemoryBlob::Lock (lua_State * L, int arg, void * key)
{
	BlobPropViewer bpv{L, arg};

	if (!bpv.mProps || !key) return false;
	if (bpv.mProps->mKey) return bpv.mProps->mKey == key;

	bpv.mProps->mKey = key;

	return true;
}

bool MemoryBlob::Unlock (lua_State * L, int arg, void * key)
{
	BlobPropViewer bpv{L, arg};

	if (!bpv.mProps || !key || !bpv.mProps->mKey) return false;
	if (bpv.mProps->mKey != key) return false;

	bpv.mProps->mKey = nullptr;

	return true;
}

bool MemoryBlob::Resize (lua_State * L, int arg, size_t size, void * key)
{
	BlobPropViewer bpv{L, arg};

	if (!bpv.mProps || !bpv.mProps->mResizable || !bpv.mProps->HasUpdatePrivileges(key)) return false;

	AuxResize(L, bpv.mProps->mAlign, arg, size);

	return true;
}

bool MemoryBlob::WhitelistUser (lua_State * L, int arg, void * key, void * user_key)
{
	BlobPropViewer bpv{L, arg};

	if (!bpv.mProps || !key || !user_key || key != bpv.mProps->mKey) return false;
	
	auto last = bpv.mProps->mWhitelist.end();

	if (std::find(bpv.mProps->mWhitelist.begin(), last, user_key) == last) bpv.mProps->mWhitelist.push_back(user_key);

	return true;
}

size_t MemoryBlob::GetAlignment (lua_State * L, int arg)
{
	BlobPropViewer bpv{L, arg};

	return bpv.mProps ? bpv.mProps->mAlign : 0U;
}

size_t MemoryBlob::GetSize (lua_State * L, int arg, bool bNSized)
{
	BlobPropViewer bpv{L, arg};

	if (!bpv.mProps) return 0U;

	size_t size;

	if (bpv.mProps->mResizable)
	{
		WITH_ALIGNMENT_DO(bpv.mProps->mAlign, {
			size = GetVectorN<VA>(L, bpv.mArg)->size();
		})
	}

	else size = lua_objlen(L, bpv.mArg) - bpv.mProps->mShift;

	if (bNSized && bpv.mProps->mAlign) size /= bpv.mProps->mAlign;

	return size;
}

unsigned char * MemoryBlob::GetData (lua_State * L, int arg)
{
	BlobPropViewer bpv{L, arg};

	if (!bpv.mProps) return nullptr;

	unsigned char * data;

	if (bpv.mProps->mResizable)
	{
		WITH_ALIGNMENT_DO(bpv.mProps->mAlign, {
			data = GetVectorN<VA>(L, bpv.mArg)->data();
		})
	}

	else
	{
		void * ud = lua_touserdata(L, bpv.mArg);

		if (bpv.mProps->mAlign) MemoryXS::Align(bpv.mProps->mAlign, lua_objlen(L, bpv.mArg), ud);

		data = static_cast<unsigned char *>(ud);
	}

	return data;
}

void * MemoryBlob::GetVector (lua_State * L, int arg)
{
	BlobPropViewer bpv{L, arg};

	if (!bpv.mProps || !bpv.mProps->mResizable) return nullptr;

	return lua_touserdata(L, bpv.mArg);
}
