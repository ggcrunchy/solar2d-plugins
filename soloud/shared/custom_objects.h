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
#include <string>

#pragma once

//
//
//

struct Entry {
	Entry () {}
	Entry (const Entry & other);
	~Entry ();


	// TODO: use this, to allow primitive keys
	struct Less {
		bool operator ()(const Entry & lhs, const Entry & rhs) const;
	};

	static bool IsPrimitive (lua_State * L, int arg);

	int mType{LUA_TNIL};
	union {
		bool mB;
		lua_Number mN;
		std::string mS;
	};
};

//
//
//

struct CustomSource;

struct CustomSourceInstance : public SoLoud::AudioSourceInstance {
	CustomSource * mParent;
	lua_State * mL{nullptr};
	std::map<std::string, Entry> * mEntries{nullptr};
	int mData{LUA_NOREF};
	uint64_t mLastUpdate{0ULL};

	CustomSourceInstance (CustomSource * parent);
	virtual ~CustomSourceInstance ();

	void Synchronize ();

	virtual unsigned int getAudio (float * buffer, unsigned int samples, unsigned int size);
	virtual bool hasEnded ();
	virtual SoLoud::result seek (float seconds, float * scratch, int scratch_size);
	virtual SoLoud::result rewind ();
	virtual float getInfo (unsigned int key);
};

struct CustomSource : public SoLoud::AudioSource {
	lua_State * mL;
	std::map<std::string, Entry> mEntries;
	uint64_t mLastUpdate{0ULL};

	bool DoCall (int nargs);
	bool FindEntry (lua_State * L, const char * name);
	bool SetEntry (lua_State * L, const char * name, int pos);

	virtual SoLoud::AudioSourceInstance * createInstance();
};

int CustomSourceInit (lua_State * L);
