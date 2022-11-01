// ===============================
// AUTHOR       : Angel Ortiz (angelo12 AT vt DOT edu)
// CREATE DATE  : 2018-07-10
// ===============================

//Headers
#include "scene.h"
// #include "objParser.h" // <- STEVE CHANGE
#include <fstream>
#include <sstream>
#include <sys/stat.h>
#include <sys/types.h>

Scene::Scene(Camera * camera/*const std::string &sceneName*/) : mainCamera(*camera){ // <- STEVE CHANGE
    // STEVE CHANGE moved to Load()
}

Scene::~Scene(){
    //Making sure you don't attempt to delete models that don't exist
    // STEVE CHANGE
    /*if (!emptyScene){
        for(Model *models : modelsInScene){
            delete models;
        }*/
    // /STEVE CHANGE
        delete [] lights;
    //} <- STEVE CHANGE
}

// STEVE CHANGE
bool Scene::Load (const std::string &sceneName)
{
    //Building all the useful path strings
    // STEVE CHANGE
    // std::string folderPath = "../scenes/" + sceneName;
    size_t pos = sceneName.find_last_of("/\\");

    if (sceneName.npos == pos) return false; // <- STEVE CHANGE
    /*
    std::string folderPath = sceneName;

    if( !findSceneFolder(folderPath)){
        //If you do not find the scene folder quit
        return false;
    }
    else{
        //Load all cameras, models and lights and return false if it fails
        return !loadContent(folderPath, sceneName.substr(pos + 1)); // <- STEVE CHANGE
    }*/
    return true;
}
// /STEVE CHANGE

//Update Order is critical for correct culling
void Scene::update(float aspect_ratio, unsigned int deltaT){ // <- STEVE CHANGE
    mainCamera.update(aspect_ratio, deltaT);
    // STEVE CHANGE
    /*
    for(int i=0; i < lightCount; ++i){
        lights[i].update(deltaT);
    }*/
    // /STEVE CHANGE
    for(Model *model : modelsInScene){
        model->update();
    }
    frustrumCulling();
}
//-----------------------------GETTERS----------------------------------------------
std::queue<Model*>* Scene::getVisiblemodels(){
    return &visibleModels;
}
Camera* Scene::getCurrentCamera(){
    return &mainCamera;
}
BaseLight * Scene::getCurrentLights(){
    return lights;
}
int Scene::getLightCount(){
    return lightCount;
}
//----------------------------------------------------------------
// STEVE CHANGE
/*bool Scene::checkIfEmpty(){
    return emptyScene;
}*/

void Scene::initializeLights (int count)
{
    delete [] lights;

    lightCount = count;

    if (count > 0) lights = new BaseLight[count];
    else lights = nullptr;
}
// /STEVE CHANGE

//-----------------------------SCENE LOADING-----------------------------------

//TODO: separate into new class or function
//Config file parsing, gets all the important 
// STEVE CHANGE
/*bool Scene::loadContent(const std::string &baseFilePath, const std::string &sceneName ){
    std::string configFilePath = baseFilePath + "/"+ sceneName + "_config.txt";
    std::ifstream file(configFilePath.c_str());
    TransformParameters initParameters;

    //Begin config file parsing
    if(!file.good()){
        //Check config file exists
        printf("Error! Config: %s does not exist.\n",configFilePath.c_str());
        return false;
    }
    else{
        //Checking that config file belongs to current scene and is properly formatted
        std::string line, key, x, y, z;
        std::getline(file,line);
        std::istringstream iss(line);
        iss >> key;
        if(key != "s"){
            printf("Error! Config file: %s is not properly formatted.\n",configFilePath.c_str());
            return false;
        }
        else{
            iss >> key;
            if(key != sceneName){
                //Checks the config file belongs to the correct scene
                printf("Error! Config file: %s does not belong to current scene.\n",configFilePath.c_str());
                return false;
            }
            else{
                //Now we can parse the rest of the file "safely"
                while(!file.eof()){
                    std::getline(file,line);
                    std::istringstream iss(line);
                    iss >> key;
                    //MODEL SETUP
                    if(key == "m"){ 
                        printf("Loading models...\n");
                        iss >> key;
                        int modelCount = stoi(key);
                        std::string modelMeshID, modelMaterialID;
                        for(int i = 0; i < modelCount; ++i){

                            //Get model mesh and material info
                            std::getline(file,line);
                            std::istringstream modelData(line);
                            modelData >> key >> modelMeshID >> modelMaterialID;

                            //Position
                            std::getline(file,line);
                            std::istringstream pos(line);
                            pos >> key >> x >> y >> z;
                            initParameters.translation = Vector3f(stof(x), stof(y), stof(z));

                            //Rotation
                            std::getline(file,line);
                            std::istringstream rot(line);
                            rot >> key >> x >> y >> z;
                            initParameters.rotation = Vector3f(stof(x)*M_PI/180.0f, stof(y)*M_PI/180.0f, stof(z)*M_PI/180.0f);

                            //Scaling
                            std::getline(file,line);
                            std::istringstream sca(line);
                            sca >> key >> x >> y >> z;
                            initParameters.scaling = Vector3f(stof(x), stof(y), stof(z));

                            //Burning empty line that makes the config easier to read
                            std::getline(file,line);
                            
                            //Attempts to load model with the initparameters it has read
                            loadSceneModel(baseFilePath, initParameters, modelMeshID, modelMaterialID);
                        }
                    }
                    //LIGHT SETUP
                    else if(key == "l"){ 
                        printf("Loading lights...\n");
                        iss >> key;
                        int lightCcount = stoi(key);
                        //Initializes light array
                        std::string lightType, radius, period;
                        lights = new BaseLight[lightCount];
                        for(int i = 0; i < lightCount; ++i){

                            //Obtain light type and depending on that get orbit or linear
                            std::getline(file,line);
                            std::istringstream lightData(line);
                            lightData >> key >> lightType;
                            if(lightType == "o"){
                                lights[i].type = 'o';
                                std::getline(file,line);
                                std::istringstream orb(line);
                                orb >> key >> radius >> period;
                                lights[i].radius = stof(radius);
                                lights[i].time /= stof(period)*1000; //miliseconds
                            }
                            else if(lightType == "l"){
                                // lights[i].type = 'l'; <- STEVE CHANGE
                                std::getline(file,line);
                                std::istringstream orb(line);
                                orb >> key >> radius >> period;
                                /*lights[i].radius = stof(radius);
                                lights[i].time /= stof(period)*1000;
                            }
                            // STEVE CHANGE
                            else if(lightType == "c"){
                                lights[i].type = 'c';
                            }
                            else if(lightType == "f"){
                                lights[i].type = 'f';
                            }

                            //Position
                            std::getline(file,line);
                            std::istringstream pos(line);
                            pos >> key >> x >> y >> z;
                            lights[i].position = Vector3f(stof(x), stof(y), stof(z));

                            //Color
                            std::getline(file,line);
                            std::istringstream col(line);
                            col >> key >> x >> y >> z;
                            lights[i].color = Vector3f(stof(x), stof(y), stof(z));

                            //Burning empty line that makes the config easier to read
                            std::getline(file,line);
                        }
                    }
                }
                //Lastly we check if the scene is empty and return 
                return !modelsInScene.empty();       
            }
        }
    }
}

bool Scene::findSceneFolder(const std::string &scenePath){
    struct stat info;
    //folder is blocking access
    if( stat( scenePath.c_str(), &info ) != 0 ){
        printf( "cannot access %s\n", scenePath.c_str() );
         return false;
    }
    else if( info.st_mode & S_IFDIR ){
        //Folder is accessible
        printf( "%s is a valid scene\n", scenePath.c_str() );
        return true;
    }
    else{
        //Folder does not exist
        printf("Error! Scene: %s does not exist.\n",scenePath.c_str());
        return false;
    }
}

void Scene::loadSceneModel(const std::string &baseFilePath, const TransformParameters &init,const std::string modelMeshID, const std::string modelMaterialID){
    std::string meshFilePath = baseFilePath + "/meshes/" + modelMeshID + "_mesh.obj";
    if(!OBJ::fileExists(meshFilePath)){
        //If the mesh deos not exist it's very likely nothing else does, quit early
        printf("Error! Mesh: %s does not exist.\n",meshFilePath.c_str());
    }
    else{
        printf( "%s is a valid mesh\n", meshFilePath.c_str() );
        std::string materialPath = baseFilePath + "/materials/" + modelMaterialID + "/"  + modelMaterialID;
    DWORD t1=timeGetTime();
        modelsInScene.push_back(new Model(init, meshFilePath, materialPath));DWORD t2=timeGetTime();
        printf("LOAD SCENE MODEL %s = %u\n", modelMaterialID.c_str(),t2-t1);
    }
}*/

void Scene::AddModelToScene (Model * model)
{
    model->Build();
    modelsInScene.push_back(model);
}
// /STEVE CHANGE

//-------------------------------------------------------------

void Scene::frustrumCulling(){
    for(Model *model : modelsInScene){
        bool visible = mainCamera.checkVisibility(model->getBounds());
        if (visible) {
            visibleModels.push(model);
        }
    }
}

// STEVE CHANGE
#define SCENE_TYPE "sceneView3D.ssge.Scene"

//
//
//

struct SceneWrapper {
    SceneWrapper (Camera * camera) : mScene(camera)
    {
    }

    Scene mScene;
    int mCameraRef;
};

//
//
//

static SceneWrapper * GetSceneWrapper (lua_State * L, int arg)
{
    return LuaXS::CheckUD<SceneWrapper>(L, arg, SCENE_TYPE);
}

Scene * Scene::Get (lua_State * L, int arg)
{
    SceneWrapper * wrapper = GetSceneWrapper(L, arg);

    luaL_argcheck(L, wrapper->mCameraRef != LUA_NOREF, 1, "Scene has been destroyed");

    return &wrapper->mScene;
}

//
//
//

void Scene::add_scene (lua_State * L)
{
    lua_pushcfunction(L, [](lua_State * L) {
        Camera * camera = Camera::Get(L, 1);
        SceneWrapper * wrapper = LuaXS::NewTyped<SceneWrapper>(L, camera); // camera, scene

        LuaXS::AttachMethods(L, SCENE_TYPE, [](lua_State * L) {
            luaL_Reg funcs[] = { 
                {
                    "addModel", [](lua_State * L)
                    {
                        Model * model = Model::Get(L, 2);
                        bool ok = false;

                        if (!model->IsReady()) printf("Model is not ready to be added\n");

                        else
                        {
                            lua_getfenv(L, 1); // scene, model, env
                            lua_pushvalue(L, 2); // scene, model, env, model
                            lua_pushvalue(L, 2); // scene, model, env, model, model
                            lua_rawget(L, -3); // scene, model, env, model, exists

                            if (lua_isnil(L, -1))
                            {
                                lua_pop(L, 1); // scene, model, env, model
                                lua_pushboolean(L, 1); // scene, model, env, model, true
                                lua_rawset(L, -3); // scene, model, env = { ..., [model] = true }

                                Scene::Get(L)->AddModelToScene(model);

                                ok = true;
                            }

                            else printf("Model already added to scene\n");
                        }

                        lua_pushboolean(L, ok); // scene, model[, env], ok

                        return 1;
                    }
                }, {
                    "destroy", [](lua_State * L)
                    {
                        SceneWrapper * wrapper = GetSceneWrapper(L, 1);

                        if (wrapper->mCameraRef != LUA_NOREF)
                        {
                            wrapper->mScene.~Scene();

                            lua_unref(L, wrapper->mCameraRef);

                            wrapper->mCameraRef = LUA_NOREF;
                        }

                        return 0;
                    }
                }, {
                    "__gc", [](lua_State * L)
                    {
                        SceneWrapper * wrapper = GetSceneWrapper(L, 1);

                        if (wrapper->mCameraRef != LUA_NOREF) wrapper->mScene.~Scene();

                        return 0;
                    }
                }, {
                    "getLightColor", [](lua_State * L)
                    {
                        Scene * scene = Scene::Get(L);
                        int index = luaL_checkint(L, 2) - 1;

                        if (index >= 0 && index < scene->getLightCount()) return PushVec3(L, scene->getCurrentLights()[index].color); // engine, index, r, g, b

                        lua_pushnil(L); // scene, index, nil

                        return 1;
                    }
                }, {
                    "getLightPosition", [](lua_State * L)
                    {
                        Scene * scene = Scene::Get(L);
                        int index = luaL_checkint(L, 2) - 1;

                        if (index >= 0 && index < scene->getLightCount()) return PushVec3(L, scene->getCurrentLights()[index].position); // engine, index, x, y, z

                        lua_pushnil(L); // scene, index, nil

                        return 1;
                    }
                }, {
                    "initializeLights", [](lua_State * L)
                    {
                        Scene::Get(L)->initializeLights(luaL_checkint(L, 2));

                        return 0;
                    }
                }, {
                    "load", [](lua_State * L)
                    {
                        lua_pushboolean(L, Scene::Get(L)->Load(luaL_checkstring(L, 2))); // scene, name, ok

                        return 1;
                    }
                }, {
                    "setLightColor", [](lua_State * L)
                    {
                        Scene * scene = Scene::Get(L);
                        int index = luaL_checkint(L, 2) - 1;

                        if (index >= 0 && index < scene->getLightCount()) scene->getCurrentLights()[index].color = MakeVec3(L, 3);

                        return 0;
                    }
                }, {
                    "setLightPosition", [](lua_State * L)
                    {
                        Scene * scene = Scene::Get(L);
                        int index = luaL_checkint(L, 2) - 1;

                        if (index >= 0 && index < scene->getLightCount()) scene->getCurrentLights()[index].position = MakeVec3(L, 3);

                        return 0;
                    }
                },
                { nullptr, nullptr }
            };

            luaL_register(L, nullptr, funcs);
        });

        lua_newtable(L); // camera, scene, env
        lua_setfenv(L, -2); // camera, scene; scene.env = env
        lua_pushvalue(L, 1); // camera, scene, camera

        wrapper->mCameraRef = lua_ref(L, 1); // camera, scene; scene.camera_ref = camera

        AddToStore(L);

        return 1;
    }); // ..., ssge, NewScene
    lua_setfield(L, -2, "NewScene"); // ..., ssge = { ..., NewScene = NewScene }
}
// /STEVE CHANGE










