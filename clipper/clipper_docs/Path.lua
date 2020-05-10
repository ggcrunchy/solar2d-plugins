--- [A sequence](http://www.angusj.com/delphi/clipper/documentation/Docs/Units/ClipperLib/Types/Path.htm) of
-- integer-valued points representing a single contour (see also [terminology](../api.html#terminology)).
--
-- Paths may be _open_ and represent a series of line segments bounded by 2 or more vertices, or they may be
-- _closed_ and represent polygons. Whether or not a path is open depends on context. _Closed_ paths may be
-- '_outer_' contours or '_hole_' contours. Which they are depends on @{core.Orientation|orientation}.
--
-- Multiple paths can be grouped into a @{Paths} structure.
-- @classmod Path
-- @see Clipper:AddPath, PolyTree

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

--- Add a point to the end of the path.
-- @function Path:AddPoint
-- @tparam cInt x Point's x-coordinate...
-- @tparam cInt y ...and y.

--- Remove all points from the path.
-- @function Path:Clear

--- Concatenate two or more **Path**s into an array, e.g. `local array = path1 .. path2 .. path3`.
-- @function Path:__concat
-- @treturn Paths Result of the concatenation.

--- 
-- @function Path:GetPoint
-- @uint index Index of point, from 1 to `#self`.
-- @treturn cInt x Point's x-coordinate...
-- @treturn cInt y ...and y.

---
-- @function Path:__len
-- @treturn uint Number of points in path.

---
-- @function Path:__tostring
-- @treturn string String representation of the path.