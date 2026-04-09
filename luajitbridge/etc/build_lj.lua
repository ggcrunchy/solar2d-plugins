--- Build lj.h from LuaJIT headers.

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

local dir = "path/to/luajit/src/dir/"

--
--
--

local function ChangeLuaToLJ (part)
  part = part:gsub("LUA", "LJ")
  part = part:gsub("lua", "lj")

  return part
end

local function AddDefines (contents, t)
  for def in contents:gmatch("#define%s+LUA_%w+%s+[^\n]*\n") do
    if not def:find('"') then -- all string-related #defines irrelevant
      t[#t + 1] = ChangeLuaToLJ(def)
    end
  end
end

--
--
--

local function GetStructName (part)
  return part:match("struct%s+([_%w]+)")
end

--
--
--

local function AddTypedefs (contents, t1, t2, structs)
  for _, v in ipairs{
    { t = t1, with_braces = false, pattern = "typedef[^{;]+;" },
    { t = t2, with_braces = true, pattern = "typedef[^{]+%b{}[^;]+;" }
  } do
    for tdef in contents:gmatch(v.pattern) do
      local name = GetStructName(tdef)
      
      if name and v.with_braces then
        structs[name] = true
      end
  
      v.t[#v.t + 1] = ChangeLuaToLJ(tdef)
    end
  end
end

--
--
--

local function AddMacros (contents, sl, ml)
  local pos = 1
  
  repeat
    local def, multiline, m1, m2 = "", false, contents:find("#define%s+lua[_%w]+%b()[^\n]*\n", pos)

    while m2 do
      local line = contents:sub(m1, m2)

      def = def .. line

      if line:find("\\") then
        multiline = true
        m1, m2 = contents:find("[^\n]+\n", m2 + 1)
      else
        local t = multiline and ml or sl
        
        t[#t + 1] = ChangeLuaToLJ(def)
        pos, m2 = m2 + 1
      end
    end
  until not m1
end

--
--
--

local function AddRemainingStructs (contents, t, structs)
  for sdef in contents:gmatch("struct[^{;]+%b{}[^;]*;") do
    local name = GetStructName(sdef)
      
    if not structs[name] then
      t[#t + 1] = ChangeLuaToLJ(sdef)
    end
  end
end

--
--
--

local function AddFunctions (contents, fns, fwds)
  for _, v in ipairs{
    { pattern = "LUA%a*_API[^%(]+%b()%s*%b();" }, -- with parens
    { pattern = "LUA%a*_API[^%(]+lua[_%w]+%s*%b();" }
  } do
    for def in contents:gmatch(v.pattern) do
      local _, api_end = def:find("_API%s+")
      local _, const_end = def:find("^const%s+", api_end + 1)
      local tpos = (const_end or api_end) + 1
      local _, type_end, tname = def:find("([_%w]+)%s+", tpos)
      local has_rets = tname ~= "void" or def:sub(type_end + 1, type_end + 1) == "*"
      local _, _, fname, args = def:find("lua([_%w]+)%)?%s*%(([^%)]+)%)", type_end + 1)
      local lj_def = ChangeLuaToLJ(def)

      fns[#fns + 1] = lj_def

      local call = ""
      local _, _, what = args:find("^%s*([^%s]+)%s*$")
      
      if what ~= "void" then
        local inputs = {}
      
        args = args .. ")"
      
        for name in args:gmatch("([_%w]+)%s*[,%)]") do
          inputs[#inputs + 1] = name
        end
        
        call = "(lua_State *)" .. table.concat(inputs, ", ")
      end

      fwds[#fwds + 1] = lj_def:sub(1, -2) .. " { " .. (has_rets and "return " or "") .. "lua" .. fname .. "(" .. call .. "); }"
    end
  end
end

--
--
--

local typedefed_structs = {}

--
--
--

local defines = {}
local forwards = {}
local functions = {}
local multi_line_macros = {}
local single_line_macros = {}
local structs = {}
local struct_typedefs = {}
local typedefs = {}

--
--
--

for _, name in ipairs{ "lua.h", "lualib.h", "lauxlib.h" } do
  local file = io.open(dir .. name, "rt")
  local contents = assert(file and file:read("*a"), "Unable to load " .. name)
  
  file:close()

  --
  --
  --

  AddDefines(contents, defines)
  AddTypedefs(contents, typedefs, struct_typedefs, typedefed_structs)
  AddFunctions(contents, functions, forwards)
  AddMacros(contents, single_line_macros, multi_line_macros)
  AddRemainingStructs(contents, structs, typedefed_structs)
end

--
--
--

local out = io.open(system.pathForFile("lj.h", system.DocumentsDirectory), "wt")

if out then
  out:write[[
#pragma once

#include <stdio.h>

typedef ptrdiff_t LJ_INTEGER;
typedef double LJ_NUMBER;

#define LJ_IDSIZE	60
#define LJL_BUFFERSIZE	(BUFSIZ > 16384 ? 8192 : BUFSIZ)

]]

  out:write(table.concat(typedefs, "\n"), "\n\n")
  out:write(table.concat(struct_typedefs, "\n\n"), "\n\n")
  out:write(table.concat(defines, ""), "\n")
  out:write("#ifdef EXPORTING_LJ\n\n")
  out:write(table.concat(forwards, "\n"), "\n\n")
  out:write("#else\n\n")
  out:write(table.concat(functions, "\n"), "\n\n")
  out:write("#endif\n\n")
  out:write(table.concat(single_line_macros, ""), "\n")
  out:write(table.concat(multi_line_macros, "\n"), "\n")
  out:write(table.concat(structs, "\n\n"))
  out:close()
end