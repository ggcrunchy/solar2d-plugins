--- Port of "megademo/thebutton".

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
local setmetatable = setmetatable

-- Modules --
local patterns = require("patterns")
local utils = require("utils")

-- Plugins --
local soloud = require("plugin.soloud")

-- Solar2D globals --
local native = native
local system = system

--
--
--

local RadioSet = {}

RadioSet.__index = RadioSet

--
--
--

function RadioSet:init (core, bus)
  self.Soloud, self.Bus = core, bus

  return true
end

--
--
--

function RadioSet:setAck (source, ack_length)
  self.Ack, self.AckLength = source, ack_length

  return true
end

--
--
--

function RadioSet:clearAck ()
  self.Ack = nil

  return true
end

--
--
--

function RadioSet:attach (source)
  for i = 1, #self.Source do
    if source == self.Source[i] then
      return false, "INVALID_PARAMETER"
    end
  end

  self.Source[#self.Source + 1] = source

  return true
end

--
--
--

function RadioSet:play (source)
  -- try to attach just in case we don't already have this
	self:attach(source)

  local sources, found = self.Source

	for i = 1, #sources do
    if self.Soloud:countAudioSource(sources[i]) > 0 then
      self.Soloud:stopAudioSource(sources[i])

      found = true
    end
  end

  local delay = 0

  if self.Ack and found then
    if self.Bus then
      self.Bus:play(self.Ack)
    else
      self.Soloud:play(self.Ack)
    end

    delay = self.AckLength
  end

	local res

	if self.Bus then
    res = self.Bus:play(source, { volume = -1, pan = 0, paused = true })
  else
    res = self.Soloud:play(source, { volume = -1, pan = 0, paused = true })
	end

  -- delay the sample by however long ack is
	self.Soloud:setDelaySamples(res, delay)
  self.Soloud:setPause(res, false)

  return res
end
    
--
--
--

local gSoloud = soloud.createCore{ flags = { "CLIP_ROUNDOFF", "ENABLE_VISUALIZATION" } }

local gRadioSet = setmetatable({ Source = {}, AckLength = 0 }, RadioSet)

local gPhrase = {}

for i, name in ipairs{
  "audio/thebutton/button1.mp3",
  "audio/thebutton/button2.mp3",
  "audio/thebutton/button3.mp3",
  "audio/thebutton/button4.mp3",
  "audio/thebutton/button5.mp3",
  "audio/thebutton/cough.mp3",
  "audio/thebutton/button6.mp3",
  "audio/thebutton/button7.mp3",
  "audio/thebutton/button1.mp3",
  "audio/thebutton/sigh.mp3",
  "audio/thebutton/thankyou.mp3",
  "audio/thebutton/ack.ogg"
} do
  gPhrase[i] = soloud.createWav()

  gPhrase[i]:load(name)
end

--
--
--

gRadioSet:init(gSoloud, nil)

local PhraseCount = #gPhrase

for i = 1, PhraseCount - 1 do
  gRadioSet:attach(gPhrase[i])
end

gRadioSet:setAck(gPhrase[PhraseCount], gPhrase[PhraseCount].SampleCount)

--
--
--

utils.Begin("Control", 50, 50)

local gNextEvent = 0

utils.Button{
  label = "The button",
  action = function()
    gRadioSet:play(gPhrase[PhraseCount - 1])

    gNextEvent = system.getTimer() + 5000
  end,
  font = native.systemFontBold, font_size = 30,
  width = 300, height = 300
}

utils.End()

--
--
--

utils.Begin("Output", 450, 50)

local histo, wave = patterns.MakeHistogramAndWave()
local active_voices = utils.Text()

utils.Text("Thanks to Anthony Salter for\nvoice acting!")

utils.End()

--
--
--

local gCycles = 0

patterns.Loop(function(event)
  if event.time > gNextEvent then
    gRadioSet:play(gPhrase[gCycles + 1])

    gNextEvent = event.time + 5000
    gCycles = (gCycles + 1) % (PhraseCount - 3)
  end

  --
  --
  --

  patterns.UpdateOutput(gSoloud, histo, wave, active_voices)
end)