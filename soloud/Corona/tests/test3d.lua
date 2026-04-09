--- Port of "megademo/3dtest".

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
local cos = math.cos
local sin = math.sin

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

local gSoloud = soloud.createCore{ flags = { "CLIP_ROUNDOFF", "ENABLE_VISUALIZATION" } }
local gSfx_mouse, gSfx_orbit = soloud.createSfxr(), soloud.createSfxr()
local gSfx_crazy = soloud.createSpeech()

--
--
--

gSoloud:setGlobalVolume(4)

gSfx_mouse:loadPreset("LASER", 3)
gSfx_mouse:setLooping(true)
gSfx_mouse:set3dMinMaxDistance(1, 200)
gSfx_mouse:set3dAttenuation("EXPONENTIAL_DISTANCE", 0.5)

local gSndHandle_mouse = gSoloud:play3d(gSfx_mouse, 100, 0, 0)

gSfx_orbit:loadPreset("COIN", 3)
gSfx_orbit:setLooping(true)
gSfx_orbit:set3dMinMaxDistance(1, 200)
gSfx_orbit:set3dAttenuation("EXPONENTIAL_DISTANCE", 0.5)

local gSndHandle_orbit = gSoloud:play3d(gSfx_orbit, 50, 0, 0)

gSfx_crazy:setText("I'm going into space with my space ship space ship space ship spaceeeeeeeeeeeeeeeeeeeeeeeeeeeeee")
gSfx_crazy:setLooping(true)
gSfx_crazy:set3dMinMaxDistance(1, 400)
gSfx_crazy:set3dAttenuation("EXPONENTIAL_DISTANCE", 0.25)

local gSndHandle_crazy = gSoloud:play3d(gSfx_crazy, 50, 0, 0)

--
--
--

local CX, CY = display.contentCenterX, display.contentCenterY

--
--
--

local function TwoTriangles (size, r, g, b)
  local shadow = utils.Triangle(size)
  local spaceship = utils.Triangle(size)

  shadow:setFillColor(0, 0x77 / 255)
  shadow:translate(5, 5)
  spaceship:setFillColor(r, g, b)

  return spaceship, shadow
end

local function SetShadow (shadow, x, y)
  shadow.x, shadow.y = x + 5, y + 5
end

local function SetShipPosition (tri, shadow, x, y)
  x, y = CX + x, CY + y

  tri.x, tri.y = x, y

  SetShadow(shadow, x, y)
end

--
--
--

local center_ship, center_ship_shadow = TwoTriangles(40, 0xee, 0xee, 0xee)

SetShipPosition(center_ship, center_ship_shadow, 0, 0)

--
--
--

local ship1, ship1_shadow = TwoTriangles(20, 1, 1, 0)
local ship2, ship2_shadow = TwoTriangles(20, 1, 0, 1)
local ship3, ship3_shadow = TwoTriangles(20, 0, 1, 1)

--
--
--

local function SpaceshipTouch (event)
  local ship, phase = event.target, event.phase

  if phase == "began" then
    display.getCurrentStage():setFocus(ship)

    ship.dx, ship.dy = event.x - ship.x, event.y - ship.y
  elseif ship.dx then
    if phase == "moved" then
      ship.x, ship.y = event.x - ship.dx, event.y - ship.dy

      SetShadow(ship3_shadow, ship3.x, ship3.y)
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

ship3:addEventListener("touch", SpaceshipTouch)

local ShipX, ShipY = 100, 500

ship3.x, ship3.y = ShipX, ShipY

SetShadow(ship3_shadow, ShipX, ShipY)

--
--
--

utils.Begin("Control", 15, 20)

local orbit_enable = utils.Checkbox{ label = "Orbit sound", checked = true }

utils.NewLine()

local crazy_enable = utils.Checkbox{ label = "Crazy sound", checked = true }

utils.NewLine()

local mouse_enable = utils.Checkbox{ label = "Mouse sound", checked = true }

utils.NewLine()
utils.Text[[
Drag the 'spaceship' (blue triangle) around
and 'fight' the other spaceships.]] 

utils.End()

--
--
--

utils.Begin("Output", 475, 20)

local histo, wave = patterns.MakeHistogramAndWave()
local active_voices = utils.Text() 

utils.End()


--
--
--

patterns.Loop(function(event)
  gSoloud:setPause(gSndHandle_crazy, not crazy_enable.isOn)
  gSoloud:setPause(gSndHandle_orbit, not orbit_enable.isOn)
  gSoloud:setPause(gSndHandle_mouse, not mouse_enable.isOn)

  local tick = event.time / 1000

  local crazyx = sin(tick) * sin(tick * 0.234) * sin(tick * 4.234) * 150
  local crazyz = cos(tick) * cos(tick * 0.234) * cos(tick * 4.234) * 150 - 50
  local tickd = tick - 0.1
  local crazyxv = sin(tickd) * sin(tickd * 0.234) * sin(tickd * 4.234) * 150
  local crazyzv = cos(tickd) * cos(tickd * 0.234) * cos(tickd * 4.234) * 150 - 50

  crazyxv = crazyxv - crazyx
  crazyzv = crazyzv - crazyz

	gSoloud:set3dSourceParameters(gSndHandle_crazy, crazyx, 0, crazyz, crazyxv, { vel_y = crazyzv })

  local orbitx = sin(tick) * 50
  local orbitz = cos(tick) * 50
  local orbitxv = sin(tickd) * 50
  local orbitzv = cos(tickd) * 50

  orbitxv = orbitxv - orbitx;
  orbitzv = orbitzv - orbitz;

  gSoloud:set3dSourceParameters(gSndHandle_orbit, orbitx, 0, orbitz, orbitxv, { vel_y = orbitzv })

	local mousex = ship3.x - CX
	local mousez = ship3.y - CY

  gSoloud:set3dSourcePosition(gSndHandle_mouse, mousex, mousez, 0)

	gSoloud:update3dAudio()

  --
  --
  --

  SetShipPosition(ship1, ship1_shadow, orbitx * 2, orbitz * 2)
  SetShipPosition(ship2, ship2_shadow, crazyx * 2, crazyz * 2)

  --
  --
  --

  patterns.UpdateOutput(gSoloud, histo, wave, active_voices)
end)