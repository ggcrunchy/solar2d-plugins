--- [This structure](http://www.angusj.com/delphi/clipper/documentation/Docs/Units/ClipperLib/Types/Paths.htm) is fundamental to the Clipper Library. It's a
-- list or array of one or more @{Path} structures. (The Path structure contains an ordered list of vertices that make a single contour.)
--
-- Paths may be _open_ (a series of line segments), or they may be _closed_ (polygons). Whether or not a path is open depends on context. Closed paths may be
-- 'outer' contours or 'hole' contours.
-- Which they are depends on @{core.Orientation|orientation}.
-- @classmod Paths
-- @see Clipper:AddPath, Clipper:AddPaths

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

--- Adds a path (more specifically, a copy of it) to the end of the sequence.
-- @function Paths:AddPath
-- @tparam Path path

--- Remove all @{Path|paths} from the sequence.
-- @function Paths:Clear

---
-- @function Paths:GetPath
-- @uint index Index of path, from 1 to `#self`.
-- @ptable[opt] opts Options, which may include:
--
-- * **out**: If this is a @{Path}, it will be populated and used as the return value.
-- @treturn Path Copy of path.

---
-- @function Paths:__len
-- @treturn uint Number of paths.

---
-- @function Paths:__tostring
-- @treturn string String representation of group.