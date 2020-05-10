--- A few GUI elements for Quirky.

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
local controls = require("quirky.controls")

-- Corona globals --
local display = display
local native = native

-- Exports --
local M = {}

-- Helper to add a rotated arrow button that propagates key input
function M.Arrow (group, x, y, w, h, key, rot)
	local button = display.newImageRect(group, "Arrow.png", w, h)

	button:addEventListener("touch", controls.Touch)

	button.x, button.y = x, y
	button.rotation = rot

	button.m_key = key

	return button
end

-- Helper to add a rounded rect button (aligned to the right-hand side) that propagates key input
function M.RoundedRect (group, str, y, key)
	local bgroup = display.newGroup()

	group:insert(bgroup)

	local text = display.newText(bgroup, str, 0, y, native.systemFont, 16)
	local right = (display.contentWidth + display.viewableContentWidth) / 2 -- VCW + (CW - VCW) / 2

	text.x = right - .5 * text.width - 10

	local rect = display.newRoundedRect(bgroup, text.x, text.y, text.width + 10, text.height + 10, 15)

	rect:addEventListener("touch", controls.Touch)
	rect:setFillColor(0, 0, 1)
	rect:toBack()

	rect.m_key = key

	return bgroup
end

-- Export the module.
return M