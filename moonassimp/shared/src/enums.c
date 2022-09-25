/* The MIT License (MIT)
 *
 * Copyright (c) 2016 Stefano Trettel
 *
 * Software repository: MoonFLTK, https://github.com/stetre/moonfltk
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


/*----------------------------------------------------------------------*
 | aiAnimBehaviour                                                      |
 *----------------------------------------------------------------------*/

unsigned int checkanimbehaviour(lua_State *L, int arg)
    {
    const char *s = luaL_checkstring(L, arg);
#define CASE(CODE,str) if((strcmp(s, str)==0)) return CODE
    CASE(aiAnimBehaviour_DEFAULT, "default");
    CASE(aiAnimBehaviour_CONSTANT, "constant");
    CASE(aiAnimBehaviour_LINEAR, "linear");
    CASE(aiAnimBehaviour_REPEAT, "repeat");
#undef CASE
    return (unsigned int)luaL_argerror(L, arg, badvalue(L,s));
    }

int pushanimbehaviour(lua_State *L, unsigned int value)
    {
    switch(value)
        {
#define CASE(CODE,str) case CODE: lua_pushstring(L, str); break
    CASE(aiAnimBehaviour_DEFAULT, "default");
    CASE(aiAnimBehaviour_CONSTANT, "constant");
    CASE(aiAnimBehaviour_LINEAR, "linear");
    CASE(aiAnimBehaviour_REPEAT, "repeat");
#undef CASE
        default:
            return unexpected(L);
        }
    return 1;
    }


/*----------------------------------------------------------------------*
 | aiDefaultLogStream                                                   |
 *----------------------------------------------------------------------*/

unsigned int checklogstream(lua_State *L, int arg)
    {
    const char *s = luaL_checkstring(L, arg);
#define CASE(CODE,str) if((strcmp(s, str)==0)) return CODE
    CASE(0, "user"); // user Lua callback
    CASE(aiDefaultLogStream_FILE, "file");
    CASE(aiDefaultLogStream_STDOUT, "stdout");
    CASE(aiDefaultLogStream_STDERR, "stderr");
    //CASE(aiDefaultLogStream_DEBUGGER, "debugger");
#undef CASE
    return (unsigned int)luaL_argerror(L, arg, badvalue(L,s));
    }

int pushlogstream(lua_State *L, unsigned int value)
    {
    switch(value)
        {
#define CASE(CODE,str) case CODE: lua_pushstring(L, str); break
    CASE(0, "user"); // user Lua callback
    CASE(aiDefaultLogStream_FILE, "file");
    CASE(aiDefaultLogStream_STDOUT, "stdout");
    CASE(aiDefaultLogStream_STDERR, "stderr");
    //CASE(aiDefaultLogStream_DEBUGGER, "debugger");
#undef CASE
        default:
            return unexpected(L);
        }
    return 1;
    }

/*----------------------------------------------------------------------*
 | aiBlendMode                                                          |
 *----------------------------------------------------------------------*/

unsigned int checkblendmode(lua_State *L, int arg)
    {
    const char *s = luaL_checkstring(L, arg);
#define CASE(CODE,str) if((strcmp(s, str)==0)) return CODE
    CASE(aiBlendMode_Default, "default");
    CASE(aiBlendMode_Additive, "additive");
#undef CASE
    return (unsigned int)luaL_argerror(L, arg, badvalue(L,s));
    }

int pushblendmode(lua_State *L, unsigned int value)
    {
    switch(value)
        {
#define CASE(CODE,str) case CODE: lua_pushstring(L, str); break
    CASE(aiBlendMode_Default, "default");
    CASE(aiBlendMode_Additive, "additive");
#undef CASE
        default:
            return unexpected(L);
        }
    return 1;
    }


/*----------------------------------------------------------------------*
 | aiShadingMode                                                        |
 *----------------------------------------------------------------------*/

unsigned int checkshadingmode(lua_State *L, int arg)
    {
    const char *s = luaL_checkstring(L, arg);
#define CASE(CODE,str) if((strcmp(s, str)==0)) return CODE
    CASE(aiShadingMode_Flat, "flat");
    CASE(aiShadingMode_Gouraud, "gouraud");
    CASE(aiShadingMode_Phong, "phong");
    CASE(aiShadingMode_Blinn, "blinn");
    CASE(aiShadingMode_Toon, "toon");
    CASE(aiShadingMode_OrenNayar, "oren nayar");
    CASE(aiShadingMode_Minnaert, "minnaert");
    CASE(aiShadingMode_CookTorrance, "cook torrance");
    CASE(aiShadingMode_NoShading, "no shading");
    CASE(aiShadingMode_Fresnel, "fresnel");
#undef CASE
    return (unsigned int)luaL_argerror(L, arg, badvalue(L,s));
    }

int pushshadingmode(lua_State *L, unsigned int value)
    {
    switch(value)
        {
#define CASE(CODE,str) case CODE: lua_pushstring(L, str); break
    CASE(aiShadingMode_Flat, "flat");
    CASE(aiShadingMode_Gouraud, "gouraud");
    CASE(aiShadingMode_Phong, "phong");
    CASE(aiShadingMode_Blinn, "blinn");
    CASE(aiShadingMode_Toon, "toon");
    CASE(aiShadingMode_OrenNayar, "oren nayar");
    CASE(aiShadingMode_Minnaert, "minnaert");
    CASE(aiShadingMode_CookTorrance, "cook torrance");
    CASE(aiShadingMode_NoShading, "no shading");
    CASE(aiShadingMode_Fresnel, "fresnel");
#undef CASE
        default:
            return unexpected(L);
        }
    return 1;
    }


/*----------------------------------------------------------------------*
 | aiTextureType                                                        |
 *----------------------------------------------------------------------*/

unsigned int checktexturetype(lua_State *L, int arg)
    {
    const char *s = luaL_checkstring(L, arg);
#define CASE(CODE,str) if((strcmp(s, str)==0)) return CODE
    CASE(aiTextureType_NONE, "none");
    CASE(aiTextureType_DIFFUSE, "diffuse");
    CASE(aiTextureType_SPECULAR, "specular");
    CASE(aiTextureType_AMBIENT, "ambient");
    CASE(aiTextureType_EMISSIVE, "emissive");
    CASE(aiTextureType_HEIGHT, "height");
    CASE(aiTextureType_NORMALS, "normals");
    CASE(aiTextureType_SHININESS, "shininess");
    CASE(aiTextureType_OPACITY, "opacity");
    CASE(aiTextureType_DISPLACEMENT, "displacement");
    CASE(aiTextureType_LIGHTMAP, "lightmap");
    CASE(aiTextureType_REFLECTION, "reflection");
    CASE(aiTextureType_UNKNOWN, "unknown");
#undef CASE
    return (unsigned int)luaL_argerror(L, arg, badvalue(L,s));
    }

int pushtexturetype(lua_State *L, unsigned int value)
    {
    switch(value)
        {
#define CASE(CODE,str) case CODE: lua_pushstring(L, str); break
    CASE(aiTextureType_NONE, "none");
    CASE(aiTextureType_DIFFUSE, "diffuse");
    CASE(aiTextureType_SPECULAR, "specular");
    CASE(aiTextureType_AMBIENT, "ambient");
    CASE(aiTextureType_EMISSIVE, "emissive");
    CASE(aiTextureType_HEIGHT, "height");
    CASE(aiTextureType_NORMALS, "normals");
    CASE(aiTextureType_SHININESS, "shininess");
    CASE(aiTextureType_OPACITY, "opacity");
    CASE(aiTextureType_DISPLACEMENT, "displacement");
    CASE(aiTextureType_LIGHTMAP, "lightmap");
    CASE(aiTextureType_REFLECTION, "reflection");
    CASE(aiTextureType_UNKNOWN, "unknown");
#undef CASE
        default:
            return unexpected(L);
        }
    return 1;
    }


/*----------------------------------------------------------------------*
 | aiTextureMapping                                                     |
 *----------------------------------------------------------------------*/

unsigned int checktexturemapping(lua_State *L, int arg)
    {
    const char *s = luaL_checkstring(L, arg);
#define CASE(CODE,str) if((strcmp(s, str)==0)) return CODE
    CASE(aiTextureMapping_UV, "uv");
    CASE(aiTextureMapping_SPHERE, "sphere");
    CASE(aiTextureMapping_CYLINDER, "cylinder");
    CASE(aiTextureMapping_BOX, "box");
    CASE(aiTextureMapping_PLANE, "plane");
    CASE(aiTextureMapping_OTHER, "other");
#undef CASE
    return (unsigned int)luaL_argerror(L, arg, badvalue(L,s));
    }

int pushtexturemapping(lua_State *L, unsigned int value)
    {
    switch(value)
        {
#define CASE(CODE,str) case CODE: lua_pushstring(L, str); break
    CASE(aiTextureMapping_UV, "uv");
    CASE(aiTextureMapping_SPHERE, "sphere");
    CASE(aiTextureMapping_CYLINDER, "cylinder");
    CASE(aiTextureMapping_BOX, "box");
    CASE(aiTextureMapping_PLANE, "plane");
    CASE(aiTextureMapping_OTHER, "other");
#undef CASE
        default:
            return unexpected(L);
        }
    return 1;
    }


/*----------------------------------------------------------------------*
 | aiTextureMapMode                                                     |
 *----------------------------------------------------------------------*/


unsigned int checktexturemapmode(lua_State *L, int arg)
    {
    const char *s = luaL_checkstring(L, arg);
#define CASE(CODE,str) if((strcmp(s, str)==0)) return CODE
    CASE(aiTextureMapMode_Wrap, "wrap");
    CASE(aiTextureMapMode_Clamp, "clamp");
    CASE(aiTextureMapMode_Decal, "decal");
    CASE(aiTextureMapMode_Mirror, "mirror");
#undef CASE
    return (unsigned int)luaL_argerror(L, arg, badvalue(L,s));
    }

int pushtexturemapmode(lua_State *L, unsigned int value)
    {
    switch(value)
        {
#define CASE(CODE,str) case CODE: lua_pushstring(L, str); break
    CASE(aiTextureMapMode_Wrap, "wrap");
    CASE(aiTextureMapMode_Clamp, "clamp");
    CASE(aiTextureMapMode_Decal, "decal");
    CASE(aiTextureMapMode_Mirror, "mirror");
#undef CASE
        default:
            return unexpected(L);
        }
    return 1;
    }


/*----------------------------------------------------------------------*
 | aiTextureOp                                                          |
 *----------------------------------------------------------------------*/

unsigned int checktextureop(lua_State *L, int arg)
    {
    const char *s = luaL_checkstring(L, arg);
#define CASE(CODE,str) if((strcmp(s, str)==0)) return CODE
    CASE(aiTextureOp_Multiply, "multiply");
    CASE(aiTextureOp_Add, "add");
    CASE(aiTextureOp_Subtract, "subtract");
    CASE(aiTextureOp_Divide, "divide");
    CASE(aiTextureOp_SmoothAdd, "smooth add");
    CASE(aiTextureOp_SignedAdd, "signed add");
#undef CASE
    return (unsigned int)luaL_argerror(L, arg, badvalue(L,s));
    }

int pushtextureop(lua_State *L, unsigned int value)
    {
    switch(value)
        {
#define CASE(CODE,str) case CODE: lua_pushstring(L, str); break
    CASE(aiTextureOp_Multiply, "multiply");
    CASE(aiTextureOp_Add, "add");
    CASE(aiTextureOp_Subtract, "subtract");
    CASE(aiTextureOp_Divide, "divide");
    CASE(aiTextureOp_SmoothAdd, "smooth add");
    CASE(aiTextureOp_SignedAdd, "signed add");
#undef CASE
        default:
            return unexpected(L);
        }
    return 1;
    }


/*----------------------------------------------------------------------*
 | Light Source Type                                                    |
 *----------------------------------------------------------------------*/

unsigned int checklightsourcetype(lua_State *L, int arg)
    {
    const char *s = luaL_checkstring(L, arg);
#define CASE(CODE,str) if((strcmp(s, str)==0)) return CODE
    CASE(aiLightSource_UNDEFINED, "undefined");
    CASE(aiLightSource_DIRECTIONAL, "directional");
    CASE(aiLightSource_POINT, "point");
    CASE(aiLightSource_SPOT, "spot");
#undef CASE
    return (unsigned int)luaL_argerror(L, arg, badvalue(L,s));
    }

int pushlightsourcetype(lua_State *L, unsigned int value)
    {
    switch(value)
        {
#define CASE(CODE,str) case CODE: lua_pushstring(L, str); break
    CASE(aiLightSource_UNDEFINED, "undefined");
    CASE(aiLightSource_DIRECTIONAL, "directional");
    CASE(aiLightSource_POINT, "point");
    CASE(aiLightSource_SPOT, "spot");
#undef CASE
        default:
            return unexpected(L);
        }
    return 1;
    }

