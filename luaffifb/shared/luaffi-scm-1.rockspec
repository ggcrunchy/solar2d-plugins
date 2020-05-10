package = "luaffi"
version = "scm-1"

source = {
   url = "git://bitbucket.org/ggcrunchy/corona_plugins.git",
   dir = "luaffifb/shared"
}

description = {
   summary = "FFI library for calling C functions from lua",
   detailed = [[
   ]],
   homepage = "https://github.com/facebook/luaffifb",
   license = "BSD"
}

dependencies = {
   "lua >= 5.1",
}

build = {
   type = "builtin",
   modules = {
      ['ffi'] = {
       --  defines = { "LUA_USE_MACOSX" },
         incdirs = {
            "dynasm"
         },
         sources = {
            "call.c", "ctype.c", "ffi.c", "parser.c",
         }
      },
      ['ffi.libtest'] = {
	--defines = { "LUA_USE_MACOSX" },
	sources = { "test.c" }
      }
   }
}
