// ===============================
// AUTHOR       : Angel Ortiz (angelo12 AT vt DOT edu)
// CREATE DATE  : 2018-07-02
// ===============================

//Headers
#include "texture.h"
#include "rasterizer.h" // <- STEVE CHANGE
#include "ByteReader.h" // <- STEVE CHANGE
// STEVE CHANGE
/*#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"*/
// /STEVE CHANGE
#include <cmath>

//Depending on the type of texture the data that is loaded must be 
//transformed in different ways. 
Texture::Texture(/*std::string path, std::string*/ TextureType type) : textureType(type) { // <- STEVE CHANGE
    // STEVE CHANGE
    /*stbi_set_flip_vertically_on_load(true);
    unsigned char * data = stbi_load(path.c_str(), &width, &height, &channels, 0);
    widthInTiles = (width + tileW -1) / tileW;
    pixelData = new float[width*height*channels];
    if (data){
        if(type == "RGB"){ //Rgb data requires a gamma correction, float conversion
            // moved into Load()
        }
        else if (type == "XYZ"){//conversion to float and rescaling to -1 1 bounds
            // moved into Load()
        }
        else if (type == "BW"){ //Simple conversion to float
            // moved into Load()
        }
        else{
            printf("Error unrecognized texture format type.\n");
        }
    // STEVE CHANGE
    /*}
    else{
        printf("Failed to load texture at: %s\n",path.c_str());
    }
    stbi_image_free(data);*/
    // /STEVE CHANGE
}

//Organizes the texture images in tiles to improve cache coherency
//and reduce the amount of cache misses that getting pixel values would cause
void Texture::tileData(){
    // STEVE CHANGE
    size_t line_size = width * channels, tile_line_size = tileW * channels, tile_size = tileH * tile_line_size;
    std::vector<float> workspace(tileH * line_size);
    /*float *tiledPixelData = new float[width*height*channels];
    /*int tileNumW    = width / tileW;
    int tileNumH    = height / tileH;
    /*int linearIndex = 0;
    int tiledIndex  = 0;*/
    float * upper = pixelData.data(), * lower = upper + pixelData.size() - line_size;
    // /STEVE CHANGE

    // TODO: account for tileH not dividing height
    for(int tileRow = 0; tileRow < height/*tileNumH*/; /*++*/tileRow += tileH){ // <- STEVE CHANGE (dimensions were swapped)
        // STEVE CHANGE
        float * tile_upper_left = upper;

        // The image must be flipped, so for half of it, starting from the bottom, move rows
        // into the workspace, and replace them with the corresponding top rows. The top row
        // memory will be overwritten by the tiling.
        if (upper < lower)
        {
            float * wdata = workspace.data();

            for (int ty = 0; ty < tileH; ++ty, wdata += line_size, upper += line_size, lower -= line_size)
            {
                memcpy(wdata, lower, line_size * sizeof(float));
                memcpy(lower, upper, line_size * sizeof(float));
            }
        }

        // By the second half, the rows are in place: blast them right into the workspace.
        else
        {
            memcpy(workspace.data(), upper, workspace.size() * sizeof(float));

            upper += workspace.size();
        }

        const float * ws_upper_left = workspace.data();
        // /STEVE CHANGE

        for(int tileCol = 0; tileCol < width/*tileNumW*/; /*++*/tileCol += tileW, tile_upper_left += tile_size, ws_upper_left += tile_line_size){ // <- STEVE CHANGE
            float * tile_row = tile_upper_left; // <- STEVE CHANGE
            const float * ws_row = ws_upper_left; // <- STEVE CHANGE

            for(int tilePixelHeight = 0; tilePixelHeight < tileH; ++tilePixelHeight, tile_row += tile_line_size, ws_row += line_size){

                //1.First multiplication accounts for a change in pixel height within a tile
                //2.Second accounts for a change of tile along arrow(tile movement column wise)
                //3.Third accounts for the movement of one whole tile row downwards
                
                // STEVE CHANGE
                // TODO: account for non-multiple-of-tileW final element
                memcpy(tile_row, ws_row, tile_line_size * sizeof(float));

                /*linearIndex = (tilePixelHeight*width + tileCol*tileW + tileRow*width*tileH)*channels;

                for(int tilePixelWidth = 0; tilePixelWidth < tileW; ++tilePixelWidth){
                    
                    //Pixel wise movement is equal to channelwise movement in the array

                    for(int pC = 0; pC < channels; ++pC){
                        tiledPixelData[tiledIndex] = pixelData[linearIndex];
                        ++linearIndex;
                        ++tiledIndex;
                        
                    }
                }*/
                // /STEVE CHANGE
            }
        }
    }
    // STEVE CHANGE
    /*delete [] pixelData;
    pixelData = tiledPixelData;*/
    // /STEVE CHANGE
}



Texture::~Texture(){
//    delete [] pixelData; <- STEVE CHANGE
}

//Tiling has invalidated my bilinear filtering code but it is here for posterity
Vector3f Texture::getPixelVal(float u, float v) const { // <- STEVE CHANGE

    //Simple bilinear filtering
    // float intU;
    // float tU = std::modf(u * (width-1), &intU);
    // int uIntLo = (int)intU; 
    // int uIntHi = std::ceil(u * (width-1)); 

    // float intV;
    // float tV = std::modf(v * (height-1), &intV);
    // int vIntLo = (int)intV; 
    // int vIntHi = std::ceil(v * (height-1));

    // int index00 = (vIntLo*width + uIntLo)*channels;
    // int index01 = (vIntLo*width + uIntHi)*channels;
    // int index02 = (vIntHi*width + uIntLo)*channels;
    // int index03 = (vIntHi*width + uIntHi)*channels;

    // float red   = (pixelData[index00]*(1 - tU) + pixelData[index01]*tU)*(1-tV) + (pixelData[index02]*(1-tU) + pixelData[index03]*tU)*tV;
    // float green = (pixelData[index00 + 1]*(1 - tU) + pixelData[index01 + 1]*tU)*(1-tV) + (pixelData[index02 + 1]*(1-tU) + pixelData[index03 + 1]*tU)*tV;
    // float blue  = (pixelData[index00 + 2]*(1 - tU) + pixelData[index01 + 2]*tU)*(1-tV) + (pixelData[index02 + 2]*(1-tU) + pixelData[index03 + 2]*tU)*tV;

    // return Vector3f{red, green, blue};

    // int uInt = u * (width-1); 
    // int vInt = v * (height-1);
    // int index = (vInt*width + uInt)*channels;
    // return Vector3f{pixelData[index], pixelData[index+1], pixelData[index+2]};

    int uInt = u * (width-1); 
    int vInt = v * (height-1);

    int tileX = uInt / tileW;
    int tileY = vInt / tileH;

    int inTileX = uInt - tileX * tileW; //uInt % tileW; <- STEVE CHANGE
    int inTileY = vInt - tileY * tileH; //vInt % tileH; <- STEVE CHANGE

    int index = ((tileY * widthInTiles + tileX) * (tileW * tileH)
                + inTileY * tileW + inTileX)*channels;

    return Vector3f{pixelData[index], pixelData[index+1], pixelData[index+2]};

}

float Texture::getIntensityVal(float u, float v) const{ //<- STEVE CHANGE
    //Simple bilinear filtering
    // float intU;
    // float tU = std::modf(u * (width-1), &intU);
    // int uIntLo = (int)intU; 
    // int uIntHi = std::ceil(u * (width-1)); 

    // float intV;
    // float tV = std::modf(v * (height-1), &intV);
    // int vIntLo = (int)intV; 
    // int vIntHi = std::ceil(v * (height-1));

    // int index00 = (vIntLo*width + uIntLo)*channels;
    // int index01 = (vIntLo*width + uIntHi)*channels;
    // int index02 = (vIntHi*width + uIntLo)*channels;
    // int index03 = (vIntHi*width + uIntHi)*channels;

    // float val   = (pixelData[index00]*(1 - tU) + pixelData[index01]*tU)*(1-tV) + (pixelData[index02]*(1-tU) + pixelData[index03]*tU)*tV;

    // return val;

    // int uInt = u * (width-1); 
    // int vInt = v * (height-1);
    // int index = (vInt*width + uInt);
    // return pixelData[index];

    int uInt = u * (width-1); 
    int vInt = v * (height-1);

    int tileX = uInt / tileW;
    int tileY = vInt / tileH;

    int inTileX = uInt - tileX * tileW; //uInt % tileW; <- STEVE CHANGE
    int inTileY = vInt - tileY * tileH; //vInt % tileH; <- STEVE CHANGE

    int index = ((tileY * widthInTiles + tileX) * (tileW * tileH)
                + inTileY * tileW
                + inTileX);

    return pixelData[index];
}

// STEVE CHANGE
void Texture::Load (const unsigned char * data, int w, int h, int comp)
{
    // STEVE CHANGE
    width = w;
    height = h;
    channels = comp;
    // /STEVE CHANGE

    widthInTiles = (width + tileW -1) / tileW;

    pixelData.resize(width*height*channels); // pixelData = new float[width*height*channels]; <- STEVE CHANGE

    if (kRGB == textureType) { //Rgb data requires a gamma correction, float conversion
        for(int i = 0; i < width*height*channels; ++i){
            pixelData[i] = Rasterizer::gammaAdjustOnLoad(data[i]); // std::pow((float)data[i] * (1/255.0f), 2.2f); // <- STEVE CHANGE (major optimization)
        }
    }
    else if (kXYZ == textureType){//conversion to float and rescaling to -1 1 bounds
        for(int i = 0; i < width*height*channels; ++i){
            pixelData[i] = (float)data[i] * (2/255.0f) - 1.0f;
        }
    }
    else if (kBW == textureType){ //Simple conversion to float
        for(int i = 0; i < width*height*channels; ++i){
            pixelData[i] = (float)data[i] * (1/255.0f);
        }
    }
    tileData();
}

//
//
//

#define TEXTURE_TYPE "sceneView3D.ssge.Texture"

//
//
//

Texture * Texture::Get (lua_State * L, int arg)
{
    return LuaXS::CheckUD<Texture>(L, arg, TEXTURE_TYPE);
}

//
//
//

static int sNewRef;

Texture * Texture::PushNew (lua_State * L, const char * type, std::vector<float> * workspace)
{
    lua_getref(L, sNewRef); // ..., NewTexture
    lua_pushstring(L, type); // ..., NewTexture, type
    lua_call(L, 1, 1); // ..., texture

    return Texture::Get(L, -1);
}

//
//
//

static Texture::TextureType GetType (lua_State * L)
{
    const char * names[] = { "RGB", "XYZ", "BW", nullptr };

    return Texture::TextureType(luaL_checkoption(L, 1, nullptr, names));
}

//
//
//

void Texture::add_texture (lua_State * L)
{
    lua_pushcfunction(L, [](lua_State * L) {
        LuaXS::NewTyped<Texture>(L, GetType(L)); // type, texture
        LuaXS::AttachMethods(L, TEXTURE_TYPE, [](lua_State * L) {
            luaL_Reg funcs[] = {
                {
                    "Bind", [](lua_State * L)
                    {
                        Texture * texture = Texture::Get(L);

                        ByteReader bytes{L, 2};

                        int w = luaL_checkint(L, 3), h = luaL_checkint(L, 4), channels = texture->ChannelCount();

                        luaL_argcheck(L, bytes.mBytes && bytes.mCount >= w * h * channels, 2, "Not enough data provided for texture");

                        texture->Load(static_cast<const unsigned char *>(bytes.mBytes), w, h, channels);

                        return 0;
                    }
                }, {
                    "__gc", LuaXS::TypedGC<Texture>
                },
                { nullptr, nullptr }
            };

            luaL_register(L, nullptr, funcs);
        });

        return 1;
    }); // ..., ssge, NewTexture
    lua_pushvalue(L, -1); // ..., ssge, NewTexture, NewTexture
    lua_setfield(L, -3, "NewTexture"); // ..., ssge = { ..., NewTexture = NewTexture }, NewTexture

    sNewRef = lua_ref(L, 1); // ..., ssge; ref = NewTexture
}
// /STEVE CHANGE