#include "capture3d.h"
#include "geometry.h"
#include "utils/LuaEx.h"

Object::Object (Model & model) : mModel{model}, mInfo{&model.mInfo}
{
}

Object::~Object (void)
{
    RemoveUniqueState();
}

Vec3f Object::Barycentric (const Vec2f & A, const Vec2f & B, const Vec2f & C, const Vec2f & P) const
{
	Vec3f s[2];

	for (int i = 0; i < 2; ++i)
	{
		s[i][0] = C[i] - A[i];
		s[i][1] = B[i] - A[i];
		s[i][2] = A[i] - P[i];
	}

	Vec3f u = cross(s[0], s[1]);

	if (std::abs(u[2]) > 1e-12) // dont forget that u[2] is integer. If it is zero then triangle ABC is degenerate
		return Vec3f{1.0f - (u.x + u.y) / u.z, u.y / u.z, u.x / u.z};
	return Vec3f{-1, 1, 1}; // in this case generate negative coordinates, it will be thrown away by the rasterizator
}

void Object::Destroy (void)
{
    RemoveUniqueState();
}

void Object::RemoveUniqueState (void)
{
    if (HasUniqueState()) delete mInfo;
}

Object & GetObject (lua_State * L, int arg)
{
    return *LuaXS::CheckUD<Object>(L, arg, "capture3d.Object");
}

struct RenderInfoRAII {
	Object & mObject;
	Model::RenderInfo mTemp, & mInfo;

    RenderInfoRAII (Object & object) : mObject{object}, mInfo{object.HasUniqueState() ? *object.mInfo : mTemp}
	{
	}

	~RenderInfoRAII (void)
	{
		if (!mInfo.mDiffuse && !mInfo.mNormal)
		{
			mObject.RemoveUniqueState();

			mObject.mInfo = &mObject.mModel.mInfo;
		}

		else if (!mObject.HasUniqueState()) mObject.mInfo = new Model::RenderInfo{mTemp};
	}
};

void open_object (lua_State * L)
{
    LuaXS::NewWeakKeyedTable(L);// capture3d, object_to_model

    AddConstructor(L, "NewObject", [](lua_State * L)
    {
        LuaXS::NewTyped<Object>(L, GetModel(L, 2)); // model, object

        lua_pushvalue(L, lua_upvalueindex(1));  // model, object, object_to_model
        lua_pushvalue(L, -2);   // model, object, object_to_model, object
        lua_pushvalue(L, 1);// model, object, object_to_model = { ..., [object] = model }
        lua_pop(L, 1);  // model, object

        LuaXS::AttachMethods(L, "capture3d.Object", [](lua_State * L) {
            luaL_Reg methods[] = {
				{
					"__gc", LuaXS::TypedGC<Object>
				}, {
					"SetDiffuse", [](lua_State * L)
					{
						RenderInfoRAII ri{GetObject(L)};

						ri.mInfo.SetDiffuse(L);

						return 0;
					}
				}, {
					"SetNormal", [](lua_State * L)
					{
						RenderInfoRAII ri{GetObject(L)};

						ri.mInfo.SetNormal(L);

						return 0;
					}
				}, {
					"SetPosition", [](lua_State * L)
					{
						GetObject(L).mXform.SetPosition(L);

						return 0;
					}
				}, {
					"SetRotation", [](lua_State * L)
					{
						GetObject(L).mXform.SetRotation(L);

						return 0;
					}
				}, {
					"SetScale", [](lua_State * L)
					{
						GetObject(L).mXform.SetScale(L);

						return 0;
					}
				},
				{ nullptr, nullptr }
			};

			luaL_register(L, nullptr, methods);
        });

        return 1;
    }, 1);  // capture3d = { ..., NewObject = NewObject }
}
