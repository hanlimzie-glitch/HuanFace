/**
 * Test Pose — Phase 5
 * yaw range, pitch range, roll range, not hardcoded
 */

#include "../sdk/src/face/pose_estimator.h"
#include "../sdk/src/face/face_detector.h"
#include "../sdk/src/face/landmark_estimator.h"
#include <iostream>
#include <vector>
#include <cmath>

using namespace huanface;

static int checks=0, passed=0;
static void CHECK(bool cond, const char* msg) { checks++; if (cond) passed++; else std::cout << "  FAIL: " << msg << std::endl; }

bool TestPose() {
    std::cout << "=== Test Pose Phase 5 ===" << std::endl;
    checks=0; passed=0;

    HFEngineConfigC config{};
    ProductionFaceDetector detector;
    ProductionLandmarkEstimator lmEst;
    ProductionPoseEstimator poseEst;

    detector.Init(config);
    lmEst.Init(config);
    poseEst.Init(config);

    int w=200,h=200,stride=w*4;
    std::vector<uint8_t> rgba(w*h*4);
    for (int y=0;y<h;++y) for (int x=0;x<w;++x) {
        uint8_t* px = rgba.data()+y*stride+x*4;
        px[0]=30; px[1]=30; px[2]=60; px[3]=255;
    }
    int fx=40,fy=40,fw=120,fh=120;
    for (int y=fy;y<fy+fh;++y) for (int x=fx;x<fx+fw;++x) {
        uint8_t* px = rgba.data()+y*stride+x*4;
        px[0]=220; px[1]=180; px[2]=160; px[3]=255;
    }
    for (int dy=-3;dy<=3;++dy) for (int dx=-3;dx<=3;++dx) {
        int x1=fx+fw*35/100+dx, y1=fy+fh*40/100+dy;
        int x2=fx+fw*65/100+dx, y2=fy+fh*40/100+dy;
        if (x1>=0&&x1<w&&y1>=0&&y1<h) { uint8_t* px=rgba.data()+y1*stride+x1*4; px[0]=20;px[1]=20;px[2]=20; }
        if (x2>=0&&x2<w&&y2>=0&&y2<h) { uint8_t* px=rgba.data()+y2*stride+x2*4; px[0]=20;px[1]=20;px[2]=20; }
    }
    for (int dy=-2;dy<=2;++dy) for (int dx=-8;dx<=8;++dx) {
        int x=fx+fw/2+dx, y=fy+fh*75/100+dy;
        if (x>=0&&x<w&&y>=0&&y<h) { uint8_t* px=rgba.data()+y*stride+x*4; px[0]=180;px[1]=50;px[2]=50; }
    }

    std::vector<FaceDetection> dets;
    detector.Detect(rgba.data(), w,h,stride, dets);
    CHECK(!dets.empty(), "Detect face for pose test");
    if (dets.empty()) { std::cout << "  Checks: " << passed << "/" << checks << std::endl; return false; }

    FaceDetection det = dets[0];
    std::vector<HFVec2> lms; std::vector<HFVec3> lms3d; std::vector<float> confs;
    lmEst.Estimate(rgba.data(), w,h,stride, det, lms, lms3d, confs);
    CHECK(lms.size()>=68, "Landmarks for pose");

    HFFacePose pose;
    HFResult r = poseEst.Estimate(lms, det, w,h, pose);
    CHECK(r==HF_RESULT_OK, "Pose estimate OK");
    CHECK(pose.IsValid(), "Pose IsValid no NaN");
    CHECK(pose.yaw>=-90 && pose.yaw<=90, "Yaw in [-90,90]");
    CHECK(pose.pitch>=-90 && pose.pitch<=90, "Pitch in [-90,90]");
    CHECK(pose.roll>=-180 && pose.roll<=180, "Roll in [-180,180]");
    CHECK(std::isfinite(pose.tx) && std::isfinite(pose.ty) && std::isfinite(pose.tz), "Translation finite");
    CHECK(pose.scale>0, "Scale >0");

    // Check not hardcoded: process two different faces should give different pose if geometry differs
    // Create second face with eyes shifted (simulate yaw)
    // For frontal face, yaw should be near 0
    CHECK(std::abs(pose.yaw) < 30, "Frontal face yaw near 0 (<30)");
    CHECK(std::abs(pose.pitch) < 30, "Frontal face pitch near 0 (<30)");
    CHECK(std::abs(pose.roll) < 20, "Frontal face roll near 0 (<20)");

    // Test that pose changes with eye asymmetry (simulate right turn)
    std::vector<HFVec2> lmsYaw = lms;
    // Move nose to right to simulate yaw right
    for (int i=27;i<=35;++i) { lmsYaw[i].x += 10; }
    HFFacePose poseYaw;
    poseEst.Estimate(lmsYaw, det, w,h, poseYaw);
    CHECK(poseYaw.yaw != pose.yaw, "Yaw changes with nose position (not hardcoded)");
    // Yaw should increase when nose moves right
    CHECK(poseYaw.yaw > pose.yaw, "Yaw increases when nose moves right");

    detector.Shutdown();
    lmEst.Shutdown();
    poseEst.Shutdown();

    std::cout << "  Checks: " << passed << "/" << checks << std::endl;
    return passed==checks;
}
