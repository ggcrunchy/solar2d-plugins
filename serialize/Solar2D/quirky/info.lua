--- Some shared info for Quirky scene.

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
local floor = math.floor

-- Corona globals --
local system = system

-- Exports --
local M = {}

-- Map of turnstile entries to fixed pivot coordinate / index info --
local Pivots

-- Level dimensions --
local W, H

-- Unpack some pivot info for easy lookup
function M.AddPivot (tname, index)
	local x = (index - 1) % W + 1

	Pivots[tname] = {
		x = x,
		y = (index - x) / W + 1,
		index = index
	}
end

-- List of dynamic colors --
local Colors

-- Clear the level's fixed info
function M.Clear ()
	Colors, Pivots = {}, {}
end

-- Various events used by Quirky --
local Events = system.newEventDispatcher()

-- Number of items in motion --
local MoveN = 0

-- Event fired when the last moving object settles --
local DoneMovingEvent = { name = "done_moving" } 

-- Indicate that one object has stopped moving, firing an event on the last one
function M.DecMoveCount ()
	if MoveN > 0 then
		MoveN = MoveN - 1

		if MoveN == 0 then
			Events:dispatchEvent(DoneMovingEvent)
		end
	end
end

-- Get the first display object in a layer whose type matches the given entry
function M.FindFirstObject (layer, entry)
	for i = 1, layer.numChildren do
		if layer[i].m_type == entry then
			return layer[i]
		end
	end
end

-- Perform an action on each display object in a layer whose type matches the given entry
function M.ForEachObject (layer, entry, func, arg1, arg2)
	for i = 1, layer.numChildren do
		local object = layer[i]

		if object.m_type == entry then
			func(object, arg1, arg2)
		end
	end
end

-- Get the current level's color with the given name
function M.GetColor (name)
	return Colors[name]
end

-- Get the current level's dimensions
function M.GetDims ()
	return W, H
end

-- Gets Quirky's event dispatcher
function M.GetEvents ()
	return Events
end

-- Resolve a cell in the level's grid to a flat index
function M.GetIndex (col, row)
	return (row - 1) * W + col
end

-- Get how many objects are still in motion
function M.GetMoveCount ()
	return MoveN
end

-- Get the pivot info corresponding to a turnstile part
function M.GetPivot (tname)
	local pivot = Pivots[tname]

	return pivot.x, pivot.y, pivot.index
end

-- Get the current level's step deltas in the given direction
function M.GetStep (dir)
	if dir == "left" then
		return -1, 0, -1
	elseif dir == "right" then
		return 1, 0, 1
	elseif dir == "up" then
		return 0, -1, -W
	elseif dir == "down" then
		return 0, 1, W
	end
end

-- Indicate that one object has begun moving
function M.IncMoveCount ()
	MoveN = MoveN + 1
end

-- Is a given cell inside the level's grid?
function M.OutOfBounds (col, row)
	return col < 1 or col > W or row < 1 or row > H
end

-- Assign a name -> color set to the current level
function M.SetColors (colors)
	Colors = colors
end

-- Set the current level's dimensions
function M.SetDims (w, h)
	W, H = w, h
end

-- Export module.
return M