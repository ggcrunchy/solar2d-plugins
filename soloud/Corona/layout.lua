--- Layout class used by UI elements.

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
local pairs = pairs
local remove = table.remove
local setmetatable = setmetatable
local type = type
local unpack = unpack

-- Solar2D globals --
local display = display
local native = native

-- Solar2D modules --
local widget = require("widget")

-- Exports --
local M = {}

--
--
--

local Layout = {}

Layout.__index = Layout

--
--
--

local function GetTop (layout)
  local stack = layout.stack

  return assert(stack[#stack], "Attempting operation on empty layout")
end

--
--
--

function Layout:AddToRow (object, extra, adjust_y)
  local top = GetTop(self)

  local raw_width = top.x + object.width - top.column_x

  if raw_width > top.max_column_width then
    top.max_column_width = raw_width
  end

  object.anchorX, object.x = 0, top.x
  object.anchorY, object.y = 0, top.y + (adjust_y or 0)

  if object.y + object.height > top.y + top.max_row_height then
    top.max_row_height = object.y + object.height - top.y

    if object.y + object.height > top.max_y then
      top.max_y = object.y + object.height
    end
  end

  top.group:insert(object)

  top.x = top.x + object.width + (extra or 0)
end

--
--
--

local function ReifyScrollView (top)
  local group = top.group
  local height, auto_added = group.sv_height, group.auto_added

  if not height and auto_added then
    height = top.max_y - top.min_y
  end

  local scroll_view = widget.newScrollView{
    width = group.width + (auto_added and 0 or 30), height = height,
    hideBackground = auto_added or group.sv_hide_background,
    horizontalScrollDisabled = true
  }

  if not auto_added then
    group:translate(10, 10)
  end

  scroll_view:insert(group)

  return scroll_view
end

--
--
--

local function NewLevel (name, group)
  if name == "existing" then
    assert(type(group) == "table" and group.insert, "Non-group object supplied as existing")
  else
    group = nil
  end

  return { name = name, group = group or display.newGroup(), max_column_width = 0, max_row_height = 0 }
end

local function AddToStack (stack, height, level, x, y)
  level.x, level.x0, level.column_x = x, x, x
  level.y, level.y0, level.min_y, level.max_y = y, y, y, y

  stack[height + 1] = level
end

local function PrepareSubgroup (level, commit)
  assert(commit == nil or type(commit) == "function", "Non-function subgroup commit")

  level.commit = commit
end

local function PrepareScrollview (level, height, hide)
  assert(height == nil or (type(height) == "number" and height > 0), "Non-number scroll view height")

  level.group.sv_height, level.group.sv_hide_background, level.name = height, not not hide, "subgroup"

  PrepareSubgroup(level, ReifyScrollView)
end

--
--
--

local SubTypes = { collapsible = true, existing = true, scrollview = true, subgroup = true, vanilla = true }

function Layout:Begin (name, x, y)
  assert(type(name) == "string", "Non-string name")

  local stack = self.stack
  local level, height = NewLevel(name, x), #stack

  if SubTypes[name] then
    assert(height ~= 0 or (name == "existing" or name == "vanilla"), "Must be in group")

    --
    --
    --

    if name == "collapsible" then
      assert(type(x) == "string", "Non-string collapsible label")

      level.group.label = x
      level.group.isVisible = not y

      local above = stack[height]

      if above.commit ~= ReifyScrollView then
        above = NewLevel()
        above.group.auto_added = true

        PrepareScrollview(above)
        AddToStack(stack, height, above, 0, 0)

        height = height + 1
      end

    --
    --
    --

    elseif name == "scrollview" then
      PrepareScrollview(level, x, y == "hide")

    --
    --
    --

    elseif name == "subgroup" then
      PrepareSubgroup(level, x)
    end

    --
    --
    --

    x, y = 0, 0
  else
    assert(height == 0, "Already in group")
    assert(type(x) == "number", "Non-numeric x")
    assert(type(y) == "number", "Non-numeric y")
  end

  --
  --
  --

  AddToStack(self.stack, height, level, x, y)

  return level.group
end

--
--
--

local Padding, LabelPadding = 25, 20

--
--
--

local function SetColors (object1, object2, method, ...)
  object1[method](object1, ...)
  object2[method](object2, ...)
end

--
--
--

local function ToggleUse (toggle)
  toggle.action(toggle.parent)

  toggle.yScale = -toggle.yScale
end

--
--
--

local function ToggleTouch (event)
  local toggle, phase = event.target, event.phase

  if phase == "began" then
    display.getCurrentStage():setFocus(toggle)

    ToggleUse(toggle)

    toggle.held = true
  elseif toggle.held and phase ~= "moved" then
    display.getCurrentStage():setFocus(nil)

    toggle.held = false
  end

  return true
end

--
--
--

local Points = { 0, 0, 20, 20, -20, 20 }

local function NewToggle (action)
  local toggle = display.newPolygon(0, 0, Points)

  toggle:addEventListener("touch", ToggleTouch)

  toggle.action = action

  return toggle
end

--
--
--

local function ToggleCollapsible (parent)
  local body = parent.body
  local showing = body.isVisible

  local grandparent, pos = parent.parent -- hgroup -> window

  for i = 1, grandparent.numChildren do
    if grandparent[i] == body then
      pos = i

      break
    end
  end

  body.isVisible = not showing

  local delta = showing and -body.height or body.height

  for i = pos + 1, grandparent.numChildren do
    grandparent[i]:translate(0, delta)
  end

  -- Find the scrollview above and update it to reflect the new height.
  while parent do
    if parent.updateScrollAreaSize then
      parent:updateScrollAreaSize()

      break
    end

    parent = parent.parent
  end
end

--
--
--

local CollapsibleFillColor = { 0, 0, 1 }
local CollapsibleStrokeColor = { 0, 0, .5 }

local function ReifyCollapsible (layout, level)
  local group = level.group

  --
  --
  --

  local hgroup = display.newGroup()
  local label = display.newText(hgroup, group.label, 0, 0, level.font or native.systemFontBold, level.font_size or 25)
  local extra_h = label.height + LabelPadding

  hgroup.body = group

  --
  --
  --

  local toggle = NewToggle(ToggleCollapsible)

  hgroup:insert(toggle)

  toggle.x, toggle.y = (toggle.width - Padding) / 2 + 15, extra_h / 2
  label.anchorX, label.x, label.y = 0, toggle.x + toggle.width / 2 + 10, toggle.y

  --
  --
  --

  local gw, lw = group.width, label.x + label.width + Padding / 2
  local w = lw > gw and lw or gw
  local heading = display.newRect(hgroup, 0, 0, w, extra_h)

  heading.anchorX, heading.anchorY = 0, 0

  heading:setFillColor(unpack(level.fill_color or CollapsibleFillColor))
  heading:setStrokeColor(unpack(level.stroke_color or CollapsibleStrokeColor))
  heading:toBack()

  heading.strokeWidth = 2

  --
  --
  --

  layout:AddToRow(hgroup)
  layout:NextRow()

  --
  --
  --

  local top = GetTop(layout)
  local y = top.y

  layout:AddToRow(group)
  layout:NextRow()

  if not group.isVisible then
    top.y, top.max_y, toggle.yScale = y, y, -toggle.yScale
  end
end

--
--
--

local function DragTouch (event)
  local backdrop, phase = event.target, event.phase
  local main_group = backdrop.parent.parent

  if phase == "began" then
    display.getCurrentStage():setFocus(backdrop)

    backdrop.dx, backdrop.dy = event.x - main_group.x, event.y - main_group.y
  elseif backdrop.dx then
    if phase == "moved" then
      main_group.x, main_group.y = event.x - backdrop.dx, event.y - backdrop.dy
    else
      display.getCurrentStage():setFocus(nil)

      backdrop.dx = nil
    end
  end

  return true
end

--
--
--

local function ToggleBackdrop (parent)
  parent.full.isVisible = not parent.full.isVisible
  parent.collapsed.isVisible = not parent.collapsed.isVisible
end

--
--
--

local MainFillColor = { 0, .1, .9, .7 }
local MainStrokeColor = { 0, 0, 1, .7 }

local function ReifyGroup (level)
  local group = level.group

  --
  --
  --

  local label = display.newText(level.name, 0, 0, level.font or native.systemFontBold, level.font_size or 25)
  local gw, gh, lw, lh = group.width, level.max_y - level.min_y, label.width, label.height
  local extra_h = lh + LabelPadding
  local gx, w, h = level.x0 + gw / 2, (lw > gw and lw or gw) + Padding, gh + Padding + extra_h

  label.x, label.y = gx, level.min_y - (Padding + extra_h) / 2

  --
  --
  --

  local backdrop_full = display.newRoundedRect(gx, level.min_y + (gh - extra_h) / 2, w, h, 12)
  local backdrop_collapsed = display.newRoundedRect(gx, label.y, w, extra_h, 12)

  SetColors(backdrop_full, backdrop_collapsed, "setFillColor", unpack(level.fill_color or MainFillColor))
  SetColors(backdrop_full, backdrop_collapsed, "setStrokeColor", unpack(level.stroke_color or MainStrokeColor))

  backdrop_full.strokeWidth = 2
  backdrop_collapsed.strokeWidth = 2

  backdrop_full:addEventListener("touch", DragTouch)
  backdrop_collapsed:addEventListener("touch", DragTouch)

  --
  --
  --

  local toggle = NewToggle(ToggleBackdrop)

  toggle.x, toggle.y = level.x0 - (toggle.width - Padding) / 2 + 25, label.y

  if lw > gw then
    toggle.x = toggle.x - (lw - gw) / 2
  end

  --
  --
  --

  local main_group = display.newGroup()

  main_group.full, main_group.collapsed = display.newGroup(), display.newGroup()

  main_group.full:insert(backdrop_full)
  main_group.full:insert(level.group)
  main_group.collapsed:insert(backdrop_collapsed)

  main_group.collapsed.isVisible = false

  --
  --
  --

  main_group:insert(main_group.full)
  main_group:insert(main_group.collapsed)
  main_group:insert(label)
  main_group:insert(toggle)
  main_group:translate((w - gw) / 2, lh + LabelPadding + Padding / 2)

  --
  --
  --

  function main_group:Toggle ()
    ToggleUse(toggle)
  end

  return main_group
end

--
--
--

function Layout:End (extra)
  local top = assert(remove(self.stack), "Attempt to end empty layout")
  local name = top.name

  if name == "collapsible" then
    ReifyCollapsible(self, top)
  elseif name == "subgroup" then
    local object

    if top.commit then
      object = top.commit(top)
    end

    self:AddToRow(object or top.group, extra)

    if top.group.auto_added then -- scroll view auto-added, but trying to end top-level group?
      return self:End(), object
    end

    return object
  elseif not SubTypes[name] then
    return ReifyGroup(top)
  end
end

--
--
--

function Layout:GetColumnWidth ()
  return GetTop(self).max_column_width
end

--
--
--

function Layout:GetPosition ()
  local top = GetTop(self)

  return top.x, top.y
end

--
--
--

function Layout:NextColumn (extra)
  local top = GetTop(self)

  top.column_x, top.max_column_width = top.column_x + top.max_column_width + (extra or 0), 0
  top.x, top.y, top.max_row_height = top.column_x, top.y0, 0
end

--
--
--

function Layout:NextRow (extra)
  local top = GetTop(self)

  top.x, top.y, top.max_row_height = top.column_x, top.y + top.max_row_height + (extra or 0), 0
end

--
--
--

function Layout:RebaseY ()
  local top = GetTop(self)

  top.y0 = top.y
end

--
--
--

local function Copy (info)
  local copy = {}

  for k, v in pairs(info) do
    copy[k] = v
  end

  return copy
end

--
--
--

function Layout:Restore (info)
  local stack = self.stack
  local height = #stack
  local top = stack[height]
  local group, name = top.group, top.name

  assert(height > 0, "Attempting restore on empty layout")

  top = Copy(info)

  stack[height], top.group, top.name = top, group, name
end

--
--
--

function Layout:Save ()
  local copy = Copy(GetTop(self))

  copy.group, copy.name = nil

  return copy
end

--
--
--

function M.New (opts)
  if opts then
    -- color, subgroup color, collapsible
  end

  return setmetatable({ stack = {} }, Layout)
end

--
--
--

return M