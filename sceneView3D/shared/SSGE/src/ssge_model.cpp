// ===============================
// AUTHOR       : Angel Ortiz (angelo12 AT vt DOT edu)
// CREATE DATE  : 2018-07-03
// ===============================

//Headers
#include "model.h"

Mesh * Model::getMesh() { return &mMesh; } // <- STEVE CHANGE

const Mesh * Model::getMesh() const { // <- STEVE CHANGE
    return &mMesh;
}

void Model::update(){
    //Recalculate model matrix for movement or scaling
    mBounds.update(mModelMatrix);
}
AABox *Model::getBounds(){
    return &mBounds;
}
Matrix4 *Model::getModelMatrix(){
    return &mModelMatrix;
}
const Matrix4 *Model::getModelMatrix() const { return &mModelMatrix; } // <- STEVE CHANGE
//Texture getters
const Texture *Model::getAlbedo() const { // <- STEVE CHANGE
    return mAlbedo; // <- STEVE CHANGE
}
const Texture *Model::getNormal() const { // <- STEVE CHANGE
    return mNormal; // <- STEVE CHANGE
}
const Texture *Model::getAO() const { // <- STEVE CHANGE
    return mAmbient; // <- STEVE CHANGE
}
const Texture *Model::getRoughness() const { // <- STEVE CHANGE
    return mRoughness; // <- STEVE CHANGE
}
const Texture *Model::getMetallic() const { // <- STEVE CHANGE
    return mMetallic; // <- STEVE CHANGE
}

// STEVE CHANGE
void Model::Build ()
{
    mBounds.buildAABB(mMesh);
    mMesh.buildFacetNormals();
    mMesh.buildTangentSpace();
}

bool Model::IsReady () const
{
    if (!(getAlbedo()->HasData() && getNormal()->HasData() && getAO()->HasData() && getRoughness()->HasData() && getMetallic()->HasData())) return false;

    const Mesh * mesh = getMesh();

    if (0 == mesh->numFaces() || 0 == mesh->numVertices()) return false;
    if (mesh->vertices.size() != mesh->normals.size()) return false;
    if (mesh->vertices.size() != mesh->texels.size()) return false;

    return true;
}

//
//
//

#define MODEL_TYPE "sceneView3D.ssge.Model"
#define TRANSFORM_PARAMS_TYPE "sceneView3D.ssge.TransformParams"

//
//
//

Model * Model::Get (lua_State * L, int arg)
{
    return LuaXS::CheckUD<Model>(L, arg, MODEL_TYPE);
}

//
//
//

TransformParameters * GetXformParams (lua_State * L, int arg)
{
    return LuaXS::CheckUD<TransformParameters>(L, arg, TRANSFORM_PARAMS_TYPE);
}

//
//
//

static Texture * AddTexture (lua_State * L, const char * name, const char * type, std::vector<float> & workspace)
{
    Texture * texture = Texture::PushNew(L, type, &workspace); // ..., env, texture

    lua_setfield(L, -2, name); // ..., env = { ..., [name] = texture

    return texture;
}

//
//
//

void Model::add_model (lua_State * L)
{
    lua_pushcfunction(L, [](lua_State * L) {
        Model * model = LuaXS::NewTyped<Model>(L); // model

        LuaXS::AttachMethods(L, MODEL_TYPE, [](lua_State * L) {
            luaL_Reg funcs[] = {
                {
                    "AddFace", [](lua_State * L)
                    {
                        Vector3i corners;
                        
                        for (int i = 0; i < 3; ++i) corners.data[i] = LuaXS::Int(L, 2 + i) - 1;

                        Model::Get(L)->getMesh()->vertexIndices.push_back(corners);

                        return 0;
                    }
                }, {
                    "AddNormal", [](lua_State * L)
                    {
                        Model::Get(L)->getMesh()->normals.push_back(MakeVec3(L, 2));

                        return 0;
                    }
                }, {
                    "AddUV", [](lua_State * L)
                    {
                        lua_settop(L, 3); // model, x, y
                        lua_pushnumber(L, 0); // model, x, y, 0

                        Model::Get(L)->getMesh()->texels.push_back(MakeVec3(L, 2));

                        return 0;
                    }
                }, {
                    "AddVertex", [](lua_State * L)
                    {
                        Model::Get(L)->getMesh()->vertices.push_back(MakeVec3(L, 2));

                        return 0;
                    }
                }, {
                    "__gc", LuaXS::TypedGC<Model>
                }, {
                    "GetTexture", [](lua_State * L)
                    {
                        lua_settop(L, 2); // model, name
                        lua_getfenv(L, 1); // model, name, env
                        lua_insert(L, 2); // model, env, name
                        lua_rawget(L, 2); // model, texture?

                        return 1;
                    }
                }, {
                    "SetTransform", [](lua_State * L)
                    {
                        *Model::Get(L)->getModelMatrix() = Matrix4::transformMatrix(*GetXformParams(L, 2));
                        
                        return 0;
                    }
                },
                { nullptr, nullptr }
            };

            luaL_register(L, nullptr, funcs);
        });

        lua_createtable(L, 0, 5); // model, env
        
        std::vector<float> workspace;

        model->mAlbedo = AddTexture(L, "albedo", "RGB", workspace);
        model->mNormal = AddTexture(L, "normal", "XYZ", workspace);
        model->mAmbient = AddTexture(L, "ambient", "BW", workspace);
        model->mRoughness = AddTexture(L, "roughness", "BW", workspace);
        model->mMetallic = AddTexture(L, "metallic", "BW", workspace);

        lua_setfenv(L, -2); // model; model.env = env

        return 1;
    }); // ..., ssge, NewModel
    lua_setfield(L, -2, "NewModel"); // ..., ssge = { ..., NewModel = NewModel }

    //
    //
    //

    lua_pushcfunction(L, [](lua_State * L) {
        LuaXS::NewTyped<TransformParameters>(L); // xform_params
        LuaXS::AttachMethods(L, TRANSFORM_PARAMS_TYPE, [](lua_State * L) {
            luaL_Reg funcs[] = {
                {
                    "SetRotation", [](lua_State * L)
                    {
                        GetXformParams(L, 1)->rotation = MakeVec3(L, 2);

                        return 0;
                    }
                }, {
                    "SetScaling", [](lua_State * L)
                    {
                        GetXformParams(L, 1)->scaling = MakeVec3(L, 2);

                        return 0;
                    }
                }, {
                    "SetTranslation", [](lua_State * L)
                    {
                        GetXformParams(L, 1)->translation = MakeVec3(L, 2);

                        return 0;
                    }
                },
                { nullptr, nullptr }
            };

            luaL_register(L, nullptr, funcs);
        });

        return 1;
    }); // ..., ssge, NewTransformParameters
    lua_setfield(L, -2, "NewTransformParameters"); // ..., ssge = { ..., NewModel, NewTransformParameters = NewTransformParameters }
}
// /STEVE CHANGE