--- Show some objects with tinyrenderer.

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
local sin = math.sin

-- Plugins --
local Bytemap = require("plugin.Bytemap")
local impack = require("plugin.impack")
local MemoryBlob = require("plugin.MemoryBlob") -- installs non-dummy blob logic
local moonassimp = require("plugin.moonassimp")
local tinyrenderer = require("plugin.tinyrenderer")

--
--
--

local WantAlpha = true--false

local FaceParams = {}

local name = system.pathForFile("squirrel/model_647897911503.obj")
local import, errmsg = moonassimp.import_file(name, -- post-processing flags:
							   "triangulate", "join identical vertices", "sort by p type")
assert(import, errmsg)

local meshes = import:meshes()

for i = 1, import:num_meshes() do
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

local model = tinyrenderer.NewModel()
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

---[[
--TEST for CoronaMemory...
--local data, w, h = impack.image.load("squirrel/" .. mesh1:material():texture_path("diffuse", 1))
--print("??", data and #data, w, h)
local bmap = Bytemap.loadTexture{ filename = "squirrel/" .. mesh1:material():texture_path("diffuse", 1), is_non_external = true }
--local bmap = Bytemap.newTexture{ is_non_external = true,width=w, height=h,format="rgb" }
--bmap:SetBytes(data)
--]]

moonassimp.release_import(import)

local texture = tinyrenderer.NewTexture()

texture:Bind(bmap:GetBytes{ format = WantAlpha and "rgba" or "rgb" }, bmap.width, bmap.height, WantAlpha and 4 or 3)

model:SetDiffuse(texture)

local scene = tinyrenderer.NewScene(bm.width, bm.height, { has_alpha = WantAlpha, using_blob = true })

local object = tinyrenderer.NewObject(model)

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
--	scene:SetEye(20 * (sin(event.count / 15)), 1, 3)
  object:SetPosition(sin(event.count / 100), 0, 0)
  object:SetScale(1.5 + .75 * sin(event.count / 20), 1,1)
	scene:Clear()
	scene:Render()
	bm:invalidate()
end, 0)

timer.performWithDelay(350, function(event)
do return end
  local resize = WantAlpha and impack.ops.resize_rgba or impack.ops.resize_rgb
  local bytes = resize(scene:GetBlob(), bm.width, bm.height, 100, 100)
  local frame = Bytemap.newTexture{ width = 100, height = 100, format = WantAlpha and "rgba" or "rgb" }

  frame:SetBytes(bytes)

  if WantAlpha then
    frame:PremultiplyAlpha()
  end
  
  local image = display.newImage(frame.filename, frame.baseDir)

  image.x, image.y = math.random(100, display.contentWidth - 100), math.random(100, display.contentHeight - 100)
end, 10)

--[=[
local parshapes = require("plugin.parshapes")

local function ExportAndFree (mesh, name)
  mesh:export(("test_shapes_%s.obj"):format(name))
  mesh:free_mesh()
end

local DoCylindersAndSpheresTest =  false
local DoCreatePlane = false
local DoExports = false
local DoMerge = false
local DoTransforms = false
local DoMiscShapes = false
local DoLsystem = false
local DoPlatonicSolids = false
local DoParametricSurfaces = false

if DoCylindersAndSpheresTest then
  print("should fail when the number of stacks or slices is invalid")

  local bad1 = parshapes.create_cylinder(1, 1)
  local bad2 = parshapes.create_cylinder(1, 3)
  local good = parshapes.create_cylinder(3, 1)

  assert(not bad1)
  assert(not bad2)
  assert(good)

  good:free_mesh(good)

  do
    print("should generate correct number of vertices")

    local m = parshapes.create_cylinder(5, 6)

    assert(m:get_point_count() == 42)

    m:free_mesh()
  end

  print("should have expected number of triangles")

  do
    local slices, stacks, m

    slices, stacks = 5, 6
    m = parshapes.create_cylinder(slices, stacks)

    assert(m:get_triangle_count() == slices * stacks * 2)

    m:free_mesh()

    slices, stacks = 5, 6
    m = parshapes.create_parametric_sphere(slices, stacks)

    assert(m:get_triangle_count() == slices * 2 + (stacks - 2) * slices * 2)

    m:free_mesh()

    slices, stacks = 12, 13
    m = parshapes.create_parametric_sphere(slices, stacks)

    assert(m:get_triangle_count() == slices * 2 + (stacks - 2) * slices * 2)

    m:free_mesh()

    slices, stacks = 16, 16
    m = parshapes.create_parametric_sphere(slices, stacks);

    assert(m:get_triangle_count() == slices * 2 + (stacks - 2) * slices * 2)

    m:free_mesh()
  end
end

if DoCreatePlane then
  print("should not have NaN's")

  local m = parshapes.create_plane(5, 6)

  for i = 1, m:get_point_count() do
    local px, py, pz = m:get_point(i)
    local nx, ny, nz = m:get_normal(i)

    assert(px == px)
    assert(py == py)
    assert(pz == pz)
    assert(nx == nx)
    assert(ny == ny)
    assert(nz == nz)
  end

  m:free_mesh()
end

if DoExports then
  ExportAndFree(parshapes.create_torus(7, 10, 0.5), "torus")
  ExportAndFree(parshapes.create_subdivided_sphere(2), "ssphere")
  ExportAndFree(parshapes.create_klein_bottle(10, 20), "klein")
  ExportAndFree(parshapes.create_trefoil_knot(20, 100, 0.5), "trefoil")
  ExportAndFree(parshapes.create_hemisphere(5, 6), "hemisphere")
  ExportAndFree(parshapes.create_icosahedron(), "icosahedron")
  ExportAndFree(parshapes.create_dodecahedron(), "dodecahedron")
  ExportAndFree(parshapes.create_octahedron(), "octahedron")
  ExportAndFree(parshapes.create_tetrahedron(), "tetrahedron")
  ExportAndFree(parshapes.create_cube(), "cube")
  ExportAndFree(parshapes.create_rock(1, 3), "rock")
  ExportAndFree(parshapes.create_cone(15, 3), "cone")
  ExportAndFree(parshapes.create_parametric_disk(15, 3), "parametric_disk")

  local center = { x = 0, y = 0, z = 0 }
  local normal = { x = 0, y = 0, z = 1 }

  ExportAndFree(parshapes.create_disk(1, 5, center, normal), "disk")
end

if DoMerge then
  print("should concatenate two meshes")

  local a = parshapes.create_klein_bottle(10, 20)
  local npts = a:get_point_count()
  local ntris = a:get_triangle_count()
  local b = parshapes.create_plane(3, 3)

  a:merge(b)

  assert(a:get_point_count() == npts + b:get_point_count())
  assert(a:get_triangle_count() == ntris + b:get_triangle_count())

  a:free_mesh()
  b:free_mesh()
end

if DoTransforms then
  local Export = true

  do
    print("should support translation")
    
    local a = parshapes.create_cylinder(20, 3)
    local b = parshapes.create_cylinder(4, 3)

    a:translate(0.5, 0.5, 0.25)
    a:merge(b)
    
    if Export then
      a:export("translated.obj")
    end
    
    a:free_mesh()
    b:free_mesh()
  end

  do
    print("should support rotation")

    local a = parshapes.create_cylinder(20, 3)
    local b = parshapes.create_cylinder(4, 3)
    local axis1 = { x = 0, y = 1, z = 0 }
    local axis2 = { x = 0, y = 0, z = 1 }

    a:rotate(math.pi * 0.5, axis1)
    a:rotate(math.pi * 0.25, axis2)
    a:merge(b)
    
    if Export then
      a:export("rotated.obj")
    end

    a:free_mesh()
    b:free_mesh()
  end
  
  do
    print("should support non-uniform scale")

    local a = parshapes.create_cylinder(15, 3)

    a:scale(1, 1, 5)

    if Export then
      a:export("uniform_scale.obj")
    end
  
    a:free_mesh()
  end
  
  do
    print("should support degenerate scale")

    local a = parshapes.create_cone(15, 3)

    assert(a)

    a:scale(1, 1, 0)

    for i = 1, a:get_point_count() do
      -- should not have nans
      local px, py, pz = a:get_point(i)
      local nx, ny, nz = a:get_normal(i)

      assert(px == px)
      assert(py == py)
      assert(pz == pz)
      assert(nx == nx)
      assert(ny == ny)
      assert(nz == nz)

      -- check components
      assert(nx == 0, nx)
      assert(ny == 0)
      assert(nz == 1)
      assert(pz == 0)
    end

    if Export then
      a:export("degenerate_scale.obj")
    end

    a:free_mesh()
  end
end

if DoMiscShapes then
  local Export = true

  do
    print("create an orientable disk in 3-space")
  
    local slices = 32
    local aradius = 1
    local anormal = { x = 0, y = 0, z = 1 }
    local acenter = { x = 0, y = 0, z = 0 }
    local a = parshapes.create_disk(aradius, slices, acenter, anormal)
    local bradius = 0.2
    local bcenter = { x = 0, y = 0, z = 0.2 }
    local bnormal = { x = 0, y = 1, z = 0 }
    local b = parshapes.create_disk(bradius, slices, bcenter, bnormal)

    a:merge(b)

    if Export then
      a:export("orientable_disk.obj")
    end

    a:free_mesh()
    b:free_mesh()
  end
  
  do
    print("create a rock on the Y plane")

    local slices = 32
    local radius = 1
    local normal = { x = 0, y = 1, z = 0 }
    local center = { x = 0, y = 0, z = 0 }
    local a = parshapes.create_disk(radius, slices, center, normal)
    local b = parshapes.create_rock(1, 2)
    local _, ymin = b:compute_aabb(b)

    b:translate(0, -ymin / 2, 0)
    a:merge(b)

    if Export then
      a:export("rock_on_y_plane.obj")
    end

    a:free_mesh()
    b:free_mesh()
  end
  
  do
    print("create a polyhedron on the Y plane")

    local slices = 32
    local radius = 1
    local normal = { x = 0, y = 1, z = 0 }
    local center = { x = 0, y = 0, z = 0 }
    local a = parshapes.create_disk(radius, slices, center, normal)
    local b = parshapes.create_dodecahedron()

    b:translate(0, 0.934, 0)
    a:merge(b)

    if Export then
      a:export("polyhedron_on_y_plane.obj")
    end

    a:free_mesh()
    b:free_mesh()
  end

  do
    print("create a rounded cylinder via composition")

    local O = { x = 0, y = 0, z = 0 }
    local I = { x = 1, y = 0, z = 0 }
    local J = { x = 0, y = 1, z = 0 }
    local K = { x = 0, y = 0, z = 1 }
    local top_center = { x = 0, y = 1.2, z = 0 }
    local tess = 30
    local a = parshapes.create_disk(2.5, tess, O, J)
    local b = parshapes.create_cylinder(tess, 3)
    local c = parshapes.create_torus(15, tess, 0.1)
    local d = parshapes.create_disk(1, tess, top_center, J)

    c:rotate(math.pi / tess, K)
    c:translate(0, 0, 1)
    b:scale(1.2, 1.2, 1)
    b:merge(c)
    b:rotate(-math.pi * 0.5, I)
    b:merge(d)
    b:merge(a)
    b:scale(1, 2, 1)

    if Export then
      b:export("rounded_cylinder_via_composition.obj")
    end

    a:free_mesh()
    b:free_mesh()
    c:free_mesh()
    d:free_mesh()
  end
end

if DoLsystem then
  local program = [[
    sx 2 sy 2
    ry 90 rx 90
    shape tube rx 15  call rlimb rx -15
    shape tube rx -15 call llimb rx 15
    shape tube ry 15  call rlimb ry -15
    shape tube ry 15  call llimb ry -15
    rule rlimb
        sx 0.925 sy 0.925 tz 1 rx 1.2
        call rlimb2
    rule rlimb2.1
        shape connect
        call rlimb
    rule rlimb2.1
        rx 15  shape tube call rlimb rx -15
        rx -15 shape tube call llimb rx 15
    rule rlimb.1
        call llimb
    rule llimb.1
        call rlimb
    rule llimb.10
        sx 0.925 sy 0.925
        tz 1
        rx -1.2
        shape connect
        call llimb
  ]]

  local O = { x = 0, y = 0, z = 0 }
  local J = { x = 0, y = 1, z = 0 }

  local mesh = parshapes.create_lsystem(program, 5, 60)
  local disk = parshapes.create_disk(10, 30, O, J)

  mesh:merge(disk)
  disk:free_mesh(disk)

  ExportAndFree(mesh, "lsystem")
end

if DoPlatonicSolids then
  local dodecahedron = parshapes.create_dodecahedron()
  
  dodecahedron:translate(0, 0.934, 0)

  local tetrahedron = parshapes.create_tetrahedron()

  tetrahedron:translate(1, 0, 3.5)

  local octahedron = parshapes.create_octahedron()

  octahedron:translate(-2.25, 0.9, -.5)

  local icosahedron = parshapes.create_icosahedron()

  icosahedron:translate(-1, 0.8, 3.5)

  local cube = parshapes.create_cube()

  cube:rotate(math.pi / 5.0, { x = 0, y = 1, z = 0 })
  cube:translate(1, 0, 0.5)
  cube:scale(1.2, 1.2, 1.2)

  local scene = parshapes.create_empty()

  scene:merge(dodecahedron)
  scene:merge(tetrahedron)
  scene:merge(octahedron)
  scene:merge(icosahedron)
  scene:merge(cube)
  scene:export("platonic_solids.obj")
end

if DoParametricSurfaces then
  local scene, shape = parshapes.create_empty()

  -- Tessellate an open-ended cylinder with 30 slices and 3 stacks.
  shape = parshapes.create_cylinder(30, 3)

  shape:rotate(-math.pi / 2.0, { x = 1, y = 0, z = 0 })
  shape:translate(0, 0, 3)
  scene:merge(shape)

  -- Create a disk-shaped cylinder cap with 30 slices.
  shape = parshapes.create_disk(1, 30, { x = 0, y = 1, z = 3}, { x = 0, y = 1, z = 0 })
  scene:merge(shape)

  -- Instantiate a dome shape.
  shape = parshapes.create_hemisphere(10, 10)

  shape:scale(0.2, 0.2, 0.2)
  shape:translate(0, 1, 3)
  scene:merge(shape)

  -- Create a rectangular backdrop.
  shape = parshapes.create_plane(3, 3)

  shape:translate(-0.5, 0, 1)
  shape:scale(4, 1.5, 1)
  scene:merge(shape)

  -- Place a submerged donut into the scene.
  shape = parshapes.create_torus(30, 40, 0.1)

  shape:scale(2, 2, 2)
  shape:translate(0, 0, 3)
  scene:merge(shape)

  scene:export("parametric_surfaces.obj")
end

--]=]

--[=[
local octasphere = require("plugin.octasphere")

local DoTiled = false
local DoRoundedCube = false

if DoTiled then
  print("should generate a tile-like shape")

  local config = {
    corner_radius = 0.1,
    width = 1.2, height = 1.2, depth = 0.3,
    num_subdivisions = 2
  }

  local indices_per_tile, vertices_per_tile = octasphere.get_counts(config)
  local mesh = octasphere.populate(config)

  mesh:export("tile_like.obj")
end

if DoRoundedCube then
  print("should generate rounded cube into glTF + bin files")

  local config = {
    corner_radius = 0.4,
    width = 1.2, height = 1.2, depth = 1.2,
    num_subdivisions = 3
  }

  local indices_per_tile, vertices_per_tile = octasphere.get_counts(config)
  local mesh = octasphere.populate(config)
  local pmin, pmax = { 99, 99, 99 }, { -99, -99, -99 }

  for i = 1, mesh:get_vertex_count() do
    local x, y, z = mesh:get_position(i)

    pmin[1] = math.min(pmin[1], x)
    pmin[2] = math.min(pmin[2], y)
    pmin[3] = math.min(pmin[3], z)
    pmax[1] = math.min(pmax[1], x)
    pmax[2] = math.min(pmax[2], y)
    pmax[3] = math.min(pmax[3], z)
  end

  -- ^^ used for gltf

  mesh:export("rounded_cube.obj")
end
--]=]

--[=[
local generator = require("plugin.generator")

local function Zero ()
  return { 0, 0, 0 }
end

local ColorComponent = { x = "r", y = "g", z = "b" }
local PointComponent  = { x = 1, y = 2, z = 3 }

local function generateAxis (svg, axis)
	local color = { r = 0, g = 0, b = 0 }

	color[ColorComponent[axis]] = 1.0

	local endp = Zero()
  
	endp[PointComponent[axis]] = 1.5

	local line = generator.createLinePath{ start = Zero(), ["end"] = endp, segments = 15 }
	local xx, prev = line:Vertices(), {}

	prev.x, prev.y, prev.z = xx:Generate():GetPosition()

	xx:Next()

  local current = {}

	while not xx:Done() do
		current.x, current.y, current.z = xx:Generate():GetPosition()

		svg:writeLine(prev, current, color)

		prev, current = current, prev

		xx:Next()
	end
end

local function WriteSVG (svg, name)
	local file = io.open(system.pathForFile(name, system.DocumentsDirectory), "wt")

	file:write(svg:getResult())
  file:close()
end

local function generateShape (shape, filename)
	local svg = generator.createSvgWriter(400, 400)

	svg:ortho(-1.5, 1.5, -1.5, 1.5)

	generateAxis(svg, "x");
	generateAxis(svg, "y");

	svg:writeShape(shape, { writeVertices = true, writeNormals = true })

	WriteSVG(svg, filename .. ".svg")
end

local function generatePath (path, filename)
	local svg = generator.createSvgWriter(800, 400)

	svg:perspective(1.0, 1.0, 0.1, 10.0)

	svg:viewport(0, 0, 400, 400)
	svg:modelView{
		{ type = "translate", x = -0.0, y = 0.0, z = -4.0 },
		{ type = "rotate", angles = { x = math.rad(45.0), y = 0.0, z = 0.0 } },
		{ type = "rotate", angles = { x = 0.0, y = math.rad(-45.0), z = 0.0 } }
	}

	generateAxis(svg, "x")
	generateAxis(svg, "y")
	generateAxis(svg, "z")

	svg:writePath(path, { writeVertices = true, writeNormals = true })

	svg:viewport(400, 0, 400, 400)
	svg:modelView{
		{ type = "translate", x = 0.0, y = 0.0, z = -4.0 },
		{ type = "rotate", angles = { x = math.rad(45.0), y = 0.0, z = 0.0 } },
		{ type = "rotate", angles = { x = 0.0, y = math.rad(-135.0), z = 0.0 } }
	}

	generateAxis(svg, "x")
	generateAxis(svg, "y")
	generateAxis(svg, "z")

	svg:writePath(path, { writeVertices = true, writeNormals = true })
	
	WriteSVG(svg, filename .. ".svg")
end


local function generateMesh (mesh, filename)
	local svg = generator.createSvgWriter(800, 400)

	svg:perspective(1.0, 1.0, 0.1, 10.0)

	svg:viewport(0, 0, 400, 400)
	svg:modelView{
		{ type = "translate", x = -0.0, y = 0.0, z = -4.0 },
		{ type = "rotate", angles = { x = math.rad(45.0), y = 0.0, z = 0.0 } },
		{ type = "rotate", angles = { x = 0.0, y = math.rad(-45.0), z = 0.0 } }
	}
  
	generateAxis(svg, "x")
	generateAxis(svg, "y")
	generateAxis(svg, "z")

	svg:writeMesh(mesh, { writeVertices = true, writeNormals = true })

	svg:viewport(400, 0, 400, 400)
	svg:modelView{
		{ type = "translate", x = 0.0, y = 0.0, z = -4.0 },
		{ type = "rotate", angles = { x = math.rad(45.0), y = 0.0, z = 0.0 } },
		{ type = "rotate", angles = { x = 0.0, y = math.rad(-135.0), z = 0.0 } }
	}
  
	generateAxis(svg, "x")
	generateAxis(svg, "y")
	generateAxis(svg, "z")

	svg:writeMesh(mesh, { writeVertices = true, writeNormals = true })

	WriteSVG(svg, filename .. ".svg")
end

-- Shapes
generateShape(
  generator.createBezierShape{
    { -1.0, -1.0 },
    { -0.5, 1.0 },
    { 0.5, -1.0 },
    { 1.0, 1.0 }
  },
  "BezierShape"
)

generateShape(generator.createCircleShape(), "CircleShape");
generateShape(generator.createEmptyShape(), "EmptyShape");
generateShape(generator.createLineShape(), "LineShape");
generateShape(generator.createRectangleShape(), "RectangleShape");
generateShape(generator.createRoundedRectangleShape(), "RoundedRectangleShape");
generateShape(generator.createGridShape(), "GridShape");

-- Paths
generatePath(generator.createEmptyPath(), "EmptyPath");
generatePath(generator.createHelixPath(), "HelixPath");
generatePath(generator.createKnotPath(), "KnotPath");
generatePath(generator.createLinePath(), "LinePath");

-- Meshes
local ctrlPoints = {
  {
    { -1.00, -1.00, 2.66 },
    { -0.33, -1.00, 0.66 },
    { 0.33, -1.00, -0.66 },
    { 1.0, -1.00, 1.33 }
  }, {
    { -1.00, -0.33, 0.66 },
    { 0.33, -0.33, 2.00 },
    { 0.33, -0.33, 0.00 },
    { 1.0, -0.33, -0.66 }
  }, {
    { -1.00, 0.33, 2.66 },
    { -0.33, 0.33, 0.00 },
    { 0.33, 0.33, 2.00 },
    { 1.0, 0.33, 2.66 }
  }, {
    { -1.00, 1.00, -1.33 },
    { -0.33, 1.00, -1.33 },
    { 0.33, 1.00, 0.00 },
    { 1.0, 1.00, -0.66 }
  }
}

generateMesh(generator.createBezierMesh{ control = ctrlPoints, segments = { 8, 8 } }, "BezierMesh")
generateMesh(generator.createBoxMesh(), "BoxMesh")
generateMesh(generator.createCappedCylinderMesh(), "CappedCylinderMesh")
generateMesh(generator.createCappedConeMesh(), "CappedConeMesh")
generateMesh(generator.createCappedTubeMesh(), "CappedTubeMesh")
generateMesh(generator.createConeMesh(), "ConeMesh")
generateMesh(generator.createCapsuleMesh(), "CapsuleMesh")
generateMesh(generator.createConvexPolygonMesh(), "ConvexPolygonMesh")
generateMesh(generator.createCylinderMesh(), "CylinderMesh")
generateMesh(generator.createDodecahedronMesh(), "DodecahedronMesh")
generateMesh(generator.createDiskMesh(), "DiskMesh")
generateMesh(generator.createEmptyMesh(), "EmptyMesh")
generateMesh(generator.createIcosahedronMesh(), "IcosahedronMesh")
generateMesh(generator.createIcoSphereMesh(), "IcoSphereMesh")
generateMesh(generator.createPlaneMesh(), "PlaneMesh")
generateMesh(generator.createRoundedBoxMesh(), "RoundedBoxMesh")
generateMesh(generator.createSphereMesh(), "SphereMesh")
generateMesh(generator.createSphericalConeMesh(), "SphericalConeMesh")
generateMesh(generator.createSphericalTriangleMesh(), "SphericalTriangleMesh")
generateMesh(generator.createSpringMesh(), "SpringMesh")
generateMesh(generator.createTeapotMesh():Scale{ x = 0.5, y = 0.5, z = 0.5 }, "TeapotMesh")
generateMesh(generator.createTorusKnotMesh(), "TorusKnotMesh")
generateMesh(generator.createTorusMesh(), "TorusMesh")
generateMesh(generator.createTriangleMesh(), "TriangleMesh")
generateMesh(generator.createTubeMesh(), "TubeMesh")

-- Group picture

local svg = generator.createSvgWriter(1000, 400)
svg:perspective(1.0, 1.0, 0.1, 10.0)
svg:modelView{
  { type = "translate", x = -0.0, y = 0.0, z = -4.0 },
  { type = "rotate", angles = { x = math.rad(45.0), y = 0.0, z = 0.0 } },
  { type = "rotate", angles = { x = 0.0, y = math.rad(-45.0), z = 0.0 } }
}


svg:viewport(0, 200, 200, 200)
svg:writeMesh(generator.createSphereMesh())

svg:viewport(200, 200, 200, 200)
svg:writeMesh(generator.createTorusMesh())

svg:viewport(400, 200, 200, 200)
svg:writeMesh(generator.createRoundedBoxMesh())

svg:viewport(600, 200, 200, 200)
svg:writeMesh(generator.createCappedTubeMesh())

svg:viewport(800, 200, 200, 200)
svg:writeMesh(generator.createTorusKnotMesh())

svg:viewport(0, 0, 200, 200)
svg:writeMesh(generator.createIcoSphereMesh())

svg:viewport(200, 0, 200, 200)
svg:writeMesh(generator.createCappedCylinderMesh())

svg:viewport(400, 0, 200, 200)
svg:writeMesh(generator.createCapsuleMesh())

svg:viewport(600, 0, 200, 200)
svg:writeMesh(generator.createSpringMesh())

svg:viewport(800, 0, 200, 200)
svg:writeMesh(generator.createCappedConeMesh())

WriteSVG(svg, "GroupPicture.svg")

--]=]
--[=[
-- Test for memory feature, consuming light userdata encodings:
local bb = Bytemap.newTexture{ is_non_external = true, width = 1, height = 1 }
local slot = _G.SLOT

print("SLOT?", slot) -- confirm match

for i = 0, 15 do
  local encoding = _G["CONTEXT" .. i] -- get light userdata...

  bb:SetBytes(encoding) -- ...assign it...

  local a, b, c, d = bb:GetBytes{ x1 = 1, y1 = 1, x2 = 1, y2 = 1 }:byte(1, 4) -- ...and read it back

  print("Bytes?", ("%s = %x, %x, %x, %x"):format("CONTEXT" .. i, a, b, c, d)) -- will show buffer bytes (in reverse)
end
--[[
Sample run on Windows:

* From Bytemap, providing memory:

11:34:32.814  Buffer element 0 = 5f323bf6
11:34:32.814  Buffer element 1 = 3a9e797d
11:34:32.814  Buffer element 2 = 5f490ddc
11:34:32.814  Buffer element 3 = 4cad314f
11:34:32.814  Buffer element 4 = 5e144df2
11:34:32.814  Buffer element 5 = 49442e40
11:34:32.814  Buffer element 6 = 13661cd0
11:34:32.814  Buffer element 7 = 366b66c4
11:34:32.814  Buffer element 8 = 42307eb7
11:34:32.814  Buffer element 9 = 60322c3b
11:34:32.814  Buffer element 10 = 15a15422
11:34:32.814  Buffer element 11 = 3ef60822
11:34:32.814  Buffer element 12 = 5991409d
11:34:32.814  Buffer element 13 = 12e1798b
11:34:32.814  Buffer element 14 = 121f73da
11:34:32.814  Buffer element 15 = 58b026ca
11:34:32.814  Slot bound = 0

* Above code, consuming it:

11:34:32.982  SLOT?	0
11:34:32.982  Bytes?	CONTEXT0 = f6, 3b, 32, 5f
11:34:32.982  Bytes?	CONTEXT1 = 7d, 79, 9e, 3a
11:34:32.982  Bytes?	CONTEXT2 = dc, d, 49, 5f
11:34:32.982  Bytes?	CONTEXT3 = 4f, 31, ad, 4c
11:34:32.982  Bytes?	CONTEXT4 = f2, 4d, 14, 5e
11:34:32.982  Bytes?	CONTEXT5 = 40, 2e, 44, 49
11:34:32.982  Bytes?	CONTEXT6 = d0, 1c, 66, 13
11:34:32.982  Bytes?	CONTEXT7 = c4, 66, 6b, 36
11:34:32.982  Bytes?	CONTEXT8 = b7, 7e, 30, 42
11:34:32.982  Bytes?	CONTEXT9 = 3b, 2c, 32, 60
11:34:32.982  Bytes?	CONTEXT10 = 22, 54, a1, 15
11:34:32.982  Bytes?	CONTEXT11 = 22, 8, f6, 3e
11:34:32.982  Bytes?	CONTEXT12 = 9d, 40, 91, 59
11:34:32.982  Bytes?	CONTEXT13 = 8b, 79, e1, 12
11:34:32.982  Bytes?	CONTEXT14 = da, 73, 1f, 12
11:34:32.982  Bytes?	CONTEXT15 = ca, 26, b0, 58
]]
--]=]
---[=[
local yoctoshapes = require("plugin.yoctoshapes")

local model2 = tinyrenderer.NewModel()
local object2 = tinyrenderer.NewObject(model2)

model2:SetDiffuse(texture)

for _, v in ipairs{
--  yoctoshapes.make_rounded_box()
--  yoctoshapes.make_sphere(),
--  yoctoshapes.make_bulged_disk(),
--  yoctoshapes.make_geosphere(),
--  yoctoshapes.make_quad(),
--  yoctoshapes.make_quady(),
--  yoctoshapes.make_rounded_uvcylinder(),
--  yoctoshapes.make_lines{ steps = { 4, 32 }, radius = { .025, .015 } }
} do
  -- v = v:make_hair{ steps = { 100, 100 }, radius = { .025, .015 } }

  local positions, normals, texcoords = v:get_positions(), v:get_normals(), v:get_texcoords()
  local lines, quads, triangles = v:get_lines(), v:get_quads(), v:get_triangles()

  if #normals > 0 then
    if #lines > 0 then
      print("L1",#positions)
      v = lines:lines_to_cylinders{ positions = positions }
      positions, normals, texcoords = v:get_positions(), v:get_normals(), v:get_texcoords()
      triangles = v:get_quads():quads_to_triangles()
      print("L2",#positions,#triangles)
    elseif #quads > 0 then
      triangles = quads:quads_to_triangles()
      print("Q?", #quads, "->", #triangles)
    end

    if #triangles > 0 then
      for i = 1, #positions do
        model2:AddVertex(positions:get(i))
        model2:AddNormal(normals:get(i))
      end

      if #texcoords > 0 then
        for i = 1, #texcoords do
          model2:AddUV(texcoords:get(i))
        end
      else
        for i = 1, #positions do
          model2:AddUV(math.random(), math.random())
        end
      end

      for i = 1, #triangles do
        model2:AddFace(triangles:get_triangle(i))
      end
    end
  end
end

object2:SetScale(.75, .75, .75)

local axis = { math.random(), math.random(), math.random() }
local len = math.sqrt(axis[1]^2 + axis[2]^2 + axis[3]^2)

axis[1], axis[2], axis[3] = axis[1] / len, axis[2] / len, axis[3] / len

timer.performWithDelay(450, function()
  local angle = math.random() * 2 * math.pi
  local ca, sa = math.cos(angle), math.sin(angle)

  object2:SetRotation(sa * axis[1], sa * axis[2], sa * axis[3], ca)
end, 0)

root:Insert(object2)
--]=]