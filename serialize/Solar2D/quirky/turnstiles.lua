--- Turnstile management for Quirky scene.

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

-- Modules --
local info = require("quirky.info")
local states = require("quirky.states")

-- Exports --
local M = {}

-- Helper to do 90-degree rotations relative to a pivot
local function Rotate (pcol, prow, tx, ty, cw)
	if cw then
		return pcol - ty, prow + tx
	else
		return pcol + ty, prow - tx
	end
end

-- Are the cells which the turnstile part must pass through open?
local function IsOpen (cells, index, middle, entry, pcol, prow, cw)
	for i = 1, cells.n, 3 do
		-- Begin by examining the corner this part must traverse en route to its rotated
		-- position. If said corner is not even on the level's grid, the rotation fails. We
		-- do not need this same test for the rotated position, since that will line up both
		-- with this corner and the pivot (already verified during load).
		local tx, ty = cells[i] - pcol, cells[i + 1] - prow
		local tcol, trow = Rotate(pcol, prow, tx, ty, cw)

		if info.OutOfBounds(tcol, trow) then
			return false
		end

		-- Check that the rotated position is either empty or contains another part of the
		-- same turnstile. The latter is fine since it would be rotating as well.
		local rentry = middle[info.GetIndex(tcol, trow)]
		local cindex = info.GetIndex(tcol + tx, trow + ty)
		local centry = cindex ~= index and middle[cindex]

		if centry or (rentry and rentry ~= entry) then
			return false
		end
	end

	return true
end

-- Infers how the turnstile is being rotated
local function GetOrientation (cells, index, pcol, prow, dx, dy)
	for i = 1, cells.n, 3 do
		-- Visit the cells until we find our corner. We then use a little trick, based on its
		-- relative position (tx, ty) to the pivot:
		--
		-- (-1, -1) | (+1, -1)
		-- -------------------
		-- (-1, +1) | (+1, +1)
		--
		-- With transformation tx - 2 * ty, these become:
		--
		-- +1 | +3
		-- -------
		-- -3 | -1
		--
		-- Since we have strictly horizontal or vertical movement, dx + dy will be negative
		-- when moving left or up, positive otherwise. Taking the product of this and the
		-- corner's result will give us a sense for our rotation: positive is clockwise,
		-- negative counter-clockwise. (It is also possible we are pushing against the
		-- turnstile from the side, in which case no motion results.)
		if cells[i + 2] == index then
			local tx, ty = cells[i] - pcol, cells[i + 1] - prow

			if tx == -dx and ty == -dy then
				return false
			else
				return true, (tx - 2 * ty) * (dx + dy) > 0 -- same sign = clockwise
			end
		end
	end

	return false
end

-- Can every part of the turnstile be moved?
function M.CanMove (state, entry, cells, index, dx, dy, di)
	local pcol, prow = info.GetPivot(entry)
	local can_move, cw = GetOrientation(cells, index, pcol, prow, dx, dy)

	if can_move and IsOpen(cells, index - di, state.middle, entry, pcol, prow, cw) then
		return true, cw
	else
		return false
	end
end

--
function M.Move (scene, state, entry, cells, cw)
	-- Clear any middle cells currently used by the turnstile.
	local middle = state.middle

	for i = 3, cells.n, 3 do
		middle[cells[i]] = false
	end

	-- Populate middle cells with the rotated turnstile. Increment the move count by the
	-- number of part tiles to ensure control remains disabled during the move.
	local pcol, prow = info.GetPivot(entry)

	for i = 1, cells.n, 3 do
		local tcol, trow = Rotate(pcol, prow, cells[i] - pcol, cells[i + 1] - prow, cw)

		middle[info.GetIndex(tcol, trow)] = entry

		info.IncMoveCount()
	end

	-- Rotate the turnstile's graphical components.
	states.MoveTurnstile(scene, entry, cw)
end

-- Export the module.
return M