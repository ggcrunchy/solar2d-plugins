// ===============================
// AUTHOR       : Angel Ortiz (angelo12 AT vt DOT edu)
// CREATE DATE  : 2018-07-10
// ===============================

//Headers
#include "camera.h"

Camera::Camera(){
    side = front.crossProduct(up).normalized();
    viewMatrix = Matrix4::lookAt(position, target, up);
// STEVE CHANGE
    /*projectionMatrix = Matrix4::projectionMatrix(cameraFrustrum.fov, cameraFrustrum.AR, cameraFrustrum.near, cameraFrustrum.far);
    cameraFrustrum.setCamInternals();
    cameraFrustrum.updatePlanes(viewMatrix, position);*/
// /STEVE CHANGE
}

//Updates target and position based on camera movement mode.
///Also updates view matrix and projection matrix for rendering
void Camera::update(float aspect_ratio, unsigned int deltaT){ // <- STEVE CHANGE
    // STEVE CHANGE
    /*if(orbiting){
        float ang    = 2 * M_PI * static_cast<float>(SDL_GetTicks()) / (period*1000);
        float camX   = std::sin(ang) * radius; 
        float camZ   = std::cos(ang) * radius;
        position.x   = camX;
        position.y   = camX;
        position.z   = camZ;
    }
    else{
        target = position + front;
    }*/
    // /STEVE CHANGE
    viewMatrix = Matrix4::lookAt(position, target, up);
    cameraFrustrum.updatePlanes(viewMatrix, position, aspect_ratio); // <- STEVE CHANGE
    projectionMatrix = Matrix4::projectionMatrix(cameraFrustrum.fov, aspect_ratio/*cameraFrustrum.AR*/, cameraFrustrum.near, cameraFrustrum.far); // <- STEVE CHANGE
}

//View frustrum culling using a models AAB
bool Camera::checkVisibility(AABox *bounds){
    return cameraFrustrum.checkIfInside(bounds);
}

//Used by input to reset camera to origin in case user loses their bearings
void Camera::resetCamera(){
    position = Vector3f(0, 0, 8.0);
    target.zero();  
    front    = Vector3f(0, 0, -1);
    side     = front.crossProduct(up);
    // STEVE CHANGE
    /*radius =   2;
    pitch    =   0;
    yaw      = -90;
    period   =  30;*/
    // /STEVE CHANGE
}

// STEVE CHANGE
#define CAMERA_TYPE "sceneView3D.ssge.Camera"

//
//
//

Camera * Camera::Get (lua_State * L, int arg)
{
    return LuaXS::CheckUD<Camera>(L, arg, CAMERA_TYPE);
}

//
//
//

void Camera::add_camera (lua_State * L)
{
    lua_pushcfunction(L, [](lua_State * L) {
        LuaXS::NewTyped<Camera>(L); // camera
        LuaXS::AttachMethods(L, CAMERA_TYPE, [](lua_State * L) {
            luaL_Reg funcs[] = {
                {
                    "getFOV", [](lua_State * L)
                    {
                        lua_pushnumber(L, Camera::Get(L)->cameraFrustrum.fov); // engine, fov

                        return 1;
                    }
                }, {
                    "getFront", [](lua_State * L)
                    {
                        return PushVec3(L, Camera::Get(L)->front); // engine, x, y, z
                    }
                }, {
                    "getPosition", [](lua_State * L)
                    {
                        return PushVec3(L, Camera::Get(L)->position); // engine, x, y, z
                    }
                }, {
                    "getSide", [](lua_State * L)
                    {
                        return PushVec3(L, Camera::Get(L)->side); // engine, x, y, z
                    }
                }, {
                    "getTarget", [](lua_State * L)
                    {
                        return PushVec3(L, Camera::Get(L)->target); // engine, x, y, z
                    }
                }, {
                    "getUp", [](lua_State * L)
                    {
                        return PushVec3(L, Camera::Get(L)->up); // engine, x, y, z
                    }
                }, {
                    "reset", [](lua_State * L)
                    {
                        Camera::Get(L)->resetCamera();

                        return 0;
                    }
                }, {
                    "setFOV", [](lua_State * L)
                    {
                        Camera::Get(L)->cameraFrustrum.fov = LuaXS::Float(L, 2);

                        return 0;
                    }
                }, {
                    "setFront", [](lua_State * L)
                    {
                        Camera * camera = Camera::Get(L);

                        camera->front = MakeVec3(L, 2);

                        camera->front = camera->front.normalized();
                        camera->side = camera->front.crossProduct(camera->up);

                        return 0;
                    }
                }, {
                    "setPosition", [](lua_State * L)
                    {
                        Camera::Get(L)->position = MakeVec3(L, 2);

                        return 0;
                    }
                }, {
                    "setTarget", [](lua_State * L)
                    {
                        Camera::Get(L)->target = MakeVec3(L, 2);

                        return 0;
                    }
                },
                { nullptr, nullptr }
            };

            luaL_register(L, nullptr, funcs);
        });

        return 1;
    }); // ..., ssge, NewCamera
    lua_setfield(L, -2, "NewCamera"); // ..., ssge = { ..., NewCamera = NewCamera }
}
// /STEVE CHANGE