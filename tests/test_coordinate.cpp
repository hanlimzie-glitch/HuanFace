/**
 * Test Coordinate Transform — Phase 5
 * normal, rotated, mirrored, rotated+mirrored
 */

#include "../sdk/src/face/coordinate_transform.h"
#include <iostream>
#include <cmath>

using namespace huanface;

static int checks=0, passed=0;
static void CHECK(bool cond, const char* msg) { checks++; if (cond) passed++; else std::cout << "  FAIL: " << msg << std::endl; }

bool TestCoordinate() {
    std::cout << "=== Test Coordinate Transform Phase 5 ===" << std::endl;
    checks=0; passed=0;

    int w=200,h=200;
    HFVec2 pt(50,60);

    // Pixel <-> Normalized
    {
        HFVec2 norm = CoordinateTransform::PixelToNormalized(pt, w,h);
        CHECK(std::abs(norm.x - 0.25f) < 0.001f, "PixelToNormalized x");
        CHECK(std::abs(norm.y - 0.3f) < 0.001f, "PixelToNormalized y");
        HFVec2 pix = CoordinateTransform::NormalizedToPixel(norm, w,h);
        CHECK(std::abs(pix.x - pt.x) < 0.001f, "NormalizedToPixel x");
        CHECK(std::abs(pix.y - pt.y) < 0.001f, "NormalizedToPixel y");
    }

    // UV to D3D11 NDC
    {
        HFVec2UV uv(0.5f,0.5f);
        HFVec2 ndc = CoordinateTransform::UVToD3D11NDC(uv);
        CHECK(std::abs(ndc.x - 0.0f) < 0.001f, "UVToD3D11NDC center x 0");
        CHECK(std::abs(ndc.y - 0.0f) < 0.001f, "UVToD3D11NDC center y 0");
        HFVec2UV uv2(0,0);
        HFVec2 ndc2 = CoordinateTransform::UVToD3D11NDC(uv2);
        CHECK(std::abs(ndc2.x - (-1.0f)) < 0.001f, "UVToD3D11NDC top-left x -1");
        CHECK(std::abs(ndc2.y - 1.0f) < 0.001f, "UVToD3D11NDC top-left y 1");
    }

    // Mirror
    {
        HFVec2 mirrored = CoordinateTransform::MirrorHorizontal(pt, w);
        CHECK(std::abs(mirrored.x - (w-1-pt.x)) < 0.001f, "MirrorHorizontal x");
        CHECK(std::abs(mirrored.y - pt.y) < 0.001f, "MirrorHorizontal y unchanged");
        // Double mirror should return original
        HFVec2 doubleMirrored = CoordinateTransform::MirrorHorizontal(mirrored, w);
        CHECK(std::abs(doubleMirrored.x - pt.x) < 0.001f, "Double mirror returns original");
    }

    // Rotation 0
    {
        HFVec2 rot0 = CoordinateTransform::RotatePoint(pt, w,h, 0);
        CHECK(std::abs(rot0.x - pt.x) < 0.001f && std::abs(rot0.y - pt.y) < 0.001f, "Rotate 0");
    }

    // Rotation 90
    {
        HFVec2 rot90 = CoordinateTransform::RotatePoint(pt, w,h, 90);
        HFVec2 expected(h-1-pt.y, pt.x);
        CHECK(std::abs(rot90.x - expected.x) < 0.001f && std::abs(rot90.y - expected.y) < 0.001f, "Rotate 90");
    }

    // Rotation 180
    {
        HFVec2 rot180 = CoordinateTransform::RotatePoint(pt, w,h, 180);
        HFVec2 expected(w-1-pt.x, h-1-pt.y);
        CHECK(std::abs(rot180.x - expected.x) < 0.001f && std::abs(rot180.y - expected.y) < 0.001f, "Rotate 180");
    }

    // Rotation 270
    {
        HFVec2 rot270 = CoordinateTransform::RotatePoint(pt, w,h, 270);
        HFVec2 expected(pt.y, w-1-pt.x);
        CHECK(std::abs(rot270.x - expected.x) < 0.001f && std::abs(rot270.y - expected.y) < 0.001f, "Rotate 270");
    }

    // Rotated + mirrored
    {
        HFVec2 mirrored = CoordinateTransform::MirrorHorizontal(pt, w);
        HFVec2 rotMirrored = CoordinateTransform::RotatePoint(mirrored, w,h, 90);
        // Should be consistent
        CHECK(std::isfinite(rotMirrored.x) && std::isfinite(rotMirrored.y), "Rotated+mirrored finite");
        // Transform back: rotate -90 then mirror
        HFVec2 backRot = CoordinateTransform::RotatePoint(rotMirrored, h,w, 270); // inverse of 90
        HFVec2 back = CoordinateTransform::MirrorHorizontal(backRot, w);
        CHECK(std::abs(back.x - pt.x) < 0.001f && std::abs(back.y - pt.y) < 0.001f, "Rotated+mirrored invertible");
    }

    // Landmark transform
    {
        std::vector<HFVec2> lms = {HFVec2(50,60), HFVec2(100,80)};
        std::vector<HFVec2> lmsCopy = lms;
        CoordinateTransform::TransformLandmarks(lms, w,h, 0, false);
        CHECK(lms[0].x==lmsCopy[0].x && lms[0].y==lmsCopy[0].y, "TransformLandmarks no transform");

        std::vector<HFVec2> lms2 = lmsCopy;
        CoordinateTransform::TransformLandmarks(lms2, w,h, 90, false);
        CHECK(lms2[0].x != lmsCopy[0].x || lms2[0].y != lmsCopy[0].y, "TransformLandmarks rotated changes");

        std::vector<HFVec2> lms3 = lmsCopy;
        CoordinateTransform::TransformLandmarks(lms3, w,h, 0, true);
        CHECK(lms3[0].x == w-1-lmsCopy[0].x, "TransformLandmarks mirrored");
    }

    // BBox rotation
    {
        float x=10,y=20,wb=50,hb=30;
        CoordinateTransform::RotateBBox(x,y,wb,hb, w,h, 0);
        CHECK(x==10 && y==20, "RotateBBox 0 unchanged");

        float x2=10,y2=20,wb2=50,hb2=30;
        CoordinateTransform::RotateBBox(x2,y2,wb2,hb2, w,h, 90);
        CHECK(wb2==30 && hb2==50, "RotateBBox 90 swaps w/h");
    }

    std::cout << "  Checks: " << passed << "/" << checks << std::endl;
    return passed==checks;
}
