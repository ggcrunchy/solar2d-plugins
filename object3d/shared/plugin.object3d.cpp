#include "CoronaLua.h"
#include "ByteReader.h"
#include "utils/Blob.h"
#include "utils/LuaEx.h"
#include "utils/Thread.h"
#include "geometry.h"
#include <algorithm>
#include <vector>

struct Color {
	unsigned char mBytes[4];

	Color operator * (float scale)
	{
		Color out;

		for (int i = 0; i < 4; ++i) out.mBytes[i] = static_cast<unsigned char>(scale * mBytes[i]);

		return out;
	}
};

//
template<typename T> struct Texture {
	std::vector<T> mPixels;
	int mComp{0}, mW{0}, mH{0};

	Texture (void) : mPixels()
	{
	}

	static void Assign (std::vector<T> & pixels, const ByteReader & reader, size_t n)
	{
		const T * from = static_cast<const T *>(reader.mBytes);

		pixels.assign(from, from + n / sizeof(T));
	}

	template<typename F> void Bind (lua_State * L, int arg, int w, int h, int comp, F && func)
	{
		ByteReader reader{L, arg};

		size_t n = size_t(w * h * comp);

		luaL_argcheck(L, reader.mBytes && reader.mCount >= n, arg, "Not enough bytes");

		mComp = comp;
		mW = w;
		mH = h;

		func(mPixels, reader, n);
	}
};

//
struct Model {std::vector<Vec3f> verts_;
    std::vector<std::vector<Vec3i>> faces_; // attention, this Vec3i means vertex/uv/normal
    std::vector<Vec3f> norms_;
    std::vector<Vec2f> uv_;
    Texture<unsigned char> mDiffuse;
	Texture<float> mNormal;
	bool mDiffuseUVs{false};

	Model (void) : verts_(), faces_(), norms_(), uv_(), mDiffuse{}, mNormal{}
	{
	}

	int nverts (void) const { return int(verts_.size()); }
	int nfaces (void) const { return int(faces_.size()); }

    Vec3f normal (int iface, int nthvert) const
	{
		int idx = faces_[iface][nthvert][2];

		return norms_[idx];
	}

    Vec3f normal (Vec2f uvf) const
	{		
		if (!mNormal.mPixels.empty())
		{
			Vec2i uv{int(uvf[0] * mNormal.mW), int(uvf[1] * mNormal.mH)};

			int offset = mNormal.mComp * (uv[1] * mNormal.mW + uv[0]); 

			Vec3f out;
			
			for (int i = 0; i < 3; ++i) out[i] = mNormal.mPixels[offset + i];

			return out;
		}

		else return Vec3f{0.0f, 1.0f, 0.0f};
	}

	Vec3f vert (int i) const { return verts_[i]; }
	Vec3f vert (int iface, int nthvert) const { return verts_[faces_[iface][nthvert][0]]; }
	Vec2f uv (int iface, int nthvert) const { return uv_[faces_[iface][nthvert][1]]; }

    Color diffuse (Vec2f uvf) const
	{
		if (!mDiffuse.mPixels.empty())
		{
			Vec2i uv{int(uvf[0] * mDiffuse.mW), int(uvf[1] * mDiffuse.mH)};

			uv.x = (std::max)(0, (std::min)(uv.x, mDiffuse.mW - 1));
			uv.y = (std::max)(0, (std::min)(uv.y, mDiffuse.mH - 1));

			int offset = mDiffuse.mComp * (uv[1] * mDiffuse.mW + uv[0]); 

			Color out;

			for (int i = 0; i < mDiffuse.mComp; ++i) out.mBytes[i] = mDiffuse.mPixels[offset + i];

			return out;
		}

		else return Color();
	}

    std::vector<int> face (int idx) const
	{
		std::vector<int> face;

		for (auto && elem : faces_[idx]) face.push_back(elem[0]);

		return face;
	}
};

//
struct Object3D {
	enum Mode { eFlat, eDiffuse, eDiffuseUV, eDiffuseNormals };

	std::vector<double> mZBuffer;
	Model mModel;
	Matrix mModelView;
	Matrix mViewport;
	Matrix mProjection;
	Matrix mMVP;
	Matrix mIT;
	Vec3f mCenter{0, 0, 0};
	Vec3f mEye{1, 1, 3};
	Vec3f mLightDir{1, 1, 1};
	Vec3f mUp{0, 1, 0};
    mat<4, 3, float> varying_tri; // triangle coordinates (clip coordinates), written by VS, read by FS
	mat<2, 3, float> varying_uv;  // triangle uv coordinates, written by the vertex shader, read by the fragment shader
    mat<3, 3, float> ndc_tri;     // triangle in normalized device coordinates
    mat<3, 3, float> varying_nrm; // normal per vertex to be interpolated by FS
	Mode mMode{eFlat};
	int mComps, mW, mH;
	int mBlobRef{LUA_NOREF};
	unsigned char * mOutput{nullptr};

	Object3D (int w, int h, bool bHasAlpha) : mW{w}, mH{h}, mComps{bHasAlpha ? 4 : 3}, mModel{}, mZBuffer(w * h)
	{
		Viewport(0, 0, w, h);
		LookAt();
		SetLightDir(mLightDir);
		SetShader();
	}

	size_t Count (void) const { return size_t(mW * mH * mComps); }

	Vec3f Barycentric (Vec2f A, Vec2f B, Vec2f C, Vec2f P)
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

	void Clear (void)
	{
		std::fill_n(mOutput, Count(), static_cast<unsigned char>(0));
		std::fill_n(mZBuffer.data(), mZBuffer.size(), -std::numeric_limits<double>::max());
	}

	void LookAt (bool bProject = true)
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

	void SetLightDir (Vec3f light_dir)
	{
		mLightDir = proj<3>(mMVP * embed<4>(light_dir, 0.f)).normalize();
	}

	struct TriangleSetupInfo {
		const mat<4, 3, float> & mClipC;
		mat<3, 4, float> mPts;
		Vec2f mP, mQ, mR;
		float mNZ;
		
		TriangleSetupInfo (const mat<4, 3, float> & clipc) : mClipC{clipc}
		{
		}
	};
	
	TriangleSetupInfo TriangleSetup (void);
	void Pixel (const TriangleSetupInfo & info, const Vec2f & p, double * z_buffer, unsigned char * output);
	void Triangle (void);

	void Viewport (int x, int y, int w, int h)
	{
		mViewport = Matrix::identity();

		mViewport[0][3] = x + w / 2.0f;
		mViewport[1][3] = y + h / 2.0f;
		mViewport[2][3] = 1.0f;
		mViewport[0][0] = w / 2.0f;
		mViewport[1][1] = h / 2.0f;
		mViewport[2][2] = 0.0f;
	}

	void SetShader (void)
	{
		if (mModel.mDiffuseUVs) mMode = eDiffuseUV;

		else if (!mModel.mDiffuse.mPixels.empty())
		{
			if (!mModel.mNormal.mPixels.empty()) mMode = eDiffuseNormals;
			else mMode = eDiffuse;
		}

		else mMode = eFlat;
	}

    Vec4f basic_vertex (int iface, int nthvert)
	{
		Vec4f gl_Vertex = mMVP * embed<4>(mModel.vert(iface, nthvert));

		varying_tri.set_col(nthvert, gl_Vertex);
		
		return gl_Vertex;
	}

    Vec4f diffuse_vertex (int iface, int nthvert)
	{
		Vec4f gl_Vertex = basic_vertex(iface, nthvert);

		varying_uv.set_col(nthvert, mModel.uv(iface, nthvert));

        return gl_Vertex;
	}

	Vec4f diffuse_normals_vertex (int iface, int nthvert)
	{
		Vec4f gl_Vertex = diffuse_vertex(iface, nthvert);

        ndc_tri.set_col(nthvert, proj<3>(gl_Vertex / gl_Vertex[3]));
        varying_nrm.set_col(nthvert, proj<3>(mIT * embed<4>(mModel.normal(iface, nthvert), 0.f)));

        return gl_Vertex;
	}

    Vec4f vertex (int iface, int nthvert)
	{
		switch (mMode)
		{
		case eDiffuse:
		case eDiffuseUV:
			return diffuse_vertex(iface, nthvert);
		case eDiffuseNormals:
			return diffuse_normals_vertex(iface, nthvert);
		default:
			return basic_vertex(iface, nthvert);
		}
	}

	Color flat_fragment (Vec3f) const
	{
		Color white = { 0xFF, 0xFF, 0xFF, 0xFF };

        return white;
    }

	Color diffuse_fragment (Vec3f bar) const
	{
		return mModel.diffuse(varying_uv * bar);
    }

	Color diffuse_uv_fragment (Vec3f bar) const
	{
        Vec2f uv = varying_uv * bar;

		float u1 = uv.x, u2 = uv.x * 255.0f, v1 = uv.y, v2 = uv.y * 255.0f;

		u1 -= floor(u1);
		u2 -= floor(u2);
		v1 -= floor(v1);
		v2 -= floor(v2);
		u1 -= u2 / 255.0f;
		v1 -= v2 / 255.0f;
	
		Color color = {
			(unsigned char)(u1 * 0xFF),
			(unsigned char)(u2 * 0xFF),
			(unsigned char)(v1 * 0xFF),
			(unsigned char)(v2 * 0xFF)
		};

		return color;
    }

	Color diffuse_normals_fragment (Vec3f bar) const
	{
        Vec3f bn = (varying_nrm * bar).normalize();
        Vec2f uv = varying_uv * bar;
        mat<3, 3, float> A;

        A[0] = ndc_tri.col(1) - ndc_tri.col(0);
        A[1] = ndc_tri.col(2) - ndc_tri.col(0);
        A[2] = bn;

        mat<3, 3, float> AI = A.invert();
		Vec3f i = AI * Vec3f{varying_uv[0][1] - varying_uv[0][0], varying_uv[0][2] - varying_uv[0][0], 0};
		Vec3f j = AI * Vec3f{varying_uv[1][1] - varying_uv[1][0], varying_uv[1][2] - varying_uv[1][0], 0};
        mat<3, 3, float> B;

        B.set_col(0, i.normalize());
        B.set_col(1, j.normalize());
        B.set_col(2, bn);

        Vec3f n = (B * mModel.normal(uv)).normalize();
        float diff = (std::max)(0.f, n * mLightDir);

        return mModel.diffuse(uv) * diff;
    }

	Color fragment (Vec3f bar) const
	{
		switch (mMode)
		{
		case eDiffuse:
			return diffuse_fragment(bar);
		case eDiffuseUV:
			return diffuse_uv_fragment(bar);
		case eDiffuseNormals:
			return diffuse_normals_fragment(bar);
		default:
			return flat_fragment(bar);
		}
	}
};

#define OBJECT_NAME "Object3dXS"

static Object3D * Object (lua_State * L, int arg = 1)
{
	return LuaXS::CheckUD<Object3D>(L, arg, OBJECT_NAME);
}

static Model & GetModel (lua_State * L, int arg = 1)
{
	return Object(L, arg)->mModel;
}

Object3D::TriangleSetupInfo Object3D::TriangleSetup (void)
{
	TriangleSetupInfo info{varying_tri};

	info.mPts = (mViewport * info.mClipC).transpose(); // transposed to ease access to each of the points
	info.mP = proj<2>(info.mPts[0] / info.mPts[0][3]);
	info.mQ = proj<2>(info.mPts[1] / info.mPts[1][3]);
	info.mR = proj<2>(info.mPts[2] / info.mPts[2][3]);

	Vec2f pq = info.mQ - info.mP, pr = info.mR - info.mP;

	info.mNZ = pq.x * pr.y - pq.y * pr.x;

	return info;
}

void Object3D::Pixel (const Object3D::TriangleSetupInfo & info, const Vec2f & p, double * z_buffer, unsigned char * output)
{
	Vec3f bc_screen = Barycentric(info.mP, info.mQ, info.mR, p);
	Vec3f bc_clip{bc_screen.x / info.mPts[0][3], bc_screen.y / info.mPts[1][3], bc_screen.z / info.mPts[2][3]};

	bc_clip = bc_clip / (bc_clip.x + bc_clip.y + bc_clip.z);

	double frag_depth = info.mClipC[2] * bc_clip;

	if (bc_screen.x < 0 || bc_screen.y < 0 || bc_screen.z < 0 || *z_buffer > frag_depth) return;

	*z_buffer = frag_depth;

	Color color = fragment(bc_clip);

	for (int i = 0; i < mComps; ++i) output[i] = color.mBytes[i];
}

static void UpdateBounds (Vec2f & bmax, Vec2f & bmin, const Vec2f & p)
{
	for (int j = 0; j < 2; ++j)
	{
		bmax[j] = (std::max)(bmax[j], p[j]);
		bmin[j] = (std::min)(bmin[j], p[j]);
	}
}

void Object3D::Triangle (void)
{
	auto info = TriangleSetup();

	if (info.mNZ >= 0.0f)
	{
		Vec2f bmax{0.0f, 0.0f}, bmin{float(mW) - 1, float(mH) - 1};

		UpdateBounds(bmax, bmin, info.mP);
		UpdateBounds(bmax, bmin, info.mQ);
		UpdateBounds(bmax, bmin, info.mR);

		int xmin = (std::max)(int(bmin.x), 0), xmax = (std::min)(int(bmax.x), mW - 1);
		int ymin = (std::max)(int(bmin.y), 0), ymax = (std::min)(int(bmax.y), mH - 1);

		ThreadXS::parallel_for(ymin, ymax + 1, [this, xmax, xmin, info](int y) {
			int pos = xmin + (mH - y - 1) * mW;
			double * z_buffer = mZBuffer.data() + pos;
			unsigned char * output = mOutput + pos * mComps;

			for (int x = xmin; x <= xmax; ++x, output += mComps) Pixel(info, Vec2f{float(x), float(y)}, z_buffer++, output);
		}, (xmax - xmin) * (ymax - ymin) >= 1024);
	}
}

static Vec3f GetVec3 (lua_State * L, int first = 2)
{
	return Vec3f{LuaXS::Float(L, first), LuaXS::Float(L, first + 1), LuaXS::Float(L, first + 2)};
}

CORONA_EXPORT int luaopen_plugin_object3d (lua_State * L)
{
	lua_newtable(L);// object3d

	luaL_Reg object3d_funcs[] = {
		{
			"New", [](lua_State * L)
			{
				Object3D * O = LuaXS::NewTyped<Object3D>(L, LuaXS::Int(L, 1), LuaXS::Int(L, 2), LuaXS::Bool(L, 3));	// w, h[, has_alpha], object

				LuaXS::AttachMethods(L, OBJECT_NAME, [](lua_State * L)
				{
					luaL_Reg methods[] = {
						{
							"AddFace", [](lua_State * L)
							{
								int n = (lua_gettop(L) - 1) / 3, j = 2;

								std::vector<Vec3i> corners;

								for (int i = 0; i < 3; ++i)
								{
									Vec3i f{0, 0, 0};

									for (int k = 0; k < n; ++k) f[k] = LuaXS::Int(L, j++) - 1;

									corners.push_back(f);
								}

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
							"Clear", [](lua_State * L)
							{
								Object(L)->Clear();

								return 0;
							}
						}, {
							"__gc", [](lua_State * L)
							{
								printf("OBJ!\n");
								lua_unref(L, Object(L)->mBlobRef);

								LuaXS::DestructTyped<Object3D>(L, 1);

								return 0;
							}
						}, {
							"GetBlob", [](lua_State * L)
							{
								lua_getref(L, Object(L)->mBlobRef);	// object, blob

								return 1;
							}
						}, {
							"GetBytes", [](lua_State * L)
							{
								Object3D * O = Object(L);

								lua_pushlstring(L, reinterpret_cast<const char *>(O->mOutput), O->Count());	// object, str

								return 1;
							}
						}, {
							"GetColor", [](lua_State * L)
							{
								Object3D * O = Object(L);
								int face = LuaXS::Int(L, 2) - 1;

								luaL_argcheck(L, face >= 0 && face < O->mModel.nfaces(), 2, "Bad face index");

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

								if (b.x >= 0.0f && b.y >= 0.0f && b.z >= 0.0f)
								{
									for (int j = 0; j < 3; ++j) O->vertex(face, j);

									Color result = O->fragment(b);

									lua_pushnumber(L, t);	// object, face, ex, ey, ez, cx, cy, cz, t

									for (int i = 0; i < O->mComps; ++i) lua_pushnumber(L, double(result.mBytes[i]) / 255.0);// object, face, ex, ey, ez, cx, cy, cz, t, ..., comp

									return O->mComps + 1;
								}

								lua_pushnil(L);	// object, face, ex, ey, ez, cx, cy, cz, nil

								return 1;
							}
						}, {
							"GetFaceVertexIndices", [](lua_State * L)
							{
								Object3D * O = Object(L);
								int face = LuaXS::Int(L, 2) - 1;

								luaL_argcheck(L, face >= 0 && face < O->mModel.nfaces(), 2, "Bad face index");

								auto f = O->mModel.face(face);

								for (size_t i = 0; i < f.size(); ++i) lua_pushinteger(L, f[i] + 1);	// object, ..., index

								return int(f.size());
							}
						}, {
							"GetVertex", [](lua_State * L)
							{
								Object3D * O = Object(L);
								int vert = LuaXS::Int(L, 2) - 1;

								luaL_argcheck(L, vert >= 0 && vert < O->mModel.nverts(), 2, "Bad vertex index");

								Vec3f v = O->mModel.vert(vert);

								for (int i = 0; i < 3; ++i) lua_pushnumber(L, v[i]);

								return 3;
							}
						}, {
							"__len", [](lua_State * L)
							{
								lua_pushinteger(L, Object(L)->mModel.nfaces());

								return 1;
							}
						}, {
							"Render", [](lua_State * L)
							{
								Object3D * O = Object(L);

								O->Clear();

								for (int i = 0; i < O->mModel.nfaces(); i++)
								{
									for (int j = 0; j < 3; j++) O->vertex(i, j);

									O->Triangle();
								}

								return 0;
							}
						}, {
							"SetCenter", [](lua_State * L)
							{
								Object3D * O = Object(L);

								O->mCenter = GetVec3(L);

								O->LookAt();

								return 0;
							}
						}, {
							"SetDiffuse", [](lua_State * L)
							{
								Object3D * O = Object(L);
								Model & model = O->mModel;

								model.mDiffuseUVs = false;

								if (!lua_isnoneornil(L, 2))
								{
									lua_settop(L, 5);	// object, uvs?[, w, h, comps]
									lua_pushliteral(L, "uvs");	// object, uvs?[, w, h, comps], "uvs"

									if (!lua_equal(L, 2, -1))
									{
										int comps = luaL_optint(L, 5, 3);

										model.mDiffuse.Bind(L, 2, LuaXS::Int(L, 3), LuaXS::Int(L, 4), comps, Texture<unsigned char>::Assign);
									}

									else model.mDiffuseUVs = true;
								}

								else model.mDiffuse.mPixels.clear();

								O->SetShader();

								return 0;
							}
						}, {
							"SetEye", [](lua_State * L)
							{
								Object3D * O = Object(L);

								O->mEye = GetVec3(L);

								O->LookAt();
								
								return 0;
							}
						}, {
							"SetLightDir", [](lua_State * L)
							{
								Object(L)->SetLightDir(GetVec3(L));

								return 0;
							}
						}, {
							"SetNormalMap", [](lua_State * L)
							{
								Object3D * O = Object(L);
								Model & model = O->mModel;
								
								if (!lua_isnoneornil(L, 2))
								{
									int w = LuaXS::Int(L, 3), h = LuaXS::Int(L, 4), comps = luaL_optint(L, 5, 3);

									luaL_argcheck(L, comps >= 3, 5, "Not enough components for normal map");

									model.mNormal.Bind(L, 2, w, h, comps, [](std::vector<float> & pixels, const ByteReader & reader, size_t n) {
										auto from = static_cast<const unsigned char *>(reader.mBytes);

										for (size_t i = 0; i < n; ++i) pixels[i] = 2.0f * (float(from[i]) / 255.0f) - 1.0f;
									});
								}

								else model.mNormal.mPixels.clear();

								O->SetShader();

								return 0;
							}
						}, {
							"SetUp", [](lua_State * L)
							{
								Object3D * O = Object(L);

								O->mUp = GetVec3(L).normalize();

								O->LookAt();

								return 0;
							}
						},
						{ nullptr, nullptr }
					};

					luaL_register(L, nullptr, methods);
				});

				BlobXS::NewBlob(L, O->Count());	// w, h[, has_alpha], object, blob

				O->mOutput = BlobXS::GetData(L, -1);
				O->mBlobRef = lua_ref(L, 1);// w, h[, has_alpha], object

				return 1;
			}
		},
		{ nullptr, nullptr }
	};
	
	luaL_register(L, nullptr, object3d_funcs);

	return 1;
}
