--- This module is motivated by the fact that, when an array's elements are required to be
-- non-numbers, said elements may be removed without disturbing the positions of elements
-- elsewhere in the array, by stuffing an integer into the vacated slot.
--
-- Furthermore, these same integers can be used to maintain a free list, thus providing O(1)
-- retrieval of free array slots.

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

-- Standard library imports --
local type = type

-- Exports --
local M = {}

--- DOCME
-- @array arr
-- @uint[opt] free
-- @treturn uint X
-- @treturn uint F
function M.GetInsertIndex (arr, free)
	if free and free > 0 then
		return free, arr[free]
	else
		return #arr + 1, free or 0
	end
end

--- DOCME
-- @array arr
-- @int index
-- @treturn boolean B
function M.InUse (arr, index)
	-- Disregard non-array indices and invalid slots. To streamline the test, treat these
	-- cases as though a number was found in the array part.
	local elem = index > 0 and arr[index] or 0

	-- The stack consists of numbers; conversely, non-numbers are in use.
	return type(elem) ~= "number"
end

--- DOCME
-- @array arr
-- @int index
-- @uint free
-- @treturn uint X
function M.RemoveAt (arr, index, free)
	local n = #arr

	-- Final slot: trim the array.
	if index == n then
		n, arr[index] = n - 1

		-- It may be possible to trim more: if the new final slot also happens to be
		-- the free stack top, it is known to not be in use. Trim the array until this
		-- is no longer the case (which may mean the free stack is empty).
		while n > 0 and n == free do
			free, n, arr[n] = arr[n], n - 1
		end

	-- Otherwise, the removed slot becomes the free stack top.
	elseif index >= 1 and index < n then
		arr[index], free = free, index
	end

	-- Adjust the free stack top.
	return free
end

-- Export the module.
return M