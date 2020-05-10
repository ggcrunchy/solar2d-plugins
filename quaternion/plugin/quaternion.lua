--- Some operations on quaternions.
--
-- A **Quaternion** is a table (or object with **__index** and **__newindex** metamethods)
-- with **x**, **y**, **z**, and **w** components, each being a **number**.
--
-- All functions are able to take any combination of their **Quaternion** arguments. Namely,
-- the same **Quaternion** may be passed multiple times without incident. Any routine that
-- produces a **Quaternion** will place it in its _qout_ argument (which is also returned, to
-- allow for easy composition). An argument may be both a pure input and _qout_; _qout_ is
-- only written once the necessary values have been read.

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
local acos = math.acos
local cos = math.cos
local exp = math.exp
local log = math.log
local pi = math.pi
local sin = math.sin
local sqrt = math.sqrt

-- Modules --
-- local robust = require("plugin.tektite_core.number.robust")

-- Robust, inlined --
local asin = math.asin

local function ROBUST_AngleBetween (dot, len, sub)
	return function(a, b, out)
		local neg = dot(a, b) < 0

		out = out or a

		sub(out, b, a)

		local angle = 2 * asin(len(out) / 2)

		return neg and pi - angle or angle
	end
end

local function ROBUST_SinOverX (x)
	return 1 + x^2 == 1 and 1 or sin(x) / x
end

local function ROBUST_SlerpCoeffs (t, theta)
	local denom, s = ROBUST_SinOverX(theta), 1 - t

	return ROBUST_SinOverX(s * theta) * s / denom, ROBUST_SinOverX(t * theta) * t / denom
end
-- /Robust, inlined

-- Cached module references --
local _Add_
local _Add_Scaled_
local _AngleBetween_
local _Conjugate_
local _Dot_
local _Exp_
local _FromAxisAngle_
local _Inverse_
local _Length_
local _Log_
local _Multiply_
local _Negate_
local _Normalize_
local _Scale_
local _Slerp_

-- Library --
local lib = require("CoronaLibrary"):new{ name = 'Quaternion', publisherId = 'com.xibalbastudios' }

--- Adds quaternions _q1_ and _q2_.
-- @tparam Quaternion qout _q1_ + _q2_.
-- @tparam Quaternion q1 Addend #1...
-- @tparam Quaternion q2 ...and #2.
-- @treturn Quaternion _qout_.
function lib.Add (qout, q1, q2)
	qout.x = q1.x + q2.x
	qout.y = q1.y + q2.y
	qout.z = q1.z + q2.z
	qout.w = q1.w + q2.w

	return qout
end

--- Adds quaternions _q1_ and a scaled _q2_.
-- @tparam Quaternion qout _q1_ + _q2_ * _k_.
-- @tparam Quaternion q1 Addend #1...
-- @tparam Quaternion q2 ...and #2.
-- @number k Scale factor.
-- @treturn Quaternion _qout_.
function lib.Add_Scaled (qout, q1, q2, k)
	qout.x = q1.x + q2.x * k
	qout.y = q1.y + q2.y * k
	qout.z = q1.z + q2.z * k
	qout.w = q1.w + q2.w * k

	return qout
end

-- Forward references --
local AuxAngleBetween

do
	local A1, A2, TwoPi = {}, {}, 2 * pi

	--- Gives the angle between two quaternions.
	-- @tparam Quaternion q1 Quaternion #1...
	-- @tparam Quaternion q2 ...and #2.
	-- @treturn number Shorter angle between _q1_ and _q2_.
	function lib.AngleBetween (q1, q2)
		local angle = 2 * AuxAngleBetween(_Normalize_(A1, q1), _Normalize_(A2, q2))

		return angle > pi and TwoPi - angle or angle
	end
end

--- Computes the conjugate of _q_.
-- @tparam Quaternion qout Conjugate.
-- @tparam Quaternion q
-- @treturn Quaternion _qout_.
function lib.Conjugate (qout, q)
	qout.x = -q.x
	qout.y = -q.y
	qout.z = -q.z
	qout.w = q.w

	return qout
end

do
	local Qi = {}

	--- Computes the quaternion _diff_ such that _q1_ * _diff_ = _q2_.
	-- @tparam Quaternion qout Difference.
	-- @tparam Quaternion q1 Initial quaternion.
	-- @tparam Quaternion q2 Final quaternion.
	-- @treturn Quaternion _qout_.
	function lib.Difference (qout, q1, q2)
		return _Multiply_(qout, _Inverse_(Qi, q1), q2)
	end
end

--- Dot product of _q1_ and _q2_.
-- @tparam Quaternion q1 Quaternion #1...
-- @tparam Quaternion q2 ...and #2.
-- @treturn number Dot product.
function lib.Dot (q1, q2)
	return q1.x * q2.x + q1.y * q2.y + q1.z * q2.z + q1.w * q2.w
end

--- Computes the exponential of _q_.
--
-- Given _q_ = [**w**, _theta_ * _v_] (with _v_ a unit vector), the exponential is defined as
-- [cos(_theta_), sin(_theta_) * _v_] * _e_<sup>**w**</sup>.
-- @tparam Quaternion qout `exp(q)`.
-- @tparam Quaternion q
-- @treturn Quaternion _qout_.
function lib.Exp (qout, q)
	local qx, qy, qz, ew = q.x, q.y, q.z, exp(q.w)
	local vnorm = sqrt(qx^2 + qy^2 + qz^2)
	local coeff = ew * ROBUST_SinOverX(vnorm)

	qout.w, qout.x, qout.y, qout.z = ew * cos(vnorm), coeff * qx, coeff * qy, coeff * qz

	return qout
end

--- Builds a quaternion representing a rotation around an axis.
--
-- The axis does not need to be normalized.
-- @tparam Quaternion qout Rotation.
-- @number angle Rotation angle, in radians.
-- @number vx Axis x-component...
-- @number vy ...y-component...
-- @number vz ...and z-component.
-- @treturn Quaternion _qout_.
function lib.FromAxisAngle (qout, angle, vx, vy, vz)
	angle = .5 * angle

	local coeff = sin(angle) / sqrt(vx^2 + vy^2 + vz^2)

	qout.w, qout.x, qout.y, qout.z = cos(angle), coeff * vx, coeff * vy, coeff * vz

	return qout
end

do
	local Order, Axis = {
		xyz = function(x, y, z) return x, y, z end,
		xzy = function(x, y, z) return x, z, y end,
		yxz = function(x, y, z) return y, x, z end,
		yzx = function(x, y, z) return y, z, x end,
		zxy = function(x, y, z) return z, x, y end,
		zyx = function(x, y, z) return z, y, x end
	}, {}

	--- Builds a quaternion representation of a sequence of rotations around the unit axes.
	-- @tparam Quaternion qout Rotation.
	-- @number x Angle around the x-axis...
	-- @number y ...around the y-axis...
	-- @number z ...and around the z-axis.
	-- @string[opt="xyz"] method One of **"xyz"**, **"xzw"**, **"yxz"**, **"yzx"**, **"zxy"**,
	-- or **"zyx"**. Where _X_, _Y_, and _Z_ are the rotations around those axes, **"xyz"**
	-- corresponds to _X_ * _Y_ * _Z_. The other methods follow likewise.
	-- @treturn Quaternion _qout_.
	function lib.FromEulerAngles (qout, x, y, z, method)
		local order = Order[method] or Order.xyz

		x, y, z = order(x, y, z)

		if method == "yzx" or method == "zxy" then -- axes are swapped in these two cases
			order = Order[method == "yzx" and "zxy" or "yzx"]
		end

		_FromAxisAngle_(qout, z, order(0, 0, 1))
		_FromAxisAngle_(Axis, y, order(0, 1, 0))
		_Multiply_(qout, Axis, qout)
		_FromAxisAngle_(Axis, x, order(1, 0, 0))
		_Multiply_(qout, Axis, qout)

		return qout
	end
end

--- Computes the inverse of a (non-zero) quaternion, i.e. the quaternion _q_<sup>-1</sup>
-- such that _q_<sup>-1</sup> * _q_ or _q_ * _q_<sup>-1</sup> gives identity quaternion _I_
-- (**x** = **y** = **z** = 0, **w** = 1).
--
-- Given quaternion _Q_, _I_ * _Q_ or _Q_ * _I_ yield back _Q_.
-- @tparam Quaternion qout Inverse.
-- @tparam Quaternion q
-- @treturn Quaternion _qout_.
function lib.Inverse (qout, q)
	return _Scale_(qout, _Conjugate_(qout, q), 1 / _Dot_(q, q))
end

--- Length of quaternion _q_.
-- @tparam Quaternion q
-- @treturn number Length.
function lib.Length (q)
	return sqrt(q.x^2 + q.y^2 + q.z^2 + q.w^2)
end

--- Computes the logarithm of _q_.
--
-- Given _q_ = [cos(_theta_), sin(_theta_) * _v_] * length(_q_), the logarithm is defined as
-- [ln(length(_q_)), _theta_ * _v_ / length(_v_)].
-- @tparam Quaternion qout `log(q)`.
-- @tparam Quaternion q
-- @treturn Quaternion _qout_.
function lib.Log (qout, q)
	-- Adapted from:
	-- https://github.com/numpy/numpy-dtypes/blob/76da931005a088f9e5f75d8ea2d58428cad2a975/npytypes/quaternion/quaternion.c#L121
	local qx, qy, qz = q.x, q.y, q.z
	local sqr = qx^2 + qy^2 + qz^2
	local vnorm = sqrt(sqr)

	if vnorm > 1e-6 then
		local qw = q.w
		local mag = sqrt(sqr + qw^2)
		local coeff = acos(qw / mag) / vnorm

		qout.w, qout.x, qout.y, qout.z = log(mag), coeff * qx, coeff * qy, coeff * qz
	else
		qout.w, qout.x, qout.y, qout.z = 0, 0, 0, 0
	end

	return qout
end

--- Multiplies quaternions _q1_ and _q2_.
-- @tparam Quaternion qout _q1_ * _q2_.
-- @tparam Quaternion q1 Multiplier.
-- @tparam Quaternion q2 Multiplicand.
-- @treturn Quaternion _qout_.
function lib.Multiply (qout, q1, q2)
	local x1, y1, z1, w1 = q1.x, q1.y, q1.z, q1.w
	local x2, y2, z2, w2 = q2.x, q2.y, q2.z, q2.w

	qout.x = w1 * x2 + w2 * x1 + y1 * z2 - y2 * z1
	qout.y = w1 * y2 + w2 * y1 + z1 * x2 - z2 * x1
	qout.z = w1 * z2 + w2 * z1 + x1 * y2 - x2 * y1
	qout.w = w1 * w2 - x1 * x2 - y1 * y2 - z1 * z2

	return qout
end

--- Negates quaternion _q_.
-- @tparam Quaternion qout _-q_.
-- @tparam Quaternion q
-- @treturn Quaternion _qout_.
function lib.Negate (qout, q)
	qout.x = -q.x
	qout.y = -q.y
	qout.z = -q.z
	qout.w = -q.w

	return qout
end

--- Normalizes a (non-zero) quaternion.
-- @tparam Quaternion qout _q_ scaled to length 1.
-- @tparam Quaternion q Non-zero quaternion.
-- @treturn Quaternion _qout_.
-- @see Length
function lib.Normalize (qout, q)
	return _Scale_(qout, q, 1 / _Length_(q))
end

-- TODO: M.Power (qout, q, t)?

--- Scales quaternion _q_.
-- @tparam Quaternion qout _q_ * _k_.
-- @tparam Quaternion q
-- @number k Scale factor.
-- @treturn Quaternion _qout_.
function lib.Scale (qout, q, k)
	qout.x = q.x * k
	qout.y = q.y * k
	qout.z = q.z * k
	qout.w = q.w * k

	return qout
end

do
	local Qf, Qt = {}, {}

	--- Performs spherical linear interpolation from _q1_ to _q2_.
	-- @tparam Quaternion qout Interpolated result.
	-- @tparam Quaternion q1 Initial quaternion.
	-- @tparam Quaternion q2 Final quaternion.
	-- @number t Interpolation time.
	--
	-- At time _t_ = 0 or _t_ = 1, _qout_ will match _q1_ or _q2_ respectively. Times within
	-- this range, on the other hand, yield quaternions between _q1_ and _q2_.
	-- @treturn Quaternion _qout_.
	function lib.Slerp (qout, q1, q2, t)
		_Normalize_(Qf, q1)
		_Normalize_(Qt, q2)

		local dot, k1, k2 = _Dot_(Qf, Qt)

		if dot < 0 then
			_Negate_(Qf, Qf)

			dot = -dot
		end

		if dot > .95 then
			k1, k2 = 1 - t, t
		else
			k1, k2 = ROBUST_SlerpCoeffs(t, _AngleBetween_(Qf, Qt))
		end

		_Add_Scaled_(qout, _Scale_(Qf, Qf, k1), Qt, k2)

		return _Normalize_(qout, qout)
	end
end

-- Forward references --
local AuxSquadQuats, AuxSquad

do
	local Qi, Log1, Log2, Sum = {}, {}, {}, {}

	-- Helper to generate intermediate squad quaternions
	function AuxSquadQuats (qout, qprev, q, qnext)
		_Inverse_(Qi, q)
		_Log_(Log1, _Multiply_(Log1, Qi, qprev))
		_Log_(Log2, _Multiply_(Log2, Qi, qnext))
		_Scale_(Sum, _Add_(Sum, Log1, Log2), -.25)

		return _Multiply_(qout, _Exp_(Sum, Sum), q)
	end
end

do
	local Qa, Qb = {}, {}

	-- Core squad computation
	function AuxSquad (qout, q1, q2, s1, s2, t)
		return _Slerp_(qout, _Slerp_(Qa, q1, q2, t), _Slerp_(Qb, s1, s2, t), 2 * t * (1 - t))
	end
end

do
	local S1, S2 = {}, {}

	--- Performs spherical quadrangle interpolation between four quaternions.
	--
	-- Quaternions _q1_, _q2_, _q3_, and _q4_ are understood to be in order.
	--
	-- Useful results will typically be sought between _q2_ and _q3_, with _q1_ and _q4_
	-- being used to guide the shape of the path.
	-- @tparam Quaternion qout Interpolated result.
	-- @tparam Quaternion q1 Quaternion #1...
	-- @tparam Quaternion q2 ...quaternion #2...
	-- @tparam Quaternion q3 ...quaternion #3...
	-- @tparam Quaternion q4 ...and #4.
	-- @number t Interpolation time.
	--
	-- At time _t_ = 0 or _t_ = 1, _qout_ will match _q2_ or _q3_ respectively. Times within
	-- this range, on the other hand, yield quaternions between _q2_ and _q3_.
	--
	-- Furthermore, _t_ = 0 aligns with _t_ = 1 in the interval (_q0_, _q1_, _q2_, _q3_).
	-- Likewise, _t_ = 1 corresponds to _t_ = 0 in the interval (_q2_, _q3_, _q4_, _q5_).
	-- Owing to this and the interpolation's C&sup2; continuity, it is possible to gracefully
	-- transition from one interval to another.
	-- @treturn Quaternion _qout_.
	function lib.Squad (qout, q1, q2, q3, q4, t)
		return AuxSquad(qout, q2, q3, AuxSquadQuats(S1, q1, q2, q3), AuxSquadQuats(S2, q2, q3, q4), t)
	end
end

--- Subtracts quaternion _q2_ from _q1_.
-- @tparam Quaternion qout _q1_ - _q2_.
-- @tparam Quaternion q1 Minuend.
-- @tparam Quaternion q2 Subtrahend.
-- @treturn Quaternion _qout_.
function lib.Sub (qout, q1, q2)
	qout.x = q1.x - q2.x
	qout.y = q1.y - q2.y
	qout.z = q1.z - q2.z
	qout.w = q1.w - q2.w

	return qout
end

-- Helper to calculate angle between two quaternions
AuxAngleBetween = ROBUST_AngleBetween(lib.Dot, lib.Length, lib.Sub)

-- Cache module members.
_Add_ = lib.Add
_Add_Scaled_ = lib.Add_Scaled
_AngleBetween_ = lib.AngleBetween
_Conjugate_ = lib.Conjugate
_Dot_ = lib.Dot
_Exp_ = lib.Exp
_FromAxisAngle_ = lib.FromAxisAngle
_Inverse_ = lib.Inverse
_Length_ = lib.Length
_Log_ = lib.Log
_Multiply_ = lib.Multiply
_Negate_ = lib.Negate
_Normalize_ = lib.Normalize
_Scale_ = lib.Scale
_Slerp_ = lib.Slerp

-- Export the module.
return lib
