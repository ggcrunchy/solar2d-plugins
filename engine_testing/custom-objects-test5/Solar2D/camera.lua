--- Mechanics for a camera that tracks the mouse.

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
local atan2 = math.atan2
local cos = math.cos
local max = math.max
local min = math.min
local pi = math.pi
local sin = math.sin
local sqrt = math.sqrt

-- Exports --
local M = {}

--
--
--

local Pos = { 0, 0, 0 }
local Dir, Side, Up = {}, {}, {}

--- DOCME
-- @tparam vec3 pos
-- @tparam vec3 dir
-- @tparam vec3 side
-- @tparam vec3 up
function M.GetVectors (pos, dir, side, up)
	for i = 1, 3 do
		pos[i] = Pos[i]
		dir[i] = Dir[i]
		side[i] = Side[i]
		up[i] = Up[i]
	end
end

--
--
--

local AngleHorz, AngleVert

--- DOCME
-- @tparam vec3 dir
-- @tparam vec3 up
function M.Init (dir, up)
	AngleHorz = atan2(dir[1], dir[3])
	AngleVert = atan2(sqrt(up[1]^2 + up[3]^2), up[2])
end

--
--
--

local function ClampIn (n, range)
	return min(max(n, -range), range)
end

local function CosSin (angle)
	return cos(angle), sin(angle)
end

local function AddScaledTo (out, a, b, k)
  for i = 1, 3 do
    out[i] = a[i] + b[i] * k
  end
end

local UpTemp = { 0, 0, 0 }

local function UpVector (dist)
	UpTemp[2] = dist

  return UpTemp
end

local HorzRange = pi / 6
local VertRange = pi * .375

local PlaneV = { 0, 0, 0 }

local function Normalize (v)
  local length = sqrt(v[1]^2 + v[2]^2 + v[3]^2)

  v[1], v[2], v[3] = v[1] / length, v[2] / length, v[3] / length
end

local function Cross (out, a, b)
   out[1], out[2], out[3] = a[2] * b[3] - a[3] * b[2], a[3] * b[1] - a[1] * b[3], a[1] * b[2] - a[2] * b[1]
end

--- DOCME
-- @number ddir
-- @number dside
-- @number dx
-- @number dy
function M.Update (ddir, dside, dx, dy)
	AngleHorz = AngleHorz + ClampIn(dx * .035, HorzRange)
	AngleVert = ClampIn(AngleVert + dy * .035, VertRange)

	local cosh, sinh = CosSin(AngleHorz)
	local cosv, sinv = CosSin(AngleVert)

	PlaneV[1], PlaneV[3] = sinh, cosh

	AddScaledTo(Dir, UpVector(sinv), PlaneV, cosv)
	AddScaledTo(Up, UpVector(cosv), PlaneV, -sinv)

	Normalize(Dir)
	Normalize(Up)
	Cross(Side, Up, Dir)

	AddScaledTo(Pos, Pos, Dir, ddir)
	AddScaledTo(Pos, Pos, Side, dside)
end

--
--
--

return M