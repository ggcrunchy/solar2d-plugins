--- Wrapper over [Spot](https://github.com/r-lyeh/spot)'s image class.
--
-- Spot images implement the [ByteReader protocol](https://ggcrunchy.github.io/corona-plugin-docs/DOCS/ByteReader/policy.html). By
-- default they send RGBA image data, but the raw floating-point data may be used instead, cf. @{SpotImage:set_reader_mode}.
--
-- The **Bytes** type&mdash;specified below&mdash;may be any object that implements said protocol, including strings.
--
-- **NOTE** Spot image support is still experimental.
--
-- ==============================================
--
-- **(The comments that follow were adapted from Spot's README.)**
--
-- - Spot is a compact and embeddable pixel / image library.
-- - Spot supports both RGBA / HSLA pixel types.
-- - Spot provides both pixel and image algebra for pixel / image manipulation.
-- - Spot loads WEBP, JPG, progressive JPG, PNG, TGA, DDS DXT1/2/3/4/5, BMP, PSD, GIF, PVR2/3 (ETC1/PVRTC), KTX (ETC1/PVRTC), PKM (ETC1), HDR, PIC, PNM (PPM/PGM), CRN, PUG, FLIF, CCZ, EXR and vectorial SVG files.
-- - Spot saves WEBP, JPG, PNG, TGA, BMP, DDS, PVR3 (PVRTC), KTX (ETC1), PKM (ETC1), CCZ and PUG files.
--
-- (**N.B.** ETC and FLIF are disabled on iOS until some issues are resolved.)
--
-- **Licenses**
--
-- - [spot](https://github.com/r-lyeh/spot) (ZLIB license).
-- - [crn2dds](redist/deps/crn2dds) by r-lyeh, SpartanJ and Evan Parker (Public Domain).
-- - [crnlib](https://code.google.com/p/crunch/), by Rich Geldreich (ZLIB license).
-- - [DDS writer](http://www.lonesock.net/soil.html) by Jonathan Dummer (Public Domain).
-- - [etc1utils](redist/deps/soil2/etc1_utils.h) by Google Inc (Apache 2.0 license).
-- - [etcpak](https://bitbucket.org/wolfpld/etcpak/wiki/Home) by Bartosz Taudul (BSD-3 license).
-- - [flif](https://github.com/flif-hub/flif) by Jon Sneyers and Pieter Wuille (Apache 2.0 license).
-- - [jpge](https://code.google.com/p/jpeg-compressor/) by Rich Geldreich (Public Domain).
-- - [libwebp](https://code.google.com/p/webp/) by Google Inc (BSD license).
-- - [lodepng](http://lodev.org/lodepng/) by Lode Vandevenne (ZLIB license).
-- - [nanosvg](https://github.com/memononen/nanosvg/) by Mikko Mononen (ZLIB license).
-- - [pngrim](https://github.com/fgenesis/pngrim) alpha bleeding algorithm by F-Genesis (Public Domain).
-- - [pug](https://github.com/r-lyeh/pug) (Public Domain).
-- - [pvrtccompressor](https://bitbucket.org/jthlim/pvrtccompressor/) by Jeffrey Lim (BSD-3 license).
-- - [rg_etc1](https://code.google.com/p/rg-etc1/) by Rich Geldreich (ZLIB license).
-- - [soil2](https://bitbucket.org/SpartanJ/soil2/) by Martin Lucas Golini and Jonathan Dummer (Public Domain).
-- - [stb_image](http://github.com/nothings/stb) by Sean Barrett (Public Domain).
-- - [tinyexr](https://github.com/syoyo/tinyexr) by Syoyo Fujita (BSD3 license).
-- - [unifont](https://github.com/r-lyeh/unifont) (ZLIB license).
--
-- ===============================================
-- @classmod SpotImage

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

--- Metamethod.
--
-- The right-hand argument must be a **SpotColor**, and is added to each pixel.
--
-- Any errors are propagated.
-- @function SpotImage:__add
-- @treturn SpotImage The post-addition image.

--- Modify this image by adding a color to each pixel.
-- @function SpotImage:add_mutate
-- @tparam SpotColor color Color to add.
-- @return[1] **true**, indicating success.
-- @return[2] **false**, meaning failure.
-- @treturn[2] string Error message.

--- Create a new image with this image's pixels run through a blank filter.
-- @function SpotImage:blank
-- @treturn[1] SpotImage New image.
-- @return[2] **nil**, meaning failure.
-- @treturn[2] string Error message.

--- Create a new image with this image's pixels run through a bleed filter.
-- @function SpotImage:bleed
-- @treturn[1] SpotImage New image.
-- @return[2] **nil**, meaning failure.
-- @treturn[2] string Error message.

--- Create a new image with this image's pixels run through a checker filter.
-- @function SpotImage:checkered
-- @treturn[1] SpotImage New image.
-- @return[2] **nil**, meaning failure.
-- @treturn[2] string Error message.

--- Create a new image with all pixels' color components clamped to [0, 1].
-- @function SpotImage:clamp
-- @treturn[1] SpotImage New image.
-- @return[2] **nil**, meaning failure.
-- @treturn[2] string Error message.

--- Copy a region of this image, given an offset and optional size.
-- @function SpotImage:copy
-- @uint x Horizontal offset of region, &ge; 1...
-- @uint y ...and vertical offset, &ge; 1.
-- @tparam ?uint w Width of new image... (If absent, the rest of the width, given _x_.)
-- @tparam ?uint h ...and height. (If absent, the rest of the height, given _y_.)
-- @treturn[1] SpotImage New image.
-- @return[2] **nil**, meaning failure.
-- @treturn[2] string Error message.

--- Copy a region of this image, given the edge offsets.
-- @function SpotImage:crop
-- @uint l Left-hand edge, &ge; 1...
-- @uint r ...and right-hand edge, between _l_ and the width, inclusive.
-- @uint t Top edge, &ge; 1...
-- @uint b ...and bottom edge, between _t_ and the height, inclusive.
-- @treturn[1] SpotImage New image.
-- @return[2] **nil**, meaning failure.
-- @treturn[2] string Error message.

--- Metamethod.
--
-- The right-hand argument must be a **SpotColor**, and each pixel is divided by it.
--
-- Any errors are propagated.
-- @function SpotImage:__div
-- @treturn SpotImage The post-division image.

--- Modify this image by dividing each pixel by a color.
-- @function SpotImage:div_mutate
-- @tparam SpotImage color Color to divide by.
-- @return[1] **true**, indicating success.
-- @return[2] **false**, meaning failure.
-- @treturn[2] string Error mesage.

--- Save this image out to memory, in the requested format.
-- @function SpotImage:encode
-- @string format One of **"PNG"**, **"JPG"**, **"PUG"**, **"WEBP"**, **"KTX"**, **"PVR"**, **"CCZ"**, or **"PKM"**,
-- corresponding to the encoding format.
-- @tparam ?table opts Encoding options, which include:
--
-- * **channels**: Number of channels in the output, an integer from 1 to 4 (by default, 4). At the moment, this only
-- applies to the PNG format.
-- * **quality**: A value between 1 and 100 (by default, 90) that describes the requested encoding quality. This is
-- available to all formats except PNG.
-- @treturn[1] string On success, the encoded image.
-- @return[2] **nil**, meaning failure.
-- @treturn[2] string Error message.

--- Create a new image with all this image's pixels' rows flipped.
-- @function SpotImage:flip_h
-- @treturn[1] SpotImage New image.
-- @return[2] **nil**, meaning failure.
-- @treturn[2] string Error message.

--- Create a new image with all this image's pixels' columns flipped.
-- @function SpotImage:flip_w
-- @treturn[1] SpotImage New image.
-- @return[2] **nil**, meaning failure.
-- @treturn[2] string Error message.
				
--- Get a color from the first row of this image.
-- @function SpotImage:get
-- @uint x Horizontal offset, from 1 to the width.
-- @treturn[1] SpotColor On success, a copy of the color.
-- @return[2] **nil**, meaning failure.
-- @treturn[2] string Error message.

--- Get a color from this image.
-- @function SpotImage:get
-- @uint x Horizontal offset, from 1 to the width...
-- @uint y ...and vertical offset, from 1 to the height.
-- @treturn[1] SpotColor On success, a copy of the color.
-- @return[2] **nil**, meaning failure.
-- @treturn[2] string Error message.

--- Get a color from this image, allowing for depth.
-- @function SpotImage:get
-- @uint x Horizontal offset, from 1 to the width...
-- @uint y ...vertical offset, from 1 to the height...
-- @uint z ...and depth-wise offset, from 1 to the depth.
-- @treturn[1] SpotColor On success, a copy of the color.
-- @return[2] **nil**, meaning failure.
-- @treturn[2] string Error message.

--- Get a color from the first row of this image, using normalized coordinates.
-- @function SpotImage:getf
-- @number x Horizontal offset, in [0, 1].
-- @treturn[1] SpotColor On success, a copy of the color.
-- @return[2] **nil**, meaning failure.
-- @treturn[2] string Error message.

--- Get a color from this image, using normalized coordinates.
-- @function SpotImage:getf
-- @uint x Horizontal offset, in [0, 1]...
-- @uint y ...and vertical offset, ditto.
-- @treturn[1] SpotColor On success, a copy of the color.
-- @return[2] **nil**, meaning failure.
-- @treturn[2] string Error message.

--- Get a color from this image, using normalized coordinates and allowing for depth.
-- @function SpotImage:getf
-- @uint x Horizontal offset, in [0, 1]...
-- @uint y ...and vertical offset, ditto...
-- @number z ...and depth-wise offset, ditto.
-- @treturn[1] SpotColor On success, a copy of the color.
-- @return[2] **nil**, meaning failure.
-- @treturn[2] string Error message.

--- Create a new image with this image's pixels run through a glow filter.
-- @function SpotImage:glow
-- @treturn[1] SpotImage New image.
-- @return[2] **nil**, meaning failure.
-- @treturn[2] string Error message.

--- Populate this image from an image file.
-- @function SpotImage:load
-- @param[opt=system.ResourceDirectory] baseDir (**WIP**) Directory to search, as per
-- [system.pathForFile](https://docs.coronalabs.com/api/library/system/pathForFile.html).
-- @string filename Name of file.
-- @return[1] **true**, indicating success.
-- @return[2] **false**, meaning failure.
-- @treturn[2] string Error message.

--- (**WIP**) Alternate method overload.
-- @function SpotImage:load
-- @ptable params Info parameters. This must include **filename** and may have a **baseDir** field, with meanings as in
-- the other overload.
--
-- If an **is\_absolute** is present and true, _filename_ is interpreted as an absolute path (any _baseDir_ is ignored). On
-- non-desktop platforms, this raises an error.
-- @return As per the other overload.

--- Populate this image from an in-memory image.
-- @function SpotImage:load_from_memory
-- @tparam Bytes bytes Undecoded image contents.
-- @return[1] **true**, indicating success.
-- @return[2] **false**, meaning failure.
-- @treturn[2] string Error message.

--- Populate this image from an HDR image file.
-- @function SpotImage:load_hdr
-- @string filename Name of file.
-- @param[opt=system.ResourceDirectory] baseDir (**WIP**) Directory to search, as per
-- [system.pathForFile](https://docs.coronalabs.com/api/library/system/pathForFile.html).
-- @return[1] **true**, indicating success.
-- @return[2] **false**, meaning failure.
-- @treturn[2] string Error message.

--- (**WIP**) Alternate method overload.
-- @function SpotImage:load_hdr
-- @ptable params Load parameters. See the table overload of @{SpotImage:load} for details.
-- @return As per the other overload.

--- Populate this image from an in-memory HDR image.
-- @function SpotImage:load_hdr_from_memory
-- @tparam Bytes bytes Undecoded image contents.
-- @return[1] **true**, indicating success.
-- @return[2] **false**, meaning failure.
-- @treturn[2] string Error message.

--- Did the most recent load operation succeed?
-- @function SpotImage:loaded
-- @treturn boolean Load was successful?

--- Metamethod.
--
-- The right-hand argument must be a **SpotColor**, and each pixel is multiplied by it.
--
-- Any errors are propagated.
-- @function SpotImage:__mul
-- @treturn SpotImage The post-multiplication image.

--- Modify this image by multiplying each pixel by a color.
-- @function SpotImage:mul_mutate
-- @tparam SpotColor color Color to multiply by.
-- @return[1] **true**, indicating success.
-- @return[2] **false**, meaning failure.
-- @treturn[2] string Error mesage.

--- Paste an image (or part of one, if too large) into this image at an offset.
-- @function SpotImage:paste
-- @uint x Horizontal offset , &ge; 1...
-- @uint y ...and vertical offset, &ge; 1.
-- @tparam SpotImage image Image to add.
-- @treturn[1] SpotImage New image.
-- @return[2] **nil**, meaning failure.
-- @treturn[2] string Error message.

--- (**WIP**) Create a new image with alpha premultiplied into all this image's pixel colors.
-- @function SpotImage:premultiply
-- @treturn[1] SpotImage New image.
-- @return[2] **nil**, meaning failure.
-- @treturn[2] string Error message.
-- @see SpotColor:premultiply

--- (**WIP**) Modify this image by premultiplying alpha into all pixels.
-- @function SpotImage:premultiply_mutate
-- @return[1] **true**, indicating success.
-- @return[2] **false**, meaning failure.
-- @treturn[2] string Error mesage.
-- @see SpotColor:premultiply

--- Get the image contents in RGBA byte form.
-- @function SpotImage:rgba
-- @treturn[1] string The image as bytes.
-- @return[2] **nil**, meaning failure.
-- @treturn[2] string Error message.

--- Create a new image with this image's contents rotated left 90&deg;.
-- @function SpotImage:rotate_left
-- @treturn[1] SpotImage New image.
-- @return[2] **nil**, meaning failure.
-- @treturn[2] string Error message.

--- Create a new image with this image's contents rotated right 90&deg;.
-- @function SpotImage:rotate_right
-- @treturn[1] SpotImage New image.
-- @return[2] **nil**, meaning failure.
-- @treturn[2] string Error message.
			
--- Save this image to a file.
-- @function SpotImage:save
-- @string filename Name of file.
-- @param[opt=system.DocumentsDirectory] baseDir (**WIP**) Directory in which to save, as per
-- [system.pathForFile](https://docs.coronalabs.com/api/library/system/pathForFile.html).
-- @tparam ?table opts Save options.
--
-- In addition to the choices available in @{SpotImage:encode}, a **format** may be provided with any of
-- **"BMP"**, **"DDS"**, **"TGA"**, **"PNG"**, **"JPG"**, **"PUG"**, **"WEBP"**, **"KTX"**, **"PVR"**,
-- **"CCZ"**, or **"PKM"**, to explicitly save in the given format (by default, the method expects one of
-- these as _filename_'s extension).
-- @return[1] **true**, indicating success.
-- @return[2] **false**, meaning failure.
-- @treturn[2] string Error message.

--- Set a color in the first row of this image.
-- @function SpotImage:set
-- @uint x Horizontal offset, from 1 to the width.
-- @tparam SpotColor color Color to assign.
-- @return[1] **true**, indicating success.
-- @return[2] **false**, meaning failure.
-- @treturn[2] string Error message.

--- Set a color in this image.
-- @function SpotImage:set
-- @uint x Horizontal offset, from 1 to the width...
-- @uint y ...and vertical offset, from 1 to the height.
-- @tparam SpotColor color Color to assign.
-- @return[1] **true**, indicating success.
-- @return[2] **false**, meaning failure.
-- @treturn[2] string Error message.

--- Set a color in this image, allowing for depth.
-- @function SpotImage:set
-- @uint x Horizontal offset, from 1 to the width...
-- @uint y ...vertical offset, from 1 to the height.
-- @uint z ...and depth-wise offset, from 1 to the depth.
-- @tparam SpotColor color Color to assign.
-- @return[1] **true**, indicating success.
-- @return[2] **false**, meaning failure.
-- @treturn[2] string Error message.

--- Set a color in the first row of this image, using normalized coordinates.
-- @function SpotImage:setf
-- @number x Horizontal offset, in [0, 1].
-- @tparam SpotColor color Color to assign.
-- @return[1] **true**, indicating success.
-- @return[2] **false**, meaning failure.
-- @treturn[2] string Error message.

--- Set a color in this image, using normalized coordinates.
-- @function SpotImage:setf
-- @number x Horizontal offset, in [0, 1]...
-- @number y ...and vertical offset, ditto.
-- @tparam SpotColor color Color to assign.
-- @return[1] **true**, indicating success.
-- @return[2] **false**, meaning failure.
-- @treturn[2] string Error message.

--- Set a color in this image, using normalized coordinates and allowing for depth.
-- @function SpotImage:setf
-- @number x Horizontal offset, in [0, 1]...
-- @number y ...and vertical offset, ditto.
-- @number z ...and depth-wise offset, ditto.
-- @tparam SpotColor color Color to assign.
-- @return[1] **true**, indicating success.
-- @return[2] **false**, meaning failure.
-- @treturn[2] string Error message.

--- Set the mode used when bytes are read from the image, cf. the summary above.
-- @function SpotImage:set_reader_mode
-- @param mode If this is **"raw"**, the image will expose its bytes directly, in floating-point form.
--
-- Otherwise, something like `result = image:rgba()` is performed, with _result_ used instead. This is
-- the default behavior.
-- @see SpotImage:rgba

--- Metamethod.
--
-- The right-hand argument must be a **SpotColor**, and is subtracted from each pixel.
--
-- Any errors are propagated.
-- @function SpotImage:__sub
-- @treturn SpotImage The post-subtraction image.

--- Modify this image by subtracting a color from each pixel.
-- @function SpotImage:sub_mutate
-- @tparam SpotColor color Color to subtract.
-- @return[1] **true**, indicating success.
-- @return[2] **false**, meaning failure.
-- @treturn[2] string Error mesage.

--- Create a clone of this image with components in HSLA form.
-- @function SpotImage:to_hsla
-- @treturn[1] SpotImage New image.
-- @return[2] **nil**, meaning failure.
-- @treturn[2] string Error message.

--- Create a clone of this image with components in RGBA form.
-- @function SpotImage:to_rbga
-- @treturn[1] SpotImage New image.
-- @return[2] **nil**, meaning failure.
-- @treturn[2] string Error message.

--- (**WIP**) Create a new image with alpha un-premultiplied from all this image's pixel colors.
-- @function SpotImage:unpremultiply
-- @treturn[1] SpotImage New image.
-- @return[2] **nil**, meaning failure.
-- @treturn[2] string Error message.
-- @see SpotColor:unpremultiply

--- (**WIP**) Modify this image by un-premultiplying alpha from all pixels.
-- @function SpotImage:unpremultiply_mutate
-- @return[1] **true**, indicating success.
-- @return[2] **false**, meaning failure.
-- @treturn[2] string Error mesage.
-- @see SpotColor:unpremultiply