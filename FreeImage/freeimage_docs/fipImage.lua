--- Corona wrapper for **fipImage**, a class used to manage all photo related images and all image types used by the library.
--
-- **fipImage** encapsulates the FIBITMAP format. It relies on the FreeImage library, especially for loading / saving images
-- and for bit depth conversion.
-- @module fipImage

--
-- Permission is hereby granted, free of charge, to any person obtaining
-- a copy of this software and associated documentation files (the
-- "Software"), to deal in the Software without restriction, including
-- without limitation the rights to use, copy, modify, merge, publish,
-- distribute, sublicense, and/or sell copies of the Software, and to
-- permit persons to whom the Software is furnished to do so, subject to
-- the following conditions:
--
-- The above copyright notice and this permission notice shall be
-- included in all copies or substantial portions of the Software.
--
-- THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
-- EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
-- MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
-- IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
-- CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
-- TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
-- SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
--
-- [ MIT license: http://www.opensource.org/licenses/mit-license.php ]
--

--- Returns a pointer to the bitmap bits.
--
-- It is up to you to interpret these bytes correctly, according to the results of FreeImage_GetBPP and GetRedMask, FreeImage_GetGreenMask
-- and FreeImage_GetBlueMask. Use this function with @{fipImage:getScanWidth} to iterates through the pixels.
-- @function fipImage:accessPixels
-- @treturn string Byte stream.

--- Adjusts the brightness of a 8, 24 or 32-bit image by a certain amount.
-- @function fipImage:adjustBrightness
-- @number percentage Where -100 <= _percentage_ <= 100.
--
-- A value 0 means no change, less than 0 will make the image darker and greater than 0 will make the image brighter.
-- @treturn boolean Operation was successful?
-- @see fipImage:adjustCurve

--- Adjusts an image's brightness, contrast and gamma within a single operation.
--
-- If more than one of these image display properties need to be adjusted, using this function should be preferred over
-- calling each adjustment function separately. That's particularly true for huge images or if performance is an issue.
-- @function fipImage:adjustBrightnessContrastGamma
-- @number brightness
-- @number constrast
-- @number gamma
-- @treturn boolean Operation was successful?
-- @see fipImage:adjustBrightness, fipImage:adjustContrast, fipImage:adjustGamma

--- Adjusts the contrast of a 8, 24 or 32-bit image by a certain amount.
-- @function fipImage:adjustContrast
-- @number percentage Where -100 <= percentage <= 100.
--
-- A value 0 means no change, less than 0 will decrease the contrast and greater than 0 will increase the contrast of the image.
-- @treturn boolean operation was successful?
-- @see fipImage:adjustCurve

--- Perfoms an histogram transformation on a 8, 24 or 32-bit image according to the values of a lookup table (LUT).
--
-- The transformation is done as follows.
--
-- Image 8-bit : if the image has a color palette, the LUT is applied to this palette, otherwise, it is applied to the grey values.
--
-- Image 24-bit & 32-bit : if channel == IPL_CC_RGB, the same LUT is applied to each color plane (R,G, and B). Otherwise, the LUT is
-- applied to the specified channel only.
-- (NYI, see FREE_IMAGE_COLOR_CHANNEL)
-- @function fipImage:adjustCurve
-- @param LUT Lookup table. The size of 'LUT' is assumed to be 256.
-- @param channel The color channel to be processed (only used with 24 & 32-bit DIB).
-- @treturn boolean Operation was successful?

--- Performs gamma correction on a 8, 24 or 32-bit image.
-- @function fipImage:adjustGamma
-- @number gamma Gamma value to use. A value of 1.0 leaves the image alone, less than one darkens it, and greater than one lightens it.
-- @treturn boolean Operation was successful?
-- @see fipImage:adjustCurve

--- Assign one image to another.
-- @function fipImage:assign
-- @tparam ?|fipImage|userdata image Image to assign. (TODO: userdata kind of shaky)

--- Destroy image data.
-- @function fipImage:clear

--- Clear the thumbnail possibly attached to the bitmap.
-- @function fipImage:clearThumbnail
-- @treturn boolean Operation was successful?

--- Creates a new image that is a copy of another.
-- @function fipImage:clone
-- @tparam fipImage image Image to clone.
-- @treturn fipImage Clone.

--- Quantizes a full colour 24-bit bitmap to a palletised 8-bit bitmap.
--
-- The quantize parameter specifies which colour reduction algorithm should be used.
-- @function fipImage:colorQuantize
-- @string algorithm Color quantization algorithm to use, cf. @{enums.FREE_IMAGE_QUANTIZE}.
-- @treturn boolean Operation was successful?

--- Builds a 24-bit RGB image given its red, green and blue channel.
-- @function fipImage:combineChannels
-- @tparam fipImage red Input red channel.
-- @tparam fipImage green Input green channel.
-- @tparam fipImage blue Input blue channel.
-- @treturn boolean Return **false** if the dib can't be allocated, if the input channels are not 8-bit
-- images; otherwise, **true**.

--- Converts the bitmap to 4 bits.
-- @function fipImage:convertTo4Bits
-- @treturn boolean Operation was successful?

--- Converts the bitmap to 8 bits.
--
-- If the bitmap is 24 or 32-bit RGB, the colour values are converted to greyscale.
-- @function fipImage:convertTo8Bits
-- @treturn boolean Operation was successful?

--- Converts the bitmap to 16 bits.
--
-- The resulting bitmap has a layout of 5 bits red, 5 bits green, 5 bits blue and 1 unused bit.
-- @function fipImage:convertTo16Bits555
-- @treturn boolean Operation was successful?

--- Converts the bitmap to 16 bits.
--
-- The resulting bitmap has a layout of 5 bits red, 6 bits green and 5 bits blue.
-- @function fipImage:convertTo16Bits565
-- @treturn boolean Operation was successful?

--- Converts the bitmap to 24 bits.
-- @function fipImage:convertTo24Bits
-- @treturn boolean Operation was successful?

--- Converts the bitmap to 32 bits.
-- @function fipImage:convertTo32Bits
-- @treturn boolean Operation was successful?

--- Converts the bitmap to a 32-bit float image.
-- @function fipImage:convertToFloat
-- @treturn boolean Operation was successful?

--- Converts the bitmap to 8 bits.
--
-- For palletized bitmaps, the color map is converted to a greyscale ramp.
-- @function fipImage:convertToGrayscale
-- @treturn boolean Operation was successful?

--- Converts the bitmap to a 48-bit RGB16 image.
-- @function fipImage:convertToRGB16
-- @treturn boolean Operation was successful?

--- Converts the bitmap to a 64-bit RGBA16 image.
-- @function fipImage:convertToRGBA16
-- @treturn boolean Operation was successful?

--- Converts the bitmap to a 128-bit RGBAF image.
-- @function fipImage:convertToRGBAF
-- @treturn boolean Operation was successful?

--- Converts the bitmap to a 96-bit RGBF image.
-- @function fipImage:convertToRGBF
-- @treturn boolean Operation was successful?

--- Converts an image to a type supported by FreeImage.
-- @function fipImage:convertToType
-- @string image_type New image type, cf. @{enums.FREE_IMAGE_TYPE}.
-- @bool scale_linearImage pixels must be scaled linearly when converting to a standard bitmap?
-- @treturn boolean Operation was successful?

--- Converts the bitmap to a 16-bit unsigned short image.
-- @function fipImage:convertToUINT16
-- @treturn boolean Operation was successful?

--- Copy a sub part of the current image and returns it as a **fipImage** object.
--
-- This method works with any bitmap type.
-- @function fipImage:copySubImage
-- @int left Specifies the left position of the cropped rectangle.
-- @int top Specifies the top position of the cropped rectangle.
-- @int right Specifies the right position of the cropped rectangle.
-- @int bottom Specifies the bottom position of the cropped rectangle.
-- @treturn boolean Operation was successful?
-- @treturn fipImage On success, output subimage.

--- Crop a sub part of the current image and update it accordingly.
--
-- This method works with any bitmap type.
-- @function fipImage:crop
-- @int left Specifies the left position of the cropped rectangle.
-- @int top Specifies the top position of the cropped rectangle.
-- @int right Specifies the right position of the cropped rectangle.
-- @int bottom Specifies the bottom position of the cropped rectangle.
-- @treturn boolean Operation was successful?

--- Converts a 8-bit image to a monochrome 1-bit image using a dithering algorithm.
-- @function fipImage:dither
-- @string algorithm Dithering algorithm to use, cf. @{enums.FREE_IMAGE_DITHER}.
-- @treturn boolean Operation was successful?

--- Flip the image horizontally along the vertical axis.
-- @function fipImage:flipHorizontal
-- @treturn boolean Operation was successful?

--- Flip the image vertically along the horizontal axis.
-- @function fipImage:flipVertical
-- @treturn boolean Operation was successful?

--- Returns the bitdepth of the bitmap.
--
-- When the image type is **BITMAP**, valid bitdepth can be 1, 4, 8, 16, 24 or 32.
-- @function fipImage:getBitsPerPixel
-- @treturn uint Bitdepth.
-- @see fipImage:getImageType

--- Retrieves the red, green, blue or alpha channel of a 24- or 32-bit BGR[A] image.
-- @function fipImage:getChannel
-- @string channel Color channel to extract, cf. @{enums.FREE_IMAGE_COLOR_CHANNEL}.
-- @treturn boolean Operation was successful?
-- @treturn fipImage On success, the output image to be extracted.

--- Retrieves the number of colours used in the bitmap.
--
-- If the bitmap is non-palletised, 0 is returned.
-- @function fipImage:getColorsUsed
-- @treturn uint Count.

--- Investigates the colour type of the bitmap.
-- @function fipImage:getColorType
-- @treturn string Color type, cf. @{enums.FREE_IMAGE_COLOR_TYPE}.

--- Retrieves the file background color of an image.
--
-- For 8-bit images, the color index in the palette is returned in the **rgbReserved** member of the result.
-- @function fipImage:getFileBkColor
-- @treturn boolean Operation was successful?
-- @treturn RGBQUAD On success, a color quad. (TODO?)

--- Returns the image height in pixels.
-- @function fipImage:getHeight
-- @treturn uint Height.

--- Computes image histogram.
--
-- For 24-bit and 32-bit images, histogram can be computed from red, green, blue and black channels. For 8-bit images,
-- histogram is computed from the black channel. Other bit depth is not supported.
--
-- NYI
-- @function fipImage:getHistogram
-- @tparam PDWORD pointer to an histogram array. Size of this array is assumed to be 256. (TODO!)
-- @string[opt="BLACK"] channel Color channel to use, cf. @{enums.FREE_IMAGE_COLOR_CHANNEL}.
-- @treturn boolean Operation was successful?

--- Returns the bitmap resolution along the X axis, in pixels / cm.
-- @function fipImage:getHorizontalResolution
-- @treturn number Resolution.

--- Returns the memory footprint of a bitmap, in bytes.
-- @function fipImage:getImageMemorySize
-- @treturn uint Size.

--- Returns the size of the bitmap in bytes.
--
-- The size of the bitmap is the BITMAPINFOHEADER + the size of the palette + the size of the bitmap data.
-- @function fipImage:getImageSize
-- @treturn uint Size.

--- Returns the data type of the image.
-- @function fipImage:getImageType
-- @treturn string Type, cf. @{enums.FREE_IMAGE_TYPE}.

--- Returns a pointer to the bitmap's BITMAPINFO header.
-- NYI
-- @function fipImage:getInfo

--- Returns a pointer to the bitmap's BITMAPINFOHEADER.
-- NYI
-- @function fipImage:getInfoHeader

--- Returns the width of the bitmap in bytes.
--
-- This is not the size of the scanline.
-- @function fipImage:getLine
-- @treturn uint Width.
-- @see fipImage:getScanWidth

--- Retrieve a metadata attached to the dib.
-- @function fipImage:getMetadata
-- @string model Metadata model to look for, cf. @{enums.FREE_IMAGE_MDMODEL}.
-- @string key Metadata field name.
-- @treturn boolean Operation was successful?
-- @treturn fipTag On success, the tag.

--- Returns the number of tags contained in the model metadata model attached to the dib.
-- @function fipImage:getMetadataCount
-- @string model Metadata model to look for, cf. @{enums.FREE_IMAGE_MDMODEL}.
-- @treturn uint Count.

--- Returns a pointer to the bitmap's palette.
--
-- If the bitmap doesn't have a palette, getPalette returns NULL. (TODO!)
-- @function fipImage:getPalette
-- @treturn array (TODO)

--- Returns the palette size in bytes.
-- @function fipImage:getPaletteSize
-- @treturn uint Size.

--- Get the pixel color of a 16-, 24- or 32-bit image at position (x, y), including range check (slow access).
-- @function fipImage:getPixelColor
-- @uint x Pixel position in horizontal direction.
-- @uint y Pixel position in vertical direction.
-- @treturn boolean Operation was successful?
-- @treturn RGBQUAD On success, pixel color. (TODO?)

--- Get the pixel index of a 1-, 4- or 8-bit palettized image at position (x, y), including range check (slow access).
-- @function fipImage:getPixelIndex
-- @uint x Pixel position in horizontal direction.
-- @uint y Pixel position in vertical direction.
-- @treturn boolean Operation was successful?
-- @treturn BYTE On success, pixel index. (TODO?)

--- Retrieves a copy the thumbnail possibly attached to the bitmap.
-- @function fipImage:getThumbnail
-- @treturn boolean Operation was successful?
-- @treturn fipImage On success (and a thumbnail was present), the image.

--- Returns a pointer to the start of the given scanline in the bitmap's data-bits.
--
-- This pointer can be cast according to the result returned by @{fipImage:getImageType}.
--
-- Use this function with @{fipImage:getScanWidth} to iterates through the pixels.
-- NYI
-- @function fipImage:getScanLine
-- @uint scanline
-- @treturn string Byte stream.

--- 8-bit transparency: get the number of transparent colors.
-- @function fipImage:getTransparencyCount
-- @treturn uint The number of transparent colors in a palletised bitmap.

--- 8-bit transparency: get the bitmap's transparency table.
-- NYI
-- @function fipImage:getTransparencyTable
-- @treturn string Returns a pointer to the bitmap's transparency table.

--- Returns the width of the bitmap in bytes rounded to the nearest DWORD.
-- @function fipImage:getScanWidth
-- @treturn uint Scan width.

--- Returns the bitmap resolution along the Y axis, in pixels / cm.
-- @function fipImage:getVerticalResolution
-- @treturn number Resolution.

--- Returns the image width in pixels.
-- @function fipImage:getWidth
-- @treturn uint Width.

--- Getter.
-- @function fipImage:hasFileBkColor
-- @treturn boolean Image has a file background color?

--- Check if the image has an embedded thumbnail.
-- @function fipImage:hasThumbnail
-- @treturn boolean Thumbnail is present in the bitmap?

--- Inverts each pixel data.
-- @function fipImage:invert
-- @treturn boolean Operation was successful?

--- Getter.
-- @function fipImage:isGrayscale
-- @treturn boolean Bitmap is an 8-bit bitmap with a greyscale palette?

--- Get the image status.
-- NYI (forgot!)
-- @function fipImage:isModified
-- @treturn boolean Image is marked as modified?
-- @see fipImage:setModified

--- Getter.
-- @function fipImage:isTransparent
-- @treturn boolean Image is transparent?

--- Indicates whether the image is valid for use.
--
-- Reimplemented in @{fipWinImage}.
-- @function fipImage:isValid
-- @treturn boolean Image is allocated?

--- Loads an image from disk, given its file name and an optional flag.
-- @function fipImage:load
-- @string name Path and file name of the image to load.
-- @tparam ?|{string,...}|string|nil flags Format-dependent @{flags}.
-- @treturn boolean Operation was successful?

--- Loads an image using the specified memory stream and an optional flag.
-- @function fipImage:loadFromMemory
-- @tparam fipMemoryIO memIO FreeImage memory stream.
-- @tparam ?|{string,...}|string|nil flags Format-dependent @{flags}.
-- @treturn boolean Operation was successful?

--- Creates a thumbnail image keeping aspect ratio.
-- @function fipImage:makeThumbnail
-- @uint max_size Maximum width or height in pixel units.
-- @bool[opt=true] convert Convert the image to a standard type?
-- @treturn boolean Operation was successful?

--- Get an external texture from the current state that may be used in Corona.
--
-- TODO: More thorough, caveats about threading
-- @function fipImage:newTexture
-- @treturn CoronaExternalTexture Texture which may be passed to Corona's **display** APIs via its **filename** and
-- **baseDir** fields.

--- Alpha blend or combine a sub part image with the current image.
--
-- The bit depth of dst bitmap must be greater than or equal to the bit depth of _src_. Upper promotion of _src_ is done
-- internally. Supported bit depth equals to 4, 8, 16, 24 or 32.
-- @function fipImage:pasteSubImage
-- @tparam fipImage src Source subimage.
-- @int left Specifies the left position of the sub image.
-- @int top Specifies the top position of the sub image.
-- @int[opt=256] alpha Alpha blend factor. The source and destination images are alpha blended if _alpha_ = 0..255. If
-- _alpha_ > 255, then the source image is combined to the destination image.
-- @treturn boolean Operation was successful?

--- Rescale the image to a new width / height.
-- @function fipImage:rescale
-- @uint new_width New image width.
-- @uint new_height New image height.
-- @string filter The filter parameter specifies which resampling filter should be used. (TODO: FREE_IMAGE_FILTER)
-- @treturn boolean Operation was successful?

--- Image rotation by means of three shears.
-- NYI
-- @function fipImage:rotate
-- @number angle Image rotation angle, in degree.
-- @param bkcolor Background color (image type dependent), default to black background. (TODO)
-- @return Returns rotated dib if successful, returns NULL otherwise. (TODO: bool in docs)

--- Image translation and rotation using B-Splines.
-- @function fipImage:rotateEx
-- @number angle Image rotation angle, in degree.
-- @number x_shift Image horizontal shift.
-- @number y_shift Image vertical shift.
-- @number x_origin Origin of the x-axis.
-- @number y_origin Origin of the y-axis.
-- @bool use_mask Mask the image? Image mirroring is applied when _use\_mask_ is false.
-- @return Returns the translated & rotated dib if successful, returns NULL otherwise. (TODO: bool in docs)

--- Saves an image to disk, given its file name and an optional flag.
-- @function fipImage:save
-- @string name Path and file name of the image to save.
-- @tparam ?|{string,...}|string|nil flags Format-dependent @{flags}.
-- @treturn boolean Operation was successful?

--- Saves an image using the specified memory stream and an optional flag.
-- @function fipImage:saveToMemory
-- @tparam fipMemoryIO memIO FreeImage memory stream.
-- @tparam ?|{string,...}|string|nil flags Format-dependent @{flags}.
-- @treturn boolean Operation was successful?

--- Insert an 8-bit dib into a 24- or 32-bit image.
-- @function fipImage:setChannel
-- @tparam fipImage image Input 8-bit image to insert.
-- @string channel Color channel to replace, cf. @{enums.FREE_IMAGE_COLOR_CHANNEL}.
-- @treturn boolean Operation was successful?

--- Set the file background color of an image.
--
-- When saving an image to PNG, this background color is transparently saved to the PNG file. When the _bkcolor_ parameter
-- is absent, the background color is removed from the image.
-- @function fipImage:setFileBkColor
-- @string[opt] bkcolor (TODO: RGBQUAD)
-- @treturn boolean Operation was successful?

--- Set the bitmap resolution along the X axis, in pixels / cm.
-- @function fipImage:setHorizontalResolution
-- @number value

--- Attach a new FreeImage tag to the dib.
--
-- Sample use (TODO: Lua version):
--     fipImage image;
--     -- ...
--     fipTag tag;
--     tag.setKeyValue("Caption/Abstract", "my caption");
--     image.setMetadata(FIMD_IPTC, tag.getKey(), tag);
--     tag.setKeyValue("Keywords", "FreeImage;Library;Images;Compression");
--     image.setMetadata(FIMD_IPTC, tag.getKey(), tag);
-- @function fipImage:setMetadata
-- @string key Tag field name.
-- @tparam fipTag tag Tag to be attached.
-- @treturn Operation was successful?

--- Set the image status as 'modified'.
--
-- When using the @{fipWinImage} class, the image status is used to refresh the display. It is changed to FALSE whenever the
-- display has just been refreshed.
-- NYI (missed it :) )
-- @function fipImage:setModified
-- @bool[opt=true] bStatus The image should be marked as modified?
-- @see fipImage:isModified

--- Set the pixel color of a 16-, 24- or 32-bit image at position (x, y), including range check (slow access).
-- @function fipImage:setPixelColor
-- @uint x Pixel position in horizontal direction.
-- @uint y Pixel position in vertical direction.
-- @tparam RGBQUAD * value Pixel color. (TODO: RGBQUAD *)
-- @treturn Operation was successful?

--- Set the pixel index of a 1-, 4- or 8-bit palettized image at position (x, y), including range check (slow access).
-- @function fipImage:setPixelIndex
-- @uint x Pixel position in horizontal direction.
-- @uint y Pixel position in vertical direction.
-- @tparam BYTE * value Pixel index. (TODO: BYTE *)
-- @treturn Operation was successful?

--- Image allocator.
-- @function fipImage:setSize
-- @string type Image type, cf. @{enums.FREE_IMAGE_TYPE}.
-- @uint w Width.
-- @uint h Height.
-- @uint bpp Bitdepth.
-- @uint[opt=0] rmask Red mask...
-- @uint[opt=0] gmask ...green...
-- @uint[opt=0] bmask ...and blue.
-- @treturn Operation was successful?

--- Attach a thumbnail to the bitmap.
-- @function fipImage:setThumbnail
-- @tparam fipImage image
-- @treturn Operation was successful?

--- 8-bit transparency: set the bitmap's transparency table.
-- NYI
-- @function fipImage:setTransparencyTable

--- Set the bitmap resolution along the Y axis, in pixels / cm.
-- @function fipImage:setVerticalResolution
-- @number value

--- Split a 24-bit RGB image into 3 greyscale images corresponding to the red, green and blue channels.
-- @function fipImage:splitChannels
-- @treturn boolean Returns **false** if the dib isn't a valid image, if it's not a 24-bit image or if one of the
-- output channel can't be allocated; otherwise **true**.
-- @treturn fipImage On success, output red channel...
-- @treturn fipImage ...green channel...
-- @treturn fipImage ...and blue channel.

--- Converts the bitmap to 1 bit using a threshold _T_.
-- @function fipImage:threshold
-- @byte T Threshold value in [0..255].
-- @treturn boolean Operation was successful?

--- Converts a High Dynamic Range image (48-bit RGB or 96-bit RGB Float) to a 24-bit RGB image.
-- @function fipImage:toneMapping
-- @string tmo Tone mapping operator, cf. @{enums.FREE_IMAGE_TMO}.
-- @number[opt=0] first First tone mapping algorithm parameter (algorithm dependant).
-- @number[opt=0] second Second tone mapping algorithm parameter (algorithm dependant).
-- @number[opt=1] third Third tone mapping algorithm parameter (algorithm dependant).
-- @number[opt=0] fourth Fourth tone mapping algorithm parameter (algorithm dependant).
-- @treturn boolean Operation was successful?