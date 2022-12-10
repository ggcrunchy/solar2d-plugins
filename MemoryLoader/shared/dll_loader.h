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

#ifndef _WIN32
	#error DLL loader not supported
#endif

#include "CoronaLua.h"
#include "CoronaMacros.h"
#include <map>
#include <string>

extern "C" {
	#include "MemoryModule.h"
	#include "miniz.h"
}

//
//
//

// This is largely adapted from https://github.com/py2exe/py2exe/blob/master/source/MyLoadLibrary.c

class Record {
public:
	Record () {}
	Record (HCUSTOMMODULE mod) : mModule{mod}
	{
	}

	//
	//
	//

	HCUSTOMMODULE GetModule () const { return mModule; }
	int DecrementRefCount () { return --mRefcount; }
	void IncrementRefCount () { ++mRefcount; }

	//
	//
	//

private:
	HCUSTOMMODULE mModule;
	int mRefcount{1};
};

//
//
//

class RecordList : public std::map<std::string, Record> {
public:
	using Proc = FARPROC;

	//
	//
	//

	HCUSTOMMODULE LoadCached (const char * filename);
	HCUSTOMMODULE LoadDLL (const char * filename, lua_State * state, const char * apath);
	Proc GetProcFromDLL (HCUSTOMMODULE dll, const char * procname);
	
	static HCUSTOMMODULE LoadFallback (const char * filename);
	static void UnloadDLL (HCUSTOMMODULE dll);

private:
	Record * Find (const char * name, HCUSTOMMODULE mod);

	bool Remove (Record * record);

	static void FreeLib (HCUSTOMMODULE mod, void * ud);
	static FARPROC GetProc (HCUSTOMMODULE mod, LPCSTR name, void * ud);
	static HCUSTOMMODULE LoadLibLua (LPCSTR filename, void * ud);
	static HCUSTOMMODULE LoadLibZip (LPCSTR filename, void * ud);

	void * mContext;
};
