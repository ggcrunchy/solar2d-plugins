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

BlobXS::BlobPimpl * GetBlobPimpl (void)
{
	struct MemoryBlobImpl : public BlobXS::BlobPimpl {
		// Methods
		bool IsBlob (lua_State * L, int arg, const char * type) override { return MemoryBlob::IsBlob(L, arg, type); }
		bool IsLocked (lua_State * L, int arg, void * key) override { return MemoryBlob::IsLocked(L, arg, key); }
		bool IsResizable (lua_State * L, int arg) override { return MemoryBlob::IsResizable(L, arg); }
		bool Lock (lua_State * L, int arg, void * key) override { return MemoryBlob::Lock(L, arg, key); }
        bool Unlock (lua_State * L, int arg, void * key) override { return MemoryBlob::Unlock(L, arg, key); }
        bool Resize (lua_State * L, int arg, size_t size, void * key = nullptr) override { return MemoryBlob::Resize(L, arg, size, key); }
        bool WhitelistUser (lua_State * L, int arg, void * key, void * user_key) override { return MemoryBlob::WhitelistUser(L, arg, key, user_key); }
		size_t GetAlignment (lua_State * L, int arg) override { return MemoryBlob::GetAlignment(L, arg); }
		size_t GetSize (lua_State * L, int arg, bool bNSized) override { return MemoryBlob::GetSize(L, arg, bNSized); }
		unsigned char * GetData (lua_State * L, int arg) override { return MemoryBlob::GetData(L, arg); }
		void * GetVector (lua_State * L, int arg) override { return MemoryBlob::GetVector(L, arg); }

		void NewBlob (lua_State * L, size_t size, const BlobXS::CreateOpts * opts) override { MemoryBlob::NewBlob(L, size, opts); }

		storage_id Submit (lua_State * L, int arg, void * key = nullptr) override { return MemoryBlob::Submit(L, arg, key); }
		storage_id GetID (lua_State * L, int arg) override { return MemoryBlob::GetID(L, arg); }
		bool Exists (storage_id id) override { return MemoryBlob::Exists(id); }
		bool Sync (lua_State * L, int arg, storage_id id, void * key = nullptr) override { return MemoryBlob::Sync(L, arg, id, key); }
	};

	static MemoryBlobImpl memblob_impl;

	return &memblob_impl;
}
