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
#include "interface.h"
#include "state_data.h"

//
//
//

static MAKE_KEY_GETTER(LuaProxyMT);
static MAKE_KEY_GETTER(MetamethodShim);

//
//
//

static int ShimCall (StateData * self)
{
	BoxedGCObject * obj = (BoxedGCObject *)self->ToUserdata(3);
	StateData * owner = obj->mOwner;
	
	/* ----- */
	
	size_t len;
	const char * name = self->ToLstring(2, &len);

	owner->PushShimFunction(name, len); // OWNER: ..., func
	
	/* ----- */
	
	int nargs = self->GetTop() - 2;
	int nrets = (int)self->ToInteger(1);

	return self->BridgeCall(owner, nargs, nrets); // CURRENT: ..., results / error
}

//
//
//

static int CheckCompatibility (StateData * self)
{
	self->GetMetatable(1); // obj1, obj2, mt1
	self->GetMetatable(2); // obj1, obj2, mt1, mt2
	self->ArgCheck(self->RawEqual(-2, -1), 1, "Binary op expects two object proxies");

	BoxedGCObject * obj1 = (BoxedGCObject *)self->ToUserdata(1);
	BoxedGCObject * obj2 = (BoxedGCObject *)self->ToUserdata(2);

	self->ArgCheck(obj1->mOwner == obj2->mOwner, 1, "Binary op expects objects to originate from same state");

	return 0;
}

//
//
//

static int CollectBoxedGCObject (StateData * self)
{
	BoxedGCObject * obj = (BoxedGCObject *)self->ToUserdata(1);

	obj->DropRefs();

	return 0;
}

//
//
//

bool StateData::InstallPrelude (void)
{
	if (!LoadCode()) return false; // ..., func / error

	/* ----- */

	PushCFunc(ShimCall); // ..., func, ShimCall
	PushCFunc(CheckCompatibility); // ..., func, ShimCall, CheckCompatibilty

	if (Pcall(2, 2, 0) != 0) return false; // ..., (proxy_mt, mm_shim) / error

	/* ----- */

	PushLightUserdata(KEY_GETTER(MetamethodShim)()); // ..., proxy_mt, mm_shim, mms_key
	Insert(-2); // ..., proxy_mt, mms_key, mm_shim
	RawSet(kREGISTRYINDEX); // ..., proxy_mt; registry[mms_key] = mm_shim

	/* ----- */

	PushLightUserdata(KEY_GETTER(LuaProxyMT)()); // ..., proxy_mt, lpmt_key
	Insert(-2); // ..., lpmt_key, proxy_mt
	PushString("__gc"); // ..., lpmt_key, proxy_mt, __gc
	PushCFunc(CollectBoxedGCObject); // ..., lpmt_key, proxy_mt, __gc, CollectBoxedGCObject
	RawSet(-3); // ..., lpmt_key, proxy_mt = { ..., __gc = CollectBoxedGCObject }
	RawSet(kREGISTRYINDEX); // ...; registry[lpmt_key] = proxy_mt

	/* ----- */

	return true;
}

//
//
//

void StateData::CopyStringTo (StateData * dst, int index)
{
	size_t len;
	const char * s = ToLstring(index, &len);
	
	dst->PushLstring(s, len); // DST: ..., s
}

//
//
//

const BoxedGCObject * StateData::GetBoxedGCObject (int index)
{
	int old_top = GetTop();
	void * ud = ToUserdata(index);

	/* ----- */

	if (ud && GetMetatable(index)) // ..., object, ..., mt1?
	{
		PushLightUserdata(KEY_GETTER(LuaProxyMT)()); // ..., object, ..., mt1, lpmt_key
		RawGet(kREGISTRYINDEX); // ..., object, ..., mt1, proxy_mt
	}
	
	/* ----- */

	bool has_mt = GetTop() == old_top + 2 && RawEqual(-1, -2);

	/* ----- */

	SetTop(old_top); // ..., object, ...

	/* ----- */

	return has_mt ? (const BoxedGCObject *)ud : nullptr;
}

//
//
//

BoxedGCObject * BoxedGCObject::New (StateData * dst)
{
	BoxedGCObject * boxed = dst->Make<BoxedGCObject>(); // ..., boxed

	/* ----- */

	dst->PushLightUserdata(KEY_GETTER(LuaProxyMT)()); // ..., boxed, lpmt_key
	dst->RawGet(kREGISTRYINDEX); // ..., boxed, proxy_mt
	dst->SetMetatable(-2); // ..., boxed; box.metatatable = proxy_mt

	/* ----- */

	return boxed;
}

//
//
//

void BoxedGCObject::ResolveOwner (void)
{
	StateData * main = mOwner->mMain;
	
	PushWeakValuedData(main, mOwner); // MAIN: ..., owner
	
	mOwnerRef = main->Ref(); // MAIN: ...; registry[ref] = owner
}

//
//
//

void StateData::PushGCObject (StateData * dst, int index)
{
	const BoxedGCObject * boxed = GetBoxedGCObject(index);

	/* ----- */
	
	if (dst->Owns(boxed)) dst->GetRef(boxed->mObjectRef); // DST: ..., obj
	
	/* ----- */
	
	else
	{
		BoxedGCObject * new_box = BoxedGCObject::New(dst); // DST: ..., box

		/* ----- */
		
		if (boxed)
		{
			StateData * owner = boxed->mOwner;

			owner->GetRef(boxed->mObjectRef); // OWNER: ..., obj		

			new_box->mObjectRef = owner->Ref(); // OWNER: ...; registry[ref] = obj
			new_box->mOwner = owner;
		}
		
		/* ----- */
		
		else
		{
			PushValue(index); // ..., obj, ..., obj
			
			new_box->mObjectRef = Ref(); // ..., obj, ...; registry[ref] = obj
			new_box->mOwner = this;
		}

		/* ----- */

		if (!new_box->mOwner->IsMain()) new_box->ResolveOwner();
	}
}

//
//
//

void StateData::PushToOtherStack (StateData * dst, int n)
{
	for (int top = GetTop(), i = top - n + 1; i <= top; ++i)
	{
		switch (Type(i))
		{
		case kTNIL:
			dst->PushNil(); // DST: ..., nil
			
			break;
		case kTBOOLEAN:
			dst->PushBoolean(ToBoolean(i)); // DST: ..., b

			break;
		case kTLIGHTUSERDATA:
			dst->PushLightUserdata(ToUserdata(i)); // DST: ..., p

			break;
		case kTNUMBER:
			dst->PushNumber(ToNumber(i)); // DST: ..., n
			
			break;
		case kTSTRING:
			CopyStringTo(dst, i); // DST: .., s
		
			break;
		default: // other gc object
			PushGCObject(dst, i); // DST: ..., boxed
	
			break;
		}
	}
}

//
//
//

void StateData::PushShimFunction (const char * name, size_t len)
{
	PushLightUserdata(KEY_GETTER(MetamethodShim)()); // ..., mms_key
	RawGet(kREGISTRYINDEX); // ..., mm_shim

	/* ----- */
	
	PushLstring(name, len); // ..., mm_shim, name

	/* ----- */
	
	RawGet(-2); // ..., mm_shim, func
	Remove(-2); // ..., func
}

//
//
//

int StateData::BridgeCall (StateData * dst, int nargs, int nrets)
{
	int top_dst = dst->GetTop() - 1; // func already on stack
	
	PushToOtherStack(dst, nargs); // DST: ..., func[, args]

	/* ----- */

	int ret = dst->Pcall(nargs, kMULTRET, 0); // DST: ..., results / error

	if (ret != 0) return -1; // forward any error

	/* ----- */

	int nresults = dst->GetTop() - top_dst;

	if (nrets != nresults && nrets != kMULTRET)
	{
		dst->SetTop(top_dst + nrets); // DST: ... (# > top = nrets)
	
		nresults = nrets;
	}

	/* ----- */

	Pop(nargs); // SRC: ...
	
	if (!CheckStack(nrets))
	{
		dst->SetTop(top_dst); // DST: ...
		dst->PushString("Too many return values for stack"); // DST: error
		
		return -1;
	}

	/* ----- */
	
	dst->PushToOtherStack(this, nresults); // SRC: ...[, results]
	dst->Pop(nresults); // DST: ...

	/* ----- */
	
	return nresults;
}

//
//
//

int StateData::ForwardError (StateData * dst)
{
	if (Type(-1) == kTSTRING) CopyStringTo(dst, -1); // DST: ..., error
	else dst->PushString("Error"); // DST: ..., "Error"

	/* ----- */
	
	Pop(1); // SRC: ...

	/* ----- */

	return dst->Error();
}
