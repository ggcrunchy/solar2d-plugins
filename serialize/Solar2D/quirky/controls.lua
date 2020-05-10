--- Quirky controls.

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
local pairs = pairs

-- Corona globals --
local Runtime = Runtime
local timer = timer

-- Exports --
local M = {}

-- Is input currently being monitored? --
local IsEnabled

-- Enable or disable input monitoring
function M.Enable (enable)
	IsEnabled = not not enable
end

-- Do-nothing action --
local function NoOp () end

-- Current input action --
local Action = NoOp

-- Name -> timer list --
local Timers = {}

-- Repeats an action while a button is held
local function DoTimedAction (event)
	local source = event.source

	for name, t in pairs(Timers) do
		if t == source then
			Action(name)

			break
		end
	end
end

-- Set the current input action, getting back the previous one
function M.SetAction (action)
	local old = Action

	Action = action or NoOp

	return old
end

-- Event used to spoof key presses from touches --
local KeyEvent = { name = "key" }

-- Filter for keys (that respond to presses) to avoid launching spurious timers --
local Keys

-- List of keys that respond to releases --
local OnRelease

-- Quirky key listener
function M.Key (event)
	-- Key down: do action once, then periodically thereafter.
	local name = event.keyName

	if event.phase == "down" then
		if not Keys or Keys[name] then
			Action(name)

			Timers[name] = timer.performWithDelay(80, DoTimedAction, 0)
		end

	-- Key up / on-release: do one-time action.
	elseif OnRelease and OnRelease[name] then
		Action(name)

	-- Key up: stop action.
	elseif Timers[name] then
		timer.cancel(Timers[name])

		Timers[name] = nil
	end

	return true
end

-- Helper to assign a new key filter and recover the old one's original form
local function AuxSetKeys (keys, prev)
	local old

	if prev then
		old = {}

		for k in pairs(prev) do
			old[#old + 1] = k
		end
	end

	if keys then
		prev = {}

		for i = 1, #keys do
			prev[keys[i]] = true
		end
	else
		prev = nil
	end

	return prev, old
end

-- Set key and / or release filters
function M.SetKeys (keys, release)
	local old_keys, old_release

	Keys, old_keys = AuxSetKeys(keys, Keys)
	OnRelease, old_release = AuxSetKeys(release, OnRelease)

	return old_keys, old_release
end

-- Quirky touch listener
function M.Touch (event)
	local phase = event.phase

	if phase == "began" or phase == "ended" or phase == "cancelled" then
		KeyEvent.keyName = event.target.m_key
		KeyEvent.phase = phase == "began" and "down" or "up"

		Runtime:dispatchEvent(KeyEvent)
	end

	return true
end

-- Export the module.
return M