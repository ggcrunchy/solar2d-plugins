--- Class used intermediately by @{Packing}s, representing an array of rectangles.
-- @classmod RectArray

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
-- @function RectArray:__concat
-- @param ... Two or more @{RectArray} objects.
-- @treturn RectArray The union of the arrays, in order. These are copies, leaving the originals intact.

--- Method variant of @{RectArray:__len}.
-- @function RectArray:Concatenate
-- @tparam RectArray other Another array to append to this one.
-- @treturn RectArray The union of the arrays, _self_ followed by _other_. These are copies, leaving the originals intact.

--- Metamethod.
-- @function RectArray:__len
-- @treturn uint Number of rectangles in the array.

--- Predicate.
-- @function RectArray:WasPacked
-- @uint index Rect index, from 1 to `#self`.
-- @treturn boolean Has the rect been packed?