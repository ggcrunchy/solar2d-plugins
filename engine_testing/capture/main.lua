--- Capture rects test.

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

local image = display.newImageRect( "Image1.jpg", display.viewableContentWidth, display.viewableContentHeight )

image.x, image.y = display.contentCenterX, display.contentCenterY

local captureTexture1 = graphics.newTexture{ type = "capture", width = 200, height = 200 }
local captureTexture2 = graphics.newTexture{ type = "capture", width = 100, height = 150 }

local captureEvent1 = captureTexture1:newCaptureEvent( display.contentCenterX, display.contentCenterY ) -- capture to texture #1...
local circle = display.newCircle( captureEvent1.x, captureEvent1.y, 100 )

circle.yScale = -1

local image1 = display.newImageRect( captureTexture1.filename, captureTexture1.baseDir, 75, 75 ) -- ...sample it in one spot...

image1:setStrokeColor( 1, 0, 0 )

image1.x, image1.y = display.contentCenterX + 150, display.contentCenterY
image1.strokeWidth = 3
image1.yScale = -1

local image2 = display.newImageRect( captureTexture1.filename, captureTexture1.baseDir, 120, 120 ) -- ...and in another

image2:setStrokeColor( 0, 1, 1 )

image2.x, image2.y = image1.x, image1.y + 250
image2.strokeWidth = 3

local captureEvent2 = captureTexture2:newCaptureEvent( image2.x - 20, image2.y + 20 ) -- capture to texture #2 (picking up part of image2)...
local image3 = display.newRoundedRect( display.contentCenterX - 200, display.contentCenterY - 100, 100, 100, 12 )

image3:setStrokeColor( 1, 0, 1 )

image3.fill = { type = "image", filename = captureTexture2.filename, baseDir = captureTexture2.baseDir } -- ...and sample it...
image3.rotation = 35
image3.strokeWidth = 3

local outline = display.newRect( captureEvent1.x, captureEvent1.y, 200, 200 )

outline:setFillColor( 0, 0 )
outline:setStrokeColor( 0, 1, 0 )

outline.strokeWidth = 2

local captureEvent3 = captureTexture2:newCaptureEvent( display.contentCenterX - 100, 100 ) -- ... then later capture to it from somewhere else... 
local image4 = display.newImageRect( captureTexture2.filename, captureTexture2.baseDir, 250, 150 ) -- ...and then sample that

image4:setStrokeColor( 1, 1, 0 )

image4.x, image4.y = display.contentCenterX, display.contentHeight - 150
image4.strokeWidth = 3

local back = display.newRect( display.contentCenterX, display.contentCenterY, display.contentWidth, display.contentHeight )

back.isVisible, back.isHitTestable = false, true

back:addEventListener( "touch", function( event )
  if event.phase == "began" or event.phase == "moved" then
    if event.phase == "began" then
      display.getCurrentStage():setFocus(event.target)
    end

    captureEvent1.x, captureEvent1.y = event.x, event.y
    circle.x, circle.y = event.x, event.y
    outline.x, outline.y = event.x, event.y
  else
    display.getCurrentStage():setFocus(nil)
  end

  return true
end)

graphics.defineEffect{
  category = "filter", name = "wavy",

  fragment = [[
    P_COLOR vec4 FragmentKernel (P_UV vec2 uv)
    {
      P_COLOR vec3 c1 = texture2D( CoronaSampler0, vec2( uv.x, 1. - uv.y ) ).rgb;

      return vec4( c1, 1. );
    }
  ]]
}

circle.fill = { type = "image", filename = captureTexture1.filename, baseDir = captureTexture1.baseDir }
--circle.fill.effect = "filter.custom.wavy"

-- For feature-testing on phones etc.:
local ext = system.getInfo( "GL_EXTENSIONS" )
local ok = ext:find( "GL_NV_framebuffer_blit" ) or ext:find( "GL_ANGLE_framebuffer_blit" )

local obj1 = display.newCircle( display.contentCenterX - 30, display.contentCenterY, 10 )
local obj2 = display.newCircle( display.contentCenterX + 30, display.contentCenterY, 10 )

if system.getInfo( "gpuSupportsScaledCaptures" ) then
  obj1:setFillColor( 0, 0, 1 )
else
  obj1:setFillColor( 0 )
end

if ok then
  obj2:setFillColor( 1, 0, 0 )
else
  obj2:setFillColor( 0 )
end