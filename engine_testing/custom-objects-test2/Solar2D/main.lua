--- Custom command test.

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

local co2 = require("plugin.customobjects2")

local g1 = co2.newScopedGroupObject()
local g2 = co2.newScopedGroupObject()

local green = display.newRect(g1, 100, 100, 180, 180)
local blue = display.newRect(g1, 300, 150, 200, 100)

green:setFillColor(0, 1, 0)
blue:setFillColor(0, 0, 1)

local color_mask1 = co2.newColorMaskObject(g1)

local colors = { "red", "green", "blue" }
local was = table.indexOf(colors, "blue")

color_mask1.blue = false

local overlap = display.newRect(g1, 225, 200, 100, 150) -- will be white minus blue = yellow

-- This will then follow the cycle = red mask: cyan, green mask: magenta, blue mask: yellow.
-- However, where it overlaps with `green` and `blue`, since `overlap` does not write those channels
-- when the associated mask is off, it will pick up the background instead, so become white.
-- More specifically, when magenta, the upper-left will be white; when yellow, the upper-right.

local alone = display.newRect(g1, 250, 400, 300, 100)

alone:setFillColor(1, 0, 1) -- will be red (then magenta, blue, repeat)

-- This will then follow the cycle = red mask: blue, green mask: magenta, blue mask: red.

for i = 1, 20 do
	local c = display.newCircle(g2, math.random(250, 600), math.random(250, 600), math.random(6, 25))

	c:setFillColor(math.random(), math.random(), math.random())
end

timer.performWithDelay(2500, function()
	local prev = colors[was]

	color_mask1[prev] = nil -- use the default

	was = was % 3 + 1

	local new = colors[was]

	color_mask1[new] = false -- disable this channel
end, 0)

--[=[

-- future use case: even if we omit the color components, we can still write stencil or depth

local xx = display.newRect(550, 300, 100, 100)

xx:setFillColor(0, 1, 1) -- unaffected, color mask and scissor cleaned up at end of g1 and g2 respectively

--local stencil_state1 = stencilLib.newStencilStateObject(g2)
local color_mask2 = co2.newColorMaskObject(g2)

--stencil_state1.enabled = true
--stencil_state1.funcRef = 1
--stencil_state1.zpass = "replace"
color_mask2.green, color_mask2.alpha = false, false

local r = display.newRect(120, 120, 100, 100)

r:setFillColor(0, 1, 0) -- no green will be written, but color only masked; stencil WILL be written

--local stencil_state2 = stencilLib.newStencilStateObject(g1)
local color_mask3 = co2.newColorMaskObject(g1)

--stencil_state2.func = "notEqual"
--stencil_state2.funcRef = 1
color_mask3.alpha = true

local t = display.newRect(120, 120, 100, 100)

g1:insert(t)

timer.performWithDelay(90, function(event)
	t.x = 120 + 200 * math.sin(event.time / 300)
end, 0)

t:scale(1.1, 1.1) -- extend the object so we still etch an outline
t:setFillColor(1, 0, 0)

]=]