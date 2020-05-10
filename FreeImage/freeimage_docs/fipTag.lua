--- Corona wrapper for **fipTag**.
--
-- FreeImage uses this structure to store metadata information.
-- @module fipTag

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

--- Copies one tag to another.
-- @function fipTag:assign
-- @tparam fipTag tag Tag to assign.

--- Creates a new tag that is a copy of another.
-- @function fipTag:clone
-- @tparam fipTag tag Tag to clone.
-- @treturn fipTag Clone.

--- Returns the number of components in the tag (in tag type units).
-- @function fipTag:getCount
-- @treturn uint Count.

--- Returns the tag description if available.
-- @function fipTag:getDescription
-- @treturn ?|string|nil Description.

--- Returns the tag ID if available, returns 0 otherwise.
-- @function fipTag:getID
-- @treturn uint ID.

--- Returns the tag field name (unique inside a metadata model).
-- @function fipTag:getKey
-- @treturn string Key.

--- Returns the length of the tag value in bytes.
-- @function fipTag:getLength
-- @treturn uint Length.

--- Returns the tag data type.
-- @function fipTag:getType
-- @treturn string Type, cf. @{enums.FREE_IMAGE_MDTYPE}.

--- Returns the tag value.
-- @function fipTag:getValue
-- @treturn userdata Value. (TODO!!!!! something more robust for Lua)

--- Indicates whether the tag is valid for use.
-- @function fipTag:isValid
-- @treturn boolean Tag is allocated?

--- Set the number of data in the tag.
-- @function fipTag:setCount
-- @uint count Count to assign.
-- @treturn boolean Assignment succeeded?

--- Set the (usually optional) tag description.
-- @function fipTag:setDescription
-- @string description Description to assign.
-- @treturn boolean Assignment succeeded?

--- Set the (usually optional) tag ID.
-- @function fipTag:setID
-- @uint id ID to assign.
-- @treturn boolean Assignment succeeded?

--- Set the tag field name.
-- @function fipTag:setKey
-- @string key Key to assign.
-- @treturn boolean Assignment succeeded?

--- Construct a FIDT_ASCII tag (ASCII string).
--
-- This method is useful to store comments or IPTC tags.
-- @function fipTag:setKeyValue
-- @string name Field name.
-- @string value Field value.
-- @treturn boolean Assignment succeeded?

--- Set the length of the tag value, in bytes.
-- @function fipTag:setLength
-- @uint length Length to assign.
-- @treturn boolean Assignment succeeded?

--- Set the tag data type.
-- @function fipTag:setType
-- @string type Type to assign, cf. @{enums.FREE_IMAGE_MDTYPE}.
-- @treturn boolean Assignment succeeded?

--- Set the tag value.
-- @function fipTag:setValue
-- @param value Value to assign. (TODO: something more robust for Lua!!!!)
-- @treturn boolean Assignment succeeded?

--- Converts a FreeImage tag structure to a string that represents the interpreted tag value.
-- (NYI: oversight?)
-- @function fipTag:toString
-- @string model Metadata model specification (metadata model from which the tag was extracted), cf. @{enums.FREE_IMAGE_MDMODEL}.
-- @treturn string Representative string.
