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

#include "clipper.hpp"
#include "utils/LuaEx.h"
#include "ByteReader.h"
#include <algorithm>
#include <cstdint>
#include <sstream>
#include <type_traits>
#include <utility>

static ClipperLib::ClipType GetClipType (lua_State * L, int arg)
{
	#define LIST() WITH(Intersection), WITH(Union), WITH(Difference), WITH(Xor)
	#define WITH(n) #n

	const char * names[] = { LIST(), nullptr };

	#undef WITH
	#define WITH(n) ClipperLib::ct##n

	ClipperLib::ClipType values[] = { LIST() };

	#undef WITH
	#undef LIST

	return values[luaL_checkoption(L, arg, nullptr, names)];
}

struct PolyTypeEx {
	ClipperLib::PolyType mType;
	bool mClosed;
};

static PolyTypeEx GetPolyType (lua_State * L, int arg)
{
	#define LIST() WITH(Subject), WITH(Clip)
	#define WITH(n) #n

	const char * names[] = { LIST(), "SubjectClosed", "ClipClosed", nullptr };

	#undef WITH
	#define WITH(n) ClipperLib::pt##n

	ClipperLib::PolyType values[] = { LIST(), WITH(Subject), WITH(Clip) };

	#undef WITH
	#undef LIST

	int index = luaL_checkoption(L, arg, nullptr, names);

	return PolyTypeEx{ values[index], index >= 2 };
}

static ClipperLib::PolyFillType GetPolyFillType (lua_State * L, int arg, const char * def = nullptr)
{
	#define LIST() WITH(EvenOdd), WITH(NonZero), WITH(Positive), WITH(Negative)
	#define WITH(n) #n

	const char * names[] = { LIST(), nullptr };

	#undef WITH
	#define WITH(n) ClipperLib::pft##n

	ClipperLib::PolyFillType values[] = { LIST() };

	#undef WITH
	#undef LIST

	return values[luaL_checkoption(L, arg, def, names)];
}

static ClipperLib::InitOptions GetInitOption (lua_State * L, int arg)
{
	#define LIST() WITH(ReverseSolution), WITH(StrictlySimple), WITH(PreserveCollinear)
	#define WITH(n) #n

	const char * names[] = { LIST(), nullptr };

	#undef WITH
	#define WITH(n) ClipperLib::io##n

	ClipperLib::InitOptions values[] = { LIST() };

	#undef WITH
	#undef LIST

	return values[luaL_checkoption(L, arg, nullptr, names)];
}

static ClipperLib::JoinType GetJoinType (lua_State * L, int arg)
{
	#define LIST() WITH(Square), WITH(Round), WITH(Miter)
	#define WITH(n) #n

	const char * names[] = { LIST(), nullptr };

	#undef WITH
	#define WITH(n) ClipperLib::jt##n

	ClipperLib::JoinType values[] = { LIST() };

	#undef WITH
	#undef LIST

	return values[luaL_checkoption(L, arg, nullptr, names)];
}

static ClipperLib::EndType GetEndType (lua_State * L, int arg)
{
	#define LIST() WITH(ClosedPolygon), WITH(ClosedLine), WITH(OpenButt), WITH(OpenSquare), WITH(OpenRound)
	#define WITH(n) #n

	const char * names[] = { LIST(), nullptr };

	#undef WITH
	#define WITH(n) ClipperLib::et##n

	ClipperLib::EndType values[] = { LIST() };

	#undef WITH
	#undef LIST

	return values[luaL_checkoption(L, arg, nullptr, names)];
}

template<typename T> struct ReverseInfo {
	int mSize{0}; // irrelevant for Paths
	bool mShouldReverse{true};

	static ReverseInfo * Get (T * ptr)
	{
		return reinterpret_cast<ReverseInfo *>(ptr + 1);
	}
};

template<typename T> void CorrectReversible (lua_State * L, int arg, T * ptr)
{
	if (lua_objlen(L, arg) == sizeof(T)) return;

	ReverseInfo<T> * ri = ReverseInfo<T>::Get(ptr);

	if (ri->mShouldReverse)
	{
		std::reverse(ptr->begin(), ptr->end());

		ri->mShouldReverse = false;
	}
}

template<typename T> T * CorrectUD (lua_State * L, int arg = 1)
{
	T * ptr = LuaXS::UD<T>(L, arg);

	CorrectReversible<T>(L, arg, ptr);

	return ptr;
}

template<typename T> T & CheckAndCorrectUD (lua_State * L, int arg, const char * name)
{
	T * ptr = LuaXS::CheckUD<T>(L, arg, name);

	CorrectReversible(L, arg, ptr);

	return *ptr;
}

static ClipperLib::Path & GetPath (lua_State * L, int arg = 1)
{
	return CheckAndCorrectUD<ClipperLib::Path>(L, arg, "clipper.Path");
}

static ClipperLib::Paths & GetPathArray (lua_State * L, int arg = 1)
{
	return CheckAndCorrectUD<ClipperLib::Paths>(L, arg, "clipper.PathArray");
}

static const ClipperLib::PolyNode * GetPolyNode (lua_State * L, int arg = 1)
{
	if (LuaXS::IsType(L, "clipper.PolyTree")) return LuaXS::UD<const ClipperLib::PolyNode>(L, arg);

	const ClipperLib::PolyNode * node = *LuaXS::CheckUD<const ClipperLib::PolyNode *>(L, arg, "clipper.PolyNode");

	luaL_argcheck(L, node, arg, "Unassigned poly node");
	lua_getfenv(L, arg);	// ..., node, ..., env
	lua_pushlightuserdata(L, (void *)node);	// ..., node, ..., env, node
	lua_rawget(L, -2);	// ..., node, ..., env, exists?

	bool exists = LuaXS::Bool(L);

	lua_pop(L, 2);	// ..., node, ...
	luaL_argcheck(L, exists, arg, "Invalid poly node");

	return node;
}

static ClipperLib::PolyTree & GetPolyTree (lua_State * L, int arg = 1)
{
	return *LuaXS::CheckUD<ClipperLib::PolyTree>(L, arg, "clipper.PolyTree");
}

static void InvalidateNode (lua_State * L, ClipperLib::PolyNode * node)
{
	for ( ; node; node = node->GetNext())
	{
		lua_pushlightuserdata(L, node);	// ..., env, node
		lua_pushnil(L);	// ..., env, node, nil
		lua_rawset(L, -3);	// ..., env = { ..., [node] = nil }

		for (int i = 0, n = node->ChildCount(); i < n; ++i) InvalidateNode(L, node->Childs[i]);
	}
}

static void InvalidateNodesInTree (lua_State * L, int arg)
{
	ClipperLib::PolyTree & tree = GetPolyTree(L, arg);

	lua_getfenv(L, arg);	// ..., tree, ..., env

	InvalidateNode(L, tree.GetFirst());

	lua_pop(L, 1);	// ..., tree, ...
}

template<typename T> T * GetOrNew (lua_State * L, T * object, lua_CFunction func)
{
	if (!object)
	{
		func(L);// ..., new_object

		return LuaXS::UD<T>(L, -1);
	}

	else return object;
}

template<typename T> int ReturnOrOut (lua_State * L, T * from, T * to, lua_CFunction func)
{
	*GetOrNew(L, to, func) = std::move(*from);	// ...[, new_object]

	return 1;
}

template<typename T> int ReturnOrOut (lua_State * L, const T * from, T * to, lua_CFunction func)
{
	*GetOrNew(L, to, func) = *from;	// ...[, new_object]

	return 1;
}

static ClipperLib::Clipper * Get (lua_State * L)
{
	return LuaXS::CheckUD<ClipperLib::Clipper>(L, 1, "clipper.Clipper");
}

static ClipperLib::ClipperOffset * GetOffset (lua_State * L)
{
	return LuaXS::CheckUD<ClipperLib::ClipperOffset>(L, 1, "clipper.Offset");
}

static ClipperLib::IntPoint GetPoint (lua_State * L)
{
	return ClipperLib::IntPoint(ClipperLib::cInt(lua_tonumber(L, -2)), ClipperLib::cInt(lua_tonumber(L, -1)));
}

template<typename I> void ReadIntPair (lua_State * L, ClipperLib::IntPoint & p, ByteReader const & bytes)
{
	luaL_argcheck(L, bytes.mCount >= 2U * sizeof(I), 1, "Too few bytes for point");

	const I * pi = static_cast<const I *>(bytes.mBytes);

	p.X = pi[0];
	p.Y = pi[1];
}

#define CATCH() catch (ClipperLib::clipperException ce) {	\
					return luaL_error(L, ce.what());		\
				}

static int AddPoint (lua_State * L)
{
	luaL_argcheck(L, LuaXS::IsType(L, "clipper.Path", 2), 2, "Expected clipper.Path");

	ClipperLib::Path * path = LuaXS::UD<ClipperLib::Path>(L, 2);

	ClipperLib::IntPoint p;

	if (lua_istable(L, 1))
	{
		lua_rawgeti(L, 1, 1);	// point, path, x
		lua_rawgeti(L, 1, 2);	// point, path, x, y

		p = GetPoint(L);

		lua_settop(L, 2);	// point, path
	}

	else
	{
		ByteReader bytes{L, 1};

		luaL_argcheck(L, bytes.mBytes, 1, "No bytes supplied");

		int size = 32;

		if (lua_objlen(L, 2) > sizeof(ClipperLib::Path)) size = ReverseInfo<ClipperLib::Path>::Get(path)->mSize;

		switch (size)
		{
		case 64:
			ReadIntPair<int64_t>(L, p, bytes);

			luaL_argcheck(L, p.X >= -ClipperLib::hiRange && p.X <= +ClipperLib::hiRange, 1, "X too large");
			luaL_argcheck(L, p.Y >= -ClipperLib::hiRange && p.Y <= +ClipperLib::hiRange, 1, "Y too large");

			break;
		case 32:
			ReadIntPair<int32_t>(L, p, bytes);

			break;
		case 16:
			ReadIntPair<int16_t>(L, p, bytes);

			break;
		case 8:
			ReadIntPair<int8_t>(L, p, bytes);

			break;
		default:
			return luaL_error(L, "Invalid size");
		}
	}

	try {
		*path << p; // n.b. not corrected
	} CATCH()

	return 1;
}

template<bool bShouldReverse = false> int NewPathArray (lua_State * L);

template<typename T> void NewReversible (lua_State * L, bool bShouldReverse)
{
	T * ptr = LuaXS::NewSizeTyped<T>(L, sizeof(T) + (bShouldReverse ? sizeof(ReverseInfo<T>) : 0U));	// object

	if (bShouldReverse) ReverseInfo<T>::Get(ptr)->mShouldReverse = true;
}

template<bool bShouldReverse = false> int NewPath (lua_State * L)
{
	NewReversible<ClipperLib::Path>(L, bShouldReverse);	// path

	LuaXS::AttachMethods(L, "clipper.Path", [](lua_State * L) {
		luaL_Reg methods[] = {
			{
				"AddPoint", [](lua_State * L)
				{
					try {
						GetPath(L) << GetPoint(L);
					} CATCH()

					return 0;
				}
			},  {
				"Clear", [](lua_State * L)
				{
					GetPath(L).clear();

					return 0;
				}
			}, {
				"__concat", [](lua_State * L)
				{
					if (LuaXS::IsType(L, "clipper.Path", 1))
					{
						ClipperLib::Path & p1 = *CorrectUD<ClipperLib::Path>(L, 1);

						if (!LuaXS::IsType(L, "clipper.PathArray", 2))
						{
							NewPathArray<true>(L);	// path, pa

							try {
								*LuaXS::UD<ClipperLib::Paths>(L, -1) << p1; // n.b. not corrected
							} CATCH()
						}

						else
						{
							ClipperLib::Path & p2 = GetPath(L, 2);

							try {
								*LuaXS::UD<ClipperLib::Paths>(L, -1) << p1 << p2; // n.b. not corrected
							} CATCH()
						}

						return 1;
					}

					else return AddPoint(L);// path
				}
			}, {
				"__gc", LuaXS::TypedGC<ClipperLib::Path>
			}, {
				"GetPoint", [](lua_State * L)
				{
					ClipperLib::Path & path = GetPath(L);
					int index = LuaXS::Int(L, 2) - 1;

					luaL_argcheck(L, index >= 0 && size_t(index) < path.size(), 2, "Invalid index");

					ClipperLib::IntPoint p = path[index];

					return LuaXS::PushMultipleArgsAndReturn(L, lua_Integer(p.X), lua_Integer(p.Y));	// path, index, x, y;
				}
			}, {
				"__len", [](lua_State * L)
				{
					return LuaXS::PushArgAndReturn(L, GetPath(L).size());	// path, count
				}
            }, {
				"__tostring", [](lua_State * L)
				{
					std::stringstream sstr;

					sstr << GetPath(L);

					return LuaXS::PushArgAndReturn(L, sstr.str().c_str());	// path, str
				}
			},
            { nullptr, nullptr }
		};

		luaL_register(L, nullptr, methods);

        lua_pushcfunction(L, [](lua_State * L) {
            ClipperLib::Path & path = GetPath(L);
            int index = LuaXS::Int(L, 2);

            if (index >= 0 && size_t(index) < path.size())
            {
                lua_pushinteger(L, index + 1);  // path, index, index + 1

                ClipperLib::IntPoint p = path[index];

                return LuaXS::PushMultipleArgsAndReturn(L, index + 1, lua_Integer(p.X), lua_Integer(p.Y));  // path, index, index + 1, x, y
            }
            
            else
            {
                lua_pushnil(L); // path, index, nil
                
                return 1;
            }
        }); // path, path_mt, AuxPoints
        lua_pushcclosure(L, [](lua_State * L) {
            lua_settop(L, 1);   // path
            lua_pushvalue(L, lua_upvalueindex(1));  // path, AuxPoints
            lua_insert(L, 1);   // AuxPoints, path
            lua_pushinteger(L, 0);  // AuxPoints, path, 0
            
            return 3;
        }, 1); // path, path_mt, Points
        lua_setfield(L, -2, "Points");  // path, path_mt = { ... Points = Points }
    });

	return 1;
}

struct Options {
	ClipperLib::Path * mPath{nullptr};
	ClipperLib::Paths * mPathArray{nullptr};
	const ClipperLib::PolyNode ** mNode{nullptr};
	ClipperLib::PolyTree * mTree{nullptr};
	double mDistance{1.415};
	ClipperLib::PolyFillType mFillType{ClipperLib::pftEvenOdd};
	bool mIsClosed{false};

	Options (lua_State * L, int arg, bool bPush = false)
	{
		if (!lua_istable(L, arg)) return;

		lua_getfield(L, arg, "out");// ..., opts, ..., out?

		bool bAny = bPush;

		if (lua_isnil(L, -1)) bAny = false;
		else if (LuaXS::IsType(L, "clipper.Path")) mPath = CorrectUD<ClipperLib::Path>(L, -1);
		else if (LuaXS::IsType(L, "clipper.PathArray")) mPathArray = CorrectUD<ClipperLib::Paths>(L, -1);
		else if (LuaXS::IsType(L, "clipper.PolyNode")) mNode = LuaXS::UD<const ClipperLib::PolyNode *>(L, -1);
		else if (LuaXS::IsType(L, "clipper.PolyTree")) mTree = LuaXS::UD<ClipperLib::PolyTree>(L, -1);
		else bAny = false;

		lua_getfield(L, arg, "distance");	// ..., opts, ..., out?, distance?
		lua_getfield(L, arg, "fill_type");	// ..., opts, ..., out?, distance, fill_type?
		lua_getfield(L, arg, "is_closed");	// ..., opts, ..., out?, distance, fill_type?, is_closed

		mDistance = luaL_optnumber(L, -3, mDistance);
		mFillType = GetPolyFillType(L, -2, "EvenOdd");
		mIsClosed = LuaXS::Bool(L, -1);

		lua_pop(L, bAny ? 3 : 4);	// ..., opts, ...[, out]
	}
};
	
template<bool bShouldReverse> int NewPathArray (lua_State * L)
{
	NewReversible<ClipperLib::Paths>(L, bShouldReverse);// path_array

	LuaXS::AttachMethods(L, "clipper.PathArray", [](lua_State * L) {
		luaL_Reg methods[] = {
			{
				"AddPath", [](lua_State * L)
				{
					try {
						GetPathArray(L) << GetPath(L, 2);
					} CATCH()

					return 0;
				}
			}, {
				"Clear", [](lua_State * L)
				{
					GetPathArray(L).clear();

					return 0;
				}
			}, {
				"__concat", [](lua_State * L)
				{
					try {
						GetPathArray(L) << GetPath(L, 2); // TODO: not quite right since right-associative, need intermediate object
					} CATCH()

					return 1;
				}
			}, {
				"__gc", LuaXS::TypedGC<ClipperLib::Paths>
			}, {
				"GetPath", [](lua_State * L)
				{
					Options opts{L, 3, true};	// path_array, index[, opts[, out]]

					ClipperLib::Paths & paths = GetPathArray(L);
					int index = LuaXS::Int(L, 2) - 1;

					luaL_argcheck(L, index >= 0 && size_t(index) < paths.size(), 2, "Invalid index");

					ClipperLib::Path path = paths[index];

					return ReturnOrOut(L, &path, opts.mPath, NewPath);	// path_array, index[, opts], out
				}
			}, {
				"__len", [](lua_State * L)
				{
					return LuaXS::PushArgAndReturn(L, GetPathArray(L).size());	// path_array, count
				}
			}, {
				"__tostring", [](lua_State * L)
				{
					std::stringstream sstr;

					sstr << GetPathArray(L);

					return LuaXS::PushArgAndReturn(L, sstr.str().c_str());	// path_array, str
				}
			},
            { nullptr, nullptr }
		};

		luaL_register(L, nullptr, methods);

        lua_newtable(L);// path_array, path_array_mt, stash
        lua_pushvalue(L, -1);   // path_array, path_array_mt, stash, stash
        lua_pushcclosure(L, [](lua_State * L) {
            int index = LuaXS::Int(L, 2);

            lua_pushinteger(L, index + 1);  // path_array, index, index + 1

            ClipperLib::Path * out = nullptr;

            if (lua_istable(L, 1))
            {
                lua_pushvalue(L, 1);// t, index, index + 1, t
                lua_rawgeti(L, 1, 2);   // t, index, index + 1, t, out
                lua_rawgeti(L, 1, 1);   // t, index, index + 1, t, out, path_array
                lua_replace(L, 1);  // path_array, index, index + 1, t, out

                out = CorrectUD<ClipperLib::Path>(L, -1);
            }

            ClipperLib::Paths & paths = GetPathArray(L);
            
            if (index >= 0 && size_t(index) < paths.size())
            {
                ClipperLib::Path path = paths[index];
                
                lua_remove(L, -2);  // path_array, index, index + 1, out

                return 1 + ReturnOrOut(L, &path, out, NewPath); // path_array, index, index + 1, t, out
            }
            
            else
            {
                if (out)
                {
                    lua_pop(L, 1);  // path_array, index, index + 1, t
                    lua_pushvalue(L, lua_upvalueindex(1));  // path_array, index, index + 1, t, stash
                    lua_insert(L, -2);  // path_array, index, index + 1, stash, t

                    for (int i = 1; i <= 2; ++i)
                    {
                        lua_pushnil(L); // path_array, index, index + 1, stash, t, nil
                        lua_rawseti(L, -2, i);  // path_array, index, index + 1, stash, t = { path_array } / {}
                    }

                    lua_rawseti(L, -2, int(lua_objlen(L, -2)) + 1); // path_array, index, index + 1, stash = { ..., t }
                }
                
                lua_pushnil(L); // paths, index, index + 1[, stash], nil
                
                return 1;
            }
        }, 1); // path_array, path_array_mt, stash, AuxPoints
        lua_insert(L, -2);  // path_array, path_array_mt, AuxPoints, stash
        lua_pushcclosure(L, [](lua_State * L) {
            if (LuaXS::IsType(L, "clipper.Path", 2))
            {
                lua_pushvalue(L, lua_upvalueindex(2));  // paths, out, stash

                size_t count = lua_objlen(L, -1);

                if (count > 0U)
                {
                    lua_rawgeti(L, -1, int(count)); // paths, out, stash, t
                    lua_pushnil(L); // paths, out, stash, t, nil
                    lua_rawseti(L, -3, int(count)); // paths, out, stash = { ..., [#t] = nil }, t
                }
                
                else lua_createtable(L, 2, 0);  // paths, out, stash, t

                for (int i = 1; i <= 2; ++i)
                {
                    lua_pushvalue(L, i);// paths, out, stash, t, paths / out
                    lua_rawseti(L, -2, i);  // paths, out, stash, t = { paths } / t = { paths, out }
                }
            }

            else lua_settop(L, 1);   // path

            lua_pushvalue(L, lua_upvalueindex(1));  // ..., path / t, AuxPoints
            lua_insert(L, -2);   // ..., AuxPoints, path / t
            lua_pushinteger(L, 0);  // ..., AuxPoints, path, 0
            
            return 3;
        }, 2); // path_array, path_array_mt, Points
        lua_setfield(L, -2, "Paths");  // path_array, path_array_mt = { ... Paths = Paths }
    });

	return 1;
}

static int NewPolyNode (lua_State * L);

static int PushNode (lua_State * L, int arg, const ClipperLib::PolyNode * node)
{
	if (node)
	{
		Options opts{L, arg, true};	// parent, ..., opts?[, out]

		const ClipperLib::PolyNode ** ppnode = GetOrNew(L, opts.mNode, NewPolyNode);// parent, ..., opts?[, out], node

		lua_getfenv(L, 1);	// parent, ..., opts?[, out], node, env
		lua_pushlightuserdata(L, (void *)node);	// parent, ..., opts?[, out], node, env, node
		lua_pushboolean(L, 1);	// parent, ..., opts?[, out], node, env, node, true
		lua_rawset(L, -3);	// parent, ..., opts?[, out], node, env = { ..., [node] = true }
		lua_setfenv(L, -2);	// parent, ..., opts?[, out], node; node.env = env

		*ppnode = node;
	}

	else lua_pushnil(L);// parent, ..., opts?, nil

	return 1;
}

static void AddPolyNodeMethods (lua_State * L)
{
	luaL_Reg methods[] = {
		{
			"ChildCount", [](lua_State * L)
			{
				return LuaXS::PushArgAndReturn(L, GetPolyNode(L)->ChildCount());// node, count
			}
		}, {
			"GetChild", [](lua_State * L)
			{
				const ClipperLib::PolyNode * node = GetPolyNode(L);
				int index = LuaXS::Int(L, 2) - 1;

				luaL_argcheck(L, index >= 0 && index < node->ChildCount(), 2, "Invalid index");

				return PushNode(L, 3, node->Childs[index]);	// node, index[, opts], child
			}
		}, {
			"GetContour", [](lua_State * L)
			{
				Options opts{L, 2, true};	// node[, opts[, out]]

				return ReturnOrOut(L, &GetPolyNode(L)->Contour, opts.mPath, NewPath);	// node[, opts], contour
			}
		}, {
			"GetNext", [](lua_State * L)
			{
				return PushNode(L, 2, GetPolyNode(L)->GetNext());	// node[, opts], out
			}
		}, {
			"GetParent", [](lua_State * L)
			{
				return PushNode(L, 2, GetPolyNode(L)->Parent);	// node[, opts], parent?
			}
		}, {
			"IsHole", [](lua_State * L)
			{
				return LuaXS::PushArgAndReturn(L, GetPolyNode(L)->IsHole());// node, is_hole
			}
		}, {
			"IsOpen", [](lua_State * L)
			{
				return LuaXS::PushArgAndReturn(L, GetPolyNode(L)->IsOpen());// node, is_open
			}
		}, {
			"ToTree", [](lua_State * L)
			{
				const ClipperLib::PolyNode * node = GetPolyNode(L);

				if (node->Parent) return LuaXS::PushArgAndReturn(L, LuaXS::Nil{});// node, nil

				lua_getfenv(L, 1);	// node, env
				lua_pushlightuserdata(L, (void *)node);	// node, env, node_ptr
				lua_rawget(L, -2);	// node, env, tree

				return 1;
			}
		},
		{ nullptr, nullptr }
	};

	luaL_register(L, nullptr, methods);
}

static int NewPolyNode (lua_State * L)
{
	const ClipperLib::PolyNode ** node = LuaXS::NewTyped<const ClipperLib::PolyNode *>(L);	// ..., node

	LuaXS::AttachMethods(L, "clipper.PolyNode", [](lua_State * L) {
		AddPolyNodeMethods(L);
	});

	*node = nullptr;

	return 1;
}

template<typename T> void Convert (std::vector<unsigned char> * out, ClipperLib::Path & path)
{
    out->resize(path.size() * sizeof(T) * 2U);

    T * data = reinterpret_cast<T *>(out->data());

    for (ClipperLib::IntPoint p : path)
    {
        *data++ = static_cast<T>(p.X);
        *data++ = static_cast<T>(p.Y);
    }
}

//
static luaL_Reg clipper_funcs[] = {
	{
		"Area", [](lua_State * L)
		{
			return LuaXS::PushArgAndReturn(L, ClipperLib::Area(GetPath(L)));// path, area
		}
	}, {
		"CleanPolygon", [](lua_State * L)
		{
			Options opts{L, 2};

			if (opts.mPath) ClipperLib::CleanPolygon(GetPath(L), *opts.mPath, opts.mDistance);
			else ClipperLib::CleanPolygon(GetPath(L), opts.mDistance);

			return 0;
		}
	}, {
		"CleanPolygons", [](lua_State * L)
		{
			Options opts{L, 2};

			if (opts.mPathArray) ClipperLib::CleanPolygons(GetPathArray(L), *opts.mPathArray, opts.mDistance);
			else ClipperLib::CleanPolygons(GetPathArray(L), opts.mDistance);

			return 0;
		}
	}, {
		"ClosedPathsFromPolyTree", [](lua_State * L)
		{
			Options opts{L, 2, true};	// poly_tree[, opts[, out]]
			ClipperLib::Paths paths;

			ClipperLib::ClosedPathsFromPolyTree(GetPolyTree(L), paths);

			return ReturnOrOut(L, &paths, opts.mPathArray, NewPathArray);	// poly_tree[, opts], out
		}
	}, {
		"MinkowskiDiff", [](lua_State * L)
		{
			Options opts{L, 3, true};	// path1, path2[, opts[, out]]
			ClipperLib::Paths solution;

			try {
				ClipperLib::MinkowskiDiff(GetPath(L, 1), GetPath(L, 2), solution);
			} CATCH()

			return ReturnOrOut(L, &solution, opts.mPathArray, NewPathArray);// path1, path2[, opts], out
		}
	}, {
		"MinkowskiSum", [](lua_State * L)
		{
			Options opts{L, 3, true};	// path, path / path_array[, opts[, out]]
			ClipperLib::Paths solution;

			try {
				if (LuaXS::IsType(L, "clipper.Path", 2)) ClipperLib::MinkowskiSum(GetPath(L), *CorrectUD<ClipperLib::Path>(L, 2), solution, opts.mIsClosed);
				else ClipperLib::MinkowskiSum(GetPath(L), GetPathArray(L, 2), solution, opts.mIsClosed);
			} CATCH()

            return ReturnOrOut(L, &solution, opts.mPathArray, NewPathArray);  // path, path / path_array[, opts], out
		}
    }, {
        "NewBuffer", [](lua_State * L)
        {
            LuaXS::NewTyped<std::vector<unsigned char>>(L); // buffer
            LuaXS::AttachMethods(L, "clipper.Buffer", [](lua_State * L) {
                luaL_Reg methods[] = {
                    {
                        "Convert", [](lua_State * L)
                        {
                            ClipperLib::Path & path = GetPath(L, 2);
                            const char * types[] = { "double", "float", nullptr };
							auto cd = &Convert<double>, cf = &Convert<float>; // n.b. in two steps
                            decltype(cd) funcs[] = { cd, cf };	// to make MSVC happy

                            funcs[luaL_checkoption(L, 3, nullptr, types)](LuaXS::UD<std::vector<unsigned char>>(L, 1), path);
                            
                            return 0;
                        }
                    },
                    { nullptr, nullptr }
                };
                
                luaL_register(L, nullptr, methods);

                ByteReaderFunc * func = ByteReader::Register(L);

                func->mGetBytes = [](lua_State * L, ByteReader & reader, int arg, void *)
                {
                    std::vector<unsigned char> * buffer = LuaXS::UD<std::vector<unsigned char>>(L, arg);

                    if (!buffer->empty())
                    {
                        VectorReader(L, reader, arg, nullptr);

                        return true;
                    }

                    return false;
                };

                lua_pushlightuserdata(L, func); // buffer, buffer_mt, func
                lua_setfield(L, -2, "__bytes"); // buffer, buffer_mt = { ..., __bytes = func }
            });

            return 1;
        }
    }, {
		"NewClipper", [](lua_State * L)
		{
			int options = 0;

			if (lua_istable(L, 1))
			{
				for (size_t i = 1U, count = lua_objlen(L, 1); i <= count; ++i, lua_pop(L, 1))
				{
					lua_rawgeti(L, 1, int(i));	// init_opts, opt

					options |= GetInitOption(L, -1);
				}
			}

			else if (lua_isstring(L, 1)) options = GetInitOption(L, 1);

			LuaXS::NewTyped<ClipperLib::Clipper>(L, options);	// [opts, ]clipper
			LuaXS::AttachMethods(L, "clipper.Clipper", [](lua_State * L) {
				luaL_Reg methods[] = {
					{
						"AddPath", [](lua_State * L)
						{
							try {
								PolyTypeEx pte = GetPolyType(L, 3);

								return LuaXS::PushArgAndReturn(L, Get(L)->AddPath(GetPath(L, 2), pte.mType, pte.mClosed));	// offset, path, poly_type, ok
							} CATCH()
						}
					}, {
						"AddPaths", [](lua_State * L)
						{
							try {
								PolyTypeEx pte = GetPolyType(L, 3);

								return LuaXS::PushArgAndReturn(L, Get(L)->AddPaths(GetPathArray(L, 2), pte.mType, pte.mClosed));// offset, path_array, poly_type, ok
							} CATCH()
						}
					}, {
						"Clear", [](lua_State * L)
						{
							Get(L)->Clear();

							return 0;
						}
					}, {
						"Execute", [](lua_State * L)
						{
							ClipperLib::ClipType clip = GetClipType(L, 2);
							bool bOptsLater = lua_isstring(L, 3) != 0;

							Options opts{L, bOptsLater ? 5 : 3, true};	// clip_type[, subj_fill_type, clip_fill_type][, opts[, out]]
							bool bOK;

							if (opts.mTree)
							{
								ClipperLib::PolyTree tree;

								InvalidateNodesInTree(L, -1);

								try {
									if (bOptsLater) bOK = Get(L)->Execute(clip, *opts.mTree, GetPolyFillType(L, 3), GetPolyFillType(L, 4));
									else bOK = Get(L)->Execute(clip, *opts.mTree, opts.mFillType);
								} CATCH()
							}

							else
							{
								ClipperLib::Paths solution;

								try {
									if (bOptsLater) bOK = Get(L)->Execute(clip, solution, GetPolyFillType(L, 3), GetPolyFillType(L, 4));
									else bOK = Get(L)->Execute(clip, solution, opts.mFillType);
								} CATCH()

								if (bOK) ReturnOrOut(L, &solution, opts.mPathArray, NewPathArray);	// clip_type[, subj_fill_type, clip_fill_type][, opts], out
							}

							return LuaXS::ResultOrNil(L, bOK);
						}
					}, {
						"__gc", LuaXS::TypedGC<ClipperLib::Clipper>
					}, {
						"GetBounds", [](lua_State * L)
						{
							lua_settop(L, 1);	// bounds?

							if (!lua_istable(L, 1))
							{
								lua_createtable(L, 0, 4);	// non_t, bounds
								lua_replace(L, 1);	// bounds
							}

							ClipperLib::IntRect bounds = Get(L)->GetBounds();

							LuaXS::SetField(L, 1, "left", lua_Integer(bounds.left));
							LuaXS::SetField(L, 1, "top", lua_Integer(bounds.top));
							LuaXS::SetField(L, 1, "right", lua_Integer(bounds.right));
							LuaXS::SetField(L, 1, "bottom", lua_Integer(bounds.bottom));

							return 1;
						}
					}, {
						"GetPreserveCollinear", [](lua_State * L)
						{
							return LuaXS::PushArgAndReturn(L, Get(L)->PreserveCollinear());	// offset, preserver_collinear
						}
					}, {
						"GetReverseSolution", [](lua_State * L)
						{
							return LuaXS::PushArgAndReturn(L, Get(L)->ReverseSolution());	// offset, reverse_solution
						}
					}, {
						"GetStrictlySimple", [](lua_State * L)
						{
							return LuaXS::PushArgAndReturn(L, Get(L)->StrictlySimple());// offset, strictly_simple
						}
					}, {
						"SetPreserveCollinear", [](lua_State * L)
						{
							Get(L)->PreserveCollinear(LuaXS::Bool(L, 2));

							return 0;
						}
					}, {
						"SetReverseSolution", [](lua_State * L)
						{
							Get(L)->ReverseSolution(LuaXS::Bool(L, 2));

							return 0;
						}
					}, {
						"SetStrictlySimple", [](lua_State * L)
						{
							Get(L)->StrictlySimple(LuaXS::Bool(L, 2));

							return 0;
						}
					},
					{ nullptr, nullptr }
				};

				luaL_register(L, nullptr, methods);
			});

			return 1;
		}
	}, {
		"NewOffset", [](lua_State * L)
		{
			double miter_limit = 2., round_precision = .25;

			if (lua_istable(L, 1))
			{
				lua_getfield(L, 1, "miter_limit");	// opts, miter_limit
				lua_getfield(L, 1, "round_precision");	// opts, miter_limit, round_precision

				miter_limit = luaL_optnumber(L, -2, miter_limit);
				round_precision = luaL_optnumber(L, -1, round_precision);
			}

			LuaXS::NewTyped<ClipperLib::ClipperOffset>(L, miter_limit, round_precision);// [opts, miter_limit, round_precision, ]offset
			LuaXS::AttachMethods(L, "clipper.Offset", [](lua_State * L) {
				luaL_Reg methods[] = {
					{
						"AddPath", [](lua_State * L)
						{
							try {
								GetOffset(L)->AddPath(GetPath(L, 2), GetJoinType(L, 3), GetEndType(L, 4));
							} CATCH()

							return 0;
						}
					}, {
						"AddPaths", [](lua_State * L)
						{
							try {
								GetOffset(L)->AddPaths(GetPathArray(L, 2), GetJoinType(L, 3), GetEndType(L, 4));
							} CATCH()

							return 0;
						}
					}, {
						"Clear", [](lua_State * L)
						{
							GetOffset(L)->Clear();

							return 0;
						}
					}, {
						"Execute", [](lua_State * L)
						{
							int dpos = lua_isnumber(L, 2) ? 2 : 3;
							double delta = lua_tonumber(L, dpos);

							if (dpos == 2) NewPathArray(L);// offset, delta, solution

							else
							{
								lua_settop(L, 2);	// offset, poly_tree / path_array

								if (!LuaXS::IsType(L, "clipper.PathArray", 2))
								{
									ClipperLib::PolyTree & tree = GetPolyTree(L, 2);

									InvalidateNodesInTree(L, 2);

									try {
										GetOffset(L)->Execute(tree, delta);
									} CATCH()

									return 1;
								}
							}
							
							try {
								GetOffset(L)->Execute(*CorrectUD<ClipperLib::Paths>(L, -1), delta);
							} CATCH()

							return 1;
						}
					}, {
						"__gc", LuaXS::TypedGC<ClipperLib::ClipperOffset>
					}, {
						"GetArcTolerance", [](lua_State * L)
						{
							return LuaXS::PushArgAndReturn(L, GetOffset(L)->ArcTolerance);	// offset, arc_tolerance
						}
					}, {
						"GetMiterLimit", [](lua_State * L)
						{
							return LuaXS::PushArgAndReturn(L, GetOffset(L)->MiterLimit);// offset, miter_limit
						}
					}, {
						"SetArcTolerance", [](lua_State * L)
						{
							GetOffset(L)->ArcTolerance = lua_tonumber(L, 2);

							return 0;
						}
					}, {
						"SetMiterLimit", [](lua_State * L)
						{
							GetOffset(L)->MiterLimit = lua_tonumber(L, 2);

							return 0;
						}
					},
					{ nullptr, nullptr }
				};

				luaL_register(L, nullptr, methods);
			});

			return 1;
		}
	}, {
		"NewPath", NewPath
	}, {
		"NewPathArray", NewPathArray
	}, {
		"NewPolyNode", NewPolyNode
	}, {
		"OpenPathsFromPolyTree", [](lua_State * L)
		{
			Options opts{L, 2, true};	// poly_tree[, opts[, out]]
			ClipperLib::Paths paths;

			ClipperLib::OpenPathsFromPolyTree(GetPolyTree(L), paths);

			return ReturnOrOut(L, &paths, opts.mPathArray, NewPathArray);	// poly_tree[, opts], out
		}
	}, {
		"Orientation", [](lua_State * L)
		{
			return LuaXS::PushArgAndReturn(L, ClipperLib::Orientation(GetPath(L)));	// path, orientation
		}
	}, {
		"PointInPolygon", [](lua_State * L)
		{
			switch (ClipperLib::PointInPolygon(ClipperLib::IntPoint{LuaXS::Int(L, 1), LuaXS::Int(L, 2)}, GetPath(L, 3)))
			{
			case -1:
				return LuaXS::PushArgAndReturn(L, "on_poly");	// x, y, path, "on_poly"
			case 1:
				return LuaXS::PushArgAndReturn(L, "inside");// x, y, path, "inside"
			default:// i.e. 0
				return LuaXS::PushArgAndReturn(L, false);	// x, y, path, false
			}
		}
	}, {
		"PolyTreeToPaths", [](lua_State * L)
		{
			Options opts{L, 2, true};	// poly_tree[, opts[, out]]
			ClipperLib::Paths paths;

			ClipperLib::PolyTreeToPaths(GetPolyTree(L), paths);

			return ReturnOrOut(L, &paths, opts.mPathArray, NewPathArray);	// poly_tree[, opts], out
		}
	}, {
		"ReversePath", [](lua_State * L)
		{
			ClipperLib::ReversePath(GetPath(L));

			return 0;
		}
	}, {
		"ReversePaths", [](lua_State * L)
		{
			ClipperLib::ReversePaths(GetPathArray(L));

			return 0;
		}
	}, {
		"SimplifyPolygon", [](lua_State * L)
		{
			Options opts{L, 2, true};	// path[, opts[, out]]
			ClipperLib::Paths paths;

			try {
				ClipperLib::SimplifyPolygon(GetPath(L), paths, opts.mFillType);
			} CATCH()

			return ReturnOrOut(L, &paths, opts.mPathArray, NewPathArray);	// path[, opts], out
		}
	}, {
		"SimplifyPolygons", [](lua_State * L)
		{
			Options opts{L, 2};

			try {
				if (opts.mPathArray) ClipperLib::SimplifyPolygons(GetPathArray(L), *opts.mPathArray, opts.mFillType);
				else ClipperLib::SimplifyPolygons(GetPathArray(L), opts.mFillType);
			} CATCH()

			return 0;
		}
	},
	{ nullptr, nullptr }
};

//
CORONA_EXPORT int luaopen_plugin_clipper (lua_State * L)
{
	lua_newtable(L);// clipper
	luaL_register(L, nullptr, clipper_funcs);

	lua_newtable(L);// clipper, env
	lua_pushliteral(L, "v");// clipper, env, "v"
	lua_setfield(L, -2, "__mode");	// clipper, env = { __mode = "v" }
	lua_pushvalue(L, -1);	// clipper, env, env
	lua_setmetatable(L, -2);// clipper, env; env.meta = env
	lua_pushcclosure(L, [](lua_State * L) {
		ClipperLib::PolyTree * tree = LuaXS::NewTyped<ClipperLib::PolyTree>(L);	// poly_tree

		lua_pushvalue(L, lua_upvalueindex(1));	// poly_tree, env
		lua_pushlightuserdata(L, tree);	// poly_tree, env, poly_tree_ptr
		lua_pushvalue(L, -3);	// poly_tree, env, poly_tree_ptr, poly_tree
		lua_rawset(L, -3);	// poly_tree, env = { ..., [poly_tree_ptr] = poly_tree }
		lua_setfenv(L, -2);	// poly_tree; poly_tree.env = env

		LuaXS::AttachMethods(L, "clipper.PolyTree", [](lua_State * L) {
			AddPolyNodeMethods(L);

			luaL_Reg methods[] = {
				{
					"__gc", [](lua_State * L)
					{
						InvalidateNodesInTree(L, 1);
						LuaXS::DestructTyped<ClipperLib::PolyTree>(L);

						return 0;
					}
				}, {
					"GetFirst", [](lua_State * L)
					{
						return PushNode(L, 2, GetPolyTree(L).GetFirst());	// poly_tree, first
					}
				}, {
					"Total", [](lua_State * L)
					{
						return LuaXS::PushArgAndReturn(L, GetPolyTree(L).Total());	// poly_tree, total
					}
				},
				{ nullptr, nullptr }
			};

			luaL_register(L, nullptr, methods);
		});

		return 1;
	}, 1);	// clipper, NewPolyTree
	lua_setfield(L, -2, "NewPolyTree");

	LuaXS::NewTyped<int>(L, 32);// clipper, PointsDone

	LuaXS::AttachMethods(L, "clipper.ToPath", [](lua_State * L) {
		luaL_Reg funcs[] = {
			{
				"__concat", [](lua_State * L)
				{
					int size = *LuaXS::CheckUD<int>(L, 2, "clipper.ToPath");

					lua_settop(L, 1);	// point
					luaL_argcheck(L, !LuaXS::IsType(L, "clipper.ToPath"), 1, "ToPath marker must be on right");

					NewPath<true>(L);	// point, path

					ReverseInfo<ClipperLib::Path>::Get(LuaXS::UD<ClipperLib::Path>(L, -1))->mSize = size;

					try {
						return AddPoint(L);
					} CATCH()
				}
			},
			{ nullptr, nullptr }
		};

		luaL_register(L, nullptr, funcs);
	});

	struct {
		const char * mName;
		int mSize;
	} other_sizes[] = {
		{ "ToPath8", 8 }, { "ToPath16", 16 }, { "ToPath64", 64 }
	};

	for (auto && osi : other_sizes)
	{
		LuaXS::NewTyped<int>(L, osi.mSize);	// clipper, ToPath, ToPathX

		lua_getmetatable(L, -2);// clipper, ToPath, ToPathX, ToPathMT
		lua_setmetatable(L, -2);// clipper, ToPath, ToPathX; ToPathX.meta = ToPathMT
		lua_setfield(L, -3, osi.mName);	// clipper = { ..., ToPathX = ToPathX }, ToPath
	}

	lua_setfield(L, -2, "ToPath");	// clipper = { ..., ToPath = ToPath }

	return 1;
}
