--- Port of "megademo/virtualvoices".

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
local patterns = require("patterns")
local utils = require("utils")

-- Plugins --
local soloud = require("plugin.soloud")

-- Solar2D globals --
local display = display
local native = native

--
--
--

local gSoloud = soloud.createCore{ flags = { "CLIP_ROUNDOFF", "ENABLE_VISUALIZATION" } }
local gSfx = {}

local VOICEGRID = 24
local VOICES = VOICEGRID * VOICEGRID

for i = 1, VOICES do
  gSfx[i] = soloud.createSfxr()
end

--
--
--

gSoloud:setGlobalVolume(4)
gSoloud:setMaxActiveVoiceCount(16)

local gSndHandle = {}

for i = 1, VOICES do
  gSfx[i]:loadPreset("COIN", i)
  gSfx[i]:setLooping(true)
  gSfx[i]:setInaudibleBehavior(false, false) -- make sure we don't kill inaudible sounds
	gSfx[i]:set3dMinMaxDistance(1, 100)
  gSfx[i]:set3dAttenuation("LINEAR_DISTANCE", 1)
end

--
--
--

local about = display.newText("Drag the rectangle near or over the grid", 0, display.contentHeight - 50, native.systemFontBold, 25)

about.anchorX, about.x = 0, 50

--
--
--

local Left, Top = 50, 50

for i = 0, VOICEGRID - 1 do
  for j = 0, VOICEGRID - 1 do
    local index = i * VOICEGRID + j + 1

    gSndHandle[index] = gSoloud:play3d(gSfx[index], i * 15 + 20.0, 0, j * 15 + 20.0)
  end
end

--
--
--

utils.Begin("Output", 450, 50)

local histo, wave = patterns.MakeHistogramAndWave()
local active_voices = utils.Text()
local total_voices = utils.Text()

utils.Text("Maximum voices   : " .. soloud.VOICE_COUNT)

utils.End()

--
--
--

local triangles = {}

for i = 0, VOICEGRID - 1 do
  for j = 0, VOICEGRID - 1 do
    local index, x, y = i * VOICEGRID + j, Left + i * 15, Top + j * 15
    local top = utils.Triangle{ width = 10, height = 5, scale = 1 }
    local bottom = utils.Triangle{ width = 10, height = 5, scale = 1, flipped = true } 

    top:translate(x, y)
    bottom:translate(x, y + 5)

    triangles[index * 2 + 1] = top
    triangles[index * 2 + 2] = bottom 
  end
end

--
--
--

local function DragTouch (event)
  local rect, phase = event.target, event.phase

  if phase == "began" then
    display.getCurrentStage():setFocus(rect)

    rect.dx, rect.dy = event.x - rect.x, event.y - rect.y
  elseif rect.dx then
    if phase == "moved" then
      rect.x, rect.y = event.x - rect.dx, event.y - rect.dy
    else
      display.getCurrentStage():setFocus(nil)

      rect.dx = nil
    end
  end

  return true
end

--
--
--

local rect = display.newRect(100, display.contentHeight - 115, 50, 50)

rect:addEventListener("touch", DragTouch)
rect:setFillColor(0, .3)
rect:setStrokeColor(0, 0, 1)

rect.strokeWidth = 2

--
--
--

patterns.Loop(function()
  gSoloud:set3dListenerParameters(
    rect.x - Left, 0, rect.y - Top,
    0, 0, 0,
    0, 1, 0
  )

  gSoloud:update3dAudio()

  local tri_index = 1

	for i = 0, VOICEGRID - 1 do
		for j = 0, VOICEGRID - 1 do
			local v = gSoloud:getOverallVolume(gSndHandle[i * VOICEGRID + j + 1])

      triangles[tri_index]:setFillColor(v)
      triangles[tri_index + 1]:setFillColor(v)
     
      tri_index = tri_index + 2
		end
  end

  --
  --
  --

  patterns.UpdateOutput(gSoloud, histo, wave, active_voices)

  total_voices.text = ("Total voices     : %d"):format(gSoloud:getVoiceCount())
end)