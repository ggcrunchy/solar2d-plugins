--- Shapes used in sample.

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

-- Exports --
local M = {}

--
--
--

local W, H = display.contentWidth, display.contentHeight

local L1, L2, L3 = 8, 20, 30

M.BoxCCW = {
	x = .25 * W, y = .3 * H,

	L1, -L1, -L1, -L1, -L1, L1, L1, L1, "sep", -- inner square
	L2, -L2, -L2, -L2, -L2, L2, L2, L2, "sep", -- middle square
	L3, -L3, -L3, -L3, -L3, L3, L3, L3 -- outer square
}

M.BoxMixed = {
	x = .75 * W, y = .3 * H,

	-L1, -L1, L1, -L1, L1, L1, -L1, L1, "sep", -- inner square
	-L2, -L2, L2, -L2, L2, L2, -L2, L2, "sep", -- middle square
	L3, -L3, -L3, -L3, -L3, L3, L3, L3 -- outer square
}

M.Overlap = {
	x = .25 * W, y = .6 * H,

	-L2, L3, L2, L3, L2, -L3, -L2, -L3, "sep", -- tall rect
	L3, -L1, -L3, -L1, -L3, L1, L3, L1, "sep", -- wide rect
	L1, -L2, -L1, -L2, 0, L3 + 15 -- triangle
}

local L4 = 40

M.SelfIntersectingSpiral = {
	x = .75 * W, y = .6 * H,

	-L1, -L1, -L1, L1, L1, L1, -- first loop
	L1, -L2, -L2, -L2, -L2, L2, L2, L2, -- second loop
	L2, -L3, -L3, -L3, -L3, L3, L3, L3, -- third loop
	L3, -L4, -L4, -L4, -L4, L4, L4, L4, L4, -L1 -- final loop
}

return M