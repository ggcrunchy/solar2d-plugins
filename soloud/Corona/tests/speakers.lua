--- Port of "megademo/speakers".

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
local wrap = coroutine.wrap
local yield = coroutine.yield

-- Modules --
local patterns = require("patterns")
local utils = require("utils")

-- Plugins --
local soloud = require("plugin.soloud")

-- Solar2D globals --
local display = display
local timer = timer

--
--
--

local filenames = {
  "ch6_1.flac",
  "ch6_2.flac",
  "ch6_3.flac",
  "ch6_4.flac",
  "ch6_5.flac",
  "ch6_6.flac",
  "ch8_1.flac",
  "ch8_2.flac",
  "ch8_3.flac",
  "ch8_4.flac",
  "ch8_5.flac",
  "ch8_6.flac",
  "ch8_7.flac",
  "ch8_8.flac"
}

--
--
--

local gSoloud
local gWav, gWavOk = {}, {}
local gChannelsAvailable = {}

for i = 1, #filenames do
  local name = ("audio/wavformats/%s"):format(filenames[i])

  gWav[i] = soloud.createWav()
  gWavOk[i] = gWav[i]:load(name)
end

--
--
--

utils.Begin("Control", 50, 50)
  utils.Begin("scrollview", display.contentHeight - 150)

  for i = 1, #filenames do
    local wav_button = utils.Button{
      isEnabled = gWavOk[i],
      label = ("Play %s"):format(filenames[i]),
      action = function()
        gSoloud:play(gWav[i])
      end
    }

    if not gWavOk[i] then
      wav_button:setFillColor(.5)
    end

    utils.NewLine()
  end

  utils.End()
local Control = utils.End()

--
--
--

utils.Begin("        Audio device        ", 450, 200) -- widen the window, else the title is scrunched

local channel_buttons = {}

for i = 1, soloud.MAX_CHANNELS do
  channel_buttons[i] = utils.Button{
    label = ("%d channels"):format(i),
    action = function()
      gSoloud:destroy()

      gSoloud = soloud.createCore{
        flags = { "CLIP_ROUNDOFF", "ENABLE_VISUALIZATION" },
        channels = i
      }
    end
  }
  
  utils.NewLine()
end

local AudioDevice = utils.End()

--
--
--

Control.isVisible, AudioDevice.isVisible = false, false

--
--
--

local function EnumerateBackends ()
  local biggest = 0

  for i = 1, soloud.MAX_CHANNELS do
    local core = soloud.createCore{
      flags = { "CLIP_ROUNDOFF", "ENABLE_VISUALIZATION" },
      channels = i
    }

    if core then
			gChannelsAvailable[i], biggest = true, i

      core:destroy()
    else
      gChannelsAvailable[i] = false
		end

    yield()
  end

	gSoloud = soloud.createCore{
    flags = { "CLIP_ROUNDOFF", "ENABLE_VISUALIZATION" },
    channels = biggest
  }
end

--
--
--

timer.performWithDelay(30, wrap(function(event) -- load gradually, to avoid hitches
  local source = event.source -- can get clobbered

  EnumerateBackends()

  timer.cancel(source)

  Control.isVisible, AudioDevice.isVisible = true, true

  for i = 1, #channel_buttons do
    if not gChannelsAvailable[i] then
      channel_buttons[i]:setEnabled(false)
      channel_buttons[i]:setFillColor(.5)
    end
  end
end), 0)

--
--
--

local tris = {}

for i = 1, soloud.MAX_CHANNELS do
  tris[i] = utils.Triangle(20)

  tris[i].isVisible = false
end

--
--
--

utils.Begin("Output", 450, 20)

local histo, wave = patterns.MakeHistogramAndWave()
local active_voices = utils.Text()
local backend = utils.Text()

utils.End()

--
--
--

local CX, CY = display.contentCenterX, display.contentCenterY

patterns.Loop(function()
  if gSoloud then
    for i = 1, soloud.MAX_CHANNELS do
      local x, y, z = gSoloud:getSpeakerPosition(i)
  
      if x then
        local vol = gSoloud:getApproximateVolume(i)
      
        vol = vol * 20

        if vol > 1 then
          vol = 1
        end

        tris[i]:setFillColor(vol)

        tris[i].x = CX - x * 90
        tris[i].y = CY - z * 90
        tris[i].isVisible = true
      else
        tris[i].isVisible = false
      end
    end

    --
    --
    --

    patterns.UpdateOutput(gSoloud, histo, wave, active_voices)

    backend.text = ("Backend: %s Channels: %d"):format(gSoloud:getBackendString(), gSoloud:getBackendChannels())
  else
    backend.text = "Enumerating backends..."
  end
end)