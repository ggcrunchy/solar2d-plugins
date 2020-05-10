--- Corona wrapper for **fipMemoryIO**, a memory handle class that allows you to load / save images from / to a memory stream.
-- @module fipMemoryIO

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

--- Provides a direct buffer access to a memory stream.
-- TODO: I suppose this is meant to be writeable...
-- NOTE: buggy if failed (returns too many results)
-- @function fipMemoryIO:acquire
-- @bool as_table Get the bytes as a table? (TODO: users can already just do string.byte()...)
-- @treturn boolean Acquire succeeded?
-- @treturn string Byte stream. (TODO: or table, but see other notes)

--- Free any allocated memory and invalidate the stream.
-- @function fipMemoryIO:close

--- Returns the buffer image format.
-- @function fipMemoryIO:getFileType
-- @treturn string Format, cf. @{enums.FREE_IMAGE_FORMAT}.

--- Indicates whether the memory handle is valid for use.
-- @function fipMemoryIO:isValid
-- @treturn boolean Internal memory buffer is a valid buffer?

--- Loads a dib from a memory stream.
-- @function fipMemoryIO:load
-- @string fif Format identifier, cf. @{enums.FREE_IMAGE_FORMAT}.
-- @tparam ?|{string,...}|string|nil flags Format-dependent @{flags}.
-- @treturn ?|userdata|nil On success, the loaded dib; otherwise **nil**. (TODO: More robust?)

--- Loads a multi-page bitmap from a memory stream.
-- NYI
-- @function fipMemoryIO:loadMultiPage

--- Reads data from a memory stream.
-- @function fipMemoryIO:read
-- @uint size Item size in bytes.
-- @uint count Maximum number of items to be read.
-- @treturn string Bytes read. (TODO: okay? not what FreeImageType does "Returns the number of full items actually read,
-- which may be less than count if an error occurs")

--- Saves a dib to a memory stream.
-- @function fipMemoryIO:save
-- @string fif Format identifier, cf. @{enums.FREE_IMAGE_FORMAT}.
-- @tparam userdata dib Dib to save. (TODO: More robust?)
-- @tparam ?|{string,...}|string|nil flags Format-dependent @{flags}.
-- @treturn boolean Save succeeded?

--- Saves a multi-page bitmap to a memory stream.
-- NYI
-- @function fipMemoryIO:saveMultiPage

--- Moves the memory pointer to a specified location.
-- @function fipMemoryIO:seek
-- @int offset
-- @int origin
-- @treturn boolean Seek succeeded?

--- Gets the current position of a memory pointer.
-- @function fipMemoryIO:tell
-- @treturn int Position.

--- Writes data to a memory stream.
-- @function fipMemoryIO:write
-- @string buffer Pointer to data to be written. (TODO: use __bytes protocol?)
-- @uint size Item size in bytes
-- @uint count Maximum number of items to be written.
-- @treturn uint The number of full items actually written, which may be less than count if an error occurs.