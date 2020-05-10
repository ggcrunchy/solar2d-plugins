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

#include "extensions.h"
#include <pthread.h>
#include <stdint.h>
#include <string>
#include <vector>

#ifdef __APPLE__
    #include "TargetConditionals.h"
#endif

#ifdef _WIN32
    #include "libcuckoo-windows/cuckoohash_map.hh"
    #include "libcuckoo-windows/city_hasher.hh"
    #include <Windows.h>
#else
    #include "libcuckoo/cuckoohash_map.hh"
    #include "libcuckoo/city_hasher.hh"
#endif

#include <chrono>
#include <functional>
#include <thread>

namespace ns = std;

#include "pevents.h"

// Variables

struct Func {
	lua_CFunction mBody;// Function proper
	void * mPayload;// First argument, if any
};

union Entry {
	lua_Integer mI;	// Integer value
	lua_Number mN;	// Numeric value
	neosmart::neosmart_event_t mEvent;	// Event value
	Func mF;// Function value

	Entry (void) {}
	Entry (lua_Integer i) : mI{i} {}
	Entry (lua_Number n) : mN{n} {}
	Entry (neosmart::neosmart_event_t e) : mEvent{e} {}
	Entry (lua_CFunction f, void * p)
	{
		mF.mBody = f;
		mF.mPayload = p;
	}

	void Get (lua_Integer & i) { i = mI; }
	void Get (lua_Number & n) { n = mN; }
	void Get (neosmart::neosmart_event_t & e) { e = mEvent; }
	void Get (Func & f) { f = mF; }
};

using cuckoo_map = cuckoohash_map<std::string, Entry, CityHasher<std::string>>;

static cuckoo_map * sInts, * sNums, * sEvents, * sFuncs;

static struct MapLifetime {
	~MapLifetime (void)
	{
	#ifdef _WIN32
		for (auto iter = sEvents->begin(); !iter.is_end(); ++iter)
	#else
		auto locks = sEvents->lock_table();

		for (auto iter = locks.begin(); iter != locks.end(); ++iter)
    #endif
		{
			DestroyEvent(iter->second.mEvent);
		}
	}
} sMapLifetime;

static void MapsDestructor (void)
{
	sMapLifetime.~MapLifetime();

	sInts->clear();
	sNums->clear();
	sEvents->clear();
	sFuncs->clear();
}

static pthread_mutex_t sNameMutex = PTHREAD_MUTEX_INITIALIZER;
static uint64_t sNameCounter;

static void NameDestructor (void)
{
	pthread_mutex_destroy(&sNameMutex);

	sNameMutex = PTHREAD_MUTEX_INITIALIZER;

	sNameCounter = 0ULL;
}

static const char * NewName (lua_State * L)
{
	pthread_mutex_lock(&sNameMutex);

	size_t hash = ns::hash<uint64_t>{}(sNameCounter++);

	pthread_mutex_unlock(&sNameMutex);

	lua_pushlstring(L, reinterpret_cast<const char *>(&hash), sizeof(size_t));	// ..., name

	return lua_tostring(L, -1);
}

CORONA_EXTERN_C void AddInteger (lua_State * L, lua_Integer i)
{
	(*sInts)[NewName(L)] = Entry(i);
}

CORONA_EXTERN_C void AddNumber (lua_State * L, lua_Number n)
{
	(*sNums)[NewName(L)] = Entry(n);
}

CORONA_EXTERN_C int UpdateInteger (const char * name, int op, lua_Integer i, lua_Integer * result)
{
    bool ok = true;

    return sInts->update_fn(name, [=, &ok](Entry & cur) {
		switch (op)
		{
		case eUpdateAssign:
			cur.mI = i;
			break;
		case eUpdateAdd:
			cur.mI += i;
			break;
		case eUpdateSub:
			cur.mI -= i;
			break;
		case eUpdateAnd:
			cur.mI &= i;
			break;
		case eUpdateOr:
			cur.mI |= i;
			break;
		case eUpdateXor:
			cur.mI ^= i;
			break;
		default:
			ok = false;
		}

		if (result) *result = cur.mI;
    }) && ok ? 1 : 0;
}

CORONA_EXTERN_C int UpdateNumber (const char * name, int op, lua_Number n, lua_Number * result)
{
	bool ok = true;

	return sNums->update_fn(name, [=, &ok](Entry & cur) {
		switch (op)
		{
		case eUpdateAssign:
			cur.mN = n;
			break;
		case eUpdateAdd:
			cur.mN += n;
			break;
		case eUpdateSub:
			cur.mN -= n;
			break;
		default:
			ok = false;
		}

		if (result) *result = cur.mN;
    }) && ok ? 1 : 0;
}

template<typename T> static int Get (cuckoo_map * map, const char * key, T & out)
{
	Entry entry;

    if (map->find(key, entry))
    {
        entry.Get(out);
        
        return 1;
    }
    
    else return 0;
}

CORONA_EXTERN_C int GetInteger (const char * name, lua_Integer * i)
{
	return Get(sInts, name, *i);
}

CORONA_EXTERN_C int GetNumber (const char * name, lua_Number * n)
{
	return Get(sNums, name, *n);
}

CORONA_EXTERN_C void RemoveInteger (const char * name)
{
	sInts->erase(name);
}

CORONA_EXTERN_C void RemoveNumber (const char * name)
{
	sNums->erase(name);
}

// Plugin support

CORONA_EXTERN_C void InitExtensions (void)
{
	static cuckoo_map sIntMap, sNumMap, sEventMap, sFuncMap;

	sInts = &sIntMap;
	sNums = &sNumMap;
	sEvents = &sEventMap;
	sFuncs = &sFuncMap;
}

CORONA_EXTERN_C int AddFunction (const char * name, lua_CFunction func, void * payload)
{
	return sFuncs->insert(name, Entry(func, payload)) ? 1 : 0;
}

CORONA_EXTERN_C int PopFunction (const char * name, lua_CFunction * func, void ** payload)
{
	Func out;

	if (!Get(sFuncs, name, out)) return 0;

	*func = out.mBody;
	*payload = out.mPayload;

	sFuncs->erase(name);

	return 1;
}

// Events

CORONA_EXTERN_C int CreateEventX (lua_State * L, int manual_reset, int initial_state)
{
	neosmart::neosmart_event_t result = neosmart::CreateEvent(manual_reset != 0, initial_state != 0);

	if (!result) return 0;

	(*sEvents)[NewName(L)] = Entry(result);

	return 1;
}

CORONA_EXTERN_C int DestroyEventX (const char * name)
{
	neosmart::neosmart_event_t ne;

	if (!Get(sEvents, name, ne)) return eEventOK;

	sEvents->erase(name);

	return neosmart::DestroyEvent(ne) == 0 ? eEventOK : eEventError;
}

CORONA_EXTERN_C int SetEventX (const char * name)
{
	neosmart::neosmart_event_t ne;

	if (!Get(sEvents, name, ne)) return eEventError;

	return neosmart::SetEvent(ne) == 0 ? eEventOK : eEventError;
}

CORONA_EXTERN_C int ResetEventX (const char * name)
{
	neosmart::neosmart_event_t ne;

	if (!Get(sEvents, name, ne)) return eEventError;

	return neosmart::ResetEvent(ne) == 0 ? eEventOK : eEventError;
}

static int ConvertResult (int result)
{
	switch (result)
	{
	case 0:
		return eEventOK;
	case WAIT_TIMEOUT:
		return eEventTimeout;
	default:
		return eEventError;
	}
}

CORONA_EXTERN_C int WaitForEventX (const char * name, uint64_t ms)
{
	neosmart::neosmart_event_t ne;

	if (!Get(sEvents, name, ne)) return eEventError;

	return ConvertResult(neosmart::WaitForEvent(ne, ms));
}

CORONA_EXTERN_C int WaitForMultipleEventsX (const char * names[], uint64_t ms, int all, int * index)
{
	std::vector<neosmart::neosmart_event_t> events;

	for (int i = 0; names[i]; ++i)
	{
		neosmart::neosmart_event_t ne;

		if (!Get(sEvents, names[i], ne)) return eEventError;

		events.push_back(ne);
	}

	return ConvertResult(neosmart::WaitForMultipleEvents(events.data(), events.size(), all != 0, ms, *index));
}

// Miscellaneous

CORONA_EXTERN_C unsigned int EstimateConcurrency (void)
{
	return std::thread::hardware_concurrency();
}

CORONA_EXTERN_C void SleepFor (unsigned int ms)
{
	if (ms > 0U) std::this_thread::sleep_for(std::chrono::milliseconds(ms));
    // ^^ TODO: some sort of loop with a condition variable to account for suspends...
    // Must interact with that stuff in luaproc code...
    // Even more so the wait-for-events stuff above (probably need to just do it manually)
        // Or need to always add a dummy, then loop on it (always setting it to "okay" state first)
	else std::this_thread::yield();
}

CORONA_EXTERN_C void ExtDestructors (void)
{
#ifndef _WIN32
	MapsDestructor();
	NameDestructor();
#endif
}
