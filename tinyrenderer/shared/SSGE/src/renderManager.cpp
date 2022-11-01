// ===============================
// AUTHOR       : Angel Ortiz (angelo12 AT vt DOT edu)
// CREATE DATE  : 2018-07-02
// ===============================

//Includes
#include "renderManager.h"

//Dummy constructors / Destructors
RenderManager::RenderManager(){}
RenderManager::~RenderManager(){}

//Sets the internal pointers to the screen and the current scene and inits the software
//renderer instance. 
bool RenderManager::startUp(int w, int h,/*DisplayManager &displayManager,SceneManager &sceneManager, */Uint32 * blob){ // <- STEVE CHANGE
//    screen = &displayManager; <- STEVE CHANGE
//    sceneLocator = &sceneManager; <- STEVE CHANGE
    if( !initSoftwareRenderer(w, h, blob) ){ // <- STEVE CHANGE
        printf("Failed to initialize software Renderer!\n");
        return false;
    }
    return true;
}

void RenderManager::shutDown(){
//    sceneLocator = nullptr; <- STEVE CHANGE
    // screen = nullptr; <- STEVE CHANGE
    renderInstance.shutDown();
}

void RenderManager::render(Scene * currentScene){ // <- STEVE CHANGE
    //Reset current buffers
    renderInstance.clearBuffers();

    //Build a render Queue for drawing multiple models
    //Also assigns the current camera to the software renderer
    //And gets info on the number of lights in the scene
    buildRenderQueue(currentScene); // <- STEVE CHANGE

    //Draw all meshes in the render queue. So far we assume they are
    //normal triangular meshes. But it could easily be changed to invoke
    //different methods based on different model types.
    while( !renderObjectQueue->empty() ){
        renderInstance.drawTriangularMesh(renderObjectQueue->front());
        renderObjectQueue->pop();
    }

    //Drawing to the screen by swapping the window's surface with the 
    //final buffer containing all rendering information
    // screen->swapBuffers(renderInstance.getRenderTarget()); <- STEVE CHANGE

    //Set camera pointer to null just in case a scene change occurs
    renderInstance.setCameraToRenderFrom(nullptr);
}

//Gets the list of visible models from the current scene
//Done every frame in case scene changes
void RenderManager::buildRenderQueue(Scene * currentScene){ // <- STEVE CHANGE

    //set scene info
    // Scene* currentScene = sceneLocator->getCurrentScene(); <- STEVE CHANGE
    
    //Set renderer camera
    renderInstance.setCameraToRenderFrom(currentScene->getCurrentCamera());

    //Update the pointer to the list of lights in the scene
    renderInstance.setSceneLights(currentScene->getCurrentLights(), currentScene->getLightCount() );

    //Get pointers to the visible model queue
    renderObjectQueue = currentScene->getVisiblemodels();
}

bool RenderManager::initSoftwareRenderer(int w, int h, Uint32 * blob){ // <- STEVE CHANGE
    // int w = DisplayManager::SCREEN_WIDTH; // <-STEVE CHANGE
    // int h = DisplayManager::SCREEN_HEIGHT; // <- STEVE CHANGE
    return renderInstance.startUp(w, h, blob); // <- STEVE CHANGE
}










