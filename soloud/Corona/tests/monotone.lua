--- Port of "megademo/monotone".

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
local gMusic = soloud.createMonotone()
local gBiquad = soloud.createBiquadResonantFilter()
local gLofi = soloud.createLofiFilter()
local gEcho = soloud.createEchoFilter()
local gDCRemoval = soloud.createDCRemovalFilter()

--
--
--

gMusic:load("audio/Jakim - Aboriginal Derivatives.mon")
gMusic:setParams{ hardwareChannels = 10 }

gEcho:setParams{ delay = 0.2, decay = 0.5, filter = 0.05 }
gBiquad:setParams{ type = "LOWPASS", frequency = 4000, resonance = 2 }

gMusic:setLooping(true)
gMusic:setFilter(1, gBiquad)
gMusic:setFilter(2, gLofi)
gMusic:setFilter(3, gEcho)
gMusic:setFilter(4, gDCRemoval)

local gMusichandle = gSoloud:play(gMusic)
local hwchannels, waveform = 4, "SAW"

gMusic:setParams{ hardwareChannels = hwchannels, waveform = waveform }

--
--
--

local filter_params = {
  {
    ["Filter.WET"] = 0,
    ["BiquadResonantFilter.FREQUENCY"] = 1000,
    ["BiquadResonantFilter.RESONANCE"] = 2
  },

  {
    ["Filter.WET"] = 0,
    ["LofiFilter.SAMPLERATE"] = 8000,
    ["LofiFilter.BITDEPTH"] = 3
  },

  {
    ["Filter.WET"] = 0
  },

  {
    ["Filter.WET"] = 0
  }
}

--
--
--

local function WaveformButton (label, what, enabled)
  utils.RadioButton{
    label = label,
    action = function(on)
      if on then
        waveform = what or label:upper()

        gMusic:setParams{ hardwareChannels = hwchannels, waveform = waveform }
      end
    end,
    enabled = enabled
  }
  utils.NewLine()
end

--
--
--

local function FilterParamSlider (label, param, attrib, low, high)
  utils.Slider{
    label = label,
    action = function(t)
      param[attrib] = (1 - t) * low + t * high
    end,
    value = (param[attrib] - low) * 100 / (high - low)
  }
  utils.NewLine()
end

--
--
--

utils.Begin("Control", 50, 20)
utils.Text("Channels")

  --
  --
  --

  utils.Begin("subgroup")

  for i = 1, 4 do
    utils.RadioButton{
      label = i,
      action = function(on)
        if on then
          hwchannels = i

          gMusic:setParams{ hardwareChannels = hwchannels, waveform = waveform }
        end
      end,
      enabled = i == 1
    }
  end

  utils.End()
  utils.NewLine()

  --
  --
  --

  local waveform_group = utils.Begin("collapsible", "Waveform")

  waveform_group.isVisible = false -- start collapsed

  --
  --
  --

    utils.Begin("subgroup")

    WaveformButton("Square")
    WaveformButton("Saw", nil, true)
    WaveformButton("Sin")
    WaveformButton("Bounce")
    WaveformButton("Jaws")
    WaveformButton("Humps")
    WaveformButton("Fourier square", "FSQUARE")
    WaveformButton("Fourier saw", "FSAW")

    utils.End()
    
  --
  --
  --

  utils.End()
  utils.NewLine(5)

local seps = {}

seps[#seps + 1] = utils.Separator()

utils.Text("Biquad filter (lowpass)", 15)

FilterParamSlider("Wet", filter_params[1], "Filter.WET", 0, 1)
FilterParamSlider("Frequency", filter_params[1], "BiquadResonantFilter.FREQUENCY", 0, 8000)
FilterParamSlider("Resonance", filter_params[1], "BiquadResonantFilter.RESONANCE", 1, 20)

seps[#seps + 1] = utils.Separator()

utils.Text("Lofi filter", 15)

FilterParamSlider("Wet", filter_params[2], "Filter.WET", 0, 1)
FilterParamSlider("Rate", filter_params[2], "LofiFilter.SAMPLERATE", 1000, 8000)
FilterParamSlider("Bit Depth", filter_params[2], "LofiFilter.BITDEPTH", 0, 8)

seps[#seps + 1] = utils.Separator()

utils.Text("Echo filter", 15)

FilterParamSlider("Wet", filter_params[3], "Filter.WET", 0, 1)

seps[#seps + 1] = utils.Separator()

utils.Text("DC removal filter", 15)

FilterParamSlider("Wet", filter_params[4], "Filter.WET", 0, 1)

local width = utils.GetColumnWidth()

for i = 1, #seps do
  seps[i].width = width
end

utils.End()

--
--
--

utils.Begin("Output", 450, 20)

local histo, wave = patterns.MakeHistogramAndWave()
local music_volume = utils.Text()
local active_voices = utils.Text()

utils.End()

--
--
--

patterns.Loop(function()
  patterns.UpdateFilterParams(gSoloud, gMusichandle, filter_params)

  --
  --
  --
  
  patterns.UpdateOutput(gSoloud, histo, wave, active_voices)

  music_volume.text = utils.PercentText("Music volume     : %d%%", gSoloud:getVolume(gMusichandle))
end)