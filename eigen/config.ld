-- ldoc configuration file
file = { "eigen_docs" }
-- TODO: Why aren't my excludes working? ...And then correct those folders when they are.
output = "api"
title = "Documentation"
project = "Eigen"
dir = 'docs'
-- readme = "readme.md"
--one = true
--no_summary = true
full_description = [[
Corona bindings to the <a href="http://eigen.tuxfamily.org/index.php?title=Main_Page">Eigen</a> numerical library.
	
To use the plugin, add the following in <code>build.settings</code>:
<pre><code class="language-lua">plugins = {  
  ["plugin.eigen"] = { publisherId = "com.xibalbastudios" }
}</code></pre>

A sample project is available [here](https://github.com/ggcrunchy/corona-plugin-docs/tree/master/eigen_sample).

This plugin is designed to be called safely from other [Lua processes](https://ggcrunchy.github.io/corona-plugin-docs/DOCS/luaproc/api.html).
]]
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