#ifndef TRUETYPE_H
#define TRUETYPE_H

#include "CoronaLua.h"
#include "utils/Memory.h"

#define TRUETYPE_BYTES "truetype.bytes"

int NewFont (lua_State * L);
int NewPacking (lua_State * L);
int PointSize (lua_State * L);

struct stbtt_fontinfo * GetFontInfo (lua_State * L, int arg = 1);

int Codepoint (lua_State * L, int arg);

//
MemoryXS::LuaMemory * truetype_GetMemory (void);

#endif