--- Port of "megademo/wavformats".

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

--
--
--

local filenames = {
  "ch1.flac",
  "ch1.mp3",
  "ch1.ogg",
  "ch1_16bit.wav",
  "ch1_24bit.wav",
  "ch1_32bit.wav",
  "ch1_8bit.wav",
  "ch1_alaw.wav",
  "ch1_double.wav",
  "ch1_float.wav",
  "ch1_imaadpcm.wav",
  "ch1_msadpcm.wav",
  "ch1_ulaw.wav",
  "ch2.flac",
  "ch2.mp3",
  "ch2.ogg",
  "ch2_16bit.wav",
  "ch2_24bit.wav",
  "ch2_32bit.wav",
  "ch2_8bit.wav",
  "ch2_alaw.wav",
  "ch2_double.wav",
  "ch2_float.wav",
  "ch2_imaadpcm.wav",
  "ch2_msadpcm.wav",
  "ch2_ulaw.wav",
  "ch4.flac",
  "ch4.ogg",
  "ch4_16bit.wav",
  "ch4_24bit.wav",
  "ch4_32bit.wav",
  "ch4_8bit.wav",
  "ch4_alaw.wav",
  "ch4_double.wav",
  "ch4_float.wav",
  "ch4_imaadpcm.wav",
  "ch4_msadpcm.wav",
  "ch4_ulaw.wav",
  "ch6.flac",
  "ch6.ogg",
  "ch6_16bit.wav",
  "ch6_24bit.wav",
  "ch6_32bit.wav",
  "ch6_8bit.wav",
  "ch6_alaw.wav",
  "ch6_double.wav",
  "ch6_float.wav",
  "ch6_imaadpcm.wav",
  "ch6_msadpcm.wav",
  "ch6_ulaw.wav",
  "ch8.flac",
  "ch8.ogg",
  "ch8_16bit.wav",
  "ch8_24bit.wav",
  "ch8_32bit.wav",
  "ch8_8bit.wav",
  "ch8_alaw.wav",
  "ch8_double.wav",
  "ch8_float.wav",
  "ch8_imaadpcm.wav",
  "ch8_msadpcm.wav",
  "ch8_ulaw.wav"
}

--
--
--

local gSoloud = soloud.createCore{ flags = { "CLIP_ROUNDOFF", "ENABLE_VISUALIZATION" } }
local gWav, gWavOk = {}, {}
local gWavStream, gWavStreamOk = {}, {}

for i = 1, #filenames do
  local name = ("audio/wavformats/%s"):format(filenames[i])

  gWav[i] = soloud.createWav()
  gWavStream[i] = soloud.createWavStream()

  gWavOk[i] = gWav[i]:load(name)
  gWavStreamOk[i] = gWavStream[i]:load(name)
end

--
--
--

utils.Begin("Control", 50, 20)
  utils.Begin("scrollview", display.contentHeight - 150)

  for i = 1, #filenames do
    local wav_button = utils.Button{
      isEnabled = gWavOk[i],
      label = ("Play %s"):format(filenames[i]),
      width = 275,

      action = function()
        gSoloud:play(gWav[i])
      end
    }

    if not gWavOk[i] then
      wav_button:setFillColor(.5)
    end

    --
    --
    --

    local wav_stream_button = utils.Button{
      isEnabled = gWavStreamOk[i],
      label = ("Stream %s"):format(filenames[i]),
      width = 275,

      action = function()
        gSoloud:play(gWavStream[i])
      end
    }

    if not gWavStreamOk[i] then
      wav_stream_button:setFillColor(.5)
    end

    --
    --
    --

    utils.NewLine()
  end

  utils.End()
utils.End()

--
--
--

utils.Begin("Output", 450, 20)

local histo, wave = patterns.MakeHistogramAndWave()
local active_voices = utils.Text()

utils.End()

--
--
--

patterns.Loop(function()
  patterns.UpdateOutput(gSoloud, histo, wave, active_voices)
end)