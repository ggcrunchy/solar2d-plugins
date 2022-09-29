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

local Bytemap = require("plugin.Bytemap")

local bm = Bytemap.loadTexture{ filename = "1261363.png" }
--[[
local image = display.newImageRect(bm.filename, bm.baseDir, 200, 200)

image.x, image.y = display.contentCenterX, display.contentCenterY
]]
bm:MakeSeamless()
bm:invalidate()


for i = 0, 1 do
	for j = 0, 1 do
		local im = display.newImageRect(bm.filename, bm.baseDir, 100, 100)

		im.anchorX, im.x = 0, 50 + i * 100
		im.anchorY, im.y = 0, 50 + j * 100
	end
end


do return end
-- Standard library imports --
local sin = math.sin

-- Plugins --
local Bytemap = require("plugin.Bytemap")
local MemoryBlob = require("plugin.MemoryBlob") -- installs non-dummy blob logic
local moonassimp = require("plugin.moonassimp")
local sceneView3D = require("plugin.sceneView3D")

--
--
--

local WantAlpha = false

local FaceParams = {}

local name = system.pathForFile("squirrel/model_647897911503.obj")
local object, errmsg = moonassimp.import_file(name, -- post-processing flags:
                               "triangulate", "join identical vertices", "sort by p type")
assert(object, errmsg)

local meshes = object:meshes()

for i = 1, object:num_meshes() do
	local mesh = meshes[i]

	print("Mesh",i,mesh:name())
	print("MeshMAT",mesh:material())

	local nverts = mesh:num_vertices()
	local nfaces = mesh:num_faces()
	
	print("NVERTS",i,nverts)
	print("NFACES",i,nfaces)
end

local bm = Bytemap.newTexture{ width = 400, height = 400, format = WantAlpha and "rgba" or "rgb" }
local image = display.newImage(bm.filename, bm.baseDir)

local model = sceneView3D.tinyrenderer.NewModel()
local mesh1 = meshes[1]

for i = 1, mesh1:num_vertices() do
	model:AddVertex(mesh1:position(i))
	model:AddNormal(mesh1:normal(i))
	model:AddUV(mesh1:texture_coords(1, i))
end

for i = 1, mesh1:num_faces() do
	local face = mesh1:face(i)

	assert(face:num_indices() == 3)

	model:AddFace(face:indices())
end

local bmap = Bytemap.loadTexture{ filename = "squirrel/" .. mesh1:material():texture_path("diffuse", 1), format = WantAlpha and "rgba" or "rgb", is_non_external = true }

local texture = sceneView3D.tinyrenderer.NewTexture()

texture:Bind(bmap:GetBytes(), bmap.width, bmap.height, WantAlpha and 4 or 3)

model:SetDiffuse(texture)

local scene = sceneView3D.tinyrenderer.NewScene(bm.width, bm.height, { using_alpha = WantAlpha, using_blob = true })

local object = sceneView3D.tinyrenderer.NewObject(model)

local root = scene:GetRoot()

root:Insert(object)

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

image.x, image.y = display.contentCenterX, display.contentCenterY

bm:BindBlob(scene:GetBlob())

scene:Render()

timer.performWithDelay(30, function(event)
	scene:SetEye(1, 1 + sin(event.count / 7), 3)
	scene:Clear()
	scene:Render()
	bm:invalidate()
end, 0)