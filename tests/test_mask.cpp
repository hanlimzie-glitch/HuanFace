/**
 * HuanFace Face Mask Tests — Phase 4
 * Tests dimensions/empty/bounds
 */

#include "../sdk/src/makeup/face_mask.h"
#include "../sdk/src/face/face_data.h"
#include <iostream>
#include <cmath>

using namespace huanface;

static int checks=0, passed=0;
static void CHECK(bool cond, const char* msg) { checks++; if(cond) passed++; else std::cout<<"  FAIL: "<<msg<<std::endl; }

bool TestMask() {
    std::cout << "=== Test Face Mask Phase 4 ===" << std::endl;
    checks=0; passed=0;

    FaceMaskGenerator gen;

    // Test empty mesh
    {
        HFFaceMesh mesh;
        FaceMask mask;
        std::string err;
        bool ok = gen.GenerateFaceMask(mesh, 100, 100, mask, err);
        CHECK(!ok, "Empty mesh should fail");
    }

    // Test valid mesh
    {
        HFFaceMesh mesh;
        mesh.width=100; mesh.height=100;
        // Simple triangle
        mesh.vertices.push_back(HFVec3(10,10,0));
        mesh.vertices.push_back(HFVec3(90,10,0));
        mesh.vertices.push_back(HFVec3(50,90,0));
        mesh.uv.push_back(HFVec2UV(0.1f,0.1f));
        mesh.uv.push_back(HFVec2UV(0.9f,0.1f));
        mesh.uv.push_back(HFVec2UV(0.5f,0.9f));
        mesh.indices = {0,1,2};
        FaceMask mask;
        std::string err;
        bool ok = gen.GenerateFaceMask(mesh, 100, 100, mask, err);
        CHECK(ok, "Valid triangle mesh should generate mask");
        CHECK(mask.IsValid(), "Mask valid");
        CHECK(mask.width==100 && mask.height==100, "Mask dimensions 100x100");
        // Check some pixels inside triangle
        int inside = 0;
        for (auto v: mask.data) if (v>0) inside++;
        CHECK(inside>0, "Mask has inside pixels");
        CHECK(inside < 100*100, "Mask not fully filled");
    }

    // Test bounds
    {
        HFFaceMesh mesh;
        mesh.width=200; mesh.height=200;
        mesh.vertices = {HFVec3(0,0,0), HFVec3(199,0,0), HFVec3(0,199,0)};
        mesh.uv = {HFVec2UV(0,0), HFVec2UV(1,0), HFVec2UV(0,1)};
        mesh.indices = {0,1,2};
        FaceMask mask;
        std::string err;
        bool ok = gen.GenerateFaceMask(mesh, 200, 200, mask, err);
        CHECK(ok, "Bounds test mask gen OK");
        // Check no out-of-bounds access
        CHECK(mask.data.size()==40000, "Mask size 200x200");
    }

    // Test lip mask
    {
        HFFaceData face;
        face.bboxX=50; face.bboxY=50; face.bboxW=100; face.bboxH=100;
        // Generate 68 landmarks like tracker does
        for (int i=0;i<68;++i) {
            float x = 50 + (i%10)*10;
            float y = 50 + (i/10)*10;
            face.landmarks.push_back(HFVec2(x,y));
        }
        // For lip, we need specific lip landmarks 48-67 to form lip
        // Set them to form a lip shape
        float mx=100, my=125;
        for (int i=48;i<60;++i) {
            float ang = (float)(i-48)/12.0f*2*3.14159f;
            face.landmarks[i] = HFVec2(mx + cosf(ang)*20, my + sinf(ang)*8);
        }
        for (int i=60;i<68;++i) {
            float ang = (float)(i-60)/8.0f*2*3.14159f;
            face.landmarks[i] = HFVec2(mx + cosf(ang)*10, my + sinf(ang)*4);
        }
        FaceMask lipMask;
        std::string err;
        bool ok = gen.GenerateFeatureMask(face, FeatureMaskType::LIP, 200, 200, lipMask, err);
        CHECK(ok, "Lip mask generation OK");
        CHECK(lipMask.IsValid(), "Lip mask valid");
        CHECK(lipMask.width==200 && lipMask.height==200, "Lip mask dimensions");
        int lipPixels=0;
        for (auto v: lipMask.data) if (v>0) lipPixels++;
        CHECK(lipPixels>0, "Lip mask has pixels");
    }

    std::cout << "  Checks: " << passed << "/" << checks << std::endl;
    return passed==checks;
}
