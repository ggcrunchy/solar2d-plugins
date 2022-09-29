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

#include "Bytemap.h"
#include "Bytes.h"
#include "utils/LuaEx.h"
#include <algorithm>
#include <cmath>

static float LinearMask (int x, int y, int hw, int hh)
{
	return 1.f - (std::max)(abs(float(x - hw) / hw), abs(float(y - hh) / hh));
}

static float RadialMask (int x, int y, int hw, int hh)
{
	float xr = float(x - hw) / hw, yr = float(y - hh) / hh;
	float sqr = xr * xr + yr * yr;
	float xv = 1.f - sqr; // cf. http://plunk.org/~hatch/rightway.html (1 - sqrt(1 - x))

	return (std::max)(xv / (sqrt(sqr) + 1.f), 0.f);
}

// Self-blend the bytemap to make it seamless
int Bytemap_MakeSeamless (lua_State * L)
{
	Bytemap * bmap = LuaXS::DualUD<Bytemap>(L, 1, BYTEMAP_NAME);
	bool bLinear = lua_type(L, 2) == LUA_TSTRING && strcmp(lua_tostring(L, 2), "linear") == 0;

	if (bmap)
	{
		float (*mask)(int, int, int, int) = bLinear ? LinearMask : RadialMask;

		lua_pushcfunction(L, Bytemap_GetBytes); // bmap[, how], GetBytes
		lua_pushvalue(L, 1); // bmap[, how]GetBytes, bmap
		lua_call(L, 1, 1); // bmap[, how], bytes

		const unsigned char * bytes = reinterpret_cast<const unsigned char *>(lua_tostring(L, -1));

		int halfw = bmap->mW / 2, oddw = bmap->mW % 2;
		int halfh = bmap->mH / 2, oddh = bmap->mH % 2;

		std::vector<unsigned char> result(lua_objlen(L, -1));
		unsigned char * out = result.data();
		int bpp = CoronaExternalFormatBPP(bmap->mFormat), bytes_per_row = bmap->mW * bpp;

		for (int y1 = 0; y1 < bmap->mH; ++y1)
		{
			int y2 = y1;

			if (y1 < halfh) y2 += halfh + oddh;
			else if (y1 >= halfh + oddh) y2 -= halfh - oddh;

			int offset1 = y1 * bytes_per_row;
			int offset2 = y2 * bytes_per_row;

			for (int x1 = 0; x1 < bmap->mW; ++x1)
			{
				int y2_was = y2;

				if (oddw && x1 == halfw) y2 = y1;

				int xpos1 = offset1 + x1 * bpp;
				int xpos2 = xpos1, x2 = x1;

				if (y1 != y2)
				{
					if (x1 < halfw) x2 += halfw + oddw;
					else if (x1 >= halfw + oddw) x2 -= halfw + oddw;

					xpos2 = offset2 + x2 * bpp;
				}

				float mask1 = mask(x1, y1, halfw, halfh), s;
				float mask2 = mask(x2, y2, halfw, halfh), t;

				float denom = mask1 + mask2;

				if (denom > 0.f)
				{
					s = mask1 / denom;
					t = mask2 / denom;
				}

				else s = t = .5f;

				for (int i = 0; i < bpp; ++i)
				{
					float v1 = s * bytes[xpos1 + i];
					float v2 = t * bytes[xpos2 + i];

					out[xpos1 + i] = (unsigned char)floor(v1 + v2);
				}

				y2 = y2_was;
			}
		}

		lua_pushcfunction(L, Bytemap_SetBytes); // bmap[, how], bytes, SetBytes
		lua_pushvalue(L, 1); // bmap[, how], bytes, SetBytes, bmap

		ByteProxy * proxy = LuaXS::NewTyped<ByteProxy>(L); // bmap[, how], bytes, SetBytes, bmap, proxy

		proxy->mBytes = result.data();
		proxy->mSize = result.size();

		luaL_getmetatable(L, BYTEMAP_TYPE_NAME(proxy)); // bmap[, how], bytes, SetBytes, bmap, proxy, proxy_mt
		lua_setmetatable(L, -2); // bmap[, how], bytes, SetBytes, bmap, proxy; proxy.mt = proxy_mt
		lua_call(L, 2, 0); // bmap[, how], bytes
	}

	return 0;
}