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

local co4 = require("plugin.customobjects4") -- probably fine, actually...
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
--display.setDrawMode("wireframe")

-- Get instanacing support.
local instancing = system.getInfo("instancingSupport")

if instancing then
  print("INSTANCING:")

  for k, v in pairs(instancing) do
    print(k, v)
  end
else
  print("INSTANCING UNSUPPORTED")
end

--------------------------
--
-- Weak support spoofing.
--
--------------------------

-- N.B. if instancing is supported, at least one of these is assumed to be true:
  -- vertexReplication ~= "none"
  -- hasInstanceID is true
-- Uncomment with this in mind!

if instancing then
  if instancing.vertexReplication ~= "none" then -- at least single instance?
    -- Uncomment this to test NO multi-instance replication:
    --[[
      instancing.vertexReplication = "singleInstance"
    --]]
    
    -- Uncomment to test NO instance ID:
    --[[
      instancing.hasInstanceID = false
    --]]
  end

  if instancing.hasInstanceID then
    -- Uncomment this to test NO replication support:
    --[[
      instancing.vertexReplication = "none"
    --]]
  end

  -- Uncomment to test NO support:
  --[[
    instancing = false
  --]]
end

---------------------
--
-- Sections to test.
--
---------------------

local WantToTestVec2s = true
local WantsToTestColorMix = true
local WantsToTestObjectWithNormals = true
local WantsToTestNormalsOnly = true

------------------------------------------------------------------------
--
-- Test some vec2 attributes (basic 2D tests, instancing if available).
--
------------------------------------------------------------------------

if WantToTestVec2s then
  
  local VertexKernelHasReplicationAndID = [[
    P_POSITION varying vec2 v_SecondVec2;
    P_POSITION varying float v_ID;

    P_POSITION vec2 VertexKernel (P_POSITION vec2 pos)
    {
      pos += CoronaFirstVec2;
      pos += CoronaShift;

      v_SecondVec2 = CoronaSecondVec2;
      v_ID = CoronaInstanceID;

      return pos;
    }
  ]]

  local VertexKernelHasReplication = [[
    P_POSITION varying vec2 v_SecondVec2;
    P_POSITION varying float v_ID;

    P_POSITION vec2 VertexKernel (P_POSITION vec2 pos)
    {
      pos += CoronaFirstVec2;
      pos += CoronaShift;

      v_SecondVec2 = CoronaSecondVec2;
      v_ID = 0.;

      return pos;
    }
  ]]

  local VertexKernelHasID = [[
    P_POSITION varying vec2 v_SecondVec2;
    P_POSITION varying float v_ID;

    P_POSITION vec2 VertexKernel (P_POSITION vec2 pos)
    {
      pos += CoronaFirstVec2;
      pos.x += CoronaInstanceID * 100.;

      v_SecondVec2 = CoronaSecondVec2;
      v_ID = CoronaInstanceID;

      return pos;
    }
  ]]

  local VertexKernelBasic = [[
    P_POSITION varying vec2 v_SecondVec2;
    P_POSITION varying float v_ID;

    P_POSITION vec2 VertexKernel (P_POSITION vec2 pos)
    {
      pos += CoronaFirstVec2;

      v_SecondVec2 = CoronaSecondVec2;
      v_ID = 0.;

      return pos;
    }
  ]]

  local VertexKernel, Shift, InstanceByID

  if instancing and instancing.vertexReplication ~= "none" then
    if instancing.hasInstanceID then
      VertexKernel = VertexKernelHasReplicationAndID
    else
      VertexKernel = VertexKernelHasReplication
    end
    
    Shift = { name = "shift", type = "float", componentCount = 2, instancesToReplicate = 1 }
  elseif instancing and instancing.hasInstanceID then
    VertexKernel = VertexKernelHasID
    InstanceByID = true
  else
    VertexKernel = VertexKernelBasic
  end

  graphics.defineVertexExtension{
    name = "Vec2s",

    instanceByID = InstanceByID,

    { name = "firstVec2", type = "float", componentCount = 2 },
    { name = "secondVec2", type = "float", componentCount = 2 },

    -- conditional array bits...
    Shift
  }

  graphics.defineEffect{
    category = "filter", name = "testVec2s", vertexExtension = "Vec2s",

    vertex = VertexKernel,

    fragment = [[
      P_POSITION varying vec2 v_SecondVec2;
      P_POSITION varying float v_ID;

      P_COLOR vec4 FragmentKernel (P_UV vec2 uv)
      {
        P_COLOR vec4 color = texture2D(CoronaSampler0, uv);
      
        color.rg *= v_SecondVec2;
        color.b = .325 + v_ID * (.675 / 4.);

        return CoronaColorScale(color);
      }
    ]]
  }

  local rect = display.newRect(display.contentCenterX, display.contentCenterY, 300, 300)

  rect.fillExtension = "Vec2s"

  rect.fillExtendedData:setAttributeValue(1, "firstVec2", -8, 0)
  rect.fillExtendedData:setAttributeValue(2, "firstVec2", 25, 33)
  -- rect.fillExtendedData:setAttributeValue(3, "firstVec2", 0, 0)
  rect.fillExtendedData:setAttributeValue(4, "firstVec2", -50, -14)

  rect.fillExtendedData:setAttributeValue(1, "secondVec2", 0, .7)
  -- rect.fillExtendedData:setAttributeValue(2, "secondVec2", 1, 1)
  -- rect.fillExtendedData:setAttributeValue(3, "secondVec2", 1, 1)
  rect.fillExtendedData:setAttributeValue(4, "secondVec2", 1, .8)

  rect.fill.effect = "filter.custom.testVec2s"

  if instancing then
    rect.fillExtendedData.instances = 5

    if Shift then -- have shift attribute?
      local shiftDetails = rect.fillExtendedData:getAttributeDetails("shift")

      print("DETAILS (shift):")

      for k, v in pairs(shiftDetails) do
        print(k, v)
      end

      -- Gradually shift the distorted rects. This will look like new ones are appearing, but
      -- in reality they all initially overlap and separate out. The last one will also move,
      -- so it might look like one is getting removed (see note for last item).
      local Shifts = {
        { x = 100, y = 200 },
        { x = 50, y = 75 },
        { x = 300, y = 150 },
        { x = 100, y = 200 },
        { x = 50, y = 150 } -- comment out to leave last instance in place
      }

      timer.performWithDelay(3000, function(event)
          local shift = Shifts[event.count]
          
          rect.fillExtendedData:setAttributeValue(event.count, "shift", shift.x, shift.y)
      end, #Shifts)
    end
  end

end


--------------------------------------------------------------------------------
--
-- Test some color-mix attributes (window, plus multi-instancing if available).
--
--------------------------------------------------------------------------------

if WantsToTestColorMix then
  repeat -- allow early-out if instancing support isn't available

  if not (instancing and instancing.vertexReplication ~= "none") then
    print("Window requires vertex replication")
    
    break -- out of repeat-until true
  end

  local VertexKernel, MultiInstance

  if instancing.vertexReplication == "multiInstance" then
    -- two rects at one y-offset, the third at another:
    MultiInstance = { name = "yOffset", type = "float", instancesToReplicate = 2 }
    
    VertexKernel = [[
      P_POSITION varying vec3 v_RGB;

      P_POSITION vec2 VertexKernel (P_POSITION vec2 pos)
      {
        pos.x += CoronaXOffset;
        pos.y += CoronaYOffset;

        v_RGB = CoronaRgb1 + CoronaRgb2 + CoronaRgb3;

        return pos;
      }
    ]]
  else
    VertexKernel = [[
      P_POSITION varying vec3 v_RGB;

      P_POSITION vec2 VertexKernel (P_POSITION vec2 pos)
      {
        pos.x += CoronaXOffset;

        v_RGB = CoronaRgb1 + CoronaRgb2 + CoronaRgb3;

        return pos;
      }
    ]]
  end

  graphics.defineVertexExtension{
    name = "ColorMix",

    { name = "rgb", type = "float", componentCount = 3, windowSize = 3 },
    { name = "xOffset", type = "float", componentCount = 1, instancesToReplicate = 1 },
    
    -- conditional array bits...
    MultiInstance
  }

  -- CPU-side test of window members.
  graphics.defineEffect{
    category = "filter", name = "testWindow", vertexExtension = "ColorMix",

    vertex = VertexKernel,

    fragment = [[
      P_POSITION varying vec3 v_RGB;

      P_COLOR vec4 FragmentKernel (P_UV vec2 uv)
      {
        return CoronaColorScale(vec4(v_RGB, 1.));
      }
    ]]
  }

  local rect = display.newRect(display.contentCenterX, display.contentCenterY - 150, 50, 50)

  rect.fillExtension = "ColorMix"

  rect.fillExtendedData.instances = 3 -- n = 3, + window size - 1 = 5

  rect.fillExtendedData:setAttributeValue(1, "rgb1", 1, 0, .5)
  rect.fillExtendedData:setAttributeValue(2, "rgb1", 0, 0, 0)
  rect.fillExtendedData:setAttributeValue(3, "rgb1", 0, .5, 0) -- red + half-green + half-blue
  rect.fillExtendedData:setAttributeValue(4, "rgb1", .5, 0, 0) -- half-red + half-green
  rect.fillExtendedData:setAttributeValue(5, "rgb1", 0, .5, 0) -- green + half-red

  rect.fillExtendedData:setAttributeValue(1, "xOffset", 0)
  rect.fillExtendedData:setAttributeValue(2, "xOffset", 90)
  rect.fillExtendedData:setAttributeValue(3, "xOffset", 180)

  if MultiInstance then
      rect.fillExtendedData:setAttributeValue(1, "yOffset", 0)
      rect.fillExtendedData:setAttributeValue(2, "yOffset", 50)
  end

  rect.fill.effect = "filter.custom.testWindow"

  -- for comparison:
  local r1 = display.newCircle(display.contentCenterX, display.contentCenterY - 225, 15)
  local r2 = display.newCircle(display.contentCenterX + 90, display.contentCenterY - 225, 15)
  local r3 = display.newCircle(display.contentCenterX + 180, display.contentCenterY - 225, 15)

  r1:setFillColor(1, .5, .5)
  r2:setFillColor(.5, .5, 0)
  r3:setFillColor(.5, 1, 0)

  until true -- see note above about early-out
end


----------------------------------------------------------------------
--
-- Test some 3D objects with normals (3D tests with GPU-side object).
--
----------------------------------------------------------------------

if WantsToTestObjectWithNormals or WantsToTestNormalsOnly then

  -- Enable 3D with normals and use our own matrices as transforms.
  graphics.defineShellTransform{
    name = "3DWithNormals",

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

  local XOffset, InstanceByID

  if instancing and instancing.vertexReplication ~= "none" then
    XOffset = { name = "xOffset", type = "float", instanced = true }
  elseif instancing and instancing.hasInstanceID then
    InstanceByID = true
  end

  graphics.defineVertexExtension{
    name = "Normals",

    instanceByID = InstanceByID,

    { name = "unused", type = "byte", normalized = true }, -- for testing
    { name = "normal", type = "float" --[[ or "byte" / normalized... ]], componentCount = 3 },

    -- conditional array bits...
    XOffset
  }

  -- Various depth states enabled, including some obvious defaults.
  local g = co4.newScopeGroupObject()
  local depth_state = co4.newDepthStateObject(g)

  depth_state.enabled = true
  depth_state.cullFaceEnabled = true

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

  -- Create a mesh from a model file.
  local normals, vertices, provisionalNormals, provisionalUVs, indices = {}, {}, {}, {}, {}
  local vi_to_uvi, vi_to_ni = {}, {}
  local handlers = {
    check_function = function(check) end,

    vertex = function(x, y, z)
      vertices[#vertices + 1] = x * 100
      vertices[#vertices + 1] = y * 100
      vertices[#vertices + 1] = z * 100
    end,

    normal = function(x, y, z)
      provisionalNormals[#provisionalNormals + 1] = x
      provisionalNormals[#provisionalNormals + 1] = y
      provisionalNormals[#provisionalNormals + 1] = z
    end,

    texcoord = function(u, v)
      provisionalUVs[#provisionalUVs + 1] = u
      provisionalUVs[#provisionalUVs + 1] = v
    end,

    start_face = function() end,

    face_vtn = function(v, t, n)
  vi_to_uvi[v] = t
  vi_to_ni[v] = n
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

  for vi, ni in pairs(vi_to_ni) do -- ditto...
    local vo, no = 3 * (vi - 1), 3 * (ni - 1)

    normals[vo + 1] = provisionalNormals[no + 1]
    normals[vo + 2] = provisionalNormals[no + 2]
    normals[vo + 3] = provisionalNormals[no + 3]
  end

  local printedNormalDetails

  local function MakeMesh (params)
      display.setDefault("skipsCulling", true)
      display.setDefault("skipsHitTesting", true)

      local mesh = display.newMesh(params)
      
      display.setDefault("skipsCulling", false)
      display.setDefault("skipsHitTesting", false)
      
      if mesh then
        mesh.fill = { type = "image", filename = Name .. ".jpg" }
        mesh.fillExtension = "Normals"
        
        if not printedNormalDetails then
          local normalDetails = mesh.fillExtendedData:getAttributeDetails("normal")

          print("DETAILS (normal):")

          for k, v in pairs(normalDetails) do
            print(k, v)
          end
          
          printedNormalDetails = true
        end
        
        for i = 3, #normals, 3 do
          mesh.fillExtendedData:setAttributeValue(i / 3, "normal", normals[i - 2], normals[i - 1], normals[i])
        end
      end
      
      return mesh
  end

  local params = { yaw = 0 }
  local model = co4.newMatrix()

  model:populateIdentity()

  local arr = model:getAsArray()

  -- Set initial camera matrices for objects.
  local pv = co4.newMatrix() -- projection * view

  pv:setProduct(projection, view)

  ------------------------------------------------------
  --
  -- Basic normals test (also instancing if available).
  --
  ------------------------------------------------------

  if WantsToTestObjectWithNormals then
    local mesh = MakeMesh{
      parent = g,
      x = 30,
      y = 10,
      mode = "indexed",
      vertices = vertices,
      uvs = uvs,
      indices = indices,
      hasZ = true
    }

    local VertexKernelHasReplication = [[
        P_POSITION varying vec3 v_Normal;

        P_POSITION vec3 VertexKernel (P_POSITION vec3 pos)
        {
          v_Normal = CoronaNormal;

          pos.x += CoronaXOffset;

          return pos;
        }
      ]]

    local VertexKernelHasID = [[
      P_POSITION varying vec3 v_Normal;

      P_POSITION vec3 VertexKernel (P_POSITION vec3 pos)
      {
        v_Normal = CoronaNormal;

        pos.x += CoronaInstanceID * 125.;
        pos.y += CoronaInstanceID * 30.;

        return pos;
      }
    ]]

    local VertexKernelBasic = [[
      P_POSITION varying vec3 v_Normal;

      P_POSITION vec3 VertexKernel (P_POSITION vec3 pos)
      {
        v_Normal = CoronaNormal;

        return pos;
      }
    ]]

    local VertexKernel

    if XOffset then
      VertexKernel = VertexKernelHasReplication
    elseif InstanceByID then
      VertexKernel = VertexKernelHasID
    else
      VertexKernel = VertexKernelBasic
    end

    -- Normal-based shading effect.
    graphics.defineEffect{
      category = "filter", name = "test", shellTransform = "3DWithNormals", vertexExtension = "Normals",

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

      vertex = VertexKernel,

      fragment = [[
        P_POSITION varying vec3 v_Normal;

        P_COLOR vec4 FragmentKernel (P_UV vec2 uv)
        {
          P_COLOR vec4 color = texture2D(CoronaSampler0, uv);
        
          P_POSITION const vec3 up = normalize(vec3(.7, 1., .3));
        
          color.rgb *= max(0., dot(up, v_Normal));
        
          return CoronaColorScale(color);
        }
      ]]
    }
  
    if instancing then
      mesh.fillExtendedData.instances = 3

      if XOffset then
        mesh.fillExtendedData:setAttributeValue(2, "xOffset", 100)
        mesh.fillExtendedData:setAttributeValue(3, "xOffset", 250)
      end
    end

    mesh.fill.effect = "filter.custom.test"
    mesh.fill.effect.model = arr
    mesh.fill.effect.view_projection = pv:getAsArray()

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

      params.yaw = params.yaw + 70 * dt

      model:populateFromEulerAngles(params)

      local arr = model:getAsArray()
      
      mesh.fill.effect.model = arr
    end, 0)

    -- Update camera matrices over time.
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

      local arr =  pv:getAsArray()

      mesh.fill.effect.view_projection = arr
    end, 0)    
    
  end

  -----------------------------------------------------------------------------
  --
  -- Compatible shader test (shader uses subset of extra geometry attributes).
  --
  -----------------------------------------------------------------------------

  if WantsToTestNormalsOnly then
    
    local mesh = MakeMesh{
      parent = g,
      x = 30,
      y = -20,
      mode = "indexed",
      vertices = vertices,
      uvs = uvs,
      indices = indices,
      hasZ = true
    }

    graphics.defineVertexExtension{
      name = "NormalsOnly",

      { name = "normal", type = "float", componentCount = 3 }
    }

    -- Normals-as-color effect.
    graphics.defineEffect{
      category = "filter", name = "normalsOnlyTest", shellTransform = "3DWithNormals", vertexExtension = "NormalsOnly",

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

      vertex = [[
        P_POSITION varying vec3 v_Normal;

        P_POSITION vec3 VertexKernel (P_POSITION vec3 pos)
        {
          v_Normal = CoronaNormal;

          return pos * .4;
        }
      ]],

      fragment = [[
        P_POSITION varying vec3 v_Normal;

        P_COLOR vec4 FragmentKernel (P_UV vec2 uv)
        {
          return CoronaColorScale(vec4(.5 * v_Normal + .5, 1.));
        }
      ]]
    }
    
    mesh.fill.effect = "filter.custom.normalsOnlyTest"
    mesh.fill.effect.model = arr
    mesh.fill.effect.view_projection = pv:getAsArray()

  end

end