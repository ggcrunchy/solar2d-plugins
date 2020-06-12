/*
* Permission is hereby granted, free of charge, to any person obtaining
* a copy of this software and associated documentation files (the
* "Software"), to deal in the Software without restriction, including
* without limitation the rights to use, copy, modify, merge, publish,
* distribute, sublicense, and/or sell copies of the Software, and to
* permit persons to whom the Software is furnished to do so, subject to
* the following conditions:
*
* The above copyright notice and this permission notice shall be
* included in all copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*
* [ MIT license: http://www.opensource.org/licenses/mit-license.php ]
*/

#pragma once

int add_codec (lua_State * L);
int add_context (lua_State * L);
int add_font (lua_State * L);
int add_fontface (lua_State * L);
int add_glyphbuffer (lua_State * L);
int add_gradient (lua_State * L);
int add_image (lua_State * L);
int add_path (lua_State * L);
int add_pattern (lua_State * L);

struct BLContextCore * GetContext (lua_State * L, int arg = 1, bool * intact_ptr = nullptr);
struct BLFontCore * GetFont (lua_State * L, int arg = 1, bool * intact_ptr = nullptr);
struct BLFontFaceCore * GetFontFace (lua_State * L, int arg = 1, bool * intact_ptr = nullptr);
struct BLGlyphBufferCore * GetGlyphBuffer (lua_State * L, int arg = 1, bool * intact_ptr = nullptr);
struct BLGradientCore * GetGradient (lua_State * L, int arg = 1, bool * intact_ptr = nullptr);
struct BLImageCodecCore * GetImageCodec (lua_State * L, int arg = 1, bool * intact_ptr = nullptr);
struct BLImageCore * GetImage (lua_State * L, int arg = 1, bool * intact_ptr = nullptr);
struct BLPathCore * GetPath (lua_State * L, int arg = 1, bool * intact_ptr = nullptr);
struct BLPatternCore * GetPattern (lua_State * L, int arg = 1, bool * intact_ptr = nullptr);

bool IsGradient (lua_State * L, int arg);
bool IsImage (lua_State * L, int arg);
bool IsPath (lua_State * L, int arg);
bool IsPattern (lua_State * L, int arg);