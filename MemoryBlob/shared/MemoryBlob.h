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

#include "utils/Blob.h"
#include <vector>

// Various per-blob properties
struct BlobProps {
	std::vector<void *> mWhitelist;	// Whitelisted keys
	size_t mAlign{0U};	// Size of which memory will be aligned to some multiple
	size_t mShift{0U};	// Align-related memory shift
	bool mResizable{false};	// Is the memory resizable?
	void * mKey{nullptr};	// If locked, the key

	bool HasUpdatePrivileges (void * key);

	static void * GetKey (void);
};

//
struct BlobPropViewer {
	BlobProps * mProps{nullptr};// Properties being viewed
	lua_State * mL;	// Current state
	int mArg, mTop{-1};	// Argument position; stack top

	BlobPropViewer (lua_State * L, int arg, bool bLeaveTop = false);
	~BlobPropViewer (void);
};

//
class BlobStateImpl : public BlobXS::State::Pimpl {
	int mArg;	// Where is the blob?
	int mW{0}, mH{0}, mBPP{0}, mStride{0};	// Interpreted width, height, bits-per-pixel, and stride
	size_t mLength{0U};	// Length of blob
	unsigned char * mData{nullptr};	// Pointer into its data
    bool mIsBlob{false};// Is this a blob?

	void Bind (lua_State * L, int arg);

public:
	bool Bound (void) const override { return mIsBlob; }
	bool Fit (lua_State * L, int x, int y, int w, int h) override;
	bool InterpretAs (lua_State * L, int w, int h, int bpp, int stride = 0) override;
	void CopyTo (void * ptr) override;
	void LoadFrom (void * ptr) override;
	void Zero (void) override;
	operator unsigned char * (void) override { return mData; }

	void Initialize (lua_State * L, int arg, const char * key, bool bLeave) override;
	bool Initialize (lua_State * L, int arg, const char * req_key, const char * opt_key, bool bLeave) override;
};

BlobXS::BlobPimpl * GetBlobPimpl (void);

namespace MemoryBlob {
	void AuxResize (lua_State * L, size_t align, int arg, size_t size);

	// General interface
	bool IsBlob (lua_State * L, int arg, const char * type);
	bool IsLocked (lua_State * L, int arg, void * key = nullptr);
	bool IsResizable (lua_State * L, int arg);
	bool Lock (lua_State * L, int arg, void * key = nullptr);
	bool Unlock (lua_State * L, int arg, void * key = nullptr);
	bool Resize (lua_State * L, int arg, size_t size, void * key = nullptr);
	bool WhitelistUser (lua_State * L, int arg, void * key, void * user_key);
	size_t GetAlignment (lua_State * L, int arg);
	size_t GetSize (lua_State * L, int arg, bool bNSized = false);
	unsigned char * GetData (lua_State * L, int arg);
	void * GetVector (lua_State * L, int arg);

	// Redefinitions to avoid going through pimpl
	template<size_t N> typename BlobXS::VectorType<N>::type * GetVectorN (void * data)
	{
		return static_cast<typename BlobXS::VectorType<N>::type *>(data);
	}

	template<size_t N> typename BlobXS::VectorType<N>::type * GetVectorN (lua_State * L, int arg)
	{
		return GetVectorN<N>(GetVector(L, arg));
	}

	// Creation
	void NewBlob (lua_State * L, size_t size, const BlobXS::CreateOpts * opts);
	void ValidateAlignment (lua_State * L, int arg, size_t align);

	// Shared storage
	BlobXS::BlobPimpl::storage_id Submit (lua_State * L, int arg, void * key = nullptr);
	BlobXS::BlobPimpl::storage_id GetID (lua_State * L, int arg);
	bool Exists (BlobXS::BlobPimpl::storage_id id);
	bool Sync (lua_State * L, int arg, BlobXS::BlobPimpl::storage_id id, void * key = nullptr);
	void CleanStorage (std::vector<BlobXS::BlobPimpl::storage_id> & removed, uint64_t past = 0ULL);
	void StepStorageFrame (void);

	void StorageDestructors (void);

	// Storage queues
	BlobXS::BlobPimpl::storage_id NewQueue (lua_State * L, size_t n);
	void * AsQueue (BlobXS::BlobPimpl::storage_id qid);
	void * NewConsumerToken (lua_State * L, BlobXS::BlobPimpl::storage_id qid);
	void * NewProducerToken (lua_State * L, BlobXS::BlobPimpl::storage_id qid);

	struct QueueOpts {
		enum Op { eDequeue, eDequeueBulk, eEnqueue, eEnqueueBulk };
		enum Type { eQueue, eConsumer, eProducer };

		void * mKey{nullptr};
		size_t mAlign{0U};
		size_t mCount{0U};
		Type mType{eQueue};
		bool mCustomAlignment{false};
		bool mNoTry{false};
	};

	bool Enqueue (lua_State * L, int arg, void * queue, const QueueOpts & opts);
	bool EnqueueBulk (lua_State * L, int arg, void * queue, const QueueOpts & opts);
	int Dequeue (lua_State * L, int arg, void * queue, const QueueOpts & opts);
	int DequeueBulk (lua_State * L, int arg, void * queue, const QueueOpts & opts);
	size_t QueueSize (const void * queue);

	// Queue API
	int AsQueue (lua_State * L);
	int NewQueue (lua_State * L);
	int NewConsumerToken (lua_State * L);
	int NewProducerToken (lua_State * L);

	int ConsumerTokenGC (lua_State * L);
	int ProducerTokenGC (lua_State * L);
};
