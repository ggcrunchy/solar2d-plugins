--- Sample code for quaternion plugin.

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

local quaternion = require("plugin.quaternion")

local function Q (what, q)
	print(("%s: x = %.2f, y = %.2f, z = %.2f, w = %.2f"):format(what, q.x, q.y, q.z, q.w))
end

-- Quaternions are just tables with x, y, z, and w
local q1 = { x = 1, y = 2, z = 3, w = 4 }
local q2 = { x = 4, y = 3, z = 2, w = 1 }

Q("q1", q1)
Q("q2", q2)

-- Add, subtract, negate, scale, add scaled, multiply
print("")
Q("q1 + q2", quaternion.Add({}, q1, q2))
Q("q1 - q2", quaternion.Sub({}, q1, q2))
Q("-q1", quaternion.Negate({}, q1))
Q("q2 * 3.1", quaternion.Scale({}, q1, 3.1))
Q("q1 + q2 * 5.2", quaternion.Add_Scaled({}, q1, q2, 5.2))
Q("q1 x q2", quaternion.Multiply({}, q1, q2))

-- Angle between
print("")
print("Angle between q1, q2 (degrees)", math.deg(quaternion.AngleBetween(q1, q2)))

-- Dot product
print("")
print("q1 . q2:", quaternion.Dot(q1, q2))

-- Length
print("")
print("Length of q2:", quaternion.Length(q2))

-- Conjugate
local conj = quaternion.Conjugate({}, q1)

print("")
Q("q1*", conj)
Q("q1* x q1", quaternion.Multiply({}, conj, q1))

-- Log, exp
local log = quaternion.Log({}, q1)

print("")
Q("log(q1)", log)
Q("exp(q1)", quaternion.Exp({}, q1))
Q("exp(log(q1))", quaternion.Exp({}, log))

-- Inverse
local qi = quaternion.Inverse({}, q1)

print("")
Q("q1^-1", qi)
Q("q1^-1 x q1", quaternion.Multiply({}, qi, q1))
Q("q1 x q1^-1", quaternion.Multiply({}, q1, qi))

-- Difference
local diff = quaternion.Difference({}, q1, q2)

print("")
Q("difference of q1, q2", diff)
Q("q1 x diff", quaternion.Multiply({}, q1, diff))

-- Normalization (using self as out)
quaternion.Normalize(q1, q1)
quaternion.Normalize(q2, q2)

print("")
Q("q1, normalized", q1)
Q("q2, normalized", q2)
print("q1, length (normalized):", quaternion.Length(q1))
print("")

-- Helper to rotate vector by quaternion
local Conj = {}

local function Rotate (v, q, reuse)
	local out

	if reuse then
		out = v
	else
		out = { x = v.x, y = v.y, z = v.z, w = 0 }
	end

	-- q x v x q*
	return quaternion.Multiply(out, quaternion.Multiply(out, q, out), quaternion.Conjugate(Conj, q))
end

-- Axis-angle rotation
local V = { x = 2, y = 1, z = 2 }

print("")
print(("Rotating (%.2f, %.2f, %.2f) around axis (0, 0, 1)"):format(V.x, V.y, V.z))

for i = 1, 10 do
	local angle = 360 * (i - 1) / 9

	Q(("%.2f degrees"):format(angle), Rotate(V, quaternion.FromAxisAngle({}, math.rad(angle), 0, 0, 1)))
end

-- Euler angle rotation
print("")
print("Euler angle rotations")

Q("xyz order, 20, 55, 30 degrees", Rotate(V, quaternion.FromEulerAngles({}, math.rad(20), math.rad(55), math.rad(30))))
Q("zxy order, 40, 35, 80 degrees", Rotate(V, quaternion.FromEulerAngles({}, math.rad(40), math.rad(35), math.rad(80), "zxy")))

-- Slerp
print("")
print("Slerp from q1 to q2")

for i = 1, 10 do
	local t = (i - 1) / 9

	Q(("t = %.2f"):format(t), quaternion.Slerp({}, q1, q2, t))
end

-- Some new vectors
local q0 = quaternion.Normalize({}, { x = 3, y = 0, z = -2, w = 1.7 })
local q3 = quaternion.Normalize({}, { x = 7, y = 32, z = 3, w = 2 })

print("")
Q("q0", q0)
Q("q3", q3)

-- Squad
print("")
print("Squad using (q0, q1, q2, q3)")

for i = 1, 10 do
	local t = (i - 1) / 9

	Q(("t = %.2f"):format(t), quaternion.Squad({}, q0, q1, q2, q3, t))
end

-- Now some graphical stuff!
do
	-- Kernel --
	local kernel = { language = "glsl", category = "filter", group = "sphere", name = "bumped" }

	-- Expose effect parameters using vertex data
	kernel.vertexData = {
		{
			name = "distance",
			default = 2, 
			min = 1,
			max = 5,
			index = 0
		},

		{
			name = "light_x",
			default = 1, 
			min = -1,
			max = 1,
			index = 1
		},

		{
			name = "light_y",
			default = 0, 
			min = -1,
			max = 1,
			index = 2
		},

		{
			name = "light_z",
			default = 0, 
			min = -1,
			max = 1,
			index = 3
		},
	}

	kernel.fragment = [[
		P_UV float PI = 4. * atan(1.);
		P_UV float TWO_PI = 8. * atan(1.);
		P_UV float PI_OVER_TWO = 2. * atan(1.);

		P_POSITION vec3 ComputeNormal (sampler2D s, P_UV vec2 uv, P_UV vec3 tcolor)
		{
			P_UV vec3 right = texture2D(s, uv + vec2(CoronaTexelSize.x, 0.)).rgb;
			P_UV vec3 above = texture2D(s, uv + vec2(0., CoronaTexelSize.y)).rgb;
			P_UV float rz = dot(right - tcolor, vec3(1.));
			P_UV float uz = dot(above - tcolor, vec3(1.));

			return normalize(vec3(-uz, -rz, 1.));
		}

		P_POSITION vec3 GetTangent (P_POSITION vec2 diff, P_POSITION float phi)
		{
			// In unit sphere, diff.y = sin(theta), sqrt(1 - sin(theta)^2) = cos(theta).
			return normalize(vec3(diff.yy * sin(vec2(phi + PI_OVER_TWO, -phi)), sqrt(1. - diff.y * diff.y)));
		}

		P_POSITION vec4 GetUV_ZPhi (P_POSITION vec2 diff)
		{
			P_POSITION float dist_sq = dot(diff, diff);
			P_POSITION float z = sqrt(1. - dist_sq);
			P_POSITION float phi = atan(z, diff.x);

			return vec4(.5 + phi / TWO_PI, .5 + asin(diff.y) / PI, z, phi);
		}

		P_POSITION vec3 GetWorldNormal_TS (P_UV vec3 bump, P_POSITION vec3 T, P_POSITION vec3 B, P_POSITION vec3 N)
		{
			return T * bump.x + B * bump.y + N * bump.z;
		}

		P_POSITION vec3 GetWorldNormal (P_UV vec3 bump, P_POSITION vec3 T, P_POSITION vec3 N)
		{
			return GetWorldNormal_TS(bump, T, cross(N, T), N);
		}

		P_COLOR vec4 FragmentKernel (P_UV vec2 uv)
		{
			P_UV vec2 diff = 2. * uv - 1.;
			P_UV vec4 uv_zp = GetUV_ZPhi(diff);

			if (uv_zp.s < 0.) return vec4(0.);

			P_COLOR vec4 tcolor = texture2D(CoronaSampler0, uv_zp.xy);
			P_COLOR vec3 bump = ComputeNormal(CoronaSampler0, uv_zp.xy, tcolor.rgb);
			P_POSITION vec3 N = vec3(diff, uv_zp.z);
			P_POSITION vec3 wn = GetWorldNormal(bump, GetTangent(diff, uv_zp.w), N);
			P_POSITION vec3 L = normalize(CoronaVertexUserData.yzw * CoronaVertexUserData.x - N); // x = distance, yzw = light position
			P_COLOR vec3 nl = min(.2 + tcolor.rgb * max(dot(wn, L), 0.), 1.);

			return CoronaColorScale(vec4(nl, 1.));
		}
	]]

	graphics.defineEffect(kernel)
end

-- Local includes
local max = math.max
local pi = math.pi
local random = math.random
local sqrt = math.sqrt

-- In-use quaternions --
local Quats = { {}, {}, {}, {} }

-- Calculates a new random quaternion
local function NewQuat (index)
	local quat = Quats[index]
	local x = 2 * (random() - .5)
	local y = 2 * (random() - .5) * sqrt(max(0, 1 - x^2))
	local z = (random() < .5 and -1 or 1) * sqrt(max(0, 1 - x^2 - y^2))
	local theta = (pi / 6 + random() * pi / 6) * (random() < .5 and -1 or 1)

	quaternion.FromAxisAngle(quat, theta, x, y, z)

	if index > 1 then
		quaternion.Multiply(quat, quat, Quats[index - 1])
	end
end

for i = 1, #Quats do
	NewQuat(i)
end

-- Transitioning quaternion time parameter --
local QuatTrans = { t = 0 }

-- Interpolation time transition --
local Params = { t = 1, iterations = -1, time = 750 }

function Params.onRepeat (qt)
	-- Evict the first quaternion. Move the remaining elements down.
	local q1 = Quats[1]

	for i = 2, 4 do
		Quats[i - 1] = Quats[i]
	end

	-- Select a new quaternion to replace the vacated final position.
	Quats[4] = q1

	NewQuat(4)

	-- Do this early, so the enterFrame handlers respect the new quaternions.
	qt.t = 0
end

transition.to(QuatTrans, Params)

do
	local slerp_circle = display.newCircle(display.contentCenterX, 70, 50)

	slerp_circle.fill = { type = "image", filename = "Image1.jpg" }

	slerp_circle.fill.effect = "filter.sphere.bumped"

	-- Slerp the light according to the current interpolation time.
	local Q, V = {}, {}

	Runtime:addEventListener("enterFrame", function()
		V.x, V.y, V.z, V.w = 0, 0, 1, 0

		quaternion.Slerp(Q, Quats[1], Quats[2], QuatTrans.t)

		Rotate(V, Q, "reuse")

		slerp_circle.fill.effect.light_x = V.x
		slerp_circle.fill.effect.light_y = V.y
		slerp_circle.fill.effect.light_z = V.z
	end)
end

do
	local squad_circle = display.newCircle(display.contentCenterX, 210, 50)

	squad_circle.fill = { type = "image", filename = "Image1.jpg" }

	squad_circle.fill.effect = "filter.sphere.bumped"

	-- Squad the light according to the current interpolation time.
	local Q, V = {}, {}

	Runtime:addEventListener("enterFrame", function()
		V.x, V.y, V.z, V.w = 0, 0, 1, 0

		quaternion.Squad(Q, Quats[1], Quats[2], Quats[3], Quats[4], QuatTrans.t)

		Rotate(V, Q, "reuse")

		squad_circle.fill.effect.light_x = V.x
		squad_circle.fill.effect.light_y = V.y
		squad_circle.fill.effect.light_z = V.z
	end)
end