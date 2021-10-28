--- Various cubic splines and related functionality.
--
-- In several of the routines, the spline type will be one of **"bezier"**, **"catmull_rom"**,
-- or **"hermite"**.
--
-- For purposes of this module, an instance of type **Coeffs** is a value, e.g. a table,
-- that has and / or receives **number** members **a**, **b**, **c**, **d**, whereas for a
-- **Vector** the members are **x** and **y**.

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

-- Cached module references --
local _ComputeCoefficients_
local _GetPosition_
local _GetTangent_
local _ResolveCoefficients_

-- Exports --
local M = {}

--
--
--

-- "Right-hand side" eval functions --
-- General idea: Given the eval matrix, i.e. the constant matrix defined by the particular
-- spline, and a time t, coefficients for spline position and / or tangent can be generated,
-- independent of the spline's geometry, and mapped / updated later when geometry is supplied.
local RHS = {}

function RHS.bezier (coeffs, a, b, c, d)
	coeffs.a = a - 3 * (b - c) - d
	coeffs.b = 3 * (b - 2 * c + d)
	coeffs.c = 3 * (c - d)
	coeffs.d = d
end

function RHS.catmull_rom (coeffs, a, b, c, d)
	coeffs.a = .5 * (-b + 2 * c - d)
	coeffs.b = .5 * (2 * a - 5 * c + 3 * d)
	coeffs.c = .5 * (b + 4 * c - 3 * d)
	coeffs.d = .5 * (-c + d)
end

function RHS.hermite (coeffs, a, b, c, d)
	coeffs.a = a - 3 * c + 2 * d
	coeffs.b = 3 * c - 2 * d
	coeffs.c = b - 2 * c + d
	coeffs.d = -c + d
end

--- Compute coefficents for use with @{ResolveCoefficients}.
-- @string stype Spline type.
-- @tparam ?|Coeffs|nil pos If present, position coefficients to evaluate at _t_.
-- @tparam ?|Coeffs|nil tan If present, tangent coefficients to evaluate at _t_.
-- @number t Interpolation time, &isin; [0, 1].
function M.ComputeCoefficients (stype, pos, tan, t)
	local eval, t2 = RHS[stype], t * t

	if pos then
		eval(pos, 1, t, t2, t2 * t)
	end

	if tan then
		eval(tan, 0, 1, 2 * t, 3 * t2)
	end
end

--
--
--

local Coeffs = {}

--- Get the position along the spline at time _t_.
--
-- This is a convenience wrapper around the common case that the user does not need to
-- consider @{ComputeCoefficients} and @{ResolveCoefficients} separately.
-- @string stype Spline type.
-- @tparam Vector a Vector #1 defining the spline...
-- @tparam Vector b ...#2...
-- @tparam Vector c ...#3...
-- @tparam Vector d ...and #4.
-- @number t Interpolation time, &isin; [0, 1].
-- @treturn number Position x-coordinate...
-- @treturn number ...and y-coordinate.
function M.GetPosition (stype, a, b, c, d, t)
	_ComputeCoefficients_(stype, Coeffs, nil, t)

	return _ResolveCoefficients_(Coeffs, a, b, c, d)
end

--
--
--

--- Array variant of @{GetPosition}.
-- @string stype Spline type.
-- @array pos Elements 1, 2, 3, 4 are interpreted as arguments _a_, _b_, _c_, _d_
-- from @{GetPosition}.
-- @number t Interpolation time, &isin; [0, 1].
-- @treturn number Position x-coordinate...
-- @treturn number ...and y-coordinate.
function M.GetPosition_Array (stype, pos, t)
	local a, b, c, d = unpack(pos)

	return _GetPosition_(stype, a, b, c, d, t)
end

--
--
--

--- Get the tangent to the spline at time _t_.
--
-- This is a convenience wrapper around the common case that the user does not need to
-- consider @{EvaluateCoeffs} and @{MapCoeffsToSpline} separately.
-- @string stype Spline type.
-- @tparam Vector a Vector #1 defining the spline...
-- @tparam Vector b ...#2...
-- @tparam Vector c ...#3...
-- @tparam Vector d ...and #4.
-- @number t Interpolation time, &isin; [0, 1].
-- @treturn number Tangent x-component...
-- @treturn number ...and y-component.
function M.GetTangent (stype, a, b, c, d, t)
	_ComputeCoefficients_(stype, nil, Coeffs, t)

	return _ResolveCoefficients_(Coeffs, a, b, c, d)
end

--
--
--

--- Array variant of @{GetTangent}.
-- @string stype Spline type.
-- @array tan Elements 1, 2, 3, 4 are interpreted as arguments _a_, _b_, _c_, _d_
-- from @{GetTangent}.
-- @number t Interpolation time, &isin; [0, 1].
-- @treturn number Tangent x-component...
-- @treturn number ...and y-component.
function M.GetTangent_Array (stype, tan, t)
	local a, b, c, d = unpack(tan)

	return _GetTangent_(stype, a, b, c, d, t)
end

--
--
--

--- Given pre-computed coefficients, resolve some geometry to a spline.
-- @tparam Coeffs coeffs Coefficients generated e.g. by @{ComputeCoefficients}.
-- @tparam Vector a Vector #1 defining the spline...
-- @tparam Vector b ...#2...
-- @tparam Vector c ...#3...
-- @tparam Vector d ...and #4.
-- @treturn number x-component...
-- @treturn number ...and y-component.
function M.ResolveCoefficients (coeffs, a, b, c, d)
	local x = coeffs.a * a.x + coeffs.b * b.x + coeffs.c * c.x + coeffs.d * d.x
	local y = coeffs.a * a.y + coeffs.b * b.y + coeffs.c * c.y + coeffs.d * d.y

	return x, y
end

--
--
--

local Pos, Tan = {}, {}

--- Truncate a B&eacute;zier spline, i.e. the part of the spline &isin; [_t1_, _t2_] becomes
-- a new B&eacute;zier spline, reparameterized to the interval [0, 1].
-- @tparam Vector src1 Vector #1 (i.e. P1)...
-- @tparam Vector src2 ...#2 (Q1)...
-- @tparam Vector src3 ...#3 (Q2)...
-- @tparam Vector src4 ...and #4 (P2).
-- @number t1 Lower bound of new interval, &isin; [0, _t2_).
-- @number t2 Upper bound of new interval, &isin; (_t1_, 1].
-- @tparam[opt=src1] Vector dst1 Target vector #1 (i.e. will receive P1)...
-- @tparam[opt=src2] Vector dst2 ...#2 (Q1)...
-- @tparam[opt=src3] Vector dst3 ...#3 (Q2)...
-- @tparam[opt=src4] Vector dst4 ...and #4 (P2).
function M.Truncate (src1, src2, src3, src4, t1, t2, dst1, dst2, dst3, dst4)
	dst1, dst2, dst3, dst4 = dst1 or src1, dst2 or src2, dst3 or src3, dst4 or src4

	-- The basic idea (see e.g. Eric Lengyel's "Mathematics for 3D Game Programming and
	-- Computer Graphics") is to do an implicit Hermite-to-Bezier conversion. The two
	-- endpoints are simply found by evaluating the spline at the ends of the interval.
	_ComputeCoefficients_("bezier", Pos, Tan, t1)

	local p1x, p1y = _ResolveCoefficients_(Pos, src1, src2, src3, src4)
	local t1x, t1y = _ResolveCoefficients_(Tan, src1, src2, src3, src4)

	_ComputeCoefficients_("bezier", Pos, Tan, t2)

	local p2x, p2y = _ResolveCoefficients_(Pos, src1, src2, src3, src4)
	local t2x, t2y = _ResolveCoefficients_(Tan, src1, src2, src3, src4)

	-- The new 0 <= u <= 1 interval is related to the old interval by t(u) = t1 + (t2 - t1)u.
	-- The truncated spline, being "in" the old spline, is given by Trunc(u) = B(t(u)).
	-- Differentiating with respect to u gives (t2 - t1) * B'(t1) and (t2 - t1) * B'(t2) at
	-- the ends of the interval, i.e. the tangents of the implicit Hermite spline.
	local dt = (t2 - t1) / 3

	dst1.x, dst1.y = p1x, p1y
	dst2.x, dst2.y = p1x + t1x * dt, p1y + t1y * dt
	dst3.x, dst3.y = p2x - t2x * dt, p2y - t2y * dt
	dst4.x, dst4.y = p2x, p2y
end

--
--
--

-- TODO: TCB, uniform B splines?

_ComputeCoefficients_ = M.ComputeCoefficients
_GetPosition_ = M.GetPosition
_GetTangent_ = M.GetTangent
_ResolveCoefficients_ = M.ResolveCoefficients

return M