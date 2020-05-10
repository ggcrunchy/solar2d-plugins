--- Quirky game loop.

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
local floor = math.floor
local ipairs = ipairs
local min = math.min
local rad = math.rad
local sin = math.sin

-- Modules --
local blocks = require("quirky.blocks")
local controls = require("quirky.controls")
local gui = require("quirky.gui")
local info = require("quirky.info")
local levels = require("quirky.levels")
local obstacle = require("quirky.obstacle")
local rewind = require("quirky.rewind")
local states = require("quirky.states")
local turnstiles = require("quirky.turnstiles")

-- Corona globals --
local display = display
local native = native
local Runtime = Runtime
local transition = transition

-- Corona modules --
local widget = require("widget")

-- Exports --
local M = {}

-- Layers of various game objects --
-- These are back-to-front, but in some cases objects in lower layers would preclude others
-- occupying the same cell in higher layers.
local Layers = {
	"m_baked", -- Rects that remain fixed at level load
	"m_ground", -- Rects that may change during gameplay or via rewind
	"m_spots", -- Circles that remain fixed at level load
	"m_middle", -- Rects that may change during gameplay or via rewind (and must cover ground and / or spots)
	"m_pivots", -- Rects that remain fixed at level load
	"m_rivets", -- Circles that remain fixed at level load (and must cover pivots)
	"m_players", -- Circles that may change during gameplay or via rewind (and must cover ground and / or spots)
	"m_hud", -- HUD elements
	"m_overlay" -- Rewind overlay GUI
}

-- Layer name -> cache list (ignores layers left intact on level unload) --
local PerLevelLayers = {
	m_baked = "rects", m_ground = "rects", m_middle = "rects", m_pivots = "rects",
	m_spots = "circles", m_rivets = "circles", m_players = "circles"
}

-- Reference to Quirky scene --
local Scene

-- Accumulator for moving objects --
local Accum = {}

-- The current state of the game, comprising all parts subject to change --
local State

-- Flags set following a move: all Sokoban-style spots covered? current character reached an exit? block sank into a hole? --
local Covered, Exit, Filled

-- Game input
local function DoGame (name)
	-- If requested, try to open the rewind overlay.
	if name == "r" then
		rewind.OpenOverlay(State)

	-- Try to switch characters.
	elseif name == "s" then
		if states.MoreThanOneCharacter(State) then
			rewind.Capture(State)
			states.SwitchCharacter(State)
		end

	-- If no character, block, or turnstile is in motion, try to move.
	elseif info.GetMoveCount() == 0 then
		local dx, dy, di = info.GetStep(name)
		local delay, col, row = nil, states.GetCoordinates(State)
		local how, entry, cw = obstacle.CanMove(State, Accum, col, row, name)

		-- If there was an obstacle, quit. Otherwise, record the current game state, since
		-- we might want to rewind back to it.
		if not how then
			return
		end

		rewind.Capture(State)

		-- If we pushed a block, move it ahead and follow a short time later.
		if how == "block" then
			blocks.Move(Scene, State, entry, Accum, dx, dy, di)

			delay = 50

		-- If we rotated a turnstile, do likewise. In this case, we move either one or two
		-- spaces, depending on whether another part of the turnstile would get rotated into
		-- the spot being vacated.
		elseif how == "turnstile" then
			turnstiles.Move(Scene, State, entry, Accum, cw)

			if State.middle[info.GetIndex(col, row) + di] == entry then
				dx, dy, delay = 2 * dx, 2 * dy, 75
			else
				delay = 120
			end
		end

		-- Clear the spot currently occupied by the character and move ahead. If we land on
		-- an exit, flag that; otherwise, mark the new spot. Disable input at least until the
		-- move ends; the move count is incremented to account for the character, along with
		-- any block or turnstile set in motion.
		State.middle[info.GetIndex(col, row)] = false

		col, row = col + dx, row + dy

		local index = info.GetIndex(col, row)

		if State.ground[index] == "E" then
			Exit = true
		else
			State.middle[index] = "C"
		end

		controls.Enable(false)
		info.IncMoveCount()
		states.SetCoordinates(State, col, row)
		states.Move(states.GetPlayer(Scene), dx, dy, delay)
	end
end

-- Listen for rewind events, replacing the state in that case
info.GetEvents():addEventListener("rewind", function(event)
	State = event.state
end)

-- Load the game levels' info --
local Levels = levels.Register(require("quirky.level_list"))

-- Current game level (start at 0 to dovetail with "next level" index rotation the first time) --
local Index = 0

-- Starts a new level
local function StartLevel ()
	-- Update the level index and load the initial game state. Clear the rewind memory.
	Index = (Index % #Levels) + 1
	State = levels.Load(Levels[Index])

	rewind.Clear(Scene)

	-- Load and position the level description.
	Scene.m_description.text = Levels[Index].description

	Scene.m_description.y = ((display.contentHeight - display.viewableContentHeight) + Scene.m_description.height) / 2 + 10

	-- Bake any fixed parts of the scene and populate the initial dynamic ones. Give the
	-- player control.
	states.Bake(Scene, State)
	states.Populate(Scene, State)
	controls.Enable(true)

	-- Reset any flags.
	Covered, Exit, Filled = nil
end

-- Create the Quirky scene.
function M.Create (scene)
	Scene = scene

	-- Configure various layers and game parameters.
	states.SetGroups(scene, Layers)
	states.SetCenter(display.contentCenterX, display.contentCenterY)

	local cellw, cellh = floor(display.viewableContentWidth / 16), floor(display.viewableContentHeight / 16)
	local cdim = min(cellw, cellh)

	states.SetCellSize(cellw, cellh)
	states.SetCharacterSize(floor(cdim / 2))
	states.SetRivetSize(floor(cdim * .2))
	states.SetSpotSize(floor(cdim * .2))

	-- Add directional controls.
	local hud, w, h = scene.m_hud, floor(display.viewableContentWidth / 12), floor(display.viewableContentHeight / 12)
	local cx, cy = 2.25 * w, display.viewableContentHeight - 3.5 * h

	for i, dir in ipairs{ "right", "down", "left", "up" } do
		local degs = (i - 1) * 90
		local rads = rad(degs)
		local x, y = floor(cx + .65 * w * cos(rads)), floor(cy + .85 * h * sin(rads))

		gui.Arrow(hud, x, y, w, h, dir, degs)
	end

	-- Add character switch button...
	local switch = gui.RoundedRect(hud, "Switch Character", display.viewableContentHeight - 3 * h, "s")

	-- ...a rewind button...
	scene.m_rewind = gui.RoundedRect(hud, "Rewind", 6 * h, "r")

	-- ...and a description or hint about the current level.
	scene.m_description = display.newText(hud, "", display.contentCenterX, 1.5 * h, native.systemFont, 16)

	-- Begin with the HUD off.
	hud.isVisible = false

	-- HUD fade-in transition --
	local FadeHudInParams = { alpha = 1 }

	-- Make a level launch button.
	scene.m_start_level = widget.newButton{
		x = display.contentCenterX,
		y = display.viewableContentHeight - 3 * h,
        label = "Start Level",

        onEvent = function(event)
			if event.phase == "ended" then
				-- Once clicked, hide the button and fade the HUD in.
				event.target.isVisible, hud.isVisible, hud.alpha = false, true, 0

				transition.to(hud, FadeHudInParams)

				-- Begin the next level. Put a few GUI elements into an appropriate initial
				-- state. Set up the game controls.
				StartLevel()

				switch.isVisible = states.MoreThanOneCharacter(State)
				Scene.m_rewind.isVisible, Scene.m_overlay.isVisible = false, false

				controls.SetAction(DoGame)
				controls.SetKeys({ "left", "right", "up", "down", "s" }, { "r" })
			end
		end
    }

	scene.view:insert(scene.m_start_level)
end

-- Resume Quirky gameplay
function M.Resume ()
	Runtime:addEventListener("key", controls.Key)
end

-- Suspend Quirky gameplay
function M.Suspend ()
	Runtime:removeEventListener("key", controls.Key)
end

-- Listen for Sokoban-style all-covered events
info.GetEvents():addEventListener("covered", function()
	Covered = true
end)

-- Listen for blocks filling in parts of holes
info.GetEvents():addEventListener("filled", function(event)
	Filled = event.entry
end)

-- Block fade-in transition --
local FadeBlockParams = { alpha = 0 }

function FadeBlockParams.onComplete (block)
	states.AddToStash("rects", block)
end

-- Helper to update ground graphics on filling part of a hole
local function FillHole (object, ground)
	local floor = states.AddFloor(ground, object.x, object.y)

	floor:toFront()

	transition.to(object, FadeBlockParams)
end

-- Player fade-out transition --
local FadePlayerParams = { alpha = 0 }

function FadePlayerParams.onComplete (player)
	states.AddToStash("circles", player)
end

-- HUD fade-out transition --
local FadeHudOutParams = { alpha = 0 }

-- Banner left-to-right transition --
local BannerParams = { x = display.viewableContentWidth * 1.5, time = 3450 }

function BannerParams.onComplete (banner)
	banner:removeSelf()

	transition.to(Scene.m_hud, FadeHudOutParams)
	states.Clear(Scene, PerLevelLayers)

	Scene.m_start_level.isVisible = true
end

-- Listen for character, along with any block or turnstile, to stop moving
info.GetEvents():addEventListener("done_moving", function()
	-- Check if a character has reached the exit or all Sokoban-style spots are covered. In
	-- the latter case, the level is complete. In the former, it will be so only if that was
	-- the last character. If we beat the level, show a congratulations banner. Otherwise,
	-- give control back to the player.
	if Exit or Covered then
		transition.to(states.GetPlayer(), FadePlayerParams)

		if Exit and states.MoreThanOneCharacter(State) then
			states.RemoveCharacter(State)
			controls.Enable(true)
		else
			local banner = display.newText(Scene.m_hud, "Level completed!", -1.5 * display.viewableContentWidth, display.contentCenterY, native.systemFontBold, 22)

			banner:setFillColor(.9, .3, .4)

			transition.to(banner, BannerParams)
		end
	else
		controls.Enable(true)
	end

	-- If a block filled part of a hole, update the graphics components.
	if Filled then
		info.ForEachObject(Scene.m_middle, Filled, FillHole, Scene.m_ground)
	end

	-- Clear any flags.
	Covered, Exit, Filled = nil
end)

-- Export the module.
return M