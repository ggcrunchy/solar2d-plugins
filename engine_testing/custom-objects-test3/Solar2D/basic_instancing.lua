--- Instancing proof-of-concept.

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

local co3 = require("plugin.customobjects3")
  
local b1 = co3.registerBasicInstancingDataType()
local b2 = co3.registerBasicInstancingDataType() -- can re-register?

print("REGISTERED?", b1, b2)

local VertexTransforms = {
  findAndReplace = {
    ["vec2 a_Position"] = "vec3 a_Position",
    ["( a_Position )"] = "( a_Position.xy )"
  },

  findAndInsertAfter = {
    -- Find this...
    ["v_UserData = a_UserData;"] =

    -- ...then insert this after:
    "\n" ..
    "\tv_InstanceIndex = a_Position.z;\n"
  }
}

local CommonTransforms = {
  findAndReplace = {
    -- Find this...
    ["vec2 v_Position;"] =

    -- ...and replace with:
    "float v_InstanceIndex;\n" ..
    "\n" ..
    "#define CoronaInstanceIndex int( v_InstanceIndex )\n" ..
    "#define CoronaInstanceFloat v_InstanceIndex\n"
  }
}

for k, v in pairs(CommonTransforms) do
  local vv = VertexTransforms[k] or {}

  for fk, fv in pairs(v) do
    vv[fk] = fv
  end

  VertexTransforms[k] = vv
end

local c1 = graphics.defineShellTransform{ name = "instances", fragmentSource = CommonTransforms, vertexSource = VertexTransforms }
local c2 = graphics.defineShellTransform{ name = "instances" } -- allow redefinition?

print("DEFINE SHELL TRANSFORM?", c1, c2)

graphics.defineEffect{
  category = "generator", name = "instanced",

  dataType = "basicInstances", details = { supportsInstancing = "z" }, shellTransform = "instances",
  isTimeDependent = true,

  vertex = [[
    P_POSITION vec2 VertexKernel (P_POSITION vec2 pos)
    {
      return pos + vec2(80. * CoronaInstanceFloat, 150. * sin(3.14159 / 7. * CoronaInstanceFloat + CoronaTotalTime));
    }
  ]],
  
  fragment = [[
    P_COLOR vec4 FragmentKernel (P_UV vec2 uv)
    {
      P_COLOR vec4 color = vec4(.5 + .5 * cos(150. * CoronaInstanceFloat + CoronaTotalTime), 0., .5 + .5 * sin(25. * CoronaInstanceFloat), 1.);
      
      return CoronaColorScale(color);
    }
  ]]
}

local r = display.newRect(display.contentCenterX, display.contentCenterY, 100, 150)

r.fill.effect = "generator.custom.instanced"

print("1", r.fill.effect.instanceCount)

r.fill.effect.instanceCount = 3

print("2", r.fill.effect.instanceCount)