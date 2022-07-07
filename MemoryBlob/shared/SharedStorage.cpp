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
#include "utils/Compat.h"
#include "utils/LuaEx.h"
#include "ByteReader.h"
#include "concurrentqueue.h"
#include <ctime>
#include <algorithm>
#include <atomic>
#include <memory>
#include <mutex>
#include <utility>
#include <vector>

#ifdef _WIN32
	#include "libcuckoo-windows/cuckoohash_map.hh"
	#include "libcuckoo-windows/city_hasher.hh"
#else
	#include "libcuckoo/cuckoohash_map.hh"
	#include "libcuckoo/city_hasher.hh"
#endif

// Vector variant base
struct VariantVectorBase {
	void * mData{nullptr};	// Vector contents
	size_t mAlignment;	// Alignment of vector data
	
	VariantVectorBase (void) = default;

	VariantVectorBase (size_t align) : mAlignment{align}
	{
		WITH_ALIGNMENT_DO(mAlignment, {
			mData = new BlobXS::VectorType<VA>::type;
		})
	}

	void Remove (void)
	{
		WITH_ALIGNMENT_DO(mAlignment, {
			delete MemoryBlob::GetVectorN<VA>(mData);
		})

		mData = nullptr;
	}

	std::pair<unsigned char *, size_t> GetDataAndSize (void)
	{
		std::pair<unsigned char *, size_t> out{nullptr, 0U};

		if (mData) WITH_ALIGNMENT_DO(mAlignment, {
			auto pvec = MemoryBlob::GetVectorN<VA>(mData);

			out.first = pvec->data();
			out.second = pvec->size();
		})
			
		return out;
	}

	void FastSwap (lua_State * L, int arg)
	{
		WITH_ALIGNMENT_DO(mAlignment, {
			MemoryBlob::GetVectorN<VA>(L, arg)->swap(*MemoryBlob::GetVectorN<VA>(mData));
		})
	}
};

// Vector variant, with timeout for memory reclamation
struct VariantVectorWithTimeout : VariantVectorBase {
	uint64_t mCreated;	// Frame when this record was created

	VariantVectorWithTimeout (void) = default;

	VariantVectorWithTimeout (size_t align, uint64_t when) : VariantVectorBase{align}, mCreated{when}
	{
	}
};

//
typedef cuckoohash_map<BlobXS::BlobPimpl::storage_id, VariantVectorWithTimeout, CityHasher<BlobXS::BlobPimpl::storage_id>> StorageMap;

static std::atomic<uint64_t> sFrame{0ULL}; // fine to not destruct this on Mac?

struct StorageBox {
	StorageMap mStorage;

	~StorageBox (void)
	{
		sFrame = uint64_t(~0) - 1ULL; // incremented in CleanStorage()

		std::vector<BlobXS::BlobPimpl::storage_id> junk;

		MemoryBlob::CleanStorage(junk);
	}
};

static StorageBox & GetStorageBox (void)
{
    static StorageBox sBox;
    
    return sBox;
}

static StorageMap & GetStorageMap (void)
{
	return GetStorageBox().mStorage;
}

//
void MemoryBlob::CleanStorage (std::vector<BlobXS::BlobPimpl::storage_id> & removed, uint64_t past)
{
	StorageMap & storage = GetStorageMap();

#ifdef _WIN32
    auto locks = storage.snapshot_table();
#else
    auto locks = storage.lock_table();
#endif

	if (locks.begin() != locks.end())
	{
		auto frame = sFrame.load(std::memory_order_acquire);

        for (auto iter = locks.begin(); iter != locks.end(); ++iter)
		{
			if (frame >= iter->second.mCreated + past)
			{
				iter->second.Remove();

				removed.push_back(iter->first);
			}
		}

		for (auto id : removed) storage.erase(id);
	}
}

void MemoryBlob::StepStorageFrame (void)
{
	sFrame.fetch_add(1ULL, std::memory_order_release);
}

static void Assign (size_t align, void * ptr, const unsigned char * data, size_t size)
{
	WITH_ALIGNMENT_DO(align, {
		MemoryBlob::GetVectorN<VA>(ptr)->assign(data, data + size);
	})
}

static BlobXS::BlobPimpl::storage_id sID;
static std::vector<uint64_t> sCachedIDs;

//
BlobXS::BlobPimpl::storage_id MemoryBlob::Submit (lua_State * L, int arg, void * key)
{
	BlobPropViewer bpv{L, arg};

	if (!bpv.mProps || (bpv.mProps->mResizable && IsLocked(L, arg, key))) return BlobXS::BlobPimpl::BadID();

	size_t id = LuaXS::GenSym(L, sID, &sCachedIDs);
	StorageMap & storage = GetStorageMap();

	VariantVectorWithTimeout v{bpv.mProps->mAlign, sFrame.load(std::memory_order_relaxed)};

	if (bpv.mProps->mResizable) v.FastSwap(L, arg);

	else Assign(bpv.mProps->mAlign, v.mData, MemoryBlob::GetData(L, arg), MemoryBlob::GetSize(L, arg));

    storage.insert(id, v);

	return id;
}

static void IDsDestructor (void)
{
	sID = 0U;

	sCachedIDs.clear();
}

//
bool MemoryBlob::Exists (BlobXS::BlobPimpl::storage_id id)
{
	return GetStorageMap().contains(id);
}

//
BlobXS::BlobPimpl::storage_id MemoryBlob::GetID (lua_State * L, int arg)
{
	BlobXS::BlobPimpl::storage_id id;

	if (LuaXS::BytesToValue(L, arg, id)) return id;

	return BlobXS::BlobPimpl::BadID();
}

//
bool MemoryBlob::Sync (lua_State * L, int arg, BlobXS::BlobPimpl::storage_id id, void * key)
{
	BlobPropViewer bpv{L, arg};

	if (!bpv.mProps || IsLocked(L, arg, key)) return false;

	StorageMap & storage = GetStorageMap();

	VariantVectorWithTimeout v;

	if (!storage.update_fn(id, [&v](VariantVectorWithTimeout & entry) {
		v = entry;
	})) return false;

	if (bpv.mProps->mResizable && bpv.mProps->mAlign == v.mAlignment) v.FastSwap(L, arg);

	else
	{
		auto ds = v.GetDataAndSize();

		if (bpv.mProps->mResizable) Assign(bpv.mProps->mAlign, GetVector(L, arg), ds.first, ds.second);

		else memcpy(GetData(L, arg), ds.first, (std::min)(GetSize(L, arg), ds.second));
	}

	v.Remove();

	storage.erase(id);

	// TODO: could do uprase() on non-Windows...

	return true;
}

// Vector variant that owns its memory
struct VariantVectorOwned : VariantVectorBase {
	VariantVectorOwned (void) = default;
    VariantVectorOwned (const VariantVectorOwned &) = default;
    
    VariantVectorOwned (size_t align) : VariantVectorBase{align}
	{
	}

	VariantVectorOwned (VariantVectorOwned && other) noexcept : VariantVectorBase{}
	{
		AuxSwap(other);
	}

	VariantVectorOwned & operator = (VariantVectorOwned && other) noexcept
	{
		AuxSwap(other);

		return *this;
	}

	void AuxSwap (VariantVectorOwned & other) noexcept
	{
		std::swap(mAlignment, other.mAlignment);
		std::swap(mData, other.mData);
	}

	~VariantVectorOwned (void)
	{
		Remove();
	}
};

//
typedef moodycamel::ConcurrentQueue<VariantVectorOwned> QueueType;

static std::vector<std::unique_ptr<QueueType>> sQueues;

static std::mutex sQueueMutex;

static void QueueDestructor (void)
{
	sQueues.clear();
}

//
static size_t GetHashBase (void)
{
	static size_t base = std::hash<time_t>{}(time(nullptr));

	return base;
}

#if defined(_WIN32)
    using std::make_unique;
#else // requires C++14
    template<typename T, typename ... Args> // cf. http://en.cppreference.com/w/cpp/memory/unique_ptr/make_unique
    std::unique_ptr<T> make_unique (Args && ... args)
    {
        return std::unique_ptr<T>(new T(std::forward<Args>(args)...));
    }
#endif

//
BlobXS::BlobPimpl::storage_id MemoryBlob::NewQueue (lua_State * L, size_t n)
{
	auto newq = n ? make_unique<QueueType>(n) : make_unique<QueueType>();

	std::lock_guard<std::mutex> lock{sQueueMutex};

	size_t count = sQueues.size();

	sQueues.push_back(std::move(newq));

	return std::hash<size_t>{}(GetHashBase() + count);
}

void * MemoryBlob::AsQueue (BlobXS::BlobPimpl::storage_id qid)
{
	size_t base = GetHashBase(), count;

	{
		std::lock_guard<std::mutex> lock{sQueueMutex};

		count = sQueues.size();
	}

	std::hash<size_t> hasher{};

	for (size_t i = 0; i < count; ++i)
	{
		if (hasher(base + i) != qid) continue;

		std::lock_guard<std::mutex> lock{sQueueMutex};

		return sQueues[i].get();
	}

	return nullptr;
}

template<typename T> struct TokenAndQueue {
	T mToken;
	QueueType * mQueue;

	TokenAndQueue (QueueType * queue) : mToken{*queue}, mQueue{queue}
	{
	}
};

template<typename T> void * NewToken (lua_State * L, BlobXS::BlobPimpl::storage_id qid)
{
	QueueType * pq = static_cast<QueueType *>(MemoryBlob::AsQueue(qid));

	if (pq) return LuaXS::NewTyped<TokenAndQueue<T>>(L, pq);// ..., token

	return nullptr;
}

void * MemoryBlob::NewConsumerToken (lua_State * L, BlobXS::BlobPimpl::storage_id qid)
{
	return NewToken<moodycamel::ConsumerToken>(L, qid);	// ...[, consumer]
}

void * MemoryBlob::NewProducerToken (lua_State * L, BlobXS::BlobPimpl::storage_id qid)
{
	return NewToken<moodycamel::ProducerToken>(L, qid);	// ...[, producer]
}

int MemoryBlob::ConsumerTokenGC (lua_State * L)
{
	return LuaXS::TypedGC<TokenAndQueue<moodycamel::ConsumerToken>>(L);
}

int MemoryBlob::ProducerTokenGC (lua_State * L)
{
	return LuaXS::TypedGC<TokenAndQueue<moodycamel::ProducerToken>>(L);
}

static bool AuxEnqueue (lua_State * L, VariantVectorOwned & blob, const MemoryBlob::QueueOpts & opts)
{
	BlobPropViewer bpv{L, -1};

	bool bSwappable = bpv.mProps && bpv.mProps->mResizable && !MemoryBlob::IsLocked(L, bpv.mArg, opts.mKey);
	size_t align = opts.mCustomAlignment ? opts.mAlign : (bpv.mProps ? bpv.mProps->mAlign : 0U);

	VariantVectorOwned v{align};

	if (bSwappable) v.FastSwap(L, bpv.mArg);

	else
	{
		ByteReader bytes{L, bpv.mArg};

		Assign(align, v.mData, static_cast<const unsigned char *>(bytes.mBytes), bytes.mCount);
	}

	blob.AuxSwap(v);

	return bSwappable;
}

template<typename T> TokenAndQueue<T> * FromToken (void * ud)
{
	return static_cast<TokenAndQueue<T> *>(ud);
}

bool MemoryBlob::Enqueue (lua_State * L, int arg, void * queue, const QueueOpts & opts)
{
	// Populate the blob to upload, possibly doing a swap.
	VariantVectorOwned blob;

	bool ok = false, swapped = AuxEnqueue(L, blob, opts);

	// Attempt the enqueue itself.
	switch (opts.mType)
	{
	case QueueOpts::eQueue: {
			QueueType * pq = static_cast<QueueType *>(queue);

			ok = opts.mNoTry ? pq->enqueue(std::move(blob)) : pq->try_enqueue(std::move(blob));
		} break;
	case QueueOpts::eProducer: {
			auto * producer = FromToken<moodycamel::ProducerToken>(queue);
			QueueType * pq = producer->mQueue;

			ok = opts.mNoTry ? pq->enqueue(producer->mToken, std::move(blob)) : pq->try_enqueue(producer->mToken, std::move(blob));
		} break;
    default:
        break;
	}

	// If the enqueue failed, repair any swap.
	if (!ok && swapped) blob.FastSwap(L, arg);

	return ok;
}

bool MemoryBlob::EnqueueBulk (lua_State * L, int arg, void * queue, const QueueOpts & opts)
{
	// Populate one or more blobs to upload, tracking any swaps.
	std::vector<VariantVectorOwned> blobs;
	std::vector<bool> swapped;

	size_t n = opts.mCount;

	for (size_t i = 0U; i < n; ++i, lua_pop(L, 1))
	{
		lua_rawgeti(L, arg, int(i + 1U));	// ..., array_of_blobs_or_bytes, ..., blob_or_bytes

		blobs.push_back(VariantVectorOwned{});
		swapped.push_back(AuxEnqueue(L, blobs.back(), opts));
	}
	
	// Attempt the bulk enqueue.
	bool ok = false;

	switch (opts.mType)
	{
	case QueueOpts::eQueue: {
			QueueType * pq = static_cast<QueueType *>(queue);

			ok = opts.mNoTry ? pq->enqueue_bulk(blobs.data(), n) : pq->try_enqueue_bulk(blobs.data(), n);
		} break;
	case QueueOpts::eProducer: {
			auto * producer = FromToken<moodycamel::ProducerToken>(queue);
			QueueType * pq = producer->mQueue;

			ok = opts.mNoTry ? pq->enqueue_bulk(producer->mToken, blobs.data(), n) : pq->try_enqueue_bulk(producer->mToken, blobs.data(), n);
		} break;
    default:
        break;
	}

	// If the enqueue failed, repair any swaps.
	if (ok) n = 0U;

	for (size_t i = 0U; i < n; ++i)
	{
		if (!swapped[i]) continue;

		lua_rawgeti(L, arg, int(i + 1U));	// ..., array_of_blobs_or_bytes, ..., blob

		blobs[i].FastSwap(L, -1);

		lua_pop(L, 1);	// ..., array_of_blobs_or_bytes, ...
	}

	return ok;
}

static void AuxDequeue (lua_State * L, VariantVectorOwned & vvo, void * key)
{
	auto ds = vvo.GetDataAndSize();

	BlobPropViewer bpv{L, -1, true};

	if (bpv.mProps && !MemoryBlob::IsLocked(L, bpv.mArg, key)) // writeable blob?
	{
		if (bpv.mProps->mAlign == vvo.mAlignment) vvo.FastSwap(L, bpv.mArg); // able to swap?

		else Assign(bpv.mProps->mAlign, MemoryBlob::GetData(L, bpv.mArg), ds.first, ds.second);
	}

	else lua_pushlstring(L, reinterpret_cast<const char *>(ds.first), ds.second);	// ..., data
}

int MemoryBlob::Dequeue (lua_State * L, int arg, void * queue, const QueueOpts & opts)
{
	lua_settop(L, arg);	// ..., blob_or_bytes?

	bool ok = false;

	VariantVectorOwned blob;

	switch (opts.mType)
	{
	case QueueOpts::eQueue: {
			ok = static_cast<QueueType *>(queue)->try_dequeue(blob);
		} break;
	case QueueOpts::eConsumer: {
			auto * consumer = FromToken<moodycamel::ConsumerToken>(queue);

			ok = consumer->mQueue->try_dequeue(consumer->mToken, blob);
		} break;
	case QueueOpts::eProducer: {
			auto * producer = FromToken<moodycamel::ProducerToken>(queue);

			ok = producer->mQueue->try_dequeue_from_producer(producer->mToken, blob);
		} break;
	}

	if (ok) AuxDequeue(L, blob, opts.mKey);	// ..., blob_or_bytes?[, data]

	else lua_pushnil(L);	// ..., blob_or_bytes?, nil

	return 1;
}

int MemoryBlob::DequeueBulk (lua_State * L, int arg, void * queue, const QueueOpts & opts)
{
	lua_settop(L, arg);	// ..., array_of_blobs_or_bytes

	// Attempt to dequeue one or more items.
	size_t n = opts.mCount;

	std::vector<VariantVectorOwned> blobs(n);

	switch (opts.mType)
	{
	case QueueOpts::eQueue: {
			n = static_cast<QueueType *>(queue)->try_dequeue_bulk(blobs.data(), n);
		} break;
	case QueueOpts::eConsumer: {
			auto * consumer = FromToken<moodycamel::ConsumerToken>(queue);

			n = consumer->mQueue->try_dequeue_bulk(consumer->mToken, blobs.data(), n);
		} break;
	case QueueOpts::eProducer: {
			auto * producer = FromToken<moodycamel::ProducerToken>(queue);

			n = producer->mQueue->try_dequeue_bulk_from_producer(producer->mToken, blobs.data(), n);
		} break;
	}

	// If we dequeued anything, lazily create an output array if necessary.
	if (n > 0U && !lua_istable(L, -1))
	{
		lua_createtable(L, int(n), 0);	// ..., n, out
		lua_replace(L, -2);	// ..., out
	}

	// Populate the output array.
	int top = lua_gettop(L);

	for (size_t i = 0U; i < n; ++i, lua_pop(L, 1))
	{
		lua_rawgeti(L, arg, int(i + 1U));	// ..., array_of_blobs_or_bytes, blob_or_bytes

		AuxDequeue(L, blobs[i], opts.mKey);	// ..., array_of_blobs_or_bytes, blob_or_bytes[, as_str]

		if (lua_gettop(L) > top) lua_rawseti(L, arg, int(i + 1U));	// ..., array_of_blobs_or_bytes = { ..., as_str, ... }, blob_or_bytes
	}

	// Return results, omitting the array when nothing is dequeued.
	lua_pushinteger(L, n);	// ..., out, n

	if (n > 0U) lua_insert(L, -2);	// ..., n, out

	return n > 0U ? 2 : 1;
}

size_t MemoryBlob::QueueSize (const void * queue)
{
	return static_cast<const QueueType *>(queue)->size_approx();
}

void MemoryBlob::StorageDestructors (void)
{
	IDsDestructor();
	QueueDestructor();
}
