--- Show some objects with SSGE.

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
local assert = assert
local cos = math.cos
local ipairs = ipairs
local open = io.open
local pairs = pairs
local pi = math.pi
local rad = math.rad
local tonumber = tonumber
local sin = math.sin

-- Plugins --
local Bytemap = require("plugin.Bytemap")
local MemoryBlob = require("plugin.MemoryBlob") -- installs non-dummy blob logic
local moonassimp = require("plugin.moonassimp")
local sceneView3D = require("plugin.sceneView3D")

-- Solar2D globals --
local display = display
local system = system
local timer = timer

--
--
--

local Width, Height = 800, 800

local ssge = sceneView3D.SSGE.NewEngine{ width = Width, height = Height, using_blob = true }

--
--
--

local NumPatt = "%s+(%-?%d+%.?%d*)"
local Vec2Patt = "(%a+)" .. NumPatt:rep(2)
local Vec3Patt = "(%a+)" .. NumPatt:rep(3)

--
--
--

local function ReadVec2 (iter, what)
	local key, x, y = iter():match(Vec2Patt)

	assert(key == what, "Wrong vec2 read")

	x = assert(tonumber(x), "Invalid x")
	y = assert(tonumber(y), "Invalid y")

	return x, y
end

--
--
--

local function ReadVec3 (iter, what)
	local key, x, y, z = iter():match(Vec3Patt)

	assert(key == what, "Wrong vec3 read")

	x = assert(tonumber(x), "Invalid x")
	y = assert(tonumber(y), "Invalid y")
	z = assert(tonumber(z), "Invalid z")

	return x, y, z
end

--
--
--

local function AuxLoadLight (iter)
	local key, ltype = iter():match("(%w+)%s+(%a)")
	local light = { type = ltype, time = 2 * pi }

	if ltype == "o" or ltype == "l" then
		local radius, period = ReadVec2(iter, ltype == "o" and "orb" or "lin")

		light.radius = radius
		light.time = light.time / (period * 1000)
	else
		assert(ltype == "c" or ltype == "f", "Invalid light type")
	end
	
	light.x, light.y, light.z = ReadVec3(iter, "pos")
	light.r, light.g, light.b = ReadVec3(iter, "col")

	iter()

	return light
end

--
--
--

local TextureNames = {
	albedo = { format = "rgb", suffix = "albedo" },
	normal = { format = "rgb", suffix = "normal" },
	ambient = { format = "red", suffix = "ao" },
	roughness = { format = "red", suffix = "rough" },
	metallic = { format = "red", suffix = "metal" }
}

local function AuxLoadModel (scene, iter, what)
	local model = sceneView3D.SSGE.NewModel()
	local xform = sceneView3D.SSGE.NewTransformParameters()

	--
	--
	--

	local key, mname, mat = iter():match("(%w+)%s+(%w+)%s(%w+)")

	xform:SetTranslation(ReadVec3(iter, "pos"))
	xform:SetRotation(ReadVec3(iter, "rot"))
	xform:SetScaling(ReadVec3(iter, "sca"))

	model:SetTransform(xform)

	--
	--
	--

	print("M",mname,mat,what)

	local import, err = assert(moonassimp.import_file(system.pathForFile(("scenes/%s/meshes/%s_mesh.obj"):format(what, mname)), -- post-processing flags:
												"triangulate", "join identical vertices", "sort by p type"))
	local mesh = import:meshes()[1]									

	for i = 1, mesh:num_vertices() do
		model:AddVertex(mesh:position(i))
		model:AddNormal(mesh:normal(i))
		model:AddUV(mesh:texture_coords(1, i))
	end

	for i = 1, mesh:num_faces() do
		local face = mesh:face(i)

		assert(face:num_indices() == 3)

		model:AddFace(face:indices())

	end

	moonassimp.release_import(import)

	--
	--
	--

	for k, v in pairs(TextureNames) do
		local name = ("scenes/%s/materials/%s/%s_%s.png"):format(what, mat, mat, v.suffix)
		local bmap = Bytemap.loadTexture{ filename = name, is_non_external = true }
		local tex = model:GetTexture(k)

		tex:Bind(bmap:GetBytes{ format = v.format }, bmap.width, bmap.height, 3)
		bmap:Deallocate()
	end

	--
	--
	--

	scene:addModel(model)

	iter()
end

--
--
--

local camera = sceneView3D.SSGE.NewCamera()
local scene = sceneView3D.SSGE.NewScene(camera)

--
--
--

local what = "statue"
local f = open(system.pathForFile(("scenes/%s/%s_config.txt"):format(what, what)))

local lights = {}

if f then
	local iter, s, v = f:lines()
	local did_start, did_camera, did_lights, did_models
	
	assert(not s, "Need fixed state")
	assert(not v, "Need initial value")
	
	while true do
		local section, res = iter() -- per assert() above

		assert(not res, "Need iteration variable")
		-- From now on, assumes no need for v

		if not section then
			break
		else
			local first = section:match("(%w+)%s")

			if first == "s" then
				assert(section:match("s%s+(%w+)") == what, "Config does not belong to current scene")
				assert(not (did_camera or did_lights or did_models), "Must do start section first")
				
				did_start = true

			--
			--
			--

			elseif first == "l" then
				assert(did_start, "Must do start before lights")
				assert(not did_lights, "Already did lights")
				
				local n = assert(tonumber(section:match("l%s+(%d+)")), "Invalid light count")

				scene:initializeLights(n)
				
				for i = 1, n do
					local v = AuxLoadLight(iter)
					
					scene:setLightColor(i, v.r, v.g, v.b)
					scene:setLightPosition(i, v.x, v.y, v.z)
	
					lights[i] = v
				end
				
				did_lights = true

			--
			--
			--

			elseif first == "m" then
				assert(did_start, "Must do start before models")
				assert(not did_models, "Already did models")
				
				local n = assert(tonumber(section:match("m%s+(%d+)")), "Invalid model count")
				
				for i = 1, n do
					AuxLoadModel(scene, iter, what)
				end
				
				did_models = true

			--
			--
			--

			elseif first == "c" then
				assert(did_start, "Must do start before camera")
				assert(not did_camera, "Already did camera")
				
				did_camera = true
			end
		end
	end

	f:close()
end

--
--
--

local bm = Bytemap.newTexture{ width = Width, height = Height }
local image = display.newImageRect(bm.filename, bm.baseDir, 400, 400)

image.x, image.y = display.contentCenterX, display.contentCenterY

bm:BindBlob(ssge:getBlob())

--
--
--

local pitch, yaw, orbiting, period, radius

local function Reset ()
	orbiting, period, radius = true, 30, 2
	pitch, yaw = 0, -90
end

Reset()

--
--
--

ssge:run(scene, 0)

local was = system.getTimer()

timer.performWithDelay(30, function(event)
	local now = event.time

	--
	--
	--

	if orbiting then
		local ang = 2 * pi * now / (period * 1000)
		local cam_x, cam_z = sin(ang) * radius, cos(ang) * radius
		
		camera:setPosition(cam_x, cam_x, cam_z)
	else
		local px, py, pz = camera:getPosition()
		local fx, fy, fz = camera:getFront()

		camera:setTarget(px + fx, py + fy, pz + fz)
	end

	--
	--
	--

	for i, v in ipairs(lights) do
		local ltype, ang = v.type, now * v.time

		if ltype == "o" then
			local radius = v.radius
			local orb_x, orb_z = sin(ang) * radius, cos(ang) * radius

			scene:setLightPosition(i, orb_z, orb_x, orb_x)
		elseif ltype == "c" then
			local col_x, col_y = (sin(ang / 12e3) + 1) / 2, (cos(ang / 6e3) + 1) / 2

			scene:setLightColor(i, col_x, 1 - col_y, col_y)
		elseif ltype == "l" then
			local _, y, z = scene:getLightPosition(i)

			scene:setLightPosition(i, sin(ang), y, z)
		end
	end

	--
	--
	--

	ssge:run(scene, now - was)
	bm:invalidate()

	was = now
end, 0)

--
--
--

local Reverse = { s = true, a = true, e = true, up = true }

local function GetDelta (key, v)
	return Reverse[key] and -v or v
end

--
--
--

local CamSpeed = .1

Runtime:addEventListener("key", function(event)
	if event.phase == "down" then
		local speed, dx, dy, dz = GetDelta(event.keyName, CamSpeed)

		if event.keyName == "w" or event.keyName == "s" then
			if orbiting then
				radius = radius + speed
			else
				dx, dy, dz = camera:getFront()
			end
		elseif event.keyName == "a" or event.keyName == "d" then
			dx, dy, dz = camera:getSide()
		elseif event.keyName == "q" or event.keyName == "e" then
			dx, dy, dz = camera:getUp()
		elseif event.keyName == "r" then
			camera:reset()

			Reset()
		elseif event.keyName == "tab" then
			orbiting = not orbiting
		elseif event.keyName == "up" or event.keyName == "down" then
			local delta = GetDelta(event.keyName, 2)
			local new_period = period + delta
			
			if new_period >= 4 and new_period <= 60 then
				period = new_period
			end
		end

		if dx then
			local x, y, z = camera:getPosition()

			camera:setPosition(x + dx * speed, y + dy * speed, z + dz * speed)
		end
	end

	return true
end)

--
--
--

local OldX, OldY

Runtime:addEventListener("mouse", function(event)
	if event.type == "down" then
		if event.isPrimaryButtonDown and not OldX then
			OldX, OldY = event.x, event.y
		end
	elseif event.type == "up" then
		if not event.isPrimaryButtonDown and OldX then
			OldX = nil
		end
	elseif event.type == "scroll" then
		local fov = camera:getFOV()

        if event.scrollY > 0 then -- scroll up
            fov = fov - 5
        elseif event.scrollY < 0 then
            fov = fov + 5
		else
			return
        end

		-- Limiting the FOV range to avoid low FPS values or weird distortion
        if fov < 20 then
            fov = 20
        elseif fov > 120 then
            fov = 120
        end

        -- Updating the camera frustrum
		camera:setFOV(fov)
	elseif event.type == "drag" and OldX then
		local dx, dy = (event.x - OldX) * .35, (event.y - OldY) * .35

		yaw = yaw + dx
        pitch = pitch + dy

        -- Limiting the range of the pitch to avoid flips
		if pitch > 89.0 then
			pitch =  89.0
		elseif pitch < -89.0 then
			pitch = -89.0
		end

		-- Updating the front and side vectors to allow wasd movement and 
		-- free camera movement.
		local pr, yr = rad(pitch), rad(yaw)
		local x = cos( pr ) * cos( yr )
		local y = sin( pr )
		local z = cos( pr ) * sin( yr )

		camera:setFront(x, y, z)

		OldX, OldY = event.x, event.y
	end
end)

