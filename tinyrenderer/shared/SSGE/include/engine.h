#ifndef ENGINE_H
#define ENGINE_H

// ===============================
// AUTHOR       : Angel Ortiz (angelo12 AT vt DOT edu)
// CREATE DATE  : 2018-07-02
// PURPOSE      : Application class containing all high level logic and init / shutdown
//                routines for each major subsystem. The purpose of this program is to
//                to build a functioning graphics engine without using libraries
//                such as OpenGL or DirectX.
// ===============================
// SPECIAL NOTES: Built for educational purposes only.
// ===============================

//Headers
// #include "displayManager.h" <- STEVE CHANGE
#include "renderManager.h"
// #include "inputManager.h" <- STEVE CHANGE
// #include "sceneManager.h" <- STEVE CHANGE
#include "scene.h" // <- STEVE CHANGE
#include "common.h" // <- STEVE CHANGE

#define printf CoronaLog

//Very basic graphics engine application. 
//In charge of initializing and closing down all manager-level classes in a safe way.
class Engine
{
  public:
    //Dummy constructors / Destructors
    Engine();
    ~Engine();

    //I use these methods instead of constructors and destructors
    //because I want to be able to control initialization order.
    //You'll see the same idea applied to all manager level classes.
    bool startUp(int w, int h, Uint32 * blob); // <- STEVE CHANGE
    void shutDown();

    //Contains all high level logic and the main application loop
    void run(Scene * currentScene, int deltaT); // <- STEVE CHANGE

    // STEVE CHANGE
    static void add_engine (lua_State * L);
    // /STEVE CHANGE

//  private: <- STEVE CHANGE
//    DisplayManager gDisplayManager; <- STEVE CHANGE
    RenderManager gRenderManager;
//    InputManager gInputManager; <- STEVE CHANGE
//    SceneManager gSceneManager; <- STEVE CHANGE
    float aspect_ratio; // <- STEVE CHANGE
};

#endif