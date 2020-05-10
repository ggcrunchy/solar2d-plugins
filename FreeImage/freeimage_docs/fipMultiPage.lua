--- Corona wrapper for **fipMultiPage**, a multi-page file stream.
--
-- **fipMultiPage** encapsulates the multi-page API. It supports reading/writing multi-page TIFF, ICO and GIF files.
-- @module fipMultiPage

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

--- Appends a new page to the end of the bitmap.
-- @function fipMultiPage:appendPage
-- @tparam fipImage image Image to append.

--- Close a file stream.
-- TODO: flags not quite right
-- @function fipMultiPage:close
-- @tparam ?|{string,...}|string|nil flags Format-dependent (save) @{flags}.
-- @treturn boolean Operation was successful?

--- Deletes the page on the given position.
-- @function fipMultiPage:deletePage
-- @int page Page number.

--- Returns an array of page-numbers that are currently locked in memory.
-- @function fipMultiPage:getLockedPageNumbers
-- @treturn boolean Operation was successful?
-- @treturn {int,...} On success, array of page numbers.

--- Returns the number of pages currently available in the multi-paged bitmap.
-- @function fipMultiPage:getPageCount
-- @treturn int Count.

--- Inserts a new page before the given position in the bitmap.
-- @function fipMultiPage:insertPage
-- @int page Page number. Page has to be a number smaller than the current number of pages available in the bitmap.
-- @tparam fipImage image Image to insert.

--- Indicates whether the multi-page is valid for use.
-- @function fipMultiPage:isValid
-- @treturn boolean The multi-page stream is opened?

--- Locks a page in memory for editing.
--
-- You must call @{fipMultiPage:unlockPage} to free the page.
--
-- Usage: (TODO: Lua)
--
--     fipMultiPage mpage;
--     -- ...
--     -- fipImage image; -- You must declare this before
--     image = mpage.lockPage(2);
--     if(image.isValid()) {
--       ...
--       mpage.unlockPage(image, TRUE);
--     }
-- @function fipMultiPage:lockPage
-- @int page Page number.
-- @treturn ?|userdata|nil On success, the page; otherwise **nil**. (TODO?)

--- Moves the source page to the position of the target page.
-- @function fipMultiPage:movePage
-- @int target Target page position.
-- @int source Source page position.
-- @treturn boolean Operation was successful?

--- Open a multi-page file.
-- @function fipMultiPage:open
-- @tparam string name Name of the multi-page bitmap file.
-- @bool create_new When true, it means that a new bitmap will be created rather than an existing one being opened.
-- @bool read_only When true the bitmap is opened read-only.
-- @tparam ?|{string,...}|string|nil flags Format-dependent @{flags}.
-- @treturn boolean Operation was successful?

--- Open a multi-page memory stream as read / write.
-- @function fipMultiPage:open
-- @tparam fipMemoryIO memIO Memory stream. The memory stream MUST BE a wrapped user buffer.
-- @tparam ?|{string,...}|string|nil flags Format-dependent @{flags}.
-- @treturn boolean Operation was successful?

--- Saves a multi-page image using the specified memory stream and an optional flag.
-- @function fipMultiPage:saveToMemory
-- @tparam fipMemoryIO memIO
-- @tparam ?|{string,...}|string|nil flags Format-dependent @{flags}.
-- @treturn boolean Operation was successful?

--- Unlocks a previously locked page and gives it back to the multi-page engine.
-- @function fipMultiPage:unlockPage
-- @tparam fipImage image Page to unlock.
-- @bool changed When true, the page is marked changed and the new page data is applied in the multi-page bitmap.