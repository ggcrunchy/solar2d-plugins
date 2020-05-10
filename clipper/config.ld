-- ldoc configuration file
file = { "clipper_docs" }
-- TODO: Why aren't my excludes working? ...And then correct those folders when they are.
output = "api"
title = "Documentation"
project = "clipper"
dir = 'docs'
-- readme = "readme.md"
--one = true
no_summary = true
full_description = [[
Corona binding of [Clipper](http://www.angusj.com/delphi/clipper.php), a library that performs line & polygon clipping - intersection, union, difference & exclusive-or,
and line & polygon offsetting. The library is based on [Vatti's clipping algorithm](http://en.wikipedia.org/wiki/Vatti_clipping_algorithm).
	
To use the plugin, add the following in <code>build.settings</code>:
<pre><code class="language-lua">plugins = {  
  ["plugin.clipper"] = { publisherId = "com.xibalbastudios" }
}</code></pre>

A sample project is available [here](https://github.com/ggcrunchy/corona-plugin-docs/tree/master/clipper_sample).

This plugin is designed to be called safely from other [Lua processes](https://ggcrunchy.github.io/corona-plugin-docs/DOCS/luaproc/api.html).

**Adapted from the Clipper home page:**

The [Clipper Library](http://www.angusj.com/delphi/clipper/documentation/Docs/_Body.htm) performs @{ClipType|clipping}, and @{Offset|offsetting} of both lines and polygons.

A number of features set Clipper apart from other clipping libraries:

  * it accepts all types of polygons including self-intersecting ones
  * it supports multiple polygon filling rules (EvenOdd, NonZero, Positive, Negative)
  * it's very fast relative to [other libraries](http://www.angusj.com/delphi/clipper.php#features)
  * it's [numerically robust](http://www.mpi-inf.mpg.de/~kettner/pub/nonrobust_cgta_06.pdf)
  * it also performs line and polygon offsetting
  * it's free to use in both freeware and commercial applications

**Version: 6.1.3**

Last updated: 19 January 2014

Freeware for both open source and commercial applications ([Boost Software License](http://www.boost.org/LICENSE_1_0.txt)).

Copyright © 2010-2014 Angus Johnson

<a name="terminology">**Terminology**</a>:

  * **Clipping**: commonly refers to the process of cutting away from a set of 2-dimensional geometric shapes those parts that are outside a rectangular '_clipping_'  window. This can be achieved by _intersecting_ subject paths (lines and polygons) with a clipping rectangle. In a more general sense, the clipping window need not be rectangular but can be any type of polygon, even multiple polygons. Also, while clipping typically refers to an _intersection_ operation, in this documentation it will refer to any one of the @{ClipType|four boolean operations} (intersection, union, difference and exclusive-or).
  * **Path**: is an ordered sequence of vertices defining a single geometric contour that's either a line (an open path) or a polygon (a closed path).
  * **Line**: or polyline is an open path containing 2 or more vertices.
  * **Polygon**: commonly refers to a two-dimensional region bounded by an outer non-intersecting closed contour. That region may also contain a number of 'holes'. In this documentation however, polygon will simply refer to a _closed_ path.
  * **Contour**: synonymous with path.
  * **Hole**: is a closed region within a polygon that's not part of the polygon. A 'hole polygon' is a closed path that forms the outer boundaries of a hole.
  * **Polygon Filling Rule**: the @{PolyFillType|filling rule}, together with a list of closed paths, defines those regions (bounded by paths) that are _inside_ (ie regions 'brush filled' in a graphical display) and those which are outside (ie 'holes').

**References**:

The Library is based on but significantly extends Bala Vatti's polygon clipping algorithm as described in ["A generic solution to polygon clipping"](http://portal.acm.org/citation.cfm?id=129906), Communications of the ACM, Vol 35, Issue 7 (July 1992) pp 56-63.

A section in ["Computer graphics and geometric modeling: implementation and algorithms"](http://books.google.com/books?q=vatti+clipping+agoston) by By Max K. Agoston (Springer, 2005) discussing _Vatti Polygon Clipping_ was also helpful in creating the initial Clipper implementation.

The paper titled ["Polygon Offsetting by Computing Winding Numbers"](http://www.me.berkeley.edu/~mcmains/pubs/DAC05OffsetPolygon.pdf) by Chen & McMains (Paper no. DETC2005-85513, ASME 2005. Pages 565-575) contains helpful discussion on the complexities of polygon offsetting together with some solutions.  
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