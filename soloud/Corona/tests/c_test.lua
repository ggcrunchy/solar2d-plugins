--- Port of C test.

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
local print = print
local random = math.random
local sin = math.sin

-- Modules --
local utils = require("utils")

-- Plugins --
local soloud = require("plugin.soloud")

-- Solar2D globals --
local display = display
local timer = timer

--
--
--

local gSoloud = soloud.createCore{ flags = { "CLIP_ROUNDOFF", "ENABLE_VISUALIZATION" } }
local speech = soloud.createSpeech()

--
--
--

utils.Begin("vanilla")

local action = utils.Text()

action:translate(50, 50)

utils.End()

--
--
--

speech:setText("1 2 3       A B C        Doooooo    Reeeeee    Miiiiii    Faaaaaa    Soooooo    Laaaaaa    Tiiiiii    Doooooo!")

gSoloud:setGlobalVolume(4)
gSoloud:play(speech)

action.text = "Playing speech test.."

--
--
--

utils.Begin("Visualization", 50, 150)

  --
  --
  --

  utils.Begin("scrollview", 600, "hide")

  local pad = utils.Text((" "):rep(59))
  local sv = utils.End()

  --
  --
  --

utils.End()

pad:removeSelf()

--
--
--

local spin = 0

local chars = { '|', '\\', '-', '/' }

local function VisualizeVolume ()
  local v = gSoloud:getApproximateVolume(1)

  spin = (spin % 4) + 1

  local p = floor(v * 60)

  if p > 59 then
    p = 59
  end

  utils.Text(chars[spin] .. ("="):rep(p))
end

--
--
--

utils.Begin("existing", sv)
utils.Text("Speech data:\n")

local QueueTest

timer.performWithDelay(50, function(event)
  if gSoloud:getVoiceCount() == 0 then
    timer.cancel(event.source)

    action.text = "Finished."

    speech:destroy()

    return QueueTest()
  else
    VisualizeVolume()
  end
end, 0)

--
--
--

local function GenerateSample (buf, count)
  local base = count

	for i = 1, #buf do
		local v = sin(220 * 3.14 * 2 * base * (1 / 44100.0)) -
			     sin(230 * 3.14 * 2 * base * (1 / 44100.0))

		v = v + ((random(0, 1023) - 512) / 512.0) *
			      sin(60 * 3.14 * 2 * base * (1 / 44100.0)) *
			      sin(1 * 3.14 * 2 * base * (1 / 44100.0))

		local fade = (44100 * 10 - base) / (44100 * 10.0)

		v = v * fade * fade

    buf:setAt(i, v)

    base = base + 1
  end

  return base
end

--
--
--

function QueueTest ()
	local count, cycle = 0, 0
  local queue = soloud.createQueue()
  local wavs = {}

  for _ = 1, 4 do
    wavs[#wavs + 1] = soloud.createWav()
  end

  local buf = soloud.createFloatBuffer(2048)

  buf:zero()
	gSoloud:play(queue)

  for i = 1, 4 do
    count = GenerateSample(buf, count)

    wavs[i]:loadRawWave(buf)
    queue:play(wavs[i])
	end

	action.text = "Playing queue / wav generation test.."

  utils.Text("\nQueue / wav data:\n")

  --
  --
  --

  timer.performWithDelay(50, function(event)
    if gSoloud:getVoiceCount() == 0 then
      timer.cancel(event.source)

    	for i = 1, 4 do
        wavs[i]:destroy()
      end

      queue:destroy()

      utils.End()

      action.text = "Cleanup done."
    else
      while count < 44100 * 10 and queue:getQueueCount() < 3 do
        count = GenerateSample(buf, count)

        wavs[cycle + 1]:loadRawWave(buf)
        queue:play(wavs[cycle + 1])

        cycle = (cycle + 1) % 4
      end

      VisualizeVolume()
    end
  end, 0)
end