---	Various FreeImage flags.
--
-- The flags are shown here as tables, for documentation purposes, but in practice the Corona binding only
-- accepts strings, namely the various field names.
--
-- These are passed to various **save** and **load** APIs. Given a file of a certain format, fields from the
-- corresponding **save** or **load** table can be passed to the call. Single flags can be passed directly,
-- e.g. as `"FLOAT"`, whereas multiple flags are passed through arrays, e.g. as `{ "B44", "LC" }`.
--
-- All formats support a **"DEFAULT"** flag, which as expected leaves the default behavior intact (though
-- additional flags will override it); an absent flags parameter has the same result. Unknown flags (either
-- to the format in question or generally) are ignored.
--
-- A special **"LOAD\_NOPIXELS"** flag is available for certain formats. Using this flag is a request that
-- only the image header is loaded. When not supported, full loading occurs.
-- @module flags

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

--- BMP save flags.
-- @field RLE Run-length encoding.
-- @table BMP_save

--- EXR save flags.
-- @field FLOAT Save data as float instead of as half (not recommended).
-- @field NONE Save with no compression.
-- @field ZIP Save with zlib compression, in blocks of 16 scan lines.
-- @field PIZ Save with piz-based wavelet compression.
-- @field PXR24 Save with lossy 24-bit float compression.
-- @field B44 Save with lossy 44% float compression - goes to 22% when combined with **LC**.
-- @field LC Save images with one luminance and two chroma channels, rather than as RGB (lossy compression).
-- @table EXR_save

--- GIF load flags.
-- @field LOAD256 Load the image as a 256 color image with unused palette entries, if it's 16 or 2 color.
-- @field PLAYBACK 'Play' the GIF to generate each frame (as 32bpp) instead of returning raw frame data when loading.
-- @table GIF_load

--- ICO load flags.
-- @field MAKEALPHA Convert to 32bpp and create an alpha channel from the AND-mask when loading.
-- @table ICO_load

--- JPEG load flags.
-- @field FAST Load the file as fast as possible, sacrificing some quality.
-- @field ACCURATE Load the file with the best quality, sacrificing some speed.
-- @field CMYK Load separated CMYK "as is" (may combine with other load flags).
-- @field EXIFROTATE Load and rotate according to Exif 'Orientation' tag if available.
-- @field GREYSCALE Load and convert to a 8-bit greyscale image.
-- @table JPEG_load

--- JPEG save flags.
-- @field QUALITYSUPERB Save with superb quality (100:1).
-- @field QUALITYGOOD Save with good quality (75:1).
-- @field QUALITYNORMAL Save with normal quality (50:1).
-- @field QUALITYAVERAGE Save with average quality (25:1).
-- @field QUALITYBAD Save with bad quality (10:1).
-- @field PROGRESSIVE Save as a progressive-JPEG (may combine with other save flags).
-- @field SUBSAMPLING_411 Save with high 4x1 chroma subsampling (4:1:1) .
-- @field SUBSAMPLING_420 Save with medium 2x2 medium chroma subsampling (4:2:0) - default value.
-- @field SUBSAMPLING_422 Save with low 2x1 chroma subsampling (4:2:2).
-- @field SUBSAMPLING_444 Save with no chroma subsampling (4:4:4).
-- @field OPTIMIZE On saving, compute optimal Huffman coding tables (can reduce a few percent of file size).
-- @field BASELINE Save basic JPEG, without metadata or any markers.
-- @table JPEG_save

--- JXR save flags.
-- @field LOSSLESS Save lossless.
-- @field PROGRESSIVE Save as a progressive-JXR (may combine with other save flags).
-- @table JXR_save

--- PCD load flags.
-- @field BASE Load the bitmap sized 768 x 512.
-- @field BASEDIV4 Load the bitmap sized 384 x 256.
-- @field BASEDIV16 Load the bitmap sized 192 x 128.
-- @table PCD_load

--- PNG load flags.
-- @field IGNOREGAMMA Loading: avoid gamma correction.
-- @table PNG_load

--- PNG save flags.
-- @field Z_BEST_SPEED Save using ZLib level 1 compression flag (default value is 6).
-- @field Z_DEFAULT_COMPRESSION Save using ZLib level 6 compression flag (default recommended value).
-- @field Z_BEST_COMPRESSION Save using ZLib level 9 compression flag (default value is 6).
-- @field Z_NO_COMPRESSION Save without ZLib compression.
-- @field INTERLACED Save using Adam7 interlacing (may combine with other save flags).
-- @table PNG_save

--- PNM save flags.
-- @field SAVE_ASCII If set the writer saves in ASCII format (i.e. P1, P2 or P3).
-- @table PNM_save

--- PSD load flags.
-- @field CMYK Reads tags for separated CMYK (default is conversion to RGB).
-- @field LAB Reads tags for CIELab (default is conversion to RGB).
-- @table PSD_load

--- PSD save flags.
-- @field CMYK As per load.
-- @field LAB As per load.
-- @table PSD_save

--- RAW load flags.
-- @field PREVIEW Try to load the embedded JPEG preview with included Exif Data or default to RGB 24-bit.
-- @field DISPLAY Load the file as RGB 24-bit.
-- @table RAW_load

--- RAW save flags.
-- @field HALFSIZE Output a half-size color image.
-- @field UNPROCESSED Output a FIT_UINT16 raw Bayer image.
-- @table RAW_save

--- TARGA load flags.
-- @field LOAD_RGB888 If set the loader converts RGB555 and ARGB8888 -> RGB888.
-- @table TARGA_load

--- TARGA save flags.
-- @field SAVE_RLE If set, the writer saves with RLE compression.
-- @table TARGA_save

--- TIFF load flags.
-- @field CMYK Reads/stores tags for separated CMYK.
-- @table TIFF_load

--- TIFF save flags.
-- @field CMYK As with load (may combine with compression flags).
-- @field PACKBITS Save using PACKBITS compression.
-- @field DEFLATE Save using DEFLATE compression (a.k.a. ZLIB compression).
-- @field ADOBE_DEFLATE Save using ADOBE DEFLATE compression.
-- @field NONE Save without any compression.
-- @field CCITTFAX3 Save using CCITT Group 3 fax encoding.
-- @field CCITTFAX4 Save using CCITT Group 4 fax encoding.
-- @field LZW Save using LZW compression.
-- @field JPEG Save using JPEG compression.
-- @field LOGLUV Save using LogLuv compression.
-- @table TIFF_save

--- WEBP save flags.
-- @field LOSSLESS Save in lossless mode.
-- @table WEBP_save