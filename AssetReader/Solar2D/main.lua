--- Sample for AssetReader plugin.

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
local ipairs = ipairs

-- Plugins --
local asset_reader = require("plugin.AssetReader")

-- Corona modules --
local widget = require("widget")

--
--
--

widget.setTheme("widget_theme_android_holo_light")

local view = widget.newScrollView{
    backgroundColor = { .7, .7, .7 },
    top = 10, left = 10,
    width = display.viewableContentWidth - 20,
    height = display.viewableContentHeight - 20
}

local y = 35

local function AddText (str)
    local message = display.newText(str, 0, y, native.systemFontBold, 10)

    message.anchorX, message.x = 0, 5

    view:insert(message)

    y = y + 13

    return message
end

local contents = asset_reader.Read("T.png")

AddText(("Read %i bytes from T.png"):format(#contents)):setFillColor(0, 1, 0)

local function EnumDir (name)
    name = name or ""

    local dir = asset_reader.EnumerateDirectory(name)

    if #name > 0 then
        name = "'" .. name .. "'"
    else
        name = "top-level"
    end

    if dir then
        local title = AddText("Enumerating " .. name .. " directory")

        title:setFillColor(0, 0, 1)

        for _, file in ipairs(dir) do
            if file:find("%.") then -- has an extension?
                AddText("  File: " .. file)
            else
                AddText("  Sub-directory: " .. file)
            end
        end
    else
        local title = AddText("Nothing to enumerate in " .. name .. " directory (or does not exist)")

        title:setFillColor(1, 0, 0)
    end
end

EnumDir()
EnumDir("more")
EnumDir("blarg")

local into = { "A", "B", "C" } -- this is more than the sample puts in more
local _, n = asset_reader.EnumerateDirectory("more", into) -- ignore first result, which is just `into`

AddText("Enumerating 'more' directory from table"):setFillColor(1, 0, 1)

for i = 1, n do -- ignore leftovers in table
    AddText("  File: " .. into[i])
end
