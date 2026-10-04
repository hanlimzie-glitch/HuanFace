/**
 * HuanFace Tests Main — Phase 7 Full Beauty & Face Retouching Engine
 */

#include <iostream>
#include <vector>
#include <string>
#include <functional>

struct TestCase {
    std::string name;
    std::function<bool()> func;
};

extern bool TestFrame();
extern bool TestBundle();
extern bool TestRendering();
extern bool TestEngine();
extern bool TestImage();
extern bool TestFace();
extern bool TestMask();
extern bool TestShader();
extern bool TestTexture();
extern bool TestIntegration();
extern bool TestProductionTracker();
extern bool TestFaceMesh();
extern bool TestPose();
extern bool TestTracking();
extern bool TestCoordinate();
extern bool TestRealMLPipeline();
bool TestRealML() { return TestRealMLPipeline(); }
extern bool TestMakeup();
extern int RunBeautyTests();
bool TestBeauty() { return RunBeautyTests()==0; }

int main() {
    std::cout << "=== HuanFace SDK Phase 7 Tests ===" << std::endl;
    std::cout << "Build: " << __DATE__ << " " << __TIME__ << std::endl;
#ifdef _WIN32
    std::cout << "Platform: Windows" << std::endl;
#else
    std::cout << "Platform: Linux (CI)" << std::endl;
#endif

    std::vector<TestCase> tests = {
        {"Frame System", TestFrame},
        {"Bundle System", TestBundle},
        {"Rendering System", TestRendering},
        {"Engine System", TestEngine},
        {"Image Loader", TestImage},
        {"Face Tracker (Simple)", TestFace},
        {"Face Mask", TestMask},
        {"Shader", TestShader},
        {"Texture", TestTexture},
        {"Integration", TestIntegration},
        {"Production Tracker", TestProductionTracker},
        {"Face Mesh", TestFaceMesh},
        {"Pose", TestPose},
        {"Tracking State", TestTracking},
        {"Coordinate Transform", TestCoordinate},
        {"Real ML Pipeline", TestRealML},
        {"Makeup Renderer", TestMakeup},
        {"Beauty Engine", TestBeauty},
    };

    int passed = 0;
    int failed = 0;

    for (auto& t : tests) {
        std::cout << "\n--- Running: " << t.name << " ---" << std::endl;
        try {
            bool result = t.func();
            if (result) {
                std::cout << "[PASS] " << t.name << std::endl;
                passed++;
            } else {
                std::cout << "[FAIL] " << t.name << std::endl;
                failed++;
            }
        } catch (const std::exception& e) {
            std::cout << "[EXCEPTION] " << t.name << " : " << e.what() << std::endl;
            failed++;
        } catch (...) {
            std::cout << "[EXCEPTION] " << t.name << " : unknown" << std::endl;
            failed++;
        }
    }

    std::cout << "\n=== Summary ===" << std::endl;
    std::cout << "Passed: " << passed << "/" << tests.size() << std::endl;
    std::cout << "Failed: " << failed << "/" << tests.size() << std::endl;

    if (failed == 0) {
        std::cout << "\nAll tests PASSED" << std::endl;
        return 0;
    } else {
        std::cout << "\nSome tests FAILED" << std::endl;
        return 1;
    }
}
