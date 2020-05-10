--- Test for impack plugin's image and write modules.

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
local impack = require("plugin.impack")
local bytemap = require("plugin.Bytemap")

-- Some basic layering for the images.
local gif_layer = display.newGroup()
local tga_layer = display.newGroup()
local bmp_layer = display.newGroup()
local scaled_layer = display.newGroup()

-- Load a bitmap.
do
	local pdata, w, h, bpp = impack.image.load("turtle.bmp", {
		as_userdata = true	-- return ByteReader-compatible userdata directly, rather than do a more
							-- convenient conversion to a string first
	})

	-- Figure out what sort of texture to make from load results.
	local comp

	if bpp == 1 then
		comp = "mask"
	elseif bpp == 3 then
		comp = "rgb"
	elseif bpp == 4 then
		comp = "rgba"
	end

	-- An image in the proper format.
	local bmap = bytemap.newTexture{ width = w, height = h, format = comp }
	local image = display.newImage(bmp_layer, bmap.filename, bmap.baseDir)

	image.x, image.y = display.contentCenterX, display.viewableContentHeight - image.height - 10

	-- Add an alpha channel to this one.
	local bmap2 = bytemap.newTexture{ width = w, height = h, format = "rgba" }
	local image2 = display.newImage(bmp_layer, bmap2.filename, bmap2.baseDir)

	image2.x, image2.y = display.contentCenterX - 30, display.viewableContentHeight - image.height - 150

	-- Re-save the image as a PNG.
	impack.write.png("Turtle.png", w, h, bpp, pdata)

	timer.performWithDelay(500, function()
		-- Write turtle bytes to a respective maps, getting some info back.
		local info = {}
		local ok, t = bmap:SetBytes(pdata, { get_info = info })

		print("Compare tables", info == t)  -- if 'get_info' had been true, the table would have been
											-- created for us, and we could retrieve it as 't'

		bmap:invalidate()

		-- Do the same with the second map, but colorkey out the black pixels.
		-- Note that the result will be a bit faded owing to the missing alpha.
		bmap2:SetBytes(pdata, { colorkey = string.char(0, 0, 0), format = "rgb" })
		bmap2:invalidate()

		-- Dump some info about the BMP.
		local bytes = bmap:GetBytes()

		print("Bytes", #pdata, #bytes, pdata == bytes) 	-- equality succeeds if 'as_userdata' not
														-- specified above

		for k, v in pairs(info) do
			print("Info", k, v)
		end
	end)	
end

-- Load a Targa and apply some operations to it.
do
	local data1, w1, h1 = impack.image.load("elephant.tga") -- n.b. some of the stuff further down knows this
															-- has four channels and relies upon the fact; in
															-- practice code might need to be more robust
	local map1 = bytemap.newTexture{ width = w1, height = h1, format = "rgba" }
	local image1 = display.newImage(tga_layer, map1.filename, map1.baseDir)

	image1.x, image1.y = display.contentCenterX - 50, display.contentCenterY - 100

	map1:SetBytes(data1)

	local bytes = map1:GetBytes()

	local data2, w2, h2 = impack.ops.rotate(bytes, w1, h1, math.pi / 3)
	local data3, w3, h3 = impack.ops.rotate(bytes, w1, h1, math.pi * 1.1)
	local data4 = impack.ops.box_filter(bytes, w1, h1, 9, 9)

	local map2 = bytemap.newTexture{ width = w2, height = h2, format = "rgba" }
	local image2 = display.newImage(tga_layer, map2.filename, map2.baseDir)

	image2.x, image2.y = display.contentCenterX + 50, display.contentCenterY - 150

	local map3 = bytemap.newTexture{ width = w3, height = h3, format = "rgba" }
	local image3 = display.newImage(tga_layer, map3.filename, map3.baseDir)

	image3.x, image3.y = display.contentCenterX - 150, h3 / 2

	local map4 = bytemap.newTexture{ width = w1, height = h1, format = "rgb" } -- example that drops alpha
	local image4 = display.newImage(tga_layer, map4.filename, map4.baseDir)

	image4.x, image4.y = display.contentCenterX + 150, display.contentCenterY - h1 / 2

	map2:SetBytes(data2)
	map3:SetBytes(data3)
	map4:SetBytes(data4, { format = "rgba" }) -- see note above
end

-- Animate a GIF.
local gif_frames, gw, gh

do
	gif_frames, gw, gh = impack.image.xload("80s.gif")

	local fmap = bytemap.newTexture{ width = gw, height = gh, format = "rgba" }
	local gif = display.newImageRect(gif_layer, fmap.filename, fmap.baseDir, display.viewableContentWidth, display.viewableContentHeight * .55)

	gif.x, gif.y = display.contentCenterX, display.contentCenterY

	local pos, n, accum, t = 1, #gif_frames, 0

	timer.performWithDelay(20, function(event)
		local et, changed = event.time, not t

		t = t or et
		accum, t = accum + et - t, et

		while accum >= gif_frames[pos].delay do
			accum, changed = accum - gif_frames[pos].delay, true

			if pos == n then
				pos = 1
			else
				pos = pos + 1
			end
		end

		if changed then
			fmap:SetBytes(gif_frames[pos].image)
			fmap:invalidate()
		end
	end, 0)
end

-- Grab a few random frames and save them as a new GIF.
do
	local taken, frames = {}, {}

	for i = 1, 5 do
		local index

		repeat
			index = math.random(#gif_frames)
		until not taken[index]

		frames[#frames + 1] = { image = gif_frames[index].image, delay = math.random(50, 200) }

		taken[index] = true
	end

	impack.write.gif("NewGIF.gif", gw, gh, frames)
end

-- Do the same, but now save them as an MPEG.
do
	local taken, frames, delays = {}, {}, 0

	for i = 1, 5 do
		local index

		repeat
			index = math.random(#gif_frames)
		until not taken[index]

		frames[#frames + 1] = gif_frames[index].image

		delays = delays + gif_frames[index].delay

		taken[index] = true
	end

	local fps = math.floor(1000 / (delays / 5))

	impack.write.mpeg("NewMovie.mpg", gw, gh, frames, fps)
end

-- Do some grayscale operations.
do
	-- Convert some image to grayscale.
	local original, w, h = impack.image.load("turtle.bmp") -- elephants are already too gray :)
	local gray = impack.grayscale.rgb_to_gray(original, w, h)
	local gmap = bytemap.newTexture{ width = w, height = h, format = "rgb" }
	local gimage = display.newImage(tga_layer, gmap.filename, gmap.baseDir)
	gimage.x, gimage.y = display.contentCenterX, display.contentCenterY

	gmap:SetBytes(gray, { format = "grayscale" })
	gmap:invalidate()

	-- Get some SDF information from a slightly filtered version of the image.
	local gray4 = gmap:GetBytes{ format = "rgba" }
	local fuzzed = impack.ops.box_filter(gray4, w, h, 3, 3) -- limitation: Accelerate expects odd
	local gray2 = impack.grayscale.rgba_to_gray(fuzzed, w, h, { gray_method = "red" })
	local distance = impack.grayscale.coverage_to_distance_field(gray2, w, h)
	local gmap2 = bytemap.newTexture{ width = w, height = h, format = "rgb" }
	local gimage2 = display.newImage(tga_layer, gmap2.filename, gmap2.baseDir)

	gimage2.x, gimage2.y = display.contentCenterX - 100, display.contentCenterY

	gmap2:SetBytes(distance, { format = "grayscale" })
	gmap2:invalidate()
end

-- Do some resize operations.
do
	local original, w, h = impack.image.load("elephant.tga")
	local new_w, new_h

	local function ResizedImage (name, x, y)
		local comp

		if name == "resize_custom" then
			comp = 4
		end

		local resized = impack.ops[name](original, w, h, new_w, new_h, comp)
		local map = bytemap.newTexture{ width = new_w, height = new_h, format = "rgba" }
		local image = display.newImage(scaled_layer, map.filename, map.baseDir)

		image.x, image.y = x, y

		map:SetBytes(resized)
		map:invalidate()
	end

	-- Upscale using a couple algorithms...
	new_w, new_h = math.floor(2.4 * w), math.floor(1.7 * h)

	ResizedImage("resize_rgba", display.viewableContentWidth * .2, display.viewableContentHeight * .75)
	ResizedImage("resize_custom", display.viewableContentWidth * .85, display.viewableContentHeight * .6)

	-- ...and downscale, likewise.
	new_w, new_h = math.floor(.6 * w), math.floor(.9 * h)

	ResizedImage("resize_rgba", display.viewableContentWidth / 5, display.contentCenterY)
	ResizedImage("resize_custom", display.viewableContentWidth * .9, display.viewableContentHeight * .3)
end