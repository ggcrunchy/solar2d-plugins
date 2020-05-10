--- Boilerplate for winding rule scenes.

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
local shapes = require("shapes")
local utils = require("utils")

-- Corona globals --
local transition = transition

-- Cached module references --
local _SetContourFuncs_

-- Exports --
local M = {}

--
--
--

function M.Hide (scene)
	scene.m_group:removeSelf()

	utils.CancelTimers()
end

function M.SetContourFuncs (group)
	local contour

	function group.on_begin_contour ()
		contour = {}
	end

	function group.on_add_vertex (x, y)
		contour[#contour + 1] = x
		contour[#contour + 1] = y
	end

	function group.on_end_contour ()
		utils.GetTess():AddContour(contour)

		contour = nil
	end
end

local FadeInParams = { alpha = 1 }

function M.Show (scene, rule)
	local group = display.newGroup()

	scene.view:insert(group)

	scene.m_group = group

	_SetContourFuncs_(group)

	local add_vert, close_poly = utils.Polygon()

	function group:on_done (tess)
		if not self.m_back then
			self.m_back = display.newGroup()

			self:insert(self.m_back)
			self.m_back:toBack()

			self.m_back.alpha = 0
		end

		self.m_back.add_vert, self.m_back.close = add_vert, close_poly

		utils.PolyTris(self.m_back, tess, rule)
	end

	function group:on_all_done ()
		transition.to(self.m_back, FadeInParams)
	end

	utils.DrawAll(group, shapes.BoxCCW, shapes.BoxMixed, shapes.Overlap, shapes.SelfIntersectingSpiral)
end

_SetContourFuncs_ = M.SetContourFuncs

return M