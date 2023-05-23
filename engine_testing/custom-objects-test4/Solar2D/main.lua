--- Depth buffer objects test.

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

display.setDefault("isShaderCompilerVerbose", true)

local co4 = require("plugin.customobjects4")
local camera = require("camera")
local obj_parser = require("obj_parser")

-- Kludge to deal with some extra files on Android.
if system.getInfo("platform") == "android" and system.getInfo("environment") == "device" then
  local AssetReader = require("plugin.AssetReader")
  local pathForFile = system.pathForFile

  function system.pathForFile (name, dir)
    if not dir or dir == system.ResourceDirectory then
      if name:sub(-4) ~= ".jpg" and name:sub(-4) ~= ".png" then
        local contents = AssetReader.Read(name)

        if not contents then
          return nil
        end
        
        dir = system.TemporaryDirectory
        name = pathForFile(name, dir)
        
        local file = io.open(name, "w")

        if file then -- this seems a little saner than trying to rewrite the bits in glue / obj_parser
          file:write(contents)
          file:close()
        end

        return name
      end
    end

    return pathForFile(name, dir)
  end
end

display.setDefault("enableDepth", true)

-- Enable 3D and use our own matrices as transforms.
graphics.defineShellTransform{
	name = "3D",

	vertexSource = {
		findAndReplace = {
			["vec2 a_Position"] = "vec3 a_Position",
			["vec2 position"] = "vec3 position",
			["vec2 VertexKernel"] = "vec3 VertexKernel",
      ["mat4 u_ViewProjectionMatrix"] = "mat4 u_UserData0, u_UserData1",
      ["u_ViewProjectionMatrix *"] = "u_UserData0 * u_UserData1 *",
			["position, 1.0"] = { value = "position.xy, 1.0", priority = 1 }, -- do this first...
			["position, 0.0"] = { value = "position", priority = 2 } -- ...else it might step on this one's result
		}
	}
}

-- Random color-changing effect.
graphics.defineEffect{
	category = "filter", name = "test", shellTransform = "3D",

	isTimeDependent = true,

  uniformData = {
    {
      name = "view_projection",
      type = "mat4",
      index = 0, -- u_UserData0
    },
    {
      name = "model",
      type = "mat4",
      index = 1, -- u_UserData1
    },
  },

	fragment = [[
		P_COLOR vec4 FragmentKernel (P_UV vec2 uv)
		{
			P_COLOR vec4 color = texture2D(CoronaSampler0, uv);
		
			color.b *= .5 + .5 * sin(CoronaTotalTime * 1.7);
		
			return CoronaColorScale(color);
		}
	]]
}

-- A non-3D object, before the scope...
local r = display.newRect(display.contentCenterX, display.contentCenterY, 50, 50)

-- ...then the scope itself, with various depth states enabled, including some obvious defaults.
local g = co4.newScopeGroupObject()
local depth_state = co4.newDepthStateObject(g)

depth_state.enabled = true
depth_state.cullFaceEnabled = true
--depth_state.cullFace = "front" -- see note about polygon

-- Create a (fixed) projection and a view that will track the camera.
local projection = co4.newMatrix()
local view = co4.newMatrix()

projection:populatePerspective{ fovy = math.rad(60), aspectRatio = 1.33, zNear = 10, zFar = 1000 }

-- Point a camera somewhere.
local eye = { -175, 20, 0 }
local center, up = { 0, 0, 0 }, { 0, 1, 0 }

camera.Init({ center[1] - eye[1], center[2] - eye[2], center[3] - eye[3] }, up)
camera.Update(-100, 0, 0, 0)

local dir, side = {}, {}

local function ReadCamera ()
  camera.GetVectors(eye, dir, side, up)

  center[1], center[2], center[3] = eye[1] + dir[1], eye[2] + dir[2], eye[3] + dir[3]

  view:populateView{ eye = eye, center = center, up = up }
end

ReadCamera()

display.setDefault("skipsCulling", true)
display.setDefault("skipsHitTesting", true)

-- Create a 3D mesh manually.
local mesh = display.newMesh{
	parent = g,
	x = 100,
	y = 100,
	mode = "triangles",
	vertices = {
		0,0,		10,
		50,0,		11,
		0,100,		 7,
		0,100,		-3,
		50,0,		-9,
		150,190,	13,
		150,190,	-2,
		50,0,		 7,
		100,0,		 6
	},
	uvs = {
		0,0, 0.5,0, 0,1,
		0,1, 0.5,0, 1,1,
		1,1, 0.5,0, 1,0
	},
	hasZ = true
}

mesh.fill = { type = "image", filename = "Image1.jpg" }

mesh.fill.effect = "filter.custom.test"

-- Create a second mesh from a model file.
local vertices, provisionalUVs, indices = {}, {}, {}
local vi_to_uvi = {}
local handlers = {
	check_function = function(check) end,

	vertex = function(x, y, z)
		vertices[#vertices + 1] = x * 100
		vertices[#vertices + 1] = y * 100
		vertices[#vertices + 1] = z * 100
	end,

	normal = function(x, y, z)
		--
	end,

	texcoord = function(u, v)
		provisionalUVs[#provisionalUVs + 1] = u
		provisionalUVs[#provisionalUVs + 1] = v
	end,

	start_face = function() end,

	face_vtn = function(v, t, n)
vi_to_uvi[v] = t
		indices[#indices + 1] = v
	end,

	end_face = function()
		--
	end,

	line = function(v, t) end,
	
	material_def = function(material_name) end,
	
	material_attr = function(cmd, ...)
		--[[
		if cmd == 'ka' or cmd  == 'kd' or cmd == 'ks' then
			local r,g,b = ...
		elseif cmd == 'illum' or cmd == 'ns' or cmd == 'd' or cmd == 'tr' then
			local N = ...
		elseif ({'map_ka', 'map_kd', 'map_ks', 'map_ns', 'map_d', 'map_bump', 'bump', 'disp', 'decal'})[cmd] then
			local filepath = ...
		end
		]]
	end,

	material = function(material_name) end,

	group = function(group_names_t) end,

	smoothing_group = function(group_names_t) end
}

local Name = "squirrel"
			 --"CrumpledDevelopable"

obj_parser(Name .. ".obj.txt", handlers) -- as txt, otherwise Android build seems to reject

local uvs = {}

for vi, uvi in pairs(vi_to_uvi) do -- MOST of these seem to be duplicates, so just run with that idea
	local vo, uvo = 2 * (vi - 1), 2 * (uvi - 1)

	uvs[vo + 1] = provisionalUVs[uvo + 1]
	uvs[vo + 2] = 1 - provisionalUVs[uvo + 2]
end

local mesh2 = display.newMesh{
	parent = g,
	x = 30,
	y = 10,
	mode = "indexed",
	vertices = vertices,
	uvs = uvs,
	indices = indices,
	hasZ = true
}

-- Create a polygon from some data.
mesh2.fill = { type = "image", filename = Name .. ".jpg" }

mesh2.fill.effect = "filter.custom.test"
 
local pvertices = {
	0,-110,		3,
	27,-35,		9,
	105,-35,	2,
	43,16,		8,
	65,90,		0,
	0,45,		6,
	-65,90,		1,
	-43,15,		9,
	-105,-35,	2,
	-27,-35,	4
	}
 
local o = display.newPolygon( g, display.contentCenterX, 50, pvertices, true )
--o.fill = { type="image", filename="Image1.jpg" }
o.strokeWidth = 10 -- the stroke will get back-face culled (see depth_state.cullFace, above)
o:setStrokeColor( 1, 0, 0 )

o.fill.effect = "filter.custom.test"

display.setDefault("skipsCulling", false)
display.setDefault("skipsHitTesting", false)

-- Put each object in an initial state.
local params1, params2 = { yaw = 0 }, { yaw = 0 }
local model = co4.newMatrix()

model:populateIdentity()

local marr = model:getAsArray()

o.fill.effect.model = marr
mesh.fill.effect.model = marr
mesh2.fill.effect.model = marr

-- Add a full-screen object to capture some inputs...
local back = display.newRect(display.contentCenterX, display.contentCenterY, display.contentWidth, display.contentHeight)

back.isHitTestable, back.isVisible = true, false

-- ...in particular, interpret drags as camera rotations.
local prevx, prevy
local total_dx, total_dy = 0, 0

back:addEventListener("touch", function(event)
    local phase = event.phase

    if phase == "began" then
      display.getCurrentStage():setFocus(event.target)

      prevx, prevy = event.x, event.y
    elseif phase == "moved" then
      local dx, dy = event.x - prevx, event.y - prevy
      
      total_dx, total_dy = total_dx + dx, total_dy + dy
      
      prevx, prevy = event.x, event.y
    else
      display.getCurrentStage():setFocus(nil)
    end

    return true
end)

-- Listen to some keys for forward / backward and strafing motion.
local keys = {}

Runtime:addEventListener("key", function(event)
	local keyName = event.keyName

	if keyName == "left" or keyName == "right" or keyName == "up" or keyName == "down" then
		if event.phase == "up" then
			keys[keyName] = false
		elseif event.phase == "down" then
			keys[keyName] = true
		end
	end
end)

-- Add some buttons, since a keyboard won't always be handy.
local buttons = {}

for _, v in ipairs{
  { x = 20, y = display.contentHeight - 100, name = "left" },
  { x = 120, y = display.contentHeight - 100, name = "right" },
  { x = 70, y = display.contentHeight - 170, name = "up" },
  { x = 70, y = display.contentHeight - 30, name = "down" }
} do
  local button = display.newRoundedRect(v.x + display.screenOriginX + 50, v.y, 70, 50, 12)
  
  button.name = v.name
  
  button:setFillColor(0, 0, .9)
  button:addEventListener("touch", function(event)
    if event.phase == "began" then
      display.getCurrentStage():setFocus(event.target)
      
      buttons[event.target.name] = true
    elseif event.phase == "ended" then
      display.getCurrentStage():setFocus(nil)

      buttons[event.target.name] = false
    end

    return true
  end)
end

-- Update object space matrices.
local prev

timer.performWithDelay(50, function(event)
	local dt = prev and (event.time - prev) / 1000 or 0

	prev = event.time

	params1.yaw = params1.yaw + 70 * dt
	params2.yaw = params2.yaw + 40 * dt

  model:populateFromEulerAngles(params1)

  mesh.fill.effect.model = model:getAsArray()

  model:populateFromEulerAngles(params2)

  mesh2.fill.effect.model = model:getAsArray()
end, 0)

-- Set camera matrices for the objects...
local pv = co4.newMatrix() -- projection * view

pv:setProduct(projection, view)

local pv_arr = pv:getAsArray()

o.fill.effect.view_projection = pv_arr
mesh.fill.effect.view_projection = pv_arr
mesh2.fill.effect.view_projection = pv_arr

-- ...then update them over time.
local keyprev

timer.performWithDelay(30, function(event)
  -- Update camera from input.
	local dt = keyprev and (event.time - keyprev) / 1000 or 0

	keyprev = event.time

	local l, r = (keys.left or buttons.left) and 1 or 0, (keys.right or buttons.right) and -1 or 0
	local u, d = (keys.up or buttons.up) and 1 or 0, (keys.down or buttons.down) and -1 or 0

  camera.Update((u + d) * 125 * dt --[[ddir]], (l + r) * 135 * dt --[[dside]], total_dx * dt --[[dx]], total_dy * dt --[[dy]])
  
  total_dx, total_dy = 0, 0
  
  ReadCamera()

  -- Update objects.
  pv:setProduct(projection, view)

  pv_arr = pv:getAsArray()

  o.fill.effect.view_projection = pv_arr
  mesh.fill.effect.view_projection = pv_arr
  mesh2.fill.effect.view_projection = pv_arr
end, 0)

-- Put some other 2D object after the 3D stuff.
local rrect = display.newRoundedRect(display.contentCenterX + 200, display.contentCenterY, 100, 500, 12)

rrect:setFillColor(0, 1, 1)