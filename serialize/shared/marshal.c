/*
* lmarshal.c
* A Lua library for serializing and deserializing Lua values
* Richard Hundt <richardhundt@gmail.com>
*
* License: MIT
*
* Copyright (c) 2010 Richard Hundt
*
* Permission is hereby granted, free of charge, to any person
* obtaining a copy of this software and associated documentation
* files (the "Software"), to deal in the Software without
* restriction, including without limitation the rights to use,
* copy, modify, merge, publish, distribute, sublicense, and/or sell
* copies of the Software, and to permit persons to whom the
* Software is furnished to do so, subject to the following
* conditions:
*
* The above copyright notice and this permission notice shall be
* included in all copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
* OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
* NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
* HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
* WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
* FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
* OTHER DEALINGS IN THE SOFTWARE.
*/


#include <stdlib.h>
#include <string.h>
#include <stdint.h>
// STEVE CHANGE
/*
#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"
*/
#include "stdafx.h"
// /STEVE CHANGE

#define MAR_TREF 1
#define MAR_TVAL 2
#define MAR_TUSR 3

#define MAR_CHR 1
#define MAR_I32 4
#define MAR_I64 8

#define MAR_MAGIC 0x8e
#define SEEN_IDX  3

typedef struct mar_Buffer {
    size_t size;
    size_t seek;
    size_t head;
    char*  data;
// STEVE CHANGE
	int arg, resizable;
// /STEVE CHANGE
} mar_Buffer;

static int mar_encode_table(lua_State *L, mar_Buffer *buf, size_t *idx);
static int mar_decode_table(lua_State *L, const char* buf, size_t len, size_t *idx);

// STEVE CHANGE
static void * bob_malloc (lua_State * L, mar_Buffer * buf)
{
	if (buf->arg && !buf->resizable) return blob_realloc(L, buf->arg, 0U); // 0 = just get data; leave buf size intact

	else
	{
		buf->size = 128;

		if (buf->resizable) return blob_realloc(L, buf->arg, buf->size);

		else return malloc(buf->size);
	}
}

static void * bob_realloc (lua_State * L, mar_Buffer * buf, size_t new_size)
{
	if (buf->arg)
	{
		if (buf->resizable) return blob_realloc(L, buf->arg, new_size);

		// realloc'ing but have a fixed-size blob: must use dynamic memory instead

		void * data = malloc(new_size);

		if (data)
		{
			memcpy(data, blob_realloc(L, buf->arg, 0U), buf->size); // 0 = just get data

			buf->arg = 0; // no longer in use
		}

		return data;
	}

	else return realloc(buf->data, new_size);
}
// /STEVE CHANGE

static void buf_init(lua_State *L, mar_Buffer *buf)
{
//  buf->size = 128; // <- STEVE CHANGE
    buf->seek = 0;
    buf->head = 0;
    if (!(buf->data = (char*)bob_malloc(L, buf)/*malloc(buf->size)*/)) luaL_error(L, "Out of memory!"); // <- STEVE CHANGE
}

static void buf_done(lua_State* L, mar_Buffer *buf)
{
    if (!buf->arg) free(buf->data); // <- STEVE CHANGE
}

static int buf_write(lua_State* L, const char* str, size_t len, mar_Buffer *buf)
{
    if (len > UINT32_MAX) luaL_error(L, "buffer too long");
    if (buf->size - buf->head < len) {
        size_t new_size = buf->size << 1;
        size_t cur_head = buf->head;
        while (new_size - cur_head <= len) {
            new_size = new_size << 1;
        }
        if (!(buf->data = (char *)bob_realloc(L, buf, new_size)/*realloc(buf->data, new_size)*/)) { // <- STEVE CHANGE
            luaL_error(L, "Out of memory!");
        }
        buf->size = new_size;
    }
    memcpy(&buf->data[buf->head], str, len);
    buf->head += len;
    return 0;
}

static const char* buf_read(lua_State *L, mar_Buffer *buf, size_t *len)
{
    if (buf->seek < buf->head) {
        buf->seek = buf->head;
        *len = buf->seek;
        return buf->data;
    }
    *len = 0;
    return NULL;
}
// STEVE CHANGE
static void WriteByte (lua_State * L, int val, mar_Buffer * buf)
{
    int8_t val8 = (int8_t)val;
    
    buf_write(L, (char*)(void*)&val8, MAR_CHR, buf);
}

#define I32(val)	char bytes[4];							\
					bytes[0] = (val32 & 0xFF000000) >> 24;	\
					bytes[1] = (val32 & 0x00FF0000) >> 16;	\
					bytes[2] = (val32 & 0x0000FF00) >> 8;	\
					bytes[3] = (val32 & 0xFF);				\
					buf_write(L, bytes, MAR_I32, buf)

static void WriteInt (lua_State * L, int val, mar_Buffer * buf)
{
    int32_t val32 = (int32_t)val;

	I32(val32);
}

static void WriteSize (lua_State * L, size_t sz, mar_Buffer * buf)
{
    uint32_t val32 = (uint32_t)sz;
    
    I32(val32);
}

static void WriteTagRef (lua_State * L, int tag, int ref, mar_Buffer * buf)
{
    WriteByte(L, tag, buf);
    WriteInt(L, ref, buf);
}

static void WriteRecord (lua_State * L, mar_Buffer * rec_buf, mar_Buffer * buf)
{
    WriteSize(L, rec_buf->head, buf);

    buf_write(L, rec_buf->data, rec_buf->head, buf);
}

static void WriteTagRecord (lua_State * L, int tag, mar_Buffer * rec_buf, mar_Buffer * buf)
{
    WriteByte(L, tag, buf);
    WriteRecord(L, rec_buf, buf);
}
// /STEVE CHANGE

static void mar_encode_value(lua_State *L, mar_Buffer *buf, int val, size_t *idx)
{
    size_t l;
    int val_type = lua_type(L, val);
    lua_pushvalue(L, val);
// STEVE CHANGE
	WriteByte(L, val_type, buf);
    //buf_write(L, (void*)&val_type, MAR_CHR, buf);
// /STEVE CHANGE
    switch (val_type) {
    case LUA_TBOOLEAN: {
        int int_val = lua_toboolean(L, -1);
// STEVE CHANGE
        WriteByte(L, int_val, buf);
        //buf_write(L, (void*)&int_val, MAR_CHR, buf);
// /STEVE CHANGE
        break;
    }
    case LUA_TSTRING: {
        const char *str_val = lua_tolstring(L, -1, &l);
// STEVE CHANGE
        WriteSize(L, l, buf);
        //buf_write(L, (void*)&l, MAR_I32, buf);
// /STEVE CHANGE
        buf_write(L, str_val, l, buf);
        break;
    }
    case LUA_TNUMBER: {
        lua_Number num_val = lua_tonumber(L, -1);
        buf_write(L, (char*)(void*)&num_val, MAR_I64, buf);
        break;
    }
    case LUA_TTABLE: {
        int tag, ref;
        lua_pushvalue(L, -1);
        lua_rawget(L, SEEN_IDX);
        if (!lua_isnil(L, -1)) {
            ref = lua_tointeger(L, -1);
            tag = MAR_TREF;
// STEVE CHANGE
            WriteTagRef(L, tag, ref, buf);
            /*
            buf_write(L, (void*)&tag, MAR_CHR, buf);
            buf_write(L, (void*)&ref, MAR_I32, buf);
            */
// /STEVE CHANGE
            lua_pop(L, 1);
        }
        else {
			mar_Buffer rec_buf = { 0 }; // <- STEVE CHANGE
            lua_pop(L, 1); /* pop nil */
            if (luaL_getmetafield(L, -1, "__persist")) {
                tag = MAR_TUSR;

                lua_pushvalue(L, -2); /* self */
                lua_call(L, 1, 1);
                if (!lua_isfunction(L, -1)) {
                    luaL_error(L, "__persist must return a function");
                }

                lua_remove(L, -2); /* __persist */

                lua_newtable(L);
                lua_pushvalue(L, -2); /* callback */
                lua_rawseti(L, -2, 1);

                buf_init(L, &rec_buf);
                mar_encode_table(L, &rec_buf, idx);
// STEVE CHANGE
                WriteTagRecord(L, tag, &rec_buf, buf);
                /*
                buf_write(L, (void*)&tag, MAR_CHR, buf);
                buf_write(L, (void*)&rec_buf.head, MAR_I32, buf);
                buf_write(L, rec_buf.data, rec_buf.head, buf);
                */
// /STEVE CHANGE
                buf_done(L, &rec_buf);
                lua_pop(L, 1);
            }
            else {
                tag = MAR_TVAL;

                lua_pushvalue(L, -1);
                lua_pushinteger(L, (*idx)++);
                lua_rawset(L, SEEN_IDX);

                lua_pushvalue(L, -1);
                buf_init(L, &rec_buf);
                mar_encode_table(L, &rec_buf, idx);
                lua_pop(L, 1);
// STEVE CHANGE
                WriteTagRecord(L, tag, &rec_buf, buf);
                /*
                buf_write(L, (void*)&tag, MAR_CHR, buf);
                buf_write(L, (void*)&rec_buf.head, MAR_I32, buf);
                buf_write(L, rec_buf.data,rec_buf.head, buf);
                */
// /STEVE CHANGE
                buf_done(L, &rec_buf);
            }
        }
        break;
    }
    case LUA_TFUNCTION: {
        int tag, ref;
        lua_pushvalue(L, -1);
        lua_rawget(L, SEEN_IDX);
        if (!lua_isnil(L, -1)) {
            ref = lua_tointeger(L, -1);
            tag = MAR_TREF;
// STEVE CHANGE
            WriteTagRef(L, tag, ref, buf);
            /*
            buf_write(L, (void*)&tag, MAR_CHR, buf);
            buf_write(L, (void*)&ref, MAR_I32, buf);
            */
// /STEVE CHANGE
            lua_pop(L, 1);
        }
        else {
			mar_Buffer rec_buf = { 0 }; // <- STEVE CHANGE
            int i;
            lua_Debug ar;
            lua_pop(L, 1); /* pop nil */

            lua_pushvalue(L, -1);
            lua_getinfo(L, ">nuS", &ar);
            if (ar.what[0] != 'L') {
                luaL_error(L, "attempt to persist a C function '%s'", ar.name);
            }
            tag = MAR_TVAL;
            lua_pushvalue(L, -1);
            lua_pushinteger(L, (*idx)++);
            lua_rawset(L, SEEN_IDX);

            lua_pushvalue(L, -1);
            buf_init(L, &rec_buf);
            lua_dump(L, (lua_Writer)buf_write, &rec_buf);
// STEVE CHANGE
            WriteTagRecord(L, tag, &rec_buf, buf);
            /*
            buf_write(L, (void*)&tag, MAR_CHR, buf);
            buf_write(L, (void*)&rec_buf.head, MAR_I32, buf);
            buf_write(L, rec_buf.data, rec_buf.head, buf);
            */
// /STEVE CHANGE
            buf_done(L, &rec_buf);
            lua_pop(L, 1);

            lua_newtable(L);

            for (i=1; i <= ar.nups; i++) {
				lua_getupvalue(L, -2, i);
                lua_rawseti(L, -2, i);
            }

            buf_init(L, &rec_buf);
            mar_encode_table(L, &rec_buf, idx);
// STEVE CHANGE
            WriteRecord(L, &rec_buf, buf);
            /*
            buf_write(L, (void*)&rec_buf.head, MAR_I32, buf);
            buf_write(L, rec_buf.data, rec_buf.head, buf);
            */
// /STEVE CHANGE
            buf_done(L, &rec_buf);
            lua_pop(L, 1);
        }

        break;
    }
    case LUA_TUSERDATA: {
        int tag, ref;
        lua_pushvalue(L, -1);
        lua_rawget(L, SEEN_IDX);
        if (!lua_isnil(L, -1)) {
            ref = lua_tointeger(L, -1);
            tag = MAR_TREF;
// STEVE CHANGE
            WriteTagRef(L, tag, ref, buf);
            /*
            buf_write(L, (void*)&tag, MAR_CHR, buf);
            buf_write(L, (void*)&ref, MAR_I32, buf);
            */
// /STEVE CHANGE
            lua_pop(L, 1);
        }
        else {
			mar_Buffer rec_buf = { 0 }; // <- STEVE CHANGE
            lua_pop(L, 1); /* pop nil */
            if (luaL_getmetafield(L, -1, "__persist")) {
                tag = MAR_TUSR;

                lua_pushvalue(L, -2);
                lua_pushinteger(L, (*idx)++);
                lua_rawset(L, SEEN_IDX);

                lua_pushvalue(L, -2);
                lua_call(L, 1, 1);
                if (!lua_isfunction(L, -1)) {
                    luaL_error(L, "__persist must return a function");
                }
                lua_newtable(L);
                lua_pushvalue(L, -2);
                lua_rawseti(L, -2, 1);
                lua_remove(L, -2);

                buf_init(L, &rec_buf);
                mar_encode_table(L, &rec_buf, idx);
// STEVE CHANGE
                WriteTagRecord(L, tag, &rec_buf, buf);
                /*
                buf_write(L, (void*)&tag, MAR_CHR, buf);
                buf_write(L, (void*)&rec_buf.head, MAR_I32, buf);
                buf_write(L, rec_buf.data, rec_buf.head, buf);
                */
// /STEVE CHANGE
                buf_done(L, &rec_buf);
            }
            else {
                luaL_error(L, "attempt to encode userdata (no __persist hook)");
            }
            lua_pop(L, 1);
        }
        break;
    }
    case LUA_TNIL: break;
    default:
        luaL_error(L, "invalid value type (%s)", lua_typename(L, val_type));
    }
    lua_pop(L, 1);
}

static int mar_encode_table(lua_State *L, mar_Buffer *buf, size_t *idx)
{
    lua_pushnil(L);
    while (lua_next(L, -2) != 0) {
        mar_encode_value(L, buf, -2, idx);
        mar_encode_value(L, buf, -1, idx);
        lua_pop(L, 1);
    }
    return 1;
}

#define mar_incr_ptr(l) \
    if (((*p)-buf)+(l) > len) luaL_error(L, "bad code"); (*p) += (l);

#define mar_next_len(l,T) \
    if (((*p)-buf)+sizeof(T) > len) luaL_error(L, "bad code"); \
    l = *(T*)*p; (*p) += sizeof(T);
// STEVE CHANGE
#define ReadLen(l, T)	if (((*p)-buf)+sizeof(T) > len) luaL_error(L, "bad code");						\
						else {																			\
							uint32_t ul = (unsigned char)*(*p)++; size_t i;								\
							for (i = 1; i < sizeof(T); ++i) { ul <<= 8; ul += (unsigned char)*(*p)++; }	\
							l = (T)ul;                                                                  \
						}

// /STEVE CHANGE
static void mar_decode_value
    (lua_State *L, const char *buf, size_t len, const char **p, size_t *idx)
{
    size_t l;
    char val_type = **p;
    mar_incr_ptr(MAR_CHR);

    switch (val_type) {
    case LUA_TBOOLEAN:
        lua_pushboolean(L, *(char*)*p);
        mar_incr_ptr(MAR_CHR);
        break;
    case LUA_TNUMBER:
// STEVE CHANGE (iOS did not like the original, potentially misaligned form)
        {
            char nbuf[sizeof(lua_Number)];
            
            memcpy(nbuf, *p, sizeof(lua_Number));

            lua_pushnumber(L, *(lua_Number*)nbuf);//*p);
        }
// /STEVE CHANGE
        mar_incr_ptr(MAR_I64);
        break;
    case LUA_TSTRING:
// STEVE CHANGE
		ReadLen(l, uint32_t);
    //    mar_next_len(l, uint32_t);
// /STEVE CHANGE
        lua_pushlstring(L, *p, l);
        mar_incr_ptr(l);
        break;
    case LUA_TTABLE: {
        char tag = *(char*)*p;
        mar_incr_ptr(MAR_CHR);
        if (tag == MAR_TREF) {
            int ref;
// STEVE CHANGE
			ReadLen(ref, int);
        //    mar_next_len(ref, int);
// /STEVE CHANGE
            lua_rawgeti(L, SEEN_IDX, ref);
        }
        else if (tag == MAR_TVAL) {
// STEVE CHANGE
			ReadLen(l, uint32_t);
        //    mar_next_len(l, uint32_t);
// /STEVE CHANGE
            lua_newtable(L);
            lua_pushvalue(L, -1);
            lua_rawseti(L, SEEN_IDX, (*idx)++);
            mar_decode_table(L, *p, l, idx);
            mar_incr_ptr(l);
        }
        else if (tag == MAR_TUSR) {
// STEVE CHANGE
			ReadLen(l, uint32_t);
        //    mar_next_len(l, uint32_t);
// /STEVE CHANGE
            lua_newtable(L);
            mar_decode_table(L, *p, l, idx);
            lua_rawgeti(L, -1, 1);
            lua_call(L, 0, 1);
            lua_remove(L, -2);
            lua_pushvalue(L, -1);
            lua_rawseti(L, SEEN_IDX, (*idx)++);
            mar_incr_ptr(l);
        }
        else {
            luaL_error(L, "bad encoded data");
        }
        break;
    }
    case LUA_TFUNCTION: {
        size_t nups;
        int i;
        mar_Buffer dec_buf;
        char tag = *(char*)*p;
        mar_incr_ptr(1);
        if (tag == MAR_TREF) {
            int ref;
// STEVE CHANGE
			ReadLen(ref, int);
        //    mar_next_len(ref, int);
// /STEVE CHANGE
            lua_rawgeti(L, SEEN_IDX, ref);
        }
        else {
// STEVE CHANGE
			ReadLen(l, uint32_t);
        //    mar_next_len(l, uint32_t);
// /STEVE CHANGE
            dec_buf.data = (char*)*p;
            dec_buf.size = l;
            dec_buf.head = l;
            dec_buf.seek = 0;
            lua_load(L, (lua_Reader)buf_read, &dec_buf, "=marshal");
            mar_incr_ptr(l);

            lua_pushvalue(L, -1);
            lua_rawseti(L, SEEN_IDX, (*idx)++);
// STEVE CHANGE
			ReadLen(l, uint32_t);
        //    mar_next_len(l, uint32_t);
// /STEVE CHANGE
            lua_newtable(L);
            mar_decode_table(L, *p, l, idx);
            nups = lua_objlen(L, -1);
            for (i=1; i <= nups; i++) {
                lua_rawgeti(L, -1, i);
				lua_setupvalue(L, -3, i);
            }
            lua_pop(L, 1);
            mar_incr_ptr(l);
        }
        break;
    }
    case LUA_TUSERDATA: {
        char tag = *(char*)*p;
        mar_incr_ptr(MAR_CHR);
        if (tag == MAR_TREF) {
            int ref;
// STEVE CHANGE
			ReadLen(ref, int);
        //    mar_next_len(ref, int);
// STEVE CHANGE
            lua_rawgeti(L, SEEN_IDX, ref);
        }
        else if (tag == MAR_TUSR) {
// STEVE CHANGE
			ReadLen(l, uint32_t);
        //    mar_next_len(l, uint32_t);
// /STEVE CHANGE
            lua_newtable(L);
            mar_decode_table(L, *p, l, idx);
            lua_rawgeti(L, -1, 1);
            lua_call(L, 0, 1);
            lua_remove(L, -2);
            lua_pushvalue(L, -1);
            lua_rawseti(L, SEEN_IDX, (*idx)++);
            mar_incr_ptr(l);
        }
        else { /* tag == MAR_TVAL */
            lua_pushnil(L);
        }
        break;
    }
    case LUA_TNIL:
    case LUA_TTHREAD:
        lua_pushnil(L);
        break;
    default:
        luaL_error(L, "bad code");
    }
}

static int mar_decode_table(lua_State *L, const char* buf, size_t len, size_t *idx)
{
    const char* p;
    p = buf;
    while (p - buf < len) {
        mar_decode_value(L, buf, len, &p, idx);
        mar_decode_value(L, buf, len, &p, idx);
        lua_settable(L, -3);
    }
    return 1;
}

static int mar_encode(lua_State* L)
{
    const unsigned char m = MAR_MAGIC;
    size_t idx, len;
	mar_Buffer buf = { 0 }; // <- STEVE CHANGE
    if (lua_isnone(L, 1)) {
        lua_pushnil(L);
    }
// STEVE CHANGE
	if (is_blob(L, 2, &buf.resizable, &buf.size))
	{
		lua_pushnil(L);	// data, blob, nil
		lua_pushnil(L);	// data, blob, nil, nil
		lua_insert(L, 2);	// data, nil, blob, nil
		lua_insert(L, 3);	// data, nil, nil, blob

		buf.arg = 4;
	}

	else if (is_blob(L, 3, &buf.resizable, &buf.size))
	{
		lua_pushnil(L);	// data, constants?, blob, nil
		lua_insert(L, 3);	// data, constants?, nil, blob

		buf.arg = 4;
	}
// /STEVE CHANGE
    if (lua_isnoneornil(L, 2)) {
// STEVE CHANGE
		// lua_newtable(L);
		lua_pushvalue(L, lua_upvalueindex(1));	// data[, nil][, nil, blob], empty

		if (lua_isnil(L, 2)) lua_replace(L, 2);	// data, empty[, nil, blob]
// /STEVE CHANGE
    }
    else if (!lua_istable(L, 2)) {
        luaL_error(L, "bad argument #2 to encode (expected table)");
    }
    lua_settop(L, buf.arg ? 4 : 2); // data, constants OR data, constants, nil, blob

    len = lua_objlen(L, 2);
    lua_newtable(L);
// STEVE CHANGE
	if (buf.arg) lua_replace(L, SEEN_IDX);	// data, constants, seen, blob
// /STEVE CHANGE
    for (idx = 1; idx <= len; idx++) {
        lua_rawgeti(L, 2, idx);
        if (lua_isnil(L, -1)) {
            lua_pop(L, 1);
            continue;
        }
        lua_pushinteger(L, idx);
        lua_rawset(L, SEEN_IDX);
    }
    lua_pushvalue(L, 1);

    buf_init(L, &buf);
    buf_write(L, (char*)(void*)&m, 1, &buf);

    mar_encode_value(L, &buf, -1, &idx);

    lua_pop(L, 1);

    if (!buf.arg) lua_pushlstring(L, buf.data, buf.head); // <- STEVE CHANGE; blob will be on top of stack otherwise

    buf_done(L, &buf);

    lua_remove(L, SEEN_IDX);

    return 1;
}

static int mar_decode(lua_State* L)
{
    size_t l, idx, len;
    const char *p;
    const char *s = bytes_checklstring/*luaL_checklstring*/(L, 1, &l); // <- STEVE CHANGE

    if (l < 1) luaL_error(L, "bad header");
    if (*(unsigned char *)s++ != MAR_MAGIC) luaL_error(L, "bad magic");
    l -= 1;

    if (lua_isnoneornil(L, 2)) {
// STEVE CHANGE
    //   lua_newtable(L);
		lua_pushvalue(L, lua_upvalueindex(1));	// data[, nil], empty

		if (lua_isnil(L, 2)) lua_replace(L, 2);	// data, empty
// /STEVE CHANGE
    }
    else if (!lua_istable(L, 2)) {
        luaL_error(L, "bad argument #2 to decode (expected table)");
    }
    lua_settop(L, 2);

    len = lua_objlen(L, 2);
    lua_newtable(L);
    for (idx = 1; idx <= len; idx++) {
        lua_rawgeti(L, 2, idx);
        lua_rawseti(L, SEEN_IDX, idx);
    }

    p = s;
    mar_decode_value(L, s, l, &p, &idx);

    lua_remove(L, SEEN_IDX);
    lua_remove(L, 2);

    return 1;
}

static int mar_clone(lua_State* L)
{
    mar_encode(L);
    lua_replace(L, 1);
    mar_decode(L);
    return 1;
}

static const luaL_reg R[] =
{
    {"encode",      mar_encode},
    {"decode",      mar_decode},
    {"clone",       mar_clone},
    {NULL,	    NULL}
};

int luaopen_marshal(lua_State *L)
{
	int i;// <- STEVE CHANGE
    lua_newtable(L);
// STEVE CHANGE
    //luaL_register(L, NULL, R);
	lua_newtable(L);	// ..., marshal, empty

	for (i = 0; R[i].func; ++i)
	{
		lua_pushvalue(L, -1);	// ..., marshal, empty, empty
		lua_pushcclosure(L, R[i].func, 1);	// ..., marshal, empty, func
		lua_setfield(L, -3, R[i].name);	// ..., marshal = { ..., name = func }, empty
	}

	lua_pop(L, 1);	// ..., marshal
// /STEVE CHANGE
    return 0;	// <- STEVE CHANGE
}
