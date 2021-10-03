--- Test driver for streamlines.

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

local streamlines = require("plugin.streamlines")

graphics.defineEffect{
    category = "generator", name = "annotations1",

    fragment = [[
        P_COLOR vec4 FragmentKernel (P_UV vec2 uv)
        {
            P_COLOR vec3 color = mix(vec3(0.0, 0.0, 0.8), vec3(0.0, 0.8, 0.0), uv.x);
            return vec4(color, 1);
        }
    ]]
}

graphics.defineEffect{
    category = "generator", name = "annotations2",

    fragment = [[
        P_COLOR vec4 FragmentKernel (P_UV vec2 uv)
        {
        #define radius 15.
        #define radius2 (15. * 15.)
        
            P_UV float dist = uv.x;

        #if defined(GL_ES)
            P_UV float x = dist - radius;
            P_UV float y = uv.y * radius;
            P_UV float alpha = 1.0 - step(dist, radius) * smoothstep(radius2 - .025, radius2, x * x + y * y);
        #else
            P_UV float alpha = 1.0;

            if (dist < radius)
            {
                P_UV float x = dist - radius;
                P_UV float y = uv.y * radius;
                P_UV float d2 = x * x + y * y;
                P_UV float t = fwidth(d2);

                alpha = 1.0 - 0.99 * smoothstep(radius2 - t, radius2 + t, d2);
            }
        #endif

            return vec4(alpha);
        }
    ]]
}

--
---[[
local flags = "ANNOTATIONS"
--]]
---[[
local u_mode = "DISTANCE"
--]]

local context = streamlines.create_context{
    thickness = 15, flags = flags, u_mode = u_mode
}

local result = context:mesh_from_lines{
    vertices = {
        50, 150, 200, 100, 550, 200,
        400, 200, 400, 100
    },
    spine_lengths = { 3, 2 }
    --[[
    , closed = true
    --]]
}
local pp=result:GetPositions()
local ii=result:GetTriangles()
local aa
---[[
    = result:GetAnnotations()
--]]
---[[
for i = 1, #ii, 3 do
    local i1,i2,i3=ii[i]*2+1,ii[i+1]*2+1,ii[i+2]*2+1
    local x1,y1,x2,y2,x3,y3=pp[i1],pp[i1+1],pp[i2],pp[i2+1],pp[i3],pp[i3+1]
    local xmin,ymin,xmax,ymax=math.min(x1,x2,x3),math.min(y1,y2,y3),math.max(x1,x2,x3),math.max(y1,y2,y3)
    local poly = display.newPolygon((xmin+xmax)/2, (ymin+ymax)/2, {
        x1,y1,x2,y2,x3,y3
    })
    poly:setFillColor(math.random(),math.random(),math.random())
end
--]]
---[[
    local uvs

    if aa then
        uvs = {}

        for _, a in ipairs(aa) do -- TODO: add some GetAnnotations() options?
print("!",a.x,a.y)
            uvs[#uvs + 1] = a.x
            if u_mode == "DISTANCE" then -- err, wrong check... or just getting too messy :P
                uvs[#uvs + 1] = a.y
            else
                uvs[#uvs + 1] = 0
            end
        end
    end

local mesh = display.newMesh{ uvs = uvs, vertices = pp, indices = ii, mode = "indexed", zeroBasedIndices = true }

mesh.x, mesh.y = display.contentCenterX, display.contentCenterY

if uvs then
    if u_mode == "DISTANCE" then
        mesh.fill.effect = "generator.custom.annotations2"
    else
        mesh.fill.effect = "generator.custom.annotations1"
    end
end
--]]
--[[

















parsl_context* context = parsl_create_context({
    .thickness = 15,
    .flags = PARSL_FLAG_ANNOTATIONS,
    .u_mode = PAR_U_MODE_DISTANCE
});




const float radius = 15.0;
const float radius2 = radius * radius;

in vec4 annotation;
out vec4 frag_color;

void main() {
  float dist = annotation.x;
  float alpha = 1.0;
  if (dist < radius) {
      float x = dist - radius;
      float y = annotation.y * radius;
      float d2 = x * x + y * y;
      float t = fwidth(d2);
      alpha = 1.0 - 0.99 * smoothstep(radius2 - t, radius2 + t, d2);
  }
  frag_color = vec4(0, 0, 0, alpha);
}

























parsl_context* context = parsl_create_context({
    .thickness = 15,
    .flags = PARSL_FLAG_ANNOTATIONS | PARSL_FLAG_SPINE_LENGTHS,
    .u_mode = PAR_U_MODE_DISTANCE
});

in vec4 annotation;
in float spine_length;
out vec4 frag_color;

void main() {
  float dist1 = abs(annotation.x);
  float dist2 = spine_length - dist1;
  float dist = min(dist1, dist2);
  // ...see prevous example for remainder...
}






















ayout(location=0) in vec2 position;
layout(location=1) in vec4 annotation;

void main() {
  vec2 spine_to_edge = annotation.zw;
  float wave = 0.5 + 0.5 * sin(10.0 * 6.28318 * annotation.x);
  vec2 p = position + spine_to_edge * 0.01 * wave;
  gl_Position = vec4(p, 0.0, 1.0);
}

// Displace p by adding noise in the correct direction:
p += annotation.y * annotation.zw * noise(freq * annotation.x);















void advect(parsl_position* point, void* userdata) {
    point->x += ...;
    point->y += ...;
}

void init() {

    parsl_context* context = parsl_create_context({
        .thickness = 3,
        .streamlines_seed_spacing = 20,
        .streamlines_seed_viewport = { left, top, right, bottom },
        .flags = PARSL_FLAG_ANNOTATIONS
    });

    parsl_mesh* mesh = parsl_mesh_from_streamlines(state->context, advect,
           0, 100, nullptr);

    ...
}







uniform float time;
in vec4 annotation;
out vec4 frag_color;

void main() {
  float alpha = annotation.x - time;
  frag_color = vec4(0.0, 0.0, 0.0, fract(alpha));
}






















// parsl_mesh_from_curves_cubic()
//
// The number of vertices in each spine should be 4+(n-1)*2 where n is the
// number of piecewise curves.
//
// Each spine is equivalent to an SVG path that looks like M C S S S.
parsl_mesh* parsl_mesh_from_curves_cubic(parsl_context*, parsl_spine_list);




// parsl_mesh_from_curves_quadratic()
//
// The number of vertices in each spine should be 3+(n-1)*2 where n is the
// number of piecewise curves.
//
// Each spine is equivalent to an SVG path that looks like M Q M Q M Q.
parsl_mesh* parsl_mesh_from_curves_quadratic(parsl_context*, parsl_spine_list);
]]