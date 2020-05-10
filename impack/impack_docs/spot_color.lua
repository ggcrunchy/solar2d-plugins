--- Wrapper over [Spot](https://github.com/r-lyeh/spot)'s color class.
--
-- A color's values may be read or written through its **h**, **s**, **l**, and **a** properties.
--
-- **NOTE** Spot color support is still experimental.
--
-- ===============================================
-- @classmod SpotColor

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

--- Metamethod.
--
-- The right-hand argument may be a **SpotColor** or a number (implicitly, a color with four equal values).
--
-- Any errors are propagated.
-- @function SpotColor:__add
-- @treturn SpotColor The sum of the two colors.

--- Modify this color by adding another color to it.
-- @function SpotColor:add_mutate
-- @tparam SpotColor color Color to add.
-- @return[1] **true**, indicating success.
-- @return[2] **false**, meaning failure.
-- @treturn[2] string Error message.

--- Create a new color with this color's components, clamped to [0, 1].
-- @function SpotColor:clamp
-- @treturn[1] SpotColor New color.
-- @return[2] **nil**, meaning failure.
-- @treturn[2] string Error message.

--- Create a clone of this color.
-- @function SpotColor:clone
-- @treturn[1] SpotColor New color.
-- @return[2] **nil**, meaning failure.
-- @treturn[2] string Error message.

--- Metamethod.
--
-- The right-hand argument may be a **SpotColor** or a number (implicitly, a color with four equal values).
--
-- Any errors are propagated.
-- @function SpotColor:__div
-- @treturn SpotColor The quotient of the two colors.

--- Modify this color by dividing it by another color.
-- @function SpotColor:div_mutate
-- @tparam SpotColor color Color to divide by.
-- @return[1] **true**, indicating success.
-- @return[2] **false**, meaning failure.
-- @treturn[2] string Error message.

--- Metamethod.
--
-- The right-hand argument may be a **SpotColor** or a number (implicitly, a color with four equal values).
--
-- Any errors are propagated.
-- @function SpotColor:__mul
-- @treturn SpotColor The product of the two colors.

--- Modify this color by multiplying it by another color.
-- @function SpotColor:mul_mutate
-- @tparam SpotColor color Color to multiply by.
-- @return[1] **true**, indicating success.
-- @return[2] **false**, meaning failure.
-- @treturn[2] string Error message.

--- Create a new color with alpha premultiplied through this color's components.
-- @function SpotColor:premultiply
-- @treturn[1] SpotColor New color.
-- @return[2] **nil**, meaning failure.
-- @treturn[2] string Error message.

--- Metamethod.
--
-- The right-hand argument may be a **SpotColor** or a number (implicitly, a color with four equal values).
--
-- Any errors are propagated.
-- @function SpotColor:__sub
-- @treturn SpotColor The difference of the two colors.

--- Modify this color by subtracting another color from it.
-- @function SpotColor:sub_mutate
-- @tparam SpotColor color Color to subtract.
-- @return[1] **true**, indicating success.
-- @return[2] **false**, meaning failure.
-- @treturn[2] string Error message.

--- Create a new color with this color's components converted to RGBA values.
-- @function SpotColor:to_rgba
-- @treturn[1] SpotColor New color.
-- @return[2] **nil**, meaning failure.
-- @treturn[2] string Error message.

--- Create a new color with alpha unpremultiplied from this color's components.
-- @function SpotColor:unpremultiply
-- @treturn[1] SpotColor New color.
-- @return[2] **nil**, meaning failure.
-- @treturn[2] string Error message.