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
#include "utils/Blob.h"
#include "utils/LuaEx.h"
#include "ByteReader.h"

static void * GetQueue (lua_State * L)
{
	void * ud = luaL_checkudata(L, 1, "MemoryBlobXS.Queue");

	return *static_cast<void **>(ud);
}

static MemoryBlob::QueueOpts GetQueueOpts (lua_State * L, MemoryBlob::QueueOpts::Op op)
{
	MemoryBlob::QueueOpts qopts;

	switch (op)
	{
	case MemoryBlob::QueueOpts::eDequeue:
		lua_pushnil(L);	// object[, opts], nil
		lua_insert(L, 2);	// object, nil[, opts]

		break;
	case MemoryBlob::QueueOpts::eDequeueBulk:
		qopts.mCount = luaL_checkinteger(L, 2);

		break;
	case MemoryBlob::QueueOpts::eEnqueueBulk:
		luaL_checktype(L, 2, LUA_TTABLE);

		qopts.mCount = lua_objlen(L, 2);

		break;
	}

	if (lua_istable(L, 3))
	{
		int top = lua_gettop(L);

		lua_getfield(L, 3, "key");	// object, data?, opts, key?

		qopts.mKey = lua_touserdata(L, -2);

		if (op == MemoryBlob::QueueOpts::eEnqueueBulk)
		{
			lua_getfield(L, 3, "count");// object, data?, opts, key?, count?

			qopts.mCount = luaL_optinteger(L, -1, qopts.mCount);
		}

		if (op == MemoryBlob::QueueOpts::eEnqueue || op == MemoryBlob::QueueOpts::eEnqueueBulk)
		{
			lua_getfield(L, 3, "alignment");// object, data?, opts, key?[, count?], align?

			qopts.mCustomAlignment = true;
			qopts.mAlign = luaL_optinteger(L, -1, 0U);

			MemoryBlob::ValidateAlignment(L, 3, qopts.mAlign);
		}

		else
		{
			lua_getfield(L, 3, "out");	// object, nil / n, opts, key?, out?

			if (op == MemoryBlob::QueueOpts::eDequeue || lua_istable(L, -1)) lua_replace(L, 2);	// object, nil / n / out?, opts, key?
		}

		lua_settop(L, top);	// object, ..., opts
	}

	return qopts;
}

static int AuxDequeueBulk (lua_State * L, void * object, const MemoryBlob::QueueOpts & qopts)
{
	if (!qopts.mCount) return LuaXS::PushArgAndReturn(L, 0U);	// object, n / out[, opts], 0

	return DequeueBulk(L, 2, GetQueue(L), qopts);	// object, out, 0 || object, n, out
}

static bool IsBytes (lua_State * L, int arg = -1)
{
	ByteReader bytes{L, arg};	// ...[, err]

	if (!bytes.mBytes) lua_pop(L, 1);	// ...

	return bytes.mBytes != nullptr;
}

static int AuxEnqueue (lua_State * L, void * object, const MemoryBlob::QueueOpts & qopts)
{
	bool result = false;

	if (IsBytes(L, 2)) result = Enqueue(L, 2, object, qopts);

	return LuaXS::PushArgAndReturn(L, result);	// object, data[, opts], ok
}

static bool IsTableOfBytes (lua_State * L, int arg, size_t count)
{
	bool all_bytes = true;

	for (size_t i = 1U; i <= count && all_bytes; ++i, lua_pop(L, 1))
	{
		lua_rawgeti(L, 2, int(i));	// object, data[, opts], data[i]

		all_bytes = !IsBytes(L);
	}

	return all_bytes;
}

static int AuxEnqueueBulk (lua_State * L, void * object, const MemoryBlob::QueueOpts & qopts)
{
	bool result = qopts.mCount == 0U;

	if (!result) result = IsTableOfBytes(L, 2, qopts.mCount) && Enqueue(L, 2, object, qopts);

	return LuaXS::PushArgAndReturn(L, result);	// object, data[, opts], ok
}

int MemoryBlob::AsQueue (lua_State * L)
{
	BlobXS::BlobPimpl::storage_id qid;

	if (LuaXS::BytesToValue(L, 1, qid))
	{
		void * queue = AsQueue(qid);

		if (queue)
		{
			void * box = lua_newuserdata(L, sizeof(void *));// qid, box

			*static_cast<void **>(box) = queue;

			LuaXS::AttachMethods(L, "MemoryBlobXS.Queue", [](lua_State * L) {
				luaL_Reg queue_methods[] = {
					{
						"Enqueue", [](lua_State * L)
						{
							QueueOpts qopts = GetQueueOpts(L, QueueOpts::eEnqueue);

							qopts.mNoTry = true;

							return AuxEnqueue(L, GetQueue(L), qopts);	// queue, data[, opts], ok
						}
					}, {
						"EnqueueBulk", [](lua_State * L)
						{
							QueueOpts qopts = GetQueueOpts(L, QueueOpts::eEnqueueBulk);

							qopts.mNoTry = true;

							return AuxEnqueueBulk(L, GetQueue(L), qopts);	// queue, data[, opts], ok
						}
					}, {
						"Size", [](lua_State * L)
						{
							return LuaXS::PushArgAndReturn(L, QueueSize(GetQueue(L)));	// queue, size
						}
					}, {
						"TryDequeue", [](lua_State * L)
						{
							QueueOpts qopts = GetQueueOpts(L, QueueOpts::eDequeue);	// queue[, blob / nil][, opts]

							return Dequeue(L, 2, GetQueue(L), qopts);	// queue[, blob / nil][, as_str / nil]
						}
					}, {
						"TryDequeueBulk", [](lua_State * L)
						{
							QueueOpts qopts = GetQueueOpts(L, QueueOpts::eDequeueBulk);	// queue, n / out[, opts]

							return AuxDequeueBulk(L, GetQueue(L), qopts);	// queue, out, 0 || queue, n, out
						}
					}, {
						"TryEnqueue", [](lua_State * L)
						{
							QueueOpts qopts = GetQueueOpts(L, QueueOpts::eEnqueue);

							return AuxEnqueue(L, GetQueue(L), qopts);	// queue, data[, opts], ok
						}
					}, {
						"TryEnqueueBulk", [](lua_State * L)
						{
							QueueOpts qopts = GetQueueOpts(L, QueueOpts::eEnqueueBulk);

							return AuxEnqueueBulk(L, GetQueue(L), qopts);	// queue, data[, opts], ok
						}
					},
					{ nullptr, nullptr }
				};

				luaL_register(L, nullptr, queue_methods);
			});

			return 1;
		}
	}

	return LuaXS::PushArgAndReturn(L, LuaXS::Nil{});// qid, nil
}

int MemoryBlob::NewQueue (lua_State * L)
{
	size_t n = luaL_optinteger(L, 1, 0U);
	BlobXS::BlobPimpl::storage_id qid = NewQueue(L, n);

	LuaXS::ValueToBytes(L, qid);// [n, ]qid

	return 1;
}

static void * ConsumerToken (lua_State * L)
{
	return luaL_checkudata(L, 1, "MemoryBlobXS.ConsumerToken");
}

int MemoryBlob::NewConsumerToken (lua_State * L)
{
	BlobXS::BlobPimpl::storage_id qid;

	if (LuaXS::BytesToValue(L, 1, qid) && NewConsumerToken(L, qid))	// qid[, token]
	{
		LuaXS::AttachMethods(L, "MemoryBlobXS.ConsumerToken", [](lua_State * L) {
			luaL_Reg consumer_methods[] = {
				{
					"__gc", ConsumerTokenGC
				}, {
					"TryDequeue", [](lua_State * L)
					{				
						QueueOpts qopts = GetQueueOpts(L, QueueOpts::eDequeue);	// ctoken[, blob / nil][, opts]

						qopts.mType = QueueOpts::eConsumer;

						return Dequeue(L, 2, ConsumerToken(L), qopts);	// ctoken[, blob / nil][, as_str / nil]
					}
				}, {
					"TryDequeueBulk", [](lua_State * L)
					{
						QueueOpts qopts = GetQueueOpts(L, QueueOpts::eDequeueBulk);	// ctoken, n / out[, opts]

						qopts.mType = QueueOpts::eConsumer;

						return AuxDequeueBulk(L, ConsumerToken(L), qopts);	// ctoken, out, 0 || ctoken, n, out
					}
				},
				{ nullptr, nullptr }
			};

			luaL_register(L, nullptr, consumer_methods);
		});
	}

	return LuaXS::PushArgAndReturn(L, LuaXS::Nil{});// qid, nil
}

static void * ProducerToken (lua_State * L)
{
	return luaL_checkudata(L, 1, "MemoryBlobXS.ProducerToken");
}

int MemoryBlob::NewProducerToken (lua_State * L)
{
	BlobXS::BlobPimpl::storage_id qid;

	if (LuaXS::BytesToValue(L, 1, qid) && NewProducerToken(L, qid)) // qid[, token]
	{
		LuaXS::AttachMethods(L, "MemoryBlobXS.ProducerToken", [](lua_State * L) {
			luaL_Reg producer_methods[] = {
				{
					"__gc", ProducerTokenGC
				}, {
					"Enqueue", [](lua_State * L)
					{
						QueueOpts qopts = GetQueueOpts(L, QueueOpts::eEnqueue);

						qopts.mNoTry = true;
						qopts.mType = QueueOpts::eProducer;

						return AuxEnqueue(L, ProducerToken(L), qopts);	// ptoken, data[, opts], ok
					}
				}, {
					"EnqueueBulk", [](lua_State * L)
					{
						QueueOpts qopts = GetQueueOpts(L, QueueOpts::eEnqueueBulk);

						qopts.mNoTry = true;
						qopts.mType = QueueOpts::eProducer;

						return AuxEnqueueBulk(L, ProducerToken(L), qopts);	// ptoken, data[, opts], ok
					}
				}, {
					"TryDequeue", [](lua_State * L)
					{
						QueueOpts qopts = GetQueueOpts(L, QueueOpts::eDequeue);	// ptoken[, blob / nil][, opts]

						qopts.mType = QueueOpts::eProducer;

						return Dequeue(L, 2, ProducerToken(L), qopts);	// ptoken[, blob / nil][, as_str / nil]
					}
				}, {
					"TryDequeueBulk", [](lua_State * L)
					{
						QueueOpts qopts = GetQueueOpts(L, QueueOpts::eDequeueBulk);	// ptoken, n / out[, opts]

						qopts.mType = QueueOpts::eProducer;

						return AuxDequeueBulk(L, ProducerToken(L), qopts);	// ptoken, out, 0 || ptoken, n, out
					}
				}, {
					"TryEnqueue", [](lua_State * L)
					{
						QueueOpts qopts = GetQueueOpts(L, QueueOpts::eEnqueue);

						qopts.mType = QueueOpts::eProducer;

						return AuxEnqueue(L, ProducerToken(L), qopts);	// ptoken, data[, opts], ok
					}
				}, {
					"TryEnqueueBulk", [](lua_State * L)
					{
						QueueOpts qopts = GetQueueOpts(L, QueueOpts::eEnqueueBulk);

						qopts.mType = QueueOpts::eProducer;

						return AuxEnqueueBulk(L, ProducerToken(L), qopts);	// ptoken, data[, opts], ok
					}
				}, 
				{ nullptr, nullptr }
			};

			luaL_register(L, nullptr, producer_methods);
		});

		return 1;
	}

	return LuaXS::PushArgAndReturn(L, LuaXS::Nil{});// qid, nil
}