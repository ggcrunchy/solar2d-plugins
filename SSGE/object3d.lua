--- Basic utility for rendering a batch of 3-D polygons (an "object") to a texture, say for use in fills.
--
-- To use the plugin, add the following in <code>build.settings</code>:
--
-- <pre><code class="language-lua">plugins = {
--   ["plugin.object3d"] = { publisherId = "com.xibalbastudios" },
--   ["plugin.MemoryBlob"] = { publisherId = "com.xibalbastudios" }
-- }</code></pre>
--
-- As shown, the (free) [MemoryBlob](https://marketplace.coronalabs.com/corona-plugins/memory-blob) is also a dependency.
-- While not strictly necessary, the (also free) [Bytemap](https://marketplace.coronalabs.com/corona-plugins/byte-map)
-- plugin is the recommended way to provide the texture to display objects.
--
-- Sample code is available [here](https://github.com/ggcrunchy/corona-plugin-docs/tree/master/object3d_sample).
--
-- The **Bytes** type&mdash;specified in a few of the bytemap methods&mdash;may be any object that implements [ByteReader](https://ggcrunchy.github.io/corona-plugin-docs/DOCS/ByteReader/policy.html),
-- including strings.
--
-- The current implementation is a veneer over a somewhat customized **tinyrenderer**.
--
-- **From tinyrenderer's project page:**
--
-- Tiny Renderer, <https://github.com/ssloy/tinyrenderer>
-- Copyright Dmitry V. Sokolov
--
-- This software is provided 'as-is', without any express or implied warranty.
-- In no event will the authors be held liable for any damages arising from the use of this software.
-- Permission is granted to anyone to use this software for any purpose,
-- including commercial applications, and to alter it and redistribute it freely,
-- subject to the following restrictions:
--
-- 1. The origin of this software must not be misrepresented; you must not claim that you wrote the original software. If you use this software in a product, an acknowledgment in the product documentation would be appreciated but is not required.
-- 2. Altered source versions must be plainly marked as such, and must not be misrepresented as being the original software.
-- 3. This notice may not be removed or altered from any source distribution.

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

--- Create a new **Object3d**.
--
-- SURFACES: TEXTURE, Z-BUFFER
-- CAMERA: EYE, LIGHT DIR, LOOK AT, UP
-- SCENE: VERTEX, NORMAL, UV STREAMS
-- @function New
-- @int w Texture width...
-- @int h ...and height.
-- @bool has_alpha Include an alpha channel? (Unnecessary if the background will be filled.)
-- @treturn ?|Object3d|nil Object interface, or **nil** on error.

--- DOCME
-- @function Object3d:AddFace
-- @param ... int triples Indices

--- DOCME
-- @function Object3d:AddNormal
-- @number x
-- @number y
-- @number z

--- DOCME
-- @function Object3d:AddUV
-- @number u
-- @number v

--- DOCME
-- @function Object3d:AddVertex
-- @number x
-- @number y
-- @number z

--- Wipe the texture (to black or clear) and z-buffer.
-- @function Object3d:Clear

--- Get the underlying (fixed-size) [memory blob](https://ggcrunchy.github.io/corona-plugin-docs/DOCS/MemoryBlob/api.html), e.g.
-- to perform blob-specific operations on it.
-- @function Object3d:GetBlob
-- @treturn MemoryBlob Blob.

--- Copy the current texture contents to a string.
-- @function Object3d:GetBytes
-- @treturn string A string of size `w * h * ncomps`, where _w_ and _h_ were supplied to @{New} and _ncomps_ is
-- 3 or 4, the latter when _has\_alpha_ was supplied.

--- Attempt to intersect a segment with a face and get the color where it hits.
-- @function Object3d:GetColor
-- @number px First endpoint of segment, x-coordinate...
-- @number py ...y...
-- @number pz ...and z.
-- @number qx Second endpoint, x-coordinate...
-- @number qy ...y...
-- @number qz ...and z.
-- @treturn[1] r Red component at hit...
-- @treturn[1] g ...green component...
-- @treturn[1] b ...red component...
-- @treturn[1] a ...and alpha component. (Only returned if this channel exists. TODO: diffuse, normal, etc.)
-- @return[2] **nil**, meaning the segment never intersected the face.

--- DOCME
-- @function Object3d:GetFaceVertexIndices
-- @uint index
-- @return ... face indices

--- DOCME
-- @function Object3d:GetVertex
-- @uint index
-- @treturn number Vertex x-coordinate...
-- @treturn number ...y...
-- @treturn number ...and z.

--- Metamethod.
-- @function Object3d:__len
-- @treturn uint Face count.

--- Render the current scene, clearing the previous contents first.
-- @function Object3d:Render
-- @see Object3d:Clear

--- Set the center position. (TODO: lookat)
-- @function Object3d:SetCenter
-- @number Center x-coordinate...
-- @number ...y...
-- @number ...and z.

--- DOCME
-- @function Object3d:SetDiffuse
-- @tparam ?|Bytes|nil how "uvs", bytes, or nil to clear
-- @uint w
-- @uint h
-- @uint[opt=3] comps

--- Set the eye position. (TODO: lookat)
-- @function Object3d:SetEye
-- @number Eye x-coordinate...
-- @number ...y...
-- @number ...and z.

--- Set the light direction. (TODO: normal)
-- @function Object3d:SetLightDir
-- @number Direction's x-component...
-- @number ...y...
-- @number ...and z.

--- DOCME
-- @function Object3d:SetNormalMap
-- @tparam ?|Bytes|nil bytes or nil to clear
-- @uint w
-- @uint h
-- @uint[opt=3] comps

--- Set the up vector. (TODO: lookat)
-- @function Object3d:SetUp
-- @number Vector's x-component...
-- @number ...y...
-- @number ...and z.