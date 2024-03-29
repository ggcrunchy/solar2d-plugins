// ===============================
// AUTHOR       : Angel Ortiz (angelo12 AT vt DOT edu)
// CREATE DATE  : 2018-07-02
// ===============================

//Headers
#include "engine.h"
// STEVE CHANGE
#include "utils/Blob.h"
// /STEVE CHANGE

//Dummy constructors and destructors
Engine::Engine(){}
Engine::~Engine(){}

//Starts up subsystems in an order that satifies their dependencies.
//If at any point any of the subsystem fails to initialize, the success flag is raised
//and the loading is exited early. This serves to avoid any further fails in the 
//initialization routine that might cause seg faults.
bool Engine::startUp(int w, int h, Uint32 * blob){ // <- STEVE CHANGE
    bool success = true;
    //Start up of all SDL Display related content
    // STEVE CHANGE
    /*if( !gDisplayManager.startUp(w, h) ){
        success = false;
        printf("Failed to initialize window manager.\n");
    }
    else{
        //Initis scene manager and loads default scene
        if( !gSceneManager.startUp() ){
            success = false;
            printf("Failed to initialize scene manager.\n");
        }
        else{
            //Initializes rendererer manager, which is in charge of high level
            //rendering tasks (render queue, locating render scene etc)
            //It gets passed references to the other major subsystems for use later
            //on setup of the render queue.
            */if( !gRenderManager.startUp(w, h, blob) ){ // <- STEVE CHANGE
                success = false;
                printf("Failed to initialize render manager.\n");
            }
            else{
                aspect_ratio = w /(float)h; // <- STEVE CHANGE
                //Initializing input manager that manages all mouse, keyboard and
                //mousewheel input. It needs access to the scene manager to apply the
                //changes on the scene caused by user input. 
                // STEVE CHANGE
                /*if ( !gInputManager.startUp(gSceneManager) ){
                    success = false;
                    printf("Failed to initialize input manager.\n");
                }*/
            }
            // STEVE CHANGE
            /*}
    }*/
    // /STEVE CHANGE
    return success;
}

//Closing in opposite order to avoid dangling pointers
void Engine::shutDown(){
    // STEVE CHANGE
    /*gInputManager.shutDown();
    printf("Closed input manager.\n");*/
    // /STEVE CHANGE

    gRenderManager.shutDown();
    printf("Closed renderer manager.\n");
    // STEVE CHANGE
    /*
    gSceneManager.shutDown();
    printf("Closed Scene manager.\n");
    gDisplayManager.shutDown();
    printf("Closed display manager.\n");*/
    // /STEVE CHANGE
}

//Runs main application loop 
void Engine::run(Scene * currentScene, int deltaT){ // <- STEVE CHANGE
    // STEVE CHANGE
    /*
    //Main flags
    bool done = false;

    //Iteration and time keeping counters
    int count = 0;
    unsigned int deltaT = 0;
    unsigned int start = 0;;
    unsigned int total = 0;

    printf("Entered Main Loop!\n");
    while(!done){
        ++count;
        start = SDL_GetTicks(); //Could probably be its own timer class, but we're keeping things simple here

        //Handle all user input
        //Any changes to the scene are directly sent to the respective objects in
        //the scene class. Also sets exit flag based on user input.
        gInputManager.processInput(done, deltaT);
        */
    // /STEVE CHANGE
        //Update all models, camera and lighting in the current scene
        //Also performs view frustrum culling to determine which objects aare visible

        /*gSceneManager.*/currentScene->update(aspect_ratio, deltaT); // <- STEVE CHANGE

        //Contains the render setup and actual software rendering loop
        gRenderManager.render(currentScene); // <- STEVE CHANGE
    // STEVE CHANGE
    /*
        //Monitoring time taken per frame to gauge engine performance
        deltaT = SDL_GetTicks() - start;
        printf("%2.1d: Frame elapsed time (ms):%d\n",count, deltaT);
        total += deltaT;
    }
    printf("Closing down engine.\n");
    printf("Average frame time over %2.1d frames:%2.fms.\n", count, total/(float)count);*/
    // /STEVE CHANGE
}

// STEVE CHANGE
#define ENGINE_TYPE "sceneView3D.ssge.Engine"

struct EngineWrapper {
    Engine mEngine;
    bool mShutDown{false};
};

//
//
//

static EngineWrapper * GetEngineWrapper (lua_State * L)
{
    return LuaXS::CheckUD<EngineWrapper>(L, 1, ENGINE_TYPE);
}

static Engine * GetEngine (lua_State * L)
{
    EngineWrapper * wrapper = GetEngineWrapper(L);

    luaL_argcheck(L, !wrapper->mShutDown, 1, "Engine has been shut down");

    return &wrapper->mEngine;
}

//
//
//

static void ShutDown (Engine * engine)
{
    engine->shutDown();
    engine->~Engine();
}

//
//
//

int PushVec3 (lua_State * L, const Vector3f & v)
{
    lua_pushnumber(L, v.x); // ..., x
    lua_pushnumber(L, v.y); // ..., x, y
    lua_pushnumber(L, v.z); // ..., x, y, z

    return 3;
}

//
//
//

Vector3f MakeVec3 (lua_State * L, int first)
{
    return Vector3f{LuaXS::Float(L, first), LuaXS::Float(L, first + 1), LuaXS::Float(L, first + 2)};
}

//
//
//

void Engine::add_engine (lua_State * L)
{
    luaL_Reg funcs[] = {
        {
            "NewEngine", [](lua_State * L)
            {
                luaL_checktype(L, 1, LUA_TTABLE);
                lua_getfield(L, 1, "width"); // params, w
                lua_getfield(L, 1, "height"); // params, w, h
                lua_getfield(L, 1, "using_blob"); // params, w, h, using_blob?

                int w = luaL_checkint(L, -3);
                int h = luaL_checkint(L, -2);
                Engine * engine = &LuaXS::NewTyped<EngineWrapper>(L)->mEngine; // params, w, h, using_blob?, engine
                Uint32 * blob = nullptr;

                if (lua_toboolean(L, -2))
                {
                    lua_createtable(L, 0, 1); // params, w, h, true, engine, env

                    BlobXS::NewBlob(L, w * h * sizeof(Uint32)); // params, w, h, true, engine, env, blob?

                    if (lua_isnil(L, -1))
                    {
                        CORONA_LOG_ERROR("Unable to create (%i x %i) blob", w, h);

                        return 1;
                    }

                    blob = reinterpret_cast<Uint32 *>(BlobXS::GetData(L, -1));

                    lua_setfield(L, -2, "blob"); // params, w, h, true, engine, env = { blob = blob }
                    lua_setfenv(L, -2); // params, w, h, true, engine; engine.env = env
                }

                if (engine->startUp(w, h, blob))
                {
                    LuaXS::AttachMethods(L, ENGINE_TYPE, [](lua_State * L) {
                        luaL_Reg funcs[] = {
                            {
                                "__gc", [](lua_State * L)
                                {
                                    EngineWrapper * wrapper = GetEngineWrapper(L);

                                    if (!wrapper->mShutDown) ShutDown(&wrapper->mEngine);

                                    return 0;
                                }
                            }, {
                                "getBlob", [](lua_State * L)
                                {
                                    GetEngine(L); // do checks

                                    lua_getfenv(L, 1); // engine, env
                                    lua_getfield(L, -1, "blob"); // engine, env, blob?

                                    return 1;
                                }
                            }, {
                                "run", [](lua_State * L)
                                {
                                    GetEngine(L)->run(Scene::Get(L, 2), LuaXS::Int(L, 3));

                                    return 0;
                                }
                            }, {
                                "shutDown", [](lua_State * L)
                                {
                                    EngineWrapper * wrapper = GetEngineWrapper(L);

                                    if (!wrapper->mShutDown)
                                    {
                                        ShutDown(&wrapper->mEngine);

                                        wrapper->mShutDown = true;
                                    }

                                    return 0;
                                }
                            },
                            { nullptr, nullptr }
                        };

                        luaL_register(L, nullptr, funcs);
                    });

                    AddToStore(L);
                }

                else
                {
                    ShutDown(engine);

                    lua_pushnil(L); // params, w, h, using_blob?, engine[, blob], nil
                }

                return 1;
            }
        },
        { nullptr, nullptr }
    };

    luaL_register(L, nullptr, funcs);
}
// /STEVE CHANGE