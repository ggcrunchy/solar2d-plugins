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

-- Standard library imports --
local sqrt = math.sqrt
local unpack = unpack

-- Cached module references --
local _GetCoefficients_

-- Exports --
local M = {}

--
--
--

-- "Left-hand side" eval functions --
-- General idea: Given geometry matrix [P1 P2 P3 P4] and eval matrix, i.e. the constant
-- matrix defined by the particular spline, compute a row of the 2x4 product matrix. This
-- will produce the polynomial coefficients for that row (x- or y-components), for spline
-- length purposes. The components are reordered (since the quadrature algorithms expect
-- [t^3, t^2, t, 1]), dropping the constant which goes to 0 during differentiation.
local LHS = {}

function LHS.bezier (a, b, c, d)
	local B = 3 * (b - a)
	local C = 3 * (a + c - 2 * b)
	local D = -a + 3 * (b - c) + d

	return D, C, B
end

function LHS.catmull_rom (a, b, c, d)
	local B = .5 * (-a + c)
	local C = .5 * (2 * a - 5 * b + 4 * c - d)
	local D = .5 * (-a + 3 * (b - c) + d)

	return D, C, B
end

function LHS.hermite (a, b, c, d)
	local B = c
	local C = 3 * (b - a) - 2 * c - d
	local D = 2 * (a - b) + c + d

	return D, C, B
end

--- Get polynomial coefficients for calculating line integrands.
-- 
-- A given cubic spline can be written as a polynomial _A_x&sup3; + _B_x&sup2; + _C_x + _D_, for
-- both its x- and y-components. Furthermore, its derivative is useful for computing the
-- arc length of the spline.
-- @string stype Spline type.
-- @tparam Vector a Vector #1 defining the spline...
-- @tparam Vector b ...#2...
-- @tparam Vector c ...#3...
-- @tparam Vector d ...and #4.
-- @treturn number _A_, for x...
-- @treturn number ...and y.
-- @treturn number _B_, for x...
-- @treturn number ...and y.
-- @treturn number _C_, for x...
-- @treturn number ...and y.
-- @see LineIntegrand, SetPolyFromCoeffs
function M.GetCoefficients (stype, a, b, c, d)
	local eval = LHS[stype]

	-- Given spline Ax^3 + Bx^2 + Cx + D, the derivative is 3Ax^2 + 2Bx + C, which when
	-- squared (in the arc length formula) yields these coefficients. 
	local ax, bx, cx = eval(a.x, b.x, c.x, d.x)
	local ay, by, cy = eval(a.y, b.y, c.y, d.y)

	return ax, ay, bx, by, cx, cy
end

--
--
--

--- Array variant of @{GetCoefficients}.
-- @string stype Spline type.
-- @array spline Elements 1, 2, 3, 4 are interpreted as arguments _a_, _b_, _c_, _d_
-- respectively from @{GetPolyCoeffs}.
-- @return As per @{GetPolyCoeffs}.
function M.GetCoefficients_Array (stype, spline)
	return _GetCoefficients_(stype, unpack(spline))
end

--
--
--

--- [Line integrand](http://en.wikipedia.org/wiki/Arc_length#Finding_arc_lengths_by_integrating) for a cubic polynomial.
-- @array[opt] poly The underlying polynomial, (dx/dt)&sup2; + (dy/dt)&sup2;: elements 1 to
-- 5 are the x&#8308;, x&sup3;, x&sup2;, x, and constant coefficients, respectively. If
-- absent, a table is supplied.
-- @treturn function Integrand function, which may be passed e.g. as the _func_ argument to
-- the various integrators.
-- @treturn array _poly_.
-- @see SetPolyFromCoeffs
function M.LineIntegrand (poly)
	poly = poly or {}

	return function(t)
		return sqrt(t * (t * (t * (t * poly[1] + poly[2]) + poly[3]) + poly[4]) + poly[5])
	end, poly
end

--
--
--

--- Assign integrand coefficents&mdash;in particular, as expected by @{LineIntegrand}'s
-- integrand function&mdash;given a cubic polynomial's derivatives: dx/dt = 3_Ax&sup2;_ +
-- 2_Bx_ + _C_, dy/dt = 3_Dy&sup2;_ + 2_Ey_ + F.
-- @array poly Polynomial.
-- @number ax _Ax&sup2;_.
-- @number ay _Dy_&sup2;.
-- @number bx _Bx_.
-- @number by _Ey_.
-- @number cx _C_.
-- @number cy _F_.
function M.SetFromCoefficients (poly, ax, ay, bx, by, cx, cy)
	-- Given curve Ax^3 + Bx^2 + Cx + D, the derivative is 3Ax^2 + 2Bx + C, which
	-- when squared (in the arc length formula) yields these coefficients. 
	poly[1] = 9 * (ax^2 + ay^2)
	poly[2] = 12 * (ax * bx + ay * by)
	poly[3] = 6 * (ax * cx + ay * cy) + 4 * (bx^2 + by^2)
	poly[4] = 4 * (bx * cx + by * cy)
	poly[5] = cx^2 + cy^2
end

--
--
--

_GetCoefficients_ = M.GetCoefficients

return M