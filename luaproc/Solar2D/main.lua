--- Sample for luaproc.

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

-- Plugins --
local lproc = require("plugin.luaproc")

-- Add a few objects that will update in the main state.
local object1 = display.newCircle(display.contentWidth * .3, display.contentCenterY, 10)
local object2 = display.newCircle(display.contentCenterX, display.contentCenterY, 15)
local object3 = display.newCircle(display.contentCenterX, display.contentCenterY * .7, 25)

for _, object in ipairs{ object1, object2, object3 } do
	object:setStrokeColor(.4)
	
	object.strokeWidth = 2
end

-- Add some text to describe what's going on in each process.
local text1 = display.newText("", 0, display.contentHeight * .1, native.systemFontBold, 13)
local text2 = display.newText("", 0, display.contentHeight * .15, native.systemFontBold, 13)
local text3 = display.newText("", 0, display.contentHeight * .2, native.systemFontBold, 13)

for _, text in ipairs{ text1, text2, text3 } do
	text.anchorX, text.x = 0, display.contentWidth * .07
end

-- Maintain three processes.
lproc.setnumworkers(3)

-- Kick off process #1.
lproc.newproc(function()
	local string = require("string")

	-- Give a little overview.
	local n = 1e8

	luaproc.alert("process1", string.format("Adding 2.5 to itself %i times\n", n))

	-- Accumulate the count.
    local count = 0

    for i = 1, n / 1e2 do
		if luaproc.wants_to_close() then
			return
		else
			for j = 1, 1e2 do
				count = count + 2.5
			end
		end
	end

	-- Give the results, wait, then start switching an object's colors.
    luaproc.alert("process1", string.format("Final tally: %.2f\n", count))
	luaproc.sleep(2500)
	luaproc.alert("process1", "Switching colors")

	local math = require("math")

	while not luaproc.wants_to_close() do
		luaproc.alert("data", math.random())
		luaproc.sleep(500)
	end
end)

-- Kicks off process #2.
lproc.newproc(function()
	-- Open a line of communication and kick off process #2, which simply sends alerts.
	luaproc.newchannel("talk")
	luaproc.newproc(function()
		luaproc.alert("process3", "Started inside another process")

		while not luaproc.wants_to_close() do
			local n, b = luaproc.receive("talk")

			if n then -- n.b. receive() fails when "talk" is deleted, giving us a nil n (this is slightly awkward
					  -- since nil is a valid value, but is what luaproc() does); note that this would only crash
					  -- the process, not Corona, in the next line  
				luaproc.alert("process3", "Received " .. n .. " " .. tostring(b))
			end
		end
	end)

	-- Wait a bit, then start talking to process #3.
	luaproc.sleep(5000)
	luaproc.alert("process2", "Sending numbers and booleans to process3")

	local math = require("math")

	while not luaproc.wants_to_close() do
		luaproc.send("talk", math.random(10000), math.random(50) % 2 == 0)
		luaproc.sleep(2000)
	end

	-- Kill the channel so process #3 doesn't hang.
	luaproc.delchannel("talk")
end)

-- Listen for alerts from the processes.
lproc.get_alert_dispatcher():addEventListener("process1", function(event)
    text1.text = "Process #1: " .. event.payload
end)

lproc.get_alert_dispatcher():addEventListener("process2", function(event)
    text2.text = "Process #2: " .. event.payload
end)

lproc.get_alert_dispatcher():addEventListener("process3", function(event)
    text3.text = "Process #3: " .. event.payload
end)

lproc.get_alert_dispatcher():addEventListener("data", function(event)
	local ptype = type(event.payload)

	if ptype == "number" then
		object2:setFillColor(event.payload, 0, 0)
	end
end)

-- Send an alert from the main state.
lproc.alert("process2", "Waiting")

-- Update objects while the processes. are running.
Runtime:addEventListener("enterFrame", function(event)
	object1.y = display.contentCenterY + math.sin(event.time / 800) * display.contentHeight * .36
    object2.y = display.contentCenterY + math.cos(event.time / 1900) * display.contentHeight * .42
    object3.x = display.contentCenterX + math.sin(event.time / 3300) * display.contentHeight * .26
end)