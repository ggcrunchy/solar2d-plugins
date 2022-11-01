#ifndef SCENE_H
#define SCENE_H

// ===============================
// AUTHOR       : Angel Ortiz (angelo12 AT vt DOT edu)
// CREATE DATE  : 2018-07-10
// PURPOSE      : Contains all of the world information. The objects that you want to 
//                render, the camera that represents the viewer and the lights within
//                a scene. It also performs the view frustrum culling check to see which
//                objects should be visible by the camera at any given time and keeps
//                that list updated.
// ===============================
// SPECIAL NOTES: I use vectors here instead of arrays. Which I though would be necessary
// given that I would not know how many items would be loaded. It probably should be 
// changed in any future update to the engine.
// ===============================

//Headers
#include <vector>
#include <queue>
#include "model.h"
#include "camera.h"
#include "light.h"
#include "common.h" // <- STEVE CHANGE

class Scene{
    public:
        //Builds scene using path to folder containing content and txt setup file
        Scene(Camera * camera);//const std::string &sceneFolder); <- STEVE CHANGE
        ~Scene();

        bool Load (const std::string &sceneFolder); // <- STEVE CHANGE
        bool LoadModel (Model * model); // <- STEVE CHANGE

        //Updates all models, lights and cameras
        void update(float aspect_ratio, unsigned int deltaT); // <- STEVE CHANGE

        //Getters used in the setup of the render queue
        std::queue<Model*>* getVisiblemodels();
        Camera * getCurrentCamera();
        BaseLight * getCurrentLights();
        int getLightCount();
        
        //Signals issues to scene Manager
        // bool checkIfEmpty(); <- STEVE CHANGE

        void initializeLights (int count); // <- STEVE CHANGE

        // STEVE CHANGE
        void AddModelToScene (Model * model); // <- STEVE CHANGE

        static void add_scene (lua_State * L);
        static Scene * Get (lua_State * L, int arg = 1);
        // /STEVE CHANGE

    private:
        // bool emptyScene; <- STEVE CHANGE
        Camera & mainCamera; // <- STEVE CHANGE
        int lightCount = 0; // <- STEVE CHANGE
        BaseLight *lights = nullptr; //Array of lights in scene <- STEVE CHANGE

        //Contains the models that remain after frustrum culling
        std::queue<Model*> visibleModels;
        std::vector<Model*> modelsInScene;

        //Loads scene models, checks by looking for the mesh .If it finds it assumes
        // (dangerously) that all the other texture data also exists
        // STEVE CHANGE
        /*bool loadContent(const std::string &baseFilePath, const std::string &sceneName);
        //Check if scene folder acually exists and also checks accessibility 
        bool findSceneFolder(const std::string &scenePath);
        void loadSceneModel(const std::string &baseFilePath, const TransformParameters &init ,const std::string modelMeshID, const std::string modelMaterialID);*/
        // /STEVE CHANGE
        
        //Finds objects that the camera can see
        void frustrumCulling();
};

#endif