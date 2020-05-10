--- This plugin provides a page curl widget, the basic idea being to give the look and feel
-- of turning a physical page as in a book or magazine.
--
-- To use the plugin, add the following in <code>build.settings</code>:
--
-- <pre><code class="language-lua">
-- plugins = {  
--   ["plugin.pagecurl"] = { publisherId = "com.xibalbastudios" }
-- }
-- </code></pre>
--
-- Sample code is available [here](https://github.com/ggcrunchy/corona-plugin-docs/blob/master/page_curl_sample.lua).

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
local unpack = unpack

-- Modules --
local class = require("plugin.pagecurl.class")
local grab = require("plugin.pagecurl.grab")
local objects = require("plugin.pagecurl.objects")
local shader = require("plugin.pagecurl.shader")

-- Corona globals --
local display = display

-- Library --
local page_curl = require("CoronaLibrary"):new{ name = 'pagecurl', publisherId = 'com.xibalbastudios' }

-- Page curl widget methods --
local PageCurlWidget = {}

--- Puts the widget in batching mode.
--
-- The motivation is to mitigate the potentially expensive overhead that comes with updating
-- the widget's graphics machinery, e.g. creating new shaders, when several setter methods
-- are to be called in succession. Instead, the intermediate values are merely recorded, but
-- not acted upon until the batch is committed, cf. @{PageCurlWidget:Commit}.
--
-- When batching is already in progress, this is a no-op.
--
-- **N.B. #1** This is irrelevant to touch-related methods.
--
-- **N.B. #2** Until the batch has been committed, the widget will be in an irregular state.
-- In the interim, polling the widget's curl properties and getter methods will not in
-- general yield correct results. Touch controls will likewise be out of sync.
function PageCurlWidget:Begin ()
	self.m_in_batch = true
end

-- Helper to add a #define to a shader prelude
local function Define (defines, symbol, value)
	if symbol then
		defines[#defines + 1] = ([[
			#define %s%s%s
		]]):format(symbol, value and " " or "", value and ("%.4f"):format(value) or "")
	end
end

-- Back texture method -> symbol map; n.b. does double duty indicating available methods, thus the false symbol --
local BackTextureMethod = { same = false, none = "NO_BACK_TEXTURE" }

-- Color method -> symbol map; cf. note on back texture methods --
local ColorMethod = { both = false, back_only = "NO_FRONT_COLOR_SCALE", front_only = "NO_BACK_COLOR_SCALE" }

-- Edge effect -> symbol map; cf. note on back texture methods --
local EdgeEffect = { edge = false, none = "NO_EDGE_EFFECT", shadow = "TINT_SHADOW" }

-- Intermediate color table --
local Colors = {}

-- Brings the curl effect into conformity with the widget's state
local function Commit (curl)
	local defines = {}

	-- Resolve image-related options. These definitions are added conditionally. Were they
	-- all added blindly, some combinations would result in spurious "undefined" kernels on
	-- account of a not-yet-encountered prelude.
	local mode = curl.m_mode

	if mode == "no_image" then
		Define(defines, "NO_FRONT_TEXTURE")
	elseif mode == "composite" then
		Define(defines, "USE_SECOND_TEXTURE")
	else
		Define(defines, BackTextureMethod[curl.m_back_texture_method])
	end

	-- Do the rest, which lead to no false positives.
	Define(defines, curl.m_clip and "NO_EXPAND")
	Define(defines, curl.m_no_inner_shadows and "NO_INNER_SHADOWS")
	Define(defines, ColorMethod[curl.m_color_method])
	Define(defines, EdgeEffect[curl.m_edge_effect])

	-- Set the fill color, using an intermediate table to get the argument count right. Clear
	-- this table afterward in case it contains object references, namely gradients.
	local object = curl.m_object

	Colors[1], Colors[2], Colors[3], Colors[4] = curl.m_saved_r, curl.m_saved_g, curl.m_saved_b, curl.m_saved_a

	object:setFillColor(unpack(Colors))

	Colors[1], Colors[2], Colors[3], Colors[4] = nil

	-- Set the effect corresponding to the resolved options and mode. Set its properties.
	local cf = object.fill

	cf.effect = shader.GetName(defines, mode)
	cf.effect.angle = curl.m_saved_angle
	cf.effect.u = curl.m_saved_u
	cf.effect.v = curl.m_saved_v

	-- Clear any batching.
	curl.m_in_batch, curl.m_dirty = false, false
end

-- Batch-aware helper to update curl widget state
local function Update (curl)
	if curl.m_in_batch then
		curl.m_dirty = true
	else
		Commit(curl)
	end
end

-- Helper to set a flag and report whether it actually changed
local function UpdateFlag (curl, key, value)
	local not_value = not value

	if not curl[key] ~= not_value then -- account for nil keys
		curl[key] = not not_value -- coerce to bool

		return true
	end
end

--- Captures a target and sets it as the current page.
--
-- This will commit any batch in progress, cf. @{PageCurlWidget:Begin}.
-- @tparam ?|DisplayObject|ContentBounds|nil target If _target_ is a display object,
-- this method has [display.capture](https://docs.coronalabs.com/api/library/display/capture.html) semantics.
--
-- If _target_ is **nil** instead, [display.captureScreen](https://docs.coronalabs.com/api/library/display/captureScreen.html) semantics are chosen.
--
-- Otherwise, [display.captureBounds](https://docs.coronalabs.com/api/library/display/captureBounds.html) semantics are used and _target_ is assumed to
-- be a table compatible with [object.contentBounds](https://docs.coronalabs.com/api/type/DisplayObject/contentBounds.html).
function PageCurlWidget:Capture (target)
	objects.HideRect(self)
	objects.RemoveInMode(self, "capture")
	objects.RemoveInMode(self, "snapshot")

	-- Capture some content.
	local capture

	if target then
		if target._type then
			capture = display.capture(target)
		else
			capture = display.captureBounds(target)
		end
	else
		capture = display.captureScreen()
	end

	-- Add it to the group, i.e. the curl widget itself.
	self:insert(capture)

	-- Install the capture as the curl object.
	self.m_object, self.m_mode = capture, "capture"

	objects.ConfigureNonRect(self)
	Commit(self)
end

--- If a batch is in progress, commits it. Otherwise, this is a no-op.
-- @see PageCurlWidget:Begin
function PageCurlWidget:Commit ()
	if self.m_dirty then
		Commit(self)
	end
end

-- Helper to create a new auxiliary rect
local function NewRect (curl)
	return display.newRect(curl, 0, 0, 1, 1)
end

--- The display object underlying the page is removed from the widget's control. Aside from
-- changing parents, it keeps its most recent state; however, the widget is no longer able to
-- affect it, and _vice versa_.
--
-- The widget itself is cleared to a blank rect; this will commit any batch in progress, cf.
-- @{PageCurlWidget:Begin}.
-- @pgroup[opt] into Group that will receive the object. If absent, the stage is used.
-- @treturn pobject Detached object.
-- @see PageCurlWidget:SetBlankRect
function PageCurlWidget:Detach (into)
	into = into or display.getCurrentStage()

	local mode, object = self.m_mode, self.m_object

	if mode ~= "capture" and mode ~= "snapshot" then
		self.m_view = NewRect(self)
	end

	local gx, gy = self:localToContent(object.x, object.y)

	into:insert(object)

	object.x, object.y = into:contentToLocal(gx, gy)

	self.m_mode = "detaching"

	self:SetBlankRect()

	return object
end

--- Disables all touch controls.
-- @bool no_reset Leave the page as is?
-- @see PageCurlWidget:SetDirection, PageCurlWidget:SetTouchSides
function PageCurlWidget:DisableTouch (no_reset)
	grab.CommitChoices(self.m_touch, no_reset)
end

--- Enables or disables expansion.
--
-- As a page curls, it tends to spill out of the boundaries of the underlying display object.
-- To accommodate this, the object can effectively be expanded, and its textures will be read
-- with this in mind.
--
-- When expansion is disabled, out-of-bounds pixels are clipped. This might suffice, for
-- instance, if the area around the widget is already obscured, say when it fills the screen.
--
-- Expansion is enabled by default.
-- @bool enable Enable expansion?
function PageCurlWidget:EnableExpansion (enable)
	local mode = self.m_mode

	if UpdateFlag(self, "m_clip", not enable) then
		if mode == "capture" or mode == "snapshot" then
			objects.ConfigureNonRect(self)
		else
			objects.ConfigureRects(self)
		end

		Update(self)
	end
end

--- Enables or disables inner shadows.
--
-- Once a page has begun curling, a subtle shadow can be applied inside the curl to convey
-- the reduced incidence of light.
--
-- Inner shadows are enabled by default.
-- @bool enable Enable inner shadows?
function PageCurlWidget:EnableInnerShadows (enable)
	if UpdateFlag(self, "m_no_inner_shadows", not enable) then
		Update(self)
	end
end

--- Gets the currently touchable regions.
--
-- The widget does not draw any of its touch components, but this method lets users know
-- where those are, for instance in order to overlay custom display objects.
-- @treturn table Any (or none) of the **bottom**, **left**, **right**, or **top** keys may
-- be present, corresponding to the sides enabled for touch. Their values are tables with
-- **x**, **y**, **width**, and **height** members, matching the current center and size of
-- the given side's grab region.
-- @see PageCurlWidget:SetDirection, PageCurlWidget:SetTouchSides
function PageCurlWidget:GetGrabRegions ()
	return grab.GetRegions(self.m_touch)
end

--- Gets the (unexpanded) page size.
-- @treturn uint Page width...
-- @treturn uint ...and height.
-- @see PageCurlWidget:EnableExpansion
function PageCurlWidget:GetSize ()
	local object = self.m_object
	local w, h = object.width, object.height

	if self.m_mode == "capture" then
		w, h = w * display.contentScaleX, h * display.contentScaleY
	end

	return w, h
end

--- Prepares a snapshot and sets it as the current page. (When the widget is already using a
-- snapshot, it simply updates that one.)
--
-- This will commit any batch in progress, cf. @{PageCurlWidget:Begin}.
-- @uint[opt] width Snapshot width... (If absent, the page's current value is reused.)
-- @uint[opt] height ...and height. (Ditto.)
-- @treturn SnapshotObject The snapshot. Well-behaved code will change neither its position
-- nor its dimensions and scale, nor manually remove it (see instead @{PageCurlWidget:Detach}),
-- but otherwise this is meant to be used like any snapshot.
function PageCurlWidget:PrepareSnapshot (width, height)
	objects.HideRect(self)

	if not (width and height) then
		local object = self.m_object

		width, height = width or object.width, height or object.height
	end

	objects.RemoveInMode(self, "capture")

	-- When the widget is already in snapshot mode, reuse the object, merely resizing it.
	-- Otherwise, create a new snapshot.
	local snapshot

	if self.m_mode == "snapshot" then
		snapshot = self.m_object

		snapshot.width, snapshot.height = width, height
	else
		snapshot = display.newSnapshot(self, width, height)
	end

	-- Install the snapshot as the curl object.
	self.m_object, self.m_mode = snapshot, "snapshot"

	objects.ConfigureNonRect(self)
	Commit(self)

	return snapshot
end

-- Helper to update the widget, given a valid method change
local function LookupMethod (curl, key, t, how)
	local what = t[how]
	local entry = what and how or nil

	if what ~= nil and entry ~= curl[key] then
		curl[key] = entry

		Update(curl)
	end
end

--- Sets the back texture behavior.
--
-- If _how_ is unrecognized, this is a no-op.
-- @string how One of the following:
--
-- * **"same"**: The front texture is reused on the back of the page. This is the default.
-- * **"none"**: The back of the page is blank, i.e. it has no texture.
-- @see PageCurlWidget:SetImage
function PageCurlWidget:SetBackTextureMethod (how)
	LookupMethod(self, "m_back_texture_method", BackTextureMethod, how)
end

-- Paint-type fill --
local PaintFill = { 1 }

--- Sets a blank rectangle as the current page.
--
-- This will commit any batch in progress, cf. @{PageCurlWidget:Begin}.
-- @ptable[opt] opts Configuration options. Fields:
--
-- * **width**: Width to assign to page... (If absent, the current value is retained.)
-- * **height**: ...and the height. (Ditto.)
function PageCurlWidget:SetBlankRect (opts)
	objects.ConfigureRects(self, opts)

	self.m_object.m_fill, self.m_mode = PaintFill, "no_image"

	Commit(self)
end

--- Sets the widget's color. This behaves like [object:setFillColor](https://docs.coronalabs.com/api/type/ShapeObject/setFillColor.html),
-- albeit constrained by the widget's color method.
--
-- White, i.e. `1, 1, 1, 1`, is the default.
-- @param ... Color parameters, cf. [object:setFillColor](https://docs.coronalabs.com/api/type/ShapeObject/setFillColor.html).
--
-- Gradients are not supported.
-- @see PageCurlWidget:SetColorMethod
function PageCurlWidget:SetColor (...)
	self.m_saved_r, self.m_saved_g, self.m_saved_b, self.m_saved_a = ...

	local object = self.m_object

	if object then
		self.m_object:setFillColor(...)
	end
end

--- Sets the color behavior.
--
-- If _how_ is unrecognized, this is a no-op.
-- @string how One of the following:
--
-- * **"both"**: Color is applied to the front and back sides of the page. This is the
-- default behavior.
-- * **"front_only"**: Color affects only the front side of the page.
-- * **"back_only"**: Color affects only the back side of the page.
-- @see PageCurlWidget:SetColor
function PageCurlWidget:SetColorMethod (how)
	LookupMethod(self, "m_color_method", ColorMethod, how)
end

--- Sets the curl direction used by touch. Touch is enabled in the appropriate region and
-- disabled elsewhere.
--
-- If _dir_ is unrecognized, this is a no-op.
-- @string dir One of **"bottom\_to\_top"**, **"left\_to\_right"**, **"right\_to\_left"**,
-- or **"top\_to\_bottom"**.
-- @see PageCurlWidget:DisableTouch, PageCurlWidget:GetGrabRegions, PageCurlWidget:SetTouchSides
function PageCurlWidget:SetDirection (dir)
	grab.Choose(self.m_touch, dir)
	grab.CommitChoices(self.m_touch)
end

--- Sets the edge effect.
--
-- Once a page has been curled a bit, its back side comes into view. An effect may be applied
-- on or near the edges of this back leaf.
--
-- If _how_ is unrecognized, this is a no-op.
-- @string how One of the following:
--
-- * **"edge"**: Thin dark lines demarcate the back side edges. This is the default.
-- * **"none"**: No effect: back side pixels are not set off from those in front.
-- * **"shadow"**: Soft shadows overlay the front side of the page near the back's edges.
function PageCurlWidget:SetEdgeEffect (how)
	LookupMethod(self, "m_edge_effect", EdgeEffect, how)
end

-- Composite-type fill --
local CompositeFill = { type = "composite", paint1 = { type = "image" }, paint2 = { type = "image" } }

--- Sets two images as the current page, one for the front side and another for the back.
-- @string front Filename of the front image...
-- @string back ...and back image.
--
-- This will commit any batch in progress, cf. @{PageCurlWidget:Begin}.
-- @ptable[opt] opts Configuration options. Fields:
--
-- * **dir1**: Base directory of _front_... (The default is `system.ResourceDirectory`.)
-- * **dir2**: ...and _back_. (Ditto.)
-- * **width**: Width to assign to page... (When absent, the current value is retained.)
-- * **height**: ...and the height. (Ditto.)
function PageCurlWidget:SetFrontAndBackImages (front, back, opts)
	local dir1, dir2

	if opts then
		dir1, dir2 = opts.dir1, opts.dir2
	end

	objects.ConfigureRects(self, opts)

	CompositeFill.paint1.filename, CompositeFill.paint1.baseDir = front, dir1
	CompositeFill.paint2.filename, CompositeFill.paint2.baseDir = back, dir2

	self.m_object.fill, self.m_mode = CompositeFill, "composite"

	Commit(self)
end

-- Image-type fill --
local ImageFill = { type = "image" }

--- Sets an image as the current page.
--
-- The back side of the page is determined by the back texture method, cf. @{PageCurlWidget:SetBackTextureMethod}.
--
-- This will commit any batch in progress, cf. @{PageCurlWidget:Begin}.
-- @string name Filename of the image.
-- @ptable[opt] opts Configuration options. Fields:
--
-- * **dir**: Base directory of _name_. (When absent, this is `system.ResourceDirectory`.)
-- * **width**: Width to assign to page... (If absent, the current value is retained.)
-- * **height**: ...and the height. (Ditto.)
function PageCurlWidget:SetImage (name, opts)
	objects.ConfigureRects(self)

	ImageFill.filename, ImageFill.baseDir = name, opts and opts.dir

	self.m_object.fill, self.m_mode = ImageFill, "image"

	Commit(self)
end

-- Side combination -> direction map --
local Sides = {
	bottom_and_top = { "bottom_to_top", "top_to_bottom" },
	left_and_right = { "right_to_left", "left_to_right" }
}

--- Enables touch on multiple sides, disabling the rest.
--
-- In each case, the curl direction is toward the opposite side, e.g. for the right side, the
-- direction is right-to-left.
--
-- If _how_ is unrecognized, this is a no-op.
-- @string how This may be **"bottom\_and\_top"** or **"left\_and\_right"**.
-- @see PageCurlWidget:DisableTouch, PageCurlWidget:GetGrabRegions, PageCurlWidget:SetDirection
function PageCurlWidget:SetTouchSides (how)
	local sides, touch = Sides[how], self.m_touch

	if sides then
		for i = 1, #sides do
			grab.Choose(touch, sides[i])
		end

		grab.CommitChoices(touch)
	end
end

-- Options for initial blank rect --
local ConfigRectsOpts = {}

--- Constructs a new page curl widget.
--
-- The **PageCurlWidget** type derives from [GroupObject](https://docs.coronalabs.com/api/type/GroupObject/index.html).
-- Well-behaved code should leave its children alone, as well as its dimensions and scale,
-- but may otherwise treat it as a normal display group.
--
-- In addition to the properties common to all display groups, **PageCurlWidget** introduces
-- **edge\_x**, **edge\_y**, **angle**, and **angle\_radians**. These may be both read and
-- written, say as `local angle = my\_curl\_widget.angle` and `my\_curl\_widget.edge\_x = .3`.
--
-- The **edge\_x** and **edge\_y** properties mark a point along the edge where the page
-- begins to curl, the so-called "curling axis". (Any such point is fine.) The coordinate
-- system goes from left to right horizontally and top to bottom vertically, with a value of
-- 0 corresponding to the left-hand side or top, and 1 to the right-hand side or bottom.
--
-- The **angle** property describes the direction of the outward-facing <a href="https://en.wikipedia.org/wiki/Normal_(geometry)">
-- normal</a> to this edge, i.e. along the direction of curl. Angles are denominated in
-- degrees, going clockwise. The **angle\_radians** property is provided as an alternative,
-- accepting radians instead. This is merely a convenience, as both modify the same state.
--
-- The widget is designed for both programmatic and touch-based usage.
--
-- When touch is enabled, the widget dispatches the **"page\_dragged"** event during any
-- touch movement that updates the page. The widget can be accessed from a listener as
-- `event.target`, and the side being touched (one of **"bottom"**, **"left"**, **"top"**,
-- or **"right"**) is available as `event.dir`.
--
-- Likewise, **"page\_grabbed"** and **"page\_released"** events are fired when touch begins
-- and ends, respectively. The **target** and **dir** members may again be found in _event_.
-- @ptable[opt] opts Constructor options. Fields:
--
-- * **left**: When present, this is the x-coordinate of the widget's left-hand side... (The
-- default is 0.)
-- * **top**: ...and this is the y-coordinate of its top. (The default is 0.)
-- * **width**: The width of the initial (blank) rect... (When absent, a default is used.)
-- * **h**: ...and the height. (Ditto.)
-- * **size**: A positive integer used to size the grabbable regions, for touch purposes.
-- This specifies both the thickness of each grab region and the amount of cushion added
-- to include touches that are only slightly out-of-bounds. For instance, the bottom will
-- have width and height of `size + page width` and `size`, respectively, whereas the right-hand
-- side has `size`, `size + page height`. (When absent, a default is used.)
-- @treturn PageCurlWidget New page curl widget.
function page_curl.NewPageCurlWidget (opts)
	local curl = display.newGroup()

	-- Bind the class machinery.
	class.Init(curl, PageCurlWidget)

	-- Get any options.
	local size, top, left, w, h

	if opts then
		size, top, left, w, h = opts.size, opts.top, opts.left, opts.width, opts.height
	end

	-- Configure the rect size, using options when available.
	ConfigRectsOpts.width, ConfigRectsOpts.height = w or 32, h or 32

	-- Create auxiliary reference and view rects.
	curl.m_ref, curl.m_view = NewRect(curl), NewRect(curl)

	curl.m_ref.isVisible = false

	-- Install touch controls and put them in a default state.
	curl.m_touch = grab.MakeSet(curl, size or 40)

	grab.Choose(curl.m_touch, "right_to_left")

	-- Begin with a blank white rectangle. Set the color and curl properties so that the
	-- widget already has "saved" values. (The intent behind these is to persist state
	-- across changes of the underlying object and / or shader.) The property assignments
	-- follow the "blank" operation (which puts the widget into "no image" mode), once an
	-- object is in place to receive them.
	curl:SetColor(1, 1, 1, 1)
	curl:SetBlankRect(ConfigRectsOpts)

	curl.angle_radians, curl.edge_x, curl.edge_y = 0, 1, 1

	-- Put the widget into position and return it.
	curl.x, curl.y = left or 0, top or 0

	return curl
end

return page_curl