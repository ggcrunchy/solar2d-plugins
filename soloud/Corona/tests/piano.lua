--- Port of "piano" demo.

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
local abs = math.abs
local cos = math.cos
local exp = math.exp
local fmod = math.fmod
local huge = math.huge
local pow = math.pow
local random = math.random
local setmetatable = setmetatable
local sin = math.sin

-- Modules --
local patterns = require("patterns")
local utils = require("utils")

-- Plugins --
local bit = require("plugin.bit")
local soloud = require("plugin.soloud")

-- Solar2D globals --
local display = display
local Runtime = Runtime
local native = native

--
--
--

local kgroup = display.newGroup()

--
--
--

local ADSR = {}

ADSR.__index = ADSR

--
--
--

function ADSR:val (t, rel_time)
  if t < self.A then
    return t / self.A
  end

  t = t - self.A

  if t < self.D then
    return 1 - (t / self.D) * (1 - self.S)
  end

  t = t - self.D

  if t < rel_time then
    return self.S
  end

  t = t - rel_time

  if t >= self.R then
    return 0
  end

  return (1 - t / self.R) * self.S
end

--
--
--

local function MakeADSR (a, d, s, r)
  if a then
    return setmetatable({ A = a, D = d, S = s, R = r }, ADSR)
  else
    return setmetatable({ A = 0, D = 0, S = 1, R = 0 }, ADSR)
  end
end

--
--
--

local Basicwave = {}

--
--
--

function Basicwave:setFreq (freq, superwave)
  self.freq = freq / self.BaseSamplerate
  self.superwave = superwave
end

--
--
--

function Basicwave:setSamplerate (rate)
  self.BaseSamplerate = rate
  self.freq = 440 / self.BaseSamplerate
end

--
--
--

function Basicwave:setWaveform (waveform)
  self.waveform = waveform
end

--
--
--

local BasicwaveParams = {
  class = Basicwave,

  --
  --
  --

  init = function(self)
    self.waveform = "SQUARE"
    self.superwave = false
    self.superwave_scale = .25
    self.superwave_detune = 1

    local adsr = MakeADSR()

    self.A, self.D, self.S, self.R = adsr.A, adsr.D, adsr.S, adsr.R

    self:setSamplerate(44100)
  end,

  --
  --
  --

  newInstance = function(parent_data)
    return { parent = parent_data, adsr = MakeADSR(), offset = 0, freq = parent_data.freq, t = 0 }
  end,

  wantParentData = true,

  --
  --
  --

  getAudio = function(self, buffer, samples, _, data)
    local parent = data.parent
		local waveform = parent.waveform
		local d = 1.0 / self.Samplerate
    local ADSR = data.adsr
    local freq, offset, t = data.freq, data.offset, data.t

    ADSR.A, ADSR.D, ADSR.S, ADSR.R = parent.A, parent.D, parent.S, parent.R

		if not parent.superwave then
			for i = 1, samples do
				buffer:setAt(i, soloud.generateWaveform(waveform, fmod(freq * offset, 1.0)) * ADSR:val(t, huge))

				offset, t = offset + 1, t + d
			end
		else
      local superwave_detune, superwave_scale = parent.superwave_detune, parent.superwave_scale

			for i = 1, samples do
        local f = freq * offset

				buffer:setAt(i, soloud.generateWaveform(waveform, fmod(f, 1.0)) * ADSR:val(t, huge))

				for _ = 1, 3 do
					f = f * 2

					local v = soloud.generateWaveform(waveform, fmod(superwave_detune * f, 1.0)) * ADSR:val(t, huge) * superwave_scale

          buffer:setAt(i, buffer:getAt(i) + v)
				end

				offset, t = offset + 1, t + d
			end
		end

    data.offset, data.t = offset, t

		return samples
  end
}

--
--
--

local PADsynth = {}

PADsynth.__index = PADsynth

--
--
--

-- set the amplitude of the n'th harmonic
function PADsynth:setharmonic (n, value)
  if n >= 2 and n <= #self.harmonics then
    self.harmonics[n] = value
  end
end

--
--
--

-- get the amplitude of the n'th harmonic
function PADsynth:getharmonic (n)
  if n < 2 or n > #self.harmonics then
    return 0.0
  else
    return self.harmonics[n]
  end
end

--
--
--

--[[
  generates the wavetable
    f		- the fundamental frequency (eg. 440 Hz)
    bw		- bandwidth in cents of the fundamental frequency (eg. 25 cents)
    bwscale	- how the bandwidth increase on the higher harmonics (recomanded value: 1.0)
    *smp	- a pointer to allocated memory that can hold N samples
]]
function PADsynth:synth (f, bw, bwscale, smp)
  local freq_amp, harmonics, sample_count = self.freq_amp, self.harmonics, self.sample_count
  
  for i = 1, sample_count / 2 do
    freq_amp[i] = 0.0 -- default, all the frequency amplitudes are zero
  end

  for nh = 2, #harmonics do
    local rF = f * self:relF(nh)

    -- bandwidth of the current harmonic measured in Hz
    local bw_Hz = (pow(2.0, bw / 1200.0) - 1.0) * f * pow(self:relF(nh), bwscale)

    local bwi = bw_Hz / (2.0 * self.samplerate)
    local fi = rF / self.samplerate

    for i = 1, sample_count / 2 do
      -- here you can optimize, by avoiding to compute the profile for the full frequency (usually it's zero or very close to zero)
      local hprofile = self:profile((i - 1) / sample_count - fi, bwi)

      freq_amp[i] = freq_amp[i] + hprofile * harmonics[nh]
    end
  end

  -- Convert the freq_amp array to complex array (real/imaginary) by making the phases random
  local j = 0

  for i = 1, self.sample_count / 2 do
    local phase = random() * 2.0 * 3.14159265358979

    smp:setAt(j + 1, freq_amp[i] * cos(phase))
    smp:setAt(j + 2, freq_amp[i] * sin(phase))

    j = j + 2
  end

  smp:ifft(self.sample_count)

  -- normalize the output
  local max = 0.0

  for i = 1, sample_count do
    local amp = abs(smp:getAt(i))

    if amp > max then
      max = amp
    end
  end

  if max < 0.000001 then
    max = 0.000001
  end

  max = max * .5

  for i = 1, sample_count do
    smp:setAt(i, smp:getAt(i) / max)
  end
end

--
--
--

--[[
  This method returns the N'th overtone's position relative 
  to the fundamental frequency.
  By default it returns N.
  You may override it to make metallic sounds or other 
  instruments where the overtones are not harmonic.
]]
function PADsynth:relF (N)
  return N
end

--
--
--

--[[
  This is the profile of one harmonic
  In this case is a Gaussian distribution (e^(-x^2))
        The amplitude is divided by the bandwidth to ensure that the harmonic
  keeps the same amplitude regardless of the bandwidth
]]
function PADsynth:profile (fi, bwi)
  local x = fi / bwi

  x = x * x

  if x > 14.71280603 then
    return 0 -- this avoids computing the e^(-x^2) where its results are very close to zero
  else
    return exp(-x) / bwi
  end
end

--
--
--

--[[
  N                - is the samplesize (eg: 262144)
  samplerate 	 - samplerate (eg. 44100)
  number_harmonics - the number of harmonics that are computed
]]
local function MakePADsynth (N, samplerate, number_harmonics)
  local harmonics = {} -- Amplitude of the harmonics

	for i = 1, number_harmonics do
		harmonics[i] = 0.0
  end

	harmonics[2] = 1.0 -- default, the first harmonic has the amplitude 1.0

  return setmetatable({
    sample_count = N, samplerate = samplerate,
    freq_amp = {}, -- Amplitude spectrum
    harmonics = harmonics
  }, PADsynth)
end

--
--
--

local function GeneratePadsynth (target, harmonic_count, harmonics, bandwidth, bandwidth_scale, principal_freq, sample_rate, size_pow)
  bandwidth, bandwidth_scale = bandwidth or .25, bandwidth_scale or 1
  principal_freq = principal_freq or 440
  sample_rate = sample_rate or 44100
  size_pow = size_pow or 18

  if harmonic_count > 0 and harmonics and size_pow >= 8 and size_pow <= 24 then
		local len = bit.lshift(1, size_pow)
    local buf = soloud.createFloatBuffer(len)
  
    local p = MakePADsynth(len, sample_rate, harmonic_count)

    for i = 1, harmonic_count do
      p:setharmonic(i, harmonics[i])
    end

		p:synth(principal_freq, bandwidth, bandwidth_scale, buf)

		target:loadRawWave(buf, len, sample_rate)
		target:setLooping(true)
  end
end

--
--
--

local gSoloud = soloud.createCore{ flags = { "CLIP_ROUNDOFF", "ENABLE_VISUALIZATION" } }
local gLoadedWave, gPadsynth = soloud.createWav(), soloud.createWav()
local gBus = soloud.createBus()

local gFilter = patterns.CreateFilterList()

--
--
--

local gWave = soloud.createCustomSource(BasicwaveParams)

--
--
--

gSoloud:setGlobalVolume(.75)
gSoloud:setPostClipScaler(.75)

--[[local bushandle = ]]gSoloud:play(gBus) -- see note in timer, at bottom

gLoadedWave:load("audio/AKWF_c604_0024.wav")
gLoadedWave:setLooping(true)

--
--
--

local harm = { 0.7, 0.3, 0.2, 1.7, 0.4, 1.3, 0.2 }
local bw = { value = 0.25 } -- wrapped for DragFloat...
local bws = { value = 1.0 } -- ...ditto

GeneratePadsynth(gPadsynth, 7, harm, bw.value, bws.value)

--
--
--

local SynthEngines = {
  "Basic wave",
  "Padsynth",
  "Basic sample",
  "Superwave"
}

local gSynthEngine

--
--
--

local gPlonked = {}

for i = 1, 128 do
  gPlonked[i] = { handle = 0, rel = 0 }
end

--
--
--

local gAttack = 0.02

local function plonk (tick, rel, vol)
	local p

  for i = 1, #gPlonked do
    if gPlonked[i].handle == 0 then
      p = gPlonked[i]

      break
    end
  end
  
  if not p then
    return
  end

  vol = vol or 0x50
	vol = (vol + 10) / (0x7f + 10)
	vol = vol * vol

	local pan, handle = sin(tick * 0.0234) -- TODO: use pan?

	if gSynthEngine == SynthEngines[2] then
		handle = gBus:play(gPadsynth, { volume = 0 })

		gSoloud:setRelativePlaySpeed(handle, 2 * rel)
	elseif gSynthEngine == SynthEngines[3] then
		handle = gBus:play(gLoadedWave, { volume = 0 })

		gSoloud:setRelativePlaySpeed(handle, 2 * rel)
	elseif gSynthEngine == SynthEngines[4] then
		gWave:setFreq(440.0 * rel * 2, true)
    
		handle = gBus:play(gWave, { volume = 0 })
	else
		gWave:setFreq(440.0 * rel * 2)

		handle = gBus:play(gWave, { volume = 0 })
  end

	gSoloud:fadeVolume(handle, vol, gAttack)

	p.handle = handle
	p.rel = rel
end

--
--
--

local gRelease = { value = 0.5 } -- wrapped for DragFloat

local function unplonk (rel)
	local p

  for i = 1, #gPlonked do
    if gPlonked[i].rel == rel then
      p = gPlonked[i]

      break
    end
  end

	if p then
    gSoloud:fadeVolume(p.handle, 0, gRelease.value)
    gSoloud:scheduleStop(p.handle, gRelease.value)

    p.handle = 0
  end
end

--
--
--

utils.Begin("Waveform", 450, 200)

local Waveforms = { "SQUARE", "SAW", "SIN", "TRIANGLE", "BOUNCE", "JAWS", "HUMPS", "FSQUARE", "FSAW" }

utils.Text("Wave")

utils.GetLayout():RebaseY()

utils.List{
  labels = {
    "Square wave",
    "Saw wave",
    "Sine wave",
    "Triangle wave",
    "Bounce wave",
    "Jaws wave",
    "Humps wave",
    "Antialized square wave",
    "Antialiazed saw wave"
  },

  on_select = function(index)
    gWave:setWaveform(Waveforms[index])
  end,

  extra = 10
}

utils.NewColumn(5)

for _, v in ipairs{
  { object = gWave, key = "A", text = "Attack" },
  { object = gWave, key = "D", text = "Decay" },
  { object = gWave, key = "S", text = "Sustain" },
  { object = gRelease, key = "value", text = "Release" }
} do
  utils.DragFloat(v.object, v.key, 200, 50, .01, v.text)
  utils.NewLine(5)
end

utils.NewLine(10)

  --
  --
  --

  local SuperwaveWindow = utils.Begin("subgroup")

  local sep = utils.Separator()

  utils.Text("Superwave")
  utils.DragFloat(gWave, "superwave_scale", 200, 50, .01, "Scale")
  utils.NewLine(5)
  utils.DragFloat(gWave, "superwave_detune", 200, 50, .001, "Detune")

  sep.width = utils.GetColumnWidth()

  utils.End()
  
  --
  --
  --

local WaveformWindow = utils.End()

WaveformWindow:Toggle()

--
--
--

utils.Begin("Output", 450, 20)

local histo, wave = patterns.MakeHistogramAndWave()
local active_voices = utils.Text()

utils.Text("Use keyboard to play!")
utils.Text("1 2 3   5 6   8 9 0")
utils.Text(" Q W E R T Y U I O P")

local InfoWindow = utils.End()

InfoWindow:Toggle()

--
--
--

local function UpdateInfo ()
  utils.UpdateWave(wave, gSoloud:getWave())
	utils.UpdateHistogram(histo, gSoloud:calcFFT())
   
  active_voices.text = ("Active voices    : %d"):format(gSoloud:getActiveVoiceCount())
end
--
--
--

local function OnDragFloatChange ()
  GeneratePadsynth(gPadsynth, 5, harm, bw.value, bws.value)
end

local function PADDragFloat (label, object, field)
  utils.DragFloat(object, field, 400, 50, .1, label, OnDragFloatChange)
  utils.NewLine(5)
end

--
--
--

utils.Begin("PADsynth", 450, 230)

PADDragFloat("Harmonic 1", harm, 1)
PADDragFloat("Harmonic 2", harm, 2)
PADDragFloat("Harmonic 3", harm, 3)
PADDragFloat("Harmonic 4", harm, 4)
PADDragFloat("Harmonic 5", harm, 5)
PADDragFloat("Harmonic 6", harm, 6)
PADDragFloat("Harmonic 7", harm, 7)
PADDragFloat("Bandwidth", bw, "value")
PADDragFloat("Bandwidth scale", bws, "value")

local PADsynthWindow = utils.End()

PADsynthWindow:Toggle()

--
--
--

local FilterWindow = patterns.AddFilterSelection(gSoloud, gFilter, 450, 125)

FilterWindow:Toggle()

--
--
--

local gPressed, gWasPressed = {}, {}

Runtime:addEventListener("key", function(event)
  local pressed, x = event.phase == "down", event.keyName
  local p = gWasPressed[x]

  gPressed[x] = pressed

	if not gPressed[x] and p then
    unplonk(pow(0.943875, p))

    gWasPressed[x] = false
  end

  return true
end)

--
--
--

local function NOTEKEY (tick, x, p)
	if gPressed[x] and not gWasPressed[x] then
    plonk(tick, pow(0.943875, p))

    gWasPressed[x] = p
  end

  return true
end

--
--
--

local function KeyTouch (event)
  local key, phase = event.target, event.phase

  if phase == "began" then
    display.getCurrentStage():setFocus(key)

    Runtime:dispatchEvent{ name = "key", keyName = key.key_name, phase = "down" }

    if key.is_white then
      key:setFillColor(.9)
    else
      key:setFillColor(.1)
    end

    key.held = true
  elseif key.held and phase ~= "moved" then
    display.getCurrentStage():setFocus(nil)

    if key.is_white then
      key:setFillColor(1)
    else
      key:setFillColor(0)
    end

    Runtime:dispatchEvent{ name = "key", keyName = key.key_name, phase = "up" }

    key.held = false
  end

  return true
end

--
--
--

-- Following example of http://solhsa.com/soloud/examples.html
local BlackKeyWidth, BlackKeyHeight = 35, 150
local WhiteKeyWidth, WhiteKeyHeight = 2 * BlackKeyWidth, 1.6 * BlackKeyHeight

local key_names, black_key = "qwertyuiop", 1

local x, y = display.contentCenterX - (#key_names - 1) * WhiteKeyWidth / 2, display.contentHeight - WhiteKeyHeight - 100

for key in key_names:gmatch(".") do
  local white = display.newRoundedRect(kgroup, x, y, WhiteKeyWidth, WhiteKeyHeight, 12)

  white:addEventListener("touch", KeyTouch)
  white:setStrokeColor(.8)

  white.anchorY, white.strokeWidth = 0, 2

  white.key_name, white.is_white = key, true

  display.newText(kgroup, key:upper(), x, y + WhiteKeyHeight - 20, native.systemFontBold, 35):setFillColor(0)

  if black_key ~= 4 and black_key ~= 7 then
    local black = display.newRoundedRect(kgroup, x - BlackKeyWidth, y, BlackKeyWidth, BlackKeyHeight, 16)

    black:addEventListener("touch", KeyTouch)
    black:setFillColor(0)

    black.anchorY = 0

    black.key_name = black_key .. "" -- stringify it

    display.newText(kgroup, black_key, black.x, y + BlackKeyHeight - 25, native.systemFontBold, 25)
  end

  black_key, x = (black_key + 1) % 10, x + WhiteKeyWidth
end

--
--
--

local SynthWindow = display.newGroup()

SynthWindow:insert(WaveformWindow)
SynthWindow:insert(InfoWindow)

--
--
--

utils.Begin("Master Control", 25, 25)

utils.Text("Synth Engine")

utils.GetLayout():RebaseY()

utils.List{
  labels = {
		"Basic wave",
		"Padsynth",
		"Basic sample",
		"Superwave"
  },

  on_select = function(index)
    gSynthEngine = SynthEngines[index]

    PADsynthWindow.isVisible = false
    WaveformWindow.isVisible = false

    if index == 1 or index == 4 then
      WaveformWindow.isVisible = true
      SuperwaveWindow.isVisible = index == 4
    elseif index == 2 then
      PADsynthWindow.isVisible = true
    end
  end
}

utils.NewColumn(5)

for _, v in ipairs{
  { label = "Synth Window", window = SynthWindow },
  { label = "Info Window", window = InfoWindow },
  { label = "Filter Window", window = FilterWindow }
} do
  local window = v.window

  utils.Checkbox{
    label = v.label,
    action = function(on)
      window.isVisible = on
    end, checked = true
  }
  utils.NewLine()
end

utils.End()

--
--
--

patterns.Loop(function(event)
  -- N.B. The original piano sample is setting some filter parameters each frame with the
  -- the bus's handle. However, no filters are ever assigned to the bus, nor are of the
  -- values ever updated. This seems to have been left over from some copy-pasting, and a
  -- no-op, so is omitted here.

  local tick = event.time

	NOTEKEY(tick, '1', 18) -- F#
	NOTEKEY(tick, 'q', 17) -- G
	NOTEKEY(tick, '2', 16) -- G#
	NOTEKEY(tick, 'w', 15) -- A
	NOTEKEY(tick, '3', 14) -- A#
	NOTEKEY(tick, 'e', 13) -- B
	NOTEKEY(tick, 'r', 12) -- C
	NOTEKEY(tick, '5', 11) -- C#
	NOTEKEY(tick, 't', 10) -- D
	NOTEKEY(tick, '6', 9) -- D#
	NOTEKEY(tick, 'y', 8) -- E
	NOTEKEY(tick, 'u', 7) -- F
	NOTEKEY(tick, '8', 6) -- F#
	NOTEKEY(tick, 'i', 5) -- G
	NOTEKEY(tick, '9', 4) -- G#
	NOTEKEY(tick, 'o', 3) -- A
	NOTEKEY(tick, '0', 2) -- A#
	NOTEKEY(tick, 'p', 1) -- B

  --
  --
  --

  if InfoWindow.isVisible then
    UpdateInfo()
  end
end)