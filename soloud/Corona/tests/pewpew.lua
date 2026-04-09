--- Port of "megademo/pewpew".

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
local system = system

--
--
--

-- Use a slightly larger audio buffer to exaggarate the effect
local gSoloud = soloud.createCore{
  flags = { "CLIP_ROUNDOFF", "ENABLE_VISUALIZATION" },
  buffersize = 4096
}
local gSfx = soloud.createSfxr()

--
--
--

gSfx:loadPreset("LASER", 3)

--
--
--

local function SpaceshipTouch (event)
  local ship, phase = event.target, event.phase

  if phase == "began" then
    display.getCurrentStage():setFocus(ship)

    ship.dx = event.x - ship.x
  elseif ship.dx then
    if phase == "moved" then
      ship.x = event.x - ship.dx
    else
      display.getCurrentStage():setFocus(nil)

      ship.dx = nil
    end
  end

  return true
end

--
--
--

local CX = display.contentCenterX

local player = utils.Triangle(50)

player.x = CX
player.anchorY, player.y = 1, display.contentHeight

player:addEventListener("touch", SpaceshipTouch)
player:setFillColor(0x33 / 255, 0x99 / 255, 1)

--
--
--

local MAX_BULLETS = 64

local bulletidx = 0

local bullet_size = { width = 5, height = 10 }
local bullet_tris = {}

for i = 1, MAX_BULLETS do
  bullet_tris[i] = utils.Triangle(bullet_size)

  bullet_tris[i]:setFillColor(0x77 / 255, 0x33 / 255, 0x33 / 255)

  bullet_tris[i].isVisible = false
end

--
--
--

utils.Begin("Output", 450, 50)

local histo, wave = patterns.MakeHistogramAndWave()
local active_voices = utils.Text()

utils.Text("----|----|----|----|----|")

local voices_line = utils.Text()

utils.End()

--
--
--

local fire1, fire2, fire3

--
--
--

utils.Begin("Control", 20, 50)

utils.Text("Click to play a single sound:")
utils.Button{
  label = "Play (single)",
  action = function()
    fire1 = true
  end
}

utils.NewLine(5)

local sep = utils.Separator(5)

utils.Text("Checkbox for repeated calls:")

utils.Checkbox{
  label = "Play",
  action = function(on)
    fire2 = on
  end
}
utils.NewLine()
utils.Checkbox{
  label = "PlayClocked",
  action = function(on)
    fire3 = on
  end
}
utils.NewLine()
utils.Text[[
Drag the blue triangle left and right to
position it for a shot]]

sep.width = utils.GetColumnWidth()

utils.End()

--
--
--

local Height = display.contentHeight

local lasttick = system.getTimer()

local x = 0

patterns.Loop(function(event)
  local tick = event.time

  x = (player.x - CX) / CX

  if x < -1 then
    x = -1
  elseif x > 1 then
    x = 1
  end

  gSfx:loadPreset("LASER", 3)

	while lasttick < tick do
    do
      local bullet = bullet_tris[bulletidx + 1]

      if fire1 then
        gSoloud:playClocked(tick / 1000, gSfx, { volume = 1, pan = x })

        bullet.clocked = true
      end

      if fire2 then
        gSoloud:play(gSfx, { volume = 1, pan = x })

        bullet.clocked = false
      end

      if fire3 then
        gSoloud:playClocked(tick / 1000, gSfx, { volume = 1, pan = x })

        bullet.clocked = true
      end

      if fire1 or fire2 or fire3 then
        bullet.x = player.x
        bullet.y = Height - player.contentHeight
        bullet.isVisible = true

        bullet.pan = x
        bullet.vel = -75

        bulletidx = (bulletidx + 1) % MAX_BULLETS
      end
    end

    --
    --
    --

    if fire1 then
      fire1 = false
    end

    --
    --
    --

    for i = 1, MAX_BULLETS do
      local bullet = bullet_tris[i]

      if bullet.isVisible then
        bullet.y = bullet.y + bullet.vel
        bullet.vel = bullet.vel + 2.75
      end

      if bullet.isVisible and bullet.y > Height then
        bullet.isVisible = false

        local v

        if bullet.clocked then
          v = gSoloud:playClocked(lasttick / 1000.0, gSfx, { volume = 1, pan = bullet.pan })
        else
          v = gSoloud:play(gSfx, { volume = 1, pan = bullet.pan })
        end
        
				gSoloud:setRelativePlaySpeed(v, 0.5)
      end
    end

    --
    --
    --

    lasttick = lasttick + 10
  end

  --
  --
  --
  
  patterns.UpdateOutput(gSoloud, histo, wave, active_voices)

  voices_line.text = ("-"):rep(gSoloud:getActiveVoiceCount())
end)