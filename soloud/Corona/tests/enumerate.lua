--- Port of "enumerate" demo.

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
local wrap = coroutine.wrap
local yield = coroutine.yield

-- Modules --
local utils = require("utils")

-- Plugins --
local soloud = require("plugin.soloud")

-- Solar2D globals --
local timer = timer

--
--
--

utils.Begin("Visualization", 50, 50)

  --
  --
  --

  utils.Begin("scrollview", 600, "hide")

  local pad = utils.Text((" "):rep(35))
  local sv = utils.End()

  --
  --
  --

utils.End()

pad:removeSelf()

--
--
--

local function GetChannelString (channels)
  if channels == 8 then
		return " (7.1 surround)"
	elseif channels == 6 then
		return " (5.1 surround)"
	elseif channels == 4 then
		return " (quad)"
	elseif channels == 2 then
		return " (stereo)"
	elseif channels == 1 then
		return " (mono)"
	else
    return " (?!)"
  end
end

--
--
--

local backends = {
	"AUTO",
	"SDL1",
	"SDL2",
	"PORTAUDIO",
	"WINMM",
	"XAUDIO2",
	"WASAPI",
	"ALSA",
	"JACK",
	"OSS",
	"OPENAL",
	"COREAUDIO",
	"OPENSLES",
	"VITA_HOMEBREW",
	"NULLDRIVER",
	"NOSOUND",
	"MINIAUDIO"
}

--
--
--

local function EnumerateBackends ()
  utils.Begin("existing", sv)

  for i, name in ipairs(backends) do
    utils.Text("-----")
    utils.Text(("Backend %d:%s"):format(i, name))

    local core, err = soloud.createCore{ backend = name }

    if core then
      utils.Text(
        (
          "ID:       %d\n" ..
          "String:   '%s'\n" ..
          "Rate:     %d\n" ..
          "Buffer:   %d\n" ..
          "Channels: %d%s (default)\n"
        ):format(
          core:getBackendId(),
          core:getBackendString() or "(null)",
          core:getBackendSamplerate(),
          core:getBackendBufferSize(),
          core:getBackendChannels(),
          GetChannelString(core:getBackendChannels())
        )
      )

      core:destroy()

      for j = 1, 11 do
        local gSoloud = soloud.createCore{ backend = name, channels = j }

        if gSoloud then
          if gSoloud:getBackendChannels() == j then
            utils.Text(
              (
                "Channels: %d%s\n"
              ):format(
                gSoloud:getBackendChannels(), GetChannelString(gSoloud:getBackendChannels())
              )
            )
          end

          gSoloud:destroy()
        end

        yield()
      end
    else
      utils.Text(("Failed: %s\n"):format(err))
    end
  end

  utils.End()
end

--
--
--

timer.performWithDelay(30, wrap(function(event) -- load gradually, to avoid hitches
  local source = event.source -- can get clobbered

  EnumerateBackends()

  timer.cancel(source)
end), 0)