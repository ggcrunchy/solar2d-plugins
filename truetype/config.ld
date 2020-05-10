-- ldoc configuration file
file = { "truetype_docs" }
-- TODO: Why aren't my excludes working? ...And then correct those folders when they are.
output = "api"
title = "Documentation"
project = "truetype"
dir = 'docs'
-- readme = "readme.md"
--one = true
no_summary = true
full_description = [[
Corona binding to **stb\_truetype**, from Sean Barrett's [stb](https://github.com/nothings/stb).
	
To use the plugin, add the following in <code>build.settings</code>:
<pre><code class="language-lua">plugins = {  
  ["plugin.truetype"] = { publisherId = "com.xibalbastudios" }
}</code></pre>

On Android, the [AssetReader plugin](https://marketplace.coronalabs.com/corona-plugins/assetreader) may also be included to improve file-reading support:
<pre><code class="language-lua">plugins = {  
  ["plugin.truetype"] = { publisherId = "com.xibalbastudios" },
  ["plugin.AssetReader"] = { publisherId = "com.xibalbastudios" }
}</code></pre>

A `require("plugin.AssetReader")` will then install support. (Down the road, this will ideally be automatic.)

A sample project is available [here](https://github.com/ggcrunchy/corona-plugin-docs/tree/master/truetype_sample).

The **Bytes** type&mdash;specified in various functions and methods&mdash;may be any object that implements [ByteReader](https://ggcrunchy.github.io/corona-plugin-docs/DOCS/ByteReader/policy.html),
including strings.

This plugin is designed to be called safely from other [Lua processes](https://ggcrunchy.github.io/corona-plugin-docs/DOCS/luaproc/api.html).

**Adapted from stb_truetype.h:**

   This library processes TrueType files:
        parse files
        extract glyph metrics
        extract glyph shapes
        render glyphs to one-channel bitmaps with antialiasing (box filter)

   Some important concepts to understand to use this library:

      Codepoint
         Characters are defined by Unicode codepoints, e.g. 65 is
         uppercase A, 231 is lowercase c with a cedilla, 0x7e30 is
         the hiragana for "ma".

      Glyph
         A visual character shape (every codepoint is rendered as
         some glyph).

      Glyph index
         A font-specific integer ID representing a glyph.

      Baseline
         Glyph shapes are defined relative to a baseline, which is the
         bottom of uppercase characters. Characters extend both above
         and below the baseline.

      Current Point
         As you draw text to the screen, you keep track of a "current point"
         which is the origin of each character. The current point's vertical
         position is the baseline. Even "baked fonts" use this model.

      Vertical Font Metrics
         The vertical qualities of the font, used to vertically position
         and space the characters. See docs for FontInfo:GetFontVMetrics.

      Font Size in Pixels or Points
         The preferred interface for specifying font sizes in stb_truetype
         is to specify how tall the font's vertical extent should be in pixels.
         If that sounds good enough, skip the next paragraph.

         Most font APIs instead use "points", which are a common typographic
         measurement for describing font size, defined as 72 points per inch.
         stb_truetype provides a point API for compatibility. However, true
         "per inch" conventions don't make much sense on computer displays
         since different monitors have different number of pixels per inch.
		 For example, Windows traditionally uses a convention that there are
		 96 pixels per inch, thus making 'inch' measurements have nothing to
		 do with inches, and thus effectively defining a point to be 1.333
		 pixels. Additionally, the TrueType font data provides an explicit
		 scale factor to scale a given font's glyphs to points, but the
		 author has observed that this scale factor is often wrong for
		 non-commercial fonts, thus making fonts scaled in points according
		 to the TrueType spec incoherently sized in practice.

**ADVANCED USAGE**

   **Quality**:

    - Use the functions with Subpixel at the end to allow your characters
      to have subpixel positioning. Since the font is anti-aliased, not
      hinted, this is very import for quality. (This is not possible with
      baked fonts.)

    - Kerning is now supported, and if you're supporting subpixel rendering
      then kerning is worth using to give your text a polished look.

   **Performance**:

    - Convert Unicode codepoints to glyph indexes and operate on the glyphs;
      if you don't do this, stb_truetype is forced to do the conversion on
      every call.

    - There are a lot of memory allocations. We should modify it to take
      a temp buffer and allocate from the temp buffer (without freeing),
      should help performance a lot.

This software is dual-licensed to the public domain and under the following
license: you are granted a perpetual, irrevocable license to copy, modify,
publish, and distribute this file as you see fit.
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