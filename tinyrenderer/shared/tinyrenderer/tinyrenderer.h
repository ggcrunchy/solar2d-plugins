#pragma once

#include "common.h"
#include "geometry.h"
#include <type_traits>
#include <utility>
#include <vector>

//
//
//

namespace tiny {

//
//
//

struct Color {
	unsigned char mBytes[4];

	Color operator * (float scale);
};

//
//
//

template<typename T> struct Texture {
	std::vector<T> mPixels;
	int mComp{0}, mW{0}, mH{0};
    int mCount{0}, mRef{LUA_NOREF};
    const T * mData;

	Texture (void) : mPixels()
	{
	}

    const T * Get (lua_State * L) const
    {
        if (mRef == LUA_NOREF) return mPixels.data();
        else
        {
            lua_getref(L, mRef);// ..., bytes

            const char * str = lua_tostring(L, -1);

            lua_pop(L, 1);  // ...

            return reinterpret_cast<const T *>(str);
        }
    }
    
	static void Assign (lua_State *, Texture & tex, const ByteReader & reader, size_t n)
	{
		const T * from = static_cast<const T *>(reader.mBytes);

		tex.mPixels.assign(from, from + n / sizeof(T));

        tex.mData = tex.mPixels.data();
	}

    static void AssignOrRef (lua_State * L, Texture & tex, const ByteReader & reader, size_t n)
    {
        if (lua_type(L, reader.mPos) == LUA_TSTRING)
        {
            lua_pushvalue(L, reader.mPos);  // ..., str, ..., str
            
            tex.mData = reinterpret_cast<const T *>(lua_tostring(L, reader.mPos));

            tex.mRef = lua_ref(L, 1);   // ..., str, ...
            
            tex.mPixels.clear();
        }
        
        else Assign(L, tex, reader, n);
    }

	template<typename F> void Bind (lua_State * L, int arg, int w, int h, int comp, F && func)
	{
		ByteReader reader{L, arg};

		size_t n = size_t(w * h * comp);

		luaL_argcheck(L, reader.mBytes && reader.mCount >= n, arg, "Not enough bytes");

		mComp = comp;
		mW = w;
		mH = h;

        if (mRef != LUA_NOREF)
        {
            lua_unref(L, mRef);
            
            mRef = LUA_NOREF;
        }
        
        func(L, *this, reader, n);
	}
};

//
//
//

struct Scene;

//
//
//

struct Transform {
    Transform * mParent{nullptr};
    float mRotation[4] = {};
    float mPosition[3] = {};
    float mScale[3] = { 1, 1, 1 };

	void SetPosition (lua_State * L);
	void SetRotation (lua_State * L);
	void SetScale (lua_State * L);

	Matrix ToMatrix (void) const;
};

//
//
//

struct Model {
    struct RenderState {
        mat<4, 3, float> varying_tri; // triangle coordinates (clip coordinates), written by VS, read by FS
        mat<2, 3, float> varying_uv;  // triangle uv coordinates, written by the vertex shader, read by the fragment shader
        mat<3, 3, float> ndc_tri;     // triangle in normalized device coordinates
        mat<3, 3, float> varying_nrm; // normal per vertex to be interpolated by FS
    };
    
    struct RenderInfo {
        Texture<unsigned char> * mDiffuse{nullptr};
        Texture<float> * mNormal{nullptr};
		int mDiffuseRef{LUA_NOREF}, mNormalRef{LUA_NOREF};
        Color (RenderInfo::*mFragment)(const Scene &, const RenderState &, const Vec3f &) const;
        Vec4f (RenderInfo::*mVertex)(const Scene &, RenderState &, const Model & model, const Matrix & mat, int, int) const;

        RenderInfo () : mFragment{&RenderInfo::flat_fragment}, mVertex{&RenderInfo::basic_vertex}
        {
        }

		void SetDiffuse (lua_State * L);
		void SetNormal (lua_State * L);
        void UpdateShaders (void);

		Color GetDiffuse (const Vec2f & uvf) const;
		Vec3f GetNormal (const Vec2f & uvf) const;

        Vec4f basic_vertex (const Scene & scene, RenderState & rs, const Model & model, const Matrix & mat, int iface, int nthvert) const;
        Vec4f diffuse_vertex (const Scene & scene, RenderState & rs, const Model & model, const Matrix & mat, int iface, int nthvert) const;
        Vec4f diffuse_normals_vertex (const Scene & scene, RenderState & rs, const Model & model, const Matrix & mat, int iface, int nthvert) const;
        
        Color flat_fragment (const Scene & scene, const RenderState & rs, const Vec3f &) const;
        Color diffuse_fragment (const Scene & scene, const RenderState & rs, const Vec3f & bar) const;
        Color diffuse_normals_fragment (const Scene & scene, const RenderState & rs, const Vec3f & bar) const;
    };
    
    std::vector<Vec3f> verts_;
    std::vector<std::vector<int/*Vec3i*/>> faces_; // <- STEVE CHANGE (used to be: attention, this Vec3i means vertex/uv/normal)
    std::vector<Vec3f> norms_;
    std::vector<Vec2f> uv_;
    RenderInfo mInfo;

	Model (void);

	int nverts (void) const { return int(verts_.size()); }
	int nfaces (void) const { return int(faces_.size()); }

    Vec3f normal (int iface, int nthvert) const;
	Vec3f vert (int i) const { return verts_[i]; }
	Vec3f vert (int iface, int nthvert) const { return verts_[faces_[iface][nthvert]/*[0]*/]; } // <- STEVE CHANGE
	Vec2f uv (int iface, int nthvert) const { return uv_[faces_[iface][nthvert]/*[1]*/]; } // <- STEVE CHANGE

    std::vector<int> face (int idx) const;
};

//
//
//

struct Object : public Transform {
    Model & mModel;
    Model::RenderInfo * mInfo;
    int mModelRef;

	Vec3f Barycentric (const Vec2f & A, const Vec2f & B, const Vec2f & C, const Vec2f & P) const;
	
	Object (Model & model);
	~Object (void);

    bool HasUniqueState (void) const { return mInfo != nullptr && mInfo != &mModel.mInfo; }
    bool IsDestroyed (void) const { return mInfo == nullptr; }
    void MarkDestroyed (void) { mInfo = nullptr; }
    
    void Destroy (void);
    void RemoveUniqueState (void);
};

//
//
//

struct Node : public Transform {
    std::vector<Object *> mObjects;
    std::vector<Node *> mSubnodes;
    bool mIsSceneRoot{false};
    
    bool IsDestroyed (void) const { return mSubnodes.size() == 1U && mSubnodes.front() == this; }
    void MarkDestroyed (void) { mSubnodes.push_back(this); }
};

//
//
//

struct Scene {
	std::vector<double> mZBuffer;
	Matrix mModelView;
	Matrix mViewport;
	Matrix mProjection;
	Matrix mMVP;
	Matrix mIT;
	Vec3f mCenter{0, 0, 0};
	Vec3f mEye{1, 1, 3};
	Vec3f mLightDir{1, 1, 1};
	Vec3f mUp{0, 1, 0};
	int mComps, mW, mH;
	int mBlobRef{LUA_NOREF}, mRootRef;
	unsigned char * mOutput{nullptr};
    int64_t mID{0};
	
    // broadphase?
	// output, z-write modes (disable, external buffers, etc.)

	struct TriangleSetupInfo {
		const Model::RenderState & mRS;
		mat<3, 4, float> mPts;
		Vec2f mP, mQ, mR;
		float mNZ;

		TriangleSetupInfo (const Model::RenderState & rs) : mRS{rs}
		{
		}
	};

	Scene (int w, int h, bool bHasAlpha);

	size_t Count (void) const { return size_t(mW * mH * mComps); }
    bool IsDestroyed (void) const { return mComps == 0; }
    void MarkDestroyed (void) { mComps = 0; }
	
	TriangleSetupInfo TriangleSetup (const Model::RenderState & rs);

	void Clear (void);
    void Destroy (void);
	void LookAt (bool bProject = true);
	void Pixel (const TriangleSetupInfo & info, const Object & object, const Vec2f & p, double * z_buffer, unsigned char * output);
	void SetLightDir (const Vec3f & light_dir);
	void Triangle (const Object & object, const Model::RenderState & rs);
	void Viewport (int x, int y, int w, int h);
};

//
//
//

Vec3f GetVec3 (lua_State * L, int first = 2);

tiny::Model & GetModel (lua_State * L, int arg = 1);
tiny::Node & GetNode (lua_State * L, int arg = 1);
tiny::Object & GetObject (lua_State * L, int arg = 1);
tiny::Scene & GetScene (lua_State * L, int arg = 1);
tiny::Texture<unsigned char> & GetTexture (lua_State * L, int arg = 1);
tiny::Texture<float> & GetFloatTexture (lua_State * L, int arg = 1);

//
//
//

void add_model (lua_State * L);
void add_node (lua_State * L);
void add_object (lua_State * L);
void add_scene (lua_State * L);
void add_texture (lua_State * L);

//
//
//

}