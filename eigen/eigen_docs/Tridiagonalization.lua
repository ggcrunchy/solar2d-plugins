--- A data type embodying a tridiagonalization.

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
-- @function Tridiagonalization:diagonal
-- @treturn Matrix D

--- DOCMEMORE
-- @function Tridiagonalization:generalizedSelfAdjointEigenSolver
-- @string? ("NoEigenvectors")
-- @treturn GeneralizedSelfAdjointEigenSolver G

--- DOCMEMORE
-- @function Tridiagonalization:householderCoefficients
-- @treturn Matrix C

--- DOCMEMORE
-- @function Tridiagonalization:matrixQ
-- @treturn Matrix Q

--- DOCMEMORE
-- @function Tridiagonalization:matrixT
-- @treturn Matrix T

--- DOCMEMORE
-- @function Tridiagonalization:packedMatrix
-- @treturn Matrix PM

--- DOCMEMORE
-- @function Tridiagonalization:selfAdjointEigenSolver
-- @string? ("NoEigenvectors")
-- @treturn SelfAdjointEigenSolver S

--- DOCMEMORE
-- @function Tridiagonalization:subDiagonal
-- @treturn Matrix D