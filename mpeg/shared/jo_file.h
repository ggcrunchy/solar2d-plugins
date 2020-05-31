#ifndef JO_FILE_H
#define JO_FILE_H

#include "CoronaLua.h"

// "File" data structure to be shared among jo_* source
struct JO_File {
	luaL_Buffer mB;	// Buffer, for memory mode
	FILE * mFP{nullptr};// File pointer, otherwise

	JO_File (lua_State * L, const char * name, const char * mode = "wb");

	void Close (void);
	void PutC (int ch);
	void Write (const void * bytes, size_t size);
};

#endif
