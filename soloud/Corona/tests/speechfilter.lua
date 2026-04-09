--- Port of "megademo/speechfilter".

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
local pairs = pairs

-- Modules --
local patterns = require("patterns")
local utils = require("utils")

-- Plugins --
local soloud = require("plugin.soloud")

--
--
--

local gSoloud = soloud.createCore{ flags = { "CLIP_ROUNDOFF", "ENABLE_VISUALIZATION" } }
local gSpeech = soloud.createSpeech()
local gVizsn = soloud.createVizsn()
local gBiquad = soloud.createBiquadResonantFilter()
local gLofi = soloud.createLofiFilter()
local gEcho = soloud.createEchoFilter()
local gWaveShaper = soloud.createWaveShaperFilter()
local gDCRemoval = soloud.createDCRemovalFilter()
local gRobotize = soloud.createRobotizeFilter()
local gBus = soloud.createBus()

--
--
--

local hwchannels, waveform = 4, "SAW"

local basefreq = 1330
local basespeed = 10
local basedeclination = 0.5
local basewaveform = "SAW"

--
--
--

gEcho:setParams{ delay = 0.2, decay = 0.5, filter = 0.05 }
gBiquad:setParams{ type = "LOWPASS", frequency = 4000, resonance = 2 }

gSpeech:setLooping(true)
gVizsn:setLooping(true)

gBus:setFilter(1, gWaveShaper)
gBus:setFilter(2, gBiquad)
gBus:setFilter(3, gLofi)
gBus:setFilter(4, gEcho)
gBus:setFilter(5, gDCRemoval)
gBus:setFilter(6, gRobotize)

--
--
--

local filter_params = {
  {
    ["Filter.WET"] = 0,
    ["WaveShaperFilter.AMOUNT"] = 0
  },
  
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
  },

  {
    ["Filter.WET"] = 0
  }
}

--
--
--

local text = [[
  The beige hue on the waters of the loch impressed all, including the French queen, before she heard that symphony again, just as young Arthur wanted. 
  Are those shy Eurasian footwear, cowboy chaps, or jolly earthmoving headgear? 
  Shaw, those twelve beige hooks are joined if I patch a young, gooey mouth. 
  With tenure, Suzie'd have all the more leisure for yachting, but her publications are no good.
]]

gVizsn:setText(text)
gSpeech:setText(text)
gSpeech:setParams{ baseFrequency = floor(basefreq), baseSpeed = basespeed, baseDeclination = basedeclination }

--
--
--

local gBushandle = gSoloud:play(gBus)
local gSpeechhandle = gBus:play(gSpeech)
local gVizsnhandle = gBus:play(gVizsn, { paused = true })

--
--
--

local function Lerp (v1, v2, t)
  return (1 - t) * v1 + t * v2
end

local function GetT (v1, v2, x)
  return (x - v1) * 100 / (v2 - v1)
end

--
--
--

utils.Begin("Filters", 25, 25)

  --
  --
  --

  local wave_shaper = utils.Begin("collapsible", "WaveShaper")

  wave_shaper.isVisible = false

  utils.NewLine(20)
  utils.Slider{
    label = "Wet #1",
    action = function(t)
      filter_params[1]["Filter.WET"] = t
    end
  }
  utils.NewLine(15)
  utils.Slider{
    label = "Amount #1",
    action = function(t)
      filter_params[1]["WaveShaperFilter.AMOUNT"] = Lerp(-1, 1, t)
    end,
    value = GetT(-1, 1, filter_params[1]["WaveShaperFilter.AMOUNT"])
  }

  utils.End()
  utils.NewLine(5)

  --
  --
  --

  local biquad = utils.Begin("collapsible", "Biquad (lowpass)")

  biquad.isVisible = false

  utils.NewLine(20)
  utils.Slider{
    label = "Wet #2",
    action = function(t)
      filter_params[2]["Filter.WET"] = t
    end
  }
  utils.NewLine(15)
  utils.Slider{
    label = "Frequency #2",
    action = function(t)
      filter_params[2]["BiquadResonantFilter.FREQUENCY"] = Lerp(0, 8000, t)
    end,
    value = GetT(0, 8000, filter_params[2]["BiquadResonantFilter.FREQUENCY"])
  }
  utils.NewLine(15)
  utils.Slider{
    label = "Resonance #2",
    action = function(t)
      filter_params[2]["BiquadResonantFilter.RESONANCE"] = Lerp(1, 20, t)
    end,
    value = GetT(1, 20, filter_params[2]["BiquadResonantFilter.RESONANCE"])
  }

  utils.End()
  utils.NewLine(5)

  --
  --
  --

  local lofi = utils.Begin("collapsible", "LoFi")

  lofi.isVisible = false

  utils.NewLine(20)
  utils.Slider{
    label = "Wet #3",
    action = function(t)
      filter_params[3]["Filter.WET"] = t
    end
  }
  utils.NewLine(15)
  utils.Slider{
    label = "Rate #3",
    action = function(t)
      filter_params[3]["LofiFilter.SAMPLERATE"] = Lerp(1000, 8000, t)
    end,
    value = GetT(1000, 8000, filter_params[3]["LofiFilter.SAMPLERATE"])
  }
  utils.NewLine(15)
  utils.Slider{
    label = "Bit depth #3",
    action = function(t)
      filter_params[3]["LofiFilter.BITDEPTH"] = Lerp(0, 8, t)
    end,
    value = GetT(0, 8, filter_params[3]["LofiFilter.BITDEPTH"])
  }
  
  utils.End()
  utils.NewLine(5)

  --
  --
  --

  local echo = utils.Begin("collapsible", "Echo")

  echo.isVisible = false

  utils.NewLine(20)
  utils.Slider{
    label = "Wet #4",
    action = function(t)
      filter_params[4]["Filter.WET"] = t
    end
  }
  
  utils.End()
  utils.NewLine(5)

  --
  --
  --

  local dc_removal = utils.Begin("collapsible", "DC Removal")

  dc_removal.isVisible = false

  utils.NewLine(20)
  utils.Slider{
    label = "Wet #5",
    action = function(t)
      filter_params[5]["Filter.WET"] = t
    end
  }

  utils.End()
  utils.NewLine(5)

  --
  --
  --

  local robotize = utils.Begin("collapsible", "Robotize")

  robotize.isVisible = false

  utils.NewLine(20)
  utils.Slider{
    label = "Wet #6",
    action = function(t)
      filter_params[6]["Filter.WET"] = t
    end
  }
  
  utils.End()

  --
  --
  --

utils.End()

--
--
--

local function OnChange ()
  gSpeech:setParams{
    baseFrequency = floor(basefreq),
    baseSpeed = basespeed,
    baseDeclination = basedeclination,
    baseWaveform = basewaveform
  }
end

--
--
--

local function SetBaseWaveform (on, what)
  if on then
    basewaveform = what

    OnChange()
  end
end

--
--
--

utils.Begin("Speech params", 475, 400)

  --
  --
  --
  
  local engine = utils.Begin("collapsible", "Engine")

  engine.isVisible = false

    --
    --
    --

    utils.Begin("subgroup")

    utils.RadioButton{
      label = "Speech",
      action = function(on)
        gSoloud:setPause(gSpeechhandle, not on)
      end,
      enabled = not gSoloud:getPause(gSpeechhandle)
    }
    utils.RadioButton{
      label = "Vizsn",
      action = function(on)
        gSoloud:setPause(gVizsnhandle, not on)
      end,
      enabled = not gSoloud:getPause(gVizsnhandle)
    }

    utils.End()

    --
    --
    --

  utils.End()
  utils.NewLine(5)
  
  --
  --
  --

  local waveform_group = utils.Begin("collapsible", "Waveform")

  waveform_group.isVisible = false

    --
    --
    --

    utils.Begin("subgroup")

    utils.RadioButton{
      label = "Sin",
      action = function(on)
        SetBaseWaveform(on, "SIN")
      end
    }
    utils.NewLine()
    utils.RadioButton{
      label = "Triangle",
      action = function(on)
        SetBaseWaveform(on, "TRIANGLE")
      end
    }
    utils.NewLine()
    utils.RadioButton{
      label = "Saw",
      action = function(on)
        SetBaseWaveform(on, "SAW")
      end,
      enabled = true
    }
    utils.NewLine()
    utils.RadioButton{
      label = "Square",
      action = function(on)
        SetBaseWaveform(on, "SQUARE")
      end
    }
    utils.NewLine()
    utils.RadioButton{
      label = "Pulse",
      action = function(on)
        SetBaseWaveform(on, "PULSE")
      end
    }
    utils.NewLine()
    utils.RadioButton{
      label = "Warble",
      action = function(on)
        SetBaseWaveform(on, "WARBLE")
      end
    }
    utils.NewLine()
    utils.RadioButton{
      label = "Noise",
      action = function(on)
        SetBaseWaveform(on, "NOISE")
      end
    }

    utils.End()

    --
    --
    --

  utils.End()
  utils.NewLine(5)

  --
  --
  --

  local base_params = utils.Begin("collapsible", "Base params")
  
  base_params.isVisible = false
  
  utils.NewLine(20)
  utils.Slider{
    label = "Base freq",
    action = function(t)
      basefreq = Lerp(0, 3000, t)

      OnChange()
    end,
    value = GetT(0, 3000, basefreq)
  }
  utils.NewLine(20)
  utils.Slider{
    label = "Base speed",
    action = function(t)
      basespeed = Lerp(.1, 30, t)

      OnChange()
    end,
    value = GetT(.1, 30, basespeed)
  }
  utils.NewLine(20)
  utils.Slider{
    label = "Base declination",
    action = function(t)
      basedeclination = Lerp(-3, 3, t)

      OnChange()
    end,
    value = GetT(-3, 3, basedeclination)
  }
  utils.NewLine()
  
  utils.End()

  --
  --
  --

utils.End()

--
--
--

utils.Begin("Output", 475, 20)

local histo, wave = patterns.MakeHistogramAndWave()
local music_volume = utils.Text()
local active_voices = utils.Text()

utils.End()

--
--
--

patterns.Loop(function()
  patterns.UpdateFilterParams(gSoloud, gBushandle, filter_params)

  --
  --
  --

  patterns.UpdateOutput(gSoloud, histo, wave, active_voices)

  music_volume.text = utils.PercentText("Music volume     : %d%%", gSoloud:getVolume(gSpeechhandle))
end)