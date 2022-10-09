#include "tinyrenderer.h"
#include "utils/LuaEx.h"

//
//
//

#define MODEL_TYPE "sceneView3D.tiny.Model"

//
//
//

namespace tiny {

//
//
//

Model & GetModel (lua_State * L, int arg)
{
    return *LuaXS::CheckUD<Model>(L, arg, MODEL_TYPE);
}

//
//
//

Model::Model (void) : verts_(), faces_(), norms_(), uv_()
{
}

//
//
//

Vec3f Model::normal (int iface, int nthvert) const
{
	int idx = faces_[iface][nthvert]/*[2]*/; // <- STEVE CHANGE

	return norms_[idx];
}

//
//
//

std::vector<int> Model::face (int idx) const
{
// STEVE CHANGE
/*	std::vector<int> face;

	for (auto && elem : faces_[idx]) face.push_back(elem[0]);*/
// /STEVE CHANGE
	return faces_[idx];//face; <- STEVE CHANGE
}

//
//
//

void Model::RenderInfo::SetDiffuse (lua_State * L)
{
    lua_settop(L, 2);   // object / model, texture / nil

    Texture<unsigned char> * tex = !lua_isnil(L, 2) ? &GetTexture(L, 2) : nullptr;

    if (tex != mDiffuse)
    {
        if (tex)
        {
            mDiffuse = tex;
            mDiffuseRef = lua_ref(L, 1);// object / model; info.ref = texture
        }

        else if (mDiffuseRef != LUA_NOREF)
        {
            lua_unref(L, mDiffuseRef);
            
            mDiffuseRef = LUA_NOREF;
        }
        
        UpdateShaders();
    }
}

//
//
//

void Model::RenderInfo::SetNormal (lua_State * L)
{
    lua_settop(L, 2);   // object / model, texture / nil
    
    Texture<float> * tex = !lua_isnil(L, 2) ? &GetFloatTexture(L, 2) : nullptr;
    
    if (tex != mNormal)
    {
        if (tex)
        {
            mNormal = tex;
            mNormalRef = lua_ref(L, 1);// object / model; info.ref = texture
        }
        
        else if (mNormalRef != LUA_NOREF)
        {
            lua_unref(L, mNormalRef);
            
            mNormalRef = LUA_NOREF;
        }

        UpdateShaders();
    }
}

//
//
//

void Model::RenderInfo::UpdateShaders (void)
{
    if (mDiffuse)
    {
        if (mNormal)
        {
            mFragment = &Model::RenderInfo::diffuse_normals_fragment;
            mVertex = &Model::RenderInfo::diffuse_normals_vertex;
        }

        else
        {
            mFragment = &Model::RenderInfo::diffuse_fragment;
            mVertex = &Model::RenderInfo::diffuse_vertex;
        }
    }

    else
    {
        mFragment = &Model::RenderInfo::flat_fragment;
        mVertex = &Model::RenderInfo::basic_vertex;
    }
}

//
//
//

Color Model::RenderInfo::GetDiffuse (const Vec2f & uvf) const
{
	if (mDiffuse)
	{
		Vec2i uv{int(uvf[0] * mDiffuse->mW), int(uvf[1] * mDiffuse->mH)};

		uv.x = (std::max)(0, (std::min)(uv.x, mDiffuse->mW - 1));
		uv.y = (std::max)(0, (std::min)(uv.y, mDiffuse->mH - 1));

		int offset = mDiffuse->mComp * (uv[1] * mDiffuse->mW + uv[0]);

		Color out;

		for (int i = 0; i < mDiffuse->mComp; ++i) out.mBytes[i] = mDiffuse->mData[offset + i];

		return out;
	}

	else return Color{};
}

//
//
//

Vec3f Model::RenderInfo::GetNormal (const Vec2f & uvf) const
{
	if (mNormal)
	{
		Vec2i uv{int(uvf[0] * mNormal->mW), int(uvf[1] * mNormal->mH)};

		int offset = mNormal->mComp * (uv[1] * mNormal->mW + uv[0]);

		Vec3f out;
			
		for (int i = 0; i < 3; ++i) out[i] = mNormal->mPixels[offset + i];

		return out;
	}

	else return Vec3f{0.0f, 1.0f, 0.0f};
}

//
//
//

Vec4f Model::RenderInfo::basic_vertex (const Scene & scene, RenderState & rs, const Model & model, const Matrix & mat, int iface, int nthvert) const
{
	Vec4f gl_Vertex = scene.mMVP * mat * embed<4>(model.vert(iface, nthvert));

	rs.varying_tri.set_col(nthvert, gl_Vertex);

	return gl_Vertex;
}

//
//
//

Vec4f Model::RenderInfo::diffuse_vertex (const Scene & scene, RenderState & rs, const Model & model, const Matrix & mat, int iface, int nthvert) const
{
	Vec4f gl_Vertex = basic_vertex(scene, rs, model, mat, iface, nthvert);

	rs.varying_uv.set_col(nthvert, model.uv(iface, nthvert));

    return gl_Vertex;
}

//
//
//

Vec4f Model::RenderInfo::diffuse_normals_vertex (const Scene & scene, RenderState & rs, const Model & model, const Matrix & mat, int iface, int nthvert) const
{
	Vec4f gl_Vertex = diffuse_vertex(scene, rs, model, mat, iface, nthvert);

    rs.ndc_tri.set_col(nthvert, proj<3>(gl_Vertex / gl_Vertex[3]));
    rs.varying_nrm.set_col(nthvert, proj<3>(scene.mIT * embed<4>(model.normal(iface, nthvert), 0.f)));

    return gl_Vertex;
}

//
//
//

Color Model::RenderInfo::flat_fragment (const Scene &, const RenderState &, const Vec3f &) const
{
	Color white = { 0xFF, 0xFF, 0xFF, 0xFF };

    return white;
}

//
//
//

Color Model::RenderInfo::diffuse_fragment (const Scene &, const RenderState & rs, const Vec3f & bar) const
{
	return GetDiffuse(rs.varying_uv * bar);
}

//
//
//

Color Model::RenderInfo::diffuse_normals_fragment (const Scene & scene, const RenderState & rs, const Vec3f & bar) const
{
    Vec3f bn = (rs.varying_nrm * bar).normalize();
    Vec2f uv = rs.varying_uv * bar;
    mat<3, 3, float> A;

    A[0] = rs.ndc_tri.col(1) - rs.ndc_tri.col(0);
    A[1] = rs.ndc_tri.col(2) - rs.ndc_tri.col(0);
    A[2] = bn;

    mat<3, 3, float> AI = A.invert();
	Vec3f i = AI * Vec3f{rs.varying_uv[0][1] - rs.varying_uv[0][0], rs.varying_uv[0][2] - rs.varying_uv[0][0], 0};
	Vec3f j = AI * Vec3f{rs.varying_uv[1][1] - rs.varying_uv[1][0], rs.varying_uv[1][2] - rs.varying_uv[1][0], 0};
    mat<3, 3, float> B;

    B.set_col(0, i.normalize());
    B.set_col(1, j.normalize());
    B.set_col(2, bn);

    Vec3f n = (B * GetNormal(uv)).normalize();
    float diff = (std::max)(0.f, n * scene.mLightDir);

    return GetDiffuse(uv) * diff;
}

//
//
//

void add_model (lua_State * L)
{
    lua_pushcfunction(L, [](lua_State * L)
    {
		LuaXS::NewTyped<Model>(L);	// model
        LuaXS::AttachMethods(L, MODEL_TYPE, [](lua_State * L) {
            luaL_Reg methods[] = {
                {
                    "AddFace", [](lua_State * L)
                    {
                        int n = (lua_gettop(L) - 1) / 3, j = 2;
                        
                        std::vector</*Vec3i*/int> corners;
                        
                        for (int i = 0; i < 3; ++i) corners.push_back(LuaXS::Int(L, j++) - 1);
                        
                        GetModel(L).faces_.push_back(corners);
                        
                        return 0;
                    }
                }, {
                    "AddNormal", [](lua_State * L)
                    {
                        GetModel(L).norms_.push_back(GetVec3(L).normalize());
                        
                        return 0;
                    }
                }, {
                    "AddUV", [](lua_State * L)
                    {
                        Vec2f uv;
                        
                        uv.x = LuaXS::Float(L, 2);
                        uv.y = 1.0f - LuaXS::Float(L, 3);
                        
                        GetModel(L).uv_.push_back(uv);
                        
                        return 0;
                    }
                }, {
                    "AddVertex", [](lua_State * L)
                    {
                        GetModel(L).verts_.push_back(GetVec3(L));
                        
                        return 0;
                    }
                }, {
                    "__gc", LuaXS::TypedGC<Model>
                }, {
                    "GetFaceVertexIndices", [](lua_State * L)
                    {
                        Model & model = GetModel(L);
                        int face = LuaXS::Int(L, 2) - 1;
                        
                        luaL_argcheck(L, face >= 0 && face < model.nfaces(), 2, "Bad face index");
                        
                        auto f = model.face(face);
                        
                        for (size_t i = 0; i < f.size(); ++i) lua_pushinteger(L, f[i] + 1);    // model, ..., index
                        
                        return int(f.size());
                    }
                }, {
                    "GetVertex", [](lua_State * L)
                    {
                        Model & model = GetModel(L);
                        int vert = LuaXS::Int(L, 2) - 1;
                        
                        luaL_argcheck(L, vert >= 0 && vert < model.nverts(), 2, "Bad vertex index");
                        
                        Vec3f v = model.vert(vert);
                        
                        for (int i = 0; i < 3; ++i) lua_pushnumber(L, v[i]);
                        
                        return 3;
                    }
                }, {
                    "__len", [](lua_State * L)
                    {
                        lua_pushinteger(L, GetModel(L).nfaces());
                        
                        return 1;
                    }
                }, {
                    "SetDiffuse", [](lua_State * L)
                    {
						GetModel(L).mInfo.SetDiffuse(L);
                        
                        return 0;
                    }
                }, {
                    "SetNormalMap", [](lua_State * L)
                    {
                        GetModel(L).mInfo.SetNormal(L);
                        
                        return 0;
                    }
                },
                { nullptr, nullptr }
            };
            
            luaL_register(L, nullptr, methods);
        });
        
        return 1;
    }); // ..., tinyrenderer
    lua_setfield(L, -2, "NewModel"); // ..., tinyrenderer = { ..., NewModel = NewModel }
}

//
//
//

}