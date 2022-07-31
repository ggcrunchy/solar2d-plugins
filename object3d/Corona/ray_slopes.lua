--- This module implements some axis-aligned bounding box / ray collision algorithms.

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

-- Notice in the original:

--[===========================================================================[
  This source code accompanies the Journal of Graphics Tools paper:


  "Fast Ray / Axis-Aligned Bounding Box Overlap Tests using Ray Slopes" 
  by Martin Eisemann, Thorsten Grosch, Stefan Müller and Marcus Magnor
  Computer Graphics Lab, TU Braunschweig, Germany and
  University of Koblenz-Landau, Germany


  Parts of this code are taken from
  "Fast Ray-Axis Aligned Bounding Box Overlap Tests With Pluecker Coordinates" 
  by Jeffrey Mahovsky and Brian Wyvill
  Department of Computer Science, University of Calgary


  This source code is public domain, but please mention us if you use it.
]===========================================================================]

-- Standard library imports --
local floor = math.floor
local max = math.max
local min = math.min
local sqrt = math.sqrt
local tonumber = tonumber

-- Plugins --
local ffi = require("plugin.ffi")
local bit = require("plugin.bit")

-- Imports --
local band = bit.band
local bor = bit.bor
local rshift = bit.rshift

-- Exports --
local M = {}

ffi.cdef[[
	typedef union {
		struct {
			double x0, x1, y0, y1, z0, z1;
		};
		struct {
			double xx[2], yy[2], zz[2];
		};
	} AABox_t;

	enum CLASSIFICATION	{ // Pattern + (group << 9): See comments below for grouping and derivation of patterns
		MMM = 0x1F8 + (1 << 9),
		MMP = 0x13C + (1 << 9),
		MPM = 0x1D2 + (1 << 9),
		MPP = 0x116 + (1 << 9),
		PMM = 0x0E9 + (1 << 9),
		PMP = 0x02D + (1 << 9),
		PPM = 0x0C3 + (1 << 9),
		PPP = 0x007 + (1 << 9),
		OMM = 0x070 + (2 << 9),
		OMP = 0x034 + (2 << 9),
		OPM = 0x052 + (2 << 9),
		OPP = 0x016 + (2 << 9),
		MOM = 0x030 + (3 << 9),
		MOP = 0x054 + (3 << 9),
		POM = 0x031 + (3 << 9),
		POP = 0x015 + (3 << 9),
		MMO = 0x070 + (4 << 9),
		MPO = 0x052 + (4 << 9),
		PMO = 0x031 + (4 << 9),
		PPO = 0x013 + (4 << 9),
		MOO = 0x050 + (5 << 9),
		POO = 0x051 + (5 << 9),
		OMO = 0x050 + (6 << 9),
		OPO = 0x052 + (6 << 9),
		OOM = 0x050 + (7 << 9),
		OOP = 0x054 + (7 << 9)
	};

	typedef struct {
		double x, y, z; // ray origin   
		double i, j, k; // ray direction        
		double ii, ij, ik; // inverses of direction components

		// ray slope
		enum CLASSIFICATION classification;
		double ibyj, jbyi, kbyj, jbyk, ibyk, kbyi; // slope
		double c_xy, c_xz, c_yx, c_yz, c_zx, c_zy;       
	} Ray_t;
]]

--- DOCME
-- @number x0
-- @number y0
-- @number z0
-- @number x1
-- @number y1
-- @number z1
-- @treturn AABox_t X
function M.MakeAABox (x0, y0, z0, x1, y1, z1)
	x0, x1 = min(x0, x1), max(x0, x1)
	y0, y1 = min(y0, y1), max(y0, y1)
	z0, z1 = min(z0, z1), max(z0, z1)

	return ffi.new("AABox_t", x0, x1, y0, y1, z0, z1)
end

--
local function Name (v)
	if v < 0 then
		return "M"
	elseif v > 0 then
		return "P"
	else
		return "O"
	end
end

--- DOCME
-- @number x
-- @number y
-- @number z
-- @number i
-- @number j
-- @number k
-- @treturn Ray_t X
function M.MakeRay (x, y, z, i, j, k)
	local ray = ffi.new("Ray_t", x, y, z, i, j, k, 1 / i, 1 / j, 1 / k)

	-- ray slope
	ray.ibyj = i * ray.ij
	ray.jbyi = j * ray.ii
	ray.jbyk = j * ray.ik
	ray.kbyj = k * ray.ij
	ray.ibyk = i * ray.ik
	ray.kbyi = k * ray.ii
	ray.c_xy = y - ray.jbyi * x
	ray.c_xz = z - ray.kbyi * x
	ray.c_yx = x - ray.ibyj * y
	ray.c_yz = z - ray.kbyj * y
	ray.c_zx = x - ray.ibyk * z
	ray.c_zy = y - ray.jbyk * z

	-- ray slope classification
	-- TODO: Is there a more elegant way?
	ray.classification = Name(i) .. Name(j) .. Name(k)
--[[
	if i < 0 then
		if j < 0 then
			if k < 0 then
				ray.classification = "MMM"
			elseif k > 0 then
				ray.classification = "MMP"
			else
				ray.classification = "MMO"
			end
		else -- j >= 0
			if k < 0 then
				ray.classification = j == 0 and "MOM" or "MPM"
			else -- k >= 0
				if j == 0 and k == 0 then
					ray.classification = "MOO"
				elseif k == 0 then
					ray.classification = "MPO"
				elseif j == 0 then
					ray.classification = "MOP"
				else
					ray.classification = "MPP"
				end
			end
		end
	else -- i >= 0
		if j < 0 then
			if k < 0 then
				ray.classification = i == 0 and "OMM" or "PMM"
			else -- k >= 0
				if i == 0 and k == 0 then
					ray.classification = "OMO"
				elseif k == 0 then
					ray.classification = "PMO"
				elseif i == 0 then
					ray.classification = "OMP"
				else
					ray.classification = "PMP"
				end
			end
		else -- j >= 0
			if k < 0 then
				if i == 0 and j == 0 then
					ray.classification = "OOM"
				elseif i == 0 then
					ray.classification = "OPM"
				elseif j == 0 then
					ray.classification = "POM"
				else
					ray.classification = "PPM"
				end
			else -- k >= 0
				if i == 0 then
					if j == 0 then
						ray.classification = "OOP"
					elseif k == 0 then
						ray.classification = "OPO"
					else
						ray.classification = "OPP"
					end
				else
					if j == 0 and k == 0 then
						ray.classification = "POO"
                    elseif j == 0 then
						ray.classification = "POP"
					elseif k == 0 then
						ray.classification = "PPO"
					else
						ray.classification = "PPP"
					end
				end
			end
		end
	end
]]
	return ray
end

--- DOCME
-- @number x1
-- @number y1
-- @number z1
-- @number x2
-- @number y2
-- @number z2
-- @treturn Ray_t X
function M.MakeRayTo (x1, y1, z1, x2, y2, z2)
	local dx, dy, dz = x2 - x1, y2 - y1, z2 - z1
	local len = sqrt(dx^2 + dy^2 + dz^2)

	return M.MakeRay(x1, y1, z1, dx / len, dy / len, dz / len)
end

--[=[
	-- XYZ (group 1):
	-- There is a general pattern to each of these cases, where all three components are
	-- taken into consideration. In the first line, an x, y, and z value are each tested
	-- to see if the ray is already "in front of" the box. Each value will either be a "0"
	-- (x0, y0, z0) or a "1" (x1, y1, z1) component.

	-- Whichever values were chosen will recur in certain places in the expressions that
	-- remain, as will the unchosen values. Thus, there is structure on which to abstract
	-- away the "encode" and "get time" operations. (This holds likewise for groups 2-7.)

	-- All of the expressions are inequalities, A < B or A > B. The test is all or nothing:
	-- EVERY expression must fail*. This suggests aggregating the results into a bit pattern
	-- that can be quickly compared against a reference pattern.

	-- Since the x, y, z indices are already on hand, they may as well be extracted from the
	-- reference pattern. Conversely, each failure must produce that same index bit, in order
	-- to reconstruct its part of the pattern.

	-- An examination of the first line shows that each "0" value belongs to an "A < value"
	-- expression, whereas "1" values belong to "A > value". Turning this around, A < B must
	-- yield 0 on failure, and 1 for A > B. This policy is generalized to all expressions**.

	-- Given all of this, the various constants are derived in the following. Note that bits
	-- are enumerated last-to-first; the final binary string (before its hex equivalent) is
	-- in correct order.

	-- * - Fail the rejection test, that is.

	-- ** - CONSIDER: As an implementation detail, the 0 or 1 corresponds to the sign bit of
	-- A - B... Are the A = B situations correct / acceptable, then?

    if ray.classification == "MMM" then -- 111 111 000 -> 0x1F8
		if ray.x < box.x0 or ray.y < box.y0 or ray.z < box.z0 or -- 000
			ray.jbyi * box.x0 - box.y1 + ray.c_xy > 0 or -- 1
			ray.ibyj * box.y0 - box.x1 + ray.c_yx > 0 or -- 1
			ray.jbyk * box.z0 - box.y1 + ray.c_zy > 0 or -- 1
			ray.kbyj * box.y0 - box.z1 + ray.c_yz > 0 or -- 1
			ray.kbyi * box.x0 - box.z1 + ray.c_xz > 0 or -- 1
			ray.ibyk * box.z0 - box.x1 + ray.c_zx > 0 then -- 1
			return false
		end

		t = max((box.x1 - ray.x) * ray.ii, (box.y1 - ray.y) * ray.ij, (box.z1 - ray.z) * ray.ik)

	elseif ray.classification == "MMP" then -- 100 111 100 -> 0x13C
		if ray.x < box.x0 or ray.y < box.y0 or ray.z > box.z1 or -- 001
			ray.jbyi * box.x0 - box.y1 + ray.c_xy > 0 or -- 1
			ray.ibyj * box.y0 - box.x1 + ray.c_yx > 0 or -- 1
			ray.jbyk * box.z1 - box.y1 + ray.c_zy > 0 or -- 1
			ray.kbyj * box.y0 - box.z0 + ray.c_yz < 0 or -- 0
			ray.kbyi * box.x0 - box.z0 + ray.c_xz < 0 or -- 0
			ray.ibyk * box.z1 - box.x1 + ray.c_zx > 0 then -- 1
			return false
		end

		t = max((box.x1 - ray.x) * ray.ii, (box.y1 - ray.y) * ray.ij, (box.z0 - ray.z) * ray.ik)

	elseif ray.classification == "MPM" then -- 111 010 010 -> 0x1D2
		if ray.x < box.x0 or ray.y > box.y1 or ray.z < box.z0 or -- 010
			ray.jbyi * box.x0 - box.y0 + ray.c_xy < 0 or -- 0
			ray.ibyj * box.y1 - box.x1 + ray.c_yx > 0 or -- 1
			ray.jbyk * box.z0 - box.y0 + ray.c_zy < 0 or -- 0
			ray.kbyj * box.y1 - box.z1 + ray.c_yz > 0 or -- 1
			ray.kbyi * box.x0 - box.z1 + ray.c_xz > 0 or -- 1
			ray.ibyk * box.z0 - box.x1 + ray.c_zx > 0 then -- 1
			return false
		end

		t = max((box.x1 - ray.x) * ray.ii, (box.y0 - ray.y) * ray.ij, (box.z1 - ray.z) * ray.ik)

	elseif ray.classification == "MPP" then -- 100 010 110 -> 0x116
		if ray.x < box.x0 or ray.y > box.y1 or ray.z > box.z1 or -- 011
			ray.jbyi * box.x0 - box.y0 + ray.c_xy < 0 or -- 0
			ray.ibyj * box.y1 - box.x1 + ray.c_yx > 0 or -- 1
			ray.jbyk * box.z1 - box.y0 + ray.c_zy < 0 or -- 0
			ray.kbyj * box.y1 - box.z0 + ray.c_yz < 0 or -- 0
			ray.kbyi * box.x0 - box.z0 + ray.c_xz < 0 or -- 0
			ray.ibyk * box.z1 - box.x1 + ray.c_zx > 0 then -- 1
			return false
		end

		t = max((box.x1 - ray.x) * ray.ii, (box.y0 - ray.y) * ray.ij, (box.z0 - ray.z) * ray.ik)

	elseif ray.classification == "PMM" then -- 011 101 001 -> 0x0E9
		if ray.x > box.x1 or ray.y < box.y0 or ray.z < box.z0 or -- 100
			ray.jbyi * box.x1 - box.y1 + ray.c_xy > 0 or -- 1
			ray.ibyj * box.y0 - box.x0 + ray.c_yx < 0 or -- 0
			ray.jbyk * box.z0 - box.y1 + ray.c_zy > 0 or -- 1
			ray.kbyj * box.y0 - box.z1 + ray.c_yz > 0 or -- 1
			ray.kbyi * box.x1 - box.z1 + ray.c_xz > 0 or -- 1
			ray.ibyk * box.z0 - box.x0 + ray.c_zx < 0 then -- 0
			return false
		end

		t = max((box.x0 - ray.x) * ray.ii, (box.y1 - ray.y) * ray.ij, (box.z1 - ray.z) * ray.ik)

	elseif ray.classification == "PMP" then -- 000 101 101 -> 0x02D
		if ray.x > box.x1 or ray.y < box.y0 or ray.z > box.z1 or -- 101
			ray.jbyi * box.x1 - box.y1 + ray.c_xy > 0 or -- 1
			ray.ibyj * box.y0 - box.x0 + ray.c_yx < 0 or -- 0
			ray.jbyk * box.z1 - box.y1 + ray.c_zy > 0 or -- 1
			ray.kbyj * box.y0 - box.z0 + ray.c_yz < 0 or -- 0
			ray.kbyi * box.x1 - box.z0 + ray.c_xz < 0 or -- 0
			ray.ibyk * box.z1 - box.x0 + ray.c_zx < 0 then -- 0
			return false
		end

		t = max((box.x0 - ray.x) * ray.ii, (box.y1 - ray.y) * ray.ij, (box.z0 - ray.z) * ray.ik)

	elseif ray.classification == "PPM" then -- 011 000 011 -> 0x0C3
		if ray.x > box.x1 or ray.y > box.y1 or ray.z < box.z0 or -- 110
			ray.jbyi * box.x1 - box.y0 + ray.c_xy < 0 or -- 0
			ray.ibyj * box.y1 - box.x0 + ray.c_yx < 0 or -- 0
			ray.jbyk * box.z0 - box.y0 + ray.c_zy < 0 or -- 0
			ray.kbyj * box.y1 - box.z1 + ray.c_yz > 0 or -- 1
			ray.kbyi * box.x1 - box.z1 + ray.c_xz > 0 or -- 1
			ray.ibyk * box.z0 - box.x0 + ray.c_zx < 0 then -- 0
			return false
		end

		t = max((box.x0 - ray.x) * ray.ii, (box.y0 - ray.y) * ray.ij, (box.z1 - ray.z) * ray.ik)

	elseif ray.classification == "PPP" then -- 000 000 111 -> 0x007
		if ray.x > box.x1 or ray.y > box.y1 or ray.z > box.z1 or -- 111
			ray.jbyi * box.x1 - box.y0 + ray.c_xy < 0 or -- 0
			ray.ibyj * box.y1 - box.x0 + ray.c_yx < 0 or -- 0
			ray.jbyk * box.z1 - box.y0 + ray.c_zy < 0 or -- 0
			ray.kbyj * box.y1 - box.z0 + ray.c_yz < 0 or -- 0
			ray.kbyi * box.x1 - box.z0 + ray.c_xz < 0 or -- 0
			ray.ibyk * box.z1 - box.x0 + ray.c_zx < 0 then -- 0
			return false
		end

		t = max((box.x0 - ray.x) * ray.ii, (box.y0 - ray.y) * ray.ij, (box.z0 - ray.z) * ray.ik)

	-- YZ (group 2):
	-- Always assume x0 <= x <= x1, thus x index is implicitly 0 and there is a 01 sign mask
	-- Thus, only the ?'s are to be filled: ? ?10 ??0

	elseif ray.classification == "OMM" then -- 1 110 000 -> 0x070
		if ray.x < box.x0 or ray.x > box.x1 or
			ray.y < box.y0 or ray.z < box.z0 or -- _00
			ray.jbyk * box.z0 - box.y1 + ray.c_zy > 0 or -- 1
			ray.kbyj * box.y0 - box.z1 + ray.c_yz > 0 then -- 1
			return false
		end

		t = max((box.y1 - ray.y) * ray.ij, (box.z1 - ray.z) * ray.ik)

	elseif ray.classification == "OMP" then -- 0 110 100 -> 0x034
		if ray.x < box.x0 or ray.x > box.x1 or
			ray.y < box.y0 or ray.z > box.z1 or -- 001
			ray.jbyk * box.z1 - box.y1 + ray.c_zy > 0 or -- 1
			ray.kbyj * box.y0 - box.z0 + ray.c_yz < 0 then -- 0
			return false
		end

		t = max((box.y1 - ray.y) * ray.ij, (box.z0 - ray.z) * ray.ik)

	elseif ray.classification == "OPM" then -- 1 010 010 -> 0x052
		if ray.x < box.x0 or ray.x > box.x1 or
			ray.y > box.y1 or ray.z < box.z0 or -- 010
			ray.jbyk * box.z0 - box.y0 + ray.c_zy < 0 or -- 0
			ray.kbyj * box.y1 - box.z1 + ray.c_yz > 0 then -- 1
			return false
		end

		t = max((box.y0 - ray.y) * ray.ij, (box.z1 - ray.z) * ray.ik)

	elseif ray.classification == "OPP" then -- 0 010 110 -> 0x016
		if ray.x < box.x0 or ray.x > box.x1 or
			ray.y > box.y1 or ray.z > box.z1 or -- 011
			ray.jbyk * box.z1 - box.y0 + ray.c_zy < 0 or -- 0
			ray.kbyj * box.y1 - box.z0 + ray.c_yz < 0 then -- 0
			return false
		end

		t = max((box.y0 - ray.y) * ray.ij, (box.z0 - ray.z) * ray.ik)

	-- XZ (group 3):
	-- As per YZ, ? ?10 ?0?

	elseif ray.classification == "MOM" then -- 1 110 000 -> 0x070
		if ray.y < box.y0 or ray.y > box.y1 or
			ray.x < box.x0 or ray.z < box.z0 or -- 000
			ray.kbyi * box.x0 - box.z1 + ray.c_xz > 0 or -- 1
			ray.ibyk * box.z0 - box.x1 + ray.c_zx > 0 then -- 1
			return false
		end

		t = max((box.x1 - ray.x) * ray.ii, (box.z1 - ray.z) * ray.ik)

	elseif ray.classification == "MOP" then -- 1 010 100 -> 0x054
		if ray.y < box.y0 or ray.y > box.y1 or
			ray.x < box.x0 or ray.z > box.z1 or -- 001
			ray.kbyi * box.x0 - box.z0 + ray.c_xz < 0 or -- 0
			ray.ibyk * box.z1 - box.x1 + ray.c_zx > 0 then -- 1
			return false
		end

		t = max((box.x1 - ray.x) * ray.ii, (box.z0 - ray.z) * ray.ik)

	elseif ray.classification == "POM" then -- 0 110 001 -> 0x031
		if ray.y < box.y0 or ray.y > box.y1 or
			ray.x > box.x1 or ray.z < box.z0 or -- 100
			ray.kbyi * box.x1 - box.z1 + ray.c_xz > 0 or -- 1
			ray.ibyk * box.z0 - box.x0 + ray.c_zx < 0 then -- 0
			return false
		end

		t = max((box.x0 - ray.x) * ray.ii, (box.z1 - ray.z) * ray.ik)

	elseif ray.classification == "POP" then -- 0 010 101 -> 0x015
		if ray.y < box.y0 or ray.y > box.y1 or
			ray.x > box.x1 or ray.z > box.z1 or -- 101
			ray.kbyi * box.x1 - box.z0 + ray.c_xz < 0 or -- 0
			ray.ibyk * box.z1 - box.x0 + ray.c_zx < 0 then -- 0
			return false
		end

		t = max((box.x0 - ray.x) * ray.ii, (box.z0 - ray.z) * ray.ik)

	-- XY (group 4):
	-- As per YZ, ? ?10 0??

	elseif ray.classification == "MMO" then -- 1 110 000 -> 0x070
		if ray.z < box.z0 or ray.z > box.z1 or
			ray.x < box.x0 or ray.y < box.y0 or -- 000
			ray.jbyi * box.x0 - box.y1 + ray.c_xy > 0 or -- 1
			ray.ibyj * box.y0 - box.x1 + ray.c_yx > 0 then -- 1
			return false
		end

		t = max((box.x1 - ray.x) * ray.ii, (box.y1 - ray.y) * ray.ij)

	elseif ray.classification == "MPO" then -- 1 010 010 -> 0x052
		if ray.z < box.z0 or ray.z > box.z1 or
			ray.x < box.x0 or ray.y > box.y1 or -- 010
			ray.jbyi * box.x0 - box.y0 + ray.c_xy < 0 or -- 0
			ray.ibyj * box.y1 - box.x1 + ray.c_yx > 0 then -- 1
			return false
		end

		t = max((box.x1 - ray.x) * ray.ii, (box.y0 - ray.y) * ray.ij)

	elseif ray.classification == "PMO" then -- 0 110 001 -> 0x031
		if ray.z < box.z0 or ray.z > box.z1 or
			ray.x > box.x1 or ray.y < box.y0 or -- 100
			ray.jbyi * box.x1 - box.y1 + ray.c_xy > 0 or -- 1
			ray.ibyj * box.y0 - box.x0 + ray.c_yx < 0 then -- 0
			return false
		end

		t = max((box.x0 - ray.x) * ray.ii, (box.y1 - ray.y) * ray.ij)

	elseif ray.classification == "PPO" then -- 0 010 011 -> 0x013
		if ray.z < box.z0 or ray.z > box.z1 or
			ray.x > box.x1 or ray.y > box.y1 or -- 110
			ray.jbyi * box.x1 - box.y0 + ray.c_xy < 0 or -- 0
			ray.ibyj * box.y1 - box.x0 + ray.c_yx < 0 then -- 0
			return false
		end

		t = max((box.x0 - ray.x) * ray.ii, (box.y0 - ray.y) * ray.ij)

	-- X (group 5):
	-- As with XZ et al. here y0 <= y <= y1 and z0 <= z <= z1, thus leading to implicit 0 indices and two 01 masks
	-- Thus, only the ? needs to be filled: 1 010 00?

	elseif ray.classification == "MOO" then -- 1 010 000 -> 0x050
		if ray.x < box.x0 or -- 000
			ray.y < box.y0 or ray.y > box.y1 or
			ray.z < box.z0 or ray.z > box.z1 then
			return false
		end

		t = (box.x1 - ray.x) * ray.ii

	elseif ray.classification == "POO" then -- 1 010 001 -> 0x051
		if ray.x > box.x1 or -- 100
			ray.y < box.y0 or ray.y > box.y1 or
			ray.z < box.z0 or ray.z > box.z1 then
			return false
		end

		t = (box.x0 - ray.x) * ray.ii

	-- Y (group 6):
	-- As per X, 1 010 0?0

	elseif ray.classification == "OMO" then -- 1 010 000 -> 0x050
		if ray.y < box.y0 or -- 000
			ray.x < box.x0 or ray.x > box.x1 or
			ray.z < box.z0 or ray.z > box.z1 then
			return false
		end

		t = (box.y1 - ray.y) * ray.ij

	elseif ray.classification == "OPO" then -- 1 010 010 -> 0x052
		if ray.y > box.y1 or -- 010
			ray.x < box.x0 or ray.x > box.x1 or
			ray.z < box.z0 or ray.z > box.z1 then
            return false
		end

		t = (box.y0 - ray.y) * ray.ij

	-- Z (group 7):
	-- As per X, 1 010 ?00

	elseif ray.classification == "OOM" then -- 1 010 000 -> 0x050
		if ray.z < box.z0 or -- 000
			ray.x < box.x0 or ray.x > box.x1 or
			ray.y < box.y0 or ray.y > box.y1 then
            return false
		end

		t = (box.z1 - ray.z) * ray.ik

	elseif ray.classification == "OOP" then -- 1 010 100 -> 0x054
		if ray.z > box.z1 or -- 001
			ray.x < box.x0 or ray.x > box.x1 or
			ray.y < box.y0 or ray.y > box.y1 then
			return false
		end

		t = (box.z0 - ray.z) * ray.ik
	end

	return true, t
]=]

-- Per-group methods for encoding sign information into packets for testing --
local Encode = {
	-- 1: XYZ --
	function(ray, box, indices) -- indices = ? ? ?
		local ix = band(indices, 0x1)
		local iy = band(rshift(indices, 1), 0x1)
		local iz = rshift(indices, 2)

		local x1 = box.xx[ix]
		local y1 = box.yy[iy]
		local z1 = box.zz[iz]

		local c1 = rshift(floor(ray.x - x1), 31)
		local c2 = band(rshift(floor(ray.y - y1), 30), 0x2)
		local c3 = band(rshift(floor(ray.z - z1), 29), 0x4)
		
		local x2 = box.xx[1 - ix]
		local y2 = box.yy[1 - iy]
		local z2 = box.zz[1 - iz]
		
		local c4 = band(rshift(floor(ray.jbyi * x1 - y2 + ray.c_xy), 28), 0x008)
		local c5 = band(rshift(floor(ray.ibyj * y1 - x2 + ray.c_yx), 27), 0x010)
		local c6 = band(rshift(floor(ray.jbyk * z1 - y2 + ray.c_zy), 26), 0x020)
		local c7 = band(rshift(floor(ray.kbyj * y1 - z2 + ray.c_yz), 25), 0x040)
		local c8 = band(rshift(floor(ray.kbyi * x1 - z2 + ray.c_xz), 24), 0x080)
		local c9 = band(rshift(floor(ray.ibyk * z1 - x2 + ray.c_zx), 23), 0x100)

		return bor(c1, c2, c3, c4, c5, c6, c7, c8, c9)
	end,

	-- 2: YZ --
	function(ray, box, indices) -- indices = 0 ? ?
		local iy = band(rshift(indices, 1), 0x1)
		local iz = rshift(indices, 2)

		local y1 = box.yy[iy]
		local z1 = box.zz[iz]

		-- X = 0: bit 31 (mask 0x01) always 0
		local c1 = band(rshift(floor(ray.y - y1), 30), 0x02)
		local c2 = band(rshift(floor(ray.z - z1), 29), 0x04)
		local c3 = band(rshift(floor(ray.x - box.xx[0]), 28), 0x08)
		local c4 = band(rshift(floor(ray.x - box.xx[1]), 27), 0x10)

		local y2 = box.yy[1 - iy]
		local z2 = box.zz[1 - iz]

		local c5 = band(rshift(floor(ray.jbyk * z1 - y2 * ray.c_zy), 26), 0x20)
		local c6 = band(rshift(floor(ray.kbyj * y1 - z2 * ray.c_yz), 25), 0x40)

		return bor(c1, c2, c3, c4, c5, c6)
	end,

	-- 3: XZ --
	function(ray, box, indices) -- indices = ? 0 ?
		local ix = band(indices, 0x1)
		local iz = rshift(indices, 2)

		local x1 = box.xx[ix]
		local z1 = box.zz[iz]

		local c1 = rshift(floor(ray.x - x1), 31)
		-- Y = 0: bit 30 (mask 0x02) always 0
		local c2 = band(rshift(floor(ray.z - z1), 29), 0x04)
		local c3 = band(rshift(floor(ray.y - box.yy[0]), 28), 0x08)
		local c4 = band(rshift(floor(ray.y - box.yy[1]), 27), 0x10)

		local x2 = box.xx[1 - ix]
		local z2 = box.zz[1 - iz]

		local c5 = band(rshift(floor(ray.kbyi * z1 - x2 * ray.c_xz), 26), 0x20)
		local c6 = band(rshift(floor(ray.ibyk * x1 - z2 * ray.c_zx), 25), 0x40)

		return bor(c1, c2, c3, c4, c5, c6)
	end,

	-- 4: XY --
	function(ray, box, indices) -- indices = ? ? 0
		local ix = band(indices, 0x1)
		local iy = rshift(indices, 1)

		local x1 = box.xx[ix]
		local y1 = box.yy[iy]

		local c1 = rshift(floor(ray.x - x1), 31)
		local c2 = band(rshift(floor(ray.y - y1), 30), 0x02)
		-- Z = 0: bit 29 (mask 0x04) always 0
		local c3 = band(rshift(floor(ray.z - box.zz[0]), 28), 0x08)
		local c4 = band(rshift(floor(ray.z - box.zz[1]), 27), 0x10)

		local x2 = box.xx[1 - ix]
		local y2 = box.yy[1 - iy]

		local c5 = band(rshift(floor(ray.jbyi * x1 - y2 * ray.c_xy), 26), 0x20)
		local c6 = band(rshift(floor(ray.ibyj * y1 - x2 * ray.c_yx), 25), 0x40)

		return bor(c1, c2, c3, c4, c5, c6)
	end,

	-- 5: X --
	function(ray, box, indices) -- indices = ? 0 0
		local ix = band(indices, 0x1)

		local c1 = rshift(floor(ray.x - box.xx[ix]), 31)
		-- Y = 0: bit 30 (mask 0x02) always 0
		-- Z = 0: bit 29 (mask 0x04) always 0
		local c2 = band(rshift(floor(ray.y - box.yy[0]), 28), 0x08)
		local c3 = band(rshift(floor(ray.y - box.yy[1]), 27), 0x10)
		local c4 = band(rshift(floor(ray.z - box.zz[0]), 26), 0x20)
		local c5 = band(rshift(floor(ray.z - box.zz[1]), 25), 0x40)

		return bor(c1, c2, c3, c4, c5)
	end,

	-- 6: Y --
	function(ray, box, indices) -- indices = 0 ? 0
		local iy = rshift(indices, 1)

		-- X = 0: bit 31 (mask 0x01) always 0
		local c1 = band(rshift(floor(ray.y - box.yy[iy]), 30), 0x02)
		-- Z = 0: bit 29 (mask 0x04) always 0
		local c2 = band(rshift(floor(ray.x - box.xx[0]), 28), 0x08)
		local c3 = band(rshift(floor(ray.x - box.xx[1]), 27), 0x10)
		local c4 = band(rshift(floor(ray.z - box.zz[0]), 26), 0x20)
		local c5 = band(rshift(floor(ray.z - box.zz[1]), 25), 0x40)

		return bor(c1, c2, c3, c4, c5)
	end,

	-- 7: Z --
	function(ray, box, indices) -- indices = 0 0 ?
		local iz = rshift(indices, 2)

		-- X = 0: bit 31 (mask 0x01) always 0
		-- Y = 0: bit 30 (mask 0x02) always 0
		local c1 = band(rshift(floor(ray.z - box.zz[iz]), 29), 0x04)
		local c2 = band(rshift(floor(ray.x - box.xx[0]), 28), 0x08)
		local c3 = band(rshift(floor(ray.x - box.xx[1]), 27), 0x10)
		local c4 = band(rshift(floor(ray.y - box.yy[0]), 26), 0x20)
		local c5 = band(rshift(floor(ray.y - box.yy[1]), 25), 0x40)

		return bor(c1, c2, c3, c4, c5)
	end
}

--- DOCME
-- @tparam Ray_t ray
-- @tparam AABox_t box
-- @treturn boolean X
function M.Slope (ray, box)
	local pattern = tonumber(ray.classification) -- Bits 0-2: indices; 3-8: non-index part of magic number; 9-11: group
	local packet = Encode[rshift(pattern, 9)](ray, box, band(pattern, 0x7))

	return packet == band(pattern, 0x1FF)
end

-- Per-group methods for getting the (earliest) time, once intersection has been confirmed --
local GetTime = {
	-- 1: XYZ --
	function(ray, box, indices) -- indices = ? ? ?
		local ix = band(indices, 0x1)
		local iy = band(rshift(indices, 1), 0x1)
		local iz = rshift(indices, 2)

		return max((box.xx[1 - ix] - ray.x) * ray.ii, (box.yy[1 - iy] - ray.y) * ray.ij, (box.zz[1 - iz] - ray.z) * ray.ik)
	end,

	-- 2: YZ --
	function(ray, box, indices) -- indices = 0 ? ?
		local iy = band(rshift(indices, 1), 0x1)
		local iz = rshift(indices, 2)

		return max((box.yy[1 - iy] - ray.y) * ray.ij, (box.zz[1 - iz] - ray.z) * ray.ik)
	end,

	-- 3: XZ --
	function(ray, box, indices) -- indices = ? 0 ?
		local ix = band(indices, 0x1)
		local iz = rshift(indices, 2)

		return max((box.xx[1 - ix] - ray.x) * ray.ii, (box.zz[1 - iz] - ray.z) * ray.ik)
	end,

	-- 4: XY --
	function(ray, box, indices) -- indices = ? ? 0
		local ix = band(indices, 0x1)
		local iy = rshift(indices, 1)

		return max((box.xx[1 - ix] - ray.x) * ray.ii, (box.yy[1 - iy] - ray.y) * ray.ij)
	end,

	-- 5: X --
	function(ray, box, indices) -- indices = ? 0 0
		local ix = band(indices, 0x1)

		return (box.xx[1 - ix] - ray.x) * ray.ii
	end,

	-- 6: Y --
	function(ray, box, indices) -- indices = 0 ? 0
		local iy = rshift(indices, 1)

		return (box.yy[1 - iy] - ray.y) * ray.ij
	end,

	-- 7: Z --
	function(ray, box, indices) -- indices = 0 0 ?
		local iz = rshift(indices, 2)

		return (box.zz[1 - iz] - ray.z) * ray.ik
	end
}

--- DOCME
-- @tparam Ray_t ray
-- @tparam AABox_t box
-- @treturn boolean X
-- @treturn number Y
function M.SlopeInt (ray, box)
	local pattern = tonumber(ray.classification) -- Bits 0-2: indices; 3-8: non-index part of magic number; 9-11: group
	local index = rshift(pattern, 9)
	local packet = Encode[index](ray, box, band(pattern, 0x7))

	if packet ~= band(pattern, 0x1FF) then
		return false
	else
		return true, GetTime[index](ray, box, band(pattern, 0x7))
	end
end

-- Export the module.
return M