--- Class that represents font info.
--
-- Many methods have both Unicode and glyph variants. The full signatures of the Unicode versions are shown; the
-- glyph versions are elided but structurally identical to their Unicode counterparts. (Both in fact take **uint**
-- arguments, cf. @{FontInfo:FindGlyphIndex}.)
-- @classmod FontInfo

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

--- If you're going to perform multiple operations on the same character
-- and you want a speed-up, call this function with the character you're
-- going to process, then use glyph-based functions instead of the
-- codepoint-based functions.
-- @function FontInfo:FindGlyphIndex
-- @uint codepoint Unicode point.
-- @treturn uint Index of glyph in this font.

--- Allocates a large-enough single-channel 8bpp bitmap and renders the
-- specified character at the provided scale into it, with antialiasing.
-- Bitmap values range from 0, for no coverage (transparent), to 255 when fully
-- covered (opaque).
--
-- The bitmap is stored left-to-right, top-to-bottom.
--
-- If either _xscale_ or _yscale_ is 0, it will be assigned the value of the other. (Both being 0 is an error.)
-- @function FontInfo:GetCodepointBitmap
-- @number xscale How much to scale the character's width...
-- @number yscale ...and height.
-- @uint codepoint Unicode point.
-- @param how How to return the bitmap.
-- @treturn[1] ?|Bytes|string If _how_ is **"as\_bytes"**, the internally allocated bitmap is given some **Bytes**
-- machinery and returned directly, which should avoid some garbage; otherwise, a copy of its contents are returned
-- as a string.
-- @treturn[1] uint Width of the bitmap...
-- @treturn[1] uint ...and height.
-- @treturn[1] int Offset in pixel space from the glyph origin to the top-left corner of the bitmap, x-coordinate...
-- @treturn[1] int ...and y-coordinate.
-- @return[2] **nil**, indicating failure.
-- @see FontInfo:GetCodepointBitmapBox

--- Get the bounding box of the bitmap, centered around the glyph origin. Following the snippet
-- below, the bitmap has width of `ix1 - ix0` and height `iy1 - iy0`; the location to place the
-- bitmap's top-left corner is (_ix0_, _iy0_). (**N.B.** the lower-right corner is actually just
-- outside the bitmap.)
--
-- Note that the bitmap uses y-increases-down, but the shape uses
-- y-increases-up, so this and @{FontInfo:GetCodepointBox} are inverted.
--
-- This is a convenience wrapper for the following:
--
-- <pre><code>local ok, x0, y0, x1, y1 = font:GetCodepointBox(codepoint)&#13;&#10;
--if ok then
--  local ix0, iy0 = math.floor(x0 * xscale), math.floor(-y1 * yscale)
--  local ix1, iy1 = math.ceil(x1 * xscale), math.ceil(-y0 * yscale)&#13;&#10;
--  return ix0, iy0, ix1, iy1
--else
--  return 0, 0, 0, 0
--end</code></pre>
-- @function FontInfo:GetCodepointBitmapBox
-- @uint codepoint Unicode point.
-- @number xscale How much to scale the character's width...
-- @number yscale ...and height.
-- @treturn uint Upper-left corner, x-coordinate...
-- @treturn uint ...and y-coordinate.
-- @treturn uint Lower-right corner, x-coordinate...
-- @treturn uint ...and y-coordinate.
-- @see FontInfo:GetCodepointBitmapBoxSubpixel

--- Variant of @{FontInfo:GetCodepointBitmapBox} that accepts a subpixel shift.
--
-- This is a convenience wrapper for the following:
--
-- <pre><code>local ok, x0, y0, x1, y1 = font:GetCodepointBox(codepoint)&#13;&#10;
--if ok then
--  local ix0, iy0 = math.floor(x0 * xscale + xshift), math.floor(-y1 * yscale + yshift)
--  local ix1, iy1 = math.ceil(x1 * xscale + xshift), math.ceil(-y0 * yscale + yshift)&#13;&#10;
--  return ix0, iy0, ix1, iy1
--else
--  return 0, 0, 0, 0
--end</code></pre>
-- @function FontInfo:GetCodepointBitmapBoxSubpixel
-- @uint codepoint Unicode point.
-- @number xscale How much to scale the character's width...
-- @number yscale ...and height.
-- @number xshift Subpixel shift of scaled x-coordinate...
-- @number yshift ...and y-coordinate.
-- @treturn uint Upper-left corner, x-coordinate...
-- @treturn uint ...and y-coordinate.
-- @treturn uint Lower-right corner, x-coordinate...
-- @treturn uint ...and y-coordinate.

--- Variant of @{FontInfo:GetCodepointBitmap} that accepts a subpixel shift, cf.
-- @{FontInfo:GetCodepointBitmapBoxSubpixel} for details.
-- @function FontInfo:GetCodepointBitmapSubpixel
-- @number xscale How much to scale the character's width...
-- @number yscale ...and height.
-- @number xshift Subpixel shift of scaled x-coordinate...
-- @number yshift ...and y-coordinate.
-- @uint codepoint Unicode point.
-- @ptable[opt] opts As per @{FontInfo:GetCodepointBitmap}.
-- @treturn[1] ?|Bytes|string As per @{FontInfo:GetCodepointBitmap}.
-- @treturn[1] uint Width of the bitmap...
-- @treturn[1] uint ...and height.
-- @treturn[1] int Offset in pixel space from the glyph origin to the top-left corner of the bitmap, x-coordinate...
-- @treturn[1] int ...and y-coordinate.
-- @return[2] **nil**, indicating failure.

--- Gets the bounding box of the visible part of the glyph, in unscaled coordinates.
-- @function FontInfo:GetCodepointBox
-- @uint codepoint Unicode point.
-- @return[1] **true**, indicating success.
-- @treturn[1] uint Upper-left corner, x-coordinate...
-- @treturn[1] uint ...and y-coordinate.
-- @treturn[1] uint Lower-right corner, x-coordinate...
-- @treturn[1] uint ...and y-coordinate.
-- @return[2] **false**, meaning failure.

--- Get various horizontal metrics for a particular codepoint in the font.
-- @function FontInfo:GetCodepointHMetrics
-- @treturn uint Advance width, the offset from the current horizontal position to the next horizontal position,
-- expressed in unscaled coordinates.
-- @treturn int Left side bearing, the offset from the current horizontal position to the left edge of the character.

--- Find the [kerning](https://en.wikipedia.org/wiki/Kerning) between two adjacent characters, an additional
-- amount to add to the 'advance' value.
-- @function FontInfo:GetCodepointKernAdvance
-- @uint char_index_1 First Unicode point...
-- @uint char_index_2 ...and second.
-- @treturn int Kerning.

--- Get the series of contours that describe the character.
-- @function FontInfo:GetCodepointShape
-- @uint codepoint Unicode point.
-- @treturn ?|Shape|nil On success, the character's shape; otherwise **nil**.

--- Get the bounding box around all possible characters.
-- @function FontInfo:GetFontBoundingBox
-- @treturn uint Upper-left corner, x-coordinate...
-- @treturn uint ...and y-coordinate.
-- @treturn uint Lower-right corner, x-coordinate...
-- @treturn uint ...and y-coordinate.

--- Get various vertical metrics from the font.
--
-- You should advance the vertical position by `ascent - descent + line_gap`. These are expressed in
-- unscaled coordinates, so you must multiply by the scale factor for a given size.
-- @function FontInfo:GetFontVMetrics
-- @treturn int Ascent, the coordinate above the baseline the font extends.
-- @treturn int Descent, the coordinate below the baseline the font extends (i.e. it is typically negative).
-- @treturn int Line gap, the spacing between one row's descent and the next row's ascent...

--- Like @{FontInfo:GetCodepointBitmap}, but takes a @{FontInfo:FindGlyphIndex|glyph index} rather than a codepoint.
-- @function FontInfo:GetGlyphBitmap

--- Like @{FontInfo:GetCodepointBitmapBox} but takes a @{FontInfo:FindGlyphIndex|glyph index} rather than a codepoint.
-- @function FontInfo:GetGlyphBitmapBox

--- Like @{FontInfo:GetCodepointBitmapBoxSubpixel}, but takes a @{FontInfo:FindGlyphIndex|glyph index} rather than a codepoint.
-- @function FontInfo:GetGlyphBitmapBoxSubpixel

--- Like @{FontInfo:GetCodepointBitmapSubpixel}, but takes a @{FontInfo:FindGlyphIndex|glyph index} rather than a codepoint.
-- @function FontInfo:GetGlyphBitmapSubpixel

--- Like @{FontInfo:GetCodepointBox}, but takes a @{FontInfo:FindGlyphIndex|glyph index} rather than a codepoint.
-- @function FontInfo:GetGlyphBox

--- Like @{FontInfo:GetCodepointHMetrics}, but takes a @{FontInfo:FindGlyphIndex|glyph index} rather than a codepoint.
-- @function FontInfo:GetGlyphHMetrics

--- Like @{FontInfo:GetCodepointKernAdvance}, but takes @{FontInfo:FindGlyphIndex|glyph indices} rather than codepoints.
-- @function FontInfo:GetGlyphKernAdvance

--- Like @{FontInfo:GetCodepointShape}, but takes a @{FontInfo:FindGlyphIndex|glyph index} rather than a codepoint.
-- @function FontInfo:GetGlyphShape

--- Predicate.
-- @function FontInfo:IsGlyphEmpty
-- @uint glyph_index @{FontInfo:FindGlyphIndex|Glyph index}.
-- @treturn boolean Is anything drawn for this glyph?

--- This is like @{FontInfo:GetCodepointBitmap}, but you pass in storage for the bitmap
-- in the form of _output_, with row spacing of _stride_ bytes. The bitmap is clipped to the output
-- region, if necessary. Call @{FontInfo:GetCodepointBitmapBox} to get the width, height, and positioning
-- info first; see also its details on scaling.
-- @function FontInfo:MakeCodepointBitmap
-- @tparam MemoryBlob output [Blob](https://ggcrunchy.github.io/corona-plugin-docs/DOCS/MemoryBlob/api.html) that will
-- receive the bitmap.
-- @uint ow Output bitmap width...
-- @uint oh ...and height.
-- @number xscale How much to scale the character's width...
-- @number yscale ...and height.
-- @uint codepoint Unicode point.
-- @ptable[opt] opts Make options, which include:
--
-- * **stride**: Bytes per row, defaulting to 0 (a synonym for _ow_).
-- * **x**: Horizontal offset into blob... (Default 0.)
-- * **y**: ...and vertical offsset. (Ditto.)
-- @treturn boolean Was a blob provided and could the output region be placed inside it (resizing, if necessary and possible)?

--- Variant of @{FontInfo:MakeCodepointBitmap} that accepts a subpixel shift, cf.
-- @{FontInfo:GetCodepointBitmapBoxSubpixel} for details.
-- @function FontInfo:MakeCodepointBitmapSubpixel
-- @uint ow Output bitmap width...
-- @uint oh ...and height.
-- @number xscale How much to scale the character's width...
-- @number yscale ...and height.
-- @number xshift Subpixel shift of scaled x-coordinate...
-- @number yshift ...and y-coordinate.
-- @uint codepoint Unicode point.
-- @ptable[opt] opts As per @{FontInfo:MakeCodepointBitmap}.
-- @treturn ?|Bytes|string

--- Like @{FontInfo:MakeCodepointBitmap}, but takes a @{FontInfo:FindGlyphIndex|glyph index} rather than a codepoint.
-- @function FontInfo:MakeGlyphBitmap

--- Like @{FontInfo:MakeCodepointBitmapSubpixel}, but takes a @{FontInfo:FindGlyphIndex|glyph index} rather than a codepoint.
-- @function FontInfo:MakeGlyphBitmapSubpixel

--- Computes a scale factor&mdash;from our actual font to an ideal one&mdash;according to the height of "M".
-- @function FontInfo:ScaleForMappingEmToPixels
-- @number pixels Desired pixel height.
-- @treturn number Scale factor, where `pixels = scale * height("M")`.

--- Compute a scale factor&mdash;from our actual font to an ideal one&mdash;according to the distance from the
-- highest ascender to the lowest descender.
--
-- In other words, it's equivalent to calling @{FontInfo:GetFontVMetrics} and
-- computing `scale = pixels / (ascent - descent)`, so if you prefer to measure
-- height by the ascent only, use a similar calculation.
-- @function FontInfo:ScaleForPixelHeight
-- @number pixels Desired pixel height.
-- @treturn number Scale factor, as described above.