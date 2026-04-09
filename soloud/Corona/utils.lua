--- Demo utilities.

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
local floor = math.floor
local max = math.max
local type = type

-- Solar2D globals --
local display = display
local native = native

-- Solar2D modules --
local layout = require("layout")
local widget = require("widget")

-- Exports --
local M = {}

--
--
--

local background = display.newImageRect("graphics/soloud_bg.png", display.contentWidth, display.contentHeight)

background.x, background.y = display.contentCenterX, display.contentCenterY
background.alpha = .6

widget.setTheme("widget_theme_ios7")

--
--
--

local Layout = layout.New()

--
--
--

function M.AddRect (w, h)
  local rect = display.newRect(0, 0, w, h)

  Layout:AddToRow(rect)

  return rect
end

--
--
--

function M.Begin (name, x, y)
  return Layout:Begin(name, x, y)
end

--
--
--

local function IsString (v)
  local vtype = type(v)

  return vtype == "string" or vtype == "number"
end

--
--
--

function M.Button (params)
  assert(type(params) == "table", "Non-table params")
  assert(IsString(params.label), "Non-string label")
  assert(type(params.action) == "function", "Non-function action")

  local action = params.action

  local button = widget.newButton{
    width = params.width, height = params.height,
    label = params.label, font = params.font, fontSize = params.font_size or 20,
    isEnabled = params.isEnabled,

    onEvent = function(event)
      if event.phase == "ended" then
        action(event.target)
      end
    end
  }

  Layout:AddToRow(button, params.extra)

  return button
end

--
--
--

local Adjustments = { ["-half"] = -.5, ["+half"] = .5 }

local function AddString (text, adjust, font, size, extra)
  local str = display.newText(text, 0, 0, font or native.systemFontBold, size or 25)
  local named_adjustment = Adjustments[adjust]

  if named_adjustment then
    adjust = named_adjustment * str.height
  end

  Layout:AddToRow(str, extra, adjust)
end

--
--
--

function M.Checkbox (params)
  assert(type(params) == "table", "Non-table params")
  assert(IsString(params.label), "Non-string label")
  assert(params.action == nil or type(params.action) == "function", "Non-function action")

  local action = params.action

  local checkbox = widget.newSwitch{
    width = params.size or 35, height = params.size or 35,
    style = "checkbox",
    initialSwitchState = params.checked,
    onPress = action and function(event)
      action(event.target.isOn)
    end
  }

  Layout:AddToRow(checkbox, params.extra or 5)

  if params.label then
    AddString(params.label)
  end

  return checkbox
end

--
--
--

local function SetDragText (str, label, v)
  str.text = ("%s: %.3f"):format(label, v)
end

local function DragFloatTouch (event)
  local drag, phase = event.target, event.phase

  if phase == "began" then
    display.getCurrentStage():setFocus(drag)

    drag.v0 = drag.object[drag.field]
    drag.x0 = event.x
  elseif drag.x0 then
    if phase == "moved" then
      local v = drag.v0 + (event.x - drag.x0) * drag.vscale

      drag.object[drag.field] = v

      if drag.func then
        drag.func(v)
      end

      SetDragText(drag.text, drag.label, v)
    else
      display.getCurrentStage():setFocus(nil)

      drag.x0 = nil
    end
  end

  return true
end

function M.DragFloat (object, field, w, h, scale, label, func, extra)
  local dgroup = display.newGroup()

  --
  --
  --

  local drag = display.newRoundedRect(dgroup, w / 2, h / 2, w, h, 12)

  drag:addEventListener("touch", DragFloatTouch)
  drag:setFillColor(.9, .1, .2)
  drag:setStrokeColor(.1, 0, .7)

  drag.strokeWidth = 2

  --
  --
  --

  drag.text = display.newText(dgroup, "", 0, 0, native.systemFontBold, 25)

  drag.text.x, drag.text.y = w / 2, h / 2
  drag.text.alpha = .7

  SetDragText(drag.text, label, object[field])

  --
  --
  --

  drag.object, drag.field, drag.func, drag.label, drag.vscale = object, field, func, label, scale

  --
  --
  --

  Layout:AddToRow(dgroup, extra)

  return dgroup
end

--
--
--

function M.GetColumnWidth ()
  return Layout:GetColumnWidth()
end

--
--
--

function M.End ()
  return Layout:End()
end

--
--
--

function M.GetLayout ()
  return Layout
end

--
--
--

local function AuxSelectFromList (list, index)
  local text = list[index + 1]

  list.index, list.selection.x, list.selection.y = index, text.x, text.y

  if list.on_select then
    list.on_select(index)
  end
end

local function GetListIndex (list, y)
  local _, ty = list:localToContent(0, 0)
  
  return floor((y - ty) / list.row_height) + 1
end

local function IsInList (list, index)
  local ntext = list.numChildren - 2

  return index >= 1 and index <= ntext
end

local function SelectFromList (list, y)
  local index = GetListIndex(list, y)

  if IsInList(list, index) and index ~= list.index then
    if list.on_unselect then
      list.on_unselect(list.index)
    end

    AuxSelectFromList(list, index)
  end
end

--
--
--

local function ListTouch (event)
  local back, phase = event.target, event.phase
  local list = back.parent

  if phase == "began" then
    display.getCurrentStage():setFocus(back)

    SelectFromList(list, event.y)

    back.is_touched = true
  elseif back.is_touched then
    if phase == "moved" then
      SelectFromList(list, event.y)
    else
      display.getCurrentStage():setFocus(nil)

      back.is_touched = false
    end
  end

  return true
end

--
--
--

function M.List (params)
  assert(type(params) == "table", "Non-table params")
  assert(type(params.labels) == "table", "Non-table labels")

  local tgroup, w, h, n = display.newGroup(), 0, 0, #params.labels

  tgroup.on_select, tgroup.on_unselect = params.on_select, params.on_unselect

  --
  --
  --

  for i = 1, n do
    local text = display.newText(tgroup, params.labels[i], 0, 0, native.systemFontBold, 20)

    text:setFillColor(0)

    w, h = max(w, text.width), max(h, text.height)
  end

  --
  --
  --

  tgroup.selection = display.newRoundedRect(tgroup, 0, 0, w + 10, h + 10, 12)

  tgroup.selection:setFillColor(.7, .4)
  tgroup.selection:setStrokeColor(.6)

  tgroup.selection.strokeWidth = 3

  --
  --
  --

  w, h = w + 20, h + 15

  local ty, gh, halfw, halfh = 0, n * h, w / 2, h / 2

  for i = 1, n do
    local text = tgroup[i]

    text.x, text.y, ty = halfw, ty + halfh, ty + h
  end

  --
  --
  --

  tgroup.back = display.newRoundedRect(tgroup, halfw, gh / 2, w, gh, 12)

  tgroup.back:toBack()
  tgroup.back:addEventListener("touch", ListTouch)

  --
  --
  --

  tgroup.row_height = h

  AuxSelectFromList(tgroup, params.def or 1) -- n.b. fallthrough

  --
  --
  --

  Layout:AddToRow(tgroup, params.extra or 5)

  return tgroup
end

--
--
--

local function Frame (group)
  local top = display.newRect(group, 0, 0, 256 * 2 + 8, 4)

  top.anchorX, top.x = 0, 0
  top.anchorY, top.y = 0, 0

  local bottom = display.newRect(group, 0, 0, 256 * 2 + 8, 4)

  bottom.anchorX, bottom.x = 0, 0
  bottom.anchorY, bottom.y = 0, 80

  local left = display.newRect(group, 0, 0, 4, 80)

  left.anchorX, left.x = 0, 0
  left.anchorY, left.y = 0, 4

  local right = display.newRect(group, 0, 0, 4, 80)

  right.anchorX, right.x = 0, 256 * 2 + 4
  right.anchorY, right.y = 0, 4
end

local HistogramHeight = 80 - 4

--
--
--

function M.MakeHistogram (r, g, b)
  local histogram = display.newGroup()

  Frame(histogram)

  histogram.bars = display.newGroup()

  histogram:insert(histogram.bars)

  local x = 4

  for _ = 1, 128 do
    local bar = display.newRect(histogram.bars, 0, 0, 4, 1)

    bar.anchorX, bar.x = 0, x
    bar.anchorY, bar.y = 1, 80

    x = x + 4

    bar:setFillColor(r, g, b)
    bar:setFillVertexColor(2, .5)
    bar:setFillVertexColor(4, .5)
  end

  local overlay = display.newText(histogram, "FFT", (256 * 2 + 8) / 2, 40, native.systemFontBold, 20)

  overlay.alpha = .7

  Layout:AddToRow(histogram)
  Layout:NextRow(10)

  return histogram
end

--
--
--

function M.MakeWave (r, g, b)
  local wave = display.newGroup()

  Frame(wave)

  wave.x0, wave.r, wave.g, wave.b = 4, r, g, b

  local overlay = display.newText(wave, "Wave", (256 * 2 + 8) / 2, 40, native.systemFontBold, 20)

  overlay.alpha = .7

  Layout:AddToRow(wave)
  Layout:NextRow(10)

  return wave
end

--
--
--

function M.NewColumn (extra)
  Layout:NextColumn(extra)
end

--
--
--

function M.NewLine (extra)
  Layout:NextRow(extra)
end

--
--
--

function M.PercentText (fmt, value, ...)
  return fmt:format(floor(value * 100), ...)
end

--
--
--

function M.RadioButton (params)
  assert(type(params) == "table", "Non-table params")
  assert(IsString(params.label), "Non-string label")
  assert(params.action == nil or type(params.action) == "function", "Non-function action")

  local action = params.action

  local radio_button = widget.newSwitch{
    style = "radio",
    width = params.width or 35, height = params.width or 35,
    initialSwitchState = params.enabled,
    onPress = action and function(event)
      local button = event.target
      local parent = button.parent
      local was = parent.was

      if button ~= was then
        if was.action then
          was.action(false)
        end

        action(true)

        parent.was = button
      end
    end
  }

  radio_button.action = action

  Layout:AddToRow(radio_button, params.extra or 5)

  if params.enabled then
    radio_button.parent.was = radio_button
  end

  if params.label then
    AddString(params.label, nil)
  end

  return radio_button
end

--
--
--

function M.Separator (extra)
  local line = display.newRect(0, 0, 1, 3)

  line:setFillColor(.7)
  Layout:AddToRow(line)
  Layout:NextRow(extra or 5)

  return line
end

--
--
--

function M.Slider (params)
  assert(type(params) == "table", "Non-table params")
  assert(IsString(params.label), "Non-string label")
  assert(params.action == nil or type(params.action) == "function", "Non-function action")

  local action = params.action

  local slider = widget.newSlider{
    width = params.width,
    value = params.value,

    listener = action and function(event)
      action(event.value / 100)
    end
  }

  Layout:AddToRow(slider, params.extra or 10)

  if params.label then
    AddString(params.label, not params.no_adjust and "-half")
  end
end

--
--
--

local MonospacedFont = native.newFont("UbuntuMono-R.ttf", 20)

function M.Text (text, extra, adjust)
  local str = display.newText(text or "", 0, 0, MonospacedFont)

  Layout:AddToRow(str, nil, adjust)
  Layout:NextRow(extra or 5)

  return str
end

--
--
--

function M.Triangle (size)
  local scale, flipped, w, h = 5, false

  if type(size) == "table" then
    w = size.width or size.size
    h = size.height or size.size
    scale = size.scale or scale
    flipped = size.flipped
  else
    w, h = size, size
  end

  local halfw, halfh = w / 2, h / 2
  local y1, y2 = -halfh, halfh

  if flipped then
    y1, y2 = y2, y1
  end

  local tri = display.newPolygon(0, 0, {
    0, y1,
    -halfw, y2,
    halfw, y2
  })

  tri:scale(scale, scale)

  return tri
end

--
--
--

function M.UpdateHistogram (histogram, buffer)
  local bars, j, hprev = histogram.bars, 1

  for _, v in buffer:values() do
    local h = v * 30

    if h > HistogramHeight then
      h = HistogramHeight
    end

    if hprev then
      h = (hprev + h) / 2

      local bar, shade = bars[j], .5 + .5 * h / HistogramHeight

      bar:setFillVertexColor(1, shade)
      bar:setFillVertexColor(3, shade * .875)

      bar.height, j, hprev = h, j + 1
    else
      hprev = h
    end
  end
end

--
--
--

function M.UpdateWave (wave, buffer, hscale)
  display.remove(wave.line)

  hscale = 40 * (hscale or 1)

  local x, y1, line

  for _, v in buffer:values() do
    local y = 40 + v * hscale

    if x then
      if line then
        line:append(x + 2, y)
      else
        line = display.newLine(wave, x, y1, x + 2, y)
      end

      x = x + 2
    else
      x, y1 = wave.x0 + 1, y -- kludge: omit leftmost (and rightmost ) point; since
                             -- waves consist of points, rather than intervals, they
                             -- end up being two pixels short if both are using two-
                             -- pixel segments and the same size of box
    end
  end

  line:setStrokeColor(wave.r, wave.g, wave.b)

  line.strokeWidth, wave.line = 2, line
end

--
--
--

return M
