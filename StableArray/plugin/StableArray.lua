--- This class provides an array from which elements may be removed without upsetting
-- the positions of other elements, while also maintaining the ability to iterate the
-- elements.
-- @module StableArray

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
local abs = math.abs
local getmetatable = getmetatable
local type = type

-- Modules --
local class = require("plugin.tektite_core.class")
local embedded_free_list = require("plugin.tektite_core.array.embedded_free_list")
local var_preds = require("plugin.tektite_core.var.predicates")

-- Imports --
local GetInsertIndex = embedded_free_list.GetInsertIndex
local InUse = embedded_free_list.InUse
local IsCallable = var_preds.IsCallable
local IsNaN = var_preds.IsNaN
local RemoveAt = embedded_free_list.RemoveAt

-- Unique member keys --
local _array = {}
local _free = {}

-- Library --
local lib = require("CoronaLibrary"):new{ name = 'stablearray', publisherId = 'com.xibalbastudios' }

--- Factory for stable array objects.
-- @function New
-- @treturn StableArray A new stable array instance.

-- StableArray class definition --
lib.New = class.Define(function(StableArray)
	--- Removes all elements from the array.
	function StableArray:Clear ()
		self[_array], self[_free] = {}, 0
	end

	-- Helper to put elements in storage form, to account for certain special values
	local False, NaN, Nil, Number = { value = false }, { value = 0 / 0 }, {}, {}

	local function Fix (elem)
		-- `false` is not a problem, in se, but fixing it allows for some assumptions.
		-- `nil` values are fixed in order to not put holes in the underlying array.
		if not elem then
			elem = elem == false and False or Nil

		-- `nan` is fixed so that Find() can correctly be written in the obvious way.
		elseif IsNaN(elem) then
			elem = NaN

		-- Elements of type "number" are ambiguous when compared to free queue components.
		elseif type(elem) == "number" then
			elem = Number
		end

		return elem
	end

	-- Set (arbitrary) metatable on special values, for easy lookup.
	setmetatable(False, Nil)
	setmetatable(NaN, Nil)
	setmetatable(Nil, Nil)

	local function Fixed (v)
		return getmetatable(v) == Nil
	end

	-- Helper to scan a range
	local function AuxFind (arr, from, to, elem)
		for i = from, to do
			if arr[i] == elem then
				return i
			end
		end
	end

	-- Helper to find an element in the array
	local function Find (arr, elem)
		local n, fixed = #arr, Fix(elem)

		if fixed ~= Number then
			return AuxFind(arr, 1, n, fixed)
		else
			return AuxFind(arr, -n, -1, elem)
		end
	end

	--- Finds _elem_ in the array.
	--
	-- If the array contains multiple instances of _elem_, only one is found.
	-- @param elem Element to find.
	-- @treturn ?|uint|nil Slot index, or **nil** if _elem_ was not found.
	function StableArray:Find (elem)
		local index = Find(self[_array], elem)

		return index and abs(index)
	end

	-- Helper to remove an element from the array
	local function Remove (SA, i)
		local arr = SA[_array]

		-- If a number is in the hash part, remove it, then correct the index to account for the
		-- nonce contained in the array part.
		if i < 0 then
			i, arr[i] = -i
		end

		-- Do removal from the array part.
		SA[_free] = RemoveAt(arr, i, SA[_free])
	end

	--- Removes _elem_ from the array, cf. @{StableArray:RemoveAt}.
	--
	-- If the array contains multiple instances of _elem_, only one is found and removed.
	-- @param elem Element to remove.
	-- @treturn ?|uint|nil Slot index of _elem_, or **nil** if it was not found.
	function StableArray:FindAndRemove (elem)
		local index = Find(self[_array], elem)

		if index then
			Remove(self, index)
		end

		return index and abs(index)
	end

	-- Helper to undo any storage-demanded fixup
	local function DeFix (arr, i)
		local elem = arr[i]

		if elem == Number then
			return arr[-i]
		elseif Fixed(elem) then
			return elem.value
		end

		return elem
	end

	--- Getter.
	-- @uint index Slot index.
	-- @return If the slot is in use, element in the slot; otherwise, **nil**.
	--
	-- **N.B.** When **nil** elements might have been inserted, @{StableArray:InUse} can be
	-- used to distinguish missing values from **nil** elements.
	function StableArray:Get (index)
		local arr = self[_array]

		if InUse(arr, index) then
			return DeFix(arr, index)
		else
			return nil
		end
	end

	--- Gets the elements as an array.
	-- @param null Value to mark unused slots.
	--
	-- If this is callable, the value is instead the result of `null(element)`, _element_
	-- being whatever occupies the slot.
	-- @param nil_elem Value to mark **nil** elements.
	-- @treturn array Copy of the stable array's elements.
	--
	-- **N.B.** The array can contain holes if one of _null_ or _nil\_elem_ is missing.
	-- @see StableArray:InUse, StableArray:__len
	function StableArray:GetArray (null, nil_elem)
		local arr, out = self[_array], {}
		local is_callable = IsCallable(null)

		-- If a slot was marked in the previous step, assign it the null value. Otherwise,
		-- load the fixed-up element.
		for i = 1, #arr do
			local elem = arr[i]

			if type(elem) ~= "number" then
				local raw = DeFix(arr, i)

				if raw == nil then
					out[i] = nil_elem
				else
					out[i] = raw
				end
			elseif is_callable then
				out[i] = null(elem)
			else
				out[i] = null
			end
		end

		return out
	end

	-- Adds an element to the array, applying fixup on ambiguous elements
	local function Add (arr, i, elem)
		local fixed = Fix(elem)

		if fixed == Number then
			arr[-i] = elem
		end

		arr[i] = fixed
	end

	--- Inserts _elem_ into the array.
	-- @param elem Element to add.
	-- @treturn uint Slot index at which _elem_ was inserted.
	function StableArray:Insert (elem)
		local arr, index = self[_array]

		index, self[_free] = GetInsertIndex(arr, self[_free])

		Add(arr, index, elem)

		return index
	end

	--- Predicate.
	-- @uint index Slot index.
	-- @treturn boolean Slot contains an element?
	function StableArray:InUse (index)
		return InUse(self[_array], index)
	end

	-- Iterator body
	local function AuxIpairs (SA, i)
		local arr = SA[_array]

		for j = i + 1, #arr do
			if InUse(arr, j) then
				return j, DeFix(arr, j)
			end
		end
	end

	--- Iterator.
	-- @{ipairs}-style iterator over the in-use slots of the array.
	-- @treturn iterator Supplies slot index, element in slot.
	function StableArray:Ipairs ()
		return AuxIpairs, self, 0
	end

	-- Metamethod (in certain Lua implementations), aliases @{StableArray:Ipairs}.
	StableArray.__ipairs = StableArray.Ipairs

	--- Metamethod.
	-- @treturn uint Array size (element count + free slots).
	function StableArray:__len ()
		return #self[_array]
	end

	--- Removes an element from the array.
	-- @uint index Slot index of element.
	--
	-- If the slot is not in use, this is a no-op.
	-- @see StableArray:InUse
	function StableArray:RemoveAt (index)
		local arr = self[_array]

		if InUse(arr, index) then
			Remove(self, arr[index] == Number and -index or index)
		end
	end

	--- Clears the stable array and loads elements from a stock Lua array.
	-- @array arr Array used to populate the stable array.
	-- @param null If non-**nil**, instances of _null_ will be removed from the array
	-- generated by _arr_, leaving those slots unused.
	-- @param nil_elem If non-**nil**, instances of _nil\_elem_ will be replaced by **nil**.
	-- @see StableArray:Clear
	function StableArray:SetArray (arr, null, nil_elem)
		local into, n, free, has_any = {}, #arr, 0

		for i = n, 1, -1 do
			local elem = arr[i]
			local non_null = elem ~= null

			if elem == nil_elem then
				elem = nil
			end

			if has_any or non_null then
				if non_null then
					Add(into, i, elem)

					has_any = true
				else
					into[i], free = free, i
				end
			end
		end

		self[_array], self[_free] = into, free
	end

	--- Updates the element at a slot.
	--
	-- If the slot is not in use, this is a no-op.
	-- @uint index Slot index of element.
	-- @param elem Element to add.
	-- @treturn boolean Was the element updated?
	-- @return If the element was updated, the previous value.
	-- @see StableArray:InUse
	function StableArray:Update (index, elem)
		local arr = self[_array]

		if InUse(arr, index) then
			local old = DeFix(arr, index)

			arr[-index] = nil

			Add(arr, index, elem)

			return true, old
		else
			return false
		end
	end

	-- Class constructor.
	function StableArray:__cons ()
		self:Clear()
	end
end)

return lib