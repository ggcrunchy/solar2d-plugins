--- Methods common to all Cholesky solvers.

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
-- @function CholeskyBase:adjoint
-- @treturn CholeskyBase self

--- DOCMEMORE
-- @function CholeskyBase:info
-- @treturn string S

--- DOCMEMORE
-- @function CholeskyBase:matrixL
-- @treturn Matrix L

--- DOCMEMORE
-- @function CholeskyBase:matrixL
-- @treturn Matrix U

--- DOCMEMORE
-- @function CholeskyBase:rankUpdate
-- @tparam Matrix V
-- @number R
-- @treturn CholeskyBase self

--- DOCMEMORE
-- @function CholeskyBase:rcond
-- @treturn number C

--- DOCMEMORE
-- @function CholeskyBase:reconstructedMatrix
-- @treturn Matrix RM

--- DOCMEMORE
-- @function CholeskyBase:solve
-- @tparam Matrix B
-- @treturn Matrix X