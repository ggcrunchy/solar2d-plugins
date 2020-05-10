--- Chebyshev filter.

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
local min = math.min
local sqrt = math.sqrt

-- Exports --
local M = {}

-- Memoized factorials --
local Facts = { 1, 1 }

-- Factorial of n
local function Fact (n)
	local from = min(#Facts, n + 1)
	local fact = Facts[from]

	for i = from, n do
		fact = fact * i

		Facts[i + 1] = fact
	end

	return fact
end

-- Chebyshev polynomial of order n
local function T (coeffs, n, x)
	local sum, power_of_omx, omx = 0, 1, 1 - x

	for i = 1, #coeffs do
		sum, power_of_omx = sum + power_of_omx * coeffs[i], power_of_omx * omx
	end

	return n * sum
end

--- Creates a [Type II Chebyshev filter](https://en.wikipedia.org/wiki/Chebyshev_filter#Type_II_Chebyshev_filters).
-- @uint n Maximum order of Chebyshev polynomials.
-- @number[opt=.01] eps Ripple factor.
-- @treturn function Filter, callable as `y = filter(x)`, where _y_ rapidly approaches 0 as
-- _x_ approaches 1.
function M.NewFilter (n, eps)
	eps = eps or .01

	-- Precompute the Chebyshev polynomial coefficents.
	local coeffs, power_of_neg2 = {}, 1

	for k = 0, n do
		coeffs[#coeffs + 1], power_of_neg2 = power_of_neg2 * Fact(n + k - 1) / (Fact(n - k) * Fact(2 * k)), -2 * power_of_neg2
	end

	--
	return function(x)
		return 1 / sqrt(1 + 1 / (eps * T(coeffs, n, 1 / x))^2)
	end
end

-- Export the module.
return M