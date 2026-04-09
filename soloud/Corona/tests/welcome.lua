--- Port of "welcome" demo.

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
local soloud = require("plugin.soloud")

-- Solar2D globals --
local display = display
local native = native
local Runtime = Runtime
local timer = timer

--
--
--

local gSoloud = soloud.createCore()
local speech = soloud.createSpeech()
local wav = soloud.createWav()
local mod = soloud.createOpenmpt()

--
--
--

local welcome = display.newText("Welcome to Soloud!", 0, 100, native.systemFontBold, 30)

welcome.anchorX, welcome.x = 0, 50

--
--
--

wav:load("audio/windy_ambience.ogg")
wav:setLooping(true)

local handle1 = gSoloud:play(wav)

gSoloud:setVolume(handle1, 0.5)
gSoloud:setPan(handle1, -0.2)
gSoloud:setRelativePlaySpeed(handle1, 0.9)

--
--
--

local prompt = display.newText("What is your name?", 0, 200, native.systemFontBold, 30)

prompt.anchorX, prompt.x = 0, 50

local DoTheRest

local box = native.newTextField( 400, 350, 680, 100 )

box.isEditable = true

box:addEventListener( "userInput", function(event)
  if event.phase == "submitted" then
    speech:setText(event.target.text)
    gSoloud:play(speech)

    box:removeSelf()

    timer.performWithDelay(50, function(event)
      if gSoloud:getVoiceCount() == 1 then
        timer.cancel(event.source)

        DoTheRest()
      end
    end, 0)
  end
end)

--
--
--

function DoTheRest()
  gSoloud:stop(handle1)

  local loaded = mod:load("audio/BRUCE.S3M")
  local comment = display.newText("", 0, 500, native.systemFontBold, 30)

  comment.anchorX, comment.x = 0, 50

  if loaded then
    gSoloud:play(mod)
  
    comment.text = "Playing music. Press a key to quit.."

    Runtime:addEventListener("key", function(event)
      if event.phase == "down" and gSoloud:getVoiceCount() > 0 then
        comment:removeSelf()
        gSoloud:stopAll()
      end
    end)
  else 
    comment.text = "Cannot find audio/BRUCE.S3M (or libopenmpt may be missing)"
  end
end