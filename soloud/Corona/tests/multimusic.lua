--- Port of "megademo/multimusic".

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
local gMusic1, gMusic2 = soloud.createWavStream(), soloud.createWavStream()

--
--
--

gMusic1:load("audio/plonk_wet.ogg")
gMusic2:load("audio/plonk_dry.ogg")

gMusic1:setLooping(true)
gMusic2:setLooping(true)

local gMusichandle1 = gSoloud:play(gMusic1, { volume = 0, paused = true })
local gMusichandle2 = gSoloud:play(gMusic2, { paused = true })
local grouphandle = gSoloud:createVoiceGroup()

gSoloud:addVoiceToGroup(grouphandle, gMusichandle1)
gSoloud:addVoiceToGroup(grouphandle, gMusichandle2)

gSoloud:setProtectVoice(grouphandle, true) -- protect all voices in group 
gSoloud:setPause(grouphandle, false) -- unpause all voices in group 
gSoloud:destroyVoiceGroup(grouphandle) -- remove group, leaves voices alone

--
--
--

local function LeftButton (label, action, extra)
  utils.Button{ label = label, action = action, width = 275 }
  utils.NewLine(extra)
end

--
--
--

utils.Begin("Control", 50, 50)

LeftButton(
  "Fade to music 1",
  function()
    gSoloud:fadeVolume(gMusichandle1, 1, 2)
    gSoloud:fadeVolume(gMusichandle2, 0, 2)
  end
)

LeftButton(
  "Fade to music 2",
  function()
    gSoloud:fadeVolume(gMusichandle2, 1, 2)
    gSoloud:fadeVolume(gMusichandle1, 0, 2)
  end
)

LeftButton(
  "Fade music out",
  function()
    gSoloud:fadeVolume(gMusichandle2, 0, 2)
    gSoloud:fadeVolume(gMusichandle1, 0, 2)
  end
)

LeftButton(
  "Fade music speed down",
  function()
    gSoloud:fadeRelativePlaySpeed(gMusichandle1, 0.2, 5)
    gSoloud:fadeRelativePlaySpeed(gMusichandle2, 0.2, 5)
  end
)

LeftButton(
  "Fade music speed to normal",
  function()
    gSoloud:fadeRelativePlaySpeed(gMusichandle1, 1, 5)
    gSoloud:fadeRelativePlaySpeed(gMusichandle2, 1, 5)
  end
)

LeftButton(
  "Fade music speed up",
  function()
    gSoloud:fadeRelativePlaySpeed(gMusichandle1, 1.5, 5)
    gSoloud:fadeRelativePlaySpeed(gMusichandle2, 1.5, 5)
  end
)

LeftButton(
  "Main resampler pointsample",
  function()
    gSoloud:setMainResampler("POINT")
  end
)

LeftButton(
  "Main resampler linear",
  function()
    gSoloud:setMainResampler("LINEAR")
  end
)

LeftButton(
  "Main resampler catmullrom",
  function()
    gSoloud:setMainResampler("CATMULLROM")
  end, 5
)

local sep = utils.Separator(5)

patterns.AddSFXRButtons(gSoloud, gSfx, 250, 50)

sep.width = utils.GetColumnWidth()

utils.End()

--
--
--

local function SetPositionText (str, pos)
  str.text = utils.PercentText("Music position   : %d%% (%3.3fs/%3.3fs)", pos / gMusic1:getLength(), pos, gMusic1:getLength())
end

--
--
--

utils.Begin("Output", 475, 50)

local histo, wave = patterns.MakeHistogramAndWave()
local music1_volume = utils.Text()
local music2_volume = utils.Text()
local music_rel_speed = utils.Text()
local music_position = utils.Text()
local active_voices = utils.Text()

SetPositionText(music_position, gMusic1:getLength()) -- this is the widest element, so make it (approximately) as wide as it gets

utils.End()

--
--
--

patterns.Loop(function()
  patterns.UpdateOutput(gSoloud, histo, wave, active_voices)

  --
  --
  --

  music1_volume.text = utils.PercentText("Music1 volume    : %d%%", gSoloud:getVolume(gMusichandle1))
  music2_volume.text = utils.PercentText("Music2 volume    : %d%%", gSoloud:getVolume(gMusichandle2))
  music_rel_speed.text = utils.PercentText("Music rel. speed : %d%%", gSoloud:getRelativePlaySpeed(gMusichandle2))

  SetPositionText(music_position, gSoloud:getStreamPosition(gMusichandle2))
end)