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

#ifndef _WIN32
	#error DLL loader not supported
#endif

#include "common.h"
#include "dll_loader.h"
#include <map>

extern "C" {
	#include "MemoryModule.h"
	#include "miniz.h"
}

//
//
//

// This is largely adapted from https://github.com/py2exe/py2exe/blob/master/source/MyLoadLibrary.c

struct Record {
	Record () {}
	Record (HCUSTOMMODULE mod) : mModule{mod}
	{
	}

	HCUSTOMMODULE mModule;
	int mRefcount{1};

	static void Add (const char * name, HCUSTOMMODULE mod);
	static Record * Find (const char * name, HCUSTOMMODULE mod);

	bool Removed ();
};

std::map<std::string, Record> sRecords;

struct Loader {
	std::map<std::string, HCUSTOMMODULE> mArchives;
	lua_State * mL;
	int mPathForFileRef{LUA_REFNIL};
	int mCPathRef{LUA_REFNIL};
	bool mInSimulator{false};

	bool ResolveZip (mz_zip_archive & zip, const char * archive);

	static void FreeLib (HCUSTOMMODULE mod, void * ud);
	static FARPROC GetProc (HCUSTOMMODULE mod, LPCSTR name, void * ud);
	static HCUSTOMMODULE LoadLib (LPCSTR filename, void * ud);
};

//
//
//

void Record::Add (const char * name, HCUSTOMMODULE mod)
{
	sRecords[name] = Record{mod};
}

//
//
//

Record * Record::Find (const char * name, HCUSTOMMODULE mod)
{
	if (name)
	{
		auto iter = sRecords.find(name);

		if (iter != sRecords.end()) return &iter->second;
	}

	if (mod)
	{
		for (auto iter = sRecords.begin(); iter != sRecords.end(); ++iter)
		{
			if (mod == iter->second.mModule) return &iter->second;
		}
	}

	return nullptr;
}

//
//
//

bool Record::Removed ()
{
	if (0 == --mRefcount)
	{
		for (auto iter = sRecords.begin(); iter != sRecords.end(); ++iter)
		{
			if (&iter->second == this)
			{
				sRecords.erase(iter);

				return true;
			}
		}
	}

	return false;
}

//
//
//

void Loader::FreeLib (HCUSTOMMODULE mod, void *)
{
	Record * record = Record::Find(nullptr, mod);

	if (!record)
	{
		SetLastError(0);

		::FreeLibrary(HMODULE(mod));
	}

	else if (record->Removed()) MemoryFreeLibrary(mod);
}

//
//
//

FARPROC Loader::GetProc (HCUSTOMMODULE mod, LPCSTR name, void *)
{
	Record * record = Record::Find(nullptr, mod);

	if (record) return MemoryGetProcAddress(record->mModule, name);

	else
	{
		SetLastError(0);

		return ::GetProcAddress(HMODULE(mod), name);
	}
}

//
//
//

HCUSTOMMODULE Loader::LoadLib (LPCSTR filename, void * ud)
{
	size_t size;
	
	mz_zip_archive * zip = static_cast<mz_zip_archive *>(ud);
	void * bytes = mz_zip_reader_extract_file_to_heap(zip, filename, &size, 0);

	if (bytes)
	{
		HCUSTOMMODULE mod = MemoryLoadLibraryEx(
			bytes, size,
			MemoryDefaultAlloc, MemoryDefaultFree,
			Loader::LoadLib, Loader::GetProc, Loader::FreeLib,
			zip
		);

		if (mod) Record::Add(filename, mod);
		else CORONA_LOG_WARNING("Failed to load dependency '%s'", filename);

		mz_free(bytes);

		return mod;
	}

	return LoadLibraryA(filename);
}

//
//
//

bool Loader::ResolveZip (mz_zip_archive & zip, const char * archive)
{
	int top = lua_gettop(mL);
	bool ok = false;

	if (!mInSimulator)
	{
		lua_getref(mL, mPathForFileRef); // ..., system.pathForFile
		lua_pushstring(mL, archive); // ..., system.pathForFile, archive
		lua_call(mL, 1, 1); // ..., file?

		ok = !lua_isnil(mL, -1) && mz_zip_reader_init_file(&zip, lua_tostring(mL, -1), 0);
	}

	else
	{
		lua_getref(mL, mCPathRef); // ..., package.cpath

		for (const char * s1 = lua_tostring(mL, -1), *s2 = s1; ; ++s2)
		{
			if (';' == *s2 || !*s2)
			{
				std::string name{s1, s2};

				luaL_gsub(mL, name.c_str(), "?.dll", archive); // ..., package.cpath, file

				if (mz_zip_reader_init_file(&zip, lua_tostring(mL, -1), 0))
				{
					ok = true;

					break;
				}

				lua_pop(mL, 1); // ..., package.cpath

				if (!*s2) break;
				else s1 = s2 + 1;
			}
		}
	}

	lua_settop(mL, top); // ...

	return ok;
}

//
//
//

static bool CheckLoaded (const Loader * loader)
{
	if (!loader) CORONA_LOG_WARNING("No loader installed");
	else if (loader->mInSimulator && LUA_REFNIL == loader->mCPathRef) CORONA_LOG_WARNING("No package.cpath available");
	else if (LUA_REFNIL == loader->mPathForFileRef) CORONA_LOG("No system.pathForFile() available");
	else return true;

	return false;
}

//
//
//

static Loader * sLoader;

//
//
//

CORONA_EXTERN_C void * LoadDLL (const char * filename, const char * archive)
{
	if (!CheckLoaded(sLoader)) return nullptr;

	//
	//
	//

	auto iter = sLoader->mArchives.find(archive);

	if (iter != sLoader->mArchives.end())
	{
		Record * record = Record::Find(filename, nullptr);

		if (!record || record->mModule != iter->second)
		{
			CORONA_LOG_WARNING("Inconsistency with loaded archive '%s' and primary file '%s'", archive, filename);

			return nullptr;
		}

		return iter->second;
	}
	
	//
	//
	//

	mz_zip_archive zip = {};
	void * result = nullptr;

	if (sLoader->ResolveZip(zip, archive))
	{
		size_t size;
		void * bytes = mz_zip_reader_extract_file_to_heap(&zip, filename, &size, 0);

		if (bytes)
		{
			HCUSTOMMODULE mod = MemoryLoadLibraryEx(
				bytes, size,
				MemoryDefaultAlloc, MemoryDefaultFree,
				Loader::LoadLib, Loader::GetProc, Loader::FreeLib,
				&zip
			);

			if (mod)
			{
				Record::Add(filename, mod);

				sLoader->mArchives[archive] = mod;

				result = mod;
			}

			mz_free(bytes);
		}

		else CORONA_LOG_WARNING("Unable to find or load '%s' (archive '%s')", filename, archive);

		mz_zip_reader_end(&zip);
	}

	//
	//
	//

	if (!result) result = LoadLibraryA(filename);

	return result;
}

//
//
//

CORONA_EXTERN_C void * GetProcFromDLL (void * dll, const char * procname)
{
	if (!CheckLoaded(sLoader)) return nullptr;

	return Loader::GetProc(dll, procname, nullptr);
}

//
//
//

void AddLoader (lua_State * L)
{
	int top = lua_gettop(L);

	sLoader = LuaXS::NewTyped<Loader>(L); // ..., loader

	sLoader->mL = L;

	LuaXS::AttachGC(L, MT_NAME(Loader), LuaXS::TypedGC<Loader>);

	AddToStore(L);

	//
	//
	//

	lua_getglobal(L, "system"); // ..., loader, system

	if (lua_istable(L, -1))
	{
		lua_getfield(L, -1, "pathForFile"); // ..., loader, system, system.pathForFile

		if (lua_isfunction(L, -1)) sLoader->mPathForFileRef = lua_ref(L, 1); // ..., loader, system; ref = system.pathForFile
		else CORONA_LOG_WARNING("Unable to find system.pathForFile(), or not a function");
	}

	if (sLoader->mPathForFileRef != LUA_REFNIL)
	{
		lua_getref(L, sLoader->mPathForFileRef); // ..., loader, system, system.pathForFile
		lua_pushliteral(L, "main.lua"); // ..., loader, system, system.pathForFile, "main.lua"
		lua_call(L, 1, 1); // ..., loader, system, name?

		sLoader->mInSimulator = !lua_isnil(L, -1);
	}
	
	

	//
	//
	//

	if (sLoader->mInSimulator)
	{
		lua_getglobal(L, "package"); // ..., loader, system, name, package

		if (lua_istable(L, -1))
		{
			lua_getfield(L, -1, "cpath"); // loader, system, name, package, package.cpath

			if (lua_type(L, -1) == LUA_TSTRING) sLoader->mCPathRef = lua_ref(L, 1); // ..., loader, system, name, package
			else CORONA_LOG_WARNING("Unable to find package.cpath, or not a string");
		}
	}

	//
	//
	//

	lua_settop(L, top); // ...

	//
	//
	//

	sRecords.clear();
}