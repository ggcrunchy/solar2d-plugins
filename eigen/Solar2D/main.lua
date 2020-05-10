display.setDefault( "background", 0.5, 0.5, 0.5 )

local memoryBitmap = require "plugin.memoryBitmap"


local tex = memoryBitmap.newTexture({
	width=100,
	height=100,
	-- format="rgb",
})

local mask = memoryBitmap.newTexture({
	width=100,
	height=100,
	format="mask",
})

display.newImage( tex.filename , tex.baseDir, display.contentCenterX*0.75, display.contentCenterY )

local masked = display.newImageRect( "Icon-76.png", mask.width, mask.height )
masked:translate( display.contentCenterX*1.25, display.contentCenterY )
masked:setMask( graphics.newMask( mask.filename , mask.baseDir ) )

-- this function declared below, just generates a bunch of vertical stripes and a frame
local stripes

local function setAll( tex, c )
	for y=1, tex.height do
		for x=1, tex.width do
			tex:setPixel(x,y, c )
		end
	end
end


setAll(tex, {1,0,0,1})
setAll(mask, {1})


print("Get Pixel: ", tex:getPixel(50,50) )

timer.performWithDelay( 1000, function ( )
	setAll(tex, {0, 1, 0, 1})
	setAll(mask, {0.6})
	tex:invalidate( )
	mask:invalidate( )
	timer.performWithDelay( 1000, function ( )
		setAll(tex, {0, 0, 1, 1})
		setAll(mask, {0.3})
		tex:invalidate( )
		mask:invalidate( )
		timer.performWithDelay(1000, function(  )
			stripes(tex)
			stripes(mask)
			tex:invalidate( )
			mask:invalidate( )
			mask:releaseSelf( )
			tex:releaseSelf( )
		end)
	end )
end )


stripes = function( tex )
	for y=1, tex.height do
		for x=1, tex.width do
			if x < tex.width *0.1 then
				tex:setPixel(x,y, 1, 0, 0, 1 )
			elseif x < tex.width *0.2 then
				tex:setPixel(x,y, 0, 1, 0, 1 )
			elseif x < tex.width *0.3 then
				tex:setPixel(x,y, 0, 0, 1, 1 )
			elseif x < tex.width *0.4 then
				tex:setPixel(x,y, 1, 0, 1, 1 )
			elseif x < tex.width *0.5 then
				tex:setPixel(x,y, 1, 1, 0, 1 )
			elseif x < tex.width *0.6 then
				tex:setPixel(x,y, 0, 1, 1, 1 )
			elseif x < tex.width *0.7 then
				tex:setPixel(x,y, 1, 1, 1, 1 )
			elseif x < tex.width *0.8 then
				tex:setPixel(x,y, 0.75, 0.75, 0.75, 0.75 )
			elseif x < tex.width *0.9 then
				tex:setPixel(x,y, 0.25, 0.25, 0.25, 0.25 )
			else
				tex:setPixel(x,y, 0, 0, 0, 0 )
			end
		end
	end

	local r = 0
	if tex.format == "mask" then
		r = 1
	end

	for b = 1,3 do
		for y=1, tex.height do
			tex:setPixel(b, y, r, 0, 0, 1 )
			tex:setPixel(tex.width-b+1, y, r, 0, 0, 1 )
		end
		for x=1, tex.width do
			tex:setPixel(x, b, r, 0, 0, 1 )
			tex:setPixel(x, tex.height-b+1, r, 0, 0, 1 )
		end
	end
end