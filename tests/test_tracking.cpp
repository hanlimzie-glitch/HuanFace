/**
 * Test Tracking — Phase 5
 * face ID persistence, landmark smoothing, pose smoothing, disappearance/reappearance
 */

#include "../sdk/src/face/tracking_state.h"
#include "../sdk/src/face/face_detector.h"
#include <iostream>
#include <vector>
#include <cmath>

using namespace huanface;

static int checks=0, passed=0;
static void CHECK(bool cond, const char* msg) { checks++; if (cond) passed++; else std::cout << "  FAIL: " << msg << std::endl; }

bool TestTracking() {
    std::cout << "=== Test Tracking Phase 5 ===" << std::endl;
    checks=0; passed=0;

    HFEngineConfigC config{};
    TemporalTracker tracker;
    tracker.Init(config);
    tracker.SetSmoothingAlpha(0.6f);

    // Create two detections close to each other (should match same ID)
    FaceDetection det1; det1.x=40; det1.y=40; det1.w=120; det1.h=120; det1.confidence=0.9f;
    FaceDetection det2; det2.x=42; det2.y=41; det2.w=118; det2.h=119; det2.confidence=0.85f;
    FaceDetection det3; det3.x=200; det3.y=40; det3.w=120; det3.h=120; det3.confidence=0.9f; // far away

    HFFaceData face1; face1.bboxX=det1.x; face1.bboxY=det1.y; face1.bboxW=det1.w; face1.bboxH=det1.h;
    face1.confidence=0.9f; face1.detectionConfidence=0.9f;
    face1.landmarks.resize(68);
    for (int i=0;i<68;++i) face1.landmarks[i]=HFVec2(det1.x+det1.w*0.5f + i, det1.y+det1.h*0.5f);
    face1.landmarks3D.resize(68);
    for (int i=0;i<68;++i) face1.landmarks3D[i]=HFVec3(face1.landmarks[i].x, face1.landmarks[i].y, 0);
    face1.pose.yaw=0; face1.pose.pitch=0; face1.pose.roll=0; face1.pose.tx=det1.x+det1.w/2; face1.pose.ty=det1.y+det1.h/2; face1.pose.tz=500; face1.pose.scale=1;

    HFFaceData face2; face2.bboxX=det2.x; face2.bboxY=det2.y; face2.bboxW=det2.w; face2.bboxH=det2.h;
    face2.confidence=0.85f; face2.detectionConfidence=0.85f;
    face2.landmarks.resize(68);
    for (int i=0;i<68;++i) face2.landmarks[i]=HFVec2(det2.x+det2.w*0.5f + i, det2.y+det2.h*0.5f);
    face2.landmarks3D.resize(68);
    for (int i=0;i<68;++i) face2.landmarks3D[i]=HFVec3(face2.landmarks[i].x, face2.landmarks[i].y, 0);
    face2.pose.yaw=2; face2.pose.pitch=1; face2.pose.roll=0; face2.pose.tx=det2.x+det2.w/2; face2.pose.ty=det2.y+det2.h/2; face2.pose.tz=500; face2.pose.scale=1;

    HFFaceData face3; face3.bboxX=det3.x; face3.bboxY=det3.y; face3.bboxW=det3.w; face3.bboxH=det3.h;
    face3.confidence=0.9f; face3.detectionConfidence=0.9f;
    face3.landmarks.resize(68);
    for (int i=0;i<68;++i) face3.landmarks[i]=HFVec2(det3.x+det3.w*0.5f + i, det3.y+det3.h*0.5f);
    face3.landmarks3D.resize(68);
    for (int i=0;i<68;++i) face3.landmarks3D[i]=HFVec3(face3.landmarks[i].x, face3.landmarks[i].y, 0);
    face3.pose.yaw=0; face3.pose.pitch=0; face3.pose.roll=0; face3.pose.tx=det3.x+det3.w/2; face3.pose.ty=det3.y+det3.h/2; face3.pose.tz=500; face3.pose.scale=1;

    // Frame 1: det1
    {
        std::vector<FaceDetection> dets={det1};
        std::vector<HFFaceData> faces={face1};
        std::vector<HFFaceData> tracked;
        HFResult r = tracker.Update(dets, faces, 1000000, tracked);
        CHECK(r==HF_RESULT_OK, "Update frame1 OK");
        CHECK(tracked.size()==1, "Tracked 1 face frame1");
        if (tracked.size()==1) {
            CHECK(tracked[0].trackingState==HFTrackingState::DETECTED, "First frame DETECTED");
            CHECK(tracked[0].id==0, "First ID 0");
        }
    }

    // Frame 2: det2 close to det1, should be same ID with TRACKED
    {
        std::vector<FaceDetection> dets={det2};
        std::vector<HFFaceData> faces={face2};
        std::vector<HFFaceData> tracked;
        HFResult r = tracker.Update(dets, faces, 2000000, tracked);
        CHECK(r==HF_RESULT_OK, "Update frame2 OK");
        CHECK(tracked.size()==1, "Tracked 1 face frame2");
        if (tracked.size()==1) {
            CHECK(tracked[0].id==0, "ID persistence frame2 same ID 0");
            CHECK(tracked[0].trackingState==HFTrackingState::TRACKED, "Second frame TRACKED");
            // Check smoothing: landmarks should be between face1 and face2, not exactly face2
            float diffFromFace2 = std::abs(tracked[0].landmarks[0].x - face2.landmarks[0].x);
            float diffFromFace1 = std::abs(tracked[0].landmarks[0].x - face1.landmarks[0].x);
            CHECK(diffFromFace2>0.01f, "Landmark smoothing not exactly current");
            CHECK(diffFromFace1>0.01f, "Landmark smoothing not exactly previous");
            // Pose smoothing
            CHECK(tracked[0].pose.yaw != face2.pose.yaw || tracked[0].pose.pitch != face2.pose.pitch, "Pose smoothing applied");
        }
    }

    // Frame 3: det3 far away, should be new ID
    {
        std::vector<FaceDetection> dets={det3};
        std::vector<HFFaceData> faces={face3};
        std::vector<HFFaceData> tracked;
        HFResult r = tracker.Update(dets, faces, 3000000, tracked);
        CHECK(r==HF_RESULT_OK, "Update frame3 far face OK");
        CHECK(tracked.size()==1, "Tracked 1 face frame3");
        if (tracked.size()==1) {
            CHECK(tracked[0].id==1, "Far face new ID 1");
            CHECK(tracked[0].trackingState==HFTrackingState::DETECTED, "Far face DETECTED");
        }
    }

    // Frame 4: no face, should handle disappearance
    {
        std::vector<FaceDetection> dets={};
        std::vector<HFFaceData> faces={};
        std::vector<HFFaceData> tracked;
        HFResult r = tracker.Update(dets, faces, 4000000, tracked);
        CHECK(r==HF_RESULT_OK, "Update no face OK");
        CHECK(tracked.size()==0, "No face tracked when empty");
        // Internal should keep lost faces for maxLostFrames
    }

    // Frame 5: reappearance of det1 after 1 frame lost, should be same ID if within maxLostFrames
    {
        std::vector<FaceDetection> dets={det1};
        std::vector<HFFaceData> faces={face1};
        std::vector<HFFaceData> tracked;
        HFResult r = tracker.Update(dets, faces, 5000000, tracked);
        CHECK(r==HF_RESULT_OK, "Update reappearance OK");
        CHECK(tracked.size()==1, "Tracked 1 face reappearance");
        if (tracked.size()==1) {
            // Should be ID 0 again (since we had ID 0 lost for 2 frames, still within maxLostFrames=10)
            CHECK(tracked[0].id==0, "Reappearance same ID 0");
            CHECK(tracked[0].trackingState==HFTrackingState::REAPPEARED, "Reappearance state REAPPEARED");
        }
    }

    // Test multi-face tracking
    {
        tracker.Shutdown();
        tracker.Init(config);
        std::vector<FaceDetection> dets={det1, det3};
        std::vector<HFFaceData> faces={face1, face3};
        std::vector<HFFaceData> tracked;
        HFResult r = tracker.Update(dets, faces, 6000000, tracked);
        CHECK(r==HF_RESULT_OK, "Multi-face tracking OK");
        CHECK(tracked.size()==2, "Multi-face 2 tracked");
        if (tracked.size()==2) {
            CHECK(tracked[0].id != tracked[1].id, "Multi-face IDs unique");
        }
    }

    tracker.Shutdown();
    std::cout << "  Checks: " << passed << "/" << checks << std::endl;
    return passed==checks;
}
