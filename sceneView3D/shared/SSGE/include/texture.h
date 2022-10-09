#ifndef TEXTURE_H
#define TEXTURE_H

// ===============================
// AUTHOR       : Angel Ortiz (angelo12 AT vt DOT edu)
// CREATE DATE  : 2018-07-02
// PURPOSE      : To store texture data for retrieval during the pixel shader phase.
//                it also has to be capable of storing all the major kinds of textures
//                that are used in a physically based renderer.
// ===============================
// SPECIAL NOTES: The two separate methods to get pixel values are not good style,
// and should probably be changed. The reason they're there is because of the different
// return statement for each. Should probably become either different classes or just
// a more general solution.
// ===============================

//Headers
#include <string>
#include <vector>
#include "vector3D.h"
#include "common.h" // <- STEVE CHANGE

class Texture{
    public:
        enum TextureType { kRGB, kXYZ, kBW }; // <- STEVE CHANGE

        Texture(/*std::string path, std::string*/ TextureType type); // <- STEVE CHANGE
        ~Texture();

        Vector3f getPixelVal(float u, float v) const; // <- STEVE CHANGE
        float getIntensityVal(float u, float v) const; // <- STEVE CHANGE

        // STEVE CHANGE
        void Load (const unsigned char * data, int w, int h, int channels);
        bool HasData (void) const { return !pixelData.empty(); }
        int ChannelCount (void) const { return kBW != textureType ? 3 : 1; }

        static void add_texture (lua_State * L);
        static Texture * Get (lua_State * L, int arg = 1);

        static Texture * PushNew (lua_State * L, const char * type, std::vector<float> * workspace);
        // /STEVE CHANGE

    private:
        std::vector<float> pixelData; // <- STEVE CHANGE
        int width, height, widthInTiles; // <- STEVE CHANGE
        short channels, textureType; // <- STEVE CHANGE
        static const int tileW = 32, tileH = 32; // <- STEVE CHANGE

        //Currently disabled after tiling has been implemented
        int bilinearFiltering(float u, float v);

        //Reorganizes pixel data into a more cache friendly form
        void tileData();
};

#endif