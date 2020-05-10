-- ldoc configuration file
file = { "MemoryBlob_docs" }
-- TODO: Why aren't my excludes working? ...And then correct those folders when they are.
output = "api"
title = "Documentation"
project = "MemoryBlob"
dir = 'docs'
-- readme = "readme.md"
--one = true
-- no_summary = true
--examples = {'examples', exclude = {'examples/slow.lua'}}
full_description = [[
Auxiliary library for creating memory blobs, for instance to recycle or share native resources.

Some concurrency primitives are provided, built upon [concurrentqueue](https://github.com/cameron314/concurrentqueue) and [libcuckoo](https://github.com/efficient/libcuckoo).

To use the plugin, add the following in <code>build.settings</code>:

<pre><code class="language-lua">plugins = {
  ["plugin.MemoryBlob"] = { publisherId = "com.xibalbastudios" }
}</code></pre>

Sample code is available [here](https://github.com/ggcrunchy/corona-plugin-docs/tree/master/MemoryBlob_sample).

Once this plugin has been loaded, native plugins may create and manage blobs via the [BlobXS API](https://raw.githubusercontent.com/ggcrunchy/corona_enterprise_utils/master/utils/Blob.h).
Until then, any calls to the API will be suitable no-ops. (Documentation is lacking at the moment!)

Blobs implement the [ByteReader protocol](https://ggcrunchy.github.io/corona-plugin-docs/DOCS/ByteReader/policy.html).

The **Bytes** type&mdash;specified in a few of the blob methods&mdash;may be any object that implements **ByteReader**,
including strings.

Functions and sections that begin with (**WIP**) describe work in progress. These features are not considered stable,
but give a reasonable idea of what to expect.

========================================================================

**From [concurrentqueue's page](https://github.com/cameron314/concurrentqueue#license):**

I'm releasing the source of this repository... under a simplified BSD license. I'm also dual-licensing under the
Boost Software License. See the [LICENSE.md](https://github.com/cameron314/concurrentqueue/blob/master/LICENSE.md) file for more details.

========================================================================

**From [libcuckoo's license](https://github.com/efficient/libcuckoo#licence):**

Copyright (C) 2013, Carnegie Mellon University and Intel Corporation

Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file except in compliance
with the License. You may obtain a copy of the License at

  [Apache License 2.0](http://www.apache.org/licenses/LICENSE-2.0)

Unless required by applicable law or agreed to in writing, software distributed under the License is distributed
on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the License for
the specific language governing permissions and limitations under the License.

========================================================================

**From the [license of CityHash](https://raw.githubusercontent.com/efficient/libcuckoo/master/cityhash-1.1.1/COPYING)
(used by libcuckoo)**:

Copyright (c) 2011 Google, Inc.

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.

========================================================================
]]
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