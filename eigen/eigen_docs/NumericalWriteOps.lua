--- Subset of methods, common to Matrix and some expressions, that change the object via numerical operations.

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

--- DOCMEMORE
-- @function NumericalWriteOps:adjointInPlace

--- DOCMEMORE
-- @function NumericalWriteOps:addInPlace
-- @tparam Matrix Y
-- @string? vectorwise
-- @treturn NumericalWriteOps self

--- DOCMEMORE
-- @function NumericalWriteOps:coeffAddInPlace
-- @uint row
-- @uint[opt] col
-- @tparam Scalar X

--- DOCMEMORE
-- @function NumericalWriteOps:coeffDivInPlace
-- @uint row
-- @uint[opt] col
-- @tparam Scalar X

--- DOCMEMORE
-- @function NumericalWriteOps:coeffMulInPlace
-- @uint row
-- @uint[opt] col
-- @tparam Scalar X

--- DOCMEMORE
-- @function NumericalWriteOps:coeffSubInPlace
-- @uint row
-- @uint[opt] col
-- @tparam Scalar X

--- DOCMEMORE
-- @function NumericalWriteOps:normalize

--- DOCMEMORE
-- @function CommonWriteOps:setFromBytes
-- @tparam Bytes bytes
-- @treturn CommonWriteOps self

--- DOCMEMORE
-- @function NumericalWriteOps:setLinSpaced
-- @tparam Scalar low
-- @tparam Scalar high
-- @treturn NumericalWriteOps self

--- DOCMEMORE
-- @function NumericalWriteOps:stableNormalize

--- DOCMEMORE
-- @function NumericalWriteOps:subInPlace
-- @tparam Matrix Y
-- @string? vectorwise
-- @treturn NumericalWriteOps self