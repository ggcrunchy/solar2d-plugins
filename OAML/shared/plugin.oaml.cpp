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

#include "CoronaLua.h"
/*
#include "tinyfiledialogs.h"
#include <string.h>

#define STATIC_FILTER_COUNT 8

static int GetBool (lua_State * L, const char * key)
{
	lua_getfield(L, 1, key);// ..., bool

	int bval = lua_toboolean(L, -1);

	lua_pop(L, 1);	// ...

	return bval;
}

static const char * GetStrOrBlank (lua_State * L, const char * key, const char * blank = "")
{
	lua_getfield(L, 1, key);// ..., str?

	const char * str = blank;	// might be NULL, thus not using luaL_optstring

	if (!lua_isnil(L, -1)) str = luaL_checkstring(L, -1);

	lua_pop(L, 1);

	return str;
}

static int GetFilters (lua_State * L, const char *** filters)
{
	int nfilters = 0;

	lua_getfield(L, 1, "filter_patterns");	// ..., patts

	if (lua_istable(L, -1))
	{
		int n = lua_objlen(L, -1);

		if (n > STATIC_FILTER_COUNT) *filters = (const char **)lua_newuserdata(L, sizeof(const char *) * n);// ..., patts, filters

		for (int i = 1; i <= n; ++i, lua_pop(L, 1))
		{
			lua_rawgeti(L, -1, i);	// ..., patts[, filters], patt

			(*filters)[nfilters++] = luaL_checkstring(L, -1);
		}
	}

	else if (!lua_isnil(L, -1)) (*filters)[nfilters++] = luaL_checkstring(L, -1);

	return nfilters;
}

static int StringResponse (lua_State * L, const char * res)
{
	if (!res) lua_pushboolean(L, 0);// ..., false

	else lua_pushstring(L, res);// ..., res

	return 1;
}

static luaL_Reg tfd_funcs[] = {
	{
		"colorChooser", [](lua_State * L)
		{
        #ifndef __APPLE__
			luaL_checktype(L, 1, LUA_TTABLE);
			lua_settop(L, 1);	// opts
			lua_getfield(L, 1, "out_rgb");	// opts, out

			const char * title = GetStrOrBlank(L, "title");

			//
			unsigned char rgb[3];

			lua_getfield(L, 1, "rgb");	// opts, out, rgb

			const char * def_hex_rgb = NULL;

			if (lua_istable(L, 3))
			{
				lua_getfield(L, 3, "r");// opts, out, rgb, r
				lua_getfield(L, 3, "g");// opts, out, rgb, r, g
				lua_getfield(L, 3, "b");// opts, out, rgb, r, g, b

				for (int i = 1; i <= 3; ++i) rgb[i - 1] = (unsigned char)(luaL_checknumber(L, 3 + i) * 255.0);
			}

			else def_hex_rgb = luaL_optstring(L, 3, "#000000");

			const char * color = tinyfd_colorChooser(title, def_hex_rgb, rgb, rgb);

			if (color && lua_istable(L, 2))
			{
				for (int i = 0; i < 3; ++i) lua_pushnumber(L, (double)rgb[i] / 255.0);	// opts, out, rgb[, r, g, b], rout, gout, bout

				lua_setfield(L, 2, "b");// opts, out, rgb[, r, g, b], rout, gout
				lua_setfield(L, 2, "g");// opts, out, rgb[, r, g, b], rout
				lua_setfield(L, 2, "r");// opts, out, rgb[, r, g, b]
			}

			return StringResponse(L, color);// opts, out, rgb[, r, g, b], color
        #else
            lua_pushboolean(L, 0);
            
            return 1;
        #endif
		}
	}, {
		"inputBox", [](lua_State * L)
		{
			luaL_checktype(L, 1, LUA_TTABLE);

			const char * title = GetStrOrBlank(L, "title");
			const char * message = GetStrOrBlank(L, "message");

			//
			lua_getfield(L, 1, "default_input");// opts, def_input

			const char * def_input;

			if (lua_type(L, -1) == LUA_TBOOLEAN && !lua_toboolean(L, -1)) def_input = NULL;

			else def_input = luaL_optstring(L, -1, "");

			return StringResponse(L, tinyfd_inputBox(title, message, def_input));	// opts, def_input, input
		}
	}, {
		"messageBox", [](lua_State * L)
		{
			luaL_checktype(L, 1, LUA_TTABLE);

			const char * title = GetStrOrBlank(L, "title");
			const char * message = GetStrOrBlank(L, "message");
			const char * dialog_types[] = { "ok", "okcancel", "yesno" };
			const char * icon_types[] = { "info", "warning", "error", "question" };

			lua_getfield(L, 1, "dialog_type");	// opts, dialog_type
			lua_getfield(L, 1, "icon_type");// opts, dialog_type, icon_type

			const char * dtype = dialog_types[luaL_checkoption(L, -2, "ok", dialog_types)];
			const char * itype = icon_types[luaL_checkoption(L, -1, "info", icon_types)];

			lua_pushboolean(L, tinyfd_messageBox(title, message, dtype, itype, GetBool(L, "default_okyes")));	// opts, dialog_type, icon_type, ok / yes

			return 1;
		}
	}, {
		"openFileDialog", [](lua_State * L)
		{
			luaL_checktype(L, 1, LUA_TTABLE);

			//
			const char * title = GetStrOrBlank(L, "title");
			const char * def_path_and_file = GetStrOrBlank(L, "default_path_and_file");
			const char * filter_description = GetStrOrBlank(L, "filter_description", NULL);
			const char * filter_array[STATIC_FILTER_COUNT] = { 0 }, ** filters = filter_array;
			int allow_multiple_selects = GetBool(L, "allow_multiple_selects");
			int nfilters = GetFilters(L, &filters);	// opts, patts[, filters]

			//
			const char * files = tinyfd_openFileDialog(title, def_path_and_file, nfilters, nfilters ? filters : NULL, filter_description, allow_multiple_selects);

			if (!allow_multiple_selects || !files) return StringResponse(L, files);	// opts, patts[, filters], files?

			else
			{
				lua_newtable(L);// opts, patts[, filters], files

				char * from = (char *)files, * sep = from; // assign sep in order to pass first iteration

				for (int fi = 1; sep; ++fi)
				{
					sep = strchr(from, '|');

					if (sep)
					{
						lua_pushlstring(L, from, sep - from);	// opts, patts[, filters], files, file

						from = sep + 1;
					}

					else lua_pushstring(L, from);// opts, patts[, filters], files, file
						
					lua_rawseti(L, -2, fi);	// opts, patts[, filters], files = { ..., file }
				}
			}

			return 1;
		}
	}, {
		"saveFileDialog", [](lua_State * L)
		{
			luaL_checktype(L, 1, LUA_TTABLE);

			const char * title = GetStrOrBlank(L, "title");
			const char * def_path_and_file = GetStrOrBlank(L, "default_path_and_file");
			const char * filter_description = GetStrOrBlank(L, "filter_description", NULL);
			const char * filter_array[STATIC_FILTER_COUNT] = { 0 }, ** filters = filter_array;
			int nfilters = GetFilters(L, &filters);	// opts, patts[, filters]

			return StringResponse(L, tinyfd_saveFileDialog(title, def_path_and_file, nfilters, filters, filter_description));	// opts, patts[, filters], file
		}
	}, {
		"selectFolderDialog", [](lua_State * L)
		{
			luaL_checktype(L, 1, LUA_TTABLE);

			const char * title = GetStrOrBlank(L, "title");
			const char * def_path = GetStrOrBlank(L, "default_path");

			return StringResponse(L, tinyfd_selectFolderDialog(title, def_path));	// opts, folder
		}
	},
	{ NULL, NULL }
};
*/
CORONA_EXPORT int luaopen_plugin_oaml (lua_State* L)
{
/*
	lua_newtable(L);// t
	luaL_register(L, NULL, tfd_funcs);
*/
	return 1;
}
