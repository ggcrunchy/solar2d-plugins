--- Quirky rewind mechanics.

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

-- Exports --
local M = {}

-- Plugins --
local serialize = require "plugin.serialize"

-- Modules --
local controls = require("quirky.controls")
local gui = require("quirky.gui")
local info = require("quirky.info")
local marshal = serialize.marshal
local ring_buffer = require("external.ring_buffer")
local states = require("quirky.states")

-- Corona globals --
local display = display
local native = native
local transition = transition

-- Head and tail of ring buffer --
local Head, Tail

-- Ring buffer array; maximum buffer size --
local Ring, N = {}, 59

-- Reference to Quirky scene --
local Scene

-- Add the current state to the rewind memory, evicting an older one if necessary
function M.Capture (state)
	if ring_buffer.IsFull(Head) then
		Head, Tail = ring_buffer.Pop(Ring, Head, Tail, N)
	end

	Head, Tail = ring_buffer.Push(Ring, marshal.encode(state), Head, Tail, N)

	-- Rewinds are possible, so show the button.
	Scene.m_rewind.isVisible = true
end

-- Clear the rewind memory
function M.Clear (scene)
	Scene, Head, Tail = scene
end

-- Helper to block touches while in the rewind overlay
local function BlockInput ()
	return true
end

-- Old input state to restore when the overlay is closed --
local OldAction, OldKeys, OldRelease

-- Overlay fade-in transition --
local FadeInParams = { alpha = 1 }

-- Overlay fade-out transition --
local FadeOutParams = { alpha = 0 }

function FadeOutParams.onComplete (overlay)
	overlay.isVisible = false
end

-- Helper to set up the overlay objects
local function MakeOverlay (scene, overlay)
	-- Backdrop used to fade out the underlying game.
	local background = display.newRect(overlay, display.contentCenterX, display.contentCenterY, display.contentWidth, display.contentHeight)

	background:addEventListener("touch", BlockInput)
	background:setFillColor(.2, .7)

	-- Add some buttons to leave the overlay.
	gui.RoundedRect(overlay, "Cancel", display.contentCenterY - 20, "escape")
	gui.RoundedRect(overlay, "Confirm", display.contentCenterY + 20, "enter")

	-- Add some elements to indicate which part of the rewind memory is being examined.
	local vcw, vch = display.viewableContentWidth, display.viewableContentHeight
	local cy = (display.contentHeight + vch) / 2

	scene.m_counter = display.newText(overlay, "", display.contentCenterX, cy - vch / 4, native.systemFont, 16)
	scene.m_larrow = gui.Arrow(overlay, 0, scene.m_counter.y, vcw / 16, vch / 16, "left", 180)
	scene.m_rarrow = gui.Arrow(overlay, 0, scene.m_counter.y, vcw / 16, vch / 16, "right", 0)
end

-- Flattened view of the rewind memory for convenience while the overlay is up --
local Flat

-- Element of rewind memory under consideration --
local Index

-- State most recently decoded from rewind memory --
local NewState

-- Event sent when we choose to rewind (more than one frame) --
local RewindEvent = { name = "rewind" }

-- Helper to update the indicator text and related elements
local function SetText ()
	if Index == Flat.n then
		Scene.m_counter.text = "Current frame"
	else
		local diff = Flat.n - Index

		Scene.m_counter.text = ("%i frame%s ago"):format(diff, diff > 0 and "s" or "")
	end

	local x, w = Scene.m_counter.x, Scene.m_counter.width

	Scene.m_larrow.x = x - (w + Scene.m_larrow.width + 10) / 2
	Scene.m_rarrow.x = x + (w + Scene.m_rarrow.width + 10) / 2
end

-- Rewind overlay input
local function DoRewind (name)
	-- On left and right actions, rotate the index. Use the new index to decode a state from
	-- the rewind memory; repopulate the level with this state to tell us visually what a
	-- rewind would give us. Update the indicator text to reflect the changed index.
	if name == "left" or name == "right" then
		if name == "left" then
			Index = Index > 1 and Index - 1 or Flat.n
		elseif name == "right" then
			Index = Index < Flat.n and Index + 1 or 1
		end

		NewState = marshal.decode(Flat[Index])

		states.Populate(Scene, NewState)

		SetText()

	-- Otherwise, check for events that would exit the overlay.
	else
		-- If the index currently points to the front of the flat list, the game state is
		-- already the way we want it, so we do nothing. Otherwise, on cancellation, we want
		-- to repopulate the state with that last entry. Failing that, we must rewind.
		if Index < Flat.n then
			if name == "escape" then
				states.Populate(Scene, marshal.decode(Flat[Flat.n]))
			else
				-- Send out a rewind event with the appropriate state.
				RewindEvent.state = NewState

				info.GetEvents():dispatchEvent(RewindEvent)

				RewindEvent.state = nil

				-- Rebuild the rewind memory with the earlier states.
				Head, Tail = nil

				for i = 1, Index - 1 do
					Head, Tail = ring_buffer.Push(Ring, Flat[i], Head, Tail, N)
				end
			end
		end

		-- If we rewound back to the earliest state, there is not yet any rewind memory, so
		-- hide the button to call up the overlay.
		if ring_buffer.IsEmpty(Head, Tail) then
			Scene.m_rewind.isVisible = false
		end

		-- Fade the overlay out and restore control to the game.
		transition.to(Scene.m_overlay, FadeOutParams)
		controls.SetAction(OldAction)
		controls.SetKeys(OldKeys, OldRelease)
	end
end

-- Open the rewind overlay
function M.OpenOverlay (state)
	local overlay = Scene.m_overlay

	-- Ignore open requests when the rewind memory is blank.
	if ring_buffer.IsEmpty(Head, Tail) then
		return
	
	-- Set up overlay elements on the first visit.
	elseif not Flat then
		Flat = {}

		MakeOverlay(Scene, overlay)
	end

	-- Fade the overlay in.
	overlay.isVisible, overlay.alpha = true, 0

	transition.to(overlay, FadeInParams)

	-- Clear overlay state.
	Index, NewState = 1

	-- Get a flattened view of the rewind memory.
	local i1, j1, i2, j2 = ring_buffer.GetRanges(Ring, Head, Tail, N)

	for i = i1, j1 do
		Flat[Index], Index = Ring[i], Index + 1
	end

	for i = i2, j2 do
		Flat[Index], Index = Ring[i], Index + 1
	end

	-- Encode and append the current game state as well, so that we can 
	Flat.n, Flat[Index] = Index, marshal.encode(state)

	-- Set initial text.
	SetText()

	-- Switch to overlay input, saving the previous state for restoration when we leave.
	OldAction = controls.SetAction(DoRewind)
	OldKeys, OldRelease = controls.SetKeys({ "left", "right" }, { "escape", "enter" })
end

-- Export the module.
return M