--- Port of "megademo/annex".

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
local gMusic = soloud.createWavStream()
local gBus1, gBus2, gBus3, gBus4 = soloud.createBus(), soloud.createBus(), soloud.createBus(), soloud.createBus()
local gLofi = soloud.createLofiFilter()
local gBiquad = soloud.createBiquadResonantFilter()
local gEcho = soloud.createEchoFilter()
local gVerb = soloud.createFreeverbFilter()

--
--
--

gMusic:load("audio/delphi_loop.ogg")
gMusic:setLooping(true)

gLofi:setParams{ sampleRate = 1000, bitDepth = 6 }
gBiquad:setParams{ type = "HIGHPASS", frequency = 500, resonance = 2 }
gEcho:setParams{ delay = 0.25, decay = 0.9 }

gBus2:setFilter(1, gLofi)
gBus3:setFilter(1, gBiquad)
gBus4:setFilter(1, gEcho)
gBus1:setFilter(1, gVerb)

local gBus1handle = gSoloud:play(gBus1)

gSoloud:play(gBus2)
gSoloud:play(gBus3)
gSoloud:play(gBus4)

local gMusichandle = gBus1:play(gMusic)

--
--
--

utils.Begin("Control", 20, 50)

utils.Button{
  label = "Annex sound to bus 1",
  action = function()
    gBus1:annexSound(gMusichandle)
  end,
  width = 225
}

local gFrozen = false

utils.Button{
  label = "Freeze",
  action = function(button)
    gSoloud:setFilterParameter(gBus1handle, 1, "FreeverbFilter.FREEZE", gFrozen and 0 or 1)
    button:setLabel(gFrozen and "Freeze" or "Thaw")

    gFrozen = not gFrozen
  end
}

for offset, bus in ipairs{ gBus2, gBus3, gBus4 } do
  utils.NewLine()
  utils.Button{
    label = "Annex sound to bus " .. (1 + offset),
    action = function()
      bus:annexSound(gMusichandle)
    end,
    width = 225
  }
end

utils.End()

--
--
--



--
--
--

utils.Begin("Output", 475, 100)

local histo, wave = patterns.MakeHistogramAndWave()
local active_voices = utils.Text()
local bus1_voices = utils.Text()
local bus2_voices = utils.Text()
local bus3_voices = utils.Text()
local bus4_voices = utils.Text()

utils.End()

--
--
--

patterns.Loop(function()
  patterns.UpdateOutput(gSoloud, histo, wave, active_voices)

  bus1_voices.text = ("Bus 1 voices    : %d"):format(gBus1:getActiveVoiceCount())
  bus2_voices.text = ("Bus 2 voices    : %d"):format(gBus2:getActiveVoiceCount())
  bus3_voices.text = ("Bus 3 voices    : %d"):format(gBus3:getActiveVoiceCount())
  bus4_voices.text = ("Bus 4 voices    : %d"):format(gBus4:getActiveVoiceCount()) 
end)