--- A data type embodying bidiagonal divide-and-conquer SVD operations.

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
-- @function BDCSVD:computeU
-- @treturn boolean CU

--- DOCMEMORE
-- @function BDCSVD:computeV
-- @treturn boolean CV

--- DOCMEMORE
-- @function BDCSVD:matrixU
-- @treturn Matrix U

--- DOCMEMORE
-- @function BDCSVD:matrixV
-- @treturn Matrix V

--- DOCMEMORE
-- @function BDCSVD:nonzeroSingularValues
-- @treturn uint n

--- DOCMEMORE
-- @function BDCSVD:rank
-- @treturn uint R

--- DOCMEMORE
-- @function BDCSVD:setThreshold
-- @tparam ?|number|string ("Default")
-- @treturn BDCSVD self

--- DOCMEMORE
-- @function BDCSVD:singularValues
-- @treturn Matrix SV

--- DOCMEMORE
-- @function BDCSVD:solve
-- @tparam Matrix B
-- @treturn Matrix X

--- DOCMEMORE
-- @function BDCSVD:threshold
-- @treturn number t