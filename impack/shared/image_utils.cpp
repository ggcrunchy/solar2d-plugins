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

#include "impack.h"
#include "image_utils.h"

bool ExtractFileArgs (lua_State * L, PathXS::Directories * dirs)
{
	int ti = dirs ? 2 : 1;

	if (!lua_istable(L, ti)) return false;
	if (!dirs) dirs = GetPathData(L);

	lua_getfield(L, ti, "filename");// [self, ]params, ..., filename
	lua_getfield(L, ti, "is_absolute");	// [self, ]params, ..., filename, is_absolute
	lua_getfield(L, ti, "baseDir");	// [self, ]params, ..., filename, is_absolute, baseDir?
	
	bool bAbsolute = LuaXS::Bool(L, -2), bIsDir = dirs->IsDir(L, -1);

	if (bIsDir) lua_insert(L, ti + 1);	// [self, ]params, base_dir, ..., filename, is_absolute

	lua_pop(L, bIsDir ? 1 : 2);	// [self, ]params[, base_dir], ..., filename
	lua_replace(L, ti);	// [self, ]filename[, base_dir], ...

	return bAbsolute;
}

bool FileArgsFromOpts (lua_State * L, int * opos)
{
	PathXS::Directories * dirs = GetPathData(L);
	int opts = dirs->IsDir(L, 2) ? 3 : 2;

    if (opos) *opos = opts;

	if (!lua_istable(L, opts)) return false;

    lua_getfield(L, opts, "is_absolute");	// ..., opts, ..., is_absolute

    bool bIsAbsolute = LuaXS::Bool(L);

	lua_pop(L, 1);	// ..., opts, ...

	return bIsAbsolute;
}

int Return (lua_State * L, BlobXS::State * state, int res, void * ptr, bool bAsUserdata)
{
	if (res && state) state->PushData(L, static_cast<unsigned char *>(ptr), IMPACK_BYTES, bAsUserdata);	// ..., ud[, str]

	else lua_pushnil(L);// ..., nil

	return 1;
}

void CheckDims (lua_State * L, int i1, int o1, int & iw, int & ih, int & ow, int & oh)
{
	iw = luaL_checkint(L, i1);
	ih = luaL_checkint(L, i1 + 1);

	luaL_argcheck(L, iw > 0, i1, "Invalid input width");
	luaL_argcheck(L, ih > 0, i1 + 1, "Invalid input height");

	if (o1 != 0)
	{
		ow = luaL_checkint(L, o1);
		oh = luaL_checkint(L, o1 + 1);

		luaL_argcheck(L, ow > 0, o1, "Invalid output width");
		luaL_argcheck(L, oh > 0, o1 + 1, "Invalid output height");
	}
}
