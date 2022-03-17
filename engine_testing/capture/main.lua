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
--[[
for i = 1, 1000 do
  local x, y, radius = math.random( 10, display.contentWidth - 10 ), math.random( 10, display.contentHeight - 10 ), math.random( 5, 15 )
  local circle = display.newCircle( x, y, radius )

  circle:setFillColor( math.random(), math.random(), math.random() )
end
]]
local image = display.newImageRect( "Image1.jpg", display.viewableContentWidth, display.viewableContentHeight )

image.x, image.y = display.contentCenterX, display.contentCenterY

local captureTexture = graphics.newTexture{ type = "capture", width = 200, height = 200 }

local captureEvent = captureTexture:newCaptureEvent( display.contentCenterX, display.contentCenterY )
local circle = display.newCircle( captureEvent.x, captureEvent.y, 100 )
--circle.isVisible=false
circle.yScale=-1
local aa = display.newImageRect( captureTexture.filename, captureTexture.baseDir, 75, 75 )

aa.x, aa.y = display.contentCenterX + 150, display.contentCenterY

aa.strokeWidth = 3

aa:setStrokeColor(1, 0, 0)
aa.yScale=-1
local rr=display.newRect(captureEvent.x,captureEvent.y,200,200)
rr:setFillColor(0,0)
rr:setStrokeColor(0,1,0)
rr.strokeWidth=2
local back = display.newRect( display.contentCenterX, display.contentCenterY, display.contentWidth, display.contentHeight )

back.isVisible, back.isHitTestable = false, true

back:addEventListener( "touch", function( event )
  if event.phase == "began" or event.phase == "moved" then
    if event.phase == "began" then
      display.getCurrentStage():setFocus(event.target)
    end
    captureEvent.x, captureEvent.y = event.x, event.y
    circle.x, circle.y = event.x, event.y
    rr.x,rr.y=event.x,event.y
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

circle.fill = { type = "image", filename = captureTexture.filename, baseDir = captureTexture.baseDir }
--circle.fill.effect = "filter.custom.wavy"