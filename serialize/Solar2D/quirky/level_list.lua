--- Quirky level list.

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

return function()
	return {
		-- Level 1 --
		{
			width = 5, height = 5,
			description = "Push blocks out of your way.\nBe careful not to block the exit!",

			EmptySpot,	Character,	EmptySpot,	EmptySpot,	EmptySpot,
			EmptySpot,	EmptySpot,	EmptySpot,	EmptySpot,	EmptySpot,
			Block(1),	Block(1),	Wall,		Block(2),	Block(2),
			EmptySpot,	EmptySpot,	EmptySpot,	EmptySpot,	EmptySpot,
			EmptySpot,	EmptySpot,	EmptySpot,	Exit,		EmptySpot
		},

		-- Level 2 --
		{
			width = 8, height = 5,
			description = "Push against turnstiles to rotate them.\nUse blocks to fill holes.",

			EmptySpot,	Character,	EmptySpot,		EmptySpot,		EmptySpot,	EmptySpot,	EmptySpot,	EmptySpot,
			EmptySpot,	EmptySpot,	Turnstile(1),	EmptySpot,		EmptySpot,	EmptySpot,	EmptySpot,	EmptySpot,
			Block(1),	EmptySpot,	Pivot(1),		Turnstile(1),	Block(2),	EmptySpot,	Hole,		EmptySpot,
			EmptySpot,	EmptySpot,	EmptySpot,		EmptySpot,		EmptySpot,	EmptySpot,	EmptySpot,	EmptySpot,
			EmptySpot,	EmptySpot,	EmptySpot,		Exit,			EmptySpot,	EmptySpot,	EmptySpot,	EmptySpot
		},

		-- Level 3 --
		{
			width = 5, height = 5,
			description = 'Cover up all the dots with "crates" to complete the level.',

			EmptySpot,	Character,	EmptySpot,	EmptySpot,	EmptySpot,
			EmptySpot,	EmptySpot,	EmptySpot,	EmptySpot,	EmptySpot,
			EmptySpot,	Block(1),	Wall,		Block(2),	EmptySpot,
			EmptySpot,	EmptySpot,	EmptySpot,	EmptySpot,	EmptySpot,
			EmptySpot,	EmptySpot,	BlockSpot,	BlockSpot,	EmptySpot
		},

		-- Level 4 --
		{
			width = 5, height = 5,
			description = "Get everybody to the exit!",

			EmptySpot,	Character,	EmptySpot,	EmptySpot,	EmptySpot,
			EmptySpot,	EmptySpot,	EmptySpot,	EmptySpot,	EmptySpot,
			Block(1),	Block(1),	Wall,		EmptySpot,	Character,
			EmptySpot,	EmptySpot,	EmptySpot,	EmptySpot,	EmptySpot,
			EmptySpot,	EmptySpot,	EmptySpot,	Exit,		EmptySpot
		},
--[[
		-- Level 5 --
		{
			description = "",
			--
		}
		]]
	}
end