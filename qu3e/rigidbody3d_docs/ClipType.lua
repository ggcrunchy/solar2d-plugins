--- One of the following **string**s:
--
-- * **"Intersection"**: Create regions where both subject **and** clip polygons are filled.
-- * **"Union"**: Create regions where **either** subject **or** clip polygons (or both) are filled.
-- * **"Difference"**: Create regions where subject polygons are filled **except** where clip polygons are filled.
-- * **"Xor"**: Create regions where **either** subject **or** clip polygons are filled **but not** where **both** are filled.
--
-- All polygon clipping is performed with a @{Clipper} object with the specific boolean operation indicated by the
-- **ClipType** parameter passed in its @{Clipper:Execute|Execute} method.
--
-- With regard to **open** paths (polylines), clipping rules generally match those of closed paths (polygons).
--
-- However, when there are both polyline and polygon subjects, the following clipping rules apply:
--
-- * union operations - polylines will be clipped by any overlapping polygons so that non-overlapped portions will be
-- returned in the solution together with the union-ed polygons
-- * intersection, difference and xor operations - polylines will be clipped only by 'clip' polygons and there will be
-- no interaction between polylines and subject polygons.
--
-- See [here](http://www.angusj.com/delphi/clipper/documentation/Docs/Units/ClipperLib/Types/ClipType.htm) for
-- some visual examples.
--
-- @classmod ClipType
-- @see PolyFillType

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