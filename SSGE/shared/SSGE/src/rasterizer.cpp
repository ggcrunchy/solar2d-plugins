// ===============================
// AUTHOR       : Angel Ortiz (angelo12 AT vt DOT edu)
// CREATE DATE  : 2018-07-03
// ===============================

//Includes
#include "rasterizer.h"
#include <algorithm>
#include <cstdint> // <- STEVE CHANGE

using Uint8 = std::uint8_t; // <- STEVE CHANGE

//Gamma correction lookup table, much much faster than actually calculating it
const unsigned char Rasterizer::gammaTable[256] = {0, 21, 28, 34, 39, 43, 46, // <- STEVE CHANGE
        50, 53, 56, 59, 61, 64, 66, 68, 70, 72, 74, 76, 78, 80, 82, 84,
        85, 87, 89, 90, 92, 93, 95, 96, 98, 99, 101, 102, 103, 105, 106,
        107, 109, 110, 111, 112, 114, 115, 116, 117, 118, 119, 120, 122,
        123, 124, 125, 126, 127, 128, 129, 130, 131, 132, 133, 134, 135,
        136, 137, 138, 139, 140, 141, 142, 143, 144, 144, 145, 146, 147,
        148, 149, 150, 151, 151, 152, 153, 154, 155, 156, 156, 157, 158,
        159, 160, 160, 161, 162, 163, 164, 164, 165, 166, 167, 167, 168,
        169, 170, 170, 171, 172, 173, 173, 174, 175, 175, 176, 177, 178,
        178, 179, 180, 180, 181, 182, 182, 183, 184, 184, 185, 186, 186,
        187, 188, 188, 189, 190, 190, 191, 192, 192, 193, 194, 194, 195,
        195, 196, 197, 197, 198, 199, 199, 200, 200, 201, 202, 202, 203,
        203, 204, 205, 205, 206, 206, 207, 207, 208, 209, 209, 210, 210,
        211, 212, 212, 213, 213, 214, 214, 215, 215, 216, 217, 217, 218,
        218, 219, 219, 220, 220, 221, 221, 222, 223, 223, 224, 224, 225,
        225, 226, 226, 227, 227, 228, 228, 229, 229, 230, 230, 231, 231,
        232, 232, 233, 233, 234, 234, 235, 235, 236, 236, 237, 237, 238,
        238, 239, 239, 240, 240, 241, 241, 242, 242, 243, 243, 244, 244,
        245, 245, 246, 246, 247, 247, 248, 248, 249, 249, 249, 250, 250,
        251, 251, 252, 252, 253, 253, 254, 254, 255, 255};

//Initializing all the basic colors 
// STEVE CHANGE
static Uint32 MapRGBA (int r, int g, int b, int a)
{
    return (a << 24) | (b << 16) | (g << 8) | r;
}

static Uint32 MapRGB (int r, int g, int b)
{
    return MapRGBA(r, g, b, 0xFF);
}

//const SDL_PixelFormat* Rasterizer::mappingFormat( SDL_AllocFormat(PIXEL_FORMAT));
const Uint32 Rasterizer::white(/*SDL_*/MapRGBA(/*mappingFormat, */0xFF,0xFF,0xFF,0xFF));
const Uint32 Rasterizer::red(/*SDL_*/MapRGBA(/*mappingFormat, */0xFF,0x00,0x00,0xFF));
const Uint32 Rasterizer::green(/*SDL_*/MapRGBA(/*mappingFormat, */0x00,0xFF,0x00,0xFF));
const Uint32 Rasterizer::blue(/*SDL_*/MapRGBA(/*mappingFormat, */0x00,0x00,0xFF,0xFF));
// /STEVE CHANGE

//Early pixel buffer traversal test
void Rasterizer::makeCoolPattern(Buffer<Uint32> *pixelBuffer){
    for(int y = 0; y < pixelBuffer->mHeight; ++y){
        for(int x = 0; x < pixelBuffer->mWidth; ++x){
            Uint8 r = x*2 % 256;
            Uint8 g = y/8 % 256;
            Uint8 b = r*y % 256;
            Uint32 color = /*SDL_*/MapRGBA(/*mappingFormat, */r,g,b,0xFF); // <- STEVE CHANGE
            (*pixelBuffer)(x,y) = color;
        }
    }
}

void Rasterizer::testPattern(Buffer<Uint32> *pixelBuffer){
    (*pixelBuffer)(600,200) = red;
    (*pixelBuffer)(400,200) = green;
    (*pixelBuffer)(200,200) = blue;

}

void Rasterizer::drawLine(Vector3f &vertex1, Vector3f &vertex2,const Uint32 &color, Buffer<Uint32> *pixelBuffer ){
    //NDC to viewport transform
    int x1 = (vertex1.x + 1 ) * pixelBuffer->mWidth * 0.5f;
    int y1 = (-vertex1.y + 1 ) * pixelBuffer->mHeight * 0.5f;
    int x2 = (vertex2.x +1 ) * pixelBuffer->mWidth  * 0.5f;
    int y2 = (-vertex2.y +1 ) * pixelBuffer->mHeight * 0.5f;

    //transpose line if it is too steep
    bool steep = false;
    if (std::abs(x1-x2) < std::abs(y1-y2) ){
        std::swap(x1,y1);
        std::swap(x2,y2);
        steep = true;
    }

    //Redefine line so that it is left to right
    if ( x1  > x2 ){
        std::swap(x1,x2);
        std::swap(y1,y2);
    }

    //Redefined to use only int arithmetic
    int dx = x2 - x1;
    int dy = y2 - y1;
    int derror2 = std::abs(dy)*2;
    int error2 = 0;
    int y = y1;

    for(int x=x1; x <= x2 ; x++){
        if(steep){
            (*pixelBuffer)(y, x) = color; //Swap back because of steep transpose
        }
        else{
            (*pixelBuffer)(x,y) = color;
        }
        error2 += derror2;
        if (error2 > dx){
            y += (y2 > y1  ? 1 : -1);
            error2 -= dx*2;
        }
    } 
}

void Rasterizer::drawWireFrame(Vector3f *vertices, IShader &shader, Buffer<Uint32> *pixelBuffer){
    drawLine(vertices[0], vertices[1], red, pixelBuffer);
    drawLine(vertices[1], vertices[2], green, pixelBuffer);
    drawLine(vertices[0], vertices[2], blue, pixelBuffer);
}  

//Candidate for future total overhaul into a SIMD function
//Why not now? Before doing this I have to do:
//1.vectorize: Shader, vector3 class rewrite
//2.Edge detection
//3.Fixed point rewrite
//4.SDL function rewrite
//5.Zbuffer rewrite
void Rasterizer::drawTriangles(Vector3f *vertices, IShader &shader, Buffer<Uint32> *pixelBuffer, Buffer<float> *zBuffer){
    //Per triangle variables
    float area;
    Vector3f hW{1/vertices[0].w, 1/vertices[1].w, 1/vertices[2].w};

    //Per fragment variables
    float depth, uPers, vPers, areaPers, count = 0; //u, v, are perspective corrected
    Vector3f e, e_row, f;
    Vector3f rgbVals{255,255,255};
    
    //Transform into viewport coordinates 
    Rasterizer::viewportTransform(pixelBuffer, vertices);

    area = edge(vertices[0],vertices[1],vertices[2]);
    if(area <= 0 ) return;
    area = 1/area;

    //Finding triangle bounding box limits & clips it to the screen width and height
    int xMax, xMin, yMax, yMin;
    Rasterizer::triBoundBox(xMax, xMin, yMax, yMin, vertices, pixelBuffer);

    //Per triangle variables
    Vector3f zVals{vertices[0].z,vertices[1].z,vertices[2].z};
    float A01 = vertices[0].y - vertices[1].y, B01= vertices[1].x - vertices[0].x;
    float A12 = vertices[1].y - vertices[2].y, B12= vertices[2].x - vertices[1].x;
    float A20 = vertices[2].y - vertices[0].y, B20= vertices[0].x - vertices[2].x;

    //Edge values at the first corner of the bounding box
    Vector3f point{(float)xMin, (float)yMin, 0};
    e_row.x = edge(vertices[1], vertices[2], point);
    e_row.y = edge(vertices[2], vertices[0], point);
    e_row.z = edge(vertices[0], vertices[1], point);

    //Iterating through each pixel in triangle bounding box
    for(int y = yMin; y <= yMax; ++y){
        //Bary coordinates at start of row
        e.x = e_row.x;
        e.y = e_row.y;
        e.z = e_row.z;

        for(int x = xMin; x <= xMax; ++x){

            //Originally I followed top left rule to avoid edge rewrites but it was
            //costing me more to perform this check every pixel than just allowing the rewrite
            //It's saved in case the pixel shader gets more complex and the tradeoff becomes
            //worth it
            // if(inside(e.x, A01, B01) && inside(e.y, A12, B12) && inside(e.z, A20, B20) ){

            //Only draw if pixel inside triangle 
            if( (e.x >= 0) && (e.y >= 0 ) && (e.z >= 0 )){

                //Zbuffer check
                depth = (e*area).dotProduct(zVals);
                if((*zBuffer)(x,y) < depth &&  depth <= 1.0){
                    (*zBuffer)(x,y) = depth;

                    //Get perspective correct barycentric coords
                    f = e * hW;
                    areaPers = 1 / (f.data[0] + f.data[1] + f.data[2]);
                    uPers =  f.data[1] * areaPers;
                    vPers =  f.data[2] * areaPers;

                    //Run fragment shader (U, v are barycentric coords)
                    rgbVals = shader.fragment(uPers , vPers);

                    //Update pixel buffer with clamped values 
                    (*pixelBuffer)(x,y) = /*SDL_*/MapRGB(//mappingFormat, // <- STEVE CHANGE
                                                gammaAdjust(rgbVals.data[0]), 
                                                gammaAdjust(rgbVals.data[1]),
                                                gammaAdjust(rgbVals.data[2]));
                }   
            }

            //One step to the right
            e.x += A12;
            e.y += A20;
            e.z += A01;      
        }

        //One row step
        e_row.x += B12;
        e_row.y += B20;
        e_row.z += B01;
    }
}  

void Rasterizer::viewportTransform(Buffer<Uint32> *pixelBuffer, Vector3f *vertices){
    for(int i = 0; i < 3; ++i){
        //Adding half a pixel to avoid gaps on small vertex values
        vertices[i].x = ((vertices[i].x + 1 ) * pixelBuffer->mWidth * 0.5f)  + 0.5f;
        vertices[i].y = ((vertices[i].y + 1 ) * pixelBuffer->mHeight * 0.5f) + 0.5f;
    }
}

void Rasterizer::triBoundBox(int &xMax, int &xMin, int &yMax, int &yMin,Vector3f *vertices, Buffer<Uint32> *pixelBuffer){
    // xMax = std::ceil(std::max({vertices[0].x, vertices[1].x, vertices[2].x}));
    // xMin = std::ceil(std::min({vertices[0].x, vertices[1].x, vertices[2].x}));
    xMax = std::max({vertices[0].x, vertices[1].x, vertices[2].x});
    xMin = std::min({vertices[0].x, vertices[1].x, vertices[2].x});

    // yMax = std::ceil(std::max({vertices[0].y, vertices[1].y, vertices[2].y}));
    // yMin = std::ceil(std::min({vertices[0].y, vertices[1].y, vertices[2].y}));
    yMax = std::max({vertices[0].y, vertices[1].y, vertices[2].y});
    yMin = std::min({vertices[0].y, vertices[1].y, vertices[2].y});

    //Clip against screen
    xMax = std::min(xMax, pixelBuffer->mWidth -1);
    xMin = std::max(xMin, 0);

    yMax = std::min(yMax, pixelBuffer->mHeight -1);
    yMin = std::max(yMin, 0);
}

float Rasterizer::edge(Vector3f &a, Vector3f &b, Vector3f &c){
    return (b.x - a.x)*(c.y - a.y) - (b.y - a.y)*(c.x - a.x);
}
//Doing this check was slower than allowing overwrites on certain triangle edges so it's
bool Rasterizer::inside(float e, float a, float b){
    if ( e > 0 )  return true;
    if ( e < 0 )  return false;
    if ( a > 0 )  return true;
    if ( a < 0 )  return false;
    if ( b > 0 )  return true;
    return false;
}

float Rasterizer::clamp(float n, float lower, float upper) {
  return std::max(lower, std::min(n, upper));
}

//Gamma adjustment table precalculated for a 2.2 gamma value
//signficant ms gains from this!!
int Rasterizer::gammaAdjust(float n) {
    int val = (int)(clamp(n*255, 0, 255)+0.5f);
    return gammaTable[val];
}

// STEVE CHANGE
float Rasterizer::gammaAdjustOnLoad (unsigned char val)
{
    static const float lut[] = {
        0.0000f, 0.0000f, 0.0000f, 0.0001f, 0.0001f, 0.0002f, 0.0003f, 0.0004f, 0.0005f, 0.0006f, 0.0008f, 0.0010f, 0.0012f, 0.0014f, 0.0017f, 0.0020f,
        0.0023f, 0.0026f, 0.0029f, 0.0033f, 0.0037f, 0.0041f, 0.0046f, 0.0050f, 0.0055f, 0.0060f, 0.0066f, 0.0072f, 0.0078f, 0.0084f, 0.0090f, 0.0097f,
        0.0104f, 0.0111f, 0.0119f, 0.0127f, 0.0135f, 0.0143f, 0.0152f, 0.0161f, 0.0170f, 0.0179f, 0.0189f, 0.0199f, 0.0210f, 0.0220f, 0.0231f, 0.0242f,
        0.0254f, 0.0265f, 0.0278f, 0.0290f, 0.0303f, 0.0316f, 0.0329f, 0.0342f, 0.0356f, 0.0370f, 0.0385f, 0.0399f, 0.0415f, 0.0430f, 0.0446f, 0.0461f,
        0.0478f, 0.0494f, 0.0511f, 0.0528f, 0.0546f, 0.0564f, 0.0582f, 0.0600f, 0.0619f, 0.0638f, 0.0658f, 0.0677f, 0.0697f, 0.0718f, 0.0738f, 0.0759f,
        0.0781f, 0.0802f, 0.0824f, 0.0846f, 0.0869f, 0.0892f, 0.0915f, 0.0939f, 0.0963f, 0.0987f, 0.1011f, 0.1036f, 0.1062f, 0.1087f, 0.1113f, 0.1139f,
        0.1166f, 0.1193f, 0.1220f, 0.1247f, 0.1275f, 0.1304f, 0.1332f, 0.1361f, 0.1390f, 0.1420f, 0.1450f, 0.1480f, 0.1511f, 0.1542f, 0.1573f, 0.1604f,
        0.1636f, 0.1669f, 0.1701f, 0.1734f, 0.1768f, 0.1801f, 0.1835f, 0.1870f, 0.1905f, 0.1940f, 0.1975f, 0.2011f, 0.2047f, 0.2084f, 0.2120f, 0.2158f,
        0.2195f, 0.2233f, 0.2271f, 0.2310f, 0.2349f, 0.2388f, 0.2428f, 0.2468f, 0.2508f, 0.2549f, 0.2590f, 0.2632f, 0.2674f, 0.2716f, 0.2758f, 0.2801f,
        0.2845f, 0.2888f, 0.2932f, 0.2977f, 0.3021f, 0.3066f, 0.3112f, 0.3158f, 0.3204f, 0.3250f, 0.3297f, 0.3345f, 0.3392f, 0.3440f, 0.3489f, 0.3537f,
        0.3587f, 0.3636f, 0.3686f, 0.3736f, 0.3787f, 0.3838f, 0.3889f, 0.3941f, 0.3993f, 0.4045f, 0.4098f, 0.4151f, 0.4205f, 0.4259f, 0.4313f, 0.4368f,
        0.4423f, 0.4479f, 0.4535f, 0.4591f, 0.4647f, 0.4704f, 0.4762f, 0.4820f, 0.4878f, 0.4936f, 0.4995f, 0.5054f, 0.5114f, 0.5174f, 0.5234f, 0.5295f,
        0.5356f, 0.5418f, 0.5480f, 0.5542f, 0.5605f, 0.5668f, 0.5732f, 0.5795f, 0.5860f, 0.5924f, 0.5989f, 0.6055f, 0.6121f, 0.6187f, 0.6253f, 0.6320f,
        0.6388f, 0.6456f, 0.6524f, 0.6592f, 0.6661f, 0.6730f, 0.6800f, 0.6870f, 0.6941f, 0.7012f, 0.7083f, 0.7155f, 0.7227f, 0.7299f, 0.7372f, 0.7445f,
        0.7519f, 0.7593f, 0.7667f, 0.7742f, 0.7818f, 0.7893f, 0.7969f, 0.8046f, 0.8122f, 0.8200f, 0.8277f, 0.8355f, 0.8434f, 0.8513f, 0.8592f, 0.8671f,
        0.8751f, 0.8832f, 0.8913f, 0.8994f, 0.9075f, 0.9158f, 0.9240f, 0.9323f, 0.9406f, 0.9490f, 0.9574f, 0.9658f, 0.9743f, 0.9828f, 0.9914f, 1.0000f

    };

    return lut[val];
}
// /STEVE CHANGE
