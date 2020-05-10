--- Methods common to a few QR solvers and occuring elsewhere as well.

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

--- DOCME
-- @function QRExtensions:isInjective
-- @treturn boolean is

--- DOCME
-- @function QRExtensions:isInvertible
-- @treturn boolean is

--- DOCME
-- @function QRExtensions:isSurjective
-- @treturn boolean is

--- DOCME
-- @function QRExtensions:maxPivot
-- @treturn number mp

--- DOCME
-- @function QRExtensions:nonzeroPivots
-- @treturn uint n

--- DOCME
-- @function QRExtensions:rank
-- @treturn uint n

--- DOCME
-- @function QRExtensions:setThreshold
-- @tparam ?|number|string ("Default")
-- @treturn QRExtensions self

--- DOCME
-- @function QRExtensions:threshold
-- @treturn number t