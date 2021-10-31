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

local co4 = require("plugin.customobjects4")
local obj_parser = require("obj_parser")

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

local r = display.newRect(display.contentCenterX, display.contentCenterY, 50, 50)

local g = co4.newScopeGroupObject()
local depth_state = co4.newDepthStateObject(g)

local eye = { -175, 20, 0 }
local length = math.sqrt(eye[1]^2 + eye[2]^2)
local dir = { -eye[1] / length, 0, -eye[2] / length }
local angle = math.atan2(dir[2], dir[1])
local center, up = { 0, 0, 0 }, { 0, 1, 0 }

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

local keyprev

local projection = co4.newMatrix()
local view = co4.newMatrix()

depth_state.enabled = true

projection:populatePerspective{ fovy = math.rad(60), aspectRatio = 1.33, zNear = .1, zFar = 1000 }
view:populateView{ eye = eye, center = center, up = up }

local mesh = co4.newTransformableMesh{
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

obj_parser(system.pathForFile(Name .. ".obj"), handlers)

local uvs = {}

for vi, uvi in pairs(vi_to_uvi) do -- MOST of these seem to be duplicates, so just run with that idea
	local vo, uvo = 2 * (vi - 1), 2 * (uvi - 1)

	uvs[vo + 1] = provisionalUVs[uvo + 1]
	uvs[vo + 2] = 1 - provisionalUVs[uvo + 2]
end

local mesh2 = co4.newTransformableMesh{
	parent = g,
	x = 30,
	y = 10,
	mode = "indexed",
	vertices = vertices,
	uvs = uvs,
	indices = indices,
	hasZ = true
}

mesh2.fill = { type = "image", filename = Name .. ".jpg" }

mesh2.fill.effect = "filter.custom.test"

local halfW = 115--display.contentWidth * 0.5
local halfH = 110--display.contentHeight * 0.5
 
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
 
local o = co4.newTransformablePolygon( g, halfW, halfH, pvertices, true )
--o.fill = { type="image", filename="Image1.jpg" }
o.strokeWidth = 10
o:setStrokeColor( 1, 0, 0 )

o.fill.effect = "filter.custom.test"

local prev

local params1, params2 = { yaw = 0 }, { yaw = 0 }
local model = co4.newMatrix()

model:populateIdentity()

local marr = model:getAsArray()

o.fill.effect.model = marr
mesh.fill.effect.model = marr
mesh2.fill.effect.model = marr

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

local pv = co4.newMatrix()

pv:setProduct(projection, view)

local pv_arr = pv:getAsArray()

o.fill.effect.view_projection = pv_arr
mesh.fill.effect.view_projection = pv_arr
mesh2.fill.effect.view_projection = pv_arr

timer.performWithDelay(30, function(event)
	local dt = keyprev and (event.time - keyprev) / 1000 or 0

	keyprev = event.time

	local l, r = keys.left and 1 or 0, keys.right and -1 or 0
	local u, d = keys.up and -1 or 0, keys.down and 1 or 0

	angle = (angle + (l + r) * math.pi / 2 * dt) % (2 * math.pi)
	length = math.max(25, length + (u + d) * 35 * dt)
	eye[1], eye[2] = length * math.cos(angle), length * math.sin(angle)

	view:populateView{ eye = eye, center = center, up = up }

  pv:setProduct(projection, view)

  pv_arr = pv:getAsArray()

  o.fill.effect.view_projection = pv_arr
  mesh.fill.effect.view_projection = pv_arr
  mesh2.fill.effect.view_projection = pv_arr
end, 0)

local rrect = display.newRoundedRect(display.contentCenterX + 200, display.contentCenterY, 100, 500, 12)

rrect:setFillColor(0, 1, 1)