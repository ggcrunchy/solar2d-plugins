--- Touch controls used by the curl effect.

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
local abs = math.abs
local atan2 = math.atan2
local max = math.max
local min = math.min
local pi = math.pi

-- Modules --
local chebyshev = require("plugin.pagecurl.chebyshev")
local offsets = require("plugin.pagecurl.offsets")

-- Corona globals --
local display = display
local transition = transition

-- Exports --
local M = {}

-- Confines a number to [0, 1]
local function Clamp (x)
	return min(max(x, 0), 1)
end

-- Finds the normalized offset corresponding to a direction
local function FindOffset (pos, ref_bounds, dir)
	if dir == "R->L" then
		return (pos - ref_bounds.xMin) / (ref_bounds.xMax - ref_bounds.xMin)
	elseif dir == "L->R" then
		return (ref_bounds.xMax - pos) / (ref_bounds.xMax - ref_bounds.xMin)
	elseif dir == "B->T" then
		return (pos - ref_bounds.yMin) / (ref_bounds.yMax - ref_bounds.yMin)
	else
		return (ref_bounds.yMax - pos) / (ref_bounds.yMax - ref_bounds.yMin)
	end
end

-- Finds an normalized offset from an event
local function FindOffset_Event (event, ref_bounds, dir)
	local offset

	if dir == "L->R" or dir == "R->L" then
		offset = FindOffset(event.x, ref_bounds, dir)
	else
		offset = FindOffset(event.y, ref_bounds, dir)
	end

	return offset
end

-- Find the offsets perpendicular to the curl direction (alternatively, parallel to the edge)
local function FindPerpendicularOffsets (event, target, ref_bounds, dir)
	if dir == "L->R" or dir == "R->L" then
		dir = "B->T"
	else
		dir = "R->L"
	end

	return Clamp(FindOffset(target.m_start, ref_bounds, dir)), Clamp(FindOffset_Event(event, ref_bounds, dir))
end

-- Near-edge filter --
local Filter = chebyshev.NewFilter(24)

-- Gets the displacement away from the appropriate edge
local function GetDeltas (offset, start, frac_perp, dir)
	local factor, delta = Filter(offset), start - frac_perp

	offset, delta = offset - .00125 * factor^2, factor * delta

	if dir == "R->L" then
		return 1 - offset, delta
	elseif dir == "L->R" then
		return offset - 1, delta
	elseif dir == "B->T" then
		return delta, 1 - offset
	else
		return delta, offset - 1
	end
end

-- Gets the final position along the curl axis
local function GetXY (start, dx, dy, dir)
	if dir == "R->L" then
		return 1 - dx, start - dy
	elseif dir == "L->R" then
		return -dx, start - dy
	elseif dir == "B->T" then
		return start - dx, 1 - dy
	else
		return start - dx, -dy
	end
end

-- Has the touch moved enough to start curling?
local function HasMoved (event, dir)
	local flip, diff

	if dir == "L->R" or dir == "R->L" then
		diff, flip = event.x - event.xStart, dir == "R->L"
	else
		diff, flip = event.y - event.yStart, dir == "B->T"
	end

	return (flip and -diff or diff) >= 3
end

-- Unfurl transition --
local UnfurlParams = {}

-- Configure the unfurl transition
local function Unfurl (target, ref, dir)
	local ref_bounds = ref.contentBounds

	-- Aim for the base range, given the current direction, reversing the target angle if
	-- the transition would end up traversing more than 180 degrees.
	local old_angle, new_angle = target.parent.angle_radians

	new_angle, UnfurlParams.edge_x, UnfurlParams.edge_y = offsets.GetBaseRange(dir)

	if abs(new_angle - old_angle) > pi then
		new_angle = 2 * pi - new_angle
	end

	UnfurlParams.angle_radians = new_angle

	-- Aim for the offset along the edge where the touch began.
	if dir == "L->R" or dir == "R->L" then
		UnfurlParams.edge_y = FindOffset(target.m_start, ref_bounds, "B->T")
	else
		UnfurlParams.edge_x = FindOffset(target.m_start, ref_bounds, "R->L")
	end
end

-- Touch event data --
local TouchEvent = { name = "page_dragged" }

-- Touch event helper
local function DoTouchEvent (curl, what, dir)
	TouchEvent.target, TouchEvent.name, TouchEvent.dir = curl, what, dir

	curl:dispatchEvent(TouchEvent)

	TouchEvent.target = nil
end

--- Touch handler.
-- @ptable event Event data.
function M.Touch (event)
	local phase, target = event.phase, event.target
	local curl, ref, dir, name = target.parent, target.m_ref, target.m_dir, target.m_name

	-- Began --
	if phase == "began" then
		display.getCurrentStage():setFocus(target, event.id)

		-- Abort any unfurl in progress.
		if target.m_unfurling then
			transition.cancel(target.m_unfurling)

			target.m_unfurling = nil
		end

		-- Record the offset along the grabbed edge and enter a waiting-to-curl state.
		target.m_start, target.m_drag_state = (dir == "L->R" or dir == "R->L") and event.y or event.x, "started"

		-- Ensure that the touch handler begins with correct initial parameters.
		offsets.SetBaseParameters(curl, dir)

		-- Announce the grab to any observers.
		DoTouchEvent(curl, "page_grabbed", name)

	-- Moved --
	elseif phase == "moved" then
		local state = target.m_drag_state

		-- Wait until some distance has been traversed to start curling.
		if state == "started" then
			if HasMoved(event, dir) then
				target.m_drag_state = "ready"
			else
				return true
			end

		-- Ignore spurious move events.
		elseif state == nil then
			return
		end

		-- If the touch has been dragged beyond the edge, leave the page uncurled. Otherwise,
		-- use the position to calculate and update the curl parameters.
		local ref_bounds = ref.contentBounds
		local offset = FindOffset_Event(event, ref_bounds, dir)

		if offset < 1 then
			local start, frac_perp = FindPerpendicularOffsets(event, target, ref_bounds, dir)
			local dx, dy = GetDeltas(offset, start, frac_perp, dir)
			local t = offsets.FindDistance(dx, dy)

			offsets.SetParameters(curl, atan2(dy, dx) % (2 * pi), GetXY(start, dx * t, dy * t, dir))
		else
			offsets.SetBaseParameters(curl, dir)
		end

		-- Announce the drag to any observers.
		DoTouchEvent(curl, "page_dragged", name)

		-- Update shadow proxies...

	-- Ended / Cancelled --
	elseif phase == "ended" or phase == "cancelled" then
		display.getCurrentStage():setFocus(target, nil)

		-- Unfurl the page and leave the handler stateless.
		Unfurl(target, ref, dir)

		target.m_unfurling, target.m_drag_state = transition.to(curl, UnfurlParams)

		-- Announce the release to any observers.
		DoTouchEvent(curl, "page_released", name)
	end

	return true
end

--- Resets any touch in progress.
-- @pobject target Grab object.
function M.Reset (target, no_reset)
	display.getCurrentStage():setFocus(target, nil)

	if target.m_unfurling then
		transition.cancel(target.m_unfurling)
	end

	if not no_reset then
		offsets.SetBaseParameters(target.parent, target.m_dir)
	end

	target.m_unfurling, target.m_drag_state = nil
end

-- Export the module.
return M