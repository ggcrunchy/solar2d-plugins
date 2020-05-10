---	Corona wrapper for **fipWinImage**, a class designed for MS Windows (TM) platforms.
--
-- **fipWinImage** provides methods used to:
--
-- * Display a DIB on the screen
-- * Copy / Paste a DIB to/from Windows devices (HANDLE, HBITMAP, Clipboard)
-- * Capture a window (HWND) and convert it to an image
--
-- **fipWinImage** derives from **fipImage** and unless otherwise noted may be treated as one.
-- @module fipWinImage

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

--- Copies one (Win) image to another.
-- @function fipWinImage:assign
-- @tparam fipWinImage image Image to assign.

--- Capture a window and convert it to an image.
-- @function fipWinImage:captureWindow
-- @tparam userdata hWndApplicationWindow Handle to the application main window. (TODO?)
-- @tparam userdata hWndSelectedWindow Handle to the window to be captured.
-- @treturn boolean Operation was successful?

--- Destroy image data.
-- @function fipWinImage:clear

--- Clipboard copy.
-- @function fipWinImage:copyToClipboard
-- @tparam userdata hWndNewOwner Handle to the window to be associated with the open clipboard. (TODO?)
-- @treturn boolean Operation was successful?

--- Get the tone mapping algorithm used for drawing, with its parameters.
-- @function fipWinImage:getToneMappingOperator
-- @treturn string Tone mapping operator, cf. @{enums.FREE_IMAGE_TMO}.
-- @treturn number First tone mapping algorithm parameter.
-- @treturn number Second tone mapping algorithm parameter
-- @treturn number Third tone mapping algorithm parameter
-- @treturn number Fourth tone mapping algorithm parameter

--- Indicates whether the image is valid for use.
-- @function fipWinImage:isValid
-- @treturn boolean The image is allocated?

--- Retrieves data from the clipboard.
-- @function fipWinImage:pasteFromClipboard
-- @treturn boolean Operation was successful?

--- Select a tone mapping algorithm used for drawing and set the image as modified so that the display will be refreshed.
-- @function fipWinImage:setToneMappingOperator
-- @string tmo Tone mapping operator, cf. @{enums.FREE_IMAGE_TMO}.
-- @number[opt=0] first First tone mapping algorithm parameter (algorithm dependant).
-- @number[opt=0] second Second tone mapping algorithm parameter (algorithm dependant).
-- @number[opt=1] third Third tone mapping algorithm parameter (algorithm dependant).
-- @number[opt=0] fourth Fourth tone mapping algorithm parameter (algorithm dependant).