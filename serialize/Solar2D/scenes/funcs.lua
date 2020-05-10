--- Scene that runs the serialize sub-modules' test suites.

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

-- Plugins --
local serialize = require "plugin.serialize"

-- Corona globals --
local display = display

-- Corona modules --
local composer = require("composer")
local widget = require("widget")

-- Function calls scene.
local Scene = composer.newScene()

--
local function Print (into, size)
    local y = 0

    return function(...)
        local strs = { ... }

        for i = 1, #strs do
            strs[i] = tostring(strs[i])
        end

        local text = display.newText(table.concat(strs, "   "), 0, 0, native.systemFontBold, size)

        text.anchorX, text.x = 0, 5
        text.anchorY, text.y = 0, y

        text:setTextColor(1, 0, 0)

        into:insert(text)

        y = y + text.contentHeight + 5
    end
end

-- Create --
function Scene:create ()
	local page = widget.newScrollView{
		backgroundColor = { .075 },
		top = (display.contentHeight - display.viewableContentHeight) / 2,
		left = (display.contentWidth - display.viewableContentWidth) / 2,
		height = display.viewableContentHeight
	}

	self.view:insert(page)

	local print = Print(page, 8)

	-----------------------------
	-- Tests for serialize.lpack:
	-----------------------------
	print("lpack...")
	print("")
	print("")
	print("")

	local lpack = serialize.lpack

	local bpack = lpack.pack
	local bunpack = lpack.unpack

	local function hex(s)
	  s=string.gsub(s,"(.)",function (x) return string.format("%02X",string.byte(x)) end)
	  return s
	end

	a=bpack("Ab8","\027Lua",5*16+1,0,1,4,4,4,8,0)
	print(hex(a),string.len(a))

	b=string.dump(hex)
	b=string.sub(b,1,string.len(a))
	print(a==b,string.len(b))
	print(bunpack(b,"bA3b8"))

	i=314159265 f="<I>I=I"
	a=bpack(f,i,i,i)
	print(hex(a))
	print(bunpack(a,f))

	i=3.14159265 f="<d>d=d"
	a=bpack(f,i,i,i)
	print(hex(a))
	print(bunpack(a,f))

	local function assert (s, ...)
		if not s then
			print(debug.getinfo(2).short_src, debug.getinfo(2).currentline)
		else
			return s
		end
	end

	------------------------------
	-- Tests for serialize.struct:
	------------------------------
	print("struct...")
	print("")
	print("")
	print("")

	local lib = serialize.struct

	-- Lua 5.1 x Lua 5.2
	local unpack = unpack or table.unpack

	--
	-- auxiliar function to print an hexadecimal `dump' of a given string
	-- (not used by the test)
	--
	local function bp (s)
		s = string.gsub(s, "(.)", function(c)
		return string.format("\\%02x", string.byte(c))
	end)
	print(s)
	end

	local a,b,c,d,e,f,x

	-- assume sizeof(int) == 4
	assert(#lib.pack("I4", 67324752) == 4 and lib.size("I4", 4) == 4)

	assert(lib.size('bbb') == 3)
	assert(lib.pack('b', 10) == string.char(10))
	assert(lib.pack('bbb', 10, 20, 30) == string.char(10, 20, 30))

	assert(lib.size('h') == 2)  -- assume sizeof(short) == 2
	assert(lib.pack('<h', 10) == string.char(10, 0))
	assert(lib.pack('>h', 10) == string.char(0, 10))
	assert(lib.pack('<h', -10) == string.char(256-10, 256-1))

	x = lib.size('l') - 1
	assert(lib.pack('<l', 10) == string.char(10) .. string.char(0):rep(x))
	assert(lib.pack('>l', 10) == string.char(0):rep(x) .. string.char(10))
	assert(lib.pack('<l', -10) == string.char(256-10) .. string.char(255):rep(x))

	assert(lib.unpack('<h', string.char(10, 0)) == 10)
	assert(lib.unpack('>h', string.char(0, 10)) == 10)
	assert(lib.unpack('<h', string.char(256-10, 256-1)) == -10)

	assert(lib.unpack('<i4', string.char(10, 0, 0, 1)) == 10 + 2^(3*8))
	assert(lib.unpack('>i4', string.char(0, 1, 0, 10)) == 10 + 2^(2*8))
	assert(lib.unpack('<i4', string.char(256-10, 256-1, 256-1, 256-1)) == -10)

	assert(lib.size("<lihT") == lib.size(">LIHT"))
	assert(lib.size("!4bi") > lib.size("!1bi"))

	x = lib.size("T") - 1
	assert(lib.pack('<T', 10) == string.char(10) .. string.char(0):rep(x))
	assert(lib.pack('>T', 10) == string.char(0):rep(x) .. string.char(10))
	assert(lib.pack('<T', -10) == string.char(256-10) .. string.char(255):rep(x))


	-- minimum limits
	lims = {{'B', 255}, {'b', 127}, {'b', -128},
	{'I1', 255}, {'i1', 127}, {'i1', -128},
	{'H', 2^16 - 1}, {'h', 2^15 - 1}, {'h', -2^15},
	{'I2', 2^16 - 1}, {'i2', 2^15 - 1}, {'i2', -2^15},
	{'L', 2^32 - 1}, {'l', 2^31 - 1}, {'l', -2^31},
	{'I4', 2^32 - 1}, {'i4', 2^31 - 1}, {'i4', -2^31},
	}

	for _, a in pairs{'', '>', '<'} do
		for _, l in pairs(lims) do
			local fmt = a .. l[1]
			assert(lib.unpack(fmt, lib.pack(fmt, l[2])) == l[2])
		end
	end


	-- tests for fixed-sized ints
	for _, i in pairs{1,2,4} do
		x = lib.pack('<i'..i, -3)
		assert(string.len(x) == i)
		assert(x == string.char(256-3) .. string.rep(string.char(256-1), i-1))
		assert(lib.unpack('<i'..i, x) == -3)
	end


	-- alignment
	d = lib.pack("d", 5.1)
	ali = {[1] = string.char(1)..d,
		[2] = string.char(1, 0)..d,
		[4] = string.char(1, 0, 0, 0)..d,
		[8] = string.char(1, 0, 0, 0, 0, 0, 0, 0)..d,
	}

	for a,r in pairs(ali) do
		assert(lib.pack("!"..a.."bd", 1, 5.1) == r)
		local x,y = lib.unpack("!"..a.."bd", r)
		assert(x == 1 and y == 5.1)
	end


	print('+')

	-- tests for non-power-of-two sizes
	assert(lib.pack("<i3", 10) == string.char(10, 0, 0))
	assert(lib.pack("<I3", -10) == string.char(256 - 10, 255, 255))
	assert(lib.unpack("<i3", string.char(10, 0, 0)) == 10)
	assert(lib.unpack(">I3", string.char(255, 255, 256 - 21)) == 2^(3*8) - 21)

	-- tests for long long
	if lib.unpack("i8", string.rep("\255", 8)) ~= -1 then
		print("no support for 'long long'")
	else
		local lim = 800
		assert(lib.pack(">i8", 2^52) == "\0\16\0\0\0\0\0\0")
		local t = {}; for i = 1, lim do t[i] = 2^52 end
		assert(lib.pack(">" .. string.rep("i8", lim), unpack(t, 1, lim))
		== string.rep("\0\16\0\0\0\0\0\0", lim))
		assert(lib.pack("<i8", 2^52 - 1) == "\255\255\255\255\255\255\15\0")
		assert(lib.pack(">i8", -2^52 - 1) == "\255\239\255\255\255\255\255\255")
		assert(lib.pack("<i8", -2^52 - 1) == "\255\255\255\255\255\255\239\255")

		assert(lib.unpack(">i8", "\255\239\255\255\255\255\255\255") == -2^52 - 1)
		assert(lib.unpack("<i8", "\255\255\255\255\255\255\239\255") == -2^52 - 1)
		assert(lib.unpack("<i8", "\255\255\254\255\255\255\015\000") ==
		2^52 - 1 - 2^16)
		assert(lib.unpack(">i8", "\000\015\255\255\255\255\254\254") == 2^52 - 258)

		local fmt = ">" .. string.rep("i16", lim)
		local t1 = {lib.unpack(fmt, lib.pack(fmt, unpack(t)))}
		assert(t1[#t1] == 16*lim + 1  and #t == #t1 - 1)
		for i = 1, lim do assert(t[i] == t1[i]) end
		print'+'
	end

	-- strings
	assert(lib.pack("c", "alo alo") == "a")
	assert(lib.pack("c4", "alo alo") == "alo ")
	assert(lib.pack("c5", "alo alo") == "alo a")
	assert(lib.pack("!4b>c7", 1, "alo alo") == "\1alo alo")
	assert(lib.pack("!2<s", "alo alo") == "alo alo\0")
	assert(lib.pack(" c0 ", "alo alo") == "alo alo")
	for _, f in pairs{"B", "l", "i2", "f", "d"} do
		for _, s in pairs{"", "a", "alo", string.rep("x", 200)} do
			local x = lib.pack(f.."c0", #s, s)
			assert(lib.unpack(f.."c0", x) == s)
		end
	end

	-- indices
	x = lib.pack("!>iiiii", 1, 2, 3, 4, 5)
	local i = 1
	local k = 1
	while i < #x do
		local v, j = lib.unpack("!>i", x, i)
		assert(j == i + 4 and v == k)
		i = j; k = k + 1
	end

	-- alignments are relative to 'absolute' positions
	x = lib.pack("!8 xd", 12)
	assert(lib.unpack("!8d", x, 3) == 12)


	a,b,c,d = lib.unpack("<lhbxxH",
	lib.pack("<lhbxxH", -2, 10, -10, 250))
	assert(a == -2 and b == 10 and c == -10 and d == 250)

    local extra = lib.size("l") - 4 -- on 64-bit machines, has 4 extra bytes

	a, b, c, d = lib.unpack(">lBxxH", lib.pack(">lBxxH", -20, 10, 250))
	assert(a == -20 and b == 10 and c == 250 and d == 10 + extra)

	a,b,c,d,e = lib.unpack(">fdfH",
	'000'..lib.pack(">fdfH", 3.5, -24e-5, 200.5, 30000),
	4)
	assert(a == 3.5 and b == -24e-5 and c == 200.5 and d == 30000 and e == 22)

	a,b,c,d,e = lib.unpack("<fdxxfH",
	'000'..lib.pack("<fdxxfH", -13.5, 24e5, 200.5, 300),
	4)
	assert(a == -13.5 and b == 24e5 and c == 200.5 and d == 300 and e == 24)

	x = lib.pack(" > I2 f i4 I2 ", 10, 20, -30, 40001)
	assert(string.len(x) == 2 + lib.size("f") + 4 + 2)
	assert(lib.unpack(">f", x, 3) == 20)
	a,b,c,d = lib.unpack(">i2fi4I2", x)
	assert(a == 10 and b == 20 and c == -30 and d == 40001)

	local s = "hello hello"
	x = lib.pack(" b c0 ", string.len(s), s)
	assert(lib.unpack("bc0", x) == s)
	x = lib.pack("Lc0", string.len(s), s)
	assert(lib.unpack("  L  c0   ", x) == s)
	x = lib.pack("cc3b", s, s, 0)
	assert(x == "hhel\0")
	assert(lib.unpack("xxxxb", x) == 0)

	assert(lib.pack("<!8i4", 3) == string.char(3, 0, 0, 0))
	assert(lib.pack("<!8xi4", 3) == string.char(0, 0, 0, 0, 3, 0, 0, 0))
	assert(lib.pack("<!8xxi4", 3) == string.char(0, 0, 0, 0, 3, 0, 0, 0))
	assert(lib.pack("<!8xxxi4", 3) == string.char(0, 0, 0, 0, 3, 0, 0, 0))

	assert(lib.unpack("<!4i4", string.char(3, 0, 0, 0)) == 3)
	assert(lib.unpack("<!4xi4", string.char(0, 0, 0, 0, 3, 0, 0, 0)) == 3)
	assert(lib.unpack("<!4xxi4", string.char(0, 0, 0, 0, 3, 0, 0, 0)) == 3)
	assert(lib.unpack("<!4xxxi4", string.char(0, 0, 0, 0, 3, 0, 0, 0)) == 3)

	assert(lib.pack("<!2 b i4 h", 2, 3, 5) == string.char(2, 0, 3, 0, 0, 0, 5, 0))
	a,b,c = lib.unpack("<!2bi4h", string.char(2, 0, 3, 0, 0, 0, 5, 0))
	assert(a == 2 and b == 3 and c == 5)

	assert(lib.pack("<!8bi4h", 2, 3, 5) ==
	string.char(2, 0, 0, 0, 3, 0, 0, 0, 5, 0))
	a,b,c = lib.unpack("<!8bi4h", string.char(2, 0, 0, 0, 3, 0, 0, 0, 5, 0))
	assert(a == 2 and b == 3 and c == 5)

	assert(lib.pack(">sh", "aloi", 3) == "aloi\0\0\3")
	assert(lib.pack(">!sh", "aloi", 3) == "aloi\0\0\0\3")
	x = "aloi\0\0\0\0\3\2\0\0"
	a, b, c = lib.unpack("<!si4", x)
	assert(a == "aloi" and b == 2*256+3 and c == string.len(x)+1)

	x = lib.pack("!4sss", "hi", "hello", "bye")
	a,b,c = lib.unpack("sss", x)
	assert(a == "hi" and b == "hello" and c == "bye")
	a, i = lib.unpack("s", x, 1)
	assert(a == "hi")
	a, i = lib.unpack("s", x, i)
	assert(a == "hello")
	a, i = lib.unpack("s", x, i)
	assert(a == "bye")



	-- test for weird conditions
	assert(lib.pack(">>>h <!!!<h", 10, 10) == string.char(0, 10, 10, 0))
	assert(not pcall(lib.pack, "!3l", 10))
	assert(not pcall(lib.pack, "3", 10))
	assert(lib.pack("") == "")
	assert(lib.pack("   ") == "")
	assert(lib.pack(">>><<<!!") == "")
	assert(not pcall(lib.unpack, "c0", "alo"))
	assert(not pcall(lib.unpack, "s", "alo"))
	assert(lib.unpack("s", "alo\0") == "alo")
	assert(not pcall(lib.pack, "c4", "alo"))
	assert(lib.pack("c3", "alo") == "alo")
	assert(not pcall(lib.unpack, "c4", "alo"))
	assert(lib.unpack("c3", "alo") == "alo")
	assert(not pcall(lib.unpack, "bc0", "\4alo"))
	assert(lib.unpack("bc0", "\3alo") == "alo")
	assert(not pcall(lib.size, "bbc0"))
	assert(not pcall(lib.size, "s"))

	assert(not pcall(lib.unpack, "b", "alo", 4))
	assert(lib.unpack("b", "alo\3", 4) == 3)


	-- tests for large numbers
	assert(lib.pack(">i8", 1000) == string.char(0, 0, 0, 0, 0, 0, 3, 232))
	assert(lib.pack("<i8", 5000) == string.char(136, 19, 0, 0, 0, 0, 0, 0))
	assert(lib.pack("<i32", 5001) ==
	string.char(137, 19) .. string.rep('\0', 30))
	assert(lib.pack(">i32", 10000000) ==
	string.rep('\0', 29) .. string.char(0x98, 0x96, 0x80))

	print'OK'

	-------------------------------
	-- Tests for serialize.marshal:
	-------------------------------
	print("marshal...")
	print("")
	print("")
	print("")

	local marshal = serialize.marshal

	local k = { "tkey" }
	local a = { "a", "b", "c", [k] = "tval" }
	local s = assert(marshal.encode(a))

	print(string.format("%q", s))

	local t = marshal.decode(s)
	--print(t)
	table.foreach(t, print)
	assert(t[1] == "a")
	assert(t[2] == "b")
	assert(t[3] == "c")
	---[==[
	assert(#t == 3)
	local _k = next(t, #t)
	assert(type(_k) == "table")
	assert(_k[1] == "tkey")
	assert(t[_k] == "tval")

	local o = { }
	o.__index = o
	local s = marshal.encode(o)
	local t = marshal.decode(s)
	assert(type(t) == 'table')
	assert(t.__index == t)

	local up = 69
	local s = marshal.encode({ answer = 42, funky = function() return up end })
	local t = marshal.decode(s)
	assert(t.answer == 42)
	assert(type(t.funky) == "function")
	assert(t.funky() == up)

	local t = { answer = 42 }
	local c = { "cycle" }
	c.this = c
	t.here = c
	local s = marshal.encode(t)
	local u = marshal.decode(s)
	assert(u.answer == 42)
	assert(type(u.here) == "table")
	assert(u.here == u.here.this)
	assert(u.here[1] == "cycle")

	local o = { x = 11, y = 22 }
	local seen_hook
	setmetatable(o, {
		__persist = function(o)
			local x = o.x
			local y = o.y
			seen_hook = true
			local mt = getmetatable(o)
			local print = print
			return function()
				local o = { }
				o.x = x
				o.y = y
				print("constant table: 'print'")
				return setmetatable(o, mt)
			end
		end
	})

	local s = marshal.encode(o, { print })
	assert(seen_hook)
	local p = marshal.decode(s, { print })
	assert(p ~= o)
	assert(p.x == o.x)
	assert(p.y == o.y)
	assert(getmetatable(p))
	assert(type(getmetatable(p).__persist) == "function")

	local o = { 42 }
	local a = { o, o, o }
	local s = marshal.encode(a)
	local t = marshal.decode(s)
	assert(type(t[1]) == "table")
	assert(t[1] == t[2])
	assert(t[2] == t[3])

	local u = { 42 }
	local f = function() return u end
	local a = { f, f, u, f }
	local s = marshal.encode(a)
	local t = marshal.decode(s)
	assert(type(t[1]) == "function")
	assert(t[1] == t[2])
	assert(t[2] == t[4])
	assert(type(t[1]()) == "table")
	assert(type(t[3]) == "table")
	assert(t[1]() == t[3])

	local u = function() return 42 end
	local f = function() return u end
	local a = { f, f, f, u }
	local s = marshal.encode(a)
	local t = marshal.decode(s)
	assert(type(t[1]) == "function")
	assert(t[1] == t[2])
	assert(t[2] == t[3])
	assert(type(t[1]()) == "function")
	assert(type(t[4]) == "function")
	assert(t[1]() == t[4])

	local u = newproxy()
	debug.setmetatable(u, {
		__persist = function()
			return function()
				return newproxy()
			end
		end
	})

	local s = marshal.encode{u}
	local t = marshal.decode(s)
	assert(type(t[1]) == "userdata")

	local t1 = { 1, 'a', b = 'b' }
	table.foreach(t1, print)
	local t2 = marshal.clone(t1)
	print('---')
	table.foreach(t1, print)
	print('---')
	table.foreach(t2, print)
	assert(t1[1] == t2[1])
	assert(t1[2] == t2[2])
	assert(t1.b == t2.b)

	local t1 = marshal.clone({ })

	local answer = 42
	local f1 = function()
		return "answer: "..answer
	end
	local s1 = marshal.encode(f1)
	local f2 = marshal.decode(s1)
	assert(f2() == 'answer: 42')

	assert(marshal.decode(marshal.encode()) == nil)
	assert(marshal.decode(marshal.encode(nil)) == nil)

	local s1 = marshal.encode(pt)
	local p2 = marshal.decode(s1)
	print(string.format('%q',s1))

	print "OK"

	--[[ micro-bench (~4.2 seconds on my laptop)
	local t = { a='a', b='b', c='c', d='d', hop='jump', skip='foo', answer=42 }
	local s = marshal.encode(t)
	for i=1, 1000000 do
		s = marshal.encode(t)
		t = marshal.decode(s)
	end
	--]]
	--]==]
end

Scene:addEventListener("create")

return Scene
