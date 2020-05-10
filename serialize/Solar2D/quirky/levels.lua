--- Level management for Quirky scene.

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
local assert = assert
local error = error
local ipairs = ipairs
local pairs = pairs
local random = math.random
local setfenv = setfenv
local tostring = tostring

-- Modules --
local info = require("quirky.info")
local pieces = require("quirky.pieces")

-- Exports --
local M = {}

-- Imports --
local EmptySpot = pieces.EmptySpot

-- Unloads any old level and loads a new one
function M.Load (level)
	local w, h = level.width, level.height

	-- Reset the level's fixed info.
	info.Clear()
	info.SetDims(w, h)

	-- Prepare a fresh game state.
	local characters, ground, middle = {}, {}, {}
	local state = { ground = ground, middle = middle, characters = characters, who = 1 }

	-- Begin with default values.
	for i = 1, w * h do
		ground[i], middle[i] = false, false
	end

	-- Iterate the grid and interpret each of the pieces.
	local color_set, pivots, index = {}, {}, 1

	for row = 1, h do
		for col = 1, w do
			local spot = level[index] or EmptySpot
			local gentry, mentry = spot[1], spot[2]

			-- Exits, holes, and Sokoban-style spots are carried over intact.
			if gentry == "E" or gentry == "H" or gentry == "S" then
				ground[index] = gentry

			-- Unhandled ground entry: error out.
			elseif gentry then
				error("Unknown ground entry in level layout! " .. tostring(gentry))
			end

			-- Add a new character, assigning a unique ID.
			if mentry == "C" then
				local id = #characters + 1

				characters[id], middle[index], color_set[id] = { col = col, row = row, id = id }, "C", true

			-- Walls carry over intact too.
			elseif mentry == "W" then
				middle[index] = mentry

			-- Handle other middle entries that have an ID.
			elseif mentry then
				local first = mentry:sub(1, 1)

				-- Blocks and turnstile parts carry over, but are given an ID-specific color too.
				if first == "B" or first == "T" then
					middle[index], color_set[mentry] = mentry, true

				-- Carry over pivots and make note of them being found.
				elseif first == "P" then
					assert(not pivots[mentry], "Duplicate pivot in level layout! " .. mentry)

					middle[index], pivots[mentry] = mentry, index

				-- Unhandled middle entry: error out.
				else
					error("Unknown middle entry in level layout! " .. tostring(mentry))
				end
			end

			index = index + 1
		end
	end

	-- Randomly generate some colors.
	for k in pairs(color_set) do
		color_set[k] = { r = random(), g = .3 + random() * .7, b = random() }
	end

	info.SetColors(color_set)

	-- TODO: Check if any blocks already plug holes? (Just convert to floor? Any use?)

	-- Do some integrity checking for turnstiles.
	for k, pi in pairs(pivots) do
		local id, ti = "T" .. k:sub(2), 1

		for _ = 1, h do
			for col = 1, w do
				if middle[ti] == id then
					local ok = pi - w == ti or pi + w == ti

					ok = ok or (col > 1 and pi - 1 == ti)
					ok = ok or (col < w and pi + 1 == ti)

					assert(ok, "Turnstile part outside pivot's `+` in level layout! " .. id)

					info.AddPivot(id, pi)

					pivots[k] = false
				end

				ti = ti + 1
			end
		end

		assert(not pivots[k], "No turnstile part corresponds to pivot in level layout! " .. k)
	end

	return state
end

-- Register the level list, running the loader in another environment so pieces may be read as "globals"
function M.Register (get_levels)
	setfenv(get_levels, pieces)

	local list = {}

	for _, layout in ipairs(get_levels()) do
		list[#list + 1] = layout
	end

	return list
end

-- Export the module.
return M