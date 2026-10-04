/**
 * HuanFace Engine Tests — Phase 3
 * Tests create, load bundle, process frame, destroy
 */

#include "../sdk/include/huanface_c_api.h"
#include "../sdk/src/frame/frame.h"
#include <iostream>
#include <filesystem>
#include <vector>
#include <cstring>

namespace fs = std::filesystem;

bool TestEngine() {
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

    // Test 1: Initialization
    {
        HFResult res = HF_Init();
        check(res == HF_RESULT_OK, "HF_Init");
        
        // Double init should be OK
        res = HF_Init();
        check(res == HF_RESULT_OK, "HF_Init double (should be OK)");
    }

    // Test 2: Engine creation
    HFEngine engine = nullptr;
    {
        HFEngineConfigC config = {};
        config.backendType = HF_RENDER_BACKEND_AUTO;
        config.width = 1280;
        config.height = 720;
        config.maxFaces = 4;
        config.faceTrackerType = "mediapipe";
        
        HFResult res = HF_CreateEngine(&config, &engine);
        check(res == HF_RESULT_OK && engine != nullptr, "HF_CreateEngine");
    }

    // Test 3: Engine creation with null config (should use default)
    {
        HFEngine engine2 = nullptr;
        HFResult res = HF_CreateEngine(nullptr, &engine2);
        check(res == HF_RESULT_OK && engine2 != nullptr, "HF_CreateEngine with null config (default)");
        if (engine2) {
            HF_DestroyEngine(engine2);
        }
    }

    // Test 4: Invalid arguments
    {
        HFResult res = HF_CreateEngine(nullptr, nullptr);
        check(res == HF_RESULT_INVALID_PARAM, "HF_CreateEngine null outEngine should fail");

        res = HF_DestroyEngine(nullptr);
        check(res == HF_RESULT_INVALID_PARAM, "HF_DestroyEngine null should fail");
    }

    // Test 5: Load bundle (directory mode)
    HFBundle bundle = nullptr;
    std::string bundlePath;
    {
        std::vector<std::string> possiblePaths = {
            "examples/bundles/simple_lip",
            "../examples/bundles/simple_lip",
            "/home/user/HuanFace/examples/bundles/simple_lip"
        };
        for (auto& p : possiblePaths) {
            if (fs::is_directory(p)) {
                bundlePath = p;
                break;
            }
        }
        if (!bundlePath.empty() && engine) {
            HFResult res = HF_LoadBundle(engine, bundlePath.c_str(), &bundle);
            check(res == HF_RESULT_OK && bundle != nullptr, "HF_LoadBundle directory: " + bundlePath);
        } else {
            std::cout << "  [SKIP] Bundle path not found for load test" << std::endl;
        }
    }

    // Test 6: Set parameters
    if (engine) {
        HFResult res = HF_SetParameterFloat(engine, "intensity_lip", 0.8f);
        check(res == HF_RESULT_OK, "HF_SetParameterFloat intensity_lip=0.8");

        res = HF_SetParameterFloat(engine, "HeavyBlur", 50.0f);
        check(res == HF_RESULT_OK, "HF_SetParameterFloat HeavyBlur=50");

        HFColorC color = {1.0f, 0.2f, 0.3f, 1.0f};
        res = HF_SetParameterColor(engine, "lip_color", color);
        check(res == HF_RESULT_OK, "HF_SetParameterColor lip_color");

        res = HF_SetParameterInt(engine, "lip_type", 1);
        check(res == HF_RESULT_OK, "HF_SetParameterInt lip_type");

        res = HF_SetParameterBool(engine, "is_makeup_on", 1);
        check(res == HF_RESULT_OK, "HF_SetParameterBool is_makeup_on");

        res = HF_SetParameterEnum(engine, "blend_type", "normal");
        check(res == HF_RESULT_OK, "HF_SetParameterEnum blend_type=normal");

        // Get parameter
        float outVal = 0;
        res = HF_GetParameterFloat(engine, "intensity_lip", &outVal);
        check(res == HF_RESULT_OK && outVal == 0.8f, "HF_GetParameterFloat intensity_lip=0.8");

        HFColorC outColor;
        res = HF_GetParameterColor(engine, "lip_color", &outColor);
        check(res == HF_RESULT_OK && outColor.r == 1.0f, "HF_GetParameterColor lip_color");
    }

    // Test 7: Process frame
    if (engine) {
        // Create test frame RGBA8 1280x720
        HFFrameC input = {};
        input.width = 1280;
        input.height = 720;
        input.format = HF_FORMAT_RGBA8;
        input.stride = 1280 * 4;
        input.timestampNanos = 123456789;
        input.rotation = 0;
        // Allocate dummy data
        size_t dataSize = (size_t)1280 * 720 * 4;
        uint8_t* dummyData = new uint8_t[dataSize];
        memset(dummyData, 128, dataSize);
        input.data = dummyData;
        input.ownsData = 0; // we own

        HFFrameC output = {};
        HFResult res = HF_ProcessFrame(engine, &input, &output);
        check(res == HF_RESULT_OK, "HF_ProcessFrame RGBA8 1280x720");

        if (res == HF_RESULT_OK) {
            check(output.width == 1280, "Output width 1280");
            check(output.height == 720, "Output height 720");
            check(output.format == HF_FORMAT_RGBA8, "Output format RGBA8");
        }

        // Test with bundle
        if (bundle) {
            HFFrameC output2 = {};
            res = HF_ProcessFrameWithBundle(engine, &input, bundle, &output2);
            check(res == HF_RESULT_OK, "HF_ProcessFrameWithBundle");
        }

        delete[] dummyData;

        // Test GetFaceData (should be 0 faces for stub)
        HFTrackingDataC tracking = {};
        res = HF_GetFaceData(engine, &tracking);
        check(res == HF_RESULT_OK, "HF_GetFaceData");
        if (res == HF_RESULT_OK) {
            check(tracking.faceCount == 0, "Face count 0 for stub engine");
            HF_FreeFaceData(&tracking);
            check(true, "HF_FreeFaceData");
        }
    }

    // Test 8: Process frame with invalid args
    if (engine) {
        HFResult res = HF_ProcessFrame(engine, nullptr, nullptr);
        check(res == HF_RESULT_INVALID_PARAM, "HF_ProcessFrame null args should fail");

        HFFrameC input = {};
        input.width = 0;
        input.height = 0;
        input.format = HF_FORMAT_UNKNOWN;
        HFFrameC output = {};
        res = HF_ProcessFrame(engine, &input, &output);
        check(res == HF_RESULT_INVALID_PARAM, "HF_ProcessFrame invalid frame should fail");
    }

    // Test 9: Unload bundle
    if (engine && bundle) {
        HFResult res = HF_UnloadBundle(engine, bundle);
        check(res == HF_RESULT_OK, "HF_UnloadBundle");
        bundle = nullptr;
    }

    // Test 10: Destroy engine
    if (engine) {
        HFResult res = HF_DestroyEngine(engine);
        check(res == HF_RESULT_OK, "HF_DestroyEngine");
        engine = nullptr;
    }

    // Test 11: Shutdown
    {
        HFResult res = HF_Shutdown();
        check(res == HF_RESULT_OK, "HF_Shutdown");

        // Double shutdown should return NOT_INITIALIZED
        res = HF_Shutdown();
        check(res == HF_RESULT_NOT_INITIALIZED, "HF_Shutdown double should be NOT_INITIALIZED");
    }

    // Test 12: Version and result string
    {
        const char* ver = HF_GetVersion();
        check(ver != nullptr && strlen(ver) > 0, std::string("HF_GetVersion: ") + (ver ? ver : "null"));

        const char* okStr = HF_GetResultString(HF_RESULT_OK);
        check(okStr != nullptr && std::string(okStr) == "OK", "HF_GetResultString OK");

        const char* invalidStr = HF_GetResultString(HF_RESULT_INVALID_PARAM);
        check(invalidStr != nullptr, "HF_GetResultString INVALID_PARAM");
    }

    std::cout << "  Engine tests: " << passed << " passed, " << failed << " failed" << std::endl;
    return failed == 0;
}
