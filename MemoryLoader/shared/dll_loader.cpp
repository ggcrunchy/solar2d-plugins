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

#include "dll_loader.h"

//
//
//

Record * RecordList::Find (const char * name, HCUSTOMMODULE mod)
{
	if (name)
	{
		auto iter = find(name);

		if (iter != end()) return &iter->second;
	}

	if (mod)
	{
		for (auto iter = begin(); iter != end(); ++iter)
		{
			if (mod == iter->second.GetModule()) return &iter->second;
		}
	}

	return nullptr;
}

//
//
//

bool RecordList::Remove (Record * record)
{
	if (record->DecrementRefCount() == 0)
	{
		for (auto iter = begin(); iter != end(); ++iter)
		{
			if (&iter->second != record) continue;

			erase(iter);

			return true;
		}
	}

	return false;
}

//
//
//

void RecordList::FreeLib (HCUSTOMMODULE mod, void * ud)
{
	RecordList * list = static_cast<RecordList *>(ud);
	Record * record = list->Find(nullptr, mod);

	if (!record)
	{
		SetLastError(0);

		::FreeLibrary(HMODULE(mod));
	}

	else if (list->Remove(record)) MemoryFreeLibrary(mod);
}

//
//
//

FARPROC RecordList::GetProc (HCUSTOMMODULE mod, LPCSTR name, void * ud)
{
	RecordList * list = static_cast<RecordList *>(ud);
	Record * record = list->Find(nullptr, mod);

	if (record) return MemoryGetProcAddress(record->GetModule(), name);

	else
	{
		SetLastError(0);

		return ::GetProcAddress(HMODULE(mod), name);
	}
}

//
//
//

HCUSTOMMODULE RecordList::LoadLibLua (LPCSTR filename, void * ud)
{
	RecordList * list = static_cast<RecordList *>(ud);
	lua_State * L = static_cast<lua_State *>(list->mContext);

	lua_pushvalue(L, 2); // dll_name*, callback, store, ..., callback
	lua_pushstring(L, filename); // dll_name*, callback, store, ..., callback, filename
	lua_call(L, 1, 3); // dll_name*, callback, store, ..., data?, size?, offset?

	const void * data = lua_tostring(L, -3);
	int offset = luaL_optinteger(L, -1, 0), size = luaL_optinteger(L, -2, lua_objlen(L, -3) - offset);

	luaL_argcheck(L, !data || size > 0, -2, "Non-positive size");
	luaL_argcheck(L, offset >= 0, -1, "Negative offset");
	luaL_argcheck(L, size_t(size + offset) <= lua_objlen(L, -3), -3, "size + offset exceeds data range");
	lua_pop(L, data ? 2 : 3); // dll_name*, callback, store, ...[, data]

	if (data)
	{
		lua_rawseti(L, 3, int(lua_objlen(L, 3) + 1)); // dll_name*, callback, store = { ..., data }, ...

		HCUSTOMMODULE mod = MemoryLoadLibraryEx(
			static_cast<const unsigned char *>(data) + offset, size_t(size),
			MemoryDefaultAlloc, MemoryDefaultFree,
			RecordList::LoadLibLua, RecordList::GetProc, RecordList::FreeLib,
			list
		);

		if (mod) (*list)[filename] = mod;
		else CORONA_LOG_WARNING("Failed to load callback-based dependency '%s'", filename);

		return mod;
	}

	return LoadFallback(filename);
}

//
//
//

HCUSTOMMODULE RecordList::LoadLibZip (LPCSTR filename, void * ud)
{
	size_t size;
	
	RecordList * list = static_cast<RecordList *>(ud);
	void * bytes = mz_zip_reader_extract_file_to_heap(static_cast<mz_zip_archive *>(list->mContext), filename, &size, 0);

	if (bytes)
	{
		HCUSTOMMODULE mod = MemoryLoadLibraryEx(
			bytes, size,
			MemoryDefaultAlloc, MemoryDefaultFree,
			LoadLibZip, GetProc, FreeLib,
			list
		);

		if (mod) (*list)[filename] = mod;
		else CORONA_LOG_WARNING("Failed to load zip-based dependency '%s'", filename);

		mz_free(bytes);

		return mod;
	}

	return LoadFallback(filename);
}

//
//
//

HCUSTOMMODULE RecordList::LoadCached (const char * filename)
{
	Record * record = Find(filename, nullptr);

	if (record)
	{
		record->IncrementRefCount();

		return record->GetModule();
	}

	else return nullptr;
}

//
//
//

HCUSTOMMODULE RecordList::LoadDLL (const char * filename, lua_State * L, const char * apath)
{
	HCUSTOMMODULE result = nullptr;

	//
	//
	//

	if (!L)
	{
		mz_zip_archive zip = {};
		
		if (mz_zip_reader_init_file(&zip, apath, 0))
		{
			size_t size;
			void * bytes = mz_zip_reader_extract_file_to_heap(&zip, filename, &size, 0);

			if (bytes)
			{
				mContext = &zip;

				result = MemoryLoadLibraryEx(
					bytes, size,
					MemoryDefaultAlloc, MemoryDefaultFree,
					RecordList::LoadLibZip, RecordList::GetProc, RecordList::FreeLib,
					this
				);

				mz_free(bytes);
			}

			mz_zip_reader_end(&zip);
		}
	}

	//
	//
	//

	else
	{
		lua_pushvalue(L, 2); // dll_name*, callback, store, ..., callback
		lua_pushvalue(L, 1); // dll_name*, callback, store, ..., callback, dll_name*
		lua_call(L, 1, 3); // dll_name*, callback, store, ..., data?, size?, offset?

		const void * data = lua_tostring(L, -3);
		int offset = luaL_optinteger(L, -1, 0), size = luaL_optinteger(L, -2, lua_objlen(L, -3) - offset);

		luaL_argcheck(L, !data || size > 0, -2, "Non-positive size");
		luaL_argcheck(L, offset >= 0, -1, "Negative offset");
		luaL_argcheck(L, size_t(size + offset) <= lua_objlen(L, -3), -3, "size + offset exceeds data range");
		lua_pop(L, data ? 2 : 3); // dll_name*, callback, store, ...[, data]

		if (data)
		{
			mContext = L;

			lua_rawseti(L, 3, int(lua_objlen(L, 3) + 1)); // dll_name*, callback, store = { ..., data }, ...

			result = MemoryLoadLibraryEx(
				static_cast<const unsigned char *>(data) + offset, size_t(size),
				MemoryDefaultAlloc, MemoryDefaultFree,
				RecordList::LoadLibLua, RecordList::GetProc, RecordList::FreeLib,
				this
			);
		}
	}

	//
	//
	//

	mContext = nullptr;

	if (result) (*this)[filename] = result;

	return result;
}

//
//
//

HCUSTOMMODULE RecordList::LoadFallback (const char * filename)
{
	return (HCUSTOMMODULE)LoadLibraryA(filename);
}

//
//
//

RecordList::Proc RecordList::GetProcFromDLL (HCUSTOMMODULE dll, const char * procname)
{
	return RecordList::GetProc(dll, procname, this);
}

//
//
//

void RecordList::UnloadDLL (HCUSTOMMODULE dll)
{
	MemoryFreeLibrary(dll);
}