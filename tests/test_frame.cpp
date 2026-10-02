/**
 * HuanFace Frame Tests — Phase 3
 * Tests frame creation, validation, format handling, rotation, CPU/GPU frame
 */

#include "../sdk/include/huanface_c_api.h"
#include "../sdk/src/frame/frame.h"
#include <iostream>
#include <vector>
#include <cstring>

using namespace huanface;

bool TestFrame() {
    int passed = 0;
    int failed = 0;

    auto check = [&](bool cond, const std::string& msg) {
        if (cond) {
            std::cout << "  [PASS] " << msg << std::endl;
            passed++;
        } else {
            std::cout << "  [FAIL] " << msg << std::endl;
            failed++;
        }
    };

    // Test 1: Valid RGBA8 frame
    {
        HFFrameC frame = {};
        frame.width = 1280;
        frame.height = 720;
        frame.format = HF_FORMAT_RGBA8;
        frame.stride = 1280 * 4;
        std::vector<uint8_t> dummy(1280*720*4, 0);
        frame.data = dummy.data();
        frame.rotation = 0;
        HFResult res = FrameValidator::Validate(&frame);
        check(res == HF_RESULT_OK, "Valid RGBA8 frame");
    }

    // Test 2: Valid BGRA8 frame
    {
        HFFrameC frame = {};
        frame.width = 1920;
        frame.height = 1080;
        frame.format = HF_FORMAT_BGRA8;
        frame.stride = 1920 * 4;
        uint8_t dummy[100] = {0};
        frame.data = dummy;
        frame.rotation = 0;
        HFResult res = FrameValidator::Validate(&frame);
        check(res == HF_RESULT_OK, "Valid BGRA8 frame 1080p");
    }

    // Test 3: Valid RGB8 frame
    {
        HFFrameC frame = {};
        frame.width = 640;
        frame.height = 480;
        frame.format = HF_FORMAT_RGB8;
        frame.stride = 640 * 3;
        uint8_t dummy[100] = {0};
        frame.data = dummy;
        frame.rotation = 0;
        HFResult res = FrameValidator::Validate(&frame);
        check(res == HF_RESULT_OK, "Valid RGB8 frame");
    }

    // Test 4: Valid NV12 frame
    {
        HFFrameC frame = {};
        frame.width = 1280;
        frame.height = 720;
        frame.format = HF_FORMAT_NV12;
        frame.stride = 1280;
        frame.strideU = 1280;
        std::vector<uint8_t> yPlane(1280*720, 0);
        std::vector<uint8_t> uvPlane(1280*720/2, 0);
        frame.data = yPlane.data();
        frame.dataU = uvPlane.data();
        frame.rotation = 0;
        HFResult res = FrameValidator::Validate(&frame);
        check(res == HF_RESULT_OK, "Valid NV12 frame");
    }

    // Test 5: Valid R8 frame (mask)
    {
        HFFrameC frame = {};
        frame.width = 256;
        frame.height = 256;
        frame.format = HF_FORMAT_R8;
        frame.stride = 256;
        std::vector<uint8_t> dummy(256*256, 0);
        frame.data = dummy.data();
        HFResult res = FrameValidator::Validate(&frame);
        check(res == HF_RESULT_OK, "Valid R8 mask frame");
    }

    // Test 6: Valid R32F frame
    {
        HFFrameC frame = {};
        frame.width = 128;
        frame.height = 128;
        frame.format = HF_FORMAT_R32F;
        frame.stride = 128 * 4;
        std::vector<uint8_t> dummy(128*128*4, 0);
        frame.data = dummy.data();
        HFResult res = FrameValidator::Validate(&frame);
        check(res == HF_RESULT_OK, "Valid R32F frame");
    }

    // Test 7: Invalid frame null
    {
        HFResult res = FrameValidator::Validate(nullptr);
        check(res == HF_RESULT_INVALID_PARAM, "Invalid null frame");
    }

    // Test 8: Zero dimensions
    {
        HFFrameC frame = {};
        frame.width = 0;
        frame.height = 720;
        frame.format = HF_FORMAT_RGBA8;
        frame.stride = 0;
        uint8_t dummy[100] = {0};
        frame.data = dummy;
        HFResult res = FrameValidator::Validate(&frame);
        check(res == HF_RESULT_INVALID_PARAM, "Invalid zero width");
    }

    // Test 9: Invalid stride
    {
        HFFrameC frame = {};
        frame.width = 1280;
        frame.height = 720;
        frame.format = HF_FORMAT_RGBA8;
        frame.stride = 100; // too small, needs 1280*4=5120
        uint8_t dummy[100] = {0};
        frame.data = dummy;
        HFResult res = FrameValidator::Validate(&frame);
        check(res == HF_RESULT_INVALID_PARAM, "Invalid stride too small for RGBA8");
    }

    // Test 10: Invalid format
    {
        HFFrameC frame = {};
        frame.width = 1280;
        frame.height = 720;
        frame.format = HF_FORMAT_UNKNOWN;
        frame.stride = 1280*4;
        uint8_t dummy[100] = {0};
        frame.data = dummy;
        HFResult res = FrameValidator::Validate(&frame);
        check(res == HF_RESULT_INVALID_PARAM, "Invalid UNKNOWN format");
    }

    // Test 11: No CPU nor GPU
    {
        HFFrameC frame = {};
        frame.width = 1280;
        frame.height = 720;
        frame.format = HF_FORMAT_RGBA8;
        frame.stride = 1280*4;
        frame.data = nullptr;
        frame.gpuTexture = nullptr;
        HFResult res = FrameValidator::Validate(&frame);
        check(res == HF_RESULT_INVALID_PARAM, "Invalid no CPU nor GPU");
    }

    // Test 12: Rotation handling
    {
        HFFrameC frame = {};
        frame.width = 1280;
        frame.height = 720;
        frame.format = HF_FORMAT_RGBA8;
        frame.stride = 1280*4;
        uint8_t dummy[100] = {0};
        frame.data = dummy;
        frame.rotation = 90;
        HFResult res = FrameValidator::Validate(&frame);
        check(res == HF_RESULT_OK, "Valid rotation 90");

        frame.rotation = 180;
        res = FrameValidator::Validate(&frame);
        check(res == HF_RESULT_OK, "Valid rotation 180");

        frame.rotation = 270;
        res = FrameValidator::Validate(&frame);
        check(res == HF_RESULT_OK, "Valid rotation 270");

        frame.rotation = 45;
        res = FrameValidator::Validate(&frame);
        check(res == HF_RESULT_INVALID_PARAM, "Invalid rotation 45");
    }

    // Test 13: GPU frame
    {
        HFFrameC frame = {};
        frame.width = 1280;
        frame.height = 720;
        frame.format = HF_FORMAT_BGRA8;
        frame.gpuTexture = (void*)0x1234; // dummy non-null
        frame.rotation = 0;
        HFResult res = FrameValidator::Validate(&frame);
        check(res == HF_RESULT_OK, "Valid GPU frame");
    }

    // Test 14: Format to string
    {
        std::string s = FrameValidator::FormatToString(HF_FORMAT_RGBA8);
        check(s == "RGBA8", "FormatToString RGBA8");
        s = FrameValidator::FormatToString(HF_FORMAT_NV12);
        check(s == "NV12", "FormatToString NV12");
    }

    // Test 15: Bytes per pixel
    {
        check(FrameValidator::GetBytesPerPixel(HF_FORMAT_RGBA8) == 4, "BPP RGBA8=4");
        check(FrameValidator::GetBytesPerPixel(HF_FORMAT_RGB8) == 3, "BPP RGB8=3");
        check(FrameValidator::GetBytesPerPixel(HF_FORMAT_R8) == 1, "BPP R8=1");
    }

    std::cout << "  Frame tests: " << passed << " passed, " << failed << " failed" << std::endl;
    return failed == 0;
}
