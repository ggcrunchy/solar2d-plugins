--- Class components of curl effect.

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
local deg = math.deg
local getmetatable = getmetatable
local rad = math.rad
local rawget = rawget
local rawset = rawset
local setmetatable = setmetatable

-- Exports --
local M = {}

-- Common group metatable; page curl class metatable --
local GroupMT, PageCurlMT

-- Helper to set up page curl properties
local function NewProp (what)
	return { key = what, saved = "m_saved_" .. what }
end

-- Name -> curl property map --
local Props = {	angle_radians = NewProp("angle"), edge_x = NewProp("u"), edge_y = NewProp("v") }

--- Initializes the curl widget's class infrastructure.
-- @tparam PageCurlWidget curl
-- @ptable methods Curl widget methods.
function M.Init (curl, methods)
	local mt = getmetatable(curl)

	-- When possible, reuse the group metatable. (Certain modules monkey-patch [display.newGroup](https://docs.coronalabs.com/api/library/display/newGroup.html)
	-- so that each instance gets a unique metatable. If this is so, give up after the first
	-- try.) In any event, when creating a new metatable, augment the indexing metamethods of
	-- the group at hand, i.e. the page curl widget.
	if mt ~= GroupMT then
		PageCurlMT, GroupMT = {}, GroupMT ~= nil or mt

		local old_index, old_newindex = mt.__index, mt.__newindex

		-- __index metamethod
		function PageCurlMT.__index (t, k)
			-- Property alias: read the underlying property and apply an transformation.
			if k == "angle" then
				return deg(t.m_saved_angle)
			else
				-- Property: return its saved value.
				local prop = Props[k]

				if prop then
					return rawget(t, prop.saved)

				-- Otherwise, return a method or, failing that, use stock group __index.
				else
					return methods[k] or old_index(t, k)
				end
			end
		end

		-- __newindex metamethod
		function PageCurlMT.__newindex (t, k, v)
			local prop = Props[k]

			-- Property: save its value locally, updating the effect as well.
			if prop then
				rawset(t, prop.saved, v)

				t.m_object.fill.effect[prop.key] = v

			-- Property alias: apply any transformation and write the underlying property.
			elseif k == "angle" then
				t.angle_radians = rad(v)

			-- Otherwise, use stock group __newindex.
			else
				old_newindex(t, k, v)
			end
		end
	end

	return setmetatable(curl, PageCurlMT)
end

-- Export the module.
return M