--- Pieces used to build a Quirky level.
--
-- Level format:
--
-- w = width, h = height, description = "Description text"
--
-- The remaining entries comprise an array of (ground, middle) pairs, with
-- these meanings:
--
-- * false = no entry
-- * "C" = character (middle layer)
-- * "E" = exit (ground layer)
-- * "F" = filled hole (ground layer)
-- * "H" = hole (ground layer)
-- * "S" = spot, for Sokoban mechanics (ground layer)
-- * "W" = wall (middle layer)
--
-- In the following, n is an integer:
--
-- * "Bn", part of block #n (middle layer)
-- * "Pn" , pivot of turnstile #n; must have one or more Tn (middle layer)
-- * "Tn", part of turnstile #n; must have corresponding Pn (middle layer)

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

-- Helper to build a ground-only entry
function M.Ground (entry)
	return { entry, false }
end

-- Helper to build a middle-only entry
function M.Middle (entry)
	return { false, entry }
end

-- Spots that are initially empty --
M.EmptySpot = M.Ground(false)

-- Spots that should be filled with a block, Sokoban-style --
M.BlockSpot = M.Ground"S"

-- Character starting position --
M.Character = M.Middle"C"

-- Spot containing an exit --
M.Exit = M.Ground"E"

-- Spot containing a hole initially --
M.Hole = M.Ground"H"

-- Spot occupied by a wall --
M.Wall = M.Middle"W"

-- Helper to add a block
function M.Block (id)
	return M.Middle("B" .. id)
end

-- Helper to add a pivot
function M.Pivot (id)
	return M.Middle("P" .. id)
end

-- Helper to add a turnstile part
function M.Turnstile (id)
	return M.Middle("T" .. id)
end

-- Export the module.
return M