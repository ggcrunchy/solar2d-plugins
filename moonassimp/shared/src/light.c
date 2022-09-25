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

int newlight(lua_State *L, scene_t *scene, light_t *light)
    {
    ud_t *ud;
    TRACE_CREATE(light, "light");
    ud = newuserdata(L, (void*)light, LIGHT_MT);
    ud->scene = scene;
    return 1;   
    }

int freelight(lua_State *L, light_t *light)
    {
    TRACE_DELETE(light, "light");
    freeuserdata(L, light);
    return 0;
    }

static int Name(lua_State *L)
    {
    light_t *light = checklight(L, 1);
    if(light->mName.length == 0)
        return 0;
    lua_pushstring(L, light->mName.data);
    return 1;
    }

static int SourceType(lua_State *L)
    {
    light_t *light = checklight(L, 1);
    pushlightsourcetype(L, light->mType);
    return 1;
    }

#define F(what)                         \
static int what(lua_State *L)           \
    {                                   \
    light_t *light = checklight(L, 1);  \
    lua_pushnumber(L, light->m##what);  \
    return 1;                           \
    }

F(AttenuationConstant)
F(AttenuationLinear)
F(AttenuationQuadratic)
F(AngleInnerCone)
F(AngleOuterCone)
#undef F


#define F(what)                                 \
static int what(lua_State *L)                   \
    {                                           \
    light_t *light = checklight(L, 1);          \
    return pushvector3(L, &(light->m##what), 0);\
    }

F(Position)
F(Direction)
#undef F


#define F(what)                                 \
static int what(lua_State *L)                   \
    {                                           \
    light_t *light = checklight(L, 1);          \
    return pushcolor3(L, &(light->m##what), 0); \
    }

F(ColorDiffuse)
F(ColorSpecular)
F(ColorAmbient)
#undef F




/*------------------------------------------------------------------------------*
 | Registration                                                                 |
 *------------------------------------------------------------------------------*/

static const struct luaL_Reg Methods[] = 
    {
        { "name", Name },
        { "source_type", SourceType },
        { "attenuation_constant", AttenuationConstant },
        { "attenuation_linear", AttenuationLinear },
        { "attenuation_quadratic", AttenuationQuadratic },
        { "angle_inner_cone", AngleInnerCone },
        { "angle_outer_cone", AngleOuterCone },
        { "position", Position },
        { "direction", Direction },
        { "color_diffuse", ColorDiffuse },
        { "color_specular", ColorSpecular },
        { "color_ambient", ColorAmbient },
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


void moonassimp_open_light(lua_State *L)
    {
    udata_define(L, LIGHT_MT, Methods, MetaMethods);
    luaL_setfuncs(L, Functions, 0);
    }

