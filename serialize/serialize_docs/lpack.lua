--- Corona binding to [lpack](http://webserver2.tecgraf.puc-rio.br/~lhf/ftp/lua/#lpack), a simple Lua library for packing and unpacking binary data.
--
--    local lpack = require("plugin.serialize").lpack
--
-- The library contains two functions: **pack** and **unpack**.
--
-- **pack** is called as follows: `pack(F,x1,x2,...)`, where _F_ is a string describing
-- how the values _x1_, _x2_, ... are to be interpreted and formatted. Each letter
-- in the format string _F_ consumes one of the given values. Only values of type
-- number or string are accepted. **pack** returns a (binary) string containing the
-- values packed as described in _F_. The letter codes understood by pack are listed
-- below (they are inspired by Perl's codes but are not the same). Numbers
-- following letter codes in _F_ indicate repetitions.
--
-- **unpack** is called as follows: `unpack(s,F,[init])`, where _s_ is a (binary) string
-- containing data packed as if by **pack**, _F_ is a format string describing what is
-- to be read from _s_, and the optional _init_ marks where in _s_ to begin reading the
-- values. **unpack** returns one value per letter in _F_ until _F_ or _s_ is exhausted
-- (the letters codes are the same as for **pack**, except that numbers following **'A'**
-- are interpreted as the number of characters to read into the string, not as
-- repetitions).
--
-- The first value returned by **unpack** is the next unread position in _s_, which can
-- be used as the _init_ position in a subsequent call to **unpack**. This allows you to
-- unpack values in a loop or in several steps. If the position returned by **unpack**
-- is beyond the end of _s_, then _s_ has been exhausted; any calls to **unpack** starting
-- beyond the end of _s_ will always return **nil** values.
--
-- **Notice in the original:**
--
-- This code is hereby placed in the public domain.
-- Please send comments, suggestions, and bug reports to lhf@tecgraf.puc-rio.br.
--
-- Codes
-- -----
--
-- * **z**: zero-terminated string
-- * **p**: string preceded by length byte
-- * **P**: string preceded by length word
-- * **a**: string preceded by length size_t
-- * **A**: string
-- * **f**: float
-- * **d**: double
-- * **n**: Lua number
-- * **c**: char
-- * **b**: byte = unsigned char
-- * **h**: short
-- * **H**: unsigned short
-- * **i**: int
-- * **I**: unsigned int
-- * **l**: long
-- * **L**: unsigned long
-- * **<**: little endian
-- * **>**: big endian
-- * **=**: native endian

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

--- Packs values into string (see summary for more info)
-- @function lpack.pack
-- @string f Format string.
-- @param ... Values to pack.
-- @treturn string Packed string.

--- Unpacks values from a string (see summary for more info).
-- @function lpack.unpack
-- @string s Packed string.
-- @string f Format string.
-- @int[opt] init Initial position.
-- @treturn int Next unread position.
-- @return Value(s) from string.

-- TESTS:

require"pack"

bpack=string.pack
bunpack=string.unpack

function hex(s)
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