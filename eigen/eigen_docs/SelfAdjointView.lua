--- A self-adjoint view of a matrix.

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
-- @function SelfAdjointView:adjoint
-- @treturn Adjoint a

--- DOCME
-- @function SelfAdjointView:asMatrix
-- @treturn Matrix M

--- DOCME
-- @function SelfAdjointView:assign
-- @tparam ?|SelfAdjointView|Matrix R

--- DOCME
-- @function SelfAdjointView:__call
-- @uint row
-- @uint col
-- @treturn Scalar S

--- DOCME
-- @function SelfAdjointView:coeffAssign
-- @uint row
-- @uint col
-- @tparam Scalar S

--- DOCME
-- @function SelfAdjointView:cols
-- @tparam uint c

--- DOCME
-- @function SelfAdjointView:conjugate
-- @treturn Conjugate c

--- DOCME
-- @function SelfAdjointView:diagonal
-- @treturn Matrix D

--- DOCME
-- @function SelfAdjointView:eigenvalues
-- @treturn Matrix E

--- DOCME
-- @function SelfAdjointView:innerStride
-- @treturn uint s

--- DOCME
-- @function SelfAdjointView:ldlt
-- @treturn LDLT ldlt

--- DOCME
-- @function SelfAdjointView:llt
-- @treturn LLT llt

--- DOCME
-- @function SelfAdjointView:__mul
-- @tparam ?|SelfAdjointView|Adjoint|Conjugate|Tranpose|Matrix R (may be param 1 and / or 2)
-- @treturn Matrix prod

--- DOCME
-- @function SelfAdjointView:operatorNorm
-- @treturn Matrix ON

--- DOCME
-- @function SelfAdjointView:outerStride
-- @treturn uint s

--- DOCME
-- @function SelfAdjointView:rankUpdate
-- @tparam Matrix U
-- @tparam[opt] Matrix V
-- @tparam Scalar S
-- @return Self, for method chaining.

--- DOCME
-- @function SelfAdjointView:rows
-- @tparam uint r

--- DOCME
-- @function SelfAdjointView:tranpose
-- @treturn Transpose st

--- DOCME
-- @function SelfAdjointView:triangularView
-- @string S ("Lower", "StrictlyLower", "StrictlyUpper", "UnitLower", "UnitUpper", "Upper")
-- @treturn TriangularView TV