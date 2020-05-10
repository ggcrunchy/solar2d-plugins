--- Subset of methods common to BoolMatrix and Matrix proper, as well as some expressions, that
-- modify the object.

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
-- @function CommonWriteOps:assign
-- @tparam Matrix Y
-- @string? vectorwise
-- @treturn CommonWriteOps self

--- DOCMEMORE
-- @function CommonWriteOps:coeffAssign
-- @uint row
-- @uint[opt] col
-- @tparam Scalar X

--- DOCMEMORE
-- @function CommonWriteOps:fill
-- @tparam Scalar s

--- DOCMEMORE
-- @function CommonWriteOps:reverseInPlace

--- DOCMEMORE
-- @function CommonWriteOps:setConstant
-- @tparam Scalar s
-- @treturn CommonWriteOps self

--- DOCMEMORE
-- @function CommonWriteOps:setIdentity
-- @treturn CommonWriteOps self

--- DOCMEMORE
-- @function CommonWriteOps:setOnes
-- @treturn CommonWriteOps self

--- DOCMEMORE
-- @function CommonWriteOps:setRandom
-- @treturn CommonWriteOps self

--- DOCMEMORE
-- @function CommonWriteOps:setZero
-- @treturn CommonWriteOps self

--- DOCMEMORE
-- @function CommonWriteOps:swap
-- @tparam Matrix RHS

--- DOCMEMORE
-- @function CommonWriteOps:transposeInPlace