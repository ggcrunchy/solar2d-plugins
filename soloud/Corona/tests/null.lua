--- Port of "null" demo.

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
local concat = table.concat
local floor = math.floor

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

local gSoloud = soloud.createCore{ backend = "NULLDRIVER" }
local speech = soloud.createSpeech()

--
--
--

speech:setText("Hello!")

gSoloud:setGlobalVolume(10)
gSoloud:play(speech)

--
--
--

local Count = 10

local pos, mixed, dirty = 1, {}

--
--
--

local Began

local AdjustEach, MaxAdjustments = 2500, 15

local function GetStepCount (now)
  local n = floor((now - Began) / AdjustEach)

  if n > MaxAdjustments then
    n = MaxAdjustments
  end

  return 1 + n
end

local function Decrement (event)
  for _ = 1, GetStepCount(event.time) do
    if pos > 1 then
      pos, dirty = pos - 1, true
    else
      return
    end
  end
end

local function Increment (event)
  for _ = 1, GetStepCount(event.time) do
    if pos + Count <= #mixed then
      pos, dirty = pos + 1, true
    else
      return
    end
  end
end

--
--
--

local function ScrollTouch (event)
  local tri, phase = event.target, event.phase

  if phase == "began" then
    Began = event.time

    tri.op(event)

    tri.timer = timer.performWithDelay(30, tri.op, 0)

    display.getCurrentStage():setFocus(tri)
  elseif tri.timer and phase ~= "moved" then
    timer.cancel(tri.timer)

    tri.timer = nil

    display.getCurrentStage():setFocus(nil)
  end

  return true
end

--
--
--

local function AddTriangle (op, how)
  local tri = utils.Triangle{ size = 20, flipped = how == "flipped", scale = 1 }

  tri:addEventListener("touch", ScrollTouch)
  tri:setFillColor(.7)
  tri:setStrokeColor(.9, .7)

  tri.strokeWidth = 1

  tri.op = op

  utils.GetLayout():AddToRow(tri, 5)
end

--
--
--

utils.Begin("Visualization", 50, 50)

AddTriangle(Decrement)

local where = utils.Text()

utils.NewLine()

local rows = { utils.Text((" "):rep(60)) }

for i = 2, Count do
  rows[i] = utils.Text()
end

AddTriangle(Increment, "flipped")

utils.End()

--
--
--

local buffer = soloud.createFloatBuffer(256 * 2)

timer.performWithDelay(50, function(event)
  if gSoloud:getActiveVoiceCount() == 0 then
    timer.cancel(event.source)
  else
    gSoloud:mix(buffer)

    for _, v, _ in buffer:values(2) do
      mixed[#mixed + 1] = floor(v * 30 + 30)
    end

    dirty = true
  end
end, 0)

--
--
--

timer.performWithDelay(50, function()
  if dirty then
    where.text = ("%d - %d (total = %d)"):format(pos, pos + Count - 1, #mixed)

    for offset = 0, Count - 1 do
      local d, row = mixed[pos + offset], {}

			row[#row + 1] = '|'

      for j = 0, 59 do
        if j == d then
          row[#row + 1] = 'o'
				elseif (d < 30 and j < 30 and j > d) or (d >= 30 and j >= 30 and j < d) then
					row[#row + 1] = '-'
				else
					row[#row + 1] = ' '
        end
      end

			row[#row + 1] = '|'

      rows[offset + 1].text = concat(row)
    end

    dirty = false
  end
end, 0)