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

local captureTexture = graphics.newTexture{ type = "capture", width = display.viewableContentWidth, height = display.viewableContentHeight }

local captureRect = captureTexture:newCaptureRect( display.contentCenterX, display.contentCenterY, 200, 200 )
local circle = display.newCircle( captureRect.x, captureRect.y, 80 )

local back = display.newRect( display.contentCenterX, display.contentCenterY, display.contentWidth, display.contentHeight )

back.isVisible, back.isHitTestable = false, true

back:addEventListener( "touch", function( event )
  if event.phase == "began" then
    captureRect.x, captureRect.y = event.x, event.y
    circle.x, circle.y = event.x, event.y
  end

  return true
end)

graphics.defineEffect{
  category = "filter", name = "wavy",

  vertexData = {
    { index = 0, name = "originX" },
    { index = 1, name = "originY" },
    { index = 2, name = "width" },
    { index = 3, name = "height" }
  },

  vertex = [[
    P_UV varying vec2 v_Coords;
  
    P_POSITION vec2 VertexKernel (P_POSITION vec2 pos)
    {
      P_UV vec2 uv = (pos - CoronaVertexUserData.xy) * CoronaTexelSize.xy;

      v_Coords = vec2( uv.x, 1. - uv.y );

      return pos;
    }
  ]],
  
  fragment = [[
    P_UV varying vec2 v_Coords;
  
    P_COLOR vec4 FragmentKernel (P_UV vec2)
    {
      P_COLOR vec3 c1 = texture2D( CoronaSampler0, v_Coords ).rgb;
      P_COLOR vec3 c2 = texture2D( CoronaSampler0, v_Coords + vec2( CoronaTexelSize.z, 0. ) ).rgb;
      P_COLOR vec3 c3 = texture2D( CoronaSampler0, v_Coords + vec2( 0., CoronaTexelSize.w ) ).rgb;

      return vec4( c1 /*(c1 + c2 + c3) / 3.*/, 1.);
    }
  ]]
}
print("DF", display.screenOriginX, display.screenOriginY)
print("??", captureTexture.pixelWidth, captureTexture.pixelHeight)
print("!!", display.pixelWidth, display.pixelHeight)
print("XX",display.pixelWidth*display.contentScaleX)
print("SS",display.contentScaleX,display.contentScaleY)
print("xxxx",display.contentWidth,display.safeActualContentWidth,display.actualContentWidth)
print("yyyy",display.contentHeight,display.safeActualContentHeight,display.actualContentHeight)


circle.fill = { type = "image", filename = captureTexture.filename, baseDir = captureTexture.baseDir }
circle.fill.effect = "filter.custom.wavy"
circle.fill.effect.originX = display.screenOriginX
circle.fill.effect.originY = display.screenOriginY
circle.fill.effect.width = display.contentWidth
circle.fill.effect.height = display.contentHeight