--- Port of "megademo/space".

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
local timer = timer

--
--
--

local gSoloud = soloud.createCore()
local gSpeech = soloud.createSpeech()
local gMod = soloud.createOpenmpt()

local gMusicBus = soloud.createBus()
local gSpeechBus = soloud.createBus()

local gFlanger = soloud.createFlangerFilter()
local gLofi	= soloud.createLofiFilter()
local gReso	= soloud.createBiquadResonantFilter()

--
--
--

gSoloud:setVisualizationEnable(true)
gSoloud:setGlobalVolume(3)
gSoloud:setPostClipScaler(0.75)

gSoloud:play(gSpeechBus)
gSoloud:play(gMusicBus)

gSpeech:setFilter(2, gFlanger)
gSpeech:setFilter(1, gLofi)
gSpeech:setFilter(3, gReso)
gLofi:setParams{ sampleRate = 8000, bitDepth = 4 }
gFlanger:setParams{ delay = 0.002, frequency = 100 }
--	gReso:setParams{ type = "LOWPASS", frequency = 500, resonance = 5 }
gReso:setParams{ type = "BANDPASS", frequency = 1000, resonance = 0.5 }

local Text = [[
  What the alien has to say might
  appear around here if this
  wasn't just a dummy mockup..

  ..........
  This is a demo of getting
  visualization data from different
  parts of the audio pipeline.]]

gSpeech:setText(Text .. ("\n..........\n"):rep(3))
gSpeech:setLooping(true)

local gSpeechhandle = gSpeechBus:play(gSpeech, { volume = 3, pan = -0.25 })

gSoloud:setRelativePlaySpeed(gSpeechhandle, 1.2)

gSoloud:oscillateFilterParameter(gSpeechhandle, 1, "LofiFilter.SAMPLERATE", 2000, 8000, 4)

gMod:load("audio/BRUCE.S3M")
gMusicBus:play(gMod)

gSpeechBus:setVisualizationEnable(true)
gMusicBus:setVisualizationEnable(true)

--
--
--

utils.Begin("Output", 450, 350)

local histo = utils.MakeHistogram(.7, .3, .5)
local music_volume = utils.Text()
local active_voices = utils.Text()

utils.Text("Active voices include 2 audio busses, music and speech.")

utils.End()

--
--
--

utils.Begin("Alien", 50, 50)

utils.GetLayout():AddToRow(display.newImage("graphics/alien.png"), 10)

local message = utils.Text(Text) -- start with the text to make room in the window...
local speech_wave = utils.MakeWave(.1, 1, .1)

utils.End()

message.text = "" -- ...but reset to start

--
--
--

local gLastloop = 0
local gTickofs
  
patterns.Loop(function(event)
  local tick = event.time

  local loop = gSoloud:getLoopCount(gSpeechhandle)

	if loop ~= gLastloop then
    gLastloop = loop
		gTickofs = tick
  end

	local i = 0

  gTickofs = gTickofs or tick -- avoid a big burst of characters at the start

  while i < (tick - gTickofs) / 70 and i < #Text do
    i = i + 1
  end

  message.text = Text:sub(1, i)

  --
  --
  --

  patterns.UpdateOutput(gSoloud, histo, nil, active_voices)

  --
  --
  --

  utils.UpdateWave(speech_wave, gSpeechBus:getWave(), .525)
end)