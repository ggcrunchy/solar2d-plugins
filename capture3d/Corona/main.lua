--- Sample for Bytemap plugin.

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
local char = string.char
local cos = math.cos
local floor = math.floor

-- Plugins --
local bytemap = require("plugin.Bytemap")

-- Some basic help.
display.newText("Click and drag the glowing box to paint the framed region", display.contentCenterX, 35, native.systemFontBold, 14)

-- Make a brush with a time-based shader-ish effect. The pattern is switched
-- frequently so that the screen stays a bit varied even as you fill it up.
local bconfig_set_bytes, rgb, w, h = { format = "rgb" }, {}, 32, 32
local btex = bytemap.newTexture{ width = w, height = h, format = "rgba" }
local brush = display.newImage(btex.filename, btex.baseDir)

brush.x, brush.y = display.contentCenterX, display.contentCenterY

local PatternTime = 1500
local Period = 6 * PatternTime  -- 6 = the number of permutations of (1, 2, 3), so how many ways we can
								-- switch around the RGB components

timer.performWithDelay(50, function(event)
	local when = event.time % Period -- switch patterns with a period of (N * Time) seconds
	local index = floor(when / PatternTime) -- which pattern? (0 to N - 1)
	local ri = floor(index / 2) + 1 -- "red" component... (use three indices so we can shuffle colors)
	local gi = (ri + index % 2) % 3 + 1 -- ...green...
	local bi = 6 - ri - gi -- ...and blue
	local frame_delta = when / 300 -- vary the colors slightly from frame to frame

	for j = 1, h do
		for i = 1, w do
			rgb[ri] = floor(255 * cos(frame_delta + (i * j) * 30)^2) -- write "red" component...
			rgb[gi] = floor(96 * cos(frame_delta * 5 + (i + j) * 45)^2) -- ...green...
			rgb[bi] = floor(255 * cos(frame_delta + j * 50)^2) -- ...blue

			-- 1-pixel region. This is just for show; alternatively, we can gather, concatenate,
			-- and blast the whole 32 x 32 batch in one shot. Note that this is RGB data sent to
			-- an RGBA target (see config declaration above).
			bconfig_set_bytes.x1, bconfig_set_bytes.x2 = i, i
			bconfig_set_bytes.y1, bconfig_set_bytes.y2 = j, j

			btex:SetBytes(char(rgb[1], rgb[2], rgb[3]), bconfig_set_bytes)
		end
	end

	btex:invalidate()
end, 0)

-- The larger bytemap that the brush will be painting.
local larger = bytemap.newTexture{
	width = math.floor(.75 * display.viewableContentWidth),
	height = math.floor(.7 * display.viewableContentHeight),
	format = "rgba"
}

local fill_me = display.newImage(larger.filename, larger.baseDir)

fill_me.x, fill_me.y = brush.x, brush.y

-- Put a frame around the larger bytemap to indicate the boundaries, since it will
-- be all black until we fill it up a bit.
local fbounds = fill_me.contentBounds
local frect = display.newRect(fill_me.x, fill_me.y, fbounds.xMax - fbounds.xMin, fbounds.yMax - fbounds.yMin)

frect:setFillColor(0, 0)
frect:setStrokeColor(1, 0, 0)

frect.strokeWidth = 2

-- Allow brush drags.
brush:addEventListener("touch", function(event)
	local phase, target = event.phase, event.target

	if phase == "began" or phase == "moved" then
		if phase == "began" then
			display.getCurrentStage():setFocus(target)

			target.xwas, target.ywas = event.x, event.y
		elseif target.xwas then -- ignore drags onto brush from outside...
			target.x, target.y = target.x + event.x - target.xwas, target.y + event.y - target.ywas
			target.xwas, target.ywas = event.x, event.y
		else -- ... in fact, don't even respond to them
			return false
		end

		-- Stuff the brush's current bytes into the larger bytemap, then refresh
		-- the latter to show the changes.
		local bounds = target.contentBounds
		local x1m1, y1m1 = bounds.xMin - fbounds.xMin, bounds.yMin - fbounds.yMin

		larger:SetBytes(btex:GetBytes(), {	-- TODO: just 'btex' if Bytemaps implement ByteReader,
			x1 = x1m1 + 1, x2 = x1m1 + w,	-- which would also avoid a wasteful copy
			y1 = y1m1 + 1, y2 = y1m1 + h
		})
		larger:invalidate()
	elseif phase == "ended" or phase == "cancelled" then
		display.getCurrentStage():setFocus(nil)

		target.xwas, target.ywas = nil
	end

	return true
end)
