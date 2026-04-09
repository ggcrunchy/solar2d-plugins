--- Prelude code used by both Lua and LuaJIT parts.

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

local ShimCall, CheckCompatibility = ...

--
--
--

local tostring = tostring

--
--
--

local MetamethodShim = {
  __add = function(obj1, obj2)
    return obj1 + obj2
  end,

  __sub = function(obj1, obj2)
    return obj1 - obj2
  end,

  __mul = function(obj1, obj2)
    return obj1 * obj2
  end,

  __div = function(obj1, obj2)
    return obj1 / obj2
  end,

  __unm = function(obj)
    return -obj
  end,

  __concat = function(obj1, obj2)
    return obj1 .. obj2
  end,

  __len = function(obj)
    return #obj
  end,

  __eq = function(obj1, obj2)
    return obj1 == obj2
  end,

  __lt = function(obj1, obj2)
    return obj1 < obj2
  end,

  __le = function(obj1, obj2)
    return obj1 <= obj2
  end,

  __tostring = tostring,

  __call = function(obj, ...)
    return obj(...)
  end,

  __index = function(obj, k)
    return obj[k]
  end,

  __newindex = function(obj, k, v)
    obj[k] = v
  end
}

--
--
--

local function BinOp (key)
  return function(obj1, obj2)
    CheckCompatibility(obj1, obj2)

    return ShimCall(1, key, obj1, obj2)
  end
end

--
--
--

local function UnaryOp (key)
  return function(obj)
    return ShimCall(1, key, obj)
  end
end

--
--
--

local MULTRET = -1

local ProxyMT = {
  __add = BinOp("__add"),
  __sub = BinOp("__sub"),
  __mul = BinOp("__mul"),
  __div = BinOp("__div"),
  __pow = BinOp("__pow"),
  __concat = BinOp("__concat"),
  __eq = BinOp("__eq"),
  __lt = BinOp("__lt"),
  __le = BinOp("__le"),
  __len = UnaryOp("__len"),
  __unm = UnaryOp("__unm"),
  __tostring = UnaryOp("__tostring"),

  __index = function(obj, k)
    return ShimCall(1, "__index", obj, k)
  end,

  __call = function(...)
    return ShimCall(MULTRET, "__call", ...)
  end,

  __newindex = function(obj, k, v)
    ShimCall(0, "__newindex", obj, k, v)
  end,

  __metatable = "BoxedGCObjectProxy"
}

--
--
--

return ProxyMT, MetamethodShim