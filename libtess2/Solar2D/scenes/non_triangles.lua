--- Scene that demonstrates tessellation as polygons with more than three sides.

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
local cos = math.cos
local sin = math.sin
local pi = math.pi
local random = math.random

-- Plugins --
local libtess2 = require("plugin.libtess2")

-- Modules --
local utils = require("utils")

-- Corona modules --
local composer = require("composer")

--
--
--

local Scene = composer.newScene()

local CX, CY = display.contentCenterX, display.contentCenterY

local Count, Radius = 13, 80 -- count = 13 introduces a triangle and quad and is still crude enough to see vertices

local UNDEF = libtess2.Undef()

function Scene:create ()
	local verts = {}

	for i = 1, Count do
		local angle = (i - 1) * (2 * pi) / Count

		verts[#verts + 1] = CX + cos(angle) * Radius
		verts[#verts + 1] = CY + sin(angle) * Radius
	end

	local tess = utils.GetTess()

	tess:AddContour(verts)

	verts[#verts + 1] = verts[1]
	verts[#verts + 2] = verts[2]

	tess:Tesselate("POSITIVE", "POLYGONS", 5) -- prefer pentagons

	local elems = tess:GetElements()
	local verts = tess:GetVertices()
	local add_vert, close_poly = utils.Polygon()

	for i = 1, tess:GetElementCount() do
		local base, offset = (i - 1) * 5, 0

		for j = 1, 5 do
			local index = elems[base + j]

			if index == UNDEF then -- unable to complete pentagon?
				break
			end

			add_vert(verts[index * 2 + 1], verts[index * 2 + 2], offset)

			offset = offset + 2
		end

		close_poly(self.view)

		local polygon = self.view[self.view.numChildren]

		polygon:setFillColor(random(), random(), random())
	end
end

Scene:addEventListener("create")

return Scene