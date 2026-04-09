--- Some large code patterns found in more than one demo.

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
local pairs = pairs
local random = math.random

-- Modules --
local utils = require("utils")
local soloud = require("plugin.soloud")

-- Solar2D globals --
local display = display
local timer = timer

-- Exports --
local M = {}

--
--
--

function M.CreateFilterList ()
  return {
    soloud.createBassboostFilter(),
    soloud.createBiquadResonantFilter(),
    soloud.createDCRemovalFilter(),
    soloud.createEchoFilter(),
    soloud.createFFTFilter(),
    soloud.createFlangerFilter(),
    soloud.createFreeverbFilter(),
    soloud.createLofiFilter(),
    soloud.createRobotizeFilter(),
    soloud.createWaveShaperFilter(),
    soloud.createEqFilter()
  }
end

--
--
--

function M.AddFilterSelection (core, filter_list, x0, y0)
  utils.Begin("Filters", x0, y0)

    utils.Begin("subgroup")

    local filter_lists = {}

    for i = 1, 4 do
      utils.RadioButton{
        label = "Filter " .. i,
        action = function(on)
          filter_lists[i].isVisible = on
        end,
        enabled = i == 1
      }
    end

    utils.End()
  
  utils.NewLine()

  --
  --
  --

  local function SetPropertiesVisible (props, index, visible)
    local pfgroup = props[index]

    if pfgroup then
       pfgroup.isVisible = visible
    end
  end

  --
  --
  --

  local layout = utils.GetLayout()
    
  local function InitFilter (fi, index, pinfo)
    local filter = filter_list[index - 1]

    local pfgroup = utils.Begin("vanilla")

    layout:Restore(pinfo)
    layout:NextColumn(10)
    layout:RebaseY()

    local sep = utils.Separator(15)

    sep.isVisible = false

    for j = 1, filter:getParamCount() do
      local fname, ftype, info = filter:getParamName(j), filter:getParamType(j)
      local fmin, fmax = filter:getParamMin(j), filter:getParamMax(j)
      local v, extra = core:getFilterParameter(0, fi, j)

      if info then
        utils.Begin("subgroup")
        utils.Text(fname)

        for fval = fmin, fmax do
          utils.RadioButton{
            label = info[fval],
            action = function(on)
              if on then
                core:setFilterParameter(0, fi, j, fval)
              end
            end, enabled = fval == v
          }
          utils.NewLine()
        end

        utils.End()

        extra = 15

      --
      --
      --

      elseif ftype == "FLOAT" then
        utils.Slider{
          label = fname,
          action = function(t)
            core:setFilterParameter(0, fi, j, fmin + t * (fmax - fmin))
          end,
          value = (v - fmin) * 100 / (fmax - fmin)
        }

        extra = 5

      --
      --
      --

      elseif ftype == "BOOL" then
        utils.Checkbox{
          label = fname,
          action = function(on)
            core:setFilterParameter(0, fi, j, on and 1 or 0)
          end,
          checked = v > .5
        }

        extra = 10

      --
      --
      --

      elseif ftype == "INT" then
        -- ?? ('info' case should cover all of these)
      end

      utils.NewLine(extra)
    end

    utils.End()

    return pfgroup
  end

  --
  --
  --

  local list_info, pad, pinfo = layout:Save()

  for fi = 1, 4 do
    if fi > 1 then
      layout:Restore(list_info)
    end

    local fgroup = utils.Begin("subgroup")
    local current, properties = 1, {}

    utils.List{
      labels = {
        "None",
        "BassboostFilter",
        "BiquadResonantFilter",
        "DCRemovalFilter",
        "EchoFilter",
        "FFTFilter",
        "FlangerFilter",
        "FreeverbFilter",
        "LofiFilter",
        "RobotizeFilter",
        "WaveShaperFilter",
        "EqFilter"
      },

      on_select = function(index)
        if index ~= current then
          SetPropertiesVisible(properties, current, false)
          SetPropertiesVisible(properties, index, true)

          current = index

          if index > 1 then
            core:setGlobalFilter(fi, filter_list[index - 1])

            if not properties[index] then
              properties[index] = InitFilter(fi, index, pinfo)

              fgroup:insert(properties[index])
            end
          else
            core:setGlobalFilter(fi, nil)
          end
        end
      end,
      extra = 10
    }

    pinfo = pinfo or layout:Save()
    
    if not pad then
      pad = display.newRect(0, 0, 350, 10) -- account for widest properties list

      layout:AddToRow(pad)
    end

    utils.End()

    if fi > 1 then
      fgroup.isVisible = false
    end

    filter_lists[fi] = fgroup
  end

  local filter_window = utils.End()

  pad:removeSelf() -- no longer needed

  return filter_window
end

--
--
--

function M.AddSFXRButtons (core, sfx)
  local function SFXRButton (what)
    utils.Button{
      label = "Play random SFXR preset " .. what, width = 375,
      action = function()
        sfx:loadPreset(what, random(32768))
        core:play(sfx, 2, (random(512) - 256) / 256.0)
      end
    }
    utils.NewLine()
  end

  SFXRButton("EXPLOSION")
  SFXRButton("BLIP")
  SFXRButton("COIN")
  SFXRButton("HURT")
  SFXRButton("JUMP")
  SFXRButton("LASER")
end

--
--
--

function M.Loop (body)
  timer.performWithDelay(50, body, 0)
end

--
--
--

function M.MakeHistogramAndWave ()
  local wave = utils.MakeWave(.3, .6, .8)
  local histo = utils.MakeHistogram(.7, .3, .5)

  return histo, wave
end

--
--
--

function M.UpdateFilterParams (core, handle, params)
  for i = 1, #params do
    for name, v in pairs(params[i]) do
      core:setFilterParameter(handle, i, name, v)
    end
  end
end

--
--
--

function M.UpdateOutput (core, histo, wave, active_voices)
  if wave then
    utils.UpdateWave(wave, core:getWave())
  end

  if histo then
    utils.UpdateHistogram(histo, core:calcFFT())
  end

  if active_voices then
    active_voices.text = ("Active voices    : %d"):format(core:getActiveVoiceCount())
  end
end

--
--
--

return M