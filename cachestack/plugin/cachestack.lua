--- Implements a system for deterministic recycling of objects, via an underlying cache.
--
-- When an object is created, or fetched from the cache, it may be registered with this
-- system. At a given point in execution, the system will then claim the object and add it to
-- the cache. The system is primarily intended for more intense parts of a program and must
-- be activated explicitly, being idle otherwise. When not running, registration is a no-op,
-- though the cache remains available for fetching.
--
-- An instance of this system is created via the @{NewCacheStack} function. (Typical programs
-- will only need one instance, but this is not required, e.g. various subsystems could each
-- have their own.) @{NewCacheStack} returns two functions: `NewType`, which we will return
-- to shortly, as well as a `WithLayer` routine.
--
-- We may invoke the latter to use the cache stack. The function takes a body along with a
-- variable argument list. In pseudo-code:
--
--    WithLayer(function(a, b) -- a: 5, b: "dog"
--      local object = GetObject() -- fetch from the cache or create a new one
--
--      Register(object) -- we want to reclaim this object later
--      InitializeAndUse(object, a, b)
--    end, 5, "dog") -- when call ends, object is sent to cache
--
-- The first time around, the cache would be empty, so a new object would need to be created.
-- Since we register this object, it will be claimed by the cache once we have executed the
-- `WithLayer` body.
--
-- Most code will not have a `Register()` call out in the open like this. A vector library,
-- for instance, might hard-wire it, as well as `GetObject()`, into various factory functions
-- and operators, e.g. `ZeroVector()`, `__add`, etc. For this reason, we can wind up with
-- objects that we want to keep, but that the cache would like to claim. We can unregister
-- such objects by returning them from the `WithLayer` body:
--
--    local result, number = WithLayer(function()
--       local a, b = vector.Random(), vector.Random()
--       local c = a * 5 + b * 6 + vector.Random() -- creates / fetches lots of intermediates
--
--       return c, 8 -- return final result of computation; non-registered objects fine too
--    end)
--
-- Since we returned it, `c` will not be claimed when `WithLayer()` completes. We can safely
-- use it&mdash;as `result`&mdash;without later code hijacking it and giving us perplexing
-- errors. The garbage collector will also now treat it like any other object.
--
-- As hinted at by the "layer" and "stack" terminology, we may nest these calls, allowing for
-- the usual benefits of composition. In these more general cases, returning an object will
-- prevent its immediate caching; however, the outer layer will try again:
--
--    local function InnerBody (rhs)
--      return IdentityMatrix() * rhs -- intermediate gets cached, result temporarily spared
--    end
--
--    local v = WithLayer(function()
--      local d, e = vector.Random(), vector.Random() -- inner layer will not try to claim these...
--      local a = WithLayer(InnerBody, e) -- ...even if we use them there
--      local b = WithLayer(InnerBody, 7)
--
--      return a:DotProduct(b)
--    end) -- outer layer ended: a and b sent to cache, along with d and e
--
-- As the last example demonstrates&mdash;mixing matrices and vectors&mdash;it can be useful
-- to allow multiple object types. In fact, the cache itself is logically divided by type,
-- and this is where the `NewType` routine mentioned a while back comes into play. Given
-- our previous example, we might have done begun our program so:
--
--    local MatrixType = NewType()
--    local VectorType = NewType()
--
-- These types are themselves functions. They accept various commands, most operating on an
-- instance of the type in question. (Since it aims for generality, the system does not make
-- many assumptions about its input; validation is the user's responsibility.) A partially
-- implemented vector type might look like:
--
--    local VectorType = NewType()
--
--    -- Vector methods.
--    local Methods = {}
--
--    Methods.__index = Methods
--
--    local function GetVector ()
--      local v = VectorType("fetch") -- try to get a vector from the cache
--
--      if not v then
--        v = setmetatable({}, Methods) -- failing that, make a new one...
--
--        VectorType("register", v) -- ...and register it (if caching is active)
--      end
--
--      return v
--    end
--
--    local function SetVector (x, y)
--       local v = GetVector()
--
--       v.x, v.y = x, y
--
--       return v
--    end
--
--    function Methods.__add (v1, v2)
--       return SetVector(v1.x + v2.x, v1.y + v2.y)
--    end
--
--    function Methods:DotProduct (other)
--      return self.x * other.x + self.y * other.y
--    end
--
--    function Methods.__mul (a, b)
--      if type(a) == "number" then
--        return SetVector(a * b.x, a * b.y)
--      elseif type(b) == "number" then
--        return SetVector(b * a.x, b * a.y)
--      else
--        return SetVector(a.x * b.x, a.y * b.y) -- arbitrarily do memberwise
--      end
--    end
--
--    -- ETC.
--
--    -- Exported interface: vector factories et al.
--    local M = {}
--
--    function M.Random ()
--      return SetVector(math.random(), math.random())
--    end
--
--    M.Vector = SetVector
--
--    -- ETC.
--
--    return M
--
-- This ought to give some idea of the "hard-wiring" mentioned earlier. All the cache-related
-- machinery&mdash;here comprising the **"fetch"** and **"register"** commands&mdash;is done
-- indirectly through the `GetVector()` and `SetVector()` utility functions, which the user
-- never needs to see.
--
-- A few additional commands allow objects to anchor other items, e.g. a matrix view might
-- need to hang on to its parent, where the actual memory is found. This is included in the
-- type since the items themselves might be registered; they must not be cached while in use.

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

-- Standard library imports --
local error = error
local pairs = pairs
local pcall = pcall
local rawequal = rawequal
local remove = table.remove
local select = select
local setmetatable = setmetatable
local type = type

-- Exports --
local M = {}

-- Metatable used a few times by this module --
local WeakKeysMT = { __mode = "k" }

-- Move any return values from the top layer into the one below.
local function MoveDown (below, top, n, first, ...)
	if n > 0 then
		local cfunc = top[first]

		if cfunc then
			below[first], top[first] = cfunc
		end

		return MoveDown(below, top, n - 1, ...)
	end
end

-- Remove any return values from the bottom layer.
local function Remove (bottom, n, first, ...)
	if n > 0 then
		if first ~= nil and first == first then -- filter out invalid keys
			bottom[first] = nil
		end

		return Remove(bottom, n - 1, ...)
	end
end

--- Instantiate a **CacheStack**. See the summary for a deeper treatment.
-- @todo below is note for on_cache, in need of some editing...
-- 
-- @treturn function Called as `local tfunc = NewType(on_cache)`, where _tfunc_ is a
-- function used to interface with the type, with the following commands and arguments:
--
-- * **"fetch"**: Called as `object = tfunc("fetch")`. If an object is available in this
-- type's cache, removes and returns it; otherwise, returns ** nil**.
-- * **"get\_ref"**: Called as `local item = tfunc("get_ref", object, category)`. If an
-- earlier **"ref"** command added an item under this category, returns it; otherwise,
-- returns **nil**.
-- * **"ref"**: Called as `tfunc("ref", object, item)`, where _category_ is some non-**nil**
-- name. Creates a weakly-keyed reference to _item_ under the given category, incrementing
-- its reference count. This ties _item_'s lifetime to _object_, while also keeping it from
-- being sent to the cache. Any item previously referenced is evicted and has its reference
-- count decremented. 
-- * **"register"**: Called as `tfunc("register", object)`. If a layer is active, the object
-- will be added it to its list for later caching; otherwise, a no-op.
-- * **"unref"**: Called as `tfunc("unref", object, category)`. If an earlier **"ref"**
-- command added an item under this category, removes it and decrements its reference count.
-- * **"unregister"**: Called as `tfunc("unregister", object)`. If the object was registered
-- in any layer's list&mdash;or is referenced and being held&mdash;it is removed. This has no
-- effect on already-cached objects.
--
-- If provided, the _on\_cache_ behavior will be called as `pcall(on_cache, item)` whenever
-- an item gets sent to this type's cache.
--
-- If Lua closes and the cache is not empty, any __gc metamethods belonging to the lingering
-- items are invoked. The "on cache" behavior should be written with this in mind to avoid
-- introducing ["double free"](https://www.owasp.org/index.php/Double_Free)-type errors.
-- @treturn function Called as `return WithLayer(body, ...)`. This pushes a layer onto the
-- cache stack and makes a protected call to _body_ with the remaining arguments. When _body_
-- concludes or errors out, the layer is popped. If there was another layer underneath, it
-- picks up where it left off.
--
-- If we try to register an item while a layer is running, it will be added to a list kept by
-- that layer. Any items in this list when the layer is popped will be added to the cache.
--
-- If _body_ throws an error, the whole list is sent to the cache. The error is propagated.
--
-- Otherwise, whatever _body_ returns is returned by **WithLayer** itself. If any of these
-- return values are items registered in this layer's list, they will be removed, although
-- the next layer down, if present, will adopt them. (See the module summary for examples.)
--
-- If an object is still referenced, it will be moved to a holding area, rather than the
-- cache, until its reference count drops to 0.
function M.NewCacheStack ()
	-- Reference counts for objects that have them --
	local Counts = setmetatable({}, WeakKeysMT)

	-- Holds item slated for the cache but still referenced by other objects --
	local Holding = setmetatable({}, WeakKeysMT)

	-- Choose some arbitrary object local to this module for internal use as a command.
	local CacheCommand = Counts
	
	-- Decrement an object's reference count. Objects end up in the holding area because
	-- they were still referenced during the add-to-cache step; any object whose count
	-- drops to 0 may therefore be immediately cached.
	local function DecrementCount (object)
		local count = Counts[object]

		if count then
			local cfunc = count == 1 and Holding[object]

			if cfunc then
				cfunc(CacheCommand, object)

				Holding[object] = nil
			end

			Counts[object] = count - 1
		end
	end

	-- Increment the object's reference count, if doing so matters (if yes, reports back).
	local function IncrementCount (object)
		local otype = type(object)

		if otype ~= "nil" and otype ~= "number" and otype ~= "boolean" then
			Counts[object] = (Counts[object] or 0) + 1

			return true
		end
	end

	-- Caching hierarchy --
	local Stack, Height = {}, 0

	-- Collection of lists allowing objects to hold named references to other items --
	local Categories = {}

	-- Add a new type to this CacheStack instance.
	local function NewType (on_cache)
		local cache, cfunc = {}

		function cfunc (how, arg1, arg2, arg3)
			-- Fetch an object of this type from the cache, if available.
			if how == "fetch" then
				return remove(cache)

			-- Put an object back in the cache, calling any user-supplied logic first.
			elseif rawequal(how, CacheCommand) then -- arg1: object
				if on_cache then
					pcall(on_cache, arg1)
				end
				
				cache[#cache + 1] = arg1

				-- Wipe any references.
				for _, clist in pairs(Categories) do
					DecrementCount(clist[arg1])

					clist[arg1] = nil
				end

			-- If caching is active, wire the object up and add it to the top layer.
			elseif how == "register" then -- arg1: object
				if Height > 0 then
					for h = 1, Height - 1 do -- sanity check: prevent presence in two layers
						Stack[h][arg1] = nil
					end

					Stack[Height][arg1] = cfunc
				end

			-- Find any reference the object is holding in a given category.
			elseif how == "get_ref" then -- arg1: object, arg2: category
				local clist = Categories[arg2]

				return clist and clist[arg1]
				
			-- Have the object reference an item in a given category.
			elseif how == "ref" then -- arg1: object, arg2: category, arg3: item
				-- Evict any item already referenced, unless it happens to be the "new" item
				-- itself, in which case we can simply quit.
				local clist = Categories[arg2]
				local cur = clist and clist[arg2]

				if not rawequal(cur, arg3) then
					DecrementCount(cur)

					-- If incrementing the object is not a no-op, add it to the category list
					-- as well, creating the list if necessary.
					if IncrementCount(arg3) then
						clist = clist or setmetatable({}, WeakKeysMT)
						clist[arg1], Categories[arg2] = arg3, clist
					end
				end

			-- Drop any reference the object is holding in a given category.
			elseif how == "unref" then -- arg1: object, arg2: category
				local clist = Categories[arg2]

				if clist then
					DecrementCount(clist[arg1])

					clist[arg1] = nil
				end

			-- Explicitly evict an object from the caching hierarchy.
			elseif how == "unregister" then -- arg1: object
				for h = 1, Height do
					Stack[h][arg1] = nil
				end

				Holding[arg1] = nil
			end
		end

		return cfunc
	end

	-- Helper to deal with WithLayer-related varargs
	local function AuxLayer (top, ok, ...)
		Height = Height - 1

		-- If no errors occurred, check for any registered objects among the return values.
		-- If the stack is still open, move them down one layer; otherwise, decouple them
		-- from the hierarchy entirely.
		if ok then
			local below, n = Stack[Height], select("#", ...)

			if below then
				MoveDown(below, top, n, ...)
			else
				Remove(top, n, ...)
			end
		end

		-- Do some cleanup on the top layer, even on error. Objects that are still referenced
		-- are moved to a holding area, while anything else is moved to the cache. 
		for object, cfunc in pairs(top) do
			local count = Counts[object]

			if count and count > 0 then
				Holding[object] = cfunc
			else
				cfunc(CacheCommand, object)
			end

			top[object] = nil
		end

		-- Return the results or propagate any error.
		if ok then
			return ...
		else
			error(...)
		end
	end

	-- Run some logic in a cache layer, returning any results
	local function WithLayer (func, ...)
		Height = Height + 1

		local top = Stack[Height] or {}

		Stack[Height] = top

		return AuxLayer(top, pcall(func, ...))
	end

	-- Export the CacheStack interface.
	return NewType, WithLayer
end

-- Export the module.
return M