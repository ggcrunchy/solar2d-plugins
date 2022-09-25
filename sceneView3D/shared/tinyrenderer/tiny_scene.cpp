#include "tinyrenderer.h"
#include "geometry.h"
#include "utils/Byte.h"
#include "utils/LuaEx.h"
#include "utils/Thread.h"
#include <algorithm>

//
//
//

#define SCENE_TYPE "scene3d.tiny.Scene"

//
//
//

namespace tiny {

//
//
//

Scene & GetScene (lua_State * L, int arg)
{
    return *LuaXS::CheckUD<Scene>(L, arg, SCENE_TYPE);
}

//
//
//

Scene::Scene (int w, int h, bool bHasAlpha) : mW{w}, mH{h}, mComps{bHasAlpha ? 4 : 3}, mZBuffer(w * h)
{
	Viewport(0, 0, w, h);
	LookAt();
	SetLightDir(mLightDir);
}

//
//
//

Scene::TriangleSetupInfo Scene::TriangleSetup (const Model::RenderState & rs)
{
	TriangleSetupInfo info{rs};

	info.mPts = (mViewport * rs.varying_tri).transpose(); // transposed to ease access to each of the points
	info.mP = proj<2>(info.mPts[0] / info.mPts[0][3]);
	info.mQ = proj<2>(info.mPts[1] / info.mPts[1][3]);
	info.mR = proj<2>(info.mPts[2] / info.mPts[2][3]);

	Vec2f pq = info.mQ - info.mP, pr = info.mR - info.mP;

	info.mNZ = pq.x * pr.y - pq.y * pr.x;

	return info;
}

//
//
//

void Scene::Clear (void)
{
	std::fill_n(mOutput, Count(), static_cast<unsigned char>(0));
	std::fill_n(mZBuffer.data(), mZBuffer.size(), -std::numeric_limits<double>::max());
}

//
//
//

void Scene::LookAt (bool bProject)
{
	float coeff = (mEye - mCenter).norm();

	Vec3f z = (mEye - mCenter) / coeff;
	Vec3f x = cross(mUp, z).normalize();
	Vec3f y = cross(z, x).normalize();
	Matrix Minv = Matrix::identity();
	Matrix Tr = Matrix::identity();

	for (int i = 0; i < 3; i++)
	{
		Minv[0][i] = x[i];
		Minv[1][i] = y[i];
		Minv[2][i] = z[i];
		Tr[i][3] = -mCenter[i];
	}

	mModelView = Minv * Tr;
	mProjection = Matrix::identity();

	if (bProject) mProjection[3][2] = -1.0f / coeff;

	mMVP = mProjection * mModelView;
	mIT = mMVP.invert_transpose();
}

//
//
//

void Scene::Pixel (const TriangleSetupInfo & info, const Object & object, const Vec2f & p, double * z_buffer, unsigned char * output)
{
	Vec3f bc_screen = object.Barycentric(info.mP, info.mQ, info.mR, p);
	Vec3f bc_clip{bc_screen.x / info.mPts[0][3], bc_screen.y / info.mPts[1][3], bc_screen.z / info.mPts[2][3]};

	bc_clip = bc_clip / (bc_clip.x + bc_clip.y + bc_clip.z);

	double frag_depth = info.mRS.varying_tri[2] * bc_clip;

	if (bc_screen.x < 0 || bc_screen.y < 0 || bc_screen.z < 0 || *z_buffer > frag_depth) return;

	*z_buffer = frag_depth;

	auto fragfn = object.mInfo->mFragment;
	Color color = (object.mInfo->*fragfn)(*this, info.mRS, bc_clip);

	for (int i = 0; i < mComps; ++i) output[i] = color.mBytes[i];
}

//
//
//

void Scene::SetLightDir (const Vec3f & light_dir)
{
	mLightDir = proj<3>(mMVP * embed<4>(light_dir, 0.f)).normalize();
}

//
//
//

static void UpdateBounds (Vec2f & bmax, Vec2f & bmin, const Vec2f & p)
{
	for (int j = 0; j < 2; ++j)
	{
		bmax[j] = (std::max)(bmax[j], p[j]);
		bmin[j] = (std::min)(bmin[j], p[j]);
	}
}

//
//
//

void Scene::Triangle (const Object & object, const Model::RenderState & rs)
{
	auto info = TriangleSetup(rs);

	if (info.mNZ >= 0.0f)
	{
		Vec2f bmax{0.0f, 0.0f}, bmin{float(mW) - 1, float(mH) - 1};

		UpdateBounds(bmax, bmin, info.mP);
		UpdateBounds(bmax, bmin, info.mQ);
		UpdateBounds(bmax, bmin, info.mR);

		int xmin = (std::max)(int(bmin.x), 0), xmax = (std::min)(int(bmax.x), mW - 1);
		int ymin = (std::max)(int(bmin.y), 0), ymax = (std::min)(int(bmax.y), mH - 1);

		ThreadXS::parallel_for(ymin, ymax + 1, [this, xmax, xmin, info, object](int y) {
			int pos = xmin + (mH - y - 1) * mW;
			double * z_buffer = mZBuffer.data() + pos;
			unsigned char * output = mOutput + pos * mComps;

			for (int x = xmin; x <= xmax; ++x, output += mComps) Pixel(info, object, Vec2f{float(x), float(y)}, z_buffer++, output);
		}, (xmax - xmin) * (ymax - ymin) >= 1024);
	}
}

//
//
//

void Scene::Viewport (int x, int y, int w, int h)
{
	mViewport = Matrix::identity();

	mViewport[0][3] = x + w / 2.0f;
	mViewport[1][3] = y + h / 2.0f;
	mViewport[2][3] = 1.0f;
	mViewport[0][0] = w / 2.0f;
	mViewport[1][1] = h / 2.0f;
	mViewport[2][2] = 0.0f;
}

//
//
//

static Vec4f Vec4 (float x, float y, float z, float w)
{
	Vec4f v;

	v[0] = x;
	v[1] = y;
	v[2] = z;
	v[3] = w;

	return v;
}

//
//
//

void Transform::SetPosition (lua_State * L)
{
	if (!lua_isnumber(L, 2)) ByteXS::EnsureFloatsN(L, 2, 3, mPosition, 3, false);
	else for (int i = 0; i < 3; ++i) mPosition[i] = LuaXS::Float(L, i + 2);
}

//
//
//

void Transform::SetRotation (lua_State * L)
{
	if (!lua_isnumber(L, 2)) ByteXS::EnsureFloatsN(L, 2, 4, mRotation, 4, false);
	else for (int i = 0; i < 4; ++i) mRotation[i] = LuaXS::Float(L, i + 2);

	float len_sq = mRotation[0] * mRotation[0] + mRotation[1] * mRotation[1] + mRotation[2] * mRotation[2] + mRotation[3] * mRotation[3];

	if (1.f + len_sq != 1.f)
	{
		float len = sqrt(len_sq);

		for (int i = 0; i < 4; ++i) mRotation[i] /= len;
	}
}

//
//
//

void Transform::SetScale (lua_State * L)
{
	if (!lua_isnumber(L, 2)) ByteXS::EnsureFloatsN(L, 2, 3, mScale, 3, false);
	else for (int i = 0; i < 3; ++i) mScale[i] = LuaXS::Float(L, i + 2);
}

//
//
//

Matrix Transform::ToMatrix (void)
{
	Matrix T, R, S;

	T.set_col(0, Vec4(1.f, 0.f, 0.f, mPosition[0]));
	T.set_col(1, Vec4(0.f, 1.f, 0.f, mPosition[1]));
	T.set_col(2, Vec4(0.f, 0.f, 1.f, mPosition[2]));
	T.set_col(3, Vec4(0.f, 0.f, 0.f, 1.f));

	float qx = mRotation[0], qy = mRotation[1], qz = mRotation[2], qw = mRotation[3];

	R.set_col(0, Vec4(1.f - 2.f * (qy * qy + qz * qz), 2.f * (qx * qy + qz * qw), 2.f * (qx * qz - qy * qw), 0.f));
	R.set_col(1, Vec4(2.f * (qx * qy - qz * qw), 1.f - 2.f * (qx * qx + qz * qz), 2.f * (qy * qz + qx * qw), 0.f));
	R.set_col(2, Vec4(2.f * (qx * qz + qy * qw), 2.f * (qy * qz - qx * qw), 1.f - 2.f * (qx * qx + qy * qy), 0.f));
	R.set_col(3, Vec4(0.f, 0.f, 0.f, 1.f));

	S.set_col(0, Vec4(mScale[0], 0.f, 0.f, mPosition[0]));
	S.set_col(1, Vec4(0.f, mScale[1], 0.f, mPosition[1]));
	S.set_col(2, Vec4(0.f, 0.f, mScale[2], mPosition[2]));
	S.set_col(3, Vec4(0.f, 0.f, 0.f, 1.f));

	return T * R * S;
}

//
//
//

template<typename F> void VisitScene (lua_State * L, Scene & scene, Model::RenderState & rs, F && func)
{
    std::vector<Group *> stack;
	std::vector<Matrix> mstack;

	lua_getref(L, scene.mGroupRef);	// scene, group

	Group & group = GetGroup(L, -1);

	stack.push_back(&group);
	mstack.push_back(group.mXform.ToMatrix());

	while (!stack.empty())
	{
		Group * cur = stack.back();
		Matrix gmat = mstack.back();

		stack.pop_back();
		mstack.pop_back();

		for (Object * object : cur->mObjects)
		{
			auto vert_fn = object->mInfo->mVertex;

			for (int i = 0; i < object->mModel.nfaces(); i++)
			{
				for (int j = 0; j < 3; j++) (object->mInfo->*vert_fn)(scene, rs, object->mModel, gmat * object->mXform.ToMatrix(), i, j);
                   
				func(*object);
			}
		}

		for (auto iter = cur->mSubGroups.rbegin(); iter != cur->mSubGroups.rend(); ++iter)
		{
			stack.push_back(*iter);
			mstack.push_back(gmat * (*iter)->mXform.ToMatrix());
		}
	}
}

//
//
//

void open_scene (lua_State * L)
{
	lua_getfield(L, -1, "NewGroup"); // ..., tinyrenderer, NewGroup

    AddConstructor(L, "NewScene", [](lua_State * L)
    {
		bool bHasAlpha = false, bUsingBlob = false;

		LuaXS::Options{L, 3}.Add("has_alpha", bHasAlpha)
							.Add("using_blob", bUsingBlob);
	
		Scene * scene = LuaXS::NewTyped<Scene>(L, LuaXS::Int(L, 1), LuaXS::Int(L, 2), bHasAlpha);	// w, h[, opts], scene

		lua_pushvalue(L, lua_upvalueindex(1));	// w, h[, opts], scene, NewGroup
		lua_call(L, 0, 1);	// w, h[, opts], scene, group
		lua_getfenv(L, -1);	// w, h[, opts], scene, group, items
		lua_pushvalue(L, -3);	// w, h[, opts], scene, group, items, scene
		lua_setfield(L, -2, "scene");	// w, h[, opts], scene, group, items = { ..., scene = scene }
		lua_pop(L, 1);	// w, h[, opts], scene, group

		scene->mGroupRef = lua_ref(L, 1);	// w, h[, opts], scene; scene.group = group

 /*
 LuaXS::AttachMethods(L, OBJECT_NAME, [](lua_State * L)
 {
 luaL_Reg methods[] = {
 { nullptr, nullptr }
 };
 
 luaL_register(L, nullptr, methods);
 });
 
 BlobXS::NewBlob(L, O->Count());    // w, h[, has_alpha], object, blob
 
 O->mOutput = BlobXS::GetData(L, -1);
 O->mBlobRef = lua_ref(L, 1);// w, h[, has_alpha], object
 */
        LuaXS::AttachMethods(L, SCENE_TYPE, [](lua_State * L) {
            luaL_Reg methods[] = {
                {
                    "Clear", [](lua_State * L)
                    {
                        GetScene(L).Clear();
                        
                        return 0;
                    }
                }, {
                    "__gc", [](lua_State * L)
                    {
                        Scene & scene = GetScene(L);
                        
                        if (scene.mBlobRef != LUA_NOREF) lua_unref(L, scene.mBlobRef);
                        
                        LuaXS::DestructTyped<Scene>(L, 1);
                        
                        return 0;
                    }
                }, {
                    "GetBlob", [](lua_State * L)
                    {
                        lua_getref(L, GetScene(L).mBlobRef);// object, blob
                        
                        return 1;
                    }
                }, {
                    "GetBytes", [](lua_State * L)
                    {
                        Scene & scene = GetScene(L);
                        
                        lua_pushlstring(L, reinterpret_cast<const char *>(scene.mOutput), scene.Count());// object, str
                        
                        return 1;
                    }
                }, {
					 "GetColor", [](lua_State * L)
					 {
                         Scene & scene = GetScene(L);
                         
						 struct ColorSearch {
							 float mColor[4];
							 float mTime{-1.f};
						 };

                         ColorSearch cs;
                         Model::RenderState rs;

                         VisitScene(L, scene, rs, [&scene, &cs](Object & object) {
/*
							 Vec3f p = GetVec3(L, 3), q = GetVec3(L, 6);
							 Vec3f v1 = O->mModel.vert(face, 0);
							 Vec3f v2 = O->mModel.vert(face, 1);
							 Vec3f v3 = O->mModel.vert(face, 2);
							 Vec3f to_v2 = v2 - v1, to_v3 = v3 - v1;
							 Vec3f n = cross(to_v2, to_v3), dir = q - p, b = Vec3f{-1, 0, 0};
 
							 float denom = dir * n, t = abs(denom) > 1e-12f ? ((v1 - p) * n) / denom : -1.0f;
 
							 if (t >= 0.0f)
							 {
								 Vec3f right = to_v2 / to_v2.norm(), up = cross(n, right).normalize();
								 Vec2f q2 = Vec2f{to_v2 * right, 0}, r2 = Vec2f{to_v3 * right, to_v3 * up};
								 Vec3f to_ip = p + dir * t - v1;
								 Vec2f h2 = Vec2f{to_ip * right, to_ip * up};
 
								 b = O->Barycentric(Vec2f{0, 0}, q2, r2, h2);
							 }
 
							 if (b.x >= 0.0f && b.y >= 0.0f && b.z >= 0.0f) ...
 */
						});

						if (cs.mTime >= 0.f)
						{
/*
							 Color result = O->fragment(b);
 
							 lua_pushnumber(L, t);    // object, face, ex, ey, ez, cx, cy, cz, t
 
							 for (int i = 0; i < O->mComps; ++i) lua_pushnumber(L, double(result.mBytes[i]) / 255.0);// object, face, ex, ey, ez, cx, cy, cz, t, ..., comp
 
							 return O->mComps + 1;
*/
						}

						else lua_pushnil(L);// scene, group, nil

						return 1;
					}
				 }, {
					"GetGroup", [](lua_State * L)
					{
						lua_getref(L, GetScene(L).mGroupRef);	// scene, group
															
						return 1;
					}
				}, {
                    "Render", [](lua_State * L)
                    {
                        Scene & scene = GetScene(L);

						Model::RenderState rs;

						VisitScene(L, scene, rs, [&scene, &rs](Object & object) {
							scene.Triangle(object, rs);
						});
                        
                        return 0;
                    }
                }, {
                    "SetCenter", [](lua_State * L)
                    {
                        Scene & scene = GetScene(L);
                        
                        scene.mCenter = GetVec3(L);
                        
                        scene.LookAt();
                        
                        return 0;
                    }
                }, {
                    "SetEye", [](lua_State * L)
                    {
                        Scene & scene = GetScene(L);
                        
                        scene.mEye = GetVec3(L);
                        
                        scene.LookAt();
                        
                        return 0;
                    }
                }, {
                    "SetLightDir", [](lua_State * L)
                    {
                        GetScene(L).SetLightDir(GetVec3(L));
                        
                        return 0;
                    }
                }, {
                    "SetUp", [](lua_State * L)
                    {
                        Scene & scene = GetScene(L);
                        
                        scene.mUp = GetVec3(L).normalize();
                        
                        scene.LookAt();
                        
                        return 0;
                    }
                },
                { nullptr, nullptr }
            };
            
            luaL_register(L, nullptr, methods);
        });
        
        return 1;
    }, 1);	// tinyrenderer
}

//
//
//

}