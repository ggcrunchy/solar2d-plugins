local StableArray = require("plugin.StableArray").New

local sa = StableArray()

local i1 = sa:Insert("bob")
local i2 = sa:Insert(-73) -- Integers (not ideal, but possible)
local i3 = sa:Insert{} -- Table
local i4 = sa:Insert(newproxy()) -- Userdata
local i5 = sa:Insert(coroutine.create(function() end)) -- Coroutine
local i6 = sa:Insert(37) -- Another integer
local i7 = sa:Insert(0 / 0) -- Handles NaN
local i8 = sa:Insert(nil) -- Handles nil

local t = {}

local i9 = sa:Insert(t)

print("Examining array, using raw iteration (index, element, type)...")

for i = 1, #sa do
	print(i, sa:Get(i), type(sa:Get(i)))
end

print("")
print("...and using iterator (index, element)")

for i, elem in sa:Ipairs() do
	print(i, elem)
end

print("")
print("Removing element at index 2")

sa:RemoveAt(2)

print("")
print("Raw iteration (sans type)...")

for i = 1, #sa do
	print(i, sa:Get(i))
end

print("")
print("...and using iterator")

for i, elem in sa:Ipairs() do
	print(i, elem)
end

print("")
print("Found t at index:", sa:Find(t))
print("Found nil at index:", sa:Find(nil)) -- possible to search for nil
print("Did not find \"BLARGH\", alas:", sa:Find("BLARGH"))
print("Found NaN at index:", sa:Find(0 / 0)) -- possible to search for NaN (not different ones, though)
print("And \"bob\"? What about him?", sa:Find("bob"))

print("")
print("Trying to remove -73 (already gone)")

sa:FindAndRemove(-73)

print("")
print("Raw iteration...")

for i = 1, #sa do
	print(i, sa:Get(i))
end

print("")
print("...and iterator")

for i, elem in sa:Ipairs() do
	print(i, elem)
end

print("")
print("Examining items in use (index, used?)")

for i = 1, #sa do
	print(i, sa:InUse(i))
end

print("")
print("Updating elements")
print("Failure, updating unused slot:", sa:Update(2, {}))
print("Failure, updating too-high slot:", sa:Update(1000, true))
print("Updating slot 5 (what was old value?):", sa:Update(5, "New result!"))

print("")
print("Using iterator")

for i, elem in sa:Ipairs() do
	print(i, elem)
end

print("")
print("Out-of-bounds accesses")
print("Index too low:", sa:Get(-3))
print("Index too high:", sa:Get(#sa + 27))

print("???", sa[-3])
print("????", sa[#sa + 27])
print("?!", sa[3])

print("")
print("Adding another entry (\"dog\") at:", sa:Insert("dog"))
print("And one more (false) at:", sa:Insert(false))
print("")
print("Raw iteration")

for i, elem in sa:Ipairs() do
	print(i, elem)
end

print("")
print("Removing 37")

sa:FindAndRemove(37)

print("")
print("Using iterator")

for i, elem in sa:Ipairs() do
	print(i, elem)
end

print("")
print("Removing item at index i4")

sa:RemoveAt(i4)

print("")
print("Using iterator")

for i, elem in sa:Ipairs() do
	print(i, elem)
end

print("")
print("Serializing array")

local NULL, NIL = {}, {} -- Make two nonces to substitute for unused and nil slots
local arr = sa:GetArray(NULL, NIL)

print("")
print("Null, nil:", NULL, NIL)
print("Contents:")

for i = 1, #sa do
	print(i, arr[i])
end

print("")
print("Clearing array")

sa:Clear()

print("")
print("Anything to iterate?")
print("")

for i, elem in sa:Ipairs() do
	print("STILL HERE?!", i, elem)
end

print("Deserializing array")

sa:SetArray(arr, NULL, NIL)

print("")
print("Using iterator")

for i, elem in sa:Ipairs() do
	print(i, elem)
end

-- Can iterate while removing current element
do
	local sa = StableArray()

	print("")
	print("Adding several elements to new array")

	for i = 1, 10 do
		sa:Insert(i * 1.3)
	end

	for i = 1, #sa do
		print(sa:Get(i))
	end

	print("")
	print("While iterating...")

	for i, elem in sa:Ipairs() do
		if i % 2 == 0 then
			print("Removing even element", sa:Get(i))

			sa:RemoveAt(i)
		end
	end

	print("")
	print("Contents now?")

	for i, elem in sa:Ipairs() do
		print(i, elem)
	end
end

-- Can iterate while removing elements before or ahead
do
	local sa = StableArray()

	print("")
	print("Adding several elements to another new array")

	for i = 1, 10 do
		sa:Insert(i * .7)
	end

	for i = 1, #sa do
		print(sa:Get(i))
	end

	print("")
	print("While iterating...")

	for i, elem in sa:Ipairs() do
		local index = #sa - i

		print("Visiting", i, elem)

		if index % 3 == 0 and sa:InUse(index) then
			print("Removing element", index, sa:Get(index))

			sa:RemoveAt(index)
		end
	end

	print("")
	print("Contents now?")

	for i, elem in sa:Ipairs() do
		print(i, elem)
	end
end