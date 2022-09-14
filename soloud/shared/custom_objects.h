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

#include <map>
#include <mutex>
#include <string>

#pragma once

//
//
//

struct Entry {
	Entry () {}
	Entry (const Entry & other);

	// TODO: use this, to allow primitive keys
	struct Less {
		bool operator ()(const Entry & lhs, const Entry & rhs) const;
	};

	static bool IsPrimitive (lua_State * L, int arg);

	int mType{LUA_TNIL};
	std::string mS;
	union {
		bool mB;
		lua_Number mN;
	};
};

//
//
//

struct CustomSource;

struct ParentData {
	std::map<std::string, Entry> mEntries;
	uint64_t mLastUpdate{0ULL};

	bool FindEntry (lua_State * L, int kpos = 2) const;
	bool SetEntry (lua_State * L, int kpos = 2);
	bool SetEntryWithMutex (lua_State * L, int kpos = 2);
};

struct ParentDataWrapper {
	ParentDataWrapper (const CustomSource * parent, const ParentData & data) : mParent{parent}, mData{data}
	{
	}

	const CustomSource * mParent;
	ParentData mData;

	void Init (lua_State * L);
	void Synchronize ();
};

//
//
//

struct CustomSourceInstance : public SoLoud::AudioSourceInstance {
	lua_State * mL;
	CustomSource * mParent;
	ParentData * mParentData{nullptr};
	bool mHasError{false};

	//
	//
	//

	CustomSourceInstance (CustomSource * parent);
	virtual ~CustomSourceInstance ();

	//
	//
	//

	void Init (lua_State * L, const ParentData * data);

	//
	//
	//

	bool PreCall (const char * name, const char * other, int & has_data);

	//
	//
	//

	virtual unsigned int getAudio (float * buffer, unsigned int samples, unsigned int size);
	virtual bool hasEnded ();
	virtual SoLoud::result seek (float seconds, float * scratch, int scratch_size);
	virtual SoLoud::result rewind ();
	virtual float getInfo (unsigned int key);
};

//
//
//

struct CustomSource : public SoLoud::AudioSource {
	lua_State * mL;
	ParentData mData;

	//
	//
	//

	enum {
		HashValues = 4 // interface, new instance, data, class
	};

	//
	//
	//

	// conveniences, to avoid some lookup:
	const char * mInterface;
	const char * mNewInstance{nullptr};
	size_t mInterfaceLen;
	size_t mNewInstanceLen{0U};
	bool mHasSeek;

	//
	//
	//

	bool mWantParentData;

	//
	//
	//
	
	static int Init (lua_State * L);

	//
	//
	//

	virtual SoLoud::AudioSourceInstance * createInstance();
};

//
//
//

bool CheckForKey (lua_State * L, const char * other = nullptr);
int GetEnvData (lua_State * L);
int SetEnvData (lua_State * L);
int GetData (lua_State * L, const ParentData & data);
int SetData (lua_State * L, ParentData & data, bool with_mutex = false);

std::mutex & GetCustomObjectsMutex ();

#define LOCK_SECONDARY_STATE() std::lock_guard<std::mutex> lock(GetCustomObjectsMutex())

void GetDecodeConstants (lua_State * L);

void CreateSecondaryState (lua_State * L);
lua_State * GetSoLoudState (lua_State * L);
