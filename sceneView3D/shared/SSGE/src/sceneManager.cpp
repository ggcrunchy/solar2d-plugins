// ===============================
// AUTHOR       : Angel Ortiz (angelo12 AT vt DOT edu)
// CREATE DATE  : 2018-07-10
// ===============================

//Headers
#include "sceneManager.h"

//Dummy constructors / destructors
SceneManager::SceneManager(){}
SceneManager::~SceneManager(){}

//Starts up the scene manager and loads the default scene 
//If for whatever reason the scene could not load any model, or there are none defined
//It just quits early.
bool SceneManager::startUp(float aspect_ratio){ // <- STEVE CHANGE
//    currentSceneID = "teapotSingle"; <- STEVE CHANGE
    aspectRatio = aspect_ratio; // <- STEVE CHANGE
// STEVE CHANGE
    /*if (!loadScene(currentSceneID)){
        printf("Could not load scene. No models succesfully loaded!\n");
        return false;
    }*/
// /STEVE CHANGE
    return true;
}

void SceneManager::shutDown(){
    delete currentScene;
}

//Checks if the scene that you want to load is not the one that is currently loaded.
// If it isn't, then it deletes the current one and loads the new one.
bool SceneManager::switchScene(std::string newSceneID){
    if( newSceneID != currentSceneID ){
        currentSceneID = newSceneID;
        delete currentScene;
        return loadScene(newSceneID);
    }
    else{
        printf("Selected already loaded scene.\n");
        return true;
    }
}

//Misdirection towards the current scene to avoid pointer dangling after scene switching
void SceneManager::update(unsigned int now, unsigned int deltaT){
    currentScene->update(now, deltaT);
}

Scene* SceneManager::getCurrentScene(){
    return currentScene;
}

//Loads the scene with the given ID. If the scene is empty it will declare it an unsuccesful
//load and attempt to quit early of the whole program
bool SceneManager::loadScene(std::string sceneID){
    currentScene = new Scene(sceneID, aspectRatio); // <- STEVE CHANGE
    return  !currentScene->checkIfEmpty(); //True if empty, so it's negated for startup
}