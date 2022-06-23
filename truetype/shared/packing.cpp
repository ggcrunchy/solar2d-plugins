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

#include "truetype.h"
#include "ByteReader.h"
#include "utils/Byte.h"
#include "utils/LuaEx.h"
#include "stb_rect_pack.h"
#include "stb_truetype.h"
#include <list>
#include <vector>

//
struct Packing {
	stbtt_pack_context mContext;
	struct Dims {
		int mW, mH;
	} mDims;
	bool mEnded{false};
	unsigned char mPixels[1];
};

//
static Packing * GetPacking (lua_State * L)
{
	return LuaXS::UD<Packing>(L, 1);
}

//
static stbtt_pack_context * GetContext (lua_State * L, bool bCheckEnded = true)
{
	Packing * packing = GetPacking(L);

	luaL_argcheck(L, !(bCheckEnded && packing->mEnded), 1, "Packing has already ended");

	return &packing->mContext;
}

static int EndPacking (lua_State * L)
{
	Packing * packing = GetPacking(L);

	if (!packing->mEnded) stbtt_PackEnd(&packing->mContext);

	packing->mEnded = true;

	return 0;
}

static int CharCount (lua_State * L)
{
	return LuaXS::ArrayN<stbtt_packedchar>(L) - 1;
}

Packing::Dims & GetDims (lua_State * L, int n, int arg = 1)
{
	return *reinterpret_cast<Packing::Dims *>(LuaXS::UD<stbtt_packedchar>(L, arg) + n);
}

static stbtt_packedchar * NewCharArray (lua_State * L, int nchars, int w, int h)
{
	stbtt_packedchar * chars = LuaXS::NewArray<stbtt_packedchar>(L, size_t(nchars) + 1);// ..., chars
	auto dims = GetDims(L, nchars, -1);

	dims.mW = w;
	dims.mH = h;

	LuaXS::AttachMethods(L, "truetype.packedchars", [](lua_State * L)
	{
		luaL_Reg pc_methods[] = {
			{
				"GetPackedQuad", [](lua_State * L)
				{
					int ci = luaL_checkint(L, 2) - 1, n = CharCount(L);
					auto dims = GetDims(L, n);

					luaL_argcheck(L, ci >= 0 && ci < n, 2, "Invalid character index");

					float xpos = LuaXS::Float(L, 3);
					float ypos = LuaXS::Float(L, 4);

					stbtt_aligned_quad q;

					stbtt_GetPackedQuad(LuaXS::UD<stbtt_packedchar>(L, 1), dims.mW, dims.mH, ci, &xpos, &ypos, &q, lua_toboolean(L, 5));

					lua_createtable(L, 0, 8);	// chars, ci, xpos, ypos[, align], quad

					struct {
						const char * mName;
						float mMember;
					} members[] = {
						{ "s0", q.s0 }, { "t0", q.t0 }, { "s1", q.s1 }, { "t1", q.t1 },
						{ "x0", q.x0 }, { "y0", q.y0 }, { "x1", q.x1 }, { "y1", q.y1 }
					};

					for (auto item : members) LuaXS::SetField(L, -1, item.mName, item.mMember);	// chars, ci, xpos, ypos[, align], quad = { ..., name = num }

					return 1 + LuaXS::PushMultipleArgsAndReturn(L, xpos, ypos);	// chars, ci, xpos, ypos[, align], quad, xpos, ypos
				}
			}, {
				"__len", CharCount
			},
			{ nullptr, nullptr }
		};

		luaL_register(L, nullptr, pc_methods);
	});

	return chars;
}

int PointSize (lua_State * L)
{
	float size = LuaXS::Float(L, 1);

	luaL_argcheck(L, size >= 0, 1, "Invalid font size");
	lua_pushlstring(L, reinterpret_cast<const char *>(&size), sizeof(float));	// size, enc_size

	return 1;
}

static float FontSize (lua_State * L, int arg)
{
	float size;

	if (lua_isstring(L, arg))
	{
		luaL_argcheck(L, lua_objlen(L, arg) == sizeof(float), arg, "Not a PointSize()'d size");

		size = *reinterpret_cast<const float *>(lua_tostring(L, arg));
	}

	else size = LuaXS::Float(L, arg);

	luaL_argcheck(L, size >= 0, arg, "Invalid font size");

	return lua_isstring(L, arg) ? STBTT_POINT_SIZE(size) : size;
}

static stbrp_rect * NewRectArray (lua_State * L, int nrects);

static int MergeArrays (lua_State * L, stbrp_rect * r1, stbrp_rect * r2)
{
	int n1 = LuaXS::LenTyped<stbrp_rect>(L);
	
	lua_settop(L, 2);	// r1, r2
	lua_insert(L, 1);	// r2, r1

	int n2 = LuaXS::LenTyped<stbrp_rect>(L), mi = 0;

	stbrp_rect * merged = NewRectArray(L, n1 + n2);	// r2, r1, merged

	for (int i = 0; i < n1; ++i) merged[mi++] = r1[i];
	for (int i = 0; i < n2; ++i) merged[mi++] = r2[i];

	return 1;
}

stbrp_rect * NewRectArray (lua_State * L, int nrects)
{
	stbrp_rect * rects = LuaXS::NewArray<stbrp_rect>(L, size_t(nrects));// ..., rects

	LuaXS::AttachMethods(L, "truetype.rects", [](lua_State * L)
	{
		luaL_Reg ra_methods[] = {
			{
				"__concat", [](lua_State * L)
				{
					stbrp_rect * r1 = LuaXS::CheckUD<stbrp_rect>(L, 1, "truetype.rects");
					stbrp_rect * r2 = LuaXS::CheckUD<stbrp_rect>(L, 2, "truetype.rects");

					return MergeArrays(L, r1, r2);
				}
			}, {
				"Concatenate", [](lua_State * L)
				{
					return MergeArrays(L, LuaXS::UD<stbrp_rect>(L, 1), LuaXS::CheckUD<stbrp_rect>(L, 2, "truetype.rects"));
				}
			}, {
				"__len", LuaXS::LenTyped<stbrp_rect>
			}, {
				"WasPacked", [](lua_State * L)
				{
					int ri = luaL_checkint(L, 2) - 1, n = LuaXS::LenTyped<stbrp_rect>(L);

					luaL_argcheck(L, ri >= 0 && ri < n, 2, "Invalid rect index");

					return LuaXS::BoolResult(L, LuaXS::UD<stbrp_rect>(L, 1)[ri].was_packed);// rects, index, was_packed
				}
			},
			{ nullptr, nullptr }
		};

		luaL_register(L, nullptr, ra_methods);
	});

	return rects;
}

struct PackRange {
	std::vector<stbtt_pack_range> mRanges{};
	std::list<std::vector<int>> mCodepoints{};
	int mNumRects{0};

	stbtt_pack_range * Get (void) { return mRanges.data(); }
	int Count (void) const { return int(mRanges.size()); }

	PackRange (lua_State * L, int ri, bool bAddChars)
	{
		luaL_checktype(L, ri, LUA_TTABLE);

		int n = lua_objlen(L, ri);

		mRanges.resize(n);
		mCodepoints.resize(n);

		auto cpi = mCodepoints.begin();

		if (bAddChars) lua_createtable(L, n, 0);	// ..., chars

		int top = lua_gettop(L);

		for (int i = 0; i < n; ++i, lua_settop(L, top))
		{
			stbtt_pack_range & range = mRanges[i];

			lua_rawgeti(L, ri, i + 1);	// ...[, chars], range
			luaL_checktype(L, -1, LUA_TTABLE);
			lua_getfield(L, -1, "font_size");	// ...[, chars], range, size
			lua_getfield(L, -2, "codepoints");	// ...[, chars], range, size, codepoints

			range.font_size = FontSize(L, -2);
							
			if (lua_istable(L, -1))
			{
				LuaXS::ForEachI(L, -1, [&range](lua_State * L, size_t n){
					range.num_chars = int(n);

					luaL_argcheck(L, range.num_chars > 0, -1, "Empty range");
				}, [&cpi](lua_State * L, size_t i){
					cpi->push_back(Codepoint(L, -1));
				});

				range.first_unicode_codepoint_in_range = 0;
				range.array_of_unicode_codepoints = cpi->data();

				++cpi;
			}

			else
			{
				range.first_unicode_codepoint_in_range = Codepoint(L, -1);
				range.array_of_unicode_codepoints = nullptr;
								
				lua_getfield(L, -3, "num_chars");	// ...[, chars], range, size, codepoints, num_chars

				range.num_chars = luaL_checkint(L, -1);

				luaL_argcheck(L, range.num_chars > 0, -1, "Invalid character count");
			}

			if (bAddChars)
			{
				Packing * packing = GetPacking(L); // always called from method

				mRanges[i].chardata_for_range = NewCharArray(L, range.num_chars, packing->mDims.mW, packing->mDims.mH);	// ..., chars, range, size, codepoints[, nchars], ca

				lua_rawseti(L, top, i + 1);	// ..., chars = { ..., ca }, range, size, codepoints
			}

			else mRanges[i].chardata_for_range = nullptr;

			mNumRects += mRanges[i].num_chars;
		}
	}
};

static inline stbtt_pack_context * GetPackingWithMemory (lua_State * L)
{
	stbtt_pack_context * packing = GetContext(L);

	lua_getfenv(L, 1);

	truetype_GetMemory()->PrepMemory();

	return packing;
}

//
int NewPacking (lua_State * L)
{
	int w = luaL_checkint(L, 1);
	int h = luaL_checkint(L, 2);
	int stride = luaL_optint(L, 3, 0);
	int padding = luaL_optint(L, 4, 1);

	luaL_argcheck(L, w > 0, 1, "Invalid width");
	luaL_argcheck(L, h > 0, 2, "Invalid height");
	luaL_argcheck(L, stride == 0 || stride >= w, 3, "Invalid stride");
	luaL_argcheck(L, padding >= 0, 4, "Invalid padding");

	size_t size = size_t(stride ? stride : w);
	Packing * packing = LuaXS::NewSizeTyped<Packing>(L, sizeof(stbtt_pack_context) + size * h - 1);	// w, h[, stride, padding], packing

	packing->mDims.mW = w;
	packing->mDims.mH = h;

	lua_newtable(L);// w, h[, stride, padding], packing, memory
	lua_setfenv(L, -2);	// w, h[, stride, padding], packing

	if (stbtt_PackBegin(&packing->mContext, &packing->mPixels[0], w, h, stride, padding, nullptr))
	{
		LuaXS::AttachMethods(L, "truetype.packing", [](lua_State * L)
		{
			luaL_reg packing_methods[] = {
				{
					"__gc", EndPacking
				}, {
					"PackEnd", EndPacking
				}, {
					"PackFontRange", [](lua_State * L)
					{
						lua_settop(L, 6);	// packing, fontdata, size, codepoint, nchars, index

						auto packing = GetPackingWithMemory(L);	// packing, fontdata, size, codepoint, nchars, index, memory

						ByteReader bytes{L, 2};

						int nchars = luaL_checkint(L, 5), index = luaL_optint(L, 6, 1) - 1;

						luaL_argcheck(L, nchars > 0, 5, "Invalid character count");
						luaL_argcheck(L, index >= 0, 6, "Invalid font index");
						
						Packing * pobject = GetPacking(L);
						stbtt_packedchar * chars = NewCharArray(L, nchars, pobject->mDims.mW, pobject->mDims.mH);	// packing, fontdata, size, codepoint, nchars, index, memory, chars
	
						return LuaXS::ResultOrNil(L, stbtt_PackFontRange(packing, (unsigned char *)bytes.mBytes, index, FontSize(L, 3), Codepoint(L, 4), nchars, chars));	// packing, fontdata, size, codepoint, nchars, index, memory, chars / nil
					}
				}, {
					"PackFontRanges", [](lua_State * L)
					{
						lua_settop(L, 4);	// packing, fontdata, ranges, index

						auto packing = GetPackingWithMemory(L);	// packing, fontdata, ranges, index, memory

						ByteReader bytes{L, 2};
						
						int index = luaL_optint(L, 4, 1) - 1;

						luaL_argcheck(L, index >= 0, 4, "Invalid font index");

						PackRange ranges{L, 3, true};	// packing, fontdata, ranges, index, memory, chars

						return LuaXS::ResultOrNil(L, stbtt_PackFontRanges(packing, (unsigned char *)bytes.mBytes, index, ranges.Get(), ranges.Count()));// packing, fontdata, ranges, index, memory, chars / nil
					}
				}, {
					"PackFontRangesGatherRects", [](lua_State * L)
					{
						lua_settop(L, 3);	// packing, font, ranges

						auto packing = GetPackingWithMemory(L);	// packing, font, ranges, memory

						PackRange ranges{L, 3, false};

						stbrp_rect * rects = NewRectArray(L, ranges.mNumRects);	// packing, font, ranges, memory, rects

						return LuaXS::ResultOrNil(L, stbtt_PackFontRangesGatherRects(packing, GetFontInfo(L, 2), ranges.Get(), ranges.Count(), rects));	// packing, font, ranges, memory, rects / nil
					}
				}, {
					"PackFontRangesPackRects", [](lua_State * L)
					{
						lua_settop(L, 2);	// packing, rects

						auto packing = GetPackingWithMemory(L);	// packing, rects, memory

						stbtt_PackFontRangesPackRects(packing, LuaXS::UD<stbrp_rect>(L, 2), LuaXS::ArrayN<stbrp_rect>(L, 2));

						return 0;
					}
				}, {
					"PackFontRangesRenderIntoRects", [](lua_State * L)
					{
						lua_settop(L, 4);	// packing, font, ranges, rects

						auto packing = GetPackingWithMemory(L);	// packing, font, ranges, rects, memory

						PackRange ranges{L, 3, true};	// packing, font, ranges, rects, memory, chars

						luaL_argcheck(L, LuaXS::ArrayN<stbrp_rect>(L, 4) >= size_t(ranges.mNumRects), 4, "Rect array too short");
						
						return LuaXS::ResultOrNil(L, stbtt_PackFontRangesRenderIntoRects(packing, GetFontInfo(L, 2), ranges.Get(), ranges.Count(), LuaXS::CheckUD<stbrp_rect>(L, 4, "truetype.rects")));	// packing, font, ranges, rects, memory, chars / nil
					}
				}, {
					"PackSetOversampling", [](lua_State * L)
					{
						int hover = LuaXS::Int(L, 2), vover = LuaXS::Int(L, 3);

						luaL_argcheck(L, hover > 0, 2, "Invalid horizontal oversample");
						luaL_argcheck(L, vover > 0, 2, "Invalid vertical oversample");

						stbtt_PackSetOversampling(GetContext(L, false), hover, vover);

						return 0;
					}
				},
				{ nullptr, nullptr }
			};

			luaL_register(L, nullptr, packing_methods);

			ByteReaderFunc * func = ByteReader::Register(L);

			func->mGetBytes = [](lua_State * L, ByteReader & reader, int arg, void *)
			{
				lua_getfenv(L, arg);// ..., proxy, ..., env
				lua_pushvalue(L, arg);	// ..., proxy, ..., env, proxy
				lua_rawget(L, -2);	// ..., proxy, ..., env, packing

				Packing * packing = LuaXS::UD<Packing>(L, -1);

				reader.mBytes = packing->mPixels;
				reader.mCount = size_t(packing->mDims.mW * packing->mDims.mH);

				lua_pop(L, 2);	// ..., proxy, ...

				return true;
			};

			lua_pushlightuserdata(L, func);	// ..., methods, func

			LuaXS::NewWeakKeyedTable(L);// ..., methods, func, proxy_to_packing

			lua_pushcclosure(L, [](lua_State * L) {
				Packing * packing = GetPacking(L);

				lua_pushliteral(L, "as_bytes");	// packing, as_bytes, "as_bytes"

				if (lua_gettop(L) > 1 && lua_equal(L, 2, -1))
				{
					ByteXS::BytesMetatableOpts opts;

					opts.mMore = [](lua_State * L, void *)
					{
						lua_pushvalue(L, -3);	// ..., func, proxy, mt, func
						lua_setfield(L, -2, "__bytes");	// ..., func, proxy, mt = { ..., __bytes = func }
					};
					
					lua_pop(L, 2);	// packing
					lua_pushvalue(L, lua_upvalueindex(1));	// packing, func
					lua_pushvalue(L, lua_upvalueindex(2));	// packing, func, env
					lua_newuserdata(L, 0U);	// packing, func, env, proxy
					lua_pushvalue(L, -1);	// packing, func, env, proxy, proxy
					lua_pushvalue(L, 1);// packing, func, env, proxy, proxy, packing
					lua_rawset(L, -4);	// packing, func, env = { ..., [proxy] = packing }, proxy
					lua_insert(L, -2);	// packing, func, proxy, env
					lua_setfenv(L, -2);	// packing, func, proxy; proxy.env = env

					ByteXS::AddBytesMetatable(L, "truetype.packing_bitmap", &opts);
				}

				else lua_pushlstring(L, reinterpret_cast<const char *>(packing->mPixels), size_t(packing->mDims.mW * packing->mDims.mH));	// packing[, as_userdata], str

				return 1;
			}, 2);	// ..., methods, GetBitmap
			lua_setfield(L, -2, "GetBitmap");	// ..., methods = { ..., GetBitmap = GetBitmap }
		});

		return 1;
	}

	else return LuaXS::PushArgAndReturn(L, LuaXS::Nil{});	// w, h[, stride, padding], packing, nil
}
