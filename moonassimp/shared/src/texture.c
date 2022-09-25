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

#include "internal.h"

int newtexture(lua_State *L, scene_t *scene, texture_t *texture)
    {
    ud_t *ud;
    TRACE_CREATE(texture, "texture");
    ud = newuserdata(L, (void*)texture, TEXTURE_MT);
    ud->scene = scene;
    return 1;   
    }

int freetexture(lua_State *L, texture_t *texture)
    {
    TRACE_DELETE(texture, "texture");
    freeuserdata(L, texture);
    return 0;
    }

#define Compressed(tex) (((tex)->mHeight == 0)) /* compressed texture */

static int IsCompressed(lua_State *L)
    {
    texture_t *texture = checktexture(L, 1);
    lua_pushboolean(L, Compressed(texture));
    return 1;
    }

static int Width(lua_State *L)
    {
    texture_t *texture = checktexture(L, 1);
    if(Compressed(texture)) return 0;
    lua_pushinteger(L, texture->mWidth);
    return 1;
    }

static int Height(lua_State *L)
    {
    texture_t *texture = checktexture(L, 1);
    if(Compressed(texture)) return 0;
    lua_pushinteger(L, texture->mHeight);
    return 1;
    }

static int Texel_(lua_State *L, int asfloat)
    {
    unsigned int u, v, i;
    texture_t *texture = checktexture(L, 1);
    if(Compressed(texture)) return 0;
    u = checkindex(L, 2);
    v = checkindex(L, 3);
    if(u >= texture->mWidth)
        return luaL_argerror(L, 2, "out of range");
    if(v >= texture->mHeight)
        return luaL_argerror(L, 3, "out of range");
    i =  v * texture->mWidth + u; /* texel index */
    if(asfloat)
        return pushtexelf(L, &(texture->pcData[i]), 0);
    return pushtexel(L, &(texture->pcData[i]), 0);
    }

static int BTexel(lua_State *L)
    { return Texel_(L, 0); }

static int Texel(lua_State *L)
    { return Texel_(L, 1); }

static int Texels_(lua_State *L, int asfloat)
/* returns a W*H matrix (table) of textels (4-value tables) */
    {
#define W texture->mWidth
#define H texture->mHeight
    unsigned int u, v, i;
    int (*push)(lua_State*L, texel_t*, int) = (asfloat==0) ? pushtexel : pushtexelf;
    texture_t *texture = checktexture(L, 1);
    if(Compressed(texture)) return 0;
    lua_newtable(L);
    for(v = 0; v < H; v++)
        {
        lua_newtable(L); /* v-th row */
        for(u = 0; u < W; u++)
            {
            i =  v*W + u; /* texel index */
            push(L, &(texture->pcData[i]), 1); /* u-th texel in the v-th row */
            lua_rawseti(L, -2, u+1);
            }
        lua_rawseti(L, -2, v+1);
        }   
    return 1;
#undef W
#undef H
    }

static int BTexels(lua_State *L)
    { return Texels_(L, 0); }

static int Texels(lua_State *L)
    { return Texels_(L, 1); }


static int Size(lua_State *L)
/* size = texture:size() */
    {
    texture_t *texture = checktexture(L, 1);
    if(!Compressed(texture)) return 0;
    lua_pushinteger(L, texture->mWidth);
    return 1;
    }

static int FormatHint(lua_State *L)
    {
    texture_t *texture = checktexture(L, 1);
    if(!Compressed(texture)) return 0;
    lua_pushstring(L, texture->achFormatHint);
    return 1;
    }

static int Data(lua_State *L)
/* returns data as a binary string */
    {
    size_t len;
    texture_t *texture = checktexture(L, 1);
    if(!Compressed(texture)) return 0;
    len = texture->mWidth;
    lua_pushlstring(L, (char*)(texture->pcData), len);
    return 1;
    }


/*------------------------------------------------------------------------------*
 | Registration                                                                 |
 *------------------------------------------------------------------------------*/

static const struct luaL_Reg Methods[] = 
    {
        { "is_compressed", IsCompressed },
        { "width", Width },
        { "height", Height },
        { "format_hint", FormatHint },
        { "btexel", BTexel },
        { "btexels", BTexels },
        { "texel", Texel },
        { "texels", Texels },
        { "data", Data },
        { "size", Size },
        { NULL, NULL } /* sentinel */
    };

static const struct luaL_Reg MetaMethods[] = 
    {
        { NULL, NULL } /* sentinel */
    };

static const struct luaL_Reg Functions[] = 
    {
        { NULL, NULL } /* sentinel */
    };


void moonassimp_open_texture(lua_State *L)
    {
    udata_define(L, TEXTURE_MT, Methods, MetaMethods);
    luaL_setfuncs(L, Functions, 0);
    }

