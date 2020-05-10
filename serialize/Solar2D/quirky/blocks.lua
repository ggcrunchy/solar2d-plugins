--- Block management for Quirky scene.

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

-- Can every part of the block be moved?
function M.CanMove (state, entry, cells, dx, dy, di)
	local middle = state.middle

	for i = 1, cells.n, 3 do
		local bcol, brow, bi = cells[i], cells[i + 1], cells[i + 2]

		if info.OutOfBounds(bcol + dx, brow + dy) then
			return false
		end

		-- An adjacent spot is fine if it happens to be either empty or contain another
		-- part of the block. The latter is fine since that would be moving too.
		local ba = middle[bi + di]

		if ba and ba ~= entry then
			return false
		end
	end

	return true
end

-- Sokoban spot accumulator --
local Spots = {}

-- Helper to gather any Sokoban-style spots
local function CoveredSpots (ground, middle)
	states.Find("S", ground, Spots)

	for i = 3, Spots.n, 3 do
		local mentry = middle[Spots[i]]

		if not mentry or mentry:sub(1, 1) ~= "B" then
			return false
		end
	end

	return Spots.n > 0
end

-- Is the block totally contained by the hole?
local function FilledHole (ground, cells, di)
	for i = 3, cells.n, 3 do
		if ground[cells[i] + di] ~= "H" then
			return false
		end
	end

	return true
end

-- Event sent when all Sokoban-style spots are covered --
local CoveredEvent = { name = "covered" }

-- Event sent when part of a hole is plugged --
local FilledEvent = { name = "filled" }

-- Move the block
function M.Move (scene, state, entry, cells, dx, dy, di)
	-- Clear any middle cells currently used by the block.
	local ground, middle = state.ground, state.middle

	for i = 3, cells.n, 3 do
		middle[cells[i]] = false
	end

	-- Check if the block should sink into a hole. If so, report it.
	local filled = FilledHole(ground, cells, di)

	if filled then
		FilledEvent.entry = entry

		info.GetEvents():dispatchEvent(FilledEvent)
	end

	-- Populate middle cells with the moved block. If the block filled a hole, replace ground
	-- entries (with a special tag, so that rewinds pick it up), otherwise middle ones. In
	-- either case, increment the move count by the number of block tiles to ensure control
	-- remains disabled during the move.
	local into, ientry = filled and ground or middle, filled and "F" or entry

	for i = 3, cells.n, 3 do
		into[cells[i] + di] = ientry

		info.IncMoveCount()
	end

	-- The block might have covered up some Sokoban-style spots. If so, report in the case
	-- that all spots in the level are now underneath blocks (whether this one or others).
	if not filled and CoveredSpots(ground, middle) then
		info.GetEvents():dispatchEvent(CoveredEvent)
	end

	-- Move the block's graphical components.
	states.MoveBlock(scene, entry, dx, dy)
end

-- Export the module.
return M