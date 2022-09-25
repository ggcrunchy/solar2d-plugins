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

int newcamera(lua_State *L, scene_t *scene, camera_t *camera)
    {
    ud_t *ud;
    TRACE_CREATE(camera, "camera");
    ud = newuserdata(L, (void*)camera, CAMERA_MT);
    ud->scene = scene;
    return 1;   
    }

int freecamera(lua_State *L, camera_t *camera)
    {
    TRACE_DELETE(camera, "camera");
    freeuserdata(L, camera);
    return 0;
    }

static int Name(lua_State *L)
    {
    camera_t *camera = checkcamera(L, 1);
    if(camera->mName.length == 0)
        return 0;
    lua_pushstring(L, camera->mName.data);
    return 1;
    }

#define F(what)                             \
static int what(lua_State *L)               \
    {                                       \
    camera_t *camera = checkcamera(L, 1);   \
    lua_pushnumber(L, camera->m##what);     \
    return 1;                               \
    }

F(HorizontalFOV)
F(ClipPlaneNear)
F(ClipPlaneFar)
F(Aspect)
#undef F


#define F(what)                                     \
static int what(lua_State *L)                       \
    {                                               \
    camera_t *camera = checkcamera(L, 1);           \
    return pushvector3(L, &(camera->m##what), 0);   \
    }

F(Position)
F(Up)
F(LookAt)
#undef F


/*------------------------------------------------------------------------------*
 | Registration                                                                 |
 *------------------------------------------------------------------------------*/

static const struct luaL_Reg Methods[] = 
    {
        { "name", Name },
        { "aspect", Aspect },
        { "clip_plane_near", ClipPlaneNear },
        { "clip_plane_far", ClipPlaneFar },
        { "horizontal_fov", HorizontalFOV },
        { "position", Position },
        { "up", Up },
        { "look_at", LookAt },
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

void moonassimp_open_camera(lua_State *L)
    {
    udata_define(L, CAMERA_MT, Methods, MetaMethods);
    luaL_setfuncs(L, Functions, 0);
    }


