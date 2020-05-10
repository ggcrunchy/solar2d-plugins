--- The Eigen plugin is divided into five namespaces, according to the underlying data type:
-- **int** (signed integers), **float** (single-precision float), **double** (double-precision
-- float), **cfloat** (single-precision complex number), and **cdouble** (double-precision
-- complex number).
--
-- Each namespace has the assortment of functions listed below (including many matrix factories),
-- but the Matrix involved will be made up of whatever type belongs to the given namespace.

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

--- DOCMEMORE: Make a constant matrix
-- @function Constant
-- @uint nrows X
-- @uint[opt=nrows] ncols Y
-- @treturn Matrix C

--- DOCMEMORE: Make an identity matrix
-- @function Identity
-- @uint nrows X
-- @uint[opt=nrows] ncols Y
-- @treturn Matrix I

--- DOCMEMORE: Make a linear-spaced vector
-- @function LinSpaced
-- @treturn Matrix V

--- DOCMEMORE: Make a linear-spaced row vector
-- @function LinSpacedRow
-- @treturn Matrix V

--- DOCMEMORE: Map memory as a matrix
-- @function MatrixFromMemory
-- @tparam ?|MemoryBlob|string memory
-- @uint nrows X
-- @uint[opt=nrows] ncols Y
-- @treturn Map M

--- DOCMEMORE: Map memory as a matrix, with inner stride
-- @function MatrixFromMemoryWithInnerStride
-- @tparam ?|MemoryBlob|string memory
-- @uint nrows X
-- @uint[opt=nrows] ncols Y
-- @uint stride
-- @treturn Map M

--- DOCMEMORE: Map memory as a matrix, with outer stride
-- @function MatrixFromMemoryWithOuterStride
-- @tparam ?|MemoryBlob|string memory
-- @uint nrows X
-- @uint[opt=nrows] ncols Y
-- @uint stride
-- @treturn Map M

--- DOCMEMORE: Make a matrix
-- @function NewMatrix
-- @uint? nrows X
-- @uint[opt=nrows] ncols Y
-- @treturn Matrix M

--- DOCMEMORE: Make an all-ones matrix
-- @function Ones
-- @uint nrows X
-- @uint[opt=nrows] ncols Y
-- @treturn Matrix O

--- DOCMEMORE: Make a random matrix
-- @function Random
-- @uint nrows X
-- @uint[opt=nrows] ncols Y
-- @treturn Matrix R

--- DOCMEMORE: Make a random permutation matrix
-- @function RandomPermutation
-- @uint n
-- @treturn Matrix R

--- DOCMEMORE: Make a row vector
-- @function RowVector
-- @uint size X
-- @treturn Matrix RV

-- DOCMEMORE
-- @function Umeyama
-- @tparam Matrix F
-- @tparam Matrix T
-- @string? S ("no_scaling")
-- @treturn Matrix U

--- DOCMEMORE: Make a (column) vector
-- @function Vector
-- @uint size X
-- @treturn Matrix V

--- DOCMEMORE: Make an all-zeroes matrix
-- @function Zero
-- @uint nrows X
-- @uint[opt=nrows] ncols Y
-- @treturn Matrix Z