--- Test for truetype plugin.

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

local truetype = require("plugin.truetype")
local bytemap = require("plugin.Bytemap")

local c, s = ("a"):byte(), 20

function copyFile( srcName, srcPath, dstName, dstPath, overwrite )

    local results = false

    local fileExists = doesFileExist( srcName, srcPath )
    if ( fileExists == false ) then
        return nil  -- nil = Source file not found
    end

    -- Check to see if destination file already exists
    if not ( overwrite ) then
        if ( fileLib.doesFileExist( dstName, dstPath ) ) then
            return 1  -- 1 = File already exists (don't overwrite)
        end
    end

    -- Copy the source file to the destination file
    local rFilePath = system.pathForFile( srcName, srcPath )
    local wFilePath = system.pathForFile( dstName, dstPath )

    local rfh = io.open( rFilePath, "rb" )
    local wfh, errorString = io.open( wFilePath, "wb" )

    if not ( wfh ) then
        -- Error occurred; output the cause
        print( "File error: " .. errorString )
        return false
    else
        -- Read the file and write to the destination directory
        local data = rfh:read( "*a" )
        if not ( data ) then
            print( "Read error!" )
            return false
        else
            if not ( wfh:write( data ) ) then
                print( "Write error!" )
                return false
            end
        end
    end

    results = 2  -- 2 = File copied successfully!

    -- Close file handles
    rfh:close()
    wfh:close()

    return results
end

local f
--[=[
if system.getInfo("platformName") == "android" then
   copyFile("8-BIT WONDER.txt", nil, "8-BIT WONDER.TTF", system.DocumentsDirectory, true)

   f = io.open(system.pathForFile("8-BIT WONDER.txt", system.DocumentsDirectory), "rb")
else
   f
]=]   
   = io.open(system.pathForFile("arial.txt"--[["8-BIT WONDER.txt"]]), "rb") -- font (extension stripped for Android)
--end

local function Print (bitmap, w, h)
    local index, text = 1, " .:ioVM@"

    for _ = 1, h do
        local line = ""

        for _ = 1, w do
            local pos = math.floor(bitmap:sub(index, index):byte() / 32) + 1

            line, index = line .. text:sub(pos, pos), index + 1
        end

        print(line)
    end
end

if f then
    local buf = f:read("*a")

    f:close()

    local font = truetype.InitFont(buf, truetype.GetFontOffsetForIndex(buf, 0))

    do
        local bitmap, w, h = font:GetCodepointBitmap(0, font:ScaleForPixelHeight(s), c)

        Print(bitmap, w, h)

        local bmap = bytemap.newTexture{ width = w, height = h, format = "rgb" }
        local image = display.newImage(bmap.filename, bmap.baseDir)

        image.x, image.y = display.contentCenterX, display.contentCenterY

        bmap:SetBytes(bitmap, { format = "grayscale" })
    end

    do
        local xpos = 2 -- leave a little padding in case the character extends left
        local text = "Heljo World!" -- intentionally misspelled to show 'lj' brokenness
        local screen = bytemap.newTexture{ width = 79, height = 20, format = "rgb" }

        local scale = font:ScaleForPixelHeight(15)
        local ascent = font:GetFontVMetrics()
        local baseline = math.floor(ascent * scale)

        local i, n = 1, #text

        for i = 1, n do
            local ch = text:byte(i)
            local xshift = xpos % 1
            local advance, lsb = font:GetCodepointHMetrics(ch)
            local x0, y0, x1, y1 = font:GetCodepointBitmapBoxSubpixel(ch, scale, scale, xshift, 0)
            local bitmap = font:GetCodepointBitmapSubpixel(scale, scale, xshift, 0, ch)

            local xf, yf = math.floor(xpos) + x0, baseline + y0

            screen:SetBytes(bitmap, {
                x1 = xf + 1, y1 = yf + 1, x2 = xf + x1 - x0, y2 = yf + y1 - y0,
                format = "grayscale"
            })

            -- note that this stomps the old data, so where character boxes overlap (e.g. 'lj') it's wrong
            -- because this API is really for baking character bitmaps into textures. if you want to render
            -- a sequence of characters, you really need to render each bitmap to a temp buffer, then
            -- "alpha blend" that into the working buffer

            xpos = xpos + advance * scale

            if i < n then
                xpos = xpos + scale * font:GetCodepointKernAdvance(ch, text:byte(i + 1))

                i = i + 1
            end
        end

        local simage = display.newImage(screen.filename, screen.baseDir)

        simage:toBack()

        simage.x, simage.y = display.contentCenterX, display.contentCenterY
simage:scale(5, 5)
        local bytes = screen:GetBytes{ format = "mask" }

        Print(bytes, 79, 20)
    end
end
