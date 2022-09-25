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

int newmaterial(lua_State *L, scene_t *scene, material_t *material)
    {
    ud_t *ud;
    TRACE_CREATE(material, "material");
    ud = newuserdata(L, (void*)material, MATERIAL_MT);
    ud->scene = scene;
    return 1;   
    }

int freematerial(lua_State *L, material_t *material)
    {
    TRACE_DELETE(material, "material");
    freeuserdata(L, material);
    return 0;
    }


/*------------------------------------------------------------------------------*
 | aiGet wrappers                                                               |
 *------------------------------------------------------------------------------*/

/* The Assimp material system is described at:
 * http://www.assimp.org/lib_html/materials.html
 */


static int GetString(lua_State *L, material_t *material, const char* pKey, unsigned int type, unsigned int index)
    {
    aistring_t s;
    if(!material) material = checkmaterial(L, 1);
    s.data[0]='\0';
    s.length = 0;
    if(aiGetMaterialString(material, pKey, type, index, &s) != AI_SUCCESS) return 0;
    lua_pushlstring(L, s.data, s.length);
    return 1;
    }

static int GetColor(lua_State *L, material_t *material, const char* pKey, unsigned int type, unsigned int index)
    {
    color4_t color;
    if(!material) material = checkmaterial(L, 1);
    if(aiGetMaterialColor(material, pKey, type, index, &color) != AI_SUCCESS) return 0;
    return pushcolor4(L, &color, 0);
    }

static int GetBoolean(lua_State *L, material_t *material, const char* pKey, unsigned int type, unsigned int index)
    {
    int val;
    if(!material) material = checkmaterial(L, 1);
    if(aiGetMaterialIntegerArray(material, pKey, type, index, &val, NULL) != AI_SUCCESS) return 0;
    lua_pushboolean(L, val);
    return 1;
    }

#if 0
static int GetInteger(lua_State *L, material_t *material, const char* pKey, unsigned int type, unsigned int index)
    {
    int val;
    if(!material) material = checkmaterial(L, 1);
    if(aiGetMaterialIntegerArray(material, pKey, type, index, &val, NULL) != AI_SUCCESS) return 0;
    lua_pushinteger(L, val);
    return 1;
    }
#endif

static int GetFloat(lua_State *L, material_t *material, const char* pKey, unsigned int type, unsigned int index)
    {
    float val;
    if(!material) material = checkmaterial(L, 1);
    if(aiGetMaterialFloatArray(material, pKey, type, index, &val, NULL) != AI_SUCCESS) return 0;
    lua_pushnumber(L, val);
    return 1;
    }

/*------------------------------------------------------------------------------*
 | Get properties                                                               |
 *------------------------------------------------------------------------------*/

static int Name(lua_State *L)
    { return GetString(L, NULL, AI_MATKEY_NAME); }

static int Twosided(lua_State *L)
    { return GetBoolean(L, NULL, AI_MATKEY_TWOSIDED); }

static int Shading(lua_State *L)
    {
    int val;
    material_t *material = checkmaterial(L, 1);
    if(aiGetMaterialIntegerArray(material, AI_MATKEY_SHADING_MODEL, &val, NULL) != AI_SUCCESS) 
        return 0;
    return pushshadingmode(L, val);
    }

static int Wireframe(lua_State *L)
    { return GetBoolean(L, NULL, AI_MATKEY_ENABLE_WIREFRAME); }

static int Blend(lua_State *L)
    {
    int val;
    material_t *material = checkmaterial(L, 1);
    if(aiGetMaterialIntegerArray(material, AI_MATKEY_BLEND_FUNC, &val, NULL) != AI_SUCCESS) 
        return 0;
    return pushblendmode(L, val);
    }

static int Opacity(lua_State *L)
    { return GetFloat(L, NULL, AI_MATKEY_OPACITY); }

static int Bumpscaling(lua_State *L)
    { return GetFloat(L, NULL, AI_MATKEY_BUMPSCALING); }

static int Shininess(lua_State *L)
    { return GetFloat(L, NULL, AI_MATKEY_SHININESS); }

static int Reflectivity(lua_State *L)
    { return GetFloat(L, NULL, AI_MATKEY_REFLECTIVITY); }

static int Shinpercent(lua_State *L)
    { return GetFloat(L, NULL, AI_MATKEY_SHININESS_STRENGTH); }

static int Refraction(lua_State *L)
    { return GetFloat(L, NULL, AI_MATKEY_REFRACTI); }

static int Diffuse(lua_State *L)
    { return GetColor(L, NULL, AI_MATKEY_COLOR_DIFFUSE); }

static int Ambient(lua_State *L)
    { return GetColor(L, NULL, AI_MATKEY_COLOR_AMBIENT); }

static int Specular(lua_State *L)
    { return GetColor(L, NULL, AI_MATKEY_COLOR_SPECULAR); }

static int Emissive(lua_State *L)
    { return GetColor(L, NULL, AI_MATKEY_COLOR_EMISSIVE); }

static int Transparent(lua_State *L)
    { return GetColor(L, NULL, AI_MATKEY_COLOR_TRANSPARENT); }

static int Reflective(lua_State *L)
    { return GetColor(L, NULL, AI_MATKEY_COLOR_REFLECTIVE); }

static int GlobalBgImage(lua_State *L)
    { return GetString(L, NULL, AI_MATKEY_GLOBAL_BACKGROUND_IMAGE); }


/* Textures ----------------------------------------------------------*/

static int TextureCount(lua_State *L)
    {
    material_t *material = checkmaterial(L, 1);
    unsigned int type = checktexturetype(L, 2);
    unsigned int count = aiGetMaterialTextureCount(material, (enum aiTextureType)type);
    lua_pushinteger(L, count);
    return 1;
    }

static int Texture_path(lua_State *L)
    {
    material_t *material = checkmaterial(L, 1);
    unsigned int type = checktexturetype(L, 2);
    unsigned int index = checkindex(L, 3);
    return GetString(L, material, _AI_MATKEY_TEXTURE_BASE, type, index);
    }


static int Texture_uvwsrc(lua_State *L)
    {
    int val;
    material_t *material = checkmaterial(L, 1);
    unsigned int type = checktexturetype(L, 2);
    unsigned int index = checkindex(L, 3);
    if(aiGetMaterialIntegerArray(material, _AI_MATKEY_UVWSRC_BASE, type, index, &val, NULL) 
        != AI_SUCCESS) return 0;
    pushindex(L, val);
    return 1;
    }

static int Texture_op(lua_State *L)
    {
    int val;
    material_t *material = checkmaterial(L, 1);
    unsigned int type = checktexturetype(L, 2);
    unsigned int index = checkindex(L, 3);
    if(aiGetMaterialIntegerArray(material, _AI_MATKEY_TEXOP_BASE, type, index, &val, NULL) 
        != AI_SUCCESS) return 0;
    pushtextureop(L, val);
    return 1;
    }

static int Texture_mapping(lua_State *L)
    {
    int val;
    material_t *material = checkmaterial(L, 1);
    unsigned int type = checktexturetype(L, 2);
    unsigned int index = checkindex(L, 3);
    if(aiGetMaterialIntegerArray(material, _AI_MATKEY_MAPPING_BASE, type, index, &val, NULL) 
        != AI_SUCCESS) return 0;
    pushtexturemapping(L, val);
    return 1;
    }

static int Texture_blend(lua_State *L)
    {
    material_t *material = checkmaterial(L, 1);
    unsigned int type = checktexturetype(L, 2);
    unsigned int index = checkindex(L, 3);
    return GetFloat(L, material, _AI_MATKEY_TEXBLEND_BASE, type, index);
    }

static int Texture_mapmode(lua_State *L)
    {
    int val;
    material_t *material = checkmaterial(L, 1);
    unsigned int type = checktexturetype(L, 2);
    unsigned int index = checkindex(L, 3);
    if(aiGetMaterialIntegerArray(material, _AI_MATKEY_MAPPINGMODE_U_BASE, type, index, &val, NULL) 
        != AI_SUCCESS) lua_pushnil(L); else pushtexturemapmode(L, val);
    if(aiGetMaterialIntegerArray(material, _AI_MATKEY_MAPPINGMODE_V_BASE, type, index, &val, NULL) 
        != AI_SUCCESS) lua_pushnil(L); else pushtexturemapmode(L, val);
    return 2;
    }

static int Texture_axis(lua_State *L)
    {
    vector3_t vec;
    material_t *material = checkmaterial(L, 1);
    unsigned int type = checktexturetype(L, 2);
    unsigned int index = checkindex(L, 3);
    unsigned int max = sizeof(vector3_t);
    if(AI_SUCCESS != 
        aiGetMaterialFloatArray(material, _AI_MATKEY_TEXMAP_AXIS_BASE, type, index,
        (float*)&vec, &max) || sizeof(vector3_t) != max) return 0;
    return pushvector3(L, &vec, 0);
    }

static int Texture_uvtranslation(lua_State *L)
    {
    struct aiUVTransform trafo;
    material_t *material = checkmaterial(L, 1);
    unsigned int type = checktexturetype(L, 2);
    unsigned int index = checkindex(L, 3);
    unsigned int max = sizeof(struct aiUVTransform);
    if(AI_SUCCESS != 
        aiGetMaterialFloatArray(material, _AI_MATKEY_UVTRANSFORM_BASE, type, index,
        (float*)&trafo, &max) || sizeof(struct aiUVTransform) != max) return 0;
    return pushvector2(L, &(trafo.mTranslation), 0);
    }

static int Texture_uvscaling(lua_State *L)
    {
    struct aiUVTransform trafo;
    material_t *material = checkmaterial(L, 1);
    unsigned int type = checktexturetype(L, 2);
    unsigned int index = checkindex(L, 3);
    unsigned int max = sizeof(struct aiUVTransform);
    if(AI_SUCCESS != 
        aiGetMaterialFloatArray(material, _AI_MATKEY_UVTRANSFORM_BASE, type, index,
        (float*)&trafo, &max) || sizeof(struct aiUVTransform) != max) return 0;
    return pushvector2(L, &(trafo.mScaling), 0);
    }

static int Texture_uvrotation(lua_State *L)
    {
    struct aiUVTransform trafo;
    material_t *material = checkmaterial(L, 1);
    unsigned int type = checktexturetype(L, 2);
    unsigned int index = checkindex(L, 3);
    unsigned int max = sizeof(struct aiUVTransform);
    if(AI_SUCCESS != 
        aiGetMaterialFloatArray(material, _AI_MATKEY_UVTRANSFORM_BASE, type, index,
        (float*)&trafo, &max) || sizeof(struct aiUVTransform) != max) return 0;
    lua_pushnumber(L, trafo.mRotation);
    return 1;
    }

static int Texture_flags(lua_State *L)
    {
    int val;
    material_t *material = checkmaterial(L, 1);
    unsigned int type = checktexturetype(L, 2);
    unsigned int index = checkindex(L, 3);
    if(aiGetMaterialIntegerArray(material, _AI_MATKEY_TEXFLAGS_BASE, type, index, &val, NULL) 
        != AI_SUCCESS) val = 0;
    return pushtextureflags(L, val, 1);
    }


static int PushTexture(lua_State *L, material_t *material, unsigned int type, unsigned int index)
/* Pushes a table with all the texture properties */
    {
    int val;
    float floatval;
    vector3_t vec;
    struct aiUVTransform trafo;
    unsigned int max = sizeof(vector3_t);

    lua_newtable(L);
    pushtexturetype(L, type);
    lua_setfield(L, -2, "type");

    if(aiGetMaterialIntegerArray(material, _AI_MATKEY_TEXFLAGS_BASE, type, index, &val, NULL)
        != AI_SUCCESS) val = 0;
    pushtextureflags(L, val, 1);
    lua_setfield(L, -2, "flags");

    if(GetString(L, material, _AI_MATKEY_TEXTURE_BASE, type, index)==1)
        lua_setfield(L, -2, "path");

    if(aiGetMaterialIntegerArray(material, _AI_MATKEY_UVWSRC_BASE, type, index, &val, NULL)
        == AI_SUCCESS)
        {
        pushindex(L, val);
        lua_setfield(L, -2, "channel");
        }

    if(aiGetMaterialIntegerArray(material, _AI_MATKEY_TEXOP_BASE, type, index, &val, NULL)
        == AI_SUCCESS)
        {
        pushtextureop(L, val);
        lua_setfield(L, -2, "op");
        }

    if(aiGetMaterialIntegerArray(material, _AI_MATKEY_MAPPING_BASE, type, index, &val, NULL)
        == AI_SUCCESS)
        {
        pushtexturemapping(L, val);
        lua_setfield(L, -2, "mapping");
        }

    if(aiGetMaterialFloatArray(material, _AI_MATKEY_TEXBLEND_BASE, type, index, &floatval, NULL)
        == AI_SUCCESS)
        {
        lua_pushnumber(L, floatval);
        lua_setfield(L, -2, "blend");
        }

    if(aiGetMaterialIntegerArray(material, _AI_MATKEY_MAPPINGMODE_U_BASE, type, index, &val, NULL)
        == AI_SUCCESS)
        {
        pushtexturemapmode(L, val);
        lua_setfield(L, -2, "mapmode_u");
        }
    if(aiGetMaterialIntegerArray(material, _AI_MATKEY_MAPPINGMODE_V_BASE, type, index, &val, NULL)
        == AI_SUCCESS)
        {
        pushtexturemapmode(L, val);
        lua_setfield(L, -2, "mapmode_v");
        }

    max = sizeof(vector3_t);
    if(AI_SUCCESS == aiGetMaterialFloatArray(material, _AI_MATKEY_TEXMAP_AXIS_BASE, type, index,
        (float*)&vec, &max) && sizeof(vector3_t) == max)
        {
        pushvector3(L, &vec, 1);
        lua_setfield(L, -2, "axis");
        }

    max = sizeof(struct aiUVTransform);
    if(AI_SUCCESS ==
        aiGetMaterialFloatArray(material, _AI_MATKEY_UVTRANSFORM_BASE, type, index,
        (float*)&trafo, &max) && sizeof(struct aiUVTransform) == max)
        {
        pushvector2(L, &(trafo.mTranslation), 1);
        lua_setfield(L, -2, "translation");
        pushvector2(L, &(trafo.mScaling), 1);
        lua_setfield(L, -2, "scaling");
        lua_pushnumber(L, trafo.mRotation);
        lua_setfield(L, -2, "rotation");
        }

    return 1;
    }

static int AllTextureCounts(lua_State *L)
    {
    unsigned int count;
    material_t *material = checkmaterial(L, 1);
    lua_newtable(L);
#define ADD(t, s) do {                                                  \
    count = aiGetMaterialTextureCount(material, aiTextureType_##t);     \
    lua_pushinteger(L, count);                                          \
    lua_setfield(L, -2, s);                                             \
} while(0)
//  ADD(NONE, "none");
    ADD(DIFFUSE, "diffuse");
    ADD(SPECULAR, "specular");
    ADD(AMBIENT, "ambient");
    ADD(EMISSIVE, "emissive");
    ADD(HEIGHT, "height");
    ADD(NORMALS, "normals");
    ADD(SHININESS, "shininess");
    ADD(OPACITY, "opacity");
    ADD(DISPLACEMENT, "displacement");
    ADD(LIGHTMAP, "lightmap");
    ADD(REFLECTION, "reflection");
    ADD(UNKNOWN, "unknown");
#undef ADD
    return 1;
    }


static int AllTextures(lua_State *L)
    {
    unsigned int count, i, tot;
    unsigned int type;
    material_t *material = checkmaterial(L, 1);
#define ADD(t) do {                                     \
    count = aiGetMaterialTextureCount(material, t);     \
    for(i=0; i < count; i++)                            \
        {                                               \
        PushTexture(L, material, t, i);                 \
        lua_rawseti(L, -2, ++tot);                      \
        }                                               \
} while(0)

    tot = 0;
    if(!lua_isnoneornil(L, 2))
        {
        lua_newtable(L);
        type = checktexturetype(L, 2);
        ADD((enum aiTextureType)type);
        }
    else
        {
        lua_newtable(L);
//      ADD(aiTextureType_NONE);
        ADD(aiTextureType_DIFFUSE);
        ADD(aiTextureType_SPECULAR);
        ADD(aiTextureType_AMBIENT);
        ADD(aiTextureType_EMISSIVE);
        ADD(aiTextureType_HEIGHT);
        ADD(aiTextureType_NORMALS);
        ADD(aiTextureType_SHININESS);
        ADD(aiTextureType_OPACITY);
        ADD(aiTextureType_DISPLACEMENT);
        ADD(aiTextureType_LIGHTMAP);
        ADD(aiTextureType_REFLECTION);
        ADD(aiTextureType_UNKNOWN);
        }
#undef ADD
    return 1;
    }



/*------------------------------------------------------------------------------*
 | Registration                                                                 |
 *------------------------------------------------------------------------------*/

static const struct luaL_Reg Methods[] = 
    {
        { "name", Name },
        { "twosided", Twosided },
        { "shading", Shading },
        { "wireframe", Wireframe },
        { "blend", Blend },
        { "opacity", Opacity },
        { "bumpscaling", Bumpscaling },
        { "shininess", Shininess },
        { "reflectivity", Reflectivity },
        { "shinpercent", Shinpercent },
        { "refraction", Refraction },
        { "color_diffuse", Diffuse },
        { "color_ambient", Ambient },
        { "color_specular", Specular },
        { "color_emissive", Emissive },
        { "color_transparent", Transparent },
        { "color_reflective", Reflective },
        { "global_bg_image", GlobalBgImage },
        { "texture_count", TextureCount },
        { "texture_path", Texture_path },
        { "texture_channel", Texture_uvwsrc },
        { "texture_op", Texture_op },
        { "texture_mapping", Texture_mapping },
        { "texture_blend", Texture_blend },
        { "texture_mapmode", Texture_mapmode },
        { "texture_axis", Texture_axis },
        { "texture_translation", Texture_uvtranslation },
        { "texture_scaling", Texture_uvscaling },
        { "texture_rotation", Texture_uvrotation },
        { "texture_flags", Texture_flags },
        { "all_texture_counts", AllTextureCounts },
        { "all_textures", AllTextures },
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


void moonassimp_open_material(lua_State *L)
    {
    udata_define(L, MATERIAL_MT, Methods, MetaMethods);
    luaL_setfuncs(L, Functions, 0);
    }


