--- A triangular view of a matrix.

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
-- @function TriangularView:adjoint
-- @treturn Adjoint a

--- DOCME
-- @function TriangularView:asMatrix
-- @treturn Matrix M

--- DOCME
-- @function TriangularView:assign
-- @tparam ?|TriangularView|Matrix R

--- DOCME
-- @function TriangularView:__call
-- @uint row
-- @uint col
-- @treturn Scalar S

--- DOCME
-- @function TriangularView:coeffAssign
-- @uint row
-- @uint col
-- @tparam Scalar S

--- DOCME
-- @function TriangularView:cols
-- @tparam uint c

--- DOCME
-- @function TriangularView:conjugate
-- @treturn Conjugate c

--- DOCME
-- @function TriangularView:determinant
-- @treturn number D

--- DOCME
-- @function TriangularView:fill
-- @tparam Scalar S

--- DOCME
-- @function TriangularView:innerStride
-- @treturn uint s

--- DOCME
-- @function TriangularView:__mul
-- @tparam ?|TriangularView|Adjoint|Conjugate|Tranpose|Matrix R (may be param 1 and / or 2)
-- @treturn Matrix prod

--- DOCME
-- @function TriangularView:outerStride
-- @treturn uint s

--- DOCME
-- @function TriangularView:rows
-- @tparam uint r

--- DOCME
-- @function TriangularView:selfadjointView
-- @string S ("Lower", "Upper")
-- @treturn SelfAdjointView SAV

--- DOCME
-- @function TriangularView:setConstant
-- @tparam Scalar S
-- @return Self, for chaining methods.

--- DOCME
-- @function TriangularView:setOnes
-- @return Self, for chaining methods.

--- DOCME
-- @function TriangularView:setZero
-- @return Self, for chaining methods.

--- DOCME
-- @function TriangularView:solve
-- @tparam Matrix B
-- @string? S ("OnTheRight")
-- @treturn Matrix X

--- DOCME
-- @function TriangularView:solveInPlace
-- @tparam Matrix B
-- @string? S ("OnTheRight")

--- DOCME
-- @function TriangularView:tranpose
-- @treturn Transpose tt