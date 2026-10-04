# SDK API Spec — HuanFace (Phase 2)

**Status:** SPECIFICATION — proposed C ABI + C++ RAII, no implementation in Phase 2  
**Target:** Windows 10+, x64, Native C/C++ DLL, no OBS dependency

---

## 1. Public API Overview

**C ABI (stable, for FFI):** `sdk/include/huanface_c_api.h`

**C++ RAII wrapper (header-only, optional):** `sdk/include/huanface.hpp`

**Why C ABI:**
- Stable across compilers (MSVC, Clang)
- FFI for other languages (C#, Python, Rust, etc.)
- No name mangling issues
- Explicit handle-based (HFEngine, HFBundle, etc.)

**C++ wrapper:**
- RAII (constructor/destructor, not manual Create/Destroy)
- Exceptions or result codes
- Easier for C++ clients

---

## 2. C ABI (Proposed)

### 2.1 Types

```c
// Opaque handles
typedef struct HFEngine_* HFEngine;
typedef struct HFBundle_* HFBundle;
typedef struct HFTexture_* HFTexture; // GPU texture wrapper

// Result codes
typedef enum HFResult {
    HF_RESULT_OK = 0,
    HF_RESULT_FAIL = 1,
    HF_RESULT_NOT_INITIALIZED = 2,
    HF_RESULT_INVALID_PARAM = 3,
    HF_RESULT_NOT_SUPPORTED = 4,
    HF_RESULT_OUT_OF_MEMORY = 5,
    HF_RESULT_FILE_NOT_FOUND = 6,
    HF_RESULT_BUNDLE_INVALID = 7,
    HF_RESULT_FACE_NOT_DETECTED = 8,
} HFResult;

// Render backend type
typedef enum HFRenderBackendType {
    HF_RENDER_BACKEND_AUTO = 0,
    HF_RENDER_BACKEND_D3D11 = 1,
    HF_RENDER_BACKEND_OPENGL = 2,
} HFRenderBackendType;

// Frame format
typedef enum HFFormat {
    HF_FORMAT_UNKNOWN = 0,
    HF_FORMAT_RGBA8 = 1,
    HF_FORMAT_BGRA8 = 2,
    HF_FORMAT_RGB8 = 3,
    HF_FORMAT_BGR8 = 4,
    HF_FORMAT_NV12 = 5,
    HF_FORMAT_YUV420P = 6,
    HF_FORMAT_R8 = 7,
    HF_FORMAT_R32F = 8,
} HFFormat;

// Param type
typedef enum HFParamType {
    HF_PARAM_TYPE_FLOAT = 0,
    HF_PARAM_TYPE_INT = 1,
    HF_PARAM_TYPE_BOOL = 2,
    HF_PARAM_TYPE_COLOR = 3,
    HF_PARAM_TYPE_VEC2 = 4,
    HF_PARAM_TYPE_VEC3 = 5,
    HF_PARAM_TYPE_VEC4 = 6,
    HF_PARAM_TYPE_TEXTURE = 7,
    HF_PARAM_TYPE_ENUM = 8,
} HFParamType;

// Frame (C version, plain)
typedef struct HFFrameC {
    int width;
    int height;
    HFFormat format;
    int64_t timestampNanos;
    uint8_t* data;
    uint8_t* dataU;
    uint8_t* dataV;
    int stride;
    int strideU;
    int strideV;
    void* gpuTexture; // ID3D11Texture2D* or GLuint as void*
    void* nativeHandle;
    int ownsData;
    int ownsGpuTexture;
    int rotation; // 0,90,180,270
    int isMirrored;
} HFFrameC;

// Face data (C version, simplified)
typedef struct HFFaceDataC {
    int id;
    float bboxX, bboxY, bboxW, bboxH;
    float confidence;
    int landmarkCount;
    float* landmarks; // 2D, [x0,y0,x1,y1,...] count*2 floats, allocated by SDK, caller must free via HF_FreeFaceData?
    float* landmarks3D; // 3D, [x0,y0,z0,...] count*3
    float rotationPitch, rotationYaw, rotationRoll;
    float translationX, translationY, translationZ;
    // Mesh, expression optional, via separate API
} HFFaceDataC;

typedef struct HFTrackingDataC {
    int faceCount;
    HFFaceDataC* faces; // array of faceCount, allocated by SDK
    int64_t timestampNanos;
} HFTrackingDataC;

// Engine config
typedef struct HFEngineConfigC {
    HFRenderBackendType backendType;
    void* windowHandle; // HWND
    int width;
    int height;
    int enableDebug;
    const char* faceTrackerType; // "mediapipe", "onnx", "dlib"
    int maxFaces;
    int detectSmallFace;
    float minFaceRatio;
    int faceLandmarkQuality;
    int faceDetectMode;
    int useAsyncAIInference;
    int enableFaceMeshV2;
} HFEngineConfigC;

// Color
typedef struct HFColorC {
    float r, g, b, a;
} HFColorC;
```

### 2.2 Functions

```c
// Lifecycle
HFResult HF_Init();
HFResult HF_Shutdown();

HFResult HF_CreateEngine(const HFEngineConfigC* config, HFEngine* outEngine);
HFResult HF_DestroyEngine(HFEngine engine);

// Bundle
HFResult HF_LoadBundle(HFEngine engine, const char* bundlePath, HFBundle* outBundle);
HFResult HF_LoadBundleFromMemory(HFEngine engine, const uint8_t* data, int dataSize, HFBundle* outBundle);
HFResult HF_UnloadBundle(HFEngine engine, HFBundle bundle);

// Parameters (generic)
// For makeup and beauty params
HFResult HF_SetParameterFloat(HFEngine engine, const char* name, float value);
HFResult HF_SetParameterInt(HFEngine engine, const char* name, int value);
HFResult HF_SetParameterBool(HFEngine engine, const char* name, int value); // 0/1
HFResult HF_SetParameterColor(HFEngine engine, const char* name, HFColorC color);
HFResult HF_SetParameterVec2(HFEngine engine, const char* name, float x, float y);
HFResult HF_SetParameterVec3(HFEngine engine, const char* name, float x, float y, float z);
HFResult HF_SetParameterVec4(HFEngine engine, const char* name, float x, float y, float z, float w);
HFResult HF_SetParameterTexture(HFEngine engine, const char* name, HFTexture texture);
HFResult HF_SetParameterEnum(HFEngine engine, const char* name, const char* enumValue);

HFResult HF_GetParameterFloat(HFEngine engine, const char* name, float* outValue);
HFResult HF_GetParameterInt(HFEngine engine, const char* name, int* outValue);
HFResult HF_GetParameterBool(HFEngine engine, const char* name, int* outValue);
HFResult HF_GetParameterColor(HFEngine engine, const char* name, HFColorC* outColor);
// ... etc.

// Process
HFResult HF_ProcessFrame(HFEngine engine, const HFFrameC* inputFrame, HFFrameC* outFrame);
// Or with bundle
HFResult HF_ProcessFrameWithBundle(HFEngine engine, const HFFrameC* inputFrame, HFBundle bundle, HFFrameC* outFrame);

// Face data
HFResult HF_GetFaceData(HFEngine engine, HFTrackingDataC* outTrackingData);
HFResult HF_FreeFaceData(HFTrackingDataC* trackingData); // free landmarks arrays

// Utility
const char* HF_GetVersion();
const char* HF_GetResultString(HFResult result);
```

**Notes:**
- `HF_Init` / `HF_Shutdown` global init/shutdown (e.g., COM init for MF, D3D11 device creation if AUTO)
- `HF_CreateEngine` creates engine with config, `HF_DestroyEngine` destroys
- `HF_LoadBundle` loads .hfbundle from file, `LoadBundleFromMemory` from memory (for embedded bundles)
- `HF_SetParameter*` generic, name is param name (e.g., "intensity_lip", "HeavyBlur")
- `HF_ProcessFrame` main processing: input HFFrame → face tracking → beauty → makeup → output HFFrame
- `HF_GetFaceData` gets last tracking data (for UI overlay, etc.)
- All functions return `HFResult` for error handling

**Memory ownership:**
- `HFFrameC` data: if `ownsData=1`, SDK owns and will free via `HF_FreeFrame`? Or caller owns? For C ABI, explicit free function needed
- `HFTrackingDataC` faces and landmarks: allocated by SDK, caller must free via `HF_FreeFaceData`
- `HFEngine`, `HFBundle`: opaque handles, created by SDK, destroyed via `HF_DestroyEngine`, `HF_UnloadBundle`

**Add free functions:**

```c
HFResult HF_FreeFrame(HFFrameC* frame);
```

---

## 3. C++ RAII Wrapper (Proposed, Header-Only)

```cpp
// sdk/include/huanface.hpp
#pragma once
#include "huanface_c_api.h"
#include <string>
#include <vector>
#include <memory>
#include <stdexcept>

namespace huanface {

class Exception : public std::runtime_error {
public:
    HFResult result;
    Exception(HFResult r, const std::string& msg) : std::runtime_error(msg), result(r) {}
};

inline void CheckResult(HFResult r) {
    if (r != HF_RESULT_OK) {
        throw Exception(r, HF_GetResultString(r));
    }
}

struct Frame {
    int width = 0;
    int height = 0;
    HFFormat format = HF_FORMAT_UNKNOWN;
    int64_t timestampNanos = 0;
    std::vector<uint8_t> data;
    std::vector<uint8_t> dataU;
    std::vector<uint8_t> dataV;
    int stride = 0;
    int strideU = 0;
    int strideV = 0;
    void* gpuTexture = nullptr;
    void* nativeHandle = nullptr;
    int rotation = 0;
    bool isMirrored = false;
    
    HFFrameC ToC() const {
        HFFrameC c{};
        c.width = width;
        c.height = height;
        c.format = format;
        c.timestampNanos = timestampNanos;
        c.data = const_cast<uint8_t*>(data.data());
        c.dataU = const_cast<uint8_t*>(dataU.data());
        c.dataV = const_cast<uint8_t*>(dataV.data());
        c.stride = stride;
        c.strideU = strideU;
        c.strideV = strideV;
        c.gpuTexture = gpuTexture;
        c.nativeHandle = nativeHandle;
        c.ownsData = 0; // C++ owns via vector
        c.ownsGpuTexture = 0;
        c.rotation = rotation;
        c.isMirrored = isMirrored ? 1 : 0;
        return c;
    }
};

struct FaceData {
    int id;
    struct Rect { float x,y,w,h; } bbox;
    float confidence;
    std::vector<std::pair<float,float>> landmarks; // 2D
    std::vector<std::tuple<float,float,float>> landmarks3D;
    // etc.
};

struct TrackingData {
    std::vector<FaceData> faces;
    int64_t timestampNanos;
};

struct EngineConfig {
    HFRenderBackendType backendType = HF_RENDER_BACKEND_AUTO;
    void* windowHandle = nullptr;
    int width = 1280;
    int height = 720;
    bool enableDebug = false;
    std::string faceTrackerType = "mediapipe";
    int maxFaces = 4;
    // ... etc.
    HFEngineConfigC ToC() const {
        HFEngineConfigC c{};
        c.backendType = backendType;
        c.windowHandle = windowHandle;
        c.width = width;
        c.height = height;
        c.enableDebug = enableDebug ? 1 : 0;
        c.faceTrackerType = faceTrackerType.c_str();
        c.maxFaces = maxFaces;
        // ...
        return c;
    }
};

class Bundle {
public:
    Bundle() : handle(nullptr) {}
    ~Bundle() { if (handle) HF_UnloadBundle(nullptr, handle); } // engine needed? Maybe store engine
    HFBundle handle;
};

class Engine {
public:
    Engine(const EngineConfig& config) {
        HFEngineConfigC c = config.ToC();
        HFResult r = HF_CreateEngine(&c, &engine);
        CheckResult(r);
    }
    ~Engine() {
        if (engine) HF_DestroyEngine(engine);
    }
    
    Bundle LoadBundle(const std::string& path) {
        Bundle bundle;
        HFResult r = HF_LoadBundle(engine, path.c_str(), &bundle.handle);
        CheckResult(r);
        return bundle;
    }
    
    void SetParameter(const std::string& name, float value) {
        CheckResult(HF_SetParameterFloat(engine, name.c_str(), value));
    }
    void SetParameter(const std::string& name, const HFColorC& color) {
        CheckResult(HF_SetParameterColor(engine, name.c_str(), color));
    }
    // ... overloads
    
    Frame ProcessFrame(const Frame& input) {
        HFFrameC inC = input.ToC();
        HFFrameC outC{};
        CheckResult(HF_ProcessFrame(engine, &inC, &outC));
        // Convert outC to Frame
        Frame out;
        out.width = outC.width;
        out.height = outC.height;
        // ... copy data if needed
        HF_FreeFrame(&outC);
        return out;
    }
    
    TrackingData GetFaceData() {
        HFTrackingDataC c{};
        CheckResult(HF_GetFaceData(engine, &c));
        TrackingData data;
        // Convert
        for (int i=0;i<c.faceCount;++i) {
            FaceData fd;
            fd.id = c.faces[i].id;
            fd.bbox = {c.faces[i].bboxX, c.faces[i].bboxY, c.faces[i].bboxW, c.faces[i].bboxH};
            fd.confidence = c.faces[i].confidence;
            // landmarks
            for (int j=0;j<c.faces[i].landmarkCount;++j) {
                fd.landmarks.emplace_back(c.faces[i].landmarks[j*2], c.faces[i].landmarks[j*2+1]);
            }
            data.faces.push_back(std::move(fd));
        }
        data.timestampNanos = c.timestampNanos;
        HF_FreeFaceData(&c);
        return data;
    }
    
private:
    HFEngine engine = nullptr;
};

// Global
inline void Init() { CheckResult(HF_Init()); }
inline void Shutdown() { CheckResult(HF_Shutdown()); }
inline std::string GetVersion() { return HF_GetVersion(); }

} // namespace huanface
```

**Usage example (C++):**

```cpp
#include "huanface.hpp"

int main() {
    huanface::Init();
    huanface::EngineConfig config;
    config.width = 1280;
    config.height = 720;
    config.faceTrackerType = "mediapipe";
    huanface::Engine engine(config);
    
    auto bundle = engine.LoadBundle("examples/bundles/simple_lip.hfbundle");
    engine.SetParameter("intensity_lip", 0.8f);
    engine.SetParameter("lip_color", HFColorC{1.0f, 0.2f, 0.3f, 1.0f});
    
    // Camera loop
    huanface::Frame input;
    input.width = 1280;
    input.height = 720;
    input.format = HF_FORMAT_RGBA8;
    input.data = cameraData; // from IFrameSource
    
    auto output = engine.ProcessFrame(input);
    auto faceData = engine.GetFaceData();
    
    // Display output
    
    huanface::Shutdown();
    return 0;
}
```

**Usage example (C):**

```c
#include "huanface_c_api.h"

int main() {
    HF_Init();
    HFEngineConfigC config = {0};
    config.backendType = HF_RENDER_BACKEND_AUTO;
    config.width = 1280;
    config.height = 720;
    config.faceTrackerType = "mediapipe";
    config.maxFaces = 4;
    
    HFEngine engine;
    HF_CreateEngine(&config, &engine);
    
    HFBundle bundle;
    HF_LoadBundle(engine, "examples/bundles/simple_lip.hfbundle", &bundle);
    
    HF_SetParameterFloat(engine, "intensity_lip", 0.8f);
    HFColorC color = {1.0f, 0.2f, 0.3f, 1.0f};
    HF_SetParameterColor(engine, "lip_color", color);
    
    HFFrameC input = {0};
    input.width = 1280;
    input.height = 720;
    input.format = HF_FORMAT_RGBA8;
    input.data = cameraData;
    input.stride = 1280*4;
    
    HFFrameC output = {0};
    HF_ProcessFrame(engine, &input, &output);
    
    HFTrackingDataC tracking = {0};
    HF_GetFaceData(engine, &tracking);
    // Use tracking
    
    HF_FreeFaceData(&tracking);
    HF_FreeFrame(&output);
    HF_UnloadBundle(engine, bundle);
    HF_DestroyEngine(engine);
    HF_Shutdown();
    return 0;
}
```

---

## 4. SDK Directory Structure

```
sdk/
├── include/
│   ├── huanface_c_api.h (C ABI)
│   ├── huanface.hpp (C++ RAII wrapper)
│   ├── huanface_frame.h (HFFrame, HFFormat)
│   ├── huanface_face.h (HFFaceData, HFTrackingData, HFFaceMesh)
│   ├── huanface_makeup.h (HFMakeupParameter)
│   ├── huanface_beauty.h (HFBeautyParameter)
│   ├── huanface_bundle.h (Bundle manifest)
│   └── huanface_version.h (version)
├── src/
│   ├── core/
│   │   ├── engine.cc (HuanFaceEngine impl)
│   │   ├── resource_manager.cc
│   │   └── clock.cc
│   ├── face/
│   │   ├── face_tracker.cc (IFaceTracker factory)
│   │   ├── mediapipe_tracker.cc
│   │   ├── onnx_tracker.cc
│   │   └── dlib_tracker.cc
│   ├── makeup/
│   │   ├── makeup_engine.cc
│   │   ├── mask_generator.cc
│   │   └── render_graph.cc
│   ├── beauty/
│   │   ├── beauty_engine.cc
│   │   ├── skin_filter.cc (bilateral)
│   │   └── face_warp.cc
│   ├── bundle/
│   │   ├── bundle_parser.cc
│   │   ├── resource_resolver.cc
│   │   └── bundle_validator.cc
│   ├── rendering/
│   │   ├── render_backend.cc (IRenderBackend factory)
│   │   ├── shader.cc
│   │   ├── texture.cc
│   │   └── mesh.cc
│   ├── rendering/d3d11/
│   │   ├── d3d11_backend.cc
│   │   ├── d3d11_texture.cc
│   │   ├── d3d11_shader.cc
│   │   └── d3d11_mesh.cc
│   ├── rendering/opengl/
│   │   ├── opengl_backend.cc
│   │   ├── opengl_texture.cc
│   │   ├── opengl_shader.cc
│   │   └── opengl_mesh.cc
│   └── platform/
│       └── windows/
│           ├── windows_camera_mf.cc
│           ├── windows_camera_dshow.cc
│           ├── windows_clock.cc
│           ├── windows_filesystem.cc
│           └── windows_threading.cc
├── lib/
│   ├── huanface.lib (import lib for DLL)
│   └── huanface.dll (built)
├── tools/
│   ├── huanface_bundle_packer.py
│   ├── huanface_bundle_unpacker.py
│   └── huanface_bundle_inspector.py
├── examples/
│   ├── bundles/
│   │   ├── simple_lip/
│   │   ├── simple_blush/
│   │   ├── simple_eyeshadow/
│   │   ├── simple_foundation/
│   │   └── *.hfbundle (packed)
│   └── desktop_camera/
│       └── main.cc (example app using SDK)
└── tests/
    ├── test_frame.cc
    ├── test_face_tracker.cc
    ├── test_bundle_parser.cc
    └── test_render_backend.cc
```

**This structure documented, placeholder files created in Phase 2, no full implementation.**

---

## 5. Desktop Camera Pipeline Example (from spec)

```cpp
// examples/desktop_camera/main.cc (pseudo, not implemented in Phase 2)
#include "huanface.hpp"
#include "platform/windows/windows_camera_mf.h"

int main() {
    huanface::Init();
    
    // Config
    huanface::EngineConfig config;
    config.backendType = HF_RENDER_BACKEND_D3D11; // P0
    config.width = 1280;
    config.height = 720;
    config.faceTrackerType = "mediapipe";
    config.maxFaces = 1;
    
    huanface::Engine engine(config);
    
    // Load bundle
    auto bundle = engine.LoadBundle("examples/bundles/simple_lip.hfbundle");
    engine.SetParameter("intensity_lip", 0.8f);
    
    // Camera
    WindowsCameraMF camera;
    camera.Open(0, 1280, 720, HF_FORMAT_NV12); // device 0, 1280x720, NV12
    
    // Window for display (HWND)
    HWND hwnd = CreateWindow(...);
    
    // Main loop
    HFFrame frame;
    while (true) {
        if (!camera.Read(frame)) break;
        
        auto output = engine.ProcessFrame(frame.ToCpp());
        
        // Render to window via IRenderBackend::Present
        // ...
        
        // Handle UI: slider for intensity_lip
        // engine.SetParameter("intensity_lip", sliderValue);
    }
    
    camera.Close();
    huanface::Shutdown();
    return 0;
}
```

---

## 6. No OBS Dependency

**Explicitly NOT in SDK API:**
- obs.dll, obsplus.dll
- Spout (optional extension, not core)
- OBS effects

**OBS is EXTERNAL REFERENCE only for research**

**SDK dependencies (open):**
- Windows SDK (MF, D3D11, OpenGL)
- stb_image, miniz, nlohmann/json, glm
- MediaPipe or ONNX Runtime

---

**End of SDK API Spec**
