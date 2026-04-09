--- Port of "megademo/radiogaga".

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
local gIntro = soloud.createSpeech()
local gMusicBus, gSpeechBus = soloud.createBus(), soloud.createBus()
local gSpeechQueue, gMusicQueue = soloud.createQueue(), soloud.createQueue()
local gDuckFilter = soloud.createDuckFilter()

local gSpeechPhrase, gMusicPhrase = {}, {}

--
--
--

local phrase = {
  "............................",
  "in empty eternity",
  "rain pours down from above",
  "Because it is",
  "but that was then",
  "Let everyone know",
  "in the spaces between",
  "than your bitter heart",
  "a cry of seabirds high over",
  "in my head",
  "sit down and please don't move",
  "I don't feel good",
  "Even if time",
  "For all it's worth.",
  "the only sound",
  "a vast thrumming of crickets",
  "voices of angels",
  "don't bother me",
  "but from an early age"
}

--
--
--

local speechBusHandle = gSoloud:play(gSpeechBus, 0.5)

gSoloud:play(gMusicBus)

for i = 1, #phrase do
  gSpeechPhrase[i] = soloud.createSpeech()

  gSpeechPhrase[i]:setText(phrase[i])
end

for i = 1, 10 do
  gMusicPhrase[i] = soloud.createWav()

  local index = i

  if index < 10 then
    index = "0" .. index
  end

  gMusicPhrase[i]:load("audio/9 (102 BPM)_Seq" .. index .. ".wav")
end

--
--
--

gSpeechQueue:setParamsFromAudioSource(gSpeechPhrase[1])

local gSpeechqueuehandle = gSpeechBus:play(gSpeechQueue, { paused = true })

gSoloud:oscillateRelativePlaySpeed(gSpeechqueuehandle, 0.6, 1.4, 4)

gSpeechQueue:play(gSpeechPhrase[random(#phrase)])

gMusicQueue:setParamsFromAudioSource(gMusicPhrase[1])
gMusicBus:play(gMusicQueue)
gMusicQueue:play(gMusicPhrase[1])
gMusicQueue:play(gMusicPhrase[random(2, 9)])

gSpeechBus:setVisualizationEnable(true)
gMusicBus:setVisualizationEnable(true)

gDuckFilter:setParams{ core = gSoloud, listenTo = speechBusHandle }
gMusicBus:setFilter(1, gDuckFilter)

gIntro:setText("Eat, Sleep, Rave, Repeat")
gIntro:setLooping(true)
gIntro:setLoopPoint(1.45)

local gIntrohandle = gSoloud:play(gIntro)

gSoloud:fadeVolume(gIntrohandle, 0, 10)
gSoloud:scheduleStop(gIntrohandle, 10)

--
--
--

utils.Begin("Music", 20, 20)

local music_histo, music_wave = patterns.MakeHistogramAndWave()
local music_queue = utils.Text()
local active_voices = utils.Text()

utils.Text("Active voices include 2 audio busses, music and speech.")

utils.End()

--
--
--

local function SetPhraseText (str, index)
  str.text = ("Speech phrase    : %d, %s"):format(index, phrase[index])
end

--
--
--

utils.Begin("Poet", 450, 350)

local speech_histo, speech_wave = patterns.MakeHistogramAndWave()
local speech_queue = utils.Text()
local speech_phrase = utils.Text()

local max_width, longest = 0

for i = 1, #phrase do
  speech_phrase.text = phrase[i]

  if speech_phrase.width > max_width then
    longest, max_width = i, speech_phrase.width
  end
end

SetPhraseText(speech_phrase, longest) -- use to measure window

utils.End()

speech_phrase.text = "" -- reset

--
--
--

patterns.Loop(function()
  patterns.UpdateOutput(gSoloud, music_histo, music_wave, active_voices)

  music_queue.text = ("Music queue      : %d"):format(gMusicQueue:getQueueCount())

  --
  --
  --

  if gSpeechQueue:getQueueCount() < 2 then
    for _ = 1, 8 do
      gSpeechQueue:play(gSpeechPhrase[random(2, #phrase)])
      gSpeechQueue:play(gSpeechPhrase[1])
    end
  end

  if gSoloud:getPause(gSpeechqueuehandle) and not gSoloud:isValidVoiceHandle(gIntrohandle) then
    gSoloud:setPause(gSpeechqueuehandle, false)
  end

  if gMusicQueue:getQueueCount() < 2 then
    for _ = 1, 4 do
      gMusicQueue:play(gMusicPhrase[random(2, 9)])
    end
  end

  --
  --
  --

  patterns.UpdateOutput(gSpeechBus, speech_histo, speech_wave)

  speech_queue.text = ("Speech queue     : %d"):format(gSpeechQueue:getQueueCount())

  for i = 1, #phrase do
		if gSpeechQueue:isCurrentlyPlaying(gSpeechPhrase[i]) then
      SetPhraseText(speech_phrase, i)

      break
    end
  end
end)