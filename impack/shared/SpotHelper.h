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

#pragma once

#include "ByteReader.h"
#include "utils/Memory.h"
#include "utils/Thread.h"

// STB_IMAGE
extern ThreadXS::TLS<MemoryXS::ScopedSystem *> tls_ImageMM;

#define STBI_ASSERT(cond) if (!(cond)) tls_ImageMM->FailAssert(#cond)
#define STBI_MALLOC(sz)           tls_ImageMM->Malloc(sz)
#define STBI_REALLOC(p, newsz)    tls_ImageMM->Realloc(p, newsz)
#define STBI_FREE(p)              tls_ImageMM->Free(p)

//#line 1 "stb_image.h"
#ifndef STBI_INCLUDE_STB_IMAGE_H
#define STBI_INCLUDE_STB_IMAGE_H

// DOCUMENTATION
//
// Limitations:
//    - no 16-bit-per-channel PNG
//    - no 12-bit-per-channel JPEG
//    - no JPEGs with arithmetic coding
//    - no 1-bit BMP
//    - GIF always returns *comp=4
//
// Basic usage (see HDR discussion below for HDR usage):
//    int x,y,n;
//    unsigned char *data = stbi_load(filename, &x, &y, &n, 0);
//    // ... process data if not NULL ...
//    // ... x = width, y = height, n = # 8-bit components per pixel ...
//    // ... replace '0' with '1'..'4' to force that many components per pixel
//    // ... but 'n' will always be the number that it would have been if you said 0
//    stbi_image_free(data)
//
// Standard parameters:
//    int *x       -- outputs image width in pixels
//    int *y       -- outputs image height in pixels
//    int *comp    -- outputs # of image components in image file
//    int req_comp -- if non-zero, # of image components requested in result
//
// The return value from an image loader is an 'unsigned char *' which points
// to the pixel data, or NULL on an allocation failure or if the image is
// corrupt or invalid. The pixel data consists of *y scanlines of *x pixels,
// with each pixel consisting of N interleaved 8-bit components; the first
// pixel pointed to is top-left-most in the image. There is no padding between
// image scanlines or between pixels, regardless of format. The number of
// components N is 'req_comp' if req_comp is non-zero, or *comp otherwise.
// If req_comp is non-zero, *comp has the number of components that _would_
// have been output otherwise. E.g. if you set req_comp to 4, you will always
// get RGBA output, but you can check *comp to see if it's trivially opaque
// because e.g. there were only 3 channels in the source image.
//
// An output image with N components has the following components interleaved
// in this order in each pixel:
//
//     N=#comp     components
//       1           grey
//       2           grey, alpha
//       3           red, green, blue
//       4           red, green, blue, alpha
//
// If image loading fails for any reason, the return value will be NULL,
// and *x, *y, *comp will be unchanged. The function stbi_failure_reason()
// can be queried for an extremely brief, end-user unfriendly explanation
// of why the load failed. Define STBI_NO_FAILURE_STRINGS to avoid
// compiling these strings at all, and STBI_FAILURE_USERMSG to get slightly
// more user-friendly ones.
//
// Paletted PNG, BMP, GIF, and PIC images are automatically depalettized.
//
// ===========================================================================
//
// Philosophy
//
// stb libraries are designed with the following priorities:
//
//    1. easy to use
//    2. easy to maintain
//    3. good performance
//
// Sometimes I let "good performance" creep up in priority over "easy to maintain",
// and for best performance I may provide less-easy-to-use APIs that give higher
// performance, in addition to the easy to use ones. Nevertheless, it's important
// to keep in mind that from the standpoint of you, a client of this library,
// all you care about is #1 and #3, and stb libraries do not emphasize #3 above all.
//
// Some secondary priorities arise directly from the first two, some of which
// make more explicit reasons why performance can't be emphasized.
//
//    - Portable ("ease of use")
//    - Small footprint ("easy to maintain")
//    - No dependencies ("ease of use")
//
// ===========================================================================
//
// I/O callbacks
//
// I/O callbacks allow you to read from arbitrary sources, like packaged
// files or some other source. Data read from callbacks are processed
// through a small internal buffer (currently 128 bytes) to try to reduce
// overhead.
//
// The three functions you must define are "read" (reads some bytes of data),
// "skip" (skips some bytes of data), "eof" (reports if the stream is at the end).
//
// ===========================================================================
//
// SIMD support
//
// The JPEG decoder will try to automatically use SIMD kernels on x86 when
// supported by the compiler. For ARM Neon support, you must explicitly
// request it.
//
// (The old do-it-yourself SIMD API is no longer supported in the current
// code.)
//
// On x86, SSE2 will automatically be used when available based on a run-time
// test; if not, the generic C versions are used as a fall-back. On ARM targets,
// the typical path is to have separate builds for NEON and non-NEON devices
// (at least this is true for iOS and Android). Therefore, the NEON support is
// toggled by a build flag: define STBI_NEON to get NEON loops.
//
// The output of the JPEG decoder is slightly different from versions where
// SIMD support was introduced (that is, for versions before 1.49). The
// difference is only +-1 in the 8-bit RGB channels, and only on a small
// fraction of pixels. You can force the pre-1.49 behavior by defining
// STBI_JPEG_OLD, but this will disable some of the SIMD decoding path
// and hence cost some performance.
//
// If for some reason you do not want to use any of SIMD code, or if
// you have issues compiling it, you can disable it entirely by
// defining STBI_NO_SIMD.
//
// ===========================================================================
//
// HDR image support   (disable by defining STBI_NO_HDR)
//
// stb_image now supports loading HDR images in general, and currently
// the Radiance .HDR file format, although the support is provided
// generically. You can still load any file through the existing interface;
// if you attempt to load an HDR file, it will be automatically remapped to
// LDR, assuming gamma 2.2 and an arbitrary scale factor defaulting to 1;
// both of these constants can be reconfigured through this interface:
//
//     stbi_hdr_to_ldr_gamma(2.2f);
//     stbi_hdr_to_ldr_scale(1.0f);
//
// (note, do not use _inverse_ constants; stbi_image will invert them
// appropriately).
//
// Additionally, there is a new, parallel interface for loading files as
// (linear) floats to preserve the full dynamic range:
//
//    float *data = stbi_loadf(filename, &x, &y, &n, 0);
//
// If you load LDR images through this interface, those images will
// be promoted to floating point values, run through the inverse of
// constants corresponding to the above:
//
//     stbi_ldr_to_hdr_scale(1.0f);
//     stbi_ldr_to_hdr_gamma(2.2f);
//
// Finally, given a filename (or an open file or memory block--see header
// file for details) containing image data, you can query for the "most
// appropriate" interface to use (that is, whether the image is HDR or
// not), using:
//
//     stbi_is_hdr(char *filename);
//
// ===========================================================================
//
// iPhone PNG support:
//
// By default we convert iphone-formatted PNGs back to RGB, even though
// they are internally encoded differently. You can disable this conversion
// by by calling stbi_convert_iphone_png_to_rgb(0), in which case
// you will always just get the native iphone "format" through (which
// is BGR stored in RGB).
//
// Call stbi_set_unpremultiply_on_load(1) as well to force a divide per
// pixel to remove any premultiplied alpha *only* if the image file explicitly
// says there's premultiplied data (currently only happens in iPhone images,
// and only if iPhone convert-to-rgb processing is on).
//

#ifndef STBI_NO_STDIO
#include <stdio.h>
#endif // STBI_NO_STDIO

#define STBI_VERSION 1

enum
{
   STBI_default = 0, // only used for req_comp

   STBI_grey       = 1,
   STBI_grey_alpha = 2,
   STBI_rgb        = 3,
   STBI_rgb_alpha  = 4
};

typedef unsigned char stbi_uc;

#ifdef __cplusplus
extern "C" {
#endif

#ifdef STB_IMAGE_STATIC
#define STBIDEF static
#else
#define STBIDEF extern
#endif

//////////////////////////////////////////////////////////////////////////////
//
// PRIMARY API - works on images of any type
//

//
// load image by filename, open file, or memory buffer
//

typedef struct
{
   int      (*read)  (void *user,char *data,int size);   // fill 'data' with 'size' bytes.  return number of bytes actually read
   void     (*skip)  (void *user,int n);                 // skip the next 'n' bytes, or 'unget' the last -n bytes if negative
   int      (*eof)   (void *user);                       // returns nonzero if we are at end of file/data
} stbi_io_callbacks;

STBIDEF stbi_uc *stbi_load               (char              const *filename,           int *x, int *y, int *comp, int req_comp);
STBIDEF stbi_uc *stbi_load_from_memory   (stbi_uc           const *buffer, int len   , int *x, int *y, int *comp, int req_comp);
STBIDEF stbi_uc *stbi_load_from_callbacks(stbi_io_callbacks const *clbk  , void *user, int *x, int *y, int *comp, int req_comp);

#ifndef STBI_NO_STDIO
STBIDEF stbi_uc *stbi_load_from_file  (FILE *f,                  int *x, int *y, int *comp, int req_comp);
// for stbi_load_from_file, file pointer is left pointing immediately after image
#endif

#ifndef STBI_NO_LINEAR
   STBIDEF float *stbi_loadf                 (char const *filename,           int *x, int *y, int *comp, int req_comp);
   STBIDEF float *stbi_loadf_from_memory     (stbi_uc const *buffer, int len, int *x, int *y, int *comp, int req_comp);
   STBIDEF float *stbi_loadf_from_callbacks  (stbi_io_callbacks const *clbk, void *user, int *x, int *y, int *comp, int req_comp);

   #ifndef STBI_NO_STDIO
   STBIDEF float *stbi_loadf_from_file  (FILE *f,                int *x, int *y, int *comp, int req_comp);
   #endif
#endif

// STEVE CHANGE
STBIDEF unsigned char *stbi_xload(char const *filename, int *x, int *y, int *frames, int ** delays, ByteReader * reader);
// /STEVE CHANGE

#ifndef STBI_NO_HDR
   STBIDEF void   stbi_hdr_to_ldr_gamma(float gamma);
   STBIDEF void   stbi_hdr_to_ldr_scale(float scale);
#endif

#ifndef STBI_NO_LINEAR
   STBIDEF void   stbi_ldr_to_hdr_gamma(float gamma);
   STBIDEF void   stbi_ldr_to_hdr_scale(float scale);
#endif // STBI_NO_HDR

// stbi_is_hdr is always defined, but always returns false if STBI_NO_HDR
STBIDEF int    stbi_is_hdr_from_callbacks(stbi_io_callbacks const *clbk, void *user);
STBIDEF int    stbi_is_hdr_from_memory(stbi_uc const *buffer, int len);
#ifndef STBI_NO_STDIO
STBIDEF int      stbi_is_hdr          (char const *filename);
STBIDEF int      stbi_is_hdr_from_file(FILE *f);
#endif // STBI_NO_STDIO

// get a VERY brief reason for failure
// NOT THREADSAFE
STBIDEF const char *stbi_failure_reason  (void);

// free the loaded image -- this is just free()
STBIDEF void     stbi_image_free      (void *retval_from_stbi_load);

// get image dimensions & components without fully decoding
STBIDEF int      stbi_info_from_memory(stbi_uc const *buffer, int len, int *x, int *y, int *comp);
STBIDEF int      stbi_info_from_callbacks(stbi_io_callbacks const *clbk, void *user, int *x, int *y, int *comp);

#ifndef STBI_NO_STDIO
STBIDEF int      stbi_info            (char const *filename,     int *x, int *y, int *comp);
STBIDEF int      stbi_info_from_file  (FILE *f,                  int *x, int *y, int *comp);

#endif

// for image formats that explicitly notate that they have premultiplied alpha,
// we just return the colors as stored in the file. set this flag to force
// unpremultiplication. results are undefined if the unpremultiply overflow.
STBIDEF void stbi_set_unpremultiply_on_load(int flag_true_if_should_unpremultiply);

// indicate whether we should process iphone images back to canonical format,
// or just pass them through "as-is"
STBIDEF void stbi_convert_iphone_png_to_rgb(int flag_true_if_should_convert);

// flip the image vertically, so the first pixel in the output array is the bottom left
STBIDEF void stbi_set_flip_vertically_on_load(int flag_true_if_should_flip);

// ZLIB client - used by PNG, available for other purposes

STBIDEF char *stbi_zlib_decode_malloc_guesssize(const char *buffer, int len, int initial_size, int *outlen);
STBIDEF char *stbi_zlib_decode_malloc_guesssize_headerflag(const char *buffer, int len, int initial_size, int *outlen, int parse_header);
STBIDEF char *stbi_zlib_decode_malloc(const char *buffer, int len, int *outlen);
STBIDEF int   stbi_zlib_decode_buffer(char *obuffer, int olen, const char *ibuffer, int ilen);

STBIDEF char *stbi_zlib_decode_noheader_malloc(const char *buffer, int len, int *outlen);
STBIDEF int   stbi_zlib_decode_noheader_buffer(char *obuffer, int olen, const char *ibuffer, int ilen);

#ifndef STBI_NO_DDS

//#line 1 "stbi_DDS.h"
#ifndef HEADER_STB_IMAGE_DDS_AUGMENTATION
#define HEADER_STB_IMAGE_DDS_AUGMENTATION

/*	is it a DDS file? */
extern int      stbi__dds_test_memory      (stbi_uc const *buffer, int len);
extern int      stbi__dds_test_callbacks   (stbi_io_callbacks const *clbk, void *user);

extern stbi_uc *stbi__dds_load_from_path   (const char *filename,           int *x, int *y, int *comp, int req_comp);
extern stbi_uc *stbi__dds_load_from_memory (stbi_uc const *buffer, int len, int *x, int *y, int *comp, int req_comp);
extern stbi_uc *stbi__dds_load_from_callbacks (stbi_io_callbacks const *clbk, void *user, int *x, int *y, int *comp, int req_comp);

#ifndef STBI_NO_STDIO
extern int      stbi__dds_test_filename    (char const *filename);
extern int      stbi__dds_test_file        (FILE *f);
extern stbi_uc *stbi__dds_load_from_file   (FILE *f,                  int *x, int *y, int *comp, int req_comp);
#endif

extern int      stbi__dds_info_from_memory (stbi_uc const *buffer, int len, int *x, int *y, int *comp, int *iscompressed);
extern int      stbi__dds_info_from_callbacks (stbi_io_callbacks const *clbk, void *user, int *x, int *y, int *comp, int *iscompressed);

#ifndef STBI_NO_STDIO
extern int      stbi__dds_info_from_path   (char const *filename,     int *x, int *y, int *comp, int *iscompressed);
extern int      stbi__dds_info_from_file   (FILE *f,                  int *x, int *y, int *comp, int *iscompressed);
#endif

/*
//
////   end header file   /////////////////////////////////////////////////////*/
#endif /* HEADER_STB_IMAGE_DDS_AUGMENTATION */


#endif

#ifndef STBI_NO_PVR

//#line 1 "stbi_pvr.h"
#ifndef HEADER_STB_IMAGE_PVR_AUGMENTATION
#define HEADER_STB_IMAGE_PVR_AUGMENTATION

/*	is it a PVR file? */
extern int      stbi__pvr_test_memory      (stbi_uc const *buffer, int len);
extern int      stbi__pvr_test_callbacks   (stbi_io_callbacks const *clbk, void *user);

extern stbi_uc *stbi__pvr_load_from_path   (char const *filename,           int *x, int *y, int *comp, int req_comp);
extern stbi_uc *stbi__pvr_load_from_memory (stbi_uc const *buffer, int len, int *x, int *y, int *comp, int req_comp);
extern stbi_uc *stbi__pvr_load_from_callbacks (stbi_io_callbacks const *clbk, void *user, int *x, int *y, int *comp, int req_comp);

#ifndef STBI_NO_STDIO
extern int      stbi__pvr_test_filename    (char const *filename);
extern int      stbi__pvr_test_file        (FILE *f);
extern stbi_uc *stbi__pvr_load_from_file   (FILE *f,                  int *x, int *y, int *comp, int req_comp);
#endif

extern int      stbi__pvr_info_from_memory (stbi_uc const *buffer, int len, int *x, int *y, int *comp, int *iscompressed);
extern int      stbi__pvr_info_from_callbacks (stbi_io_callbacks const *clbk, void *user, int *x, int *y, int *comp, int *iscompressed);

#ifndef STBI_NO_STDIO
extern int      stbi__pvr_info_from_path   (char const *filename,     int *x, int *y, int *comp, int *iscompressed);
extern int      stbi__pvr_info_from_file   (FILE *f,                  int *x, int *y, int *comp, int *iscompressed);
#endif

/*
//
////   end header file   /////////////////////////////////////////////////////*/
#endif /* HEADER_STB_IMAGE_PVR_AUGMENTATION */


#endif

#ifndef STBI_NO_PKM

//#line 1 "stbi_pkm.h"
#ifndef HEADER_STB_IMAGE_PKM_AUGMENTATION
#define HEADER_STB_IMAGE_PKM_AUGMENTATION

/*	is it a PKM file? */
extern int      stbi__pkm_test_memory      (stbi_uc const *buffer, int len);
extern int      stbi__pkm_test_callbacks   (stbi_io_callbacks const *clbk, void *user);

extern stbi_uc *stbi__pkm_load_from_path   (char const *filename,           int *x, int *y, int *comp, int req_comp);
extern stbi_uc *stbi__pkm_load_from_memory (stbi_uc const *buffer, int len, int *x, int *y, int *comp, int req_comp);
extern stbi_uc *stbi__pkm_load_from_callbacks (stbi_io_callbacks const *clbk, void *user, int *x, int *y, int *comp, int req_comp);

#ifndef STBI_NO_STDIO
extern int      stbi__pkm_test_filename    (char const *filename);
extern int      stbi__pkm_test_file        (FILE *f);
extern stbi_uc *stbi__pkm_load_from_file   (FILE *f,                  int *x, int *y, int *comp, int req_comp);
#endif

extern int      stbi__pkm_info_from_memory (stbi_uc const *buffer, int len, int *x, int *y, int *comp);
extern int      stbi__pkm_info_from_callbacks (stbi_io_callbacks const *clbk, void *user, int *x, int *y, int *comp);

#ifndef STBI_NO_STDIO
extern int      stbi__pkm_info_from_path   (char const *filename,     int *x, int *y, int *comp);
extern int      stbi__pkm_info_from_file   (FILE *f,                  int *x, int *y, int *comp);
#endif

/*
//
////   end header file   /////////////////////////////////////////////////////*/
#endif /* HEADER_STB_IMAGE_PKM_AUGMENTATION */


#endif

#ifndef STBI_NO_EXT

//#line 1 "stbi_ext.h"
#ifndef HEADER_STB_IMAGE_EXT
#define HEADER_STB_IMAGE_EXT

enum {
	STBI_unknown= 0,
	STBI_jpeg	= 1,
	STBI_png	= 2,
	STBI_bmp	= 3,
	STBI_gif	= 4,
	STBI_tga	= 5,
	STBI_psd	= 6,
	STBI_pic	= 7,
	STBI_pnm	= 8,
	STBI_dds	= 9,
	STBI_pvr	= 10,
	STBI_pkm	= 11,
	STBI_hdr	= 12
};

extern int      stbi_test_from_memory      (stbi_uc const *buffer, int len);
extern int      stbi_test_from_callbacks   (stbi_io_callbacks const *clbk, void *user);

#ifndef STBI_NO_STDIO
extern int      stbi_test					(char const *filename);
extern int      stbi_test_from_file        (FILE *f);
#endif

#endif /* HEADER_STB_IMAGE_EXT */


#endif

#ifdef __cplusplus
}
#endif

//
//
////   end header file   /////////////////////////////////////////////////////
#endif // STBI_INCLUDE_STB_IMAGE_H

// STB_IMAGE_WRITE
extern ThreadXS::TLS<MemoryXS::ScopedSystem *> tls_WriteMM;
/*
#define STBIW_ASSERT(cond) if (!(cond)) tls_WriteMM->FailAssert(#cond)
#define STBIW_MALLOC(sz)           tls_WriteMM->Malloc(sz)
#define STBIW_REALLOC(p, newsz)    tls_WriteMM->Realloc(p, newsz)
#define STBIW_FREE(p)              tls_WriteMM->Free(p)
*/
//#line 1 "stb_image_write.h"
#ifndef INCLUDE_STB_IMAGE_WRITE_H
#define INCLUDE_STB_IMAGE_WRITE_H

#ifdef __cplusplus
extern "C" {
#endif

#ifdef STB_IMAGE_WRITE_STATIC
#define STBIWDEF static
#else
#define STBIWDEF extern
extern int stbi_write_tga_with_rle;
#endif

#ifndef STBI_WRITE_NO_STDIO
STBIWDEF int stbi_write_png(char const *filename, int w, int h, int comp, const void  *data, int stride_in_bytes);
STBIWDEF int stbi_write_bmp(char const *filename, int w, int h, int comp, const void  *data);
STBIWDEF int stbi_write_tga(char const *filename, int w, int h, int comp, const void  *data);
STBIWDEF int stbi_write_hdr(char const *filename, int w, int h, int comp, const float *data);
#endif

typedef void stbi_write_func(void *context, void *data, int size);

STBIWDEF int stbi_write_png_to_func(stbi_write_func *func, void *context, int w, int h, int comp, const void  *data, int stride_in_bytes);
STBIWDEF int stbi_write_bmp_to_func(stbi_write_func *func, void *context, int w, int h, int comp, const void  *data);
STBIWDEF int stbi_write_tga_to_func(stbi_write_func *func, void *context, int w, int h, int comp, const void  *data);
STBIWDEF int stbi_write_hdr_to_func(stbi_write_func *func, void *context, int w, int h, int comp, const float *data);

#ifdef __cplusplus
}
#endif

#endif//INCLUDE_STB_IMAGE_WRITE_H
