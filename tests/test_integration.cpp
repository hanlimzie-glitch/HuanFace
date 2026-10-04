/**
 * HuanFace Integration Tests — Phase 4
 * Tests image->face->mask->makeup->output end-to-end
 */

#include "../sdk/include/huanface/huanface_image.h"
#include "../sdk/src/face/simple_face_tracker.h"
#include "../sdk/src/makeup/face_mask.h"
#include "../sdk/src/makeup/makeup_engine.h"
#include "../sdk/include/huanface_c_api.h"
#include <iostream>
#include <vector>
#include <chrono>

using namespace huanface;

static int checks=0, passed=0;
static void CHECK(bool cond, const char* msg) { checks++; if(cond) passed++; else std::cout<<"  FAIL: "<<msg<<std::endl; }

bool TestIntegration() {
    std::cout << "=== Test Integration Phase 4 (image->face->mask->makeup->output) ===" << std::endl;
    checks=0; passed=0;

    // Create synthetic image with face
    int w=200,h=200;
    HFImage img;
    img.width=w; img.height=h; img.channels=4;
    img.data.resize(w*h*4);
    for (int y=0;y<h;++y) for (int x=0;x<w;++x) {
        size_t idx=(y*w+x)*4;
        img.data[idx+0]=0; img.data[idx+1]=0; img.data[idx+2]=0; img.data[idx+3]=255;
    }
    for (int y=50;y<150;++y) for (int x=50;x<150;++x) {
        size_t idx=(y*w+x)*4;
        img.data[idx+0]=220; img.data[idx+1]=180; img.data[idx+2]=150; img.data[idx+3]=255;
    }

    // Face tracker
    SimpleFaceTracker tracker;
    HFEngineConfigC config={}; config.maxFaces=1; config.minFaceRatio=0.05f;
    tracker.Init(config);
    HFFrameC frame={};
    frame.width=w; frame.height=h; frame.format=HF_FORMAT_RGBA8; frame.stride=w*4; frame.data=img.data.data();
    HFTrackingData tracking;
    HFResult res = tracker.Process(&frame, tracking);
    CHECK(res==HF_RESULT_OK, "Face tracker process OK");
    CHECK(!tracking.faces.empty(), "Face detected in integration");

    if (tracking.faces.empty()) {
        std::cout << "  No face, cannot continue integration" << std::endl;
        std::cout << "  Checks: " << passed << "/" << checks << std::endl;
        return false;
    }

    // Mask generator
    FaceMaskGenerator maskGen;
    FaceMask faceMask, lipMask;
    std::string err;
    bool ok = maskGen.GenerateFaceMask(tracking.faces[0].mesh, w, h, faceMask, err);
    CHECK(ok, "Face mask gen OK");
    CHECK(faceMask.IsValid(), "Face mask valid");
    ok = maskGen.GenerateFeatureMask(tracking.faces[0], FeatureMaskType::LIP, w, h, lipMask, err);
    CHECK(ok, "Lip mask gen OK");
    CHECK(lipMask.IsValid(), "Lip mask valid");

    // Makeup engine
    MakeupEnginePrototype makeup;
    makeup.Init();
    MakeupParams params;
    params.intensityLip=0.8f;
    params.lipColor={1.0f,0.2f,0.3f,1.0f};
    HFImage output;
    res = makeup.Process(img, tracking.faces[0], lipMask, params, output, err);
    CHECK(res==HF_RESULT_OK, "Makeup process OK");
    CHECK(output.IsValid(), "Makeup output valid");
    CHECK(output.width==w && output.height==h, "Output dims match input");

    // Check that lip area changed color (at least some pixels)
    int changed=0;
    for (int y=0;y<h;++y) for (int x=0;x<w;++x) {
        size_t idx=(y*w+x)*4;
        if (lipMask.data[y*w+x]>0) {
            // Should be more reddish
            if (output.data[idx+0] != img.data[idx+0] || output.data[idx+1]!=img.data[idx+1]) changed++;
        }
    }
    CHECK(changed>0, "Lip makeup visible (pixels changed)");
    std::cout << "  Lip pixels changed: " << changed << std::endl;

    // Performance measure
    auto t0 = std::chrono::high_resolution_clock::now();
    for (int i=0;i<10;++i) {
        HFTrackingData td;
        tracker.Process(&frame, td);
        FaceMask fm, lm;
        if (!td.faces.empty()) {
            maskGen.GenerateFaceMask(td.faces[0].mesh, w, h, fm, err);
            maskGen.GenerateFeatureMask(td.faces[0], FeatureMaskType::LIP, w, h, lm, err);
            HFImage out;
            makeup.Process(img, td.faces[0], lm, params, out, err);
        }
    }
    auto t1 = std::chrono::high_resolution_clock::now();
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(t1-t0).count();
    std::cout << "  Perf: 10 iterations " << ms << "ms, avg " << ms/10.0f << "ms, FPS baseline " << 1000.0f/(ms/10.0f) << std::endl;

    tracker.Shutdown();

    std::cout << "  Checks: " << passed << "/" << checks << std::endl;
    return passed==checks;
}
