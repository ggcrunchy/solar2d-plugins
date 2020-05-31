#include "jo_file.h"

JO_File::JO_File (lua_State * L, const char * name, const char * mode)
{
	if (name) mFP = fopen(name, mode);

	else luaL_buffinit(L, &mB);
}

void JO_File::Close (void)
{
	if (mFP) fclose(mFP);

	else luaL_pushresult(&mB);	// ..., memory
}

void JO_File::PutC (int ch)
{
	if (mFP) putc(ch, mFP);

	else luaL_addchar(&mB, ch);
}

void JO_File::Write (const void * bytes, size_t size)
{
	if (mFP) fwrite(bytes, size, 1, mFP);

	else luaL_addlstring(&mB, (const char *)bytes, size);
}
