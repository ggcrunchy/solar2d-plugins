#ifndef MODEL_H
#define MODEL_H

// ===============================
// AUTHOR       : Angel Ortiz (angelo12 AT vt DOT edu)
// CREATE DATE  : 2018-07-03
// PURPOSE      : Container for all of the data realted to a model such as texture data,
//                mesh data and even a model matrix for transformations.
//                It has an update method that could be called based on physics updates
//                but is not much more than a stub.
// ===============================
// SPECIAL NOTES: Should probably be rewritten to be a struct instead of having to use
// all of these useless getters. This class is little more than a glorified container.
// ===============================

//Headers
#include <string>
#include "mesh.h"
#include "geometry.h"
#include "matrix.h"
#include "texture.h"
// #include "objParser.h"// <- STEVE CHANGE
#include "common.h" // <- STEVE CHANGE

class Model{
    public:
        //On model creation all textures are loaded, the mesh is built and even an
        //AABB is built.
        // STEVE CHANGE
        /*Model( const TransformParameters &initParameters, const std::string meshPath, const std::string materialPath) : 
            mAlbedo(materialPath + "_albedo.png", "RGB"),
            mNormal(materialPath + "_normal.png", "XYZ"),
            mAmbient(materialPath + "_ao.png", "BW"),
            mRoughness(materialPath + "_rough.png", "BW"),
            mMetallic(materialPath + "_metal.png", "BW"),
            mModelMatrix(Matrix4::transformMatrix(initParameters))
        {   
            DWORD t1=timeGetTime();
            OBJ::buildMeshFromFile(mMesh, meshPath);DWORD t2=timeGetTime();
            mBounds.buildAABB(mMesh);DWORD t3=timeGetTime();
            mMesh.buildFacetNormals();DWORD t4=timeGetTime();
            mMesh.buildTangentSpace();DWORD t5=timeGetTime();
            printf("MODEL built mesh = %u, built AABB=%u, facet normals = %u, tangent space=%u\n", t2-t1,t3-t2,t4-t3,t5-t4);
        };*/
        // /STEVE CHANGE

        //TODO: too many getters, unify into one method?
        Mesh *getMesh(); // <- STEVE CHANGE
        const Mesh *getMesh() const; // <- STEVE CHANGE
        Matrix4 *getModelMatrix();
        const Matrix4 *getModelMatrix() const; // <- STEVE CHANGE
        AABox *getBounds();
        const Texture *getAlbedo() const; // <- STEVE CHANGE
        const Texture *getNormal() const; // <- STEVE CHANGE
        const Texture *getAO() const; // <- STEVE CHANGE
        const Texture *getRoughness() const; // <- STEVE CHANGE
        const Texture *getMetallic() const; // <- STEVE CHANGE


        void update();

        //Prints the mesh vertices for debugging
        void describeMesh();

        // STEVE CHANGE
        void Build ();
        bool IsReady () const;

        static void add_model (lua_State * L);
        static Model * Get (lua_State * L, int arg = 1);
        // /STEVE CHANGE
    private:
        Texture * mAlbedo; // <- STEVE CHANGE
        Texture * mNormal; // <- STEVE CHANGE
        Texture * mAmbient; // <- STEVE CHANGE
        Texture * mRoughness; // <- STEVE CHANGE
        Texture * mMetallic; // <- STEVE CHANGE

        Mesh mMesh;
        AABox mBounds;
        Matrix4 mModelMatrix;
};

#endif