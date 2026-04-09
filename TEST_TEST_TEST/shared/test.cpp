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

#include "CoronaGraphics.h"
#include "CoronaLua.h"
#include "CoronaLog.h"

#include <stdlib.h>
#include <string.h>

#include "ktx.h"
#include "gl_format.h"
#include "vkformat_enum.h"

#define GL_SR8_EXT GL_SR8
#define GL_SRG8_EXT GL_SRG8
#define GL_ALPHA8_EXT GL_ALPHA8

//
//
//


typedef struct {
	int w, h;
	const float * buf;
} ExtraTexture;
	
//
//
//

static unsigned int GetWidth (void * ud)
{
	return ((ExtraTexture *)ud)->w;
}

static unsigned int GetHeight (void * ud)
{
	return ((ExtraTexture *)ud)->h;
}

enum { kExternalBitmapFormat_RequestedByName = kExternalBitmapFormat_RGBA + 1 };

static CoronaExternalBitmapFormat GetFormat (void * _)
{
	return (CoronaExternalBitmapFormat)kExternalBitmapFormat_RequestedByName;
}

//
//
//

static const void * OnRequestBitmap (void * ud)
{
	return ((ExtraTexture *)ud)->buf;
}

//
//
//

static const char * GetRequestedFormat (void * _)
{
	return "r32f";
}

//
//
//

const float FloatData[] = {
	0, 21, 0, 11, 0,
	0, 0, 0, 21, 1,
	1, 0, 0, 0, 0,
	0, 0, 21, 0, 1,
	21, 0, 0, 11, 0
};
	
//
//
//

static unsigned int GetWidth2 (void * ud)
{
	return ((ktxTexture *)ud)->baseWidth;
}

static unsigned int GetHeight2 (void * ud)
{
	return ((ktxTexture *)ud)->baseHeight;
}

// reuse GetFormat

//
//
//

static const void * OnRequestBitmap2 (void * ud)
{
	return ((ktxTexture *)ud)->pData; // offset, etc.
}

//
//
//
/*
static const char * GetRequestedFormat2 (void * _)
{
	return "etc2-rgb";
}
*/

static unsigned int SupplyInternalFormatByValue (void * ud)
{
	ktxTexture * ktx = (ktxTexture *)ud;

	if (1 == ktx->classId) return ((ktxTexture1 *)ud)->glInternalformat;
	
	switch (((ktxTexture2 *)ud)->vkFormat)
	{
	#include "vkFormat2glInternalFormat.inl"
		default:
			return 0;
	}
}

//
//
//

static unsigned int GetCustomUploadTextureTarget (void * ud)
{
	ktxTexture * ktx = (ktxTexture *)ud;
	
	if ( !ktx->isArray && !ktx->isCubemap && 2 == ktx->numDimensions ) return 0x0DE1; // GL_TEXTURE_2D
	
	else return 0; // meaningless
}

//
//
//

static int GetCustomUploadProvidesMipmaps (void * ud)
{
	ktxTexture * ktx = (ktxTexture *)ud;
	
	return ktx->numLevels > 1;
}

//
//
//

static unsigned int CustomUpload (void * resource, void * ud)
{
	ktxTexture * ktx = (ktxTexture *)ud;
	GLenum error = 0, target;

	ktxTexture_GLUpload(ktx, (GLuint *)resource, &target, &error);
                    
	return (unsigned int)error;
}

//
//
//

static void Finalize2 (void * ud)
{
	ktxTexture_Destroy((ktxTexture *)ud);
}

//
//
//

struct CoronaExternalTextureCallbacks2 {
	CoronaExternalTextureCallbacks base;
	const char* (*getRequestedFormat)(void* userData);
	unsigned int (*supplyInternalFormatByValue)(void* userData);
	unsigned int (*getCustomUploadTextureTarget)(void* userData);
	int (*getCustomUploadProvidesMipmaps)(void* userData);
	unsigned int (*customUpload)(void* resource, void* userData);
};

//
//
//

CORONA_EXPORT int luaopen_plugin_TESTTEST (lua_State* L)
{
	lua_newtable(L); // ktx

	luaL_Reg funcs[] = {
		"load", [](lua_State * L)
		{
			const char * bytes = lua_tostring(L, 1);
			size_t len = lua_objlen(L, 1);

			ktxTexture * tex;
			KTX_error_code err = ktxTexture_CreateFromMemory((const uint8_t *)bytes, len, KTX_TEXTURE_CREATE_LOAD_IMAGE_DATA_BIT, &tex);
	     
			if (KTX_SUCCESS == err)
			{
				CoronaExternalTextureCallbacks2 callbacks = {};

				callbacks.base.size = sizeof(CoronaExternalTextureCallbacks2);
				callbacks.base.getWidth = GetWidth2;
				callbacks.base.getHeight = GetHeight2;
				callbacks.base.getFormat = GetFormat;
				callbacks.base.onRequestBitmap = OnRequestBitmap2;
				callbacks.base.onFinalize = Finalize2;
				callbacks.supplyInternalFormatByValue = SupplyInternalFormatByValue;
				callbacks.getCustomUploadTextureTarget = GetCustomUploadTextureTarget;
				callbacks.getCustomUploadProvidesMipmaps = GetCustomUploadProvidesMipmaps;
				callbacks.customUpload = CustomUpload;	
				
				CoronaExternalPushTexture(L, &callbacks.base, tex); // texture

				return 1;
			}
			
			return 0;
		},
		{ nullptr, nullptr }
	};
	
	luaL_register(L, nullptr, funcs);
	
	ktxLoadOpenGL(nullptr);
	
    return 1;
}
