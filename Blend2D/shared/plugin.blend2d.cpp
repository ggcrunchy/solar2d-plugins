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

#include "CoronaLua.h"
#include "common.h"
#include "blend2d.h"

int Return (lua_State * L, uint32_t result)
{
	if (BL_SUCCESS == result)
	{
		lua_pushboolean(L, 1); // ..., true

		return 1;
	}

	else
	{
		lua_pushnil(L); // ..., nil

		#define BL_ERROR(err, what) case BL_ERROR_##err: lua_pushliteral(L, what); break

		switch (result)
		{
		BL_ERROR(OUT_OF_MEMORY, "Out of memory [ENOMEM]."); // ..., nil, error (and so on for rest)
		BL_ERROR(INVALID_VALUE, "Invalid value/argument [EINVAL].");
		BL_ERROR(INVALID_STATE, "Invalid state [EFAULT].");
		BL_ERROR(INVALID_HANDLE, "Invalid handle or file [EBADF].");
		BL_ERROR(VALUE_TOO_LARGE, "Value too large [EOVERFLOW].");
		BL_ERROR(NOT_INITIALIZED, "Object not initialized.");
		BL_ERROR(NOT_IMPLEMENTED, "Not implemented [ENOSYS].");
		BL_ERROR(NOT_PERMITTED, "Operation not permitted [EPERM].");

		BL_ERROR(IO, "IO error [EIO].");
		BL_ERROR(BUSY, "Device or resource busy [EBUSY].");
		BL_ERROR(INTERRUPTED, "Operation interrupted [EINTR].");
		BL_ERROR(TRY_AGAIN, "Try again [EAGAIN].");
		BL_ERROR(TIMED_OUT, "Timed out [ETIMEDOUT].");
		BL_ERROR(BROKEN_PIPE, "Broken pipe [EPIPE].");
		BL_ERROR(INVALID_SEEK, "File is not seekable [ESPIPE].");
		BL_ERROR(SYMLINK_LOOP, "Too many levels of symlinks [ELOOP].");
		BL_ERROR(FILE_TOO_LARGE, "File is too large [EFBIG].");
		BL_ERROR(ALREADY_EXISTS, "File/directory already exists [EEXIST].");
		BL_ERROR(ACCESS_DENIED, "Access denied [EACCES].");
		BL_ERROR(MEDIA_CHANGED, "Media changed [Windows::ERROR_MEDIA_CHANGED].");
		BL_ERROR(READ_ONLY_FS, "The file/FS is read-only [EROFS].");
		BL_ERROR(NO_DEVICE, "Device doesn't exist [ENXIO].");
		BL_ERROR(NO_ENTRY, "Not found, no entry (fs) [ENOENT].");
		BL_ERROR(NO_MEDIA, "No media in drive/device [ENOMEDIUM].");
		BL_ERROR(NO_MORE_DATA, "No more data / end of file [ENODATA].");
		BL_ERROR(NO_MORE_FILES, "No more files [ENMFILE].");
		BL_ERROR(NO_SPACE_LEFT, "No space left on device [ENOSPC].");
		BL_ERROR(NOT_EMPTY, "Directory is not empty [ENOTEMPTY].");
		BL_ERROR(NOT_FILE, "Not a file  [EISDIR].");
		BL_ERROR(NOT_DIRECTORY, "Not a directory [ENOTDIR].");
		BL_ERROR(NOT_SAME_DEVICE, "Not same device [EXDEV].");
		BL_ERROR(NOT_BLOCK_DEVICE, "Not a block device [ENOTBLK].");

		BL_ERROR(INVALID_FILE_NAME, "File/path name is invalid [n/a].");
		BL_ERROR(FILE_NAME_TOO_LONG, "File/path name is too long [ENAMETOOLONG].");

		BL_ERROR(TOO_MANY_OPEN_FILES, "Too many open files [EMFILE].");
		BL_ERROR(TOO_MANY_OPEN_FILES_BY_OS, "Too many open files by OS [ENFILE].");
		BL_ERROR(TOO_MANY_LINKS, "Too many symbolic links on FS [EMLINK].");
		BL_ERROR(TOO_MANY_THREADS, "Too many threads [EAGAIN].");
		BL_ERROR(THREAD_POOL_EXHAUSTED, "Thread pool is exhausted and couldn't acquire the requested thread count.");

		BL_ERROR(FILE_EMPTY, "File is empty (not specific to any OS error).");
		BL_ERROR(OPEN_FAILED, "File open failed [Windows::ERROR_OPEN_FAILED].");
		BL_ERROR(NOT_ROOT_DEVICE, "Not a root device/directory   [Windows::ERROR_DIR_NOT_ROOT].");

		BL_ERROR(UNKNOWN_SYSTEM_ERROR, "Unknown system error that failed to translate to Blend2D result code.");

		BL_ERROR(INVALID_ALIGNMENT, "Invalid data alignment.");
		BL_ERROR(INVALID_SIGNATURE, "Invalid data signature or header.");
		BL_ERROR(INVALID_DATA, "Invalid or corrupted data.");
		BL_ERROR(INVALID_STRING, "Invalid string (invalid data of either UTF8, UTF16, or UTF32).");
		BL_ERROR(DATA_TRUNCATED, "Truncated data (more data required than memory/stream provides).");
		BL_ERROR(DATA_TOO_LARGE, "Input data too large to be processed.");
		BL_ERROR(DECOMPRESSION_FAILED, "Decompression failed due to invalid data (RLE, Huffman, etc).");

		BL_ERROR(INVALID_GEOMETRY, "Invalid geometry (invalid path data or shape).");
		BL_ERROR(NO_MATCHING_VERTEX, "Returned when there is no matching vertex in path data.");

		BL_ERROR(NO_MATCHING_COOKIE, "No matching cookie (BLContext).");
		BL_ERROR(NO_STATES_TO_RESTORE, "No states to restore (BLContext).");

		BL_ERROR(IMAGE_TOO_LARGE, "The size of the image is too large.");
		BL_ERROR(IMAGE_NO_MATCHING_CODEC, "Image codec for a required format doesn't exist.");
		BL_ERROR(IMAGE_UNKNOWN_FILE_FORMAT, "Unknown or invalid file format that cannot be read.");
		BL_ERROR(IMAGE_DECODER_NOT_PROVIDED, "Image codec doesn't support reading the file format.");
		BL_ERROR(IMAGE_ENCODER_NOT_PROVIDED, "Image codec doesn't support writing the file format.");

		BL_ERROR(PNG_MULTIPLE_IHDR, "Multiple IHDR chunks are not allowed (PNG).");
		BL_ERROR(PNG_INVALID_IDAT, "Invalid IDAT chunk (PNG).");
		BL_ERROR(PNG_INVALID_IEND, "Invalid IEND chunk (PNG).");
		BL_ERROR(PNG_INVALID_PLTE, "Invalid PLTE chunk (PNG).");
		BL_ERROR(PNG_INVALID_TRNS, "Invalid tRNS chunk (PNG).");
		BL_ERROR(PNG_INVALID_FILTER, "Invalid filter type (PNG).");

		BL_ERROR(JPEG_UNSUPPORTED_FEATURE, "Unsupported feature (JPEG).");
		BL_ERROR(JPEG_INVALID_SOS, "Invalid SOS marker or header (JPEG).");
		BL_ERROR(JPEG_INVALID_SOF, "Invalid SOF marker (JPEG).");
		BL_ERROR(JPEG_MULTIPLE_SOF, "Multiple SOF markers (JPEG).");
		BL_ERROR(JPEG_UNSUPPORTED_SOF, "Unsupported SOF marker (JPEG).");

		BL_ERROR(FONT_NOT_INITIALIZED, "Font doesn't have any data as it's not initialized.");
		BL_ERROR(FONT_NO_MATCH, "Font or font-face was not matched (BLFontManager).");
		BL_ERROR(FONT_NO_CHARACTER_MAPPING, "Font has no character to glyph mapping data.");
		BL_ERROR(FONT_MISSING_IMPORTANT_TABLE, "Font has missing an important table.");
		BL_ERROR(FONT_FEATURE_NOT_AVAILABLE, "Font feature is not available.");
		BL_ERROR(FONT_CFF_INVALID_DATA, "Font has an invalid CFF data.");
		BL_ERROR(FONT_PROGRAM_TERMINATED, "Font program terminated because the execution reached the limit.");

		BL_ERROR(INVALID_GLYPH, "Invalid glyph identifier.");
		}

		#undef BL_ERROR

		return 2;
	}
}

CORONA_PUBLIC CORONA_EXPORT int luaopen_plugin_blend2d (lua_State * L) CORONA_PUBLIC_SUFFIX
{
	lua_newtable(L);// blend2d

    luaL_Reg classes[] = {
		{ "codec", add_codec },
		{ "context", add_context },
		{ "font", add_font },
		{ "fontface", add_fontface },
		{ "glyphbuffer", add_glyphbuffer },
		{ "gradient", add_gradient },
		{ "image", add_image },
		{ "path", add_path },
		{ "pattern", add_pattern },
		{ nullptr, nullptr }
	};

	for (int i = 0; classes[i].func; ++i)
	{
		lua_pushcfunction(L, classes[i].func);	// blend2d, func
		lua_call(L, 0, 1);	// blend2d, class
		lua_setfield(L, -2, classes[i].name);	// blend2d = { ..., name = class }
	}

	return 1;
}
