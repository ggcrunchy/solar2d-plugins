--- A data type returned by various matrix predicate operations.
--
-- Inherits from [CommonWriteOps](./commonwriteops.html) and [XprOps](./xprops.html).

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

--- Are all entries true?
-- @function BoolMatrix:all
-- @param how (TODO: **"colwise"**, **"rowwise"**, or nothing)
-- @treturn[1] BoolMatrix A column or row vector with results for column or row (q.v. _how_).
-- @treturn[2] boolean Were all entries true?

--- Are any entries true?
-- @function BoolMatrix:any
-- @param how As per @{BoolMatrix:all}.
-- @treturn[1] BoolMatrix Ditto.
-- @treturn[2] boolean Was any entry true?

--- Boolean and of the two matrices.
-- @function BoolMatrix:band
-- @tparam BoolMatrix other
-- @treturn BoolMatrix res


--- Boolean or of the two matrices.
-- @function BoolMatrix:bor
-- @tparam BoolMatrix other
-- @treturn BoolMatrix res

--- Metamethod.
--
-- Fetch the coefficient at the given row and column (or offset for vectors).
-- @function BoolMatrix:__call
-- @uint _row_ Matrix row (or offset, in a vector).
-- @uint? _col_ Matrix column (must be present unless this is a vector).
-- @treturn boolean Coefficient.

--- Getter.
-- @function BoolMatrix:cols
-- @treturn uint Number of columns.

--- Getter.
-- @function BoolMatrix:count
-- @treturn uint Number of **true** coefficients. (TODO: should support "colwise", "rowwise") 

--- Metamethod.
--
-- Compares two BoolMatrix instances.
-- @function BoolMatrix:__eq
-- @treturn boolean Are the two matrices equal?

-- TODO: __len?

--- Getter.
-- @function BoolMatrix:rows
-- @treturn uint Number of rows.

--- Given two matrices, or a matrix-scalar pair (where the scalar is interpreted as a
-- constant matrix matching its partner in shape), goes through this matrix's coefficents,
-- building up a new output matrix. Any **true** coefficient results in the corresponding
-- entry coming from _then\_source_, otherwise from _else\_source_.
-- TODO: If both matrices, then and else must have same shape?
-- @function BoolMatrix:select
-- @tparam ?|Matrix|Scalar then_source
-- @tparam ?|Matrix|Scalar else_source
-- @treturn Matrix R

--- Getter.
-- @function BoolMatrix:size
-- @treturn uint Number of elements (rows &times; columns).

--- Metamethod.
--
-- Calls to `tostring` (and thus to `print`) will pretty-print this matrix.
-- @function BoolMatrix:__tostring