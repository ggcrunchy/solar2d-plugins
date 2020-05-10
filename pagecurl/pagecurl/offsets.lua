--- Some offset-related operations involved in curl effect.

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
local pi = math.pi
local sin = math.sin
local sqrt = math.sqrt

-- Modules --
local constants = require("plugin.pagecurl.constants")

-- Cached module references --
local _GetBaseRange_
local _SetParameters_

-- Exports --
local M = {}

-- Solves y = x - sin(x), given y
local function XMinusSinX (y)
	local low, high = 0, pi / 2

	while true do
		local mid = (low + high) / 2
		local try = mid - sin(mid)
		local diff = try - y

		if abs(diff) < 1e-4 then
			return mid
		elseif diff < 0 then
			low = mid
		else
			high = mid
		end
	end
end

-- Solves y = x + sin(x), given y
local function XPlusSinX (n)
	local low, high = 0, pi / 2
	
	while true do
		local mid = (low + high) / 2
		local try = mid + sin(mid)
		local diff = try - n

		if abs(diff) < 1e-4 then
			return pi - mid
		elseif diff < 0 then
			low = mid
		else
			high = mid
		end
	end
end

-- TODO: Rather than binary search, try Newton, Euler, or RK4?

-- Curling radius --
local R = constants.Radius()

-- Common constants --
local PiR, HalfPiR = pi * R, pi * R / 2

--- Finds the scaling needed to land a displacement on the curling axis.
-- @number dx Displacement, x-coordinate...
-- @number dy ...and y-coordinate.
-- @treturn number Scale factor.
function M.FindDistance (dx, dy)
	local dist, t = sqrt(dx^2 + dy^2)

	-- First half of the cylinder's semicircle...
	if dist < HalfPiR - R then
		t = R * XMinusSinX(dist / R)

	-- ...second half...
	elseif dist < PiR then
		t = R * XPlusSinX(pi - dist / R)

	-- ...and beyond the cylinder, along the plane.
	else
		t = (dist + PiR) / 2
	end

	-- Normalize and scale the deltas.
	return t / dist
end

-- TODO: Find dist, given pos and angle, e.g. for positioning shadow proxies

--- Gets the default curl parameters.
-- @string dir Curl direction (for touch).
-- @treturn number Base angle...
-- @treturn number ...u...
-- @treturn number ...and v.
function M.GetBaseRange (dir)
	if dir == "R->L" then
		return 0, 1, 1
	elseif dir == "L->R" then
		return pi, 0, 1
	elseif dir == "B->T" then
		return pi / 2, 1, 1
	else
		return 3 * pi / 2, 1, 0
	end
end

--- Helper to update the curl effect's parameters via its object.
-- @tparam PageCurlWidget curl
-- @number angle Angle...
-- @number u ...edge x-coordinate, in [0, 1]...
-- @number v ...and y-coordinate.
function M.SetParameters (curl, angle, u, v)
	curl.angle_radians, curl.edge_x, curl.edge_y = angle, u, v
end

--- Puts the curl effect into its default state.
-- @tparam PageCurlWidget curl
-- @string dir Curl direction (for touch).
function M.SetBaseParameters (curl, dir)
	_SetParameters_(curl, _GetBaseRange_(dir))
end

-- Cache module members.
_GetBaseRange_ = M.GetBaseRange
_SetParameters_ = M.SetParameters

-- Export the module.
return M