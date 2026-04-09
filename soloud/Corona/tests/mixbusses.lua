--- Port of "megademo/mixbusses".

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

-- Modules --
local patterns = require("patterns")
local utils = require("utils")

-- Plugins --
local soloud = require("plugin.soloud")

--
--
--

local gSoloud = soloud.createCore{ flags = { "CLIP_ROUNDOFF", "ENABLE_VISUALIZATION" } }
local gSfxloop, gMusicloop = soloud.createWav(), soloud.createWav()
local gSfxbus, gMusicbus, gSpeechbus = soloud.createBus(), soloud.createBus(), soloud.createBus()

--
--
--

local gSpeechbusHandle = gSoloud:play(gSpeechbus)
local gSfxbusHandle = gSoloud:play(gSfxbus)
local gMusicbusHandle = gSoloud:play(gMusicbus)

local gSpeech = {}

for i, text in ipairs{
		"There is flaky pastry in my volkswagon.",
		"The fragmentation of empiricism is hardly influential in its interdependence.",
		"Sorry, my albatros is not inflatable.",
		"The clairvoyance of omnipotence is in fact quite closed-minded in its ecology.",
		"Cheese is quite nice.",
		"Pineapple Scones with Squash and Pastrami Sandwich",
		"The smart trader nowadays will be sure not to prorate OTC special-purpose entities.",
		"The penguins are in the toilets.",
		"Don't look, but there is a mountain lion stalking your children",
		"The train has already gone, would you like to hire a bicycle?"
} do
  gSpeech[i] = soloud.createSpeech()

  gSpeech[i]:setText(text)
end

gSfxloop:load("audio/war_loop.ogg")
gSfxloop:setLooping(true)
gMusicloop:load("audio/algebra_loop.ogg")
gMusicloop:setLooping(true)

gSfxbus:play(gSfxloop)
gMusicbus:play(gMusicloop)

--
--
--

utils.Begin("Output", 450, 300)

local histo, wave = patterns.MakeHistogramAndWave()
local speech_bus_volume = utils.Text()
local music_bus_volume = utils.Text()
local sfx_bus_volume = utils.Text()
local active_voices = utils.Text()

utils.End()

--
--
--

utils.Begin("Control", 50, 50)

utils.Slider{
  label = "Speech bus volume",
  action = function(t)
    gSoloud:setVolume(gSpeechbusHandle, t * 2)
  end,
  value = 50
}
utils.NewLine(10)

utils.Slider{
  label = "Music bus volume",
  action = function(t)
    gSoloud:setVolume(gMusicbusHandle, t * 2)
  end,
  value = 50
}
utils.NewLine(10)

utils.Slider{
  label = "Sfx bus volume",
  action = function(t)
    gSoloud:setVolume(gSfxbusHandle, t * 2)
  end,
  value = 50
}

utils.End()

--
--
--

local speechtick, speechcount = 0, 0
  
patterns.Loop(function(event)
  if speechtick < event.time then
    local h = gSpeechbus:play(gSpeech[speechcount % 10 + 1], { volume = random(200) / 50 + 2, pan = random(20) / 10 - 1 })

    gSoloud:setRelativePlaySpeed(h, random(100) / 200 + .75)
    gSoloud:fadePan(h, random(20) / 10 - 1, 2)

    speechtick, speechcount = event.time + 4000, speechcount + 1
  end

  --
  --
  --
  
  patterns.UpdateOutput(gSoloud, histo, wave, active_voices)

  speech_bus_volume.text = utils.PercentText("Speech bus volume    : %d%%", gSoloud:getVolume(gSpeechbusHandle))
  music_bus_volume.text = utils.PercentText("Music bus volume    : %d%%", gSoloud:getVolume(gMusicbusHandle))
  sfx_bus_volume.text = utils.PercentText("Sfx bus volume : %d%%", gSoloud:getVolume(gSfxbusHandle))
end)