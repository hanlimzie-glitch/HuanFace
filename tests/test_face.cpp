/**
 * HuanFace Face Tracker Tests — Phase 4
 * Tests init/no-face/face/landmarks count/confidence/coord conversion
 */

#include "../sdk/src/face/simple_face_tracker.h"
#include "../sdk/src/face/face_data.h"
#include "../sdk/include/huanface_c_api.h"
#include "../sdk/include/huanface/huanface_image.h"
#include <iostream>
#include <vector>

using namespace huanface;

static int checks=0, passed=0;
static void CHECK(bool cond, const char* msg) { checks++; if(cond) passed++; else std::cout<<"  FAIL: "<<msg<<std::endl; }

bool TestFace() {
    std::cout << "=== Test Face Tracker Phase 4 ===" << std::endl;
    checks=0; passed=0;

    // Test init
    {
        SimpleFaceTracker tracker;
        HFEngineConfigC config={};
        config.maxFaces=1;
        config.minFaceRatio=0.1f;
        HFResult res = tracker.Init(config);
        CHECK(res==HF_RESULT_OK, "Tracker Init OK");
        tracker.Shutdown();
    }

    // Test no-face: black image
    {
        SimpleFaceTracker tracker;
        HFEngineConfigC config={}; config.maxFaces=1; config.minFaceRatio=0.1f;
        tracker.Init(config);
        int w=128,h=128;
        std::vector<uint8_t> data(w*h*4,0);
        for (int i=0;i<w*h*4;i+=4) data[i+3]=255;
        HFFrameC frame={};
        frame.width=w; frame.height=h; frame.format=HF_FORMAT_RGBA8; frame.stride=w*4; frame.data=data.data();
        HFTrackingData tracking;
        HFResult res = tracker.Process(&frame, tracking);
        CHECK(res==HF_RESULT_OK, "Process black image OK");
        CHECK(tracking.faces.empty(), "No face in black image");
        tracker.Shutdown();
    }

    // Test face: create synthetic skin-colored face
    {
        SimpleFaceTracker tracker;
        HFEngineConfigC config={}; config.maxFaces=1; config.minFaceRatio=0.05f;
        tracker.Init(config);
        int w=200,h=200;
        std::vector<uint8_t> data(w*h*4,0);
        // Fill background black
        for (int y=0;y<h;++y) for (int x=0;x<w;++x) {
            size_t idx=(y*w+x)*4;
            data[idx+0]=0; data[idx+1]=0; data[idx+2]=0; data[idx+3]=255;
        }
        // Draw skin-colored rectangle as face (approx skin color)
        // Skin RGB approx 220,180,150
        for (int y=50;y<150;++y) for (int x=50;x<150;++x) {
            size_t idx=(y*w+x)*4;
            data[idx+0]=220; data[idx+1]=180; data[idx+2]=150; data[idx+3]=255;
        }
        HFFrameC frame={};
        frame.width=w; frame.height=h; frame.format=HF_FORMAT_RGBA8; frame.stride=w*4; frame.data=data.data();
        HFTrackingData tracking;
        HFResult res = tracker.Process(&frame, tracking);
        CHECK(res==HF_RESULT_OK, "Process skin face OK");
        CHECK(!tracking.faces.empty(), "Face detected in skin rect");
        if (!tracking.faces.empty()) {
            auto& face = tracking.faces[0];
            CHECK(face.confidence>0.2f && face.confidence<=1.0f, "Confidence in range");
            CHECK(face.bboxW>0 && face.bboxH>0, "BBox valid");
            CHECK(face.landmarks.size()>=5, "Landmarks at least 5");
            CHECK(face.landmarks.size()==68, "Landmarks 68 for Phase 4");
            CHECK(face.mesh.IsValid(), "Mesh valid");
            CHECK(face.mesh.vertices.size()==25, "Mesh 5x5=25 vertices");
            // Coord conversion test
            HFVec2UV uv = face.LandmarkToUV(0, w, h);
            CHECK(uv.u>=0 && uv.u<=1 && uv.v>=0 && uv.v<=1, "LandmarkToUV in [0,1]");
            HFVec2 ndc = HFFaceData::UVToD3D11NDC(uv);
            CHECK(ndc.x>=-1 && ndc.x<=1 && ndc.y>=-1 && ndc.y<=1, "UVToD3D11NDC in [-1,1]");
        }
        tracker.Shutdown();
    }

    // Test landmarks count runtime-defined (not hardcoded 68/106 in API, but we generate 68)
    {
        HFFaceData face;
        face.bboxX=10; face.bboxY=10; face.bboxW=100; face.bboxH=100;
        face.confidence=0.9f;
        // Generate landmarks via tracker helper (we already tested)
        // Just check that API allows any count
        face.landmarks.resize(5);
        CHECK(face.landmarks.size()==5, "Runtime-defined landmarks 5 allowed");
        face.landmarks.resize(68);
        CHECK(face.landmarks.size()==68, "Runtime-defined landmarks 68 allowed");
        face.landmarks.resize(106);
        CHECK(face.landmarks.size()==106, "Runtime-defined landmarks 106 allowed");
    }

    std::cout << "  Checks: " << passed << "/" << checks << std::endl;
    return passed==checks;
}
