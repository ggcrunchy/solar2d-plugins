--- TODO

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

--- Convert coefficients from B&eacute;zier (P1, Q1, Q2, P2) to Hermite (P1, P2, T1, T2) form.
--
-- This (and the similar functions in this module) are written so that output coefficients
-- may safely overwrite the inputs, i.e. if _src*_ = _dst*_.
-- @tparam Vector src1 Vector #1 (i.e. P1)...
-- @tparam Vector src2 ...#2 (Q1)...
-- @tparam Vector src3 ...#3 (Q2)...
-- @tparam Vector src4 ...and #4 (P2).
-- @tparam[opt=src1] Vector dst1 Target vector #1 (i.e. will receive P1)...
-- @tparam[opt=src2] Vector dst2 ...#2 (P2)...
-- @tparam[opt=src3] Vector dst3 ...#3 (T1)...
-- @tparam[opt=src4] Vector dst4 ...and #4 (T2).
function M.BezierToHermite (src1, src2, src3, src4, dst1, dst2, dst3, dst4)
	dst1, dst2, dst3, dst4 = dst1 or src1, dst2 or src2, dst3 or src3, dst4 or src4

	local t1x, t1y = (src2.x - src1.x) * 3, (src2.y - src1.y) * 3
	local t2x, t2y = (src4.x - src3.x) * 3, (src4.y - src3.y) * 3

	dst1.x, dst1.y = src1.x, src1.y
	dst2.x, dst2.y = src4.x, src4.y
	dst3.x, dst3.y = t1x, t1y
	dst4.x, dst4.y = t2x, t2y
end

--
--
--

--- Convert coefficients from Catmull-Rom (P1, P2, P3, P4) to Hermite (P2, P3, T1, T2) form.
-- @tparam Vector src1 Vector #1 (i.e. P1)...
-- @tparam Vector src2 ...#2 (P2)...
-- @tparam Vector src3 ...#3 (P3)...
-- @tparam Vector src4 ...and #4 (P4).
-- @tparam[opt=src1] Vector dst1 Target vector #1 (i.e. will receive P2)...
-- @tparam[opt=src2] Vector dst2 ...#2 (P3)...
-- @tparam[opt=src3] Vector dst3 ...#3 (T1)...
-- @tparam[opt=src4] Vector dst4 ...and #4 (T2).
function M.CatmullRomToHermite (src1, src2, src3, src4, dst1, dst2, dst3, dst4)
	dst1, dst2, dst3, dst4 = dst1 or src1, dst2 or src2, dst3 or src3, dst4 or src4

	local t1x, t1y = src3.x - src1.x, src3.y - src1.y
	local t2x, t2y = src4.x - src2.x, src4.y - src2.y

	dst1.x, dst1.y = src2.x, src2.y
	dst2.x, dst2.y = src3.x, src3.y
	dst3.x, dst3.y = t1x, t1y
	dst4.x, dst4.y = t2x, t2y
end

--
--
--

local Div = 1 / 3

--- Convert coefficients from Hermite (P1, P2, T1, T2) to B&eacute;zier (P1, Q1, Q2, P2) form.
-- @tparam Vector src1 Vector #1 (i.e. P1)...
-- @tparam Vector src2 ...#2 (P2)...
-- @tparam Vector src3 ...#3 (T1)...
-- @tparam Vector src4 ...and #4 (T2).
-- @tparam[opt=src1] Vector dst1 Target vector #1 (i.e. will receive P1)...
-- @tparam[opt=src2] Vector dst2 ...#2 (Q1)...
-- @tparam[opt=src3] Vector dst3 ...#3 (Q2)...
-- @tparam[opt=src4] Vector dst4 ...and #4 (P2).
function M.HermiteToBezier (src1, src2, src3, src4, dst1, dst2, dst3, dst4)
	dst1, dst2, dst3, dst4 = dst1 or src1, dst2 or src2, dst3 or src3, dst4 or src4

	local q1x, q1y = src1.x + src3.x * Div, src1.y + src3.y * Div
	local q2x, q2y = src2.x - src4.x * Div, src2.y - src4.y * Div

	dst1.x, dst1.y = src1.x, src1.y
	dst4.x, dst4.y = src2.x, src2.y
	dst2.x, dst2.y = q1x, q1y
	dst3.x, dst3.y = q2x, q2y
end

--
--
--

--- Convert coefficients from Hermite (P1, P2, T1, T2) to Catmull-Rom (P0, P1, P2, P3) form.
-- @tparam Vector src1 Vector #1 (i.e. P1)...
-- @tparam Vector src2 ...#2 (P2)...
-- @tparam Vector src3 ...#3 (T1)...
-- @tparam Vector src4 ...and #4 (T2).
-- @tparam[opt=src1] Vector dst1 Target vector #1 (i.e. will receive P0)...
-- @tparam[opt=src2] Vector dst2 ...#2 (P1)...
-- @tparam[opt=src3] Vector dst3 ...#3 (P2)...
-- @tparam[opt=src4] Vector dst4 ...and #4 (P3).
function M.HermiteToCatmullRom (src1, src2, src3, src4, dst1, dst2, dst3, dst4)
	dst1, dst2, dst3, dst4 = dst1 or src1, dst2 or src2, dst3 or src3, dst4 or src4

	local p1x, p1y = src2.x - src3.x, src2.y - src3.y
	local p4x, p4y = src4.x - src1.x, src4.y - src1.y

	dst3.x, dst3.y = src2.x, src2.y
	dst2.x, dst2.y = src1.x, src1.y
	dst1.x, dst1.y = p1x, p1y
	dst4.x, dst4.y = p4x, p4y
end

--
--
--

return M