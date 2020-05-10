--- Grab components for curl effect's touch controls.

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
local min = math.min
local pairs = pairs

-- Modules --
local touch = require("plugin.pagecurl.touch")

-- Corona globals --
local display = display

-- Exports --
local M = {}

--- Given a direction, chooses and prepares the appropriate grab rect.
--
-- This is a no-op for already enabled rects.
-- @tparam TouchSet set
-- @tparam string dir The direction to assign.
function M.Choose (set, dir)
	local new = set[dir]
	local state = new.m_grab_state

	if state == "disabled" or state == "hidden" then
		new:addEventListener("touch", touch.Touch)
	end

	new.m_grab_state = "chosen"
end

--- Enables any new rects and disables defunct old choices.
-- @bool no_reset Keep the current curl state?
function M.CommitChoices (set, no_reset)
	for _, rect in pairs(set) do
		local state = rect.m_grab_state

		if state == "chosen" then
			rect.m_grab_state = "enabled"
		elseif state ~= "disabled" then
			touch.Reset(rect, no_reset)

			if state ~= "hidden" then
				rect:removeEventListener("touch", touch.Touch)
			end

			rect.m_grab_state = "disabled"
		end
	end
end

--- Gets the region belonging to the enabled grab rects.
-- @tparam TouchSet
-- @treturn table Table populated with enabled rects' regions.
function M.GetRegions (set)
	local regions = {}

	for _, rect in pairs(set) do
		if rect.m_grab_state ~= "disabled" then
			local bounds = rect.contentBounds
			local x1, y1, x2, y2 = bounds.xMin, bounds.yMin, bounds.xMax, bounds.yMax

			regions[rect.m_name] = { x = .5 * (x1 + x2), y = .5 * (y1 + y2), width = x2 - x1, height = y2 - y1 }
		end
	end

	return regions
end

-- Helper to add a new grab rect
local function AddRect (set, curl, dir, size, name)
	local grab = display.newRect(curl, 0, 0, 1, 1)

	set[dir], grab.m_name, grab.m_size, grab.m_grab_state = grab, name, size, "disabled"

	if dir == "bottom_to_top" or dir == "top_to_bottom" then
		grab.m_dir = dir == "bottom_to_top" and "B->T" or "T->B"
	else
		grab.m_dir = dir == "left_to_right" and "L->R" or "R->L"
	end

	grab.isHitTestable, grab.isVisible = true, false
end

-- When there are multiple grab rects enabled, hides those that were not grabbed
local function HideRest (event)
	local dir = event.dir

	for _, rect in pairs(event.target.m_touch) do
		if rect.m_grab_state == "enabled" and rect.m_name ~= dir then
			rect:removeEventListener("touch", touch.Touch)

			rect.m_grab_state = "hidden"
		end
	end
end

-- Restores hidden grab rects following a release
local function Restore (event)
	for _, rect in pairs(event.target.m_touch) do
		if rect.m_grab_state == "hidden" then
			rect:addEventListener("touch", touch.Touch)

			rect.m_grab_state = "enabled"
		end
	end
end

--- Makes a new set of touch controls.
-- @tparam PageCurlWidget curl
-- @uint size Size used by grab rects (along the edge, the extra amount; along the curl
-- direction, the dimension itself).
-- @treturn TouchSet Set used by other operations.
function M.MakeSet (curl, size)
	local set = {}

	AddRect(set, curl, "bottom_to_top", size, "bottom")
	AddRect(set, curl, "left_to_right", size, "left")
	AddRect(set, curl, "right_to_left", size, "right")
	AddRect(set, curl, "top_to_bottom", size, "top")

	curl:addEventListener("page_grabbed", HideRest)
	curl:addEventListener("page_released", Restore)

	return set
end

-- Gets a grab rect's region with respect to the reference object
local function GetRect (rect, ref_bounds)
	local dir, size, x, y, w, h = rect.m_dir, rect.m_size

	if dir == "B->T" or dir == "T->B" then
		local x1, x2 = ref_bounds.xMin, ref_bounds.xMax

		y = dir == "B->T" and ref_bounds.yMax or ref_bounds.yMin
		x, w, h = .5 * (x1 + x2), x2 - x1 + size, size
	else
		local y1, y2 = ref_bounds.yMin, ref_bounds.yMax

		x = dir == "L->R" and ref_bounds.xMin or ref_bounds.xMax
		y, w, h = .5 * (y1 + y2), size, y2 - y1 + size
	end

	return x, y, w, h
end

--- Positions and sizes the grab rects with respect to the curl object.
--
-- When unclipped, this object will be too large, and a stand-in is used. The object itself
-- suffices otherwise.
-- @tparam TouchSet set
-- @pobject ref Reference object, with "true" dimensions.
function M.PutOver (set, ref)
	local ref_bounds = ref.contentBounds

	for _, rect in pairs(set) do
		local x, y, w, h = GetRect(rect, ref_bounds)
		local gx, gy = rect.parent:contentToLocal(x, y)

		rect.x, rect.y, rect.width, rect.height = gx, gy, w, h

		rect.m_ref = ref
	end
end

-- Export the module.
return M