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

#include "CoronaLog.h"
#include "common.h"
#include "custom_objects.h"
#include <mutex>
#include <utility>

extern "C" {
	#include "marshal.h"
}

//
//
//

Entry::Entry (const Entry & other)
{
	mType = other.mType;

	switch (mType)
	{
	case LUA_TBOOLEAN:
		mB = other.mB;
		break;
	case LUA_TNUMBER:
		mN = other.mN;
		break;
	case LUA_TSTRING:
		mS = other.mS;
		break;
	default:
		break;
	}
}

//
//
//

Entry::~Entry ()
{
	using T = std::string;

	if (LUA_TSTRING == mType) mS.~T();
}

//
//
//

bool Entry::Less::operator ()(const Entry & lhs, const Entry & rhs) const
{
	if (lhs.mType == rhs.mType)
	{
		switch (lhs.mType)
		{
		case LUA_TBOOLEAN:
			return lhs.mB; // true < false
		case LUA_TNUMBER:
			return lhs.mN < rhs.mN;
		case LUA_TSTRING:
			return lhs.mS < rhs.mS;
		default:
			return false;
		}
	}

	return lhs.mType < rhs.mType;
}

//
//
//

bool Entry::IsPrimitive (lua_State * L, int arg)
{
	switch (lua_type(L, arg))
	{
	case LUA_TNIL:
	case LUA_TBOOLEAN:
	case LUA_TNUMBER:
	case LUA_TSTRING:
		return true;
	default:
		return false;
	}
}

//
//
//

static std::mutex sCustomObjectMutex;

//
//
//

static int sCustomObjectsSoLoudRef;

static lua_State * GetSoLoudState (lua_State * L, SoLoud::AudioSource * source)
{
	lua_State * soloud_L;

	if (luaL_newmetatable(L, MT_NAME(SoLoudState))) // ..., mt
	{
		soloud_L = luaL_newstate();

		luaL_openlibs(soloud_L);
		lua_atpanic(soloud_L, [](lua_State * L) {
			if (lua_isstring(L, -1)) CORONA_LOG_WARNING("Panic: %s", lua_tostring(L, -1));

			return 0;
		});

		lua_newtable(soloud_L); // soloud

		AddBasics(soloud_L);

		lua_pushvalue(soloud_L, -1); // soloud, soloud
		lua_setglobal(soloud_L, "soloud"); // soloud; _G.soloud = soloud

		sCustomObjectsSoLoudRef = lua_ref(soloud_L, 1); // (empty); ref = soloud

		//
		//
		//
		
		lua_pushcfunction(L, [](lua_State * L) {
			lua_close(*LuaXS::UD<lua_State *>(L, 1));

			return 0;
		}); // ..., mt, CloseState
		lua_setfield(L, -2, "__gc"); // ..., mt = { __gc = CloseState }

		lua_State ** box = LuaXS::NewTyped<lua_State *>(L); // ..., mt, box

		*box = soloud_L;

		lua_pushvalue(L, -2); // ..., mt, box, mt
		lua_setmetatable(L, -2); // ..., mt, box; box.metatable = mt
		lua_setfield(L, -2, "state"); // ..., mt = { __gc, state = box }
		lua_pop(L, 1); // ...
	}

	else
	{
		lua_getfield(L, -1, "state"); // ..., mt, state

		soloud_L = *LuaXS::UD<lua_State *>(L, -1);

		lua_pop(L, 2); // ...
	}

	return soloud_L;
}

//
//
//
/*
struct DoOpts {
	const char * mOther{nullptr};
	bool mGetData{true};
};
*/
template<typename F>
bool Do (CustomSource * source, const char * name, F && func, const char * other = nullptr)
{
	source->mSoloud->unlockAudioMutex_internal();

	int top = lua_gettop(source->mL);

	GetFromStore(source->mL, source); // ..., source

	lua_getfenv(source->mL, -1); // ..., source, env
	lua_getfield(source->mL, -1, "interface"); // ..., source, env, interface
	lua_insert(source->mL, -3); // ..., interface, source, env
	lua_pop(source->mL, 2); // ..., interface
	lua_getfield(source->mL, -1, name); // ..., interface, func

	bool exists = !lua_isnil(source->mL, -1);

	if (exists)
	{
		if (other) lua_getfield(source->mL, -2, other); // ..., interface, func[, other]

		lua_remove(source->mL, other ? -3 : -2); // ..., func[, other]

		if (!func(source, 0) && lua_isstring(source->mL, -1)) CORONA_LOG_WARNING("error in custom source logic (%s): %s", name, lua_tostring(source->mL, -1));
	}

	lua_settop(source->mL, top); // ...

	source->mSoloud->lockAudioMutex_internal();

	return exists;
}

//
//
//

CustomSourceInstance::CustomSourceInstance (CustomSource * parent) : mParent{parent}
{
}

//
//
//

CustomSourceInstance::~CustomSourceInstance ()
{
	CoronaLog("~CSI 1");
	if (mL) lua_unref(mL, mData);
	CoronaLog("~CSI 2");
}

//
//
//

void CustomSourceInstance::Synchronize ()
{
	if (!mEntries) return;

	std::lock_guard<std::mutex> lock(sCustomObjectMutex);

	if (mLastUpdate != mParent->mLastUpdate)
	{
		mEntries->clear();

		*mEntries = mParent->mEntries;
		mLastUpdate = mParent->mLastUpdate;
	}
}

//
//
//
/*
bool CustomSourceInstance::PrepareCall (const char * name, const char * other)
{


	bool has_data = false;//HasRef(mData);

	if (has_data) lua_getref(mParent->mL, mData); // ..., func[, other][, data]

	return has_data;
}
*/
//
//
//

bool CustomSource::DoCall (int nargs)
{
	return 0 == lua_pcall(mL, nargs, 1, 0); // ...[, result / err]
}

//
//
//

bool CustomSource::FindEntry (lua_State * L, const char * name)
{
	std::lock_guard<std::mutex> lock(sCustomObjectMutex);

	auto iter = mEntries.find(name);
	
	if (iter == mEntries.end() || LUA_TNIL == iter->second.mType) return false;

	switch (iter->second.mType)
	{
	case LUA_TBOOLEAN:
		lua_pushboolean(L, iter->second.mB); // ..., b
		break;
	case LUA_TNUMBER:
		lua_pushnumber(L, iter->second.mN); // ..., n
		break;
	case LUA_TSTRING:
		lua_pushlstring(L, iter->second.mS.c_str(), iter->second.mS.length()); // ..., s
		break;
	default:
		break;
	}

	return true;
}

//
//
//

static bool EntryChanged (lua_State * L, const Entry & entry, int pos)
{
	switch (entry.mType)
	{
	case LUA_TBOOLEAN:
		return entry.mB == !!lua_toboolean(L, pos);
	case LUA_TNUMBER:
		return entry.mN == lua_tonumber(L, pos);
	case LUA_TSTRING:
		return entry.mS == lua_tostring(L, pos);
	default:
		return false;
	}
}

bool CustomSource::SetEntry (lua_State * L, const char * name, int pos)
{
	if (!Entry::IsPrimitive(L, pos)) return false;

	std::lock_guard<std::mutex> lock(sCustomObjectMutex);

	int type = lua_type(L, pos);
	auto iter = mEntries.find(name);
	bool exists = iter != mEntries.end();

	// If the value is nil, wipe the entry. If there was one, this was a change.
	if (LUA_TNIL == type)
	{
		if (exists)
		{
			mEntries.erase(iter);

			++mLastUpdate;
		}

		return exists;
	}

	// No change?
	if (exists && type == iter->second.mType && !EntryChanged(L, iter->second, pos)) return false;
	
	// Create a new entry or reset the old one.
	Entry * entry = &iter->second;

	if (!exists) entry = &mEntries[name];
	else iter->second.~Entry();

	switch (entry->mType)
	{
	case LUA_TBOOLEAN:
		entry->mB = lua_toboolean(L, pos);
		break;
	case LUA_TNUMBER:
		entry->mN = lua_tonumber(L, pos);
		break;
	case LUA_TSTRING:
		entry->mS = lua_tostring(L, pos);
		break;
	default:
		break;
	}

	++mLastUpdate;

	return true;
}

//
//
//

unsigned int CustomSourceInstance::getAudio (float * buffer, unsigned int samples, unsigned int size)
{
	unsigned int result = 0;

	Do(mParent, "getAudio", [&result, this, buffer, samples, size](CustomSource * parent, int has_data) {
		FloatBuffer * smp = GetFloatBuffer(parent->mL, -(1 + has_data));

		smp->mData = buffer;
		smp->mSize = parent->mChannels * size;
CoronaLog("GA 1");
		lua_pushinteger(parent->mL, samples); // ..., get_audio, buffer[, data], samples
		lua_pushinteger(parent->mL, size); // ..., get_audio, buffer[, data], samples, size

		if (has_data)
		{
			lua_insert(parent->mL, -3); // ..., get_audio, buffer, size, data, samples
			lua_insert(parent->mL, -3); // ..., get_audio, buffer, samples, size, data
		}

		bool ok = parent->DoCall(3 + has_data); // ..., count / err

		if (ok && lua_isnumber(parent->mL, -1)) result = lua_tointeger(parent->mL, -1);
CoronaLog("GA 2");
		return ok;
	}, "samples");

	return result;
}

//
//
//

bool CustomSourceInstance::hasEnded ()
{
	bool result = false;

	Do(mParent, "hasEnded", [&result, this](CustomSource * parent, int has_data) {
		bool ok = parent->DoCall(has_data); // ..., ended / err

		if (ok) result = lua_toboolean(parent->mL, -1);

		return ok;
	});

	return result;
}

//
//
//

void GetResultFromStack (SoLoud::result & result, lua_State * L, bool file_ops = false)
{
	if (lua_isstring(L, -1))
	{
		const char * what = lua_tostring(L, -1);

		if (strcmp(what, "INVALID_PARAMETER") == 0) result = SoLoud::INVALID_PARAMETER;
		else if (strcmp(what, "NOT_IMPLEMENTED") == 0) result = SoLoud::NOT_IMPLEMENTED;
		else if (strcmp(what, "OUT_OF_MEMORY") == 0) result = SoLoud::OUT_OF_MEMORY;
		else if (file_ops && strcmp(what, "FILE_NOT_FOUND") == 0) result = SoLoud::FILE_NOT_FOUND;
		else if (file_ops && strcmp(what, "FILE_LOAD_FAILED") == 0) result = SoLoud::FILE_LOAD_FAILED;
	}

	else if (lua_type(L, -1) != LUA_TBOOLEAN || lua_toboolean(L, -1)) result = SoLoud::SO_NO_ERROR;
}

//
//
//

SoLoud::result CustomSourceInstance::seek (float seconds, float * scratch, int scratch_size)
{
	SoLoud::result result = SoLoud::UNKNOWN_ERROR;

	bool found = Do(mParent, "seek", [&result, this, seconds, scratch, scratch_size](CustomSource * parent, int has_data) {
		FloatBuffer * buffer = GetFloatBuffer(parent->mL, -(1 + has_data));

		buffer->mData = scratch;
		buffer->mSize = size_t(scratch_size);

		lua_pushnumber(parent->mL, seconds); // ..., seek, scratch[, data], seconds
		lua_insert(parent->mL, -(2 + has_data)); // ..., seek, seconds, scratch[, data]

		bool ok = parent->DoCall(2 + has_data); // ..., result? / err

		if (ok) GetResultFromStack(result, parent->mL);

		return ok;
	});

	if (!found) return AudioSourceInstance::seek(seconds, scratch, scratch_size);

	return result;
}

//
//
//

SoLoud::result CustomSourceInstance::rewind ()
{
	SoLoud::result result = SoLoud::UNKNOWN_ERROR;

	bool found = Do(mParent, "rewind", [&result, this](CustomSource * parent, int had_data) {
		bool ok = parent->DoCall(had_data); // ..., result? / err
			
		if (ok) GetResultFromStack(result, parent->mL);

		return ok;
	});

	if (!found) return AudioSourceInstance::rewind();

	return result;
}

//
//
//

float CustomSourceInstance::getInfo (unsigned int key)
{
	float result = 0;

	Do(mParent, "getInfo", [&result, this, key](CustomSource * parent, int has_data) {
		lua_pushinteger(parent->mL, key); // get_info[, data], key
		lua_insert(parent->mL, -(1 + has_data)); // get_info, key[, data]

		bool ok = parent->DoCall(1 + has_data);

		if (ok && lua_isnumber(parent->mL, -1)) result = LuaXS::Float(parent->mL, -1);

		return ok;
	});

	return result;
}

//
//
//

SoLoud::AudioSourceInstance * CustomSource::createInstance()
{
	CustomSourceInstance * instance = new CustomSourceInstance(this);

	// TODO: must be in other Lua state
		// create samples and scratch buffer here...
	// set wantsParentData...

	CoronaLog("CI 1");
	Do(this, "newInstance", [instance](CustomSource * source, int) {
		bool ok = source->DoCall(1); // ..., instance_data? / err

		if (ok) instance->mData = lua_ref(source->mL, 1); // ...

		return ok;
	}, "data");
	CoronaLog("CI 2");

	return instance;
}

//
//
//

static void Assign (lua_State * L, const char * name, bool leave_on_stack = false)
{
	if (!leave_on_stack) lua_setfield(L, -2, name); // source, params, ..., t = { ..., [name] = value }
	
	lua_pushnil(L); // source, params, ...[, t][, value], nil
	lua_setfield(L, 2, name); // source, params = { ..., [name] = nil }, ...[, t][, value]
}

static void AssignMethod (lua_State * L, const char * name, bool leave_on_stack = false)
{
	luaL_checktype(L, -1, LUA_TFUNCTION);

	Assign(L, name, leave_on_stack); // source, params = { ..., [name] = nil }[, t][, func]
}

static void GetOptionalMethod (lua_State * L, const char * name, bool leave_on_stack = false)
{
	lua_getfield(L, 2, name); // source, params, ...[, t], func?

	if (!lua_isnil(L, -1)) AssignMethod(L, name, leave_on_stack); // source, params, ...[, t][, func]
	else if (!leave_on_stack) lua_pop(L, 1); // source, params, ...[, t]
}

/*
	FloatBuffer * samples = NewFloatBuffer(L); // source, params, interface, samples

	samples->mCanWrite = true;
	samples->mOwnsData = false;

	lua_setfield(L, -2, "samples"); // source, params, interface = { ..., getAudio, samples = samples }

	if (GetOptionalMethod(L, "seek"))
	{
		FloatBuffer * scratch = NewFloatBuffer(L); // source, params, interface, scratch
		
		scratch->mCanWrite = true;
		scratch->mOwnsData = false;

		lua_setfield(L, -2, "scratch"); // source, params, env, interface = { ..., getAudio, samples, scratch = scratch }
	}
*/

int CustomSourceInit (lua_State * L)
{
	lua_settop(L, 2); // source, params

	CustomSource * source = LuaXS::UD<CustomSource>(L, 1);

	// Make a snapshot of the params table, then use it instead.
	luaL_checktype(L, 2, LUA_TTABLE);
	lua_newtable(L); // source, params, params2
	lua_pushnil(L); // source, params, params2, nil

	while (lua_next(L, 2))
	{
		lua_pushvalue(L, -2); // source, params, params2, k, v, k
		lua_insert(L, -2); // source, params, params2, k, k, v
		lua_rawset(L, -4); // source, params, params2 = { ..., [k] = v }, k
	}

	lua_replace(L, 2); // source, params2

	// Move any methods into an interface table...
	lua_getfenv(L, 1); // source, params2, env
	lua_pushcfunction(L, mar_encode); // source, params2, env, mar_encode
	lua_createtable(L, 0, 5); // source, params2, env, mar_encode, interface
	lua_getfield(L, 2, "getAudio"); // source, params2, env, mar_encode, interface, getAudio

	AssignMethod(L, "getAudio"); // source, params2, env, mar_encode, interface
	GetOptionalMethod(L, "hasEnded");
	GetOptionalMethod(L, "seek");
	GetOptionalMethod(L, "rewind");
	GetOptionalMethod(L, "getInfo");

	// ...then bake it into a form that instances can instantiate in another state. Any capture
	// of the SoLoud plugin module will be translated to its more minimal counterpart.
	lua_createtable(L, 1, 0); // source, params2, env, mar_encode, interface, constants

	PushPluginModule(L); // source, params2, env, mar_encode, interface, constants, soloud

	lua_rawseti(L, -2, 1); // source, params2, env, mar_encode, interface, constants = { soloud }
	lua_call(L, 2, 1); // source, params2, env, serialized
	lua_setfield(L, -2, "interface"); // source, params2, env = { ..., interface = serialized }

	// Save any instance constructor, and whether it should get a snapshot of the parent data.
	GetOptionalMethod(L, "newInstance");

	lua_getfield(L, 2, "wantParentData"); // source, params2, env, wantParentData

	Assign(L, "wantParentData"); // source, params2, env = { ..., wantParentData = wantParentData }

	// Call an initialization method if available, with the source and remaining params as input.
	GetOptionalMethod(L, "init", true); // source, params2, env, init?

	if (!lua_isnil(L, -1))
	{
		lua_insert(L, 1); // init, source, params2, env
		lua_insert(L, 1); // env, init, source, params2
		lua_call(L, 2, 0); // env, data?
	}

	return 0;
}