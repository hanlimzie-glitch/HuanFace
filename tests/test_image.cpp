/**
 * HuanFace Image Loader Tests — Phase 4
 * Tests PNG/JPG/invalid/save
 */

#include "../sdk/include/huanface/huanface_image.h"
#include <iostream>
#include <vector>
#include <cstring>
#include <filesystem>

using namespace huanface;
namespace fs = std::filesystem;

static int checks = 0;
static int passed = 0;
static void CHECK(bool cond, const char* msg) {
    checks++;
    if (cond) { passed++; } else { std::cout << "  FAIL: " << msg << std::endl; }
}

bool TestImage() {
    std::cout << "=== Test Image Loader Phase 4 ===" << std::endl;
    checks = 0; passed = 0;

    // Test 1: Create solid PNG and load it
    {
        HFImage img;
        img.width = 64;
        img.height = 64;
        img.channels = 4;
        img.data.resize(64*64*4);
        for (int y=0;y<64;++y) for (int x=0;x<64;++x) {
            size_t idx = (y*64+x)*4;
            img.data[idx+0]= (uint8_t)(x*4);
            img.data[idx+1]= (uint8_t)(y*4);
            img.data[idx+2]= 128;
            img.data[idx+3]= 255;
        }
        std::string err;
        std::string tmpPng = (fs::temp_directory_path() / "test_image.png").string();
        bool ok = ImageLoader::SaveImage(tmpPng, img, err);
        CHECK(ok, "Save PNG should succeed");
        if (!ok) std::cout << "  err=" << err << std::endl;

        HFImage loaded;
        ok = ImageLoader::LoadImage(tmpPng, loaded, err);
        CHECK(ok, "Load PNG should succeed");
        CHECK(loaded.width==64 && loaded.height==64, "Loaded dimensions 64x64");
        CHECK(loaded.data.size()==64*64*4, "Loaded data size");
        if (loaded.IsValid()) {
            // Check first pixel
            CHECK(loaded.data[0]==0 && loaded.data[1]==0, "First pixel check");
        }
    }

    // Test 2: Invalid file
    {
        HFImage img;
        std::string err;
        std::string tmpNon = (fs::temp_directory_path() / "nonexistent.png").string();
        bool ok = ImageLoader::LoadImage(tmpNon, img, err);
        CHECK(!ok, "Load nonexistent should fail");
    }

    // Test 3: Invalid data memory
    {
        HFImage img;
        std::string err;
        uint8_t badData[10]={0,1,2,3,4,5,6,7,8,9};
        bool ok = ImageLoader::LoadImageFromMemory(badData, 10, img, err);
        CHECK(!ok, "Load invalid memory should fail");
    }

    // Test 4: PNG encode to memory
    {
        std::vector<uint8_t> png;
        std::string err;
        uint8_t rgba[16]={255,0,0,255, 0,255,0,255, 0,0,255,255, 255,255,0,255};
        bool ok = ImageLoader::EncodePNGToMemory(2,2, rgba, png, err);
        CHECK(ok, "EncodePNGToMemory should succeed");
        CHECK(png.size()>0, "PNG size >0");
        CHECK(ImageLoader::IsPNG(png.data(), png.size()), "IsPNG true");
    }

    // Test 5: BMP load (create minimal BMP)
    {
        // Create 2x2 24-bit BMP manually
        // BMP header 54 bytes
        uint8_t bmp[70];
        memset(bmp,0,sizeof(bmp));
        bmp[0]='B'; bmp[1]='M';
        // file size 70
        bmp[2]=70; bmp[3]=0; bmp[4]=0; bmp[5]=0;
        bmp[10]=54; // offset
        bmp[14]=40; // header size
        bmp[18]=2; bmp[22]=2; // w,h
        bmp[26]=1; bmp[28]=24; // planes, bpp
        // data: bottom row first, padded to 4 bytes
        // row size = ((2*3+3)/4)*4 = 8
        // row0 (bottom): blue, white
        bmp[54]=255; bmp[55]=0; bmp[56]=0; // blue (BGR)
        bmp[57]=255; bmp[58]=255; bmp[59]=255; // white
        bmp[60]=0; bmp[61]=0; // padding
        // row1 (top): red, green
        bmp[62]=0; bmp[63]=0; bmp[64]=255; // red
        bmp[65]=0; bmp[66]=255; bmp[67]=0; // green
        bmp[68]=0; bmp[69]=0; // padding

        HFImage img;
        std::string err;
        bool ok = ImageLoader::LoadImageFromMemory(bmp, 70, img, err);
        CHECK(ok, "Load BMP should succeed");
        CHECK(img.width==2 && img.height==2, "BMP dimensions 2x2");
    }

    std::cout << "  Checks: " << passed << "/" << checks << std::endl;
    return passed==checks;
}
