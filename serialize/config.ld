-- ldoc configuration file
file = { "serialize_docs" }
-- TODO: Why aren't my excludes working? ...And then correct those folders when they are.
output = "api"
title = "Documentation"
project = "serialize"
dir = 'docs'
full_description = [[
Bindings to various serialization libraries.
	
To use the plugin, add the following in <code>build.settings</code>:
<pre><code class="language-lua">plugins = {  
  ["plugin.serialize"] = { publisherId = "com.xibalbastudios" }
}
</code></pre>

A sample project is available [here](https://github.com/ggcrunchy/corona-plugin-docs/blob/master/serialize_sample).

A second sample, demonstrating interop with [luaproc](https://marketplace.coronalabs.com/plugin/luaproc), may be
found [here](https://github.com/ggcrunchy/corona-plugin-docs/blob/master/serialize_luaproc_sample).

struct and lpack come courtesy of two of Lua's own authors. These are both
similar in intent; which one to use	will largely be a matter of preference. 
Both pack data into a Lua string and unpack from the same. This allows for
structured data to be sent to APIs, e.g. a C library that expects an array of
floats or small structures.	Memory can be sent directly, rather than
destructuring the data into tables, yet more safely than via blind userdata,
since strings have a definite length.

marshal encodes and decodes data directly, without the aforementioned
libraries' use of an auxiliary data	format specifier. This is useful for
passing around data when human readability is not a concern (as it is with
JSON), say to persist across program runs or to communicate between threads.
]]
-- readme = "readme.md"
--one = true
-- no_summary = true
--examples = {'examples', exclude = {'examples/slow.lua'}}
description = [[ Module documentation. ]]
--no_return_or_parms = true
--manual_url 'file:///D:/dev/lua/projects/lua-5.1.4/doc/manual.html'

format = 'markdown'

-- Type aliases
tparam_alias('bool', 'bool')

tparam_alias('number', 'number')
tparam_alias('byte', 'byte')
tparam_alias('int', 'int')
tparam_alias('uint', 'uint')

tparam_alias('string', 'string')

tparam_alias('callable', 'callable')

tparam_alias('array', 'array')
tparam_alias('ptable', 'table')

tparam_alias('pgroup', 'DisplayGroup')
tparam_alias('pobject', 'DisplayObject')