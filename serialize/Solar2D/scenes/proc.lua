--- Scene that sends complex data to and from a Lua process.

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

-- Plugins --
local lproc = require("plugin.luaproc")
local serialize = require "plugin.serialize"

-- Modules --
local marshal = serialize.marshal

-- Corona modules --
local composer = require("composer")

-- Proc scene.
local Scene = composer.newScene()

-- Create --
function Scene:create (event)
	local cx, cy = display.contentCenterX, display.contentCenterY
	local message = display.newText(self.view, "Capturing serialized table", cx, cy, native.systemFont, 19)

	-- Respond to alerts from another process.
	local has_doubled = false

	lproc.get_alert_dispatcher():addEventListener("alerts", function(event)
		if event.payload == true then
			message.text = "Doubling all values"
			has_doubled = true
		else
			local content = event.payload

			-- On the second string-type alert, the payload will be an encoded table.
			if has_doubled then
				local decoded = marshal.decode(content)

				content = decoded.get_contents(decoded.t)
			end

			message.text = "Contents of table: " .. content
		end
	end)

	-- Encode a table as a string so that it can be captured. Launch a process to
	-- decode and operate on it asynchronously.
	local bytes = marshal.encode{ t = 37, a = 16, d = 4 }

	lproc.newproc(function()
		local marshal = require("plugin.serialize").marshal
		local string = require("string")

		luaproc.sleep(2500)

		-- Get the table back.
		local original = marshal.decode(bytes)

		-- Returns table contents as a string
		local function GetContents (t)
			local contents, prev = ""

			for k, v in pairs(t) do
				contents = contents .. (prev and ", " or "") .. k .. " = " .. tostring(v)
				prev = true
			end

			return contents
		end

		-- Report back to the main state, then wait a bit.
		luaproc.alert("alerts", GetContents(original))
		luaproc.sleep(2500)
		luaproc.alert("alerts", true)
		luaproc.sleep(2500)

		-- Double the original values and send them back to the main state.
		for k, v in pairs(original) do
			original[k] = v * 2
		end

		luaproc.alert("alerts", marshal.encode{
			get_contents = GetContents, t = original
		})
	end)
end

Scene:addEventListener("create")

return Scene