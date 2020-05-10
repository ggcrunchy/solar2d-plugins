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

#include "stdafx.h"
#include "classes.h"

struct TextureData {
	CoronaExternalBitmapFormat format;
	unsigned int w, h;
	unsigned char * data;
};

unsigned GetWidth (void * context)
{
	return ((TextureData *)context)->w;
}

unsigned GetHeight (void * context)
{
	return ((TextureData *)context)->h;
}

const void * GetBitmap (void * context)
{
	return ((TextureData *)context)->data;
}

CoronaExternalBitmapFormat GetFormat (void * context)
{
	return ((TextureData *)context)->format;
}

int GetField (lua_State * L, const char * field, void * context)
{
	return 0;
}

void Finalize (void * context)
{
	TextureData * td = (TextureData *)context;

	if (td->data) delete [] td->data;

	delete td;
}

void LoadBytes3 (fipImage & image, BYTE * bytes)
{
	unsigned h = image.getHeight(), w = image.getScanWidth() / 3;

	for (unsigned i = 0; i < h; ++i)
	{
		RGBTRIPLE * rgb = (RGBTRIPLE *)image.getScanLine(h - i - 1);

		for (unsigned j = 0; j < w; ++j)
		{
			*bytes++ = rgb[j].rgbtRed;
			*bytes++ = rgb[j].rgbtGreen;
			*bytes++ = rgb[j].rgbtBlue;
		}
	}
}

void LoadBytes4 (fipImage & image, BYTE * bytes)
{
	unsigned h = image.getHeight(), w = image.getScanWidth() / 4;

	for (unsigned i = 0; i < h; ++i)
	{
		RGBQUAD * rgb = (RGBQUAD *)image.getScanLine(h - i - 1);

		for (unsigned j = 0; j < w; ++j)
		{
			*bytes++ = rgb[j].rgbRed;
			*bytes++ = rgb[j].rgbGreen;
			*bytes++ = rgb[j].rgbBlue;
			*bytes++ = rgb[j].rgbReserved;
		}
	}
}

int NewTexture (lua_State * L, fipImage & image)
{
	fipImage * dup;

	try {
		dup = new fipImage(image);
	} catch (...) {
        dup = nullptr;

		return 0;
	}

	BOOL ok = FALSE;

	CoronaExternalBitmapFormat format;

    BYTE * bytes = nullptr;

	if (dup)
	{
		if (image.isTransparent())
		{
			format = kExternalBitmapFormat_RGBA;
			ok = dup->convertTo32Bits();
		}

		else
		{
			format = kExternalBitmapFormat_RGB;
			ok = dup->convertTo24Bits();
		}

		try {
			bytes = new BYTE[dup->getHeight() * dup->getWidth() * CoronaExternalFormatBPP(format)];

			switch (CoronaExternalFormatBPP(format))
			{
			case 3:
				LoadBytes3(*dup, bytes);
				break;
			case 4:
				LoadBytes4(*dup, bytes);
				break;
			default:
				break;
			}
		} catch (...) {
			ok = FALSE;
		}

		delete dup;
	}

	if (!ok) return 0;

	TextureData * data = new TextureData;

	data->format = format;
	data->w = image.getWidth();
	data->h = image.getHeight();
	data->data = bytes;

	CoronaExternalTextureCallbacks callbacks = {};

	callbacks.size = sizeof(CoronaExternalTextureCallbacks);
	callbacks.getWidth = GetWidth;
	callbacks.getHeight = GetHeight;
	callbacks.onRequestBitmap = GetBitmap;
	callbacks.getFormat = GetFormat;
	callbacks.onGetField = GetField;
	callbacks.onFinalize = Finalize;

	int r = CoronaExternalPushTexture(L, &callbacks, data);	// ..., texture

	return r;
}