-- Optionally, enable this (and include it in the build settings)
-- require("plugin.bit")

local mwc = require("plugin.mwc")

do
	local rng1 = mwc.MakeGenerator()
	local rng2 = mwc.MakeGenerator()

	for i = 1, 30 do
		if i == 1 then
			print("Random integers")
		elseif i == 11 then
			print("")
			print("Random integers, 0-49")
		elseif i == 21 then
			print("")
			print("Random floats")
		end

		if i <= 10 then
			print(rng1(), rng2())
		elseif i <= 20 then
			print(rng1() % 50, rng2() % 50)
		else
			print(rng1("float"), rng2("float"))
		end
	end

	print("")
	print("Do two sequences match? (integer, float)")

	for i = 1, 10 do
		print(rng1() == rng2(), rng1("float") == rng2("float"))
	end
end

do
	print("")
	print("Random integers, new sequence")

	local rng = mwc.MakeGenerator()

	for i = 1, 10 do
		print(rng())
	end
end

do
	print("")
	print("New integer sequences, custom w seed")

	local rng1 = mwc.MakeGenerator{ w = 50 }
	local rng2 = mwc.MakeGenerator{ w = 50 }

	for i = 1, 10 do
		print(rng1(), rng2())
	end
end

do
	print("")
	print("New integer sequences, custom z seed")

	local rng1 = mwc.MakeGenerator{ z = 1250 }
	local rng2 = mwc.MakeGenerator{ z = 1250 }

	for i = 1, 10 do
		print(rng1(), rng2())
	end
end

do
	print("")
	print("New integer sequences, custom w and z seeds")

	local rng1 = mwc.MakeGenerator{ w = 150, z = 3000 }
	local rng2 = mwc.MakeGenerator{ w = 150, z = 3000 }

	for i = 1, 10 do
		print(rng1(), rng2())
	end
end

do
	print("")
	print("Some sequence of 10 floats")

	local rng1 = mwc.MakeGenerator{ w = 1507, z = 3932 }
	local z, w

	for i = 1, 10 do
		if i == 6 then
			print("Making a note of our position")

			z, w = rng1("get_zw")
		end

		print(rng1("float"))
	end

	print("")
	print("Instantiating a new generator, resuming sequence from remembered position")

	local rng2 = mwc.MakeGenerator{ w = w, z = z }

	for i = 6, 10 do
		print(rng2("float"))
	end
end

do
	local rng = mwc.MakeGenerator_Lib{ w = 23931 }

	print("")
	print("Lua-style generator, no arguments")

	for i = 1, 10 do
		print(rng())
	end

	print("")
	print("Lua-style generator, single argument (25)")

	for i = 1, 10 do
		print(rng(25))
	end

	print("")
	print("Lua-style generator, two arguments (15, 65)")

	for i = 1, 10 do
		print(rng(15, 65))
	end
end

-- Pure examples (won't be run) --

-- #1
if false then
	-- For each object, in a game for instance, we can assign a generator. To avoid all
	-- objects coming up with the same sequence, each can take a unique seed. This could
	-- itself be random, say by passing the current time, or some fixed value in order to
	-- get a reproducible sequence.
	object.m_generator = mwc.MakeGenerator{ w = math.floor(object.starting_position.x / 32) } -- tile column as seed
end

-- #2
if false then
	-- We might want multiple generators, e.g. per property.
	local col = math.floor(object.starting_position.x / 32)

	object.m_pos_generator = mwc.MakeGenerator{ w = col, z = 32174 } -- some distinct integer
	object.m_angle_generator = mwc.MakeGenerator{ w = col, z = 3232 } -- another
end

-- #3
if false then
	-- Furthermore, we can save these seeds and use these to reseed our sequence, say when we
	-- quit and later resume the game. In the one-generator case, for instance:
	local w, z = GetSeedsFromSaveFile()

	if w == nil then
		w = math.floor(object.starting_position.x / 32)
	end

	object.m_generator = mwc.MakeGenerator{ w = w, z = z }
end

-- #4
if false then
	-- Elsewhere in the file, when saving, we would have something along the lines of:
	local w, z = object.m_generator("get_zw")

	AddSeedsToSaveFile(w, z)
end

-- #5
if false then
	-- Last but not least, we can make a Lua-style generator and use it like math.random().
	-- For instance, to make 20 circles with random positions, radii, and colors:
	local my_random = mwc.MakeGenerator_Lib{ w = 42 }

	for i = 1, 20 do
		local x, y = my_random(display.contentWidth), my_random(display.contentHeight)
		local circle = display.newCircle(x, y, my_random(15, 35))

		circle:setFillColor(my_random(), my_random(), my_random())
	end
end