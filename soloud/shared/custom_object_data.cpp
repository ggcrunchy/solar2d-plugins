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
#include "custom_objects.h"

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

bool ParentData::FindEntry (lua_State * L, int kpos) const
{
	if (!lua_isstring(L, kpos)) return false; // < TODO: relax, use IsPrimitive()...

	const char * name = lua_tostring(L, kpos);
	auto iter = mEntries.find(name);

	if (iter == mEntries.end()) return false;

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
		return false;
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
		return entry.mB != !!lua_toboolean(L, pos);
	case LUA_TNUMBER:
		return entry.mN != lua_tonumber(L, pos);
	case LUA_TSTRING:
		return entry.mS != lua_tostring(L, pos);
	default:
		return false;
	}
}

//
//
//

static bool PreSetEntry (lua_State * L, int kpos)
{
	if (!lua_isstring(L, kpos)) return false; // < TODO: relax, use IsPrimitive() (non-nil)...
	if (!Entry::IsPrimitive(L, kpos + 1)) return false;

	return true;
}

//
//
//

bool PostSetEntry (lua_State * L, int kpos, std::map<std::string, Entry> & entries, uint64_t & last_update)
{
	const char * name = lua_tostring(L, kpos);
	int vpos = kpos + 1, type = lua_type(L, vpos);
	auto iter = entries.find(name);
	bool exists = iter != entries.end();

	// For most missing keys, a new entry is brought into existence. However, since nil only
	// wipes a key, it is treated as not a map entry.
	if (LUA_TNIL == type && !exists) return false;

	// Possible changes, from most broad to least:
	// * came into existence
	// * changed type (an existing entry will not have nil type)
	// * changed value
	// We can exit early if none of these occurred. The result is still reported as a map entry.
	if (exists && type == iter->second.mType && !EntryChanged(L, iter->second, vpos)) return true;

	// Create a new entry or update an old one.
	Entry * entry = exists ? &iter->second : &entries[name];

	if (exists && type != LUA_TSTRING) entry->mS.clear();

	entry->mType = type;

	switch (type)
	{
	case LUA_TNIL: // must exist, per missing key check above; destructor is no-op
		entries.erase(iter);
		break;
	case LUA_TBOOLEAN:
		entry->mB = lua_toboolean(L, vpos);
		break;
	case LUA_TNUMBER:
		entry->mN = lua_tonumber(L, vpos);
		break;
	case LUA_TSTRING:
		entry->mS = lua_tostring(L, vpos);
		break;
	default:
		break;
	}

	++last_update;

	return true;
}

//
//
//

bool ParentData::SetEntry (lua_State * L, int kpos)
{
	return PreSetEntry(L, kpos) && PostSetEntry(L, kpos, mEntries, mLastUpdate);
}

//
//
//

bool ParentData::SetEntryWithMutex (lua_State * L, int kpos)
{
	if (!PreSetEntry(L, kpos)) return false;
	
	LOCK_SECONDARY_STATE();

	return PostSetEntry(L, kpos, mEntries, mLastUpdate);
}

//
//
//

void ParentDataWrapper::Init (lua_State * L)
{
	LuaXS::AttachMethods(L, MT_NAME(ParentData), [](lua_State * L) {
		luaL_Reg funcs[] = {
			{
				"__gc", LuaXS::TypedGC<ParentDataWrapper>
			}, {
				"__newindex", [](lua_State * L)
				{
					CORONA_LOG_WARNING("Custom instance's parent data is read-only");

					return 0;
				}
			},
			{ nullptr, nullptr }
		};

		luaL_register(L, nullptr, funcs);

		//
		//
		//

		LuaXS::AttachProperties(L, [](lua_State * L) {
			ParentDataWrapper * pdata = LuaXS::CheckUD<ParentDataWrapper>(L, 1, MT_NAME(ParentData));

			pdata->Synchronize();

			if (!pdata->mData.FindEntry(L, 2)) lua_pushnil(L); // parent_data, k, v / nil

			return 1;
		});
	});
}

//
//
//

void ParentDataWrapper::Synchronize ()
{
	if (mData.mLastUpdate == mParentData->mLastUpdate) return;
	
	mData = *mParentData;
}

//
//
//

static void Assign (lua_State * L, const char * name, bool leave_on_stack)
{
	if (!leave_on_stack) lua_setfield(L, -2, name); // object, params, ..., t = { ..., [name] = value }
	
	lua_pushnil(L); // object, params, ...[, t][, value], nil
	lua_setfield(L, 2, name); // object, params = { ..., [name] = nil }, ...[, t][, value]
}

//
//
//

void AssignMember (lua_State * L, const char * name, int type, bool leave_on_stack)
{
	luaL_checktype(L, -1, type);

	Assign(L, name, leave_on_stack); // object, params = { ..., [name] = nil }[, t][, func]
}

//
//
//

void GetOptionalMember (lua_State * L, const char * name, int type, bool leave_on_stack)
{
	lua_getfield(L, 2, name); // object, params, ...[, t], value?

	if (!lua_isnil(L, -1)) AssignMember(L, name, type, leave_on_stack); // object, params, ...[, t][, value]
	else if (!leave_on_stack) lua_pop(L, 1); // object, params, ...[, t]
}

//
//
//

bool CheckForKeyInEnv (lua_State * L, const char * name)
{
	lua_getfenv(L, 1); // object, k, ..., env
	lua_getfield(L, -1, name); // object, k, ..., env, t?

	if (!lua_isnil(L, -1))
	{
		lua_pushvalue(L, 2); // object, k, ..., env, t, k
		lua_rawget(L, -2); // object, k, ..., env, t, v?
	}

	return !lua_isnil(L, -1);
}

//
//
//

bool CheckForKey (lua_State * L, const char * other)
{
	lua_getmetatable(L, 1); // object, k, v, mt
	lua_pushvalue(L, 2); // object, k, v, mt, k
	lua_rawget(L, -2); // object, k, v, mt, v?

	if (!lua_isnil(L, -1))
	{
		CORONA_LOG_WARNING("Attempt to modify built-in value (%s)", lua_isstring(L, 2) ? lua_tostring(L, 2) : "?");

		return true;
	}

	if (other && !CheckForKeyInEnv(L, other)) return false; // object, k, v, mt, nil, env[, other[, v?]]

	if (!lua_isnil(L, -1))
	{
		CORONA_LOG_WARNING("Attempt to modify class constant (%s)", lua_isstring(L, 2) ? lua_tostring(L, 2) : "?");

		return true;
	}

	return false;
}

//
//
//

int GetEnvData (lua_State * L)
{
	lua_getfenv(L, 1); // object, k, env
	lua_getfield(L, -1, "data"); // object, k, env, data?

	if (!lua_isnil(L, -1))
	{
		lua_pushvalue(L, 2); // object, k, env, data, k
		lua_rawget(L, -2); // object, k, env, data, v?
	}

	return 1;
}

//
//
//

int GetData (lua_State * L, const ParentData & data)
{
	if (!data.FindEntry(L, 2)) return GetEnvData(L); // look in map first

	return 1;
}

//
//
//

int SetEnvData (lua_State * L)
{
	lua_getfenv(L, 1); // object, k, v, ..., env
	lua_getfield(L, -1, "data"); // object, k, v, ..., env, data?

	if (lua_isnil(L, -1))
	{
		lua_newtable(L); // object, k, v, ..., env, nil, data
		lua_pushvalue(L, -1); // object, k, v, ..., env, nil, data, data
		lua_setfield(L, -4, "data"); // object, k, v, ..., env = { ..., data = data }, nil, data
	}

	lua_replace(L, 1); // data, k, v, ..., env[, nil]
	lua_settop(L, 3); // data, k, v
	lua_rawset(L, 1); // data = { ..., [k] = v }

	return 0;
}

//
//
//

int SetData (lua_State * L, ParentData & data, bool with_mutex)
{
	bool found = false;

	if (with_mutex) found = data.SetEntryWithMutex(L);
	else found = data.SetEntry(L);
		
	if (!found) SetEnvData(L); // add to map, if possible

	return 0;
}