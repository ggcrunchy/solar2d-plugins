--- Object components of curl effect.

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

-- Modules --
local grab = require("plugin.pagecurl.grab")

-- Cached module references --
local _RemoveInMode_

-- Exports --
local M = {}

--- Configures the widget when the underlying object is not an auxiliary rect.
-- @tparam PageCurlWidget curl
function M.ConfigureNonRect (curl)
	local object, mode, scale, ref = curl.m_object, curl.m_mode

	-- Without clipping, the object is scaled up, so a reference object (arbitrarily, the
	-- small auxiliary rect) is fitted to its original form. Otherwise, the object doubles
	-- as the reference and is left unscaled.
	local w, h = object.width, object.height

	if mode == "capture" then
		w, h = w * display.contentScaleX, h * display.contentScaleY
	end
	
	object.x, object.y = w / 2, h / 2

	if curl.m_clip then
		ref, scale = object, 1
	else
		ref, scale = curl.m_ref, 2

		ref.x, ref.y, ref.width, ref.height = object.x, object.y, w, h
	end

	object.xScale, object.yScale = scale, scale

	if mode == "capture" then
		object.xScale, object.yScale = object.xScale * display.contentScaleX, object.yScale * display.contentScaleY
	end
	-- Put the touch controls over the reference.
	grab.PutOver(curl.m_touch, ref)
end

--- Configures the widget when the underlying object is one of the auxiliary rects.
-- @tparam PageCurlWidget curl
-- @ptable[opt] opts Configuration options, which may include custom **width** and **height**.
function M.ConfigureRects (curl, opts)
	-- Get the rect dimensions, favoring custom ones when provided, and otherwise using
	-- these from the previous curl object.
	local object, w, h = curl.m_object

	if opts then
		w, h = opts.width, opts.height
	end

	w, h = w or object.width, h or object.height

	-- Center the rects.
	local x, y = w / 2, h / 2

	-- Without clipping, the second object comes into play; otherwise, it need not be shown.
	local ref, scale

	if curl.m_clip then
		ref, scale = curl.m_view, 1
	else
		ref, scale = curl.m_ref, 2

		ref.x, ref.y, ref.width, ref.height = x, y, w, h
	end

	-- The small object will always be the reference, so fit it to the original size of the
	-- object. If it will also be the object itself, make it visible.
	object = curl.m_view

	object.x, object.y, object.width, object.height, object.isVisible = x, y, w, h, true
	object.xScale, object.yScale = scale, scale

	-- Remove any capture or snapshot and bind the object.
	_RemoveInMode_(curl, "capture")
	_RemoveInMode_(curl, "snapshot")

	curl.m_object = object

	-- Put the touch controls over the reference.
	grab.PutOver(curl.m_touch, ref)
end

--- Hides the auxiliary rect.
-- @tparam PageCurlWidget curl
function M.HideRect (curl)
	curl.m_view.isVisible = false
end

--- Tries to removes the underlying object.
-- @tparam PageCurlWidget curl
-- @string mode If this mode is active, remove the object.
function M.RemoveInMode (curl, mode)
	if curl.m_mode == mode then
		curl.m_object:removeSelf()
	end
end

-- Cache module members.
_RemoveInMode_ = M.RemoveInMode

-- Export the module.
return M