--- State management for Quirky scene.

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
local floor = math.floor
local ipairs = ipairs
local min = math.min
local pairs = pairs
local sin = math.sin

-- Modules --
local colors = require("quirky.colors")
local info = require("quirky.info")

-- Corona globals --
local display = display
local Runtime = Runtime
local transition = transition

-- Cached module references --
local _AddFloor_
local _AddToStash_
local _MoreThanOneCharacter_
local _SwitchCharacter_

-- Exports --
local M = {}

-- Stashes used to recycle circles and rects --
local Circles, Rects

-- Put an object in a stash for reuse
function M.AddToStash (what, object)
	if what == "circles" then
		Circles:insert(object)
	elseif what == "rects" then
		Rects:insert(object)
	end
end

-- Object getters that try to favor the stash --
local GetCircle, GetRect

-- Put a floor graphic at a given position
function M.AddFloor (group, x, y)
	local rect = GetRect(group, x, y, "Floor.png")

	colors.Fill(rect, .9, .8, 1)

	return rect
end

-- Dimensions of a cell in the level --
local CellW, CellH

-- Position of level center --
local CX, CY

-- Radius (average when pulsing) of a character... --
local CharacterSize

-- ...and of a rivet... --
local RivetSize

-- ...and a Sokoban-style spot --
local SpotSize

-- Make a function that visits each cell in the level's grid
local function GridVisitor (func)
	return function(fixed, a, b, c, d, e)
		local w, h = info.GetDims()
		local index, x0, y = 1, CX - floor((w * CellW + 1) / 2), CY - ((h * CellH + 1) / 2)

		for row = 1, h do
			local x = x0

			for col = 1, w do
				func(fixed, col, row, x, y, index, a, b, c, d, e)

				index, x = index + 1, x + CellW
			end

			y = y + CellH
		end
	end
end

-- Load graphics for a cell's level entries that remain fixed
local AuxBake = GridVisitor(function(scene, _, _, x, y, index, ground, middle)
	local gentry, mentry = ground[index], middle[index]

	-- Walls are a simple tinted graphic.
	if mentry == "W" then
		colors.Fill(GetRect(scene.m_baked, x, y, "Wall.png"), .4)

	-- Pivots consist of a block and a rivet on top.
	elseif mentry and mentry:sub(1, 1) == "P" then
		colors.Fill(GetRect(scene.m_pivots, x, y, "Block.png"), info.GetColor("T" .. mentry:sub(2)))
		colors.Fill(GetCircle(scene.m_rivets, x, y, RivetSize), .825)

	-- Empty ground entries or Sokoban-style spots have at least a floor graphic. In the
	-- latter case we also put a spot graphic on top.
	elseif not gentry or gentry == "S" then
		_AddFloor_(scene.m_baked, x, y)

		if gentry == "S" then
			colors.Fill(GetCircle(scene.m_spots, x, y, SpotSize), 0, 1, 0)
		end

	-- Exits are a tinted graphic with a gray border.
	elseif gentry == "E" then
		colors.FillWithGrayStroke(GetRect(scene.m_baked, x, y, "Exit.png"), .35, .2)
	end
end)

-- Bake the level's fixed graphics
function M.Bake (scene, state)
	AuxBake(scene, state.ground, state.middle)
end

-- Ensure an empty group (in the same display order), caching any entries in the old group
local function GetFreshGroup (scene, name, what)
	--
	local group = scene[name]

	if group.numChildren > 0 then
		local game, index = group.parent, 1

		while game[index] ~= group do
			index = index + 1
		end

		_AddToStash_(what, group)

		group = display.newGroup()

		game:insert(index, group)

		scene[name] = group
	end

	return group
end

-- Clear a given set of layers
function M.Clear (scene, layers)
	for k, v in pairs(layers) do
		GetFreshGroup(scene, k, v)
	end
end

-- Accumulate an object if it matches a given type
local AuxFind = GridVisitor(function(layer, col, row, _, _, index, acc, what)
	if layer[index] == what then
		local n = acc.n

		acc[n + 1] = col
		acc[n + 2] = row
		acc[n + 3] = index

		acc.n = n + 3
	end
end)

-- Find all objects with a given type in a layer, accumulating them
function M.Find (what, layer, acc)
	acc.n = 0

	AuxFind(layer, acc, what)
end

-- Get the column and row of the currently controlled character
function M.GetCoordinates (state)
	local who = state.who

	for _, character in ipairs(state.characters) do
		if character.id == who then
			return character.col, character.row
		end
	end
end

-- Current character's display object --
local Player

-- Get the display object belonging to the currently controlled character
function M.GetPlayer (scene)
	return Player
end

-- Are there multiple characters active?
function M.MoreThanOneCharacter (state)
	return #state.characters > 1
end

-- Object move transition --
local MoveParams = { delta = true, time = 350, onComplete = info.DecMoveCount }

-- Move logic body
local function AuxMove (object, dx, dy)
	MoveParams.x, MoveParams.y = dx * CellW, dy * CellH

	transition.to(object, MoveParams)
end

-- Move a graphic, possibly with delay
function M.Move (object, dx, dy, delay)
	MoveParams.delay = delay

	AuxMove(object, dx, dy)
end

-- Move a block's graphic components
function M.MoveBlock (scene, entry, dx, dy)
	MoveParams.delay = nil

	info.ForEachObject(scene.m_middle, entry, AuxMove, dx, dy)
end

-- Turnstile rotate transition --
local RotateParams = { onComplete = info.DecMoveCount }

-- Rotate logic body
local function AuxRotate (object, da)
	RotateParams.rotation = object.rotation + da

	transition.to(object, RotateParams)
end

-- Move a turnstile's graphic components
function M.MoveTurnstile (scene, entry, cw)
	info.ForEachObject(scene.m_middle, entry, AuxRotate, cw and 90 or -90)
end

-- Sets a turnstile part's size (and anchored positions) so that it rotates nicely
local function SizeTurnstilePart (rect, pcol, prow, tcol, trow, x, y)
	local thickness = min(CellW, CellH)

	if tcol ~= pcol then -- left or right
		rect.path.width, rect.path.height, rect.y = 1.5 * thickness, thickness, y

		if tcol < pcol then
			rect.anchorX, rect.x = 1, x + CellW
		else
			rect.anchorX, rect.x = 0, x - CellW
		end
	else -- above or below
		rect.path.width, rect.path.height, rect.x = thickness, 1.5 * thickness, x

		if trow < prow then
			rect.anchorY, rect.y = 1, y + CellH
		else
			rect.anchorY, rect.y = 0, y - CellH
		end
	end
end

-- Pulse the character, if any, currently controlled by the player
Runtime:addEventListener("enterFrame", function(event)
	if Player then
		local scale = 1 + .15 * sin(event.time / 450)

		Player.xScale, Player.yScale = scale, scale
	end
end)

-- Load graphics for a cell's current game state entries
local AuxPopulate = GridVisitor(function(state, col, row, x, y, index, ggroup, mgroup, pgroup, ground, middle)
	local gentry, mentry = ground[index], middle[index]

	-- Holes are a black rect.
	if gentry == "H" then
		colors.Fill(GetRect(ggroup, x, y), 0)

	-- Put floors in filled parts of holes.
	elseif gentry == "F" then
		_AddFloor_(ggroup, x, y)
	end

	-- Characters are tinted circles. The graphic belonging to the character with the
	-- current ID is elected as the player.
	if mentry == "C" then
		for _, character in ipairs(state.characters) do
			if character.col == col and character.row == row then
				local player = GetCircle(pgroup, x, y, CharacterSize)

				if state.who == character.id then
					Player = player
				end

				player.m_who = character.id

				colors.Fill(player, info.GetColor(character.id))

				break
			end
		end

	-- Otherwise, the middle entry is either empty or is part of a block or turnstile. In
	-- the latter case these consist of a block graphic, with some resizing in the case of
	-- turnstiles. The (ID'd) entry is also assigned as a type to allow later finds.
	elseif mentry then
		local what = mentry:sub(1, 1)

		if what == "B" or what == "T" then
			local rect = GetRect(mgroup, x, y, "Block.png")

			rect.m_type = mentry

			if what == "T" then
				local pcol, prow = info.GetPivot(mentry)

				SizeTurnstilePart(rect, pcol, prow, col, row, x, y)
			end

			colors.Fill(rect, info.GetColor(mentry))
		end
	end	
end)

-- Update the game state's dynamic graphics
function M.Populate (scene, state)
	local players = GetFreshGroup(scene, "m_players", "circles")
	local middle = GetFreshGroup(scene, "m_middle", "rects")
	local ground = GetFreshGroup(scene, "m_ground", "rects")

	AuxPopulate(state, ground, middle, players, state.ground, state.middle)
end

-- Remove the currently controlled character and update related state
function M.RemoveCharacter (state)
	local who = state.who

	-- When this is the only character, invalidate the player and current ID. Otherwise,
	-- begin by passing control to the next in line.
	if _MoreThanOneCharacter_(state) then
		_SwitchCharacter_(state)
	else
		Player, state.who = nil
	end

	-- Find the character's position. Since we always look up characters by ID, order is
	-- unimportant, and the last array entry can be back-filled in its place.
	local n = #state.characters

	for i = 1, n do
		if state.characters[i].id == who then
			state.characters[i] = state.characters[n]
			state.characters[n] = nil

			break
		end
	end
end

-- Assign a cell size for the level's grid
function M.SetCellSize (cellw, cellh)
	CellW, CellH = cellw, cellh
end

-- Assign a level center
function M.SetCenter (cx, cy)
	CX, CY = cx, cy
end

-- Assign a character size
function M.SetCharacterSize (size)
	CharacterSize = size
end

-- Set the column and row of the currently controlled character
function M.SetCoordinates (state, col, row)
	local who = state.who

	for _, character in ipairs(state.characters) do
		if character.id == who then
			character.col, character.row = col, row
		end
	end
end

-- Makes a function to initialize a display object, preferentially reusing one from a stash
local function ItemGetter (stash, new, init)
	return function(into, a, b, c)
		for i = stash.numChildren, 1, -1 do
			local elem, item = stash[i]

			-- The stashed item might be a group. If empty, remove it and move on. Otherwise,
			-- extract one of its children.
			if elem._type == "GroupObject" then
				local n = elem.numChildren

				if n == 0 then
					stash:remove(i)
				else
					item = elem[n]
				end

			-- Otherwise, use the item itself.
			else
				item = elem
			end

			-- If we found an item, put it into our target group. Clear any type information
			-- and do appropriate initialization, then return it.
			if item then
				into:insert(item)

				item.m_type = nil

				init(item, a, b, c)

				return item
			end
		end

		-- Nothing in stash: make a new item.
		return new(into, a, b, c)
	end
end

-- Set up Quirky's groups
function M.SetGroups (scene, layers)
	local game = display.newGroup()

	-- Create our stashes.
	Circles = display.newGroup()
	Rects = display.newGroup()

	-- Make a common game group and add it and the (invisible) stashes to the scene.
	for i, group in ipairs{ game, Circles, Rects } do
		scene.view:insert(group)

		group.isVisible = i == 1
	end

	-- Add each of the game layers.
	for _, name in ipairs(layers) do
		scene[name] = display.newGroup()

		game:insert(scene[name])
	end

	-- Make a circle getter.
	GetCircle = ItemGetter(Circles, function(into, x, y, radius)
		return display.newCircle(into, x, y, radius)
	end, function(circle, x, y, radius)
		circle.x, circle.y = x, y
		circle.path.radius = radius
		circle.alpha, circle.xScale, circle.yScale = 1, 1, 1
	end)

	-- Fill to dynamically assign an image to recyclable rects --
	local Fill = { type = "image" }

	-- Updates the rect's image
	local function SetFill (rect, name)
		if name then
			Fill.filename = name
			rect.fill = Fill
		else
			rect.fill = nil
		end

		return rect
	end

	-- Make a rect getter.
	GetRect = ItemGetter(Rects, function(into, x, y, image)
		return SetFill(display.newRect(into, x, y, CellW, CellH), image)
	end, function(rect, x, y, image)
		rect.anchorX, rect.x = .5, x
		rect.anchorY, rect.y = .5, y
		rect.path.width, rect.path.height = CellW, CellH
		rect.rotation, rect.alpha = 0, 1

		SetFill(rect, image)
	end)
end

-- Assign a rivet size
function M.SetRivetSize (size)
	RivetSize = size
end

-- Assign a Sokoban-style spot size
function M.SetSpotSize (size)
	SpotSize = size
end

-- Helper to find the next ID in a cyclic array
local function LowestID (characters, too_low)
	local lowest

	for _, character in ipairs(characters) do
		local id = character.id

		if id > too_low and (not lowest or id < lowest) then
			return id
		end
	end

	return lowest
end

-- Character scale restore transition --
local RestoreScaleParams = { xScale = 1, yScale = 1 }

-- Passes control to another character and updates related state
function M.SwitchCharacter (state)
	local characters, who = state.characters, state.who
	local next_who = LowestID(characters, who) or LowestID(characters, 0)

	if next_who ~= who then
		state.who = next_who

		-- Restore the previous player to unit scale since it has stopped pulsing.
		transition.to(Player, RestoreScaleParams)

		-- Pick the new player graphic.
		local pgroup = Player.parent

		for i = 1, pgroup.numChildren do
			if pgroup[i].m_who == next_who then
				Player = pgroup[i]

				break
			end
		end
	end
end

-- Cache module members.
_AddFloor_ = M.AddFloor
_AddToStash_ = M.AddToStash
_MoreThanOneCharacter_ = M.MoreThanOneCharacter
_SwitchCharacter_ = M.SwitchCharacter

-- Export the module.
return M