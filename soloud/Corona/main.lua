--- 3D object thing.

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
local abs = math.abs
local cos = math.cos
local floor = math.floor
local huge = math.huge
local ipairs = ipairs
local max = math.max
local min = math.min
local sin = math.sin

-- Modules --
local line = require("line")
local obj_parser = require("obj_parser")

-- Plugins --
local Bytemap = require("plugin.Bytemap")
local MemoryBlob = require("plugin.MemoryBlob") -- installs non-dummy blob logic
local object3d = require("plugin.object3d")
local memoryBitmap = require("plugin.memoryBitmap")

local WantAlpha = false

local mode = "diffuse"

local NeedAlpha = WantAlpha or mode == "uvs"

local obj = object3d.New(400, 400, NeedAlpha)

local FaceParams = {}

local xmin, ymin, zmin = huge, huge, huge
local xmax, ymax, zmax = -huge, -huge, -huge

local handlers = {
	check_function = function(check) end,

	vertex = function(x, y, z)
		xmax, ymax, zmax = max(x, xmax), max(y, ymax), max(z, zmax)
		xmin, ymin, zmin = min(x, xmin), min(y, ymin), min(z, zmin)

		obj:AddVertex(x, y, z)
	end,

	normal = function(x, y, z)
		obj:AddNormal(x, y, z)
	end,

	texcoord = function(u, v)
		obj:AddUV(u, v)
	end,

	start_face = function() end,

	face_vtn = function(v, t, n)
		FaceParams[#FaceParams + 1] = v
		FaceParams[#FaceParams + 1] = t
		FaceParams[#FaceParams + 1] = n
	end,

	end_face = function()
		obj:AddFace(unpack(FaceParams))

		for i = #FaceParams, 1, -1 do
			FaceParams[i] = nil
		end
	end,

	line = function(v, t) end,
	
	material_def = function(material_name) end,
	
	material_attr = function(cmd, ...)
		if cmd == 'ka' or cmd  == 'kd' or cmd == 'ks' then
			local r,g,b = ...
		elseif cmd == 'illum' or cmd == 'ns' or cmd == 'd' or cmd == 'tr' then
			local N = ...
		elseif ({'map_ka', 'map_kd', 'map_ks', 'map_ns', 'map_d', 'map_bump', 'bump', 'disp', 'decal'})[cmd] then
			local filepath = ...
		end
	end,

	material = function(material_name) end,

	group = function(group_names_t) end,

	smoothing_group = function(group_names_t) end
}

obj_parser(system.pathForFile("squirrel/model_647897911503.obj"), handlers)

local bm = Bytemap.newTexture{ width = 400, height = 400, format = NeedAlpha and "rgba" or "rgb" }
local image

if mode == "uvs" then
	local kernel = { category = "composite", name = "uv_to_diffuse" }

	kernel.fragment = [[
		P_UV vec2 DecodeTwoFloatsRGBA (P_COLOR vec4 rgba)
		{
			return vec2(dot(rgba.xy, vec2(1., 1. / 255.)), dot(rgba.zw, vec2(1., 1. / 255.)));
		}

		P_COLOR vec4 FragmentKernel (P_UV vec2 uv)
		{
			uv = DecodeTwoFloatsRGBA(texture2D(CoronaSampler0, uv));

			return texture2D(CoronaSampler1, uv);
		}
	]]

	graphics.defineEffect(kernel)

	image = display.newRect(0, 0, 400, 400)

	image.fill = {
		type = "composite",
		paint1 = { type = "image", filename = bm.filename, baseDir = bm.baseDir },
		paint2 = { type = "image", filename = "squirrel/texture_647897911503.jpg" }
	}

	image.fill.effect = "composite.custom.uv_to_diffuse"

	obj:SetDiffuse("uvs")
else
	image = display.newImage(bm.filename, bm.baseDir)

	if mode == "diffuse" then
		local bmap = Bytemap.loadTexture{ filename = "squirrel/texture_647897911503.jpg", format = NeedAlpha and "rgba" or "rgb", is_non_external = true }

		obj:SetDiffuse(bmap:GetBytes(), bmap.width, bmap.height, NeedAlpha and 4 or 3)
	else
		-- all white
	end
end
--[[
-- test alpha:
local g = display.newGroup()
g:toBack()
for i = 1, 50 do
  local x = math.random(100, display.contentWidth - 100)
  local d = display.newCircle(g,x, display.contentCenterY, math.random(8, 15))

  d:setFillColor(math.random(), math.random(), math.random())
end
--]]
---[[
image.x, image.y = display.contentCenterX, display.contentCenterY

bm:BindBlob(obj:GetBlob())

obj:Render()

timer.performWithDelay(30, function(event)
	obj:SetEye(1, 1 + sin(event.count / 7), 3)
	obj:Render()
	bm:invalidate()
end, 0)
--]]
do return end
local CellDim = .15 -- with test model, gives density of 1 to 8 faces per cube

local grid = {}

local function CellIndex (v)
	return floor(v / CellDim) + 1
end

local Bleed = .15 * CellDim

for fi = 1, #obj do
	local i1, i2, i3 = obj:GetFaceVertexIndices(fi)
	local x1, y1, z1 = obj:GetVertex(i1)
	local x2, y2, z2 = obj:GetVertex(i2)
	local x3, y3, z3 = obj:GetVertex(i3)
	local xa, xb = min(x1, x2, x3) - Bleed, max(x1, x2, x3) + Bleed
	local ya, yb = min(y1, y2, y3) - Bleed, max(y1, y2, y3) + Bleed
	local za, zb = min(z1, z2, z3) - Bleed, max(z1, z2, z3) + Bleed

	for y = CellIndex(ya), CellIndex(yb) do
		local zcells = grid[y] or {}

		for z = CellIndex(za), CellIndex(zb) do
			local xcells = zcells[z] or {}
				
			for x = CellIndex(xa), CellIndex(xb) do
				local faces = xcells[x] or {}

				xcells[x], faces[#faces + 1] = faces, fi
			end

			zcells[z] = xcells
		end

		grid[y] = zcells
	end
end

--
local extent = ((xmax - xmin)^2 + (zmax - zmin)^2)^.5
local R = 64 * extent
local HorzPerPixel, VertPerPixel = 1, 1

local HorzN = math.ceil(2 * math.pi * R) * HorzPerPixel
local VertN = 128 * (ymax - ymin) * VertPerPixel

HorzN = math.ceil(HorzN / 4) * 4
VertN = math.ceil(VertN)

--
local tex = memoryBitmap.newTexture{ width = HorzN, height = VertN, format = "rgb" }
local object = display.newImage(tex.filename, tex.baseDir)

object:setStrokeColor(0, 0, 1)

object.x, object.y = display.contentCenterX, display.contentHeight - object.height / 2 - 50
object.strokeWidth = 3

local cy, dy = ymax, (ymin - ymax) / VertN
local a0, da = 0, 2 * math.pi / HorzN
local dist = 1.5 * extent
local examined, pixel_id = {}, 1

local now

local function MaybeYield ()
	local t = system.getTimer()

	if not now then
		now = t
	elseif t - now > 50 then
		tex:invalidate()

		coroutine.yield()

		now = system.getTimer()
	end
end

local co = coroutine.create(function()
	for y = 1, VertN do
		local angle, yi = a0, CellIndex(cy)
		local zcells = grid[yi]

		for x = 1, HorzN do
			local dx, dz = cos(angle), sin(angle)
			local px, pz = dx * dist, dz * dist

			for xi, zi in line.LineIter(px, pz, -px, -pz, CellDim) do
				local xcells = zcells and zcells[zi]
				local faces, tmin = xcells and xcells[xi]

				for i = 1, #(faces or "") do
					local fi = faces[i]

					if examined[fi] ~= pixel_id then
						examined[fi] = pixel_id

						local t, r, g, b, a = obj:GetColor(fi, px, cy, pz, 0, cy, 0)

						if t and (not tmin or t < tmin) then
							tmin = t

							if a then
								--
							else
								tex:setPixel(x, y, r, g, b)
							end
						end
					end
				end

				--
				if tmin then
					break
				end

				MaybeYield()
			end

			angle, pixel_id = angle + da, pixel_id + 1
		end

		cy = cy + dy
	end
end)

timer.performWithDelay(75, function(event)
	if coroutine.status(co) == "dead" then
		timer.cancel(event.source)
	else
		coroutine.resume(co)
	end
end, 0)


local rect = display.newRect(0, 0, 200, 300)

rect:setStrokeColor(1, 0, 0)

rect.x, rect.y = display.contentWidth - rect.width / 2 - 50, display.contentCenterY
rect.strokeWidth = 3

rect.fill = { type = "image", filename = tex.filename, baseDir = tex.baseDir }

do
	local kernel = { category = "filter", name = "cylinder" }

	kernel.vertexData = {
		{ name = "angle", index = 0, default = 0 }
	}
	
	kernel.fragment = [[
		P_COLOR vec4 FragmentKernel (P_UV vec2 uv)
		{
			P_UV float ca = 2. * uv.x - 1.;
			P_UV float angle = acos(ca) + CoronaVertexUserData.x;

			uv.x = fract(angle / (2. * 3.14159));

			return CoronaColorScale(texture2D(CoronaSampler0, uv));
		}
	]]
	
	graphics.defineEffect(kernel)
end

rect.fill.effect = "filter.custom.cylinder"

local l, r = display.newCircle(0, 0, 15), display.newCircle(0, 0, 15)

l.x, l.y = rect.x - 35, rect.y - rect.height / 2 - 20
r.x, r.y = rect.x + 35, rect.y - rect.height / 2 - 20

local delta = 0

l:addEventListener("touch", function(event)
	if event.phase == "began" then
		delta = 1
	elseif event.phase == "ended" or event.phase == "cancelled" then
		delta = 0
	end

	return true
end)

r:addEventListener("touch", function(event)
	if event.phase == "began" then
		delta = -1
	elseif event.phase == "ended" or event.phase == "cancelled" then
		delta = 0
	end

	return true
end)

local at

Runtime:addEventListener("enterFrame", function(event)
	local now = event.time
	local diff = at and now - at or 0

	rect.fill.effect.angle = rect.fill.effect.angle + delta * diff * .75 / 1000

	at = now
end)

-- Can make mask using "none" mode... (eye level with center at middle z, every x degrees rotation...)