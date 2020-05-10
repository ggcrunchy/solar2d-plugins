--- Scene that demonstrates Kim Alvefur's (struct-based) [CBOR library](https://www.zash.se/lua-cbor.html).

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
local cbor = require("external.cbor")

-- Corona globals --
local display = display

-- Corona modules --
local composer = require("composer")
local widget = require("widget")

-- CBOR scene.
local Scene = composer.newScene()

--
local function Print (into, size)
    local y = 0

    return function(...)
        local strs = { ... }

        for i = 1, #strs do
            strs[i] = tostring(strs[i])
        end

        local text = display.newText(table.concat(strs, "   "), 0, 0, native.systemFontBold, size)

        text.anchorX, text.x = 0, 5
        text.anchorY, text.y = 0, y

        text:setTextColor(1, 0, 0)

        into:insert(text)

        y = y + text.contentHeight + 5
    end
end

-- Create --
function Scene:create (event)
	local page = widget.newScrollView{
		backgroundColor = { .075 },
		top = (display.contentHeight - display.viewableContentHeight) / 2,
		left = (display.contentWidth - display.viewableContentWidth) / 2,
		height = display.viewableContentHeight
	}

	self.view:insert(page)

	local print = Print(page, 8)

	local function DumpTable (t, indent)
		for k, v in pairs(t) do
			if type(v) == "table" then
				print(indent, k, "=  {")

				DumpTable(v, indent .. "  ")

				print(indent, "}")
			else
				print(indent, k, "=", v)
			end
		end
	end

	print("Original table:")
	print("")
	
	local t = {
		d = "ffdfa", [2] = "13333", [4] = false, bob = { duck = "cat" }
	}

	DumpTable(t, "")

	print("")
	print("Encoding...")
	print("")

	local encoded = cbor.encode(t)
	
	print("Result: ", encoded)
	print("")
	print("Decoding...")
	print("")
	print("Result:")
	print("")

	DumpTable(cbor.decode(encoded), "")
end

Scene:addEventListener("create")

return Scene