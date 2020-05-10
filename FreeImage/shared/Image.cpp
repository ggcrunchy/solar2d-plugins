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
#include "enums.h"

// ==========================================================
// fipImage class implementation
//
// Design and implementation by
// - Hervé Drolon (drolon@infonie.fr)
//
// This file is part of FreeImage 3
//
// COVERED CODE IS PROVIDED UNDER THIS LICENSE ON AN "AS IS" BASIS, WITHOUT WARRANTY
// OF ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING, WITHOUT LIMITATION, WARRANTIES
// THAT THE COVERED CODE IS FREE OF DEFECTS, MERCHANTABLE, FIT FOR A PARTICULAR PURPOSE
// OR NON-INFRINGING. THE ENTIRE RISK AS TO THE QUALITY AND PERFORMANCE OF THE COVERED
// CODE IS WITH YOU. SHOULD ANY COVERED CODE PROVE DEFECTIVE IN ANY RESPECT, YOU (NOT
// THE INITIAL DEVELOPER OR ANY OTHER CONTRIBUTOR) ASSUME THE COST OF ANY NECESSARY
// SERVICING, REPAIR OR CORRECTION. THIS DISCLAIMER OF WARRANTY CONSTITUTES AN ESSENTIAL
// PART OF THIS LICENSE. NO USE OF ANY COVERED CODE IS AUTHORIZED HEREUNDER EXCEPT UNDER
// THIS DISCLAIMER.
//
// Use at your own risk!
// ==========================================================

///////////////////////////////////////////////////////////////////   
// Protected functions

BOOL fipImage::replace(FIBITMAP *new_dib) {
	if(new_dib == NULL) 
		return FALSE;
	if(_dib)
		FreeImage_Unload(_dib);
	_dib = new_dib;
	_bHasChanged = TRUE;
	return TRUE;
}

///////////////////////////////////////////////////////////////////
// Creation & Destruction

fipImage::fipImage(FREE_IMAGE_TYPE image_type, unsigned width, unsigned height, unsigned bpp) {
	_dib = NULL;
	_bHasChanged = FALSE;
	if(width && height && bpp)
		setSize(image_type, width, height, bpp);
}

fipImage::~fipImage() {
	if(_dib) {
		FreeImage_Unload(_dib);
		_dib = NULL;
	}
}

BOOL fipImage::setSize(FREE_IMAGE_TYPE image_type, unsigned width, unsigned height, unsigned bpp, unsigned red_mask, unsigned green_mask, unsigned blue_mask) {
	if(_dib) {
		FreeImage_Unload(_dib);
	}
	if((_dib = FreeImage_AllocateT(image_type, width, height, bpp, red_mask, green_mask, blue_mask)) == NULL)
		return FALSE;

	if(image_type == FIT_BITMAP) {
		// Create palette if needed
		switch(bpp)	{
			case 1:
			case 4:
			case 8:
				RGBQUAD *pal = FreeImage_GetPalette(_dib);
				for(unsigned i = 0; i < FreeImage_GetColorsUsed(_dib); i++) {
					pal[i].rgbRed = i;
					pal[i].rgbGreen = i;
					pal[i].rgbBlue = i;
				}
				break;
		}
	}

	_bHasChanged = TRUE;

	return TRUE;
}

void fipImage::clear() {
	if(_dib) {
		FreeImage_Unload(_dib);
		_dib = NULL;
	}
	_bHasChanged = TRUE;
}

///////////////////////////////////////////////////////////////////
// Copying

fipImage::fipImage(const fipImage& Image) {
	_dib = NULL;
	_fif = FIF_UNKNOWN;
	FIBITMAP *clone = FreeImage_Clone((FIBITMAP*)Image._dib);
	replace(clone);
}

fipImage& fipImage::operator=(const fipImage& Image) {
	if(this != &Image) {
		FIBITMAP *clone = FreeImage_Clone((FIBITMAP*)Image._dib);
		replace(clone);
	}
	return *this;
}

fipImage& fipImage::operator=(FIBITMAP *dib) {
	if(_dib != dib) {
		replace(dib);
	}
	return *this;
}

BOOL fipImage::copySubImage(fipImage& dst, int left, int top, int right, int bottom) const {
	if(_dib) {
		dst = FreeImage_Copy(_dib, left, top, right, bottom);
		return dst.isValid();
	}
	return FALSE;
}

BOOL fipImage::pasteSubImage(fipImage& src, int left, int top, int alpha) {
	if(_dib) {
		BOOL bResult = FreeImage_Paste(_dib, src._dib, left, top, alpha);
		_bHasChanged = TRUE;
		return bResult;
	}
	return FALSE;
}

BOOL fipImage::crop(int left, int top, int right, int bottom) {
	if(_dib) {
		FIBITMAP *dst = FreeImage_Copy(_dib, left, top, right, bottom);
		return replace(dst);
	}
	return FALSE;
}


///////////////////////////////////////////////////////////////////
// Information functions

FREE_IMAGE_TYPE fipImage::getImageType() const {
	return FreeImage_GetImageType(_dib);
}

unsigned fipImage::getWidth() const {
	return FreeImage_GetWidth(_dib); 
}

unsigned fipImage::getHeight() const {
	return FreeImage_GetHeight(_dib); 
}

unsigned fipImage::getScanWidth() const {
	return FreeImage_GetPitch(_dib);
}

BOOL fipImage::isValid() const {
	return (_dib != NULL) ? TRUE:FALSE;
}

BITMAPINFO* fipImage::getInfo() const {
	return FreeImage_GetInfo(_dib);
}

BITMAPINFOHEADER* fipImage::getInfoHeader() const {
	return FreeImage_GetInfoHeader(_dib);
}

unsigned fipImage::getImageSize() const {
	return FreeImage_GetDIBSize(_dib);
}

unsigned fipImage::getImageMemorySize() const {
	return FreeImage_GetMemorySize(_dib);
}

unsigned fipImage::getBitsPerPixel() const {
	return FreeImage_GetBPP(_dib);
}

unsigned fipImage::getLine() const {
	return FreeImage_GetLine(_dib);
}

double fipImage::getHorizontalResolution() const {
	return (FreeImage_GetDotsPerMeterX(_dib) / (double)100); 
}

double fipImage::getVerticalResolution() const {
	return (FreeImage_GetDotsPerMeterY(_dib) / (double)100);
}

void fipImage::setHorizontalResolution(double value) {
	FreeImage_SetDotsPerMeterX(_dib, (unsigned)(value * 100 + 0.5));
}

void fipImage::setVerticalResolution(double value) {
	FreeImage_SetDotsPerMeterY(_dib, (unsigned)(value * 100 + 0.5));
}


///////////////////////////////////////////////////////////////////
// Palette operations

RGBQUAD* fipImage::getPalette() const {
	return FreeImage_GetPalette(_dib);
}

unsigned fipImage::getPaletteSize() const {
	return FreeImage_GetColorsUsed(_dib) * sizeof(RGBQUAD);
}

unsigned fipImage::getColorsUsed() const {
	return FreeImage_GetColorsUsed(_dib);
}

FREE_IMAGE_COLOR_TYPE fipImage::getColorType() const { 
	return FreeImage_GetColorType(_dib);
}

BOOL fipImage::isGrayscale() const {
	return ((FreeImage_GetBPP(_dib) == 8) && (FreeImage_GetColorType(_dib) != FIC_PALETTE)); 
}

///////////////////////////////////////////////////////////////////
// Thumbnail access

BOOL fipImage::getThumbnail(fipImage& image) const {
	image = FreeImage_Clone( FreeImage_GetThumbnail(_dib) );
	return image.isValid();
}

BOOL fipImage::setThumbnail(const fipImage& image) {
	return FreeImage_SetThumbnail(_dib, (FIBITMAP*)image._dib);
}

BOOL fipImage::hasThumbnail() const {
	return (FreeImage_GetThumbnail(_dib) != NULL);
}

BOOL fipImage::clearThumbnail() {
	return FreeImage_SetThumbnail(_dib, NULL);
}


///////////////////////////////////////////////////////////////////
// Pixel access

BYTE* fipImage::accessPixels() const {
	return FreeImage_GetBits(_dib); 
}

BYTE* fipImage::getScanLine(unsigned scanline) const {
	if(scanline < FreeImage_GetHeight(_dib)) {
		return FreeImage_GetScanLine(_dib, scanline);
	}
	return NULL;
}

BOOL fipImage::getPixelIndex(unsigned x, unsigned y, BYTE *value) const {
	return FreeImage_GetPixelIndex(_dib, x, y, value);
}

BOOL fipImage::getPixelColor(unsigned x, unsigned y, RGBQUAD *value) const {
	return FreeImage_GetPixelColor(_dib, x, y, value);
}

BOOL fipImage::setPixelIndex(unsigned x, unsigned y, BYTE *value) {
	_bHasChanged = TRUE;
	return FreeImage_SetPixelIndex(_dib, x, y, value);
}

BOOL fipImage::setPixelColor(unsigned x, unsigned y, RGBQUAD *value) {
	_bHasChanged = TRUE;
	return FreeImage_SetPixelColor(_dib, x, y, value);
}

///////////////////////////////////////////////////////////////////
// File type identification

FREE_IMAGE_FORMAT fipImage::identifyFIF(const char* lpszPathName) {
	FREE_IMAGE_FORMAT fif = FIF_UNKNOWN;

	// check the file signature and get its format
	// (the second argument is currently not used by FreeImage)
	fif = FreeImage_GetFileType(lpszPathName, 0);
	if(fif == FIF_UNKNOWN) {
		// no signature ?
		// try to guess the file format from the file extension
		fif = FreeImage_GetFIFFromFilename(lpszPathName);
	}

	return fif;
}

FREE_IMAGE_FORMAT fipImage::identifyFIFU(const wchar_t* lpszPathName) {
	FREE_IMAGE_FORMAT fif = FIF_UNKNOWN;

	// check the file signature and get its format
	// (the second argument is currently not used by FreeImage)
	fif = FreeImage_GetFileTypeU(lpszPathName, 0);
	if(fif == FIF_UNKNOWN) {
		// no signature ?
		// try to guess the file format from the file extension
		fif = FreeImage_GetFIFFromFilenameU(lpszPathName);
	}

	return fif;
}

FREE_IMAGE_FORMAT fipImage::identifyFIFFromHandle(FreeImageIO *io, fi_handle handle) {
	if(io && handle) {
		// check the file signature and get its format
		return FreeImage_GetFileTypeFromHandle(io, handle, 16);
	}
	return FIF_UNKNOWN;
}

FREE_IMAGE_FORMAT fipImage::identifyFIFFromMemory(FIMEMORY *hmem) {
	if(hmem != NULL) {
		return FreeImage_GetFileTypeFromMemory(hmem, 0);
	}
	return FIF_UNKNOWN;
}


///////////////////////////////////////////////////////////////////
// Loading & Saving

BOOL fipImage::load(const char* lpszPathName, int flag) {
	FREE_IMAGE_FORMAT fif = FIF_UNKNOWN;

	// check the file signature and get its format
	// (the second argument is currently not used by FreeImage)
	fif = FreeImage_GetFileType(lpszPathName, 0);
	if(fif == FIF_UNKNOWN) {
		// no signature ?
		// try to guess the file format from the file extension
		fif = FreeImage_GetFIFFromFilename(lpszPathName);
	}
	// check that the plugin has reading capabilities ...
	if((fif != FIF_UNKNOWN) && FreeImage_FIFSupportsReading(fif)) {
		// Free the previous dib
		if(_dib) {
			FreeImage_Unload(_dib);			
		}
		// Load the file
		_dib = FreeImage_Load(fif, lpszPathName, flag);
		_bHasChanged = TRUE;
		if(_dib == NULL)
			return FALSE;
		return TRUE;
	}
	return FALSE;
}

BOOL fipImage::loadU(const wchar_t* lpszPathName, int flag) {
	FREE_IMAGE_FORMAT fif = FIF_UNKNOWN;

	// check the file signature and get its format
	// (the second argument is currently not used by FreeImage)
	fif = FreeImage_GetFileTypeU(lpszPathName, 0);
	if(fif == FIF_UNKNOWN) {
		// no signature ?
		// try to guess the file format from the file extension
		fif = FreeImage_GetFIFFromFilenameU(lpszPathName);
	}
	// check that the plugin has reading capabilities ...
	if((fif != FIF_UNKNOWN) && FreeImage_FIFSupportsReading(fif)) {
		// Free the previous dib
		if(_dib) {
			FreeImage_Unload(_dib);			
		}
		// Load the file
		_dib = FreeImage_LoadU(fif, lpszPathName, flag);
		_bHasChanged = TRUE;
		if(_dib == NULL)
			return FALSE;
		return TRUE;
	}
	return FALSE;
}

BOOL fipImage::loadFromHandle(FreeImageIO *io, fi_handle handle, int flag) {
	FREE_IMAGE_FORMAT fif = FIF_UNKNOWN;

	// check the file signature and get its format
	fif = FreeImage_GetFileTypeFromHandle(io, handle, 16);
	if((fif != FIF_UNKNOWN) && FreeImage_FIFSupportsReading(fif)) {
		// Free the previous dib
		if(_dib) {
			FreeImage_Unload(_dib);			
		}
		// Load the file
		_dib = FreeImage_LoadFromHandle(fif, io, handle, flag);
		_bHasChanged = TRUE;
		if(_dib == NULL)
			return FALSE;
		return TRUE;
	}
	return FALSE;
}

BOOL fipImage::loadFromMemory(fipMemoryIO& memIO, int flag) {
	FREE_IMAGE_FORMAT fif = FIF_UNKNOWN;

	// check the file signature and get its format
	fif = memIO.getFileType();
	if((fif != FIF_UNKNOWN) && FreeImage_FIFSupportsReading(fif)) {
		// Free the previous dib
		if(_dib) {
			FreeImage_Unload(_dib);			
		}
		// Load the file
		_dib = memIO.load(fif, flag);
		_bHasChanged = TRUE;
		if(_dib == NULL)
			return FALSE;
		return TRUE;
	}
	return FALSE;
}

BOOL fipImage::save(const char* lpszPathName, int flag) const {
	FREE_IMAGE_FORMAT fif = FIF_UNKNOWN;
	BOOL bSuccess = FALSE;

	// Try to guess the file format from the file extension
	fif = FreeImage_GetFIFFromFilename(lpszPathName);
	if(fif != FIF_UNKNOWN ) {
		// Check that the dib can be saved in this format
		BOOL bCanSave;

		FREE_IMAGE_TYPE image_type = FreeImage_GetImageType(_dib);
		if(image_type == FIT_BITMAP) {
			// standard bitmap type
			WORD bpp = FreeImage_GetBPP(_dib);
			bCanSave = (FreeImage_FIFSupportsWriting(fif) && FreeImage_FIFSupportsExportBPP(fif, bpp));
		} else {
			// special bitmap type
			bCanSave = FreeImage_FIFSupportsExportType(fif, image_type);
		}

		if(bCanSave) {
			bSuccess = FreeImage_Save(fif, _dib, lpszPathName, flag);
			return bSuccess;
		}
	}
	return bSuccess;
}

BOOL fipImage::saveU(const wchar_t* lpszPathName, int flag) const {
	FREE_IMAGE_FORMAT fif = FIF_UNKNOWN;
	BOOL bSuccess = FALSE;

	// Try to guess the file format from the file extension
	fif = FreeImage_GetFIFFromFilenameU(lpszPathName);
	if(fif != FIF_UNKNOWN ) {
		// Check that the dib can be saved in this format
		BOOL bCanSave;

		FREE_IMAGE_TYPE image_type = FreeImage_GetImageType(_dib);
		if(image_type == FIT_BITMAP) {
			// standard bitmap type
			WORD bpp = FreeImage_GetBPP(_dib);
			bCanSave = (FreeImage_FIFSupportsWriting(fif) && FreeImage_FIFSupportsExportBPP(fif, bpp));
		} else {
			// special bitmap type
			bCanSave = FreeImage_FIFSupportsExportType(fif, image_type);
		}

		if(bCanSave) {
			bSuccess = FreeImage_SaveU(fif, _dib, lpszPathName, flag);
			return bSuccess;
		}
	}
	return bSuccess;
}

BOOL fipImage::saveToHandle(FREE_IMAGE_FORMAT fif, FreeImageIO *io, fi_handle handle, int flag) const {
	BOOL bSuccess = FALSE;

	if(fif != FIF_UNKNOWN ) {
		// Check that the dib can be saved in this format
		BOOL bCanSave;

		FREE_IMAGE_TYPE image_type = FreeImage_GetImageType(_dib);
		if(image_type == FIT_BITMAP) {
			// standard bitmap type
			WORD bpp = FreeImage_GetBPP(_dib);
			bCanSave = (FreeImage_FIFSupportsWriting(fif) && FreeImage_FIFSupportsExportBPP(fif, bpp));
		} else {
			// special bitmap type
			bCanSave = FreeImage_FIFSupportsExportType(fif, image_type);
		}

		if(bCanSave) {
			bSuccess = FreeImage_SaveToHandle(fif, _dib, io, handle, flag);
			return bSuccess;
		}
	}
	return bSuccess;
}

BOOL fipImage::saveToMemory(FREE_IMAGE_FORMAT fif, fipMemoryIO& memIO, int flag) const {
	BOOL bSuccess = FALSE;

	if(fif != FIF_UNKNOWN ) {
		// Check that the dib can be saved in this format
		BOOL bCanSave;

		FREE_IMAGE_TYPE image_type = FreeImage_GetImageType(_dib);
		if(image_type == FIT_BITMAP) {
			// standard bitmap type
			WORD bpp = FreeImage_GetBPP(_dib);
			bCanSave = (FreeImage_FIFSupportsWriting(fif) && FreeImage_FIFSupportsExportBPP(fif, bpp));
		} else {
			// special bitmap type
			bCanSave = FreeImage_FIFSupportsExportType(fif, image_type);
		}

		if(bCanSave) {
			bSuccess = memIO.save(fif, _dib, flag);
			return bSuccess;
		}
	}
	return bSuccess;
}

///////////////////////////////////////////////////////////////////   
// Conversion routines

BOOL fipImage::convertToType(FREE_IMAGE_TYPE image_type, BOOL scale_linear) {
	if(_dib) {
		FIBITMAP *dib = FreeImage_ConvertToType(_dib, image_type, scale_linear);
		return replace(dib);
	}
	return FALSE;
}

BOOL fipImage::threshold(BYTE T) {
	if(_dib) {
		FIBITMAP *dib1 = FreeImage_Threshold(_dib, T);
		return replace(dib1);
	}
	return FALSE;
}

BOOL fipImage::convertTo4Bits() {
	if(_dib) {
		FIBITMAP *dib4 = FreeImage_ConvertTo4Bits(_dib);
		return replace(dib4);
	}
	return FALSE;
}

BOOL fipImage::convertTo8Bits() {
	if(_dib) {
		FIBITMAP *dib8 = FreeImage_ConvertTo8Bits(_dib);
		return replace(dib8);
	}
	return FALSE;
}

BOOL fipImage::convertTo16Bits555() {
	if(_dib) {
		FIBITMAP *dib16_555 = FreeImage_ConvertTo16Bits555(_dib);
		return replace(dib16_555);
	}
	return FALSE;
}

BOOL fipImage::convertTo16Bits565() {
	if(_dib) {
		FIBITMAP *dib16_565 = FreeImage_ConvertTo16Bits565(_dib);
		return replace(dib16_565);
	}
	return FALSE;
}

BOOL fipImage::convertTo24Bits() {
	if(_dib) {
		FIBITMAP *dibRGB = FreeImage_ConvertTo24Bits(_dib);
		return replace(dibRGB);
	}
	return FALSE;
}

BOOL fipImage::convertTo32Bits() {
	if(_dib) {
		FIBITMAP *dib32 = FreeImage_ConvertTo32Bits(_dib);
		return replace(dib32);
	}
	return FALSE;
}

BOOL fipImage::convertToGrayscale() {
	if(_dib) {
		FIBITMAP *dib8 = FreeImage_ConvertToGreyscale(_dib);
		return replace(dib8);
	}
	return FALSE;
}

BOOL fipImage::colorQuantize(FREE_IMAGE_QUANTIZE algorithm) {
	if(_dib) {
		FIBITMAP *dib8 = FreeImage_ColorQuantize(_dib, algorithm);
		return replace(dib8);
	}
	return FALSE;
}

BOOL fipImage::dither(FREE_IMAGE_DITHER algorithm) {
	if(_dib) {
		FIBITMAP *dib = FreeImage_Dither(_dib, algorithm);
		return replace(dib);
	}
	return FALSE;
}

BOOL fipImage::convertToFloat() {
	if(_dib) {
		FIBITMAP *dib = FreeImage_ConvertToFloat(_dib);
		return replace(dib);
	}
	return FALSE;
}

BOOL fipImage::convertToRGBF() {
	if(_dib) {
		FIBITMAP *dib = FreeImage_ConvertToRGBF(_dib);
		return replace(dib);
	}
	return FALSE;
}

BOOL fipImage::convertToRGBAF() {
	if(_dib) {
		FIBITMAP *dib = FreeImage_ConvertToRGBAF(_dib);
		return replace(dib);
	}
	return FALSE;
}

BOOL fipImage::convertToUINT16() {
	if(_dib) {
		FIBITMAP *dib = FreeImage_ConvertToUINT16(_dib);
		return replace(dib);
	}
	return FALSE;
}

BOOL fipImage::convertToRGB16() {
	if(_dib) {
		FIBITMAP *dib = FreeImage_ConvertToRGB16(_dib);
		return replace(dib);
	}
	return FALSE;
}

BOOL fipImage::convertToRGBA16() {
	if(_dib) {
		FIBITMAP *dib = FreeImage_ConvertToRGBA16(_dib);
		return replace(dib);
	}
	return FALSE;
}

BOOL fipImage::toneMapping(FREE_IMAGE_TMO tmo, double first_param, double second_param, double third_param, double fourth_param) {
	if(_dib) {
		FIBITMAP *dst = NULL;
		// Apply a tone mapping algorithm and convert to 24-bit 
		switch(tmo) {
			case FITMO_REINHARD05:
				dst = FreeImage_TmoReinhard05Ex(_dib, first_param, second_param, third_param, fourth_param);
				break;
			default:
				dst = FreeImage_ToneMapping(_dib, tmo, first_param, second_param);
				break;
		}

		return replace(dst);
	}
	return FALSE;
}

///////////////////////////////////////////////////////////////////   
// Transparency support: background colour and alpha channel

BOOL fipImage::isTransparent() const {
	return FreeImage_IsTransparent(_dib);
}

unsigned fipImage::getTransparencyCount() const {
	return FreeImage_GetTransparencyCount(_dib);
}

BYTE* fipImage::getTransparencyTable() const {
	return FreeImage_GetTransparencyTable(_dib);
}

void fipImage::setTransparencyTable(BYTE *table, int count) {
	FreeImage_SetTransparencyTable(_dib, table, count);
	_bHasChanged = TRUE;
}

BOOL fipImage::hasFileBkColor() const {
	return FreeImage_HasBackgroundColor(_dib);
}

BOOL fipImage::getFileBkColor(RGBQUAD *bkcolor) const {
	return FreeImage_GetBackgroundColor(_dib, bkcolor);
}

BOOL fipImage::setFileBkColor(RGBQUAD *bkcolor) {
	_bHasChanged = TRUE;
	return FreeImage_SetBackgroundColor(_dib, bkcolor);
}

///////////////////////////////////////////////////////////////////   
// Channel processing support

BOOL fipImage::getChannel(fipImage& image, FREE_IMAGE_COLOR_CHANNEL channel) const {
	if(_dib) {
		image = FreeImage_GetChannel(_dib, channel);
		return image.isValid();
	}
	return FALSE;
}

BOOL fipImage::setChannel(fipImage& image, FREE_IMAGE_COLOR_CHANNEL channel) {
	if(_dib) {
		_bHasChanged = TRUE;
		return FreeImage_SetChannel(_dib, image._dib, channel);
	}
	return FALSE;
}

BOOL fipImage::splitChannels(fipImage& RedChannel, fipImage& GreenChannel, fipImage& BlueChannel) {
	if(_dib) {
		RedChannel = FreeImage_GetChannel(_dib, FICC_RED);
		GreenChannel = FreeImage_GetChannel(_dib, FICC_GREEN);
		BlueChannel = FreeImage_GetChannel(_dib, FICC_BLUE);

		return (RedChannel.isValid() && GreenChannel.isValid() && BlueChannel.isValid());
	}
	return FALSE;
}

BOOL fipImage::combineChannels(fipImage& red, fipImage& green, fipImage& blue) {
	if(!_dib) {
		int width = red.getWidth();
		int height = red.getHeight();
		_dib = FreeImage_Allocate(width, height, 24, FI_RGBA_RED_MASK, FI_RGBA_GREEN_MASK, FI_RGBA_BLUE_MASK);
	}

	if(_dib) {
		BOOL bResult = TRUE;
		bResult &= FreeImage_SetChannel(_dib, red._dib, FICC_RED);
		bResult &= FreeImage_SetChannel(_dib, green._dib, FICC_GREEN);
		bResult &= FreeImage_SetChannel(_dib, blue._dib, FICC_BLUE);

		_bHasChanged = TRUE;

		return bResult;
	}
	return FALSE;
}

///////////////////////////////////////////////////////////////////   
// Rotation and flipping

BOOL fipImage::rotateEx(double angle, double x_shift, double y_shift, double x_origin, double y_origin, BOOL use_mask) {
	if(_dib) {
		if(FreeImage_GetBPP(_dib) >= 8) {
			FIBITMAP *rotated = FreeImage_RotateEx(_dib, angle, x_shift, y_shift, x_origin, y_origin, use_mask);
			return replace(rotated);
		}
	}
	return FALSE;
}

BOOL fipImage::rotate(double angle, const void *bkcolor) {
	if(_dib) {
		switch(FreeImage_GetImageType(_dib)) {
			case FIT_BITMAP:
				switch(FreeImage_GetBPP(_dib)) {
					case 1:
					case 8:
					case 24:
					case 32:
						break;
					default:
						return FALSE;
				}
				break;

			case FIT_UINT16:
			case FIT_RGB16:
			case FIT_RGBA16:
			case FIT_FLOAT:
			case FIT_RGBF:
			case FIT_RGBAF:
				break;
			default:
				return FALSE;
				break;
		}

		FIBITMAP *rotated = FreeImage_Rotate(_dib, angle, bkcolor);
		return replace(rotated);

	}
	return FALSE;
}

BOOL fipImage::flipVertical() {
	if(_dib) {
		_bHasChanged = TRUE;

		return FreeImage_FlipVertical(_dib);
	}
	return FALSE;
}

BOOL fipImage::flipHorizontal() {
	if(_dib) {
		_bHasChanged = TRUE;

		return FreeImage_FlipHorizontal(_dib);
	}
	return FALSE;
}

///////////////////////////////////////////////////////////////////   
// Color manipulation routines

BOOL fipImage::invert() {
	if(_dib) {
		_bHasChanged = TRUE;

		return FreeImage_Invert(_dib);
	}
	return FALSE;
}

BOOL fipImage::adjustCurve(BYTE *LUT, FREE_IMAGE_COLOR_CHANNEL channel) {
	if(_dib) {
		_bHasChanged = TRUE;

		return FreeImage_AdjustCurve(_dib, LUT, channel);
	}
	return FALSE;
}

BOOL fipImage::adjustGamma(double gamma) {
	if(_dib) {
		_bHasChanged = TRUE;

		return FreeImage_AdjustGamma(_dib, gamma);
	}
	return FALSE;
}

BOOL fipImage::adjustBrightness(double percentage) {
	if(_dib) {
		_bHasChanged = TRUE;

		return FreeImage_AdjustBrightness(_dib, percentage);
	}
	return FALSE;
}

BOOL fipImage::adjustContrast(double percentage) {
	if(_dib) {
		_bHasChanged = TRUE;

		return FreeImage_AdjustContrast(_dib, percentage);
	}
	return FALSE;
}

BOOL fipImage::adjustBrightnessContrastGamma(double brightness, double contrast, double gamma) {
	if(_dib) {
		_bHasChanged = TRUE;

		return FreeImage_AdjustColors(_dib, brightness, contrast, gamma, FALSE);
	}
	return FALSE;
}

BOOL fipImage::getHistogram(DWORD *histo, FREE_IMAGE_COLOR_CHANNEL channel) const {
	if(_dib) {
		return FreeImage_GetHistogram(_dib, histo, channel);
	}
	return FALSE;
}

///////////////////////////////////////////////////////////////////
// Upsampling / downsampling routine

BOOL fipImage::rescale(unsigned new_width, unsigned new_height, FREE_IMAGE_FILTER filter) {
	if(_dib) {
		switch(FreeImage_GetImageType(_dib)) {
			case FIT_BITMAP:
			case FIT_UINT16:
			case FIT_RGB16:
			case FIT_RGBA16:
			case FIT_FLOAT:
			case FIT_RGBF:
			case FIT_RGBAF:
				break;
			default:
				return FALSE;
				break;
		}

		// Perform upsampling / downsampling
		FIBITMAP *dst = FreeImage_Rescale(_dib, new_width, new_height, filter);
		return replace(dst);
	}
	return FALSE;
}

BOOL fipImage::makeThumbnail(unsigned max_size, BOOL convert) {
	if(_dib) {
		switch(FreeImage_GetImageType(_dib)) {
			case FIT_BITMAP:
			case FIT_UINT16:
			case FIT_RGB16:
			case FIT_RGBA16:
			case FIT_FLOAT:
			case FIT_RGBF:
			case FIT_RGBAF:
				break;
			default:
				return FALSE;
				break;
		}

		// Perform downsampling
		FIBITMAP *dst = FreeImage_MakeThumbnail(_dib, max_size, convert);
		return replace(dst);
	}
	return FALSE;
}

///////////////////////////////////////////////////////////////////
// Metadata

unsigned fipImage::getMetadataCount(FREE_IMAGE_MDMODEL model) const {
	return FreeImage_GetMetadataCount(model, _dib);
}

BOOL fipImage::getMetadata(FREE_IMAGE_MDMODEL model, const char *key, fipTag& tag) const {
	FITAG *searchedTag = NULL;
	FreeImage_GetMetadata(model, _dib, key, &searchedTag);
	if(searchedTag != NULL) {
		tag = FreeImage_CloneTag(searchedTag);
		return TRUE;
	} else {
		// clear the tag
		tag = (FITAG*)NULL;
	}
	return FALSE;
}

BOOL fipImage::setMetadata(FREE_IMAGE_MDMODEL model, const char *key, fipTag& tag) {
	return FreeImage_SetMetadata(model, _dib, key, tag);
}

#define FI_FUNC (Image(L, 1).*func)

template<BOOL (fipImage::*func)(void)> int ImageBool (lua_State * L)
{
	lua_pushboolean(L, FI_FUNC());	// image, ok

	return 1;
}

#define FI_BOOL(name) #name, ImageBool<&fipImage::name>

template<BOOL (fipImage::*func)(void) const> int ImageBoolConst (lua_State * L)
{
	lua_pushboolean(L, FI_FUNC());	// image, ok

	return 1;
}

#define FI_BOOLK(name) #name, ImageBoolConst<&fipImage::name>

template<BOOL (fipImage::*func)(double)> int ImageBool_Double (lua_State * L)
{
	lua_pushboolean(L, FI_FUNC(lua_tonumber(L, 2)));	// image, arg, ok

	return 1;
}

#define FI_BOOL_D(name) #name, ImageBool_Double<&fipImage::name>

template<unsigned (fipImage::*func)() const> int ImageUnsignedConst (lua_State * L)
{
	lua_pushinteger(L, FI_FUNC());	// image, arg, ok

	return 1;
}

#define FI_UK(name) #name, ImageUnsignedConst<&fipImage::name>

// Forward declaration
extern luaL_Reg image_methods[];
extern luaL_Reg image_gc[];

static fipImage * PushImage (lua_State * L, bool init = true)
{
	fipImage * image = New<fipImage>(L, "fipImage", image_methods, image_gc); // image

	if (init) new (image) fipImage;

	return image;
}

template<BOOL (fipImage::*func)(fipImage &, fipImage &, fipImage &)> int Images3 (lua_State * L)
{
	fipImage * i1 = PushImage(L);	// image, i1
	fipImage * i2 = PushImage(L);	// image, i1, i2
	fipImage * i3 = PushImage(L);	// image, i1, i2, i3

	lua_pushboolean(L, FI_FUNC(*i1, *i2, *i3));	// image, i1, i2, i3, ok

	if (!lua_toboolean(L, 1)) return 1;

	lua_insert(L, -4);	// image, ok, i1, i2, i3

	return 4;
}

#define FI_IMAGES3(name) #name, Images3<&fipImage::name>

#undef FI_FUNC

static double D (lua_State * L, int index)
{
	return lua_tonumber(L, index);
}

static int I (lua_State * L, int index)
{
	return lua_tointeger(L, index);
}

static int I0 (lua_State * L, int index)
{
	return luaL_optint(L, index, 0);
}

static const char * S (lua_State * L, int index)
{
	return lua_tostring(L, index);
}

static unsigned U (lua_State * L, int index)
{
	return lua_tointeger(L, index);
}

static unsigned U0 (lua_State * L, int index)
{
	return luaL_optinteger(L, index, 0U);
}

static void RGBQuad (lua_State * L, int index, RGBQUAD * quad)
{
	lua_getfield(L, index, "rgbRed");	// ..., red
	lua_getfield(L, index, "rgbGreen");	// ..., red, green
	lua_getfield(L, index, "rgbBlue");	// ..., red, green, blue

	quad->rgbBlue = lua_tointeger(L, -1);
	quad->rgbGreen = lua_tointeger(L, -2);
	quad->rgbRed = lua_tointeger(L, -3);
	quad->rgbReserved = 0;

	lua_pop(L, 3);	// ...
}

static void NewRGBQuad (lua_State * L, RGBQUAD & quad)
{
	lua_createtable(L, 0, 3);	// ..., t
	lua_pushinteger(L, quad.rgbRed);// ..., t, red
	lua_pushinteger(L, quad.rgbGreen);	// ..., t, red, green
	lua_pushinteger(L, quad.rgbBlue);	// ..., t, red, green, blue
	lua_setfield(L, -4, "rgbBlue");	// ..., t = { rgbBlue = blue }, red, green
	lua_setfield(L, -3, "rgbGreen");	// ..., t = { rgbGreen = green, rgbBlue = blue }, red
	lua_setfield(L, -2, "rgbRed");	// ..., t = { rgbRed = red, rgbGreen = green, rgbBlue = blue }
}

static int ReturnBoolVal (lua_State * L)
{
	if (!lua_toboolean(L, -1)) return 1;

	lua_insert(L, -2);	// ..., ok, value

	return 2;
}

luaL_Reg image_methods[] = {
	{
		"accessPixels", [](lua_State * L)
		{
//			BYTE * Image(L, 1).accessPixels()

			return 0;
		}
	}, {
		"adjustBrightnessContrastGamma", [](lua_State * L)
		{
			lua_pushboolean(L, Image(L, 1).adjustBrightnessContrastGamma(D(L, 2), D(L, 3), D(L, 4)));	// image, brightness, constrast, gamma, ok

			return 1;
		}
	}, {
		FI_BOOL_D(adjustBrightness)
	}, {
		FI_BOOL_D(adjustContrast)
	}, {
		"adjustCurve", [](lua_State * L)
		{
//			BOOL Image(L, 1).adjustCurve(BYTE *, FREE_IMAGE_COLOR_CHANNGEL)

			return 0;
		}
	}, {
		FI_BOOL_D(adjustGamma)
	}, {
		"assign", [](lua_State * L)
		{
			if (IsType(L, "fipImage", "fipWinImage", 2)) Image(L, 1) = Image(L, 2);

			else Image(L, 1) = (FIBITMAP *)lua_touserdata(L, 2);
			// TODO: More robust?
			return 0; // return arg 1?
		}
	}, {
		"clear", [](lua_State * L)
		{
			Image(L, 1).clear();

			return 0;
		}
	}, {
		FI_BOOL(clearThumbnail)
	}, {
		"clone", [](lua_State * L)
		{
			lua_settop(L, 1);	// image

			fipImage * new_image = PushImage(L, false);	// image, new_image

			new (new_image) fipImage(Image(L, 1));

			return 1;
		}
	}, {
		"colorQuantize", [](lua_State * L)
		{
			lua_pushboolean(L, Image(L, 1).colorQuantize((FREE_IMAGE_QUANTIZE)GetCode(L, kQuantize, 2)));	// image, quantize, ok
			
			return 1;
		}
	}, {
		FI_IMAGES3(combineChannels)
	}, {
		FI_BOOL(convertTo4Bits)
	}, {
		FI_BOOL(convertTo8Bits)
	}, {
		FI_BOOL(convertTo16Bits555)
	}, {
		FI_BOOL(convertTo16Bits565)
	}, {
		FI_BOOL(convertTo24Bits)
	}, {
		FI_BOOL(convertTo32Bits)
	}, {
		FI_BOOL(convertToFloat)
	}, {
		FI_BOOL(convertToGrayscale)
	}, {
		FI_BOOL(convertToRGB16)
	}, {
		FI_BOOL(convertToRGBA16)
	}, {
		FI_BOOL(convertToRGBAF)
	}, {
		FI_BOOL(convertToRGBF)
	}, {
		"convertToType", [](lua_State * L)
		{
			BOOL linear = !lua_isnoneornil(L, 3) ? lua_toboolean(L, 3) : TRUE;

			lua_pushboolean(L, Image(L, 1).convertToType((FREE_IMAGE_TYPE)GetCode(L, kType, 2), linear));	// image[, linear], ok

			return 1;
		}
	}, {
		FI_BOOL(convertToUINT16)
	}, {
		"copySubImage", [](lua_State * L)
		{
			fipImage * dst = PushImage(L);	// image, left, top, right, bottom, dst

			lua_pushboolean(L, Image(L, 1).copySubImage(*dst, I(L, 2), I(L, 3), I(L, 4), I(L, 5)));	// image, left, top, right, bottom, dst, ok

			return ReturnBoolVal(L);
		}
	}, {
		"crop", [](lua_State * L)
		{
			lua_pushboolean(L, Image(L, 1).crop(I(L, 2), I(L, 3), I(L, 4), I(L, 5)));	// image, left, top, right, bottom, ok

			return 1;
		}
	}, {
		"dither", [](lua_State * L)
		{
			lua_pushboolean(L, Image(L, 1).dither((FREE_IMAGE_DITHER)GetCode(L, kDither, 2)));	// image, dither, ok

			return 1;
		}
	}, {
		FI_BOOL(flipHorizontal)
	}, {
		FI_BOOL(flipVertical)
	}, {
		FI_UK(getBitsPerPixel)
	}, {
		"getChannel", [](lua_State * L)
		{
			fipImage * dst = PushImage(L);	// image, channel, dst

			lua_pushboolean(L, Image(L, 1).getChannel(*dst, (FREE_IMAGE_COLOR_CHANNEL)GetCode(L, kColorChannel, 2)));	// image, channel, dst, ok

			return ReturnBoolVal(L);
		}
	}, {
		FI_UK(getColorsUsed)
	}, {
		"getColorType", [](lua_State * L)
		{
			lua_pushinteger(L, Image(L, 1).getColorType());	// image, color_type

			GetName(L, kColorType, -1);

			return 1;
		}
	}, {
		"getFileBkColor", [](lua_State * L)
		{
			lua_settop(L, 1);	// image

			RGBQUAD quad = { 0 };

			lua_pushboolean(L, Image(L, 1).getFileBkColor(&quad));	// image, ok

			if (lua_toboolean(L, 2)) NewRGBQuad(L, quad);	// image, ok[, t]

			return lua_gettop(L) - 1;
		}
	}, {
		FI_UK(getHeight)
	}, {
		"getHistogram", [](lua_State * L)
		{
//			BOOL Image(L, 1).getHistogram(DWORD *, FREE_IMAGE_COLOR_CHANNEL = FICC_BLACK)

			return 0;
		}
	}, {
		"getHorizontalResolution", [](lua_State * L)
		{
			lua_pushnumber(L, Image(L, 1).getHorizontalResolution());	// image, res

			return 1;
		}
	}, {
		FI_UK(getImageMemorySize)
	}, {
		FI_UK(getImageSize)
	}, {
		"getImageType", [](lua_State * L)
		{
			lua_pushinteger(L, Image(L, 1).getImageType());	// image, type

			GetCode(L, kType, 2);

			return 1;
		}
	}, {
		"getInfo", [](lua_State * L)
		{
//			BITMAPINFO * Image(L, 1).getInfo()

			return 0;
		}
	}, {
		"getInfoHeader", [](lua_State * L)
		{
//			BITMAPINFOHEADER *Image(L, 1).getInfoHeader()

			return 0;
		}
	}, {
		FI_UK(getLine)
	}, {
		"getMetadata", [](lua_State * L)
		{
			fipTag & tag = NewTag(L);

			FREE_IMAGE_MDMODEL model = (FREE_IMAGE_MDMODEL)GetCode(L, kMdModel, 2);

			lua_pushboolean(L, Image(L, 1).getMetadata(model, S(L, 3), tag));	// image, model, key, tag, ok

			return ReturnBoolVal(L);
		}
	}, {
		"getMetadataCount", [](lua_State * L)
		{
			lua_pushinteger(L, Image(L, 1).getMetadataCount((FREE_IMAGE_MDMODEL)GetCode(L, kMdModel, 2)));	// image, model, count

			return 1;
		}
	}, {
		"getPalette", [](lua_State * L)
		{
			fipImage & image = Image(L, 1);

			lua_createtable(L, image.getPaletteSize(), 0);	// image, palette

			for (unsigned i = 0; i < image.getPaletteSize(); ++i)
			{
				NewRGBQuad(L, image.getPalette()[i]);	// image, palette, quad

				lua_rawseti(L, -2, i + 1);	// image, palette = { ..., quad }
			}

			return 1;
		}
	}, {
		FI_UK(getPaletteSize)
	}, {
		"getPixelColor", [](lua_State * L)
		{
			lua_settop(L, 3);	// image, x, y

			RGBQUAD quad = { 0 };

			lua_pushboolean(L, Image(L, 1).getPixelColor(U(L, 2), U(L, 3), &quad));	// image, x, y, ok

			if (lua_toboolean(L, 4)) NewRGBQuad(L, quad);	// image, x, y, ok, quad

			return lua_gettop(L) - 3;
		}
	}, {
		"getPixelIndex", [](lua_State * L)
		{
			lua_settop(L, 3);	// image, x, y

			BYTE value;

			lua_pushboolean(L, Image(L, 1).getPixelIndex(U(L, 2), U(L, 3), &value));// image, x, y, ok

			if (lua_toboolean(L, 4)) lua_pushinteger(L, value);	// image, x, y, ok, value

			return lua_gettop(L) - 3;
		}
	}, {
		FI_UK(getScanWidth)
	}, {
		"getThumbnail", [](lua_State * L)
		{
			fipImage * dst = PushImage(L);	// image, dst

			lua_pushboolean(L, Image(L, 1).getThumbnail(*dst));	// image, dst, ok

			return ReturnBoolVal(L);
		}
	}, {
		FI_UK(getTransparencyCount)
	}, {
		"getTransparencyTable", [](lua_State * L)
		{
//			BYTE *Image(L, 1).getTransparencyTable()

			return 0;
		}
	}, {
		"getVerticalResolution", [](lua_State * L)
		{
			lua_pushnumber(L, Image(L, 1).getVerticalResolution());	// image, res

			return 1;
		}
	}, {
		FI_UK(getWidth)
	}, {
		FI_BOOLK(hasFileBkColor)
	}, {
		FI_BOOLK(hasThumbnail)
	}, {
		FI_BOOL(invert)
	}, {
		FI_BOOLK(isGrayscale)
	}, {
		FI_BOOLK(isTransparent)
	}, {
		FI_BOOLK(isValid)
	}, {
		"load", [](lua_State * L)
		{
			const char * name = S(L, 2);

			lua_pushboolean(L, Image(L, 1).load(name, GetLoadFlags(L, fipImage::identifyFIF(name), 3)));// image, name, flag, ok

			return 1;
		}
	}, {
		"loadFromMemory", [](lua_State * L)
		{
			fipMemoryIO & mio = MemoryIO(L, 2);

			lua_pushboolean(L, Image(L, 1).loadFromMemory(mio, GetLoadFlags(L, fipImage::identifyFIFFromMemory(mio), 3)));	// image, mio, flags, ok

			return 1;
		}
	}, {
		"makeThumbnail", [](lua_State * L)
		{
			lua_pushboolean(L, Image(L, 1).makeThumbnail(U(L, 2), lua_isnoneornil(L, 3) || lua_toboolean(L, 3)));	// image, size, convert, ok

			return 1;
		}
	}, {
		"newTexture", [](lua_State * L)
		{
			return NewTexture(L, Image(L, 1));
		}
	}, {
		"pasteSubImage", [](lua_State * L)
		{
			lua_pushboolean(L, Image(L, 1).pasteSubImage(Image(L, 2), I(L, 3), I(L, 4), luaL_optint(L, 5, 256)));	// image, src, left, top, alpha, ok

			return 1;
		}
	}, {
		"rescale", [](lua_State * L)
		{
			lua_pushboolean(L, Image(L, 1).rescale(U(L, 2), U(L, 3), (FREE_IMAGE_FILTER)GetCode(L, kFilter, 4)));	// image, w, h, filter, ok

			return 1;
		}
	}, {
		"rotate", [](lua_State * L)
		{
//			BOOL Image(L, 1).rotate(double, bkcolor = NULL)

			return 0;
		}
	}, {
		"rotateEx", [](lua_State * L)
		{
			lua_pushboolean(L, Image(L, 1).rotateEx(D(L, 2), D(L, 3), D(L, 4), D(L, 5), D(L, 6), lua_toboolean(L, 7))); // image, angle, xshift, yshift, xorigin, yorigin, use_mask, ok

			return 1;
		}
	}, {
		"save", [](lua_State * L)
		{
			const char * name = S(L, 2);

			lua_pushboolean(L, Image(L, 1).save(name, GetSaveFlags(L, fipImage::identifyFIF(name), 3)));// image, name, flags, ok

			return 1;
		}
	}, {
		"saveToMemory", [](lua_State * L)
		{
			fipMemoryIO & mio = MemoryIO(L, 3);

			lua_pushboolean(L, Image(L, 1).saveToMemory((FREE_IMAGE_FORMAT)GetCode(L, kFormat, 2), mio, GetSaveFlags(L, fipImage::identifyFIFFromMemory(mio), 4)));// image, format, mio[, flags], ok

			return 1;
		}
	}, {
		"setChannel", [](lua_State * L)
		{
			lua_pushboolean(L, Image(L, 1).setChannel(Image(L, 2), (FREE_IMAGE_COLOR_CHANNEL)GetCode(L, kColorChannel, 3)));	// image, new, channel, ok

			return 1;
		}
	}, {
		"setFileBkColor", [](lua_State * L)
		{
			lua_settop(L, 2);	// image, color

			RGBQUAD quad = { 0 }, * color = !lua_isnil(L, 1) ? &quad : NULL;

			if (color) RGBQuad(L, 2, color);

			lua_pushboolean(L, Image(L, 1).setFileBkColor(color));	// image, color, ok

			return 1;
		}
	}, {
		"setHorizontalResolution", [](lua_State * L)
		{
			Image(L, 1).setHorizontalResolution(D(L, 2));

			return 0;
		}
	}, {
		"setMetadata", [](lua_State * L)
		{
			lua_pushboolean(L, Image(L, 1).setMetadata((FREE_IMAGE_MDMODEL)GetCode(L, kMdModel, 2), S(L, 3), Tag(L, 4)));	// image, model, key, tag, ok

			return 1;
		}
	}, {
		"setPixelColor", [](lua_State * L)
		{
			RGBQUAD quad;

			RGBQuad(L, 4, &quad);

			lua_pushboolean(L, Image(L, 1).setPixelColor(U(L, 2), U(L, 3), &quad));	// image, x, y, quad, ok

			return 1;
		}
	}, {
		"setPixelIndex", [](lua_State * L)
		{
			BYTE index = lua_tointeger(L, 4);

			lua_pushboolean(L, Image(L, 1).setPixelIndex(U(L, 2), U(L, 3), &index));// image, x, y, index, ok

			return 1;
		}
	}, {
		"setSize", [](lua_State * L)
		{
			lua_pushboolean(L, Image(L, 1).setSize((FREE_IMAGE_TYPE)GetCode(L, kType, 2), U(L, 3), U(L, 4), U(L, 5), U0(L, 6), U0(L, 7), U0(L, 8)));	// image, type, w, h, bpp, rmask, gmask, bmask, ok

			return 1;
		}
	}, {
		"setThumbnail", [](lua_State * L)
		{
			lua_pushboolean(L, Image(L, 1).setThumbnail(Image(L, 2)));	// image, thumb, ok

			return 1;
		}
	}, {
		"setTransparencyTable", [](lua_State * L)
		{
//			Image(L, 1).setTransparencyTable(BYTE *, int)

			return 0;
		}
	}, {
		"setVerticalResolution", [](lua_State * L)
		{
			Image(L, 1).setVerticalResolution(D(L, 2));

			return 0;
		}
	}, {
		FI_IMAGES3(splitChannels)
	}, {
		"threshold", [](lua_State * L)
		{
			lua_pushboolean(L, Image(L, 1).threshold(lua_tointeger(L, 2)));	// image, t

			return 1;
		}
	}, {
		"toneMapping", [](lua_State * L)
		{
			lua_pushboolean(L, Image(L, 1).toneMapping((FREE_IMAGE_TMO)GetCode(L, kTMO, 2), luaL_optnumber(L, 3, 0.0), luaL_optnumber(L, 4, 0.0), luaL_optnumber(L, 5, 1.0), luaL_optnumber(L, 6, 0.0)));	// image, tmo, p1, p2, p3, p4, ok

			return 1;
		}
	},
	// ...
	{ NULL, NULL }
};

luaL_reg * GetImageMethods (void)
{
	return image_methods;
}

luaL_Reg image_gc[] = {
	{
		"gc", GC<fipImage>
	},
	{ NULL, NULL }
};

#undef FI_BOOL
#undef FI_BOOLK
#undef FI_BOOL_D
#undef FI_UK
#undef FI_IMAGES3

luaL_Reg image_static_funcs[] = {
	{
		"IdentifyFIF", [](lua_State * L)
		{
			lua_pushinteger(L, fipImage::identifyFIF(S(L, 1)));	// name, fif

			GetCode(L, kFormat, -1);

			return 1;
		}
	}, {
		"NewImage", [](lua_State * L)
		{
			lua_settop(L, 4);	// type, w, h, bpp

			if (!lua_isnil(L, 1)) GetCode(L, kType, 1);

			fipImage * image = PushImage(L, false); // type, w, h, bpp, image

			new (image) fipImage((FREE_IMAGE_TYPE)luaL_optinteger(L, 1, FIT_BITMAP), U0(L, 2), U0(L, 3), U0(L, 4));

			return 1;
		}
	},
	{ NULL, NULL }
};

int FI_LoadImage (lua_State * L)
{
	luaL_register(L, NULL, image_static_funcs);

	return 0;
}