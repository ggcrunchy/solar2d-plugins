--- Port of "env" demo.

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
local floor = math.floor

-- Modules --
local patterns = require("patterns")
local utils = require("utils")

-- Plugins --
local soloud = require("plugin.soloud")

-- Solar2D globals --
local display = display

--
--
--

local gSoloud = soloud.createCore{ flags = { "CLIP_ROUNDOFF", "ENABLE_VISUALIZATION" } }
local gLPFilter = soloud.createBiquadResonantFilter()
local gRain, gWind, gMusic = soloud.createWav(), soloud.createWav(), soloud.createWav()

local gWalker, gBackground
local gRainHandle, gWindHandle, gMusicHandle

--
--
--

local xs, ys = display.contentWidth / 800,display.contentHeight / 400 -- adjust from demo dimensions

--
--
--

local function drawbg (x, y)
  gBackground.x, gBackground.y = x * xs, (y + 100) * ys
end

local function drawwalker(frame, x, y)
  gWalker.x, gWalker.y = (x - 12) * xs, (y + frame + 100) * ys
end

local function drawrect (r, x, y, w, h, gray)
  r.x, r.y = x * xs, (y + 100) * ys
  r.width, r.height = w, h
  r.isVisible = true

  r:setFillColor(gray)
end

--
--
--

local function SMOOTHSTEP (x)
  return x * x * (3 - 2 * x)
end

local mode_a, mode_b, mode_c, mode_d, mode_e = 0, 0, 0, 0, 0

local rects = {}

local function render (tick)
	local p = (tick % 60000) / 60000.0

	local xpos, ypos, dudey

  if p < .1 then
		xpos = 0
		ypos = -340
		dudey = -8
	elseif p < 0.5 then
		local v = (p - 0.1) * 2.5

		v = SMOOTHSTEP(v)
		v = SMOOTHSTEP(v)
		v = SMOOTHSTEP(v)

		xpos = -floor(v * (800 - 400))
		ypos = -340;
		dudey = floor((1 - v) * -8)
	elseif p < 0.9 then
		local v = (p - 0.5) * 2.5
    
		v = SMOOTHSTEP(v)
		v = SMOOTHSTEP(v)
		v = SMOOTHSTEP(v)
		xpos = -(800 - 400)
		ypos = floor((1 - v) * (- 340))
		dudey = floor(v * 90)
	else
		xpos = -(800 - 400)
		ypos = 0
		dudey = 90
	end

	if p < 0.35 then
		if mode_a ~= 0 then
			gSoloud:fadeVolume(gRainHandle, 1, 0.2)
    end

		mode_a = 0
	else
		if mode_a ~= 1 then
			gSoloud:fadeVolume(gRainHandle, 0, 0.2)
    end

		mode_a = 1
	end

	if p < 0.7 then
		if mode_b ~= 0 then
			gSoloud:fadeVolume(gWindHandle, 0, 0.2)
    end

		mode_b = 0
	elseif p < 0.8 then
		gSoloud:setVolume(gWindHandle, (p - 0.7) * 10)

		mode_b = 1
	else
		if mode_b ~= 2 then
			gSoloud:fadeVolume(gWindHandle, 1, 0.2)
    end

		mode_b = 2
	end

	if p < 0.2 then
		if mode_c ~= 0 then
			gSoloud:fadeVolume(gMusicHandle, 0, 0.2)
    end

		mode_c = 0
	elseif p < 0.4 then
		gSoloud:setVolume(gMusicHandle, (p - 0.2) * 5)

		mode_c = 1
	elseif p < 0.5 then
		if mode_c ~= 2 then
			gSoloud:fadeVolume(gMusicHandle, 1, 0.2)
    end

		mode_c = 2
	elseif p < 0.7 then
		gSoloud:setVolume(gMusicHandle, 1 - (p - 0.5) * 4.5)

		mode_c = 3
	else
		if mode_c ~= 4 then
			gSoloud:fadeVolume(gMusicHandle, 0.1, 0.2)
    end

		mode_c = 4
	end

	if p < 0.25 then
		if mode_d ~= 0 then
			gSoloud:fadeFilterParameter(gMusicHandle, 1, "BiquadResonantFilter.FREQUENCY", 200, 0.2)
			gSoloud:fadeFilterParameter(gMusicHandle, 1, "Filter.WET", 1, 0.2)
		end

		mode_d = 0
	elseif p < 0.35 then
		if mode_d ~= 1 then
			gSoloud:fadeFilterParameter(gMusicHandle, 1, "Filter.WET", 0.5, 2.0)
		end

		mode_d = 1
	elseif p < 0.55 then
		if mode_d ~= 2 then
			gSoloud:fadeFilterParameter(gMusicHandle, 1, "BiquadResonantFilter.FREQUENCY", 2000, 1.0)
			gSoloud:fadeFilterParameter(gMusicHandle, 1, "Filter.WET", 0, 1.0)
		end

		mode_d = 2
	else
		if mode_d ~= 3 then
			gSoloud:fadeFilterParameter(gMusicHandle, 1, "BiquadResonantFilter.FREQUENCY", 200, 0.3)
			gSoloud:fadeFilterParameter(gMusicHandle, 1, "Filter.WET", 1, 0.3)
		end

		mode_d = 3
	end

	if p < 0.2 then
		if mode_e ~= 0 then
			gSoloud:fadePan(gMusicHandle, 1, 0.2)
    end

		mode_e = 0
	elseif p < 0.4 then
		gSoloud:setPan(gMusicHandle, 1 - ((p - 0.2) * 5))

		mode_e = 1
	else
		if mode_e ~= 2 then
			gSoloud:fadePan(gMusicHandle, 0, 0.2)
    end

		mode_e = 2
	end
	
  drawbg(xpos, ypos)

  drawwalker(floor(tick / 128) % (floor(tick / 256) % 5 + 1), (400 - 32) / 2 + 12, 256 - 32 * 2 - 32 - dudey)
	
	if p > 0.5 then
		local w = floor((p - 0.5) * 600)

		if w > 32 then
      w = 32
    end

		drawrect(rects[1], (400 - 32) / 2 + 12, 256 - 32 * 2 - 32 + ypos + 340, w / 2, 64, 1)
		drawrect(rects[2], (400 - 32) / 2 + 12 + 32 - (w / 2), 256 - 32 * 2 - 32 + ypos + 340, w / 2, 64, 1)

		drawrect(rects[3], (400 - 32) / 2 + 12 + (w / 2), 256 - 32 * 2 - 32 + ypos + 340, 1, 64, 0xA / 0xF)
		drawrect(rects[4], (400 - 32) / 2 + 12 + 32 - (w / 2), 256 - 32 * 2 - 32 + ypos + 340, 1, 64, 0xA / 0xF)
  else
    for i = 1, #rects do
      rects[i].isVisible = false
    end
	end

	return p
end

--
--
--

gSoloud:setGlobalVolume(0.75)
gSoloud:setPostClipScaler(0.75)

gRain:load("audio/rainy_ambience.ogg")
gRain:setLooping(true)
gWind:load("audio/windy_ambience.ogg")
gWind:setLooping(true)
gMusic:load("audio/tetsno.ogg")
gMusic:setLooping(true)
gLPFilter:setParams{ type = "LOWPASS", frequency = 100, resonance = 10 }
gMusic:setFilter(1, gLPFilter)

--
--
--

gRainHandle = gSoloud:play(gRain)
gWindHandle = gSoloud:play(gWind, { volume = 0 })
gMusicHandle = gSoloud:play(gMusic, { volume = 0 })

gBackground = display.newImageRect("graphics/env_bg.png", 800, 600)

gBackground.anchorX, gBackground.anchorY = 0, 0
gBackground.xScale, gBackground.yScale = xs, ys

gWalker = display.newImageRect("graphics/env_walker.png", 60, 60)

gWalker.anchorX, gWalker.anchorY = 0, 0
gWalker.xScale, gWalker.yScale = xs, ys

for i = 1, 4 do
  rects[i] = display.newRect(0, 0, 1, 1)

  rects[i].anchorX, rects[i].anchorY = 0, 0
  rects[i].xScale, rects[i].yScale = xs, ys
  rects[i].isVisible = false
end

--
--
--

utils.Begin("Output", 450, 50)

local histo, wave = patterns.MakeHistogramAndWave()
local active_voices = utils.Text()
local progress = utils.Text()
local rain_volume = utils.Text()
local music_volume = utils.Text()
local wind_volume = utils.Text()
local music_pan = utils.Text()
local music_filter_wet = utils.Text()
local music_filter_freq = utils.Text()

utils.End()

--
--
--

render(0) -- avoid a little jump at the beginning

patterns.Loop(function(event)
  local p = render(event.time)

  --
  --
  --

  patterns.UpdateOutput(gSoloud, histo, wave, active_voices)

  --
  --
  --

	progress.text = ("Progress         : %3.3f%%"):format(100 * p)
	rain_volume.text = ("Rain volume      : %3.3f"):format(gSoloud:getVolume(gRainHandle))
	music_volume.text = ("Music volume     : %3.3f"):format(gSoloud:getVolume(gMusicHandle))
	wind_volume.text = ("Wind volume      : %3.3f"):format(gSoloud:getVolume(gWindHandle))
	music_pan.text = ("Music pan        : %3.3f"):format(gSoloud:getPan(gMusicHandle))
	music_filter_wet.text = ("Music filter wet : %3.3f"):format(gSoloud:getFilterParameter(gMusicHandle, 1, "Filter.WET"))
	music_filter_freq.text = ("Music filter freq: %3.3f"):format(gSoloud:getFilterParameter(gMusicHandle, 1, "BiquadResonantFilter.FREQUENCY"))
end)