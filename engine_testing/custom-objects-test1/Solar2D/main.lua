--- Method customization test.

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

local co1 = require("plugin.customobjects1")

local gg = co1.newScopedGroupObject() -- this is a group, but will send a "willDraw" message to all its children, in
                                      -- order, before it draws, and (in reverse) "didDraw" to all of them afterward

local a1 = co1.newPrintObject(gg) -- print objects are display objects in name only: their culling and hit test logic
                                  -- is dummied out, and their "draw" logic involves printing their `drawMessage`, if one
                                  -- was provided...

a1.drawMessage = "a1:draw" -- ...like so

local a2 = co1.newPrintObject(gg)

a2.willDrawMessage = "a2:willDraw" -- they also listen for "willDraw" and "didDraw" messages (as sent by the scoped
                                   -- group object), and will print the corresponding `*Message`, as provided here...

a2.didDrawMessage = "a2:didDraw" -- ...and here

-- (These print objects might have some utility in debugging, but are mostly intended as a very basic demo of the
-- "custom objects" machinery, i.e. being able to invoke plugin-based behavior within the render hierarchy. More
-- sophisticated use cases will be forthcoming as new features roll out.)

local a3 = co1.newPrintObject(gg)

a3.willDrawMessage = "a3:willDraw"
a3.drawMessage = "a3:draw"

local a4 = co1.newPrintObject(gg)

a4.drawMessage = "a4:draw"
a4.didDrawMessage = "a4:didDraw"

local a5 = co1.newPrintObject(gg)

a5.didDrawMessage = "a5:didDraw"

local a6 = co1.newPrintObject(gg)

a6.willDrawMessage = "a6:willDraw"
a6.drawMessage = "a6:draw"
a6.didDrawMessage = "a6:didDraw"

timer.performWithDelay(20, function()
    a2.x = math.random(20, 40) -- alter an arbitrary "geometric" property so that the scene "draws" again
end, 0)