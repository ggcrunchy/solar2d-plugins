--- Boilerplate for winding rule scenes with boundary contours.

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
local unpack = unpack

-- Modules --
local shapes = require("shapes")
local utils = require("utils")
local winding = require("winding")

-- Corona globals --
local transition = transition

-- Exports --
local M = {}

--
--
--

M.Hide = winding.Hide

local FadeInParams = { alpha = 1 }

function M.Show (scene, rule)
	local group = display.newGroup()

	scene.view:insert(group)

	scene.m_group = group

	winding.SetContourFuncs(group)

	function group:on_done (tess)
		if not self.m_back then
			self.m_back = display.newGroup()

			self:insert(self.m_back)
			self.m_back:toBack()

			self.m_back.alpha = 0
		end

		local tess = utils.GetTess()

		tess:Tesselate(rule, "BOUNDARY_CONTOURS")

		local elems = tess:GetElements()
		local verts = tess:GetVertices()

		for i = 1, tess:GetElementCount() do
			local bi, boundary = i * 2, {}
			local base, count = elems[bi - 1], elems[bi]

			for j = 0, count - 1 do
				local offset = (base + j) * 2

				boundary[#boundary + 1] = verts[offset + 1]
				boundary[#boundary + 1] = verts[offset + 2]
			end

			boundary[#boundary + 1] = boundary[1]
			boundary[#boundary + 1] = boundary[2]

			local line = display.newLine(self.m_back, unpack(boundary))

			line:setStrokeColor(0, 0, 1)

			line.strokeWidth = 5
		end
	end

	function group:on_all_done ()
		transition.to(self.m_back, FadeInParams)
	end

	utils.DrawAll(group, shapes.BoxCCW, shapes.BoxMixed, shapes.Overlap, shapes.SelfIntersectingSpiral)
end

return M