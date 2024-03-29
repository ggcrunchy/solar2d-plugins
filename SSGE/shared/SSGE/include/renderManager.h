#ifndef RENDERMANAGER_H
#define RENDERMANAGER_H

// ===============================
// AUTHOR       : Angel Ortiz (angelo12 AT vt DOT edu)
// CREATE DATE  : 2018-07-02
// PURPOSE      : To perform all the high level operations that the actual rendering 
//                class shouldn't really be concerned with such as: building a render
//                queue, finding the scene you want to render, and locating the camera.
//                It also performs the calls to the other manager level classes.
// ===============================
// SPECIAL NOTES: I think there is much more that could be offloaded to this class 
// which is currently being done by the software renderer. For example, the creation of
// the shader object and the setting of data for each model could easily be prepped earlier
// And saved into something like a VBO. Alas, I had no idea what I was doing when I wrote this
// way back and it's time to move on to bigger and better things anyway.
// ===============================

//Includes
// #include "displayManager.h" <- STEVE CHANGE
// #include "sceneManager.h" <- STEVE CHANGE
#include "softwareRenderer.h"
#include "model.h"
#include "scene.h" // <- STEVE CHANGE
#include <queue>

//High level render operations that shouldn't be done by the
//basic graphics lib.
class RenderManager{

    public:
        //Dummy constructors / Destructors
        RenderManager();
        ~RenderManager();

        //Gets scene and display info. Will be used to build render Queue
        bool startUp(int w, int h,/*DisplayManager &displayManager,SceneManager &sceneManager,  */Uint32 * blob); // <- STEVE CHANGE
        void shutDown();

        //Performs all high level prep operations that the graphics library
        //Needs to do before beginning to draw each model in the scene.
        void render(Scene * current); // <- STEVE CHANGE

    private:
        void buildRenderQueue(Scene * scene); // <- STEVE CHANGE
        bool initSoftwareRenderer(int w, int h, Uint32 * blob); // <- STEVE CHANGE
        
        //This is a pointer to a pointer to allow for scene switching
        // SceneManager   * sceneLocator; <- STEVE CHANGE
        // DisplayManager * screen; <- STEVE CHANGE

        SoftwareRenderer renderInstance;
        std::queue<Model*> *renderObjectQueue;        
};




#endif