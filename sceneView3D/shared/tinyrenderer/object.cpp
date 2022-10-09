#include "tinyrenderer.h"

//
//
//

#define OBJECT_TYPE "sceneView3D.tiny.Object"

//
//
//

namespace tiny {

//
//
//

Object & GetObject (lua_State * L, int arg)
{
    return *LuaXS::CheckUD<Object>(L, arg, OBJECT_TYPE);
}

//
//
//

Object::Object (Model & model) : mModel{model}, mInfo{&model.mInfo}
{
}

//
//
//

Object::~Object (void)
{
    RemoveUniqueState();
}

//
//
//

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

//
//
//

void Object::Destroy (void)
{
    RemoveUniqueState();
}

//
//
//

void Object::RemoveUniqueState (void)
{
    if (HasUniqueState()) delete mInfo;
}

//
//
//

struct RenderInfoRAII {
	Object & mObject;
	Model::RenderInfo mTemp, & mInfo;

	//
	//
	//

    RenderInfoRAII (Object & object) : mObject{object}, mInfo{object.HasUniqueState() ? *object.mInfo : mTemp}
	{
	}

	//
	//
	//

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

//
//
//

void add_object (lua_State * L)
{
    lua_pushcfunction(L, [](lua_State * L)
    {
		lua_settop(L, 1); // model

        Object * object = LuaXS::NewTyped<Object>(L, GetModel(L, 1)); // model, object

		lua_insert(L, 1); // object, model

		object->mModelRef = lua_ref(L, 1); // object; object.ref = model

        LuaXS::AttachMethods(L, OBJECT_TYPE, [](lua_State * L) {
            luaL_Reg methods[] = {
				{
                    "DetachSelf", [](lua_State * L)
                    {
						Object & object = GetObject(L);
						Node * parent = static_cast<Node *>(object.mParent);

						if (parent)
						{
							auto iter = std::find(parent->mObjects.begin(), parent->mObjects.end(), &object);

							if (iter != parent->mObjects.end()) parent->mObjects.erase(iter);
						}

						object.mParent = nullptr;

                        return 0;
                    }
				}, {
					"__gc", LuaXS::TypedGC<Object>
				}, {
					"GetParent", [](lua_State * L)
					{
                        GetFromStore(L, GetObject(L).mParent); // object, parent
                        
                        return 1;
					}
				}, {
					"SetDiffuse", [](lua_State * L)
					{
						RenderInfoRAII ri{GetObject(L)};

						ri.mInfo.SetDiffuse(L);

						return 0;
					}
				}, {
					"RemoveSelf", [](lua_State * L)
					{
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
						GetObject(L).SetPosition(L);

						return 0;
					}
				}, {
					"SetRotation", [](lua_State * L)
					{
						GetObject(L).SetRotation(L);

						return 0;
					}
				}, {
					"SetScale", [](lua_State * L)
					{
						GetObject(L).SetScale(L);

						return 0;
					}
				},
				{ nullptr, nullptr }
			};

			luaL_register(L, nullptr, methods);
        });

		AddToStore(L);

        return 1;
    });  // ..., tinyrenderer, NewObject
	lua_setfield(L, -2, "NewObject"); // ..., tinyrenderer = { ..., NewObject = NewObject }
}

//
//
//

}
