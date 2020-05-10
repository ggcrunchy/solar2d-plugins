--- Subset of methods, common to BoolMatrix and Matrix, that create expression objects.
--
-- These operations are also provided to expression themselves, though in this case the
-- expression is copied to a new matrix and an expression is built from that.

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
-- @function MatrixXprOps:block
-- @uint a
-- @uint b
-- @uint c
-- @uint d
-- @treturn Block x

--- DOCMEMORE
-- @function MatrixXprOps:bottomLeftCorner
-- @uint nrows
-- @uint ncols
-- @treturn Block x

--- DOCMEMORE
-- @function MatrixXprOps:bottomRightCorner
-- @uint nrows
-- @uint ncols
-- @treturn Block x

--- DOCMEMORE
-- @function MatrixXprOps:bottomRows
-- @uint nrows
-- @treturn Block x

--- DOCMEMORE
-- @function MatrixXprOps:col
-- @uint col
-- @treturn Block S

--- DOCMEMORE
-- @function MatrixXprOps:diagonal
-- @uint? index
-- @treturn Diagonal D

--- DOCMEMORE
-- @function MatrixXprOps:head
-- @uint n
-- @treturn VectorBlock S

--- DOCMEMORE
-- @function MatrixXprOps:leftCols
-- @uint ncols
-- @treturn Block x

--- DOCMEMORE
-- @function MatrixXprOps:middleCols
-- @uint c1
-- @uint c2
-- @treturn Block x

--- DOCMEMORE
-- @function MatrixXprOps:middleRows
-- @uint r1
-- @uint r2
-- @treturn Block x

--- DOCMEMORE
-- @function MatrixXprOps:rightCols
-- @uint ncols
-- @treturn Block x

--- DOCMEMORE
-- @function MatrixXprOps:row
-- @uint row
-- @treturn Block S

--- DOCMEMORE
-- @function MatrixXprOps:segment
-- @uint pos
-- @uint n
-- @treturn VectorBlock S

--- DOCMEMORE
-- @function MatrixXprOps:tail
-- @uint n
-- @treturn VectorBlock S

--- DOCMEMORE
-- @function MatrixXprOps:topLeftCorner
-- @uint nrows
-- @uint ncols
-- @treturn Block x

--- DOCMEMORE
-- @function MatrixXprOps:topRightCorner
-- @uint nrows
-- @uint ncols
-- @treturn Block x

--- DOCMEMORE
-- @function MatrixXprOps:topRows
-- @uint nrows
-- @treturn Block x