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
local random = math.random
local remove = table.remove
local unpack = unpack

-- Plugins --
local libtess2 = require("plugin.libtess2")

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

local UNDEF = libtess2.Undef()

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

		tess:Tesselate(rule, "CONNECTED_POLYGONS")

		local elems = tess:GetElements()
		local verts = tess:GetVertices()
		local stack, visited, poly_index = {}, {}, 1
		local add_vert, close_poly = utils.Polygon()

		for i = 1, tess:GetElementCount() do -- multiply by 2 * poly_size (poly_size = 3 in this case) for #elems
			if not visited[i] then -- start a new flood fill if not already absorbed
				stack[#stack + 1], visited[i] = i, true

				local r, g, b = random(), random(), random()

				repeat
					local index = remove(stack)

					if index then
						local base = (index - 1) * 6
						local nbase, offset = base + 3, 0

						for j = 1, 3 do
							local vi, ni = elems[base + j], elems[nbase + j]

							-- in case of poly_size > 3: if vi == UNDEF then break end

							add_vert(verts[vi * 2 + 1], verts[vi * 2 + 2], offset)

							if ni ~= UNDEF and not visited[ni + 1] then
								stack[#stack + 1], visited[ni + 1] = ni + 1, true
							end

							offset = offset + 2
						end

						close_poly(self.m_back)

						local poly = self.m_back[self.m_back.numChildren]

						poly:setFillColor(r, g, b)
					end
				until not index
			end
		end
	end

	function group:on_all_done ()
		transition.to(self.m_back, FadeInParams)
	end

	utils.DrawAll(group, shapes.BoxCCW, shapes.BoxMixed, shapes.Overlap, shapes.SelfIntersectingSpiral)
end

return M