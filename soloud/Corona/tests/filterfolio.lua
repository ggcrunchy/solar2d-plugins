--- Port of "megademo/filterfolio".

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

--
--
--

local gSoloud = soloud.createCore{ flags = { "CLIP_ROUNDOFF", "ENABLE_VISUALIZATION" } }
local gSfx = soloud.createSfxr()
local gSpeech = soloud.createSpeech()
local gMusic1, gMusic2, gMusic3 = soloud.createWavStream(), soloud.createWavStream(), soloud.createWavStream()
local gNoise = soloud.createNoise()

local gFilter = patterns.CreateFilterList()

--
--
--

gMusic1:load("audio/plonk_wet.ogg")
gMusic2:load("audio/delphi_loop.ogg")
gMusic3:load("audio/tetsno.ogg")

gMusic1:setLooping(true)
gMusic2:setLooping(true)
gMusic3:setLooping(true)

gSpeech:setText("My banana is yellow")

local gMusichandle1 = gSoloud:play(gMusic1)
local gMusichandle2 = gSoloud:play(gMusic2, { volume = 0 })
local gMusichandle3 = gSoloud:play(gMusic3, { volume = 0 })

gSoloud:setProtectVoice(gMusichandle1, true)
gSoloud:setProtectVoice(gMusichandle2, true)
gSoloud:setProtectVoice(gMusichandle3, true)

local gNoisehandle = gSoloud:play(gNoise, { volume = 0 })

--
--
--

local function Checkbox (label, handle, checked)
  utils.Checkbox{
    label = label, checked = checked,
    action = function(on)
      gSoloud:fadeVolume(handle, on and 1 or 0, 0.5)
    end
  }
  utils.NewLine()
end

--
--
--

utils.Begin("Control", 50, 20)

patterns.AddSFXRButtons(gSoloud, gSfx)

Checkbox("Toggle Music 1", gMusichandle1, true)
Checkbox("Toggle Music 2", gMusichandle2)
Checkbox("Toggle Music 3", gMusichandle3)
Checkbox("Toggle Noise", gNoisehandle)

utils.Button{
  label = "Speech",
  action = function()
    gSoloud:play(gSpeech, { volume = 1 })
  end
}

utils.End()

--
--
--

patterns.AddFilterSelection(gSoloud, gFilter, 475, 50)

--
--
--

utils.Begin("Sounds", 250, 250)

local histo, wave = patterns.MakeHistogramAndWave()
local active_voices = utils.Text()

utils.End()

--
--
--


patterns.Loop(function()
  patterns.UpdateOutput(gSoloud, histo, wave, active_voices)
end)