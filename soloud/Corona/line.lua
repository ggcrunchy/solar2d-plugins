--- Some iterators over rectangular grid regions.

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
local assert = assert
local min = math.min
local remove = table.remove

-- Exports --
local M = {}

-- Default reclaim: no-op
local function NoReclaim () end

local function InstancedAutocacher (builder)
	local cache = {}

	return function(...)
		local instance = remove(cache)

		if not instance then
			local body, done, setup, reclaim = builder(...)
--[[
			assert(IsCallable(body), "Uncallable body function")
			assert(IsCallable(done), "Uncallable done function")
			assert(IsCallable(setup), "Uncallable setup function")
			assert(reclaim == nil or IsCallable(reclaim), "Uncallable reclaim function")
]]
			reclaim = reclaim or NoReclaim

			-- Build a reclaim function.
			local active

			local function reclaim_func (state)
				assert(active, "Iterator is not active")

				reclaim(state)

				cache[#cache + 1] = instance

				active = false
			end

			-- Iterator function
			local function iter (s, i)
				assert(active, "Iterator is done")

				if done(s, i) then
					reclaim_func(s)
				else
					return body(s, i)
				end
			end

			-- Iterator instance
			function instance (...)
				assert(not active, "Iterator is already in use")

				active = true

				local state, var0 = setup(...)

				return iter, state, var0, reclaim_func
			end
		end

		return instance(...)
	end
end

local floor = math.floor

local function DivRem (a, b)
	local quot = floor(a / b)

	return quot, a - quot * b
end

local function CellToIndex (col, row, w)
	return (row - 1) * w + col
end









-- Standard library imports --
local abs = math.abs
local max = math.max

-- Modules --
--[[
local divide = require("tektite_core.number.divide")
local grid_funcs = require("tektite_core.array.grid")
local iterator_utils = require("iterator_ops.utils")

-- Imports --
local CellToIndex = grid_funcs.CellToIndex
local DivRem = divide.DivRem
]]
-- Exports --
local M = {}

--- Iterator over a rectangular region on an array-based grid.
-- @function GridIter
-- @uint c1 Column index #1.
-- @uint r1 Row index #1.
-- @uint c2 Column index #2.
-- @uint r2 Row index #2.
-- @number dw Uniform cell width.
-- @number dh Uniform cell height.
-- @uint[opt=max(c1, c2)] ncols Number of columns in a grid row.
-- @treturn iterator Supplies the following, in order, at each iteration:
--
-- * Current iteration index.
-- * Array index, as per @{tektite_core.array.grid.CellToIndex}.
-- * Column index.
-- * Row index.
-- * Cell corner x-coordinate, 0 at _c_ = 1.
-- * Cell corner y-coordinate, 0 at _r_ = 1.
-- @see iterator_ops.utils.InstancedAutocacher

M.LineIter = --[[iterator_utils.]]InstancedAutocacher(function()
	local c1, c2, r1, r2

	--
	local rinc

	local function UpdateCol ()
		r1 = r1 + rinc

		return c1, r1
	end

	local function ColDone ()
		return r1 == r2
	end

	--
	local cinc

	local function UpdateRow ()
		c1 = c1 + cinc

		return c1, r1
	end

	local function RowDone ()
		return c1 == c2
	end

	--
	local x1, x2, y1, y2, xdim, ydim, xoff, yoff, xoffinc, rlast, slope

	local function Col (x)
		return floor((x - xoff) / xdim) + 1
	end

	local function Row (y)
		return floor((y - yoff) / ydim) + 1
	end
	
	local function UpdateDiag ()
		local c, r = c1, r1

		-- On each column, go from the current to the final row. The final row
		-- becomes current at the end of each pass.
		if c1 ~= c2 then
			rlast = rlast or Row(y1 + slope * xoff)

			if r1 == rlast then
				c1, xoff, rlast = c1 + cinc, xoff + xoffinc
			else
				r1 = r1 + rinc
			end

		-- Treat the final column separately, since the line could cover several
		-- more rows than desired while in this column.
		else
			r1 = r1 + rinc
		end

		return c, r
	end

	local function DiagDone ()
		return c1 == c2 and r1 == r2 + rinc
	end

	--
	local update, done

	-- Body --
	return function()
		return update()
	end,

	-- Done --
	function()
		return done()
	end,

	-- Setup --
	function(...)
		--
		local opts

		x1, y1, x2, y2, xdim, opts = ...
		
		if opts then
			ydim, xoff, yoff = opts.ydim, opts.xoff, opts.yoff
		else
			ydim, xoff, yoff = nil
		end

		ydim, xoff, yoff = ydim or xdim, xoff or 0, yoff or 0

		--
		c1, r1, c2, r2 = Col(x1), Row(y1), Col(x2), Row(y2)

		--
		if c1 == c2 then
			rinc = r1 < r2 and 1 or -1
			r1 = r1 - rinc
			update, done = UpdateCol, ColDone

		--
		elseif r1 == r2 then
			cinc = c1 < c2 and 1 or -1
			c1 = c1 - cinc
			update, done = UpdateRow, RowDone

		--
		else
			cinc, rinc, xoffinc, rlast = 1, 1, xdim
			slope, xoff = (y2 - y1) / (x2 - x1), xoff + c1 * xoffinc - x1
			update, done = UpdateDiag, DiagDone

			-- If the line tends left, adjust horizontal values.
			if c2 < c1 then
				cinc, xoff, xoffinc = -1, xoff - xoffinc, -xoffinc
			end

			-- If the line tends down, adjust vertical values.
			if r2 < r1 then
				rinc = -1
			end
		end
	end
end)

-- Export the module.
return M