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

-- Plugins --
local Bytemap = require("plugin.Bytemap")
local MemoryBlob = require("plugin.MemoryBlob") -- installs non-dummy blob logic
-- local moonassimp = require("plugin.moonassimp")
local sceneView3D = require("plugin.sceneView3D")

--
--
--

local ssge = sceneView3D.SSGE.NewEngine{ width = 500, height = 500, using_blob = true }

print("!!",ssge:switchScene(system.pathForFile("scenes/teapotMultiMaterial")))

local bm = Bytemap.newTexture{ width = 500, height = 500 }
local image = display.newImageRect(bm.filename, bm.baseDir, 400, 400)

image.x, image.y = display.contentCenterX, display.contentCenterY

bm:BindBlob(ssge:getBlob())

--
--
--

ssge:run(0, 0)

local was = system.getTimer()

timer.performWithDelay(30, function(event)
	local now = event.time

	ssge:run(now, now - was)
	bm:invalidate()

	was = now
end, 0)

--
--
--

Runtime:addEventListener("key", function(event)
	if event.phase == "down" then
		local speed = ssge:getCameraSpeed()

		if event.keyName == "w" then
			if ssge:isCameraOrbiting() then
				ssge:updateCameraRadius(speed)
			else
				ssge:moveCameraFront(speed)
			end
		elseif event.keyName == "s" then
			if ssge:isCameraOrbiting() then
				ssge:updateCameraRadius(-speed)
			else
				ssge:moveCameraFront(-speed)
			end
		elseif event.keyName == "a" or event.keyName == "d" then
			local delta = event.keyName == "a"  and -speed or speed
			
			ssge:moveCameraSide(delta)
		elseif event.keyName == "q" or event.keyName == "e" then
			local delta = event.keyName == "e"  and -speed or speed
			
			ssge:moveCameraUp(delta)
		elseif event.keyName == "r" then
			ssge:resetCamera()
		elseif event.keyName == "tab" then
			ssge:setCameraOrbiting(not ssge:isCameraOrbiting())
		elseif event.keyName == "up" or event.keyName == "down" then
			local delta, period = event.keyName == "up" and -2 or 2, ssge:getCameraPeriod()

			period = period + delta
			
			if period >= 4 and period <= 60 then
				ssge:setCameraPeriod(period)
			end
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
        if event.scrollY > 0 then -- scroll up
            fov = -5
        elseif event.scrollY < 0 then
            fov = 5
		else
			return
        end
--[[
        //Limiting the FOV range to avoid low FPS values or weird distortion
        if(fov < 20){
            fov = 20;
        }
        else if (fov > 120){
            fov = 120;
        }

        //Updating the camera frustrum
        sceneCamera->cameraFrustrum.fov = fov;]]
		ssge:updateCameraFOV(fov)
	elseif event.type == "drag" and OldX then
		local dx, dy = event.x - OldX, event.y - OldY
		
		ssge:moveCamera(dx * .35, dy * .35)
		
		OldX, OldY = event.x, event.y
	end
end)

