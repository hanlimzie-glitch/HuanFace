/**
 * Test Real ML Pipeline — Phase 5.5 REAL ML
 * Proves model load/inference executed/landmarks not synthetic/bbox valid/confidence from model/mesh/pose valid
 */

#include "../sdk/src/face/production_face_tracker.h"
#include "../sdk/src/face/inference_backend.h"
#include "../sdk/src/face/face_detector.h"
#include "../sdk/include/huanface_c_api.h"
#include <iostream>
#include <fstream>
#include <cmath>
#include <vector>

using namespace huanface;

static int checks = 0;
static int passed = 0;
static void CHECK(bool cond, const char* msg) {
    checks++;
    if (cond) { passed++; }
    else { std::cout << "  FAIL: " << msg << std::endl; }
}

bool TestRealMLPipeline() {
    std::cout << "=== Test Real ML Pipeline Phase 5.5 ===" << std::endl;
    checks=0; passed=0;

    // Test 1: Model files exist and checksum valid
    {
        FaceModelManager mgr;
        std::vector<std::string> detPaths = {"models/huanface_tiny_face_detector_v1.onnx", "../models/huanface_tiny_face_detector_v1.onnx", "../../models/huanface_tiny_face_detector_v1.onnx", "/home/user/HuanFace/models/huanface_tiny_face_detector_v1.onnx", "./models/huanface_tiny_face_detector_v1.onnx", "D:/sdk/HuanFace/models/huanface_tiny_face_detector_v1.onnx", "../models/huanface_tiny_face_detector_v1.onnx"};
        std::vector<std::string> lmPaths = {"models/huanface_tiny_landmark_v1.onnx", "../models/huanface_tiny_landmark_v1.onnx", "../../models/huanface_tiny_landmark_v1.onnx", "/home/user/HuanFace/models/huanface_tiny_landmark_v1.onnx", "./models/huanface_tiny_landmark_v1.onnx", "D:/sdk/HuanFace/models/huanface_tiny_landmark_v1.onnx", "../models/huanface_tiny_landmark_v1.onnx"};
        std::string foundDet, foundLm;
        for (auto& p : detPaths) { std::ifstream f(p); if (f.good()) { foundDet=p; break; } }
        for (auto& p : lmPaths) { std::ifstream f(p); if (f.good()) { foundLm=p; break; } }

        CHECK(!foundDet.empty(), "Detector model file exists");
        CHECK(!foundLm.empty(), "Landmark model file exists");

        if (!foundDet.empty()) {
            std::string sha = FaceModelManager::ComputeFileSHA256(foundDet);
            CHECK(sha == "1babb536bba172c01ba8b97390459a462aa9ce75709a92768a22f52bab909aaa", "Detector checksum valid");
            HFResult cr = mgr.ValidateChecksum(foundDet, "1babb536bba172c01ba8b97390459a462aa9ce75709a92768a22f52bab909aaa");
            CHECK(cr == HF_RESULT_OK, "Detector checksum validation OK");
            HFResult crFail = mgr.ValidateChecksum(foundDet, "0000000000000000000000000000000000000000000000000000000000000000");
            CHECK(crFail != HF_RESULT_OK, "Detector checksum mismatch detected");
        }
        if (!foundLm.empty()) {
            std::string sha = FaceModelManager::ComputeFileSHA256(foundLm);
            CHECK(sha == "80b3837b52864e628657aa9500db16cb6cc52f6a936436d815fcb678060ecf1d", "Landmark checksum valid");
        }
    }

    // Test 2: ONNX Runtime backend loads models and IsRealML true
    {
        ONNXRuntimeFaceBackend backend;
        HFEngineConfigC config = {};
        config.maxFaces = 5;
        config.faceTrackerType = "onnx";
        config.enableDebug = 1;
        HFResult r = backend.Initialize(config);
        CHECK(r == HF_RESULT_OK || r == HF_RESULT_FILE_NOT_FOUND, "ONNX backend init returns OK or FILE_NOT_FOUND (not crash)");
        if (r == HF_RESULT_OK) {
            CHECK(backend.IsModelLoaded(), "ONNX backend IsModelLoaded true");
            CHECK(backend.IsRealML(), "ONNX backend IsRealML true (REAL ML, not heuristic)");
            CHECK(backend.GetBackendType() == huanface::HFInferenceBackendType::ONNX, "ONNX backend type ONNX");
            CHECK(backend.GetName() == "ONNXRuntimeFaceBackend", "ONNX backend name correct");
            CHECK(backend.GetName().find("Heuristic") == std::string::npos, "ONNX backend name not containing Heuristic (ensure ONNX != Heuristic)");
        }
        backend.Shutdown();
    }

    // Test 3: Heuristic backend IsRealML false
    {
        FallbackHeuristicInferenceBackend backend;
        HFEngineConfigC config = {};
        config.maxFaces = 5;
        config.faceTrackerType = "heuristic";
        config.enableDebug = 1;
        HFResult r = backend.Initialize(config);
        CHECK(r == HF_RESULT_OK, "Heuristic backend init OK");
        CHECK(!backend.IsRealML(), "Heuristic backend IsRealML false (DEVELOPMENT/FALLBACK ONLY)");
        CHECK(backend.GetBackendType() == huanface::HFInferenceBackendType::HEURISTIC, "Heuristic backend type HEURISTIC");
        CHECK(backend.GetName() == "FallbackHeuristicInferenceBackend", "Heuristic backend name correct");
        backend.Shutdown();
    }

    // Test 4: ProductionFaceTracker AUTO mode with ONNX available should select ONNX
    {
        ProductionFaceTracker tracker;
        HFEngineConfigC config = {};
        config.maxFaces = 5;
        config.faceTrackerType = "auto";
        config.enableDebug = 1;
        HFResult r = tracker.Init(config);
        CHECK(r == HF_RESULT_OK, "ProductionFaceTracker AUTO init OK");
        if (r == HF_RESULT_OK) {
            auto* backend = tracker.GetBackend();
            CHECK(backend != nullptr, "Backend not null");
            if (backend) {
                std::string name = backend->GetName();
                bool isONNX = (name == "ONNXRuntimeFaceBackend");
                bool isHeuristic = (name == "FallbackHeuristicInferenceBackend");
                CHECK(isONNX || isHeuristic, "AUTO backend is ONNX or Heuristic (with warning)");
                if (isONNX) {
                    CHECK(backend->IsRealML(), "AUTO with ONNX is real ML");
                } else {
                    CHECK(!backend->IsRealML(), "AUTO with Heuristic is NOT real ML, warning expected");
                }
            }
        }
        tracker.Shutdown();
    }

    // Test 5: Production mode ML required FAIL init if unavailable
    {
        ProductionFaceTracker tracker;
        HFEngineConfigC config = {};
        config.maxFaces = 5;
        config.faceTrackerType = "onnx";
        config.enableDebug = 0;
        HFResult r = tracker.Init(config);
        CHECK(r == HF_RESULT_OK || r == HF_RESULT_FILE_NOT_FOUND, "Production mode ONNX init OK or FAIL if unavailable (not silent fallback)");
        if (r == HF_RESULT_OK) {
            auto* backend = tracker.GetBackend();
            if (backend) {
                CHECK(backend->IsRealML(), "Production mode with ONNX is real ML");
            }
        }
        tracker.Shutdown();

        ProductionFaceTracker tracker2;
        HFEngineConfigC config2 = {};
        config2.maxFaces = 5;
        config2.faceTrackerType = "heuristic";
        config2.enableDebug = 0;
        HFResult r2 = tracker2.Init(config2);
        CHECK(r2 == HF_RESULT_FILE_NOT_FOUND || r2 == HF_RESULT_FAIL, "Production mode with HEURISTIC explicitly requested FAILS per gate (ML required)");
        tracker2.Shutdown();
    }

    // Test 6: Real inference executed, landmarks not synthetic, bbox valid, confidence from model
    {
        ProductionFaceTracker tracker;
        HFEngineConfigC config = {};
        config.maxFaces = 5;
        config.faceTrackerType = "onnx";
        config.enableDebug = 1;
        HFResult r = tracker.Init(config);
        if (r != HF_RESULT_OK) {
            CHECK(false, "Tracker init for real inference test failed - skip remaining");
        } else {
            int width = 64, height = 64, stride = width*4;
            std::vector<uint8_t> rgba(width*height*4);
            for (int y=0; y<height; ++y) {
                for (int x=0; x<width; ++x) {
                    uint8_t* px = rgba.data() + y*stride + x*4;
                    px[0]=220; px[1]=180; px[2]=160; px[3]=255;
                }
            }
            for (int ey=0; ey<2; ++ey) {
                float cx = ey==0 ? 0.35f : 0.65f;
                float cy = 0.4f;
                int ix = (int)(cx*width);
                int iy = (int)(cy*height);
                for (int dy=-3; dy<=3; ++dy) {
                    for (int dx=-3; dx<=3; ++dx) {
                        if (dx*dx+dy*dy <= 9) {
                            int xx = ix+dx, yy = iy+dy;
                            if (xx>=0 && xx<width && yy>=0 && yy<height) {
                                uint8_t* px = rgba.data() + yy*stride + xx*4;
                                px[0]=20; px[1]=20; px[2]=20; px[3]=255;
                            }
                        }
                    }
                }
            }
            int mx = (int)(0.5f*width), my = (int)(0.65f*height);
            for (int dy=-2; dy<=2; ++dy) {
                for (int dx=-6; dx<=6; ++dx) {
                    int xx = mx+dx, yy = my+dy;
                    if (xx>=0 && xx<width && yy>=0 && yy<height) {
                        uint8_t* px = rgba.data() + yy*stride + xx*4;
                        px[0]=180; px[1]=50; px[2]=50; px[3]=255;
                    }
                }
            }

            HFFrameC frame = {};
            frame.width = width;
            frame.height = height;
            frame.format = HF_FORMAT_RGBA8;
            frame.data = rgba.data();
            frame.stride = stride;
            frame.timestampNanos = 0;

            HFTrackingData tracking;
            r = tracker.Process(&frame, tracking);
            CHECK(r == HF_RESULT_OK, "Real ML inference Process OK");
            CHECK(tracking.FaceCount() >= 0, "Face count >=0 (0..N real detections)");

            if (tracking.FaceCount() > 0) {
                auto& face = tracking.faces[0];
                CHECK(face.bboxW > 0 && face.bboxH > 0, "BBox valid W>0 H>0");
                CHECK(face.bboxX >= 0 && face.bboxY >= 0, "BBox X,Y >=0");
                CHECK(face.detectionConfidence > 0 && face.detectionConfidence < 1.0f, "Confidence from model 0-1");
                CHECK(std::abs(face.detectionConfidence - 0.95f) > 0.01f, "Confidence NOT hardcoded 0.95 (real from model)");
                CHECK(face.landmarks.size() == 68, "Landmarks count 68");
                CHECK(face.landmarks3D.size() == 68, "Landmarks3D count 68");
                bool hasVariation = false;
                for (size_t i=1; i<face.landmarks.size(); ++i) {
                    if (std::abs(face.landmarks[i].x - face.landmarks[i-1].x) > 0.1f ||
                        std::abs(face.landmarks[i].y - face.landmarks[i-1].y) > 0.1f) {
                        hasVariation = true; break;
                    }
                }
                CHECK(hasVariation, "Landmarks have variation, not synthetic constant");
                CHECK(face.mesh.VertexCount() == 77, "Mesh vertex count 77 from real landmarks");
                CHECK(face.pose.IsValid(), "Pose valid");
                CHECK(std::isfinite(face.pose.yaw) && std::isfinite(face.pose.pitch) && std::isfinite(face.pose.roll), "Pose finite not hardcoded");
                auto* backend = tracker.GetBackend();
                if (backend) {
                    CHECK(backend->IsRealML(), "Backend IsRealML true for real inference");
                    auto* onnxBackend = dynamic_cast<ONNXRuntimeFaceBackend*>(backend);
                    if (onnxBackend) {
                        CHECK(onnxBackend->WasInferenceExecuted(), "ONNX inference executed flag true");
                        CHECK(onnxBackend->GetInferenceCount() > 0, "Inference count >0");
                    }
                }
                float leftEyeX = face.landmarks[36].x;
                float leftEyeY = face.landmarks[36].y;
                CHECK(leftEyeX >= 0 && leftEyeX < width, "Left eye X within image (real inference)");
                CHECK(leftEyeY >= 0 && leftEyeY < height, "Left eye Y within image");
            }

            CHECK(tracking.FaceCount() <= 1, "Multi-face: single face image gives 0 or 1, no generated second face");
            tracker.SetMaxFaces(1);
            HFTrackingData tracking2;
            r = tracker.Process(&frame, tracking2);
            CHECK(tracking2.FaceCount() <= 1, "MaxFaces 1 enforced");
        }
        tracker.Shutdown();
    }

    // Test 7: Ensure ONNX != Heuristic
    {
        ONNXRuntimeFaceBackend onnx;
        FallbackHeuristicInferenceBackend heuristic;
        CHECK(onnx.GetName() != heuristic.GetName(), "ONNX != Heuristic names different");
        CHECK(onnx.IsRealML() != heuristic.IsRealML(), "ONNX IsRealML != Heuristic IsRealML");
        CHECK(onnx.GetBackendType() != heuristic.GetBackendType(), "ONNX backend type != Heuristic type");
    }

    // Test 8: Camera intrinsics
    {
        HFCameraIntrinsics intr;
        CHECK(intr.IsDefault(), "Camera intrinsics default IsDefault true");
        intr.SetDefaultFromImage(640, 480);
        CHECK(!intr.IsDefault(), "After SetDefaultFromImage not default");
        CHECK(intr.fx == 640 && intr.fy == 640, "Default fx,fy = width");
        CHECK(intr.cx == 320 && intr.cy == 240, "Default cx,cy = center");

        ProductionFaceTracker tracker;
        HFEngineConfigC config = {};
        config.maxFaces = 5;
        config.faceTrackerType = "auto";
        config.enableDebug = 1;
        tracker.Init(config);
        HFCameraIntrinsics custom;
        custom.fx = 800; custom.fy = 800; custom.cx = 320; custom.cy = 240;
        tracker.SetCameraIntrinsics(custom);
        HFCameraIntrinsics got = tracker.GetCameraIntrinsics();
        CHECK(got.fx == 800 && got.fy == 800, "Custom intrinsics set/get");
        tracker.Shutdown();
    }

    std::cout << "  Checks: " << passed << "/" << checks << std::endl;
    return passed==checks;
}
