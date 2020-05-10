--- Sample for MemoryBlob plugin.

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
local memory_blob = require("plugin.MemoryBlob")

-- Corona modules --
local widget = require("widget")

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

local page = widget.newScrollView{
	backgroundColor = { .075 },
	top = (display.contentHeight - display.viewableContentHeight) / 2,
	left = (display.contentWidth - display.viewableContentWidth) / 2,
	height = display.viewableContentHeight
}

local print = Print(page, 8)

-- Make some blobs.
local aligned = memory_blob.New{ alignment = 4, size = 13 }
local fixed = memory_blob.New(25)
local resizable = memory_blob.New()

print("Check blob-ness")
print("Is blob? (aligned):", memory_blob.IsBlob(aligned))
print("Is blob? (fixed):", memory_blob.IsBlob(fixed))
print("Is blob? (resizable):", memory_blob.IsBlob(resizable))
print("Is blob? (27):", memory_blob.IsBlob(27))
print("Is blob? (\"a string\"):", memory_blob.IsBlob("a string"))
print("")

print("Sizes:", #aligned, #fixed, #resizable)
print("")

local function PrintContents ()
	print("Fixed-size aligned blob contents: ", aligned:GetBytes())
	print("Fixed-size default blob contents: ", fixed:GetBytes())
	print("Resizable blob contents: ", resizable:GetBytes())
	print("")
end

print("Doing various operations on aligned and default fixed-size blobs, plus a resizable blob")
print("")

print("Writing at slot 1")
print("")

aligned:Write(1, "This is more than 13 bytes")
fixed:Write(1, "ABCDEFGHIJKLMNOPQRSTUVWXY")
resizable:Write(1, "X") -- size = 0, thus this is an append

PrintContents()

print("Writing at an offset")
print("")

aligned:Write(4, "Short")
fixed:Write(-3, "abc")
resizable:Write(1, "Something else")

PrintContents()

print("Again, but with different offsets")
print("")

aligned:Write(100, "Too far") -- no-op
fixed:Write(-82, "This too") -- ditto
resizable:Write(#resizable, "At the end")

PrintContents()

print("Appending")
print("")

aligned:Append("Won't do anything") -- no-op
fixed:Append("This either") -- ditto
resizable:Append("But this will")

PrintContents()

print("Cloning current blobs")
print("")

local aligned_clone = aligned:Clone()
local fixed_clone = fixed:Clone()
local resizable_clone = resizable:Clone()

print("Inserting")
print("")

aligned:Insert(4, "A few bytes")
fixed:Insert(-11, "New!")
resizable:Insert(3, "Bytes!!")

PrintContents()

print("Removing")
print("")

aligned:Remove(3, 6)
fixed:Remove(10, 12)
resizable:Remove(-9, -2)

PrintContents()

print("Getting byte ranges")
print("")

print("In aligned (3, 7):", aligned:GetBytes(3, 7))
print("In fixed (2, -8):", fixed:GetBytes(2, -8))
print("In resizable(-22, 17):", resizable:GetBytes(-22, 17))
print("")

print("Getting properties")
print("")

local rprops = {}

for k, props in pairs{
	aligned = aligned:GetProperties(),
	fixed = fixed:GetProperties(),
	resizable = resizable:GetProperties(rprops),
	aligned_clone = aligned_clone:GetProperties(),
	fixed_clone = fixed_clone:GetProperties(),
	resizable_clone = resizable_clone:GetProperties()
} do
	print("Which one?", k)

	for name, prop in pairs(props) do
		print("     " .. name, prop)
	end

	print("")
end

print("Showing captured properties (resizable)")
print("")

for name, prop in pairs(rprops) do
	print("     " .. name, prop)
end

print("")

print("Send in the clones!")
print("")

aligned = aligned_clone -- swap these out to spoof PrintContents(); originals are garbage-collected
fixed = fixed_clone
resizable = resizable_clone

PrintContents()