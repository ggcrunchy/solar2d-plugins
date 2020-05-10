--- Obstacle management for Quirky scene.

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
local blocks = require("quirky.blocks")
local info = require("quirky.info")
local states = require("quirky.states")
local turnstiles = require("quirky.turnstiles")

-- Exports --
local M = {}

-- Can the spot be occupied, and if so must an obstacle be moved?
function M.CanMove (state, cells, col, row, dir)
	local dx, dy, di = info.GetStep(dir)

	-- The move trivially fails if it would go off the level's grid.
	if info.OutOfBounds(col + dx, row + dy) then
		return false
	end

	-- On the ground, check if a hole is in the way.
	local index = info.GetIndex(col, row) + di

	if state.ground[index] == "H" then
		return false
	end

	-- At this point, the way is open when there is no middle entry, whereas a character or
	-- wall there blocks it. Failing that, we have an ID'd object of some sort.
	local middle = state.middle
	local mentry = middle[index]

	if not mentry then
		return "open"
	elseif mentry == "C" or mentry == "W" then
		return false
	else
		local what = mentry:sub(1, 1)

		-- Pivots are fixed, so they block the way.
		if what == "P" then
			return false

		-- Otherwise, check if the block or turnstile can be moved. If so, gather some
		-- details about the move and return them.
		elseif what == "B" or what == "T" then
			states.Find(mentry, state.middle, cells)

			if what == "B" then
				return blocks.CanMove(state, mentry, cells, dx, dy, di) and "block", mentry
			else
				local can_move, cw = turnstiles.CanMove(state, mentry, cells, index, dx, dy, di)

				return can_move and "turnstile", mentry, cw
			end
		end
	end
end

-- Export the module.
return M