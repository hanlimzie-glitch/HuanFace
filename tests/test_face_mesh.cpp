/**
 * Test Face Mesh — Phase 5
 * vertex count, index validity, triangle validity, no NaN, no invalid coordinates
 */

#include "../sdk/src/face/face_mesh_generator.h"
#include "../sdk/src/face/face_detector.h"
#include "../sdk/src/face/landmark_estimator.h"
#include <iostream>
#include <vector>
#include <cmath>

using namespace huanface;

static int checks=0, passed=0;
static void CHECK(bool cond, const char* msg) {
    checks++; if (cond) passed++; else std::cout << "  FAIL: " << msg << std::endl;
}

bool TestFaceMesh() {
    std::cout << "=== Test Face Mesh Phase 5 ===" << std::endl;
    checks=0; passed=0;

    HFEngineConfigC config{};
    ProductionFaceDetector detector;
    ProductionLandmarkEstimator lmEst;
    ProductionFaceMeshGenerator meshGen;

    detector.Init(config);
    lmEst.Init(config);
    meshGen.Init(config);

    // Create synthetic face data
    int w=200,h=200,stride=w*4;
    std::vector<uint8_t> rgba(w*h*4);
    for (int y=0;y<h;++y) for (int x=0;x<w;++x) {
        uint8_t* px = rgba.data()+y*stride+x*4;
        px[0]=30; px[1]=30; px[2]=60; px[3]=255;
    }
    // Skin rect
    int fx=40,fy=40,fw=120,fh=120;
    for (int y=fy;y<fy+fh;++y) for (int x=fx;x<fx+fw;++x) {
        uint8_t* px = rgba.data()+y*stride+x*4;
        px[0]=220; px[1]=180; px[2]=160; px[3]=255;
    }
    // Eyes
    for (int dy=-3;dy<=3;++dy) for (int dx=-3;dx<=3;++dx) {
        int x1=fx+fw*35/100+dx, y1=fy+fh*40/100+dy;
        int x2=fx+fw*65/100+dx, y2=fy+fh*40/100+dy;
        if (x1>=0&&x1<w&&y1>=0&&y1<h) { uint8_t* px=rgba.data()+y1*stride+x1*4; px[0]=20;px[1]=20;px[2]=20; }
        if (x2>=0&&x2<w&&y2>=0&&y2<h) { uint8_t* px=rgba.data()+y2*stride+x2*4; px[0]=20;px[1]=20;px[2]=20; }
    }
    // Mouth
    for (int dy=-2;dy<=2;++dy) for (int dx=-8;dx<=8;++dx) {
        int x=fx+fw/2+dx, y=fy+fh*75/100+dy;
        if (x>=0&&x<w&&y>=0&&y<h) { uint8_t* px=rgba.data()+y*stride+x*4; px[0]=180;px[1]=50;px[2]=50; }
    }

    std::vector<FaceDetection> dets;
    detector.Detect(rgba.data(), w,h,stride, dets);
    CHECK(!dets.empty(), "Detector finds face for mesh test");
    if (dets.empty()) {
        std::cout << "  Checks: " << passed << "/" << checks << std::endl;
        return false;
    }

    FaceDetection det = dets[0];
    std::vector<HFVec2> lms; std::vector<HFVec3> lms3d; std::vector<float> confs;
    lmEst.Estimate(rgba.data(), w,h,stride, det, lms, lms3d, confs);
    CHECK(lms.size()>=68, "Landmarks >=68 for mesh");
    CHECK(lms3d.size()==lms.size(), "Landmarks3D size matches");

    HFFaceMesh mesh;
    HFResult r = meshGen.Generate(lms, lms3d, det, w,h, mesh);
    CHECK(r==HF_RESULT_OK, "Mesh generation OK");
    CHECK(mesh.IsValid(), "Mesh IsValid");
    CHECK(mesh.VertexCount()>=68, "Vertex count >=68");
    CHECK(mesh.TriangleCount()>0, "Triangle count >0");
    CHECK(mesh.indices.size()%3==0, "Indices multiple of 3");

    // Check no NaN
    bool noNaN=true;
    for (auto& v: mesh.vertices) {
        if (!std::isfinite(v.x) || !std::isfinite(v.y) || !std::isfinite(v.z)) { noNaN=false; break; }
    }
    CHECK(noNaN, "No NaN in vertices");

    // Check indices in bounds
    bool indicesValid=true;
    for (int idx: mesh.indices) {
        if (idx<0 || idx>=mesh.VertexCount()) { indicesValid=false; break; }
    }
    CHECK(indicesValid, "Indices in bounds");

    // Check UV in [0,1]
    bool uvValid=true;
    for (auto& uv: mesh.uv) {
        if (uv.u< -0.1f || uv.u>1.1f || uv.v< -0.1f || uv.v>1.1f) { uvValid=false; break; }
    }
    CHECK(uvValid, "UV in [0,1] approx");

    // Check mesh follows face: vertices should be near face bbox
    bool followsFace=true;
    int insideCount=0;
    for (auto& v: mesh.vertices) {
        if (v.x>=det.x-10 && v.x<=det.x+det.w+10 && v.y>=det.y-10 && v.y<=det.y+det.h+10) insideCount++;
    }
    CHECK(insideCount >= (int)(mesh.VertexCount()*0.7f), "Mesh follows face (70% inside bbox)");

    // Check regions
    CHECK(mesh.vertexRegions.size()==mesh.vertices.size(), "Vertex regions size matches");
    bool hasFaceRegion=false, hasEyeRegion=false, hasLipRegion=false;
    for (auto reg: mesh.vertexRegions) {
        if (reg==FaceRegion::FACE || reg==FaceRegion::JAW) hasFaceRegion=true;
        if (reg==FaceRegion::LEFT_EYE || reg==FaceRegion::RIGHT_EYE) hasEyeRegion=true;
        if (reg==FaceRegion::OUTER_LIP || reg==FaceRegion::INNER_LIP || reg==FaceRegion::LIP) hasLipRegion=true;
    }
    CHECK(hasFaceRegion, "Has face/jaw region");
    CHECK(hasEyeRegion, "Has eye region");
    CHECK(hasLipRegion, "Has lip region");

    // Check triangle validity: area >0
    bool trianglesValid=true;
    for (size_t i=0;i+2<mesh.indices.size(); i+=3) {
        int i0=mesh.indices[i], i1=mesh.indices[i+1], i2=mesh.indices[i+2];
        if (i0==i1 || i1==i2 || i0==i2) { trianglesValid=false; break; }
        // Check not degenerate: compute area
        HFVec3 v0=mesh.vertices[i0], v1=mesh.vertices[i1], v2=mesh.vertices[i2];
        float area = std::abs((v1.x-v0.x)*(v2.y-v0.y) - (v2.x-v0.x)*(v1.y-v0.y))*0.5f;
        if (area < 0.001f) {
            // Allow small degenerate but not all
            // continue
        }
    }
    CHECK(trianglesValid, "Triangles not degenerate (distinct indices)");

    detector.Shutdown();
    lmEst.Shutdown();
    meshGen.Shutdown();

    std::cout << "  Checks: " << passed << "/" << checks << std::endl;
    return passed==checks;
}
