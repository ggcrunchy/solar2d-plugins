/* The MIT License (MIT)
 *
 * Copyright (c) 2016 Stefano Trettel
 *
 * Software repository: MoonAssimp, https://github.com/stetre/moonassimp
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

/********************************************************************************
 * internal common header                                                       *
 ********************************************************************************/

#ifndef internalDEFINED
#define internalDEFINED

#include <string.h>
#include <stdlib.h>
#include "moonassimp.h"
#include "objects.h"

#define TOSTR_(x) #x
#define TOSTR(x) TOSTR_(x)

/* Note: all the dynamic symbols of this library (should) start with 'moonassimp_' .
 * The only exception is the luaopen_moonassimp() function, which is searched for
 * with that name by Lua.
 * MoonAssimp's string references on the Lua registry also start with 'moonassimp_'.
 */

#if 0
/* .c */
#define  moonassimp_
#endif

/* utils.c */
#define noprintf moonassimp_noprintf
int noprintf(const char *fmt, ...);
#define notavailable moonassimp_notavailable
int notavailable(lua_State *L, ...);
#define nilerrmsg moonassimp_nilerrmsg
int nilerrmsg(lua_State *L);
#define checkoption_hint moonassimp_checkoption_hint 
int checkoption_hint(lua_State *L, int arg, const char *def, const char *const lst[]);
#define checkboolean moonassimp_checkboolean
int checkboolean(lua_State *L, int arg);
#define optboolean moonassimp_optboolean
int optboolean(lua_State *L, int arg, int d);
#define checkindex moonassimp_checkindex
int checkindex(lua_State *L, int arg);
#define optindex moonassimp_optindex
int optindex(lua_State *L, int arg, int optval /* 0-based */);
#define pushindex moonassimp_pushindex
void pushindex(lua_State *L, int val);

#define pushvector2 moonassimp_pushvector2
int pushvector2(lua_State *L, vector2_t *vec, int astable);
#define pushvector3 moonassimp_pushvector3
int pushvector3(lua_State *L, vector3_t *vec, int astable);
#define pushcolor3 moonassimp_pushcolor3
int pushcolor3(lua_State *L, color3_t *col, int astable);
#define pushcolor4 moonassimp_pushcolor4
int pushcolor4(lua_State *L, color4_t *col, int astable);
#define pushquaternion moonassimp_pushquaternion
int pushquaternion(lua_State *L, quaternion_t *quat, int astable);
#define pushtexel moonassimp_pushtexel
int pushtexel(lua_State *L, texel_t *vec, int astable);
#define pushtexelf moonassimp_pushtexelf
int pushtexelf(lua_State *L, texel_t *vec, int astable);
#define pushmatrix3 moonassimp_pushmatrix3
int pushmatrix3(lua_State *L, matrix3_t *mat, int how);
#define pushmatrix4 moonassimp_pushmatrix4
int pushmatrix4(lua_State *L, matrix4_t *mat, int how);

/* enums.c */
#define checkanimbehaviour moonassimp_checkanimbehaviour
unsigned int checkanimbehaviour(lua_State *L, int arg);
#define pushanimbehaviour moonassimp_pushanimbehaviour
int pushanimbehaviour(lua_State *L, unsigned int value);
#define checklogstream moonassimp_checklogstream
unsigned int checklogstream(lua_State *L, int arg);
#define pushlogstream moonassimp_pushlogstream
int pushlogstream(lua_State *L, unsigned int value);
#define checkblendmode moonassimp_checkblendmode
unsigned int checkblendmode(lua_State *L, int arg);
#define pushblendmode moonassimp_pushblendmode
int pushblendmode(lua_State *L, unsigned int value);
#define checkshadingmode moonassimp_checkshadingmode
unsigned int checkshadingmode(lua_State *L, int arg);
#define pushshadingmode moonassimp_pushshadingmode
int pushshadingmode(lua_State *L, unsigned int value);
#define checktexturetype moonassimp_checktexturetype
unsigned int checktexturetype(lua_State *L, int arg);
#define pushtexturetype moonassimp_pushtexturetype
int pushtexturetype(lua_State *L, unsigned int value);
#define checktexturemapping moonassimp_checktexturemapping
unsigned int checktexturemapping(lua_State *L, int arg);
#define pushtexturemapping moonassimp_pushtexturemapping
int pushtexturemapping(lua_State *L, unsigned int value);
#define checktexturemapmode moonassimp_checktexturemapmode
unsigned int checktexturemapmode(lua_State *L, int arg);
#define pushtexturemapmode moonassimp_pushtexturemapmode
int pushtexturemapmode(lua_State *L, unsigned int value);
#define checktextureop moonassimp_checktextureop
unsigned int checktextureop(lua_State *L, int arg);
#define pushtextureop moonassimp_pushtextureop
int pushtextureop(lua_State *L, unsigned int value);
#define checklightsourcetype moonassimp_checklightsourcetype
unsigned int checklightsourcetype(lua_State *L, int arg);
#define pushlightsourcetype moonassimp_pushlightsourcetype
int pushlightsourcetype(lua_State *L, unsigned int value);


/* bitfields.c */
#define checktextureflags moonassimp_checktextureflags
unsigned int checktextureflags(lua_State *L, int arg);
#define pushtextureflags moonassimp_pushtextureflags
int pushtextureflags(lua_State *L, unsigned int flags, int pushcode);
#define checkprimitivetype moonassimp_checkprimitivetype
unsigned int checkprimitivetype(lua_State *L, int arg);
#define pushprimitivetype moonassimp_pushprimitivetype
int pushprimitivetype(lua_State *L, unsigned int flags, int pushcode);
#define checksceneflags moonassimp_checksceneflags
unsigned int checksceneflags(lua_State *L, int arg);
#define pushsceneflags moonassimp_pushsceneflags
int pushsceneflags(lua_State *L, unsigned int flags, int pushcode);
#define checkpostprocessflags moonassimp_checkpostprocessflags
unsigned int checkpostprocessflags(lua_State *L, int arg);
#define pushpostprocessflags moonassimp_pushpostprocessflags
int pushpostprocessflags(lua_State *L, unsigned int flags, int pushcode);

/* face.c */
#define pushfaceindices moonassimp_pushfaceindices
int pushfaceindices(lua_State *L, face_t *face, int zero_based);

/* main.c */
int luaopen_moonassimp(lua_State *L);
void moonassimp_open_import(lua_State *L);
void moonassimp_open_scene(lua_State *L);
void moonassimp_open_node(lua_State *L);
void moonassimp_open_mesh(lua_State *L);
void moonassimp_open_animmesh(lua_State *L);
void moonassimp_open_material(lua_State *L);
void moonassimp_open_animation(lua_State *L);
void moonassimp_open_texture(lua_State *L);
void moonassimp_open_light(lua_State *L);
void moonassimp_open_camera(lua_State *L);
void moonassimp_open_face(lua_State *L);
void moonassimp_open_bone(lua_State *L);
void moonassimp_open_nodeanim(lua_State *L);
void moonassimp_open_meshanim(lua_State *L);
void moonassimp_open_additional(lua_State *L);


/*------------------------------------------------------------------------------*
 | Debug and other utilities                                                    |
 *------------------------------------------------------------------------------*/

/* Dynamic referencing on the Lua registry */

#define reference(L, dst, arg) do {                 \
    lua_pushvalue((L), (arg));                      \
    (dst) = luaL_ref((L), LUA_REGISTRYINDEX);       \
} while(0)

#define unreference(L, ref) do {                    \
    if((ref)!=LUA_NOREF) {                          \
        luaL_unref((L), LUA_REGISTRYINDEX, (ref));  \
        (ref) = LUA_NOREF; }                        \
} while(0)

#define pushvalue(L, ref) /* returns LUA_TXXX */    \
    lua_rawgeti((L), LUA_REGISTRYINDEX, (ref)) 

/* objects tracing -----------------------------------------------*/
#define trace_enabled moonvulkan_trace_enabled
extern int trace_enabled;
#define trace moonassimp_trace
int trace(const char *fmt, ...);

#define TRACE trace
#define TRACE_CREATE(p, what) do { trace("create "what" 0x%p\n", (void*)(p)); } while(0)
#define TRACE_CREATE_N(n, what) do { trace("create %d "what"\n", (n)); } while(0)
#define TRACE_DELETE(p, what) do { trace("delete "what" 0x%p\n", (void*)(p)); } while(0)
#define TRACE_DELETE_N(n, what) do { trace("delete %d "what"\n", (n)); } while(0)

/*----------------------------------------------------------------*/

/* If this is printed, it denotes a suspect bug: */
#define UNEXPECTED_ERROR "unexpected error (%s, %d)", __FILE__, __LINE__
#define unexpected(L) luaL_error((L), UNEXPECTED_ERROR)
#define NOT_AVAILABLE do { return luaL_error(L, "function not available"); } while(0)

#define badvalue(L,s)   lua_pushfstring((L), "invalid value '%s'", (s))

#define NOT_IMPLEMENTED(func)               \
static int func(lua_State *L)               \
    {                                       \
    luaL_error(L, "function "#func" is not implemented");   \
    return 0;                           \
    }

#define NOT_SUPPORTED(func)                 \
static int func(lua_State *L)               \
    {                                       \
    luaL_error(L, "function "#func" is not supported"); \
    return 0;                           \
    }

#if defined(DEBUG)

#define checkoption checkoption_hint
#define DBG printf

#define TR() do {                                           \
    printf("trace %s %d\n",__FILE__,__LINE__);              \
} while(0)

#define BK() do {                                           \
    printf("break %s %d\n",__FILE__,__LINE__);              \
    getchar();                                              \
} while(0)

#else 

#define checkoption luaL_checkoption
#define DBG noprintf
#define TR()
#define BK()

#endif /* DEBUG */

#endif /* internalDEFINED */
