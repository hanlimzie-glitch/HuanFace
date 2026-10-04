# Windows SDK Architecture — HuanFace (Phase 2)

**Status:** SPECIFICATION — Phase 2, no implementation, architecture only  
**Target Platform:** Windows 10+, Windows 11+, x64, Native C/C++ DLL  
**OBS Role:** EXTERNAL REFERENCE only, NOT runtime dependency

---

## 1. SDK Boundary

### 1.1 What HuanFace SDK IS

```
Application (C++, C#, Python, Unity, Unreal, custom)
    │
    ▼
┌─────────────────────────────────────────┐
│          HuanFace SDK (Windows)         │
│                                         │
│  Public C ABI (huanface_c_api.h)        │
│  C++ RAII (huanface.hpp)                │
│                                         │
│  ┌─────────────────────────────────┐    │
│  │         Core Runtime            │    │
│  │  - Runtime init/shutdown        │    │
│  │  - Memory management            │    │
│  │  - Resource Manager             │    │
│  │  - Config & Logging             │    │
│  └─────────────────────────────────┘    │
│  ┌─────────────────────────────────┐    │
│  │         Face Engine             │    │
│  │  - IFaceTracker interface       │    │
│  │  - Face detection               │    │
│  │  - Landmark (runtime-defined)   │    │
│  │  - Face Mesh (proposed)         │    │
│  │  - Tracking & Expression        │    │
│  └─────────────────────────────────┘    │
│  ┌─────────────────────────────────┐    │
│  │        Makeup Engine            │    │
│  │  - Lip, Eye, Eyebrow, Eyelash,  │    │
│  │    Eyeliner, Blush, Foundation, │    │
│  │    Pupil, Highlight, Shadow     │    │
│  │  - Generic HFMakeupParameter    │    │
│  │  - Mask Generation              │    │
│  │  - Render Graph (configurable)  │    │
│  └─────────────────────────────────┘    │
│  ┌─────────────────────────────────┐    │
│  │        Beauty Engine            │    │
│  │  - Skin Smooth (HeavyBlur)      │    │
│  │  - Whitening (ColorLevel)       │    │
│  │  - Sharpen, Clarity, Tone       │    │
│  │  - Face Shape (FaceThreed)      │    │
│  │  - Eye, Nose, Chin              │    │
│  └─────────────────────────────────┘    │
│  ┌─────────────────────────────────┐    │
│  │       Bundle Runtime            │    │
│  │  - BundleParser (manifest.json) │    │
│  │  - ResourceResolver             │    │
│  │  - ResourceManager cache        │    │
│  │  - Render Graph builder         │    │
│  └─────────────────────────────────┘    │
│  ┌─────────────────────────────────┐    │
│  │      Rendering Engine           │    │
│  │  - IRenderBackend abstraction   │    │
│  │  - D3D11 backend (P0)           │    │
│  │  - OpenGL backend (P1)          │    │
│  │  - Texture, Shader, Mesh,       │    │
│  │    Compositor                   │    │
│  └─────────────────────────────────┘    │
│  ┌─────────────────────────────────┐    │
│  │      Platform Layer             │    │
│  │  - Windows (Camera, FS, Thread, │    │
│  │    GPU, Timing)                 │    │
│  │  - Abstraction: IFrameSource,   │    │
│  │    IGpuDevice, IGpuTexture, etc.│    │
│  └─────────────────────────────────┘    │
└────────────────────┬────────────────────┘
                     │
                     ▼
              GPU Backend
              - D3D11 (P0, Windows priority)
              - OpenGL (P1, evidence CNamaSDK uses OpenGL 4.6)
              - Future: Vulkan, D3D12, Metal
                     │
                     ▼
              CPU (Face tracking, image processing)
```

### 1.2 What HuanFace SDK is NOT

```
NOT:

Application
    │
    ▼
OBS Studio (obs.dll) — EXTERNAL REFERENCE ONLY, NOT DEPENDENCY
    │
    ▼
HuanFace SDK

NOT:

HuanFace SDK → obs.dll
HuanFace SDK → obsplus.dll
HuanFace SDK → Spout.dll
HuanFace SDK → OBS .effect shaders
```

**OBS Role Clarification (EXTERNAL REFERENCE):**

OBS was used in Phase 0/1 as research evidence for:
- Frame flow (camera → OBS source → texture → FaceUnity → output → OBS composite → preview)
- Integration clues (obs-cam-beauty.dll is bridge, 63KB, 9 exports obs_module_*, imports obs.dll)
- Parameter usage (logs show SetParamTex tex_*, makeupController, INI beauty params HeavyBlur etc.)
- Runtime logs (face_makeup, body_beautify, hair_normal, GL 4.6, JS liufei)
- Texture flow (Spout for sharing, D3D11/OpenGL)
- Rendering clues (default.effect color space, but OBS effect NOT FaceUnity shader)

All OBS information is labeled **EXTERNAL REFERENCE** and NOT a runtime dependency.

**HuanFace SDK dependencies (PROPOSED, clean-room, no OBS):**
- Windows 10+ SDK (Win32, D3D11, DXGI)
- OpenGL 4.6 (optional, for OpenGL backend)
- Standard C++ library
- Third-party open: nlohmann/json or rapidjson (MIT), miniz (MIT) for ZIP, stb_image (MIT) for PNG, glm (MIT) for math, MediaPipe or ONNX Runtime for face tracking (Apache 2.0 / MIT)
- No proprietary FaceUnity, no OBS

---

## 2. Public API Boundary

Public API must NOT expose implementation details.

**C ABI (huanface_c_api.h) — stable for FFI:**
- `HF_Init()`, `HF_Shutdown()`
- `HF_CreateEngine()`, `HF_CreateEngineWithConfig()`, `HF_DestroyEngine()`
- `HF_LoadBundle()`, `HF_UnloadBundle()`
- `HF_SetParameter()`, `HF_GetParameter()`, `HF_SetTexture()`
- `HF_ProcessFrame()`, `HF_ProcessImage()`
- `HF_GetFaceData()`, `HF_GetFaceCount()`
- `HF_SetBeautyParameter()`, `HF_SetMakeupParameter()` (or generic SetParameter)

**C++ RAII (huanface.hpp):**
- `huanface::Engine` class wrapping C ABI
- `huanface::Bundle` class
- `huanface::FaceData`, `huanface::BeautyConfig`, etc.

**Internal (not public):**
- `IFaceTracker`, `IRenderBackend`, `IGpuDevice`, `ResourceManager`, `BundleParser`, `MakeupEffect`, `BeautyEffect`, etc.

**See:** `docs/SDK_API_SPEC.md` for full API spec (PROPOSED)

---

## 3. Platform Abstraction

**Goal:** Core engine NOT directly dependent on Win32, uses interfaces.

```
Platform
├── Windows
│   ├── Camera (Webcam capture via Media Foundation or DirectShow)
│   ├── FileSystem (ReadFile, WriteFile, ZIP via miniz)
│   ├── Threading (std::thread, Win32 threads, thread pool)
│   ├── GPU (D3D11 device, OpenGL context, DXGI)
│   └── Timing (QueryPerformanceCounter, std::chrono)
```

**Interfaces (PROPOSED):**

```cpp
// IFrameSource — camera or video or image
class IFrameSource {
public:
    virtual ~IFrameSource() = default;
    virtual bool Open(int deviceId, int width, int height) = 0;
    virtual bool Read(HFFrame& outFrame) = 0;
    virtual void Close() = 0;
};

// IGpuDevice — D3D11 or OpenGL
class IGpuDevice {
public:
    virtual ~IGpuDevice() = default;
    virtual bool Init() = 0;
    virtual IGpuTexture* CreateTexture(int w, int h, HFFormat format, const void* data) = 0;
    virtual IRenderTarget* CreateRenderTarget(int w, int h) = 0;
    virtual IShader* CreateShader(const std::string& vs, const std::string& fs) = 0;
    virtual void Shutdown() = 0;
};

// IGpuTexture
class IGpuTexture {
public:
    virtual ~IGpuTexture() = default;
    virtual int GetWidth() const = 0;
    virtual int GetHeight() const = 0;
    virtual HFFormat GetFormat() const = 0;
    virtual void* GetNativeHandle() const = 0; // ID3D11Texture2D* or GLuint
};

// IRenderTarget
class IRenderTarget {
public:
    virtual ~IRenderTarget() = default;
    virtual IGpuTexture* GetTexture() = 0;
    virtual void Bind() = 0;
    virtual void Unbind() = 0;
};

// IClock
class IClock {
public:
    virtual ~IClock() = default;
    virtual int64_t NowNanos() = 0;
    virtual double NowMillis() = 0;
};

// IFileSystem
class IFileSystem {
public:
    virtual ~IFileSystem() = default;
    virtual bool ReadFile(const std::string& path, std::vector<uint8_t>& outData) = 0;
    virtual bool WriteFile(const std::string& path, const void* data, size_t size) = 0;
    virtual bool Exists(const std::string& path) = 0;
};
```

**Windows Implementations (PROPOSED, not implemented in Phase 2):**
- `WindowsCamera_MF` — Media Foundation
- `WindowsCamera_DShow` — DirectShow (fallback)
- `D3D11GpuDevice`, `OpenGLGpuDevice`
- `WindowsFileSystem`
- `WindowsClock` (QueryPerformanceCounter)
- `WindowsThreadPool`

**See:** `docs/PLATFORM_ABSTRACTION.md` for full spec

---

## 4. Rendering Backend Abstraction

**IRenderBackend:**

```cpp
class IRenderBackend {
public:
    virtual ~IRenderBackend() = default;
    virtual bool Init(void* windowHandle = nullptr) = 0;
    virtual IGpuTexture* CreateTexture(int w, int h, HFFormat format, const void* data) = 0;
    virtual IShader* CreateShader(const std::string& vsSrc, const std::string& fsSrc) = 0;
    virtual IMesh* CreateMesh(const std::vector<HFFaceMeshVertex>& vertices, const std::vector<int>& indices) = 0;
    virtual IRenderTarget* CreateRenderTarget(int w, int h, HFFormat format) = 0;
    virtual void SetRenderTarget(IRenderTarget* rt) = 0;
    virtual void Clear(float r, float g, float b, float a) = 0;
    virtual void DrawMesh(IMesh* mesh, IShader* shader, const std::map<std::string, Uniform>& uniforms) = 0;
    virtual void Blit(IGpuTexture* src, IRenderTarget* dst) = 0;
    virtual void Present() = 0;
    virtual void Shutdown() = 0;
};
```

**Backends:**

**P0 D3D11 (Windows priority):**
- `D3D11RenderBackend`, `D3D11Texture`, `D3D11Shader`, `D3D11Mesh`, `D3D11RenderTarget`
- Uses `ID3D11Device`, `ID3D11DeviceContext`, `IDXGISwapChain`, `ID3D11Texture2D`, `ID3D11ShaderResourceView`, `ID3D11RenderTargetView`, `ID3D11VertexShader`, `ID3D11PixelShader`, `ID3D11Buffer`, `ID3D11InputLayout`
- Shader language: HLSL (compiled to bytecode)
- Evidence: OBS shader-cache .v2 is D3D11 bytecode cache, 223 files, suggests D3D11 is used in OBS environment, but HuanFace D3D11 backend is clean-room, not copying OBS

**P1 OpenGL (evidence CNamaSDK uses OpenGL 4.6):**
- `OpenGLRenderBackend`, `OpenGLTexture`, `OpenGLShader`, `OpenGLMesh`, `OpenGLRenderTarget`
- Uses `glCreateTextures`, `glTexImage2D`, `glCreateShader`, `glCompileShader`, `glCreateProgram`, `glGenBuffers`, `glBindBuffer`, `glDrawElements`, etc.
- Shader language: GLSL
- Evidence: Log `GLLoader.cc:212 initialGLExtentions: glversion max = 4, min = 6` from CNamaSDK, CNamaSDK imports OPENGL32.dll, beauty.exe imports OPENGL32.dll, strings GLProgramNew, GLTechnique, etc.
- **But:** HuanFace OpenGL backend is clean-room, not copying FaceUnity's GLProgramNew

**Architecture:**
```
Rendering API (HuanFace public)
      │
      ▼
IRenderBackend (abstraction)
   ┌──┴───┐
   │      │
 D3D11  OpenGL
   │      │
   └──┬───┘
      ▼
   GPU
```

**See:** `docs/RENDER_BACKEND.md` for full spec

---

## 5. Frame Abstraction

**HFFrame (PROPOSED):**

```cpp
enum class HFFormat {
    UNKNOWN,
    RGBA8,      // 8-bit RGBA, 4 bytes per pixel
    BGRA8,      // 8-bit BGRA (Windows D3D11 default)
    RGB8,       // 8-bit RGB, 3 bytes
    NV12,       // YUV 4:2:0, 2 planes
    YUV420P,    // YUV 4:2:0, 3 planes
    R8,         // 8-bit single channel (mask)
    R32F,       // 32-bit float single channel
};

struct HFFrame {
    int width = 0;
    int height = 0;
    HFFormat format = HFFormat::UNKNOWN;
    int64_t timestampNanos = 0;
    
    // CPU data (optional, for CPU processing)
    uint8_t* data = nullptr; // RGBA or Y plane
    uint8_t* dataU = nullptr; // U plane for NV12/YUV420P
    uint8_t* dataV = nullptr; // V plane
    int stride = 0; // bytes per row for data
    int strideU = 0;
    int strideV = 0;
    
    // GPU data (optional, for GPU processing, avoids copy)
    IGpuTexture* gpuTexture = nullptr; // D3D11 or OpenGL texture
    void* nativeHandle = nullptr; // ID3D11Texture2D* or GLuint for interop
    
    // Ownership
    bool ownsData = false; // if true, HuanFace will free data
    bool ownsGpuTexture = false;
};
```

**CPU vs GPU frame separation for performance:**

- **CPU frame:** `data` not null, `gpuTexture` null — for face tracking (MediaPipe, ONNX) which needs CPU image
- **GPU frame:** `gpuTexture` not null, `data` null — for rendering (makeup, beauty) which needs GPU texture, avoids CPU→GPU copy
- **Both:** `data` and `gpuTexture` both not null — for cases where both needed, but copy happens once

**Supported formats:**
- RGBA8, BGRA8, RGB8, NV12 (from OBS evidence: format_conversion.effect YUV→RGB, default.effect RGBA handling)
- R8 for masks (lip, eye, etc.)
- R32F for advanced processing (optional)

**See:** `docs/FRAME_PIPELINE.md` for full spec

---

## 6. Face Engine Boundary

**Based on Phase 1 FUAI analysis (OBSERVED):**
- Face Processor, FaceMeshV2, ProcessFacemesh, BMesh triangulation, Face Beauty Processor, Background Segmenter, Face Parsing, Face Attribute, Hand Processor, Human Processor/Driver, etc.
- But internal model architecture PROTECTED (TFLite weights inside encrypted bundles)

**HuanFace Face Engine (PROPOSED, clean-room, not copying internal implementation):**

```cpp
// IFaceTracker interface (swappable)
class IFaceTracker {
public:
    virtual ~IFaceTracker() = default;
    virtual bool Init(const HFEngineConfig& config) = 0;
    virtual HFTrackingData Process(const HFFrame& frame) = 0;
    virtual void Shutdown() = 0;
};

// Output structures (PROPOSED)
struct HFFaceData {
    int id = 0; // tracking ID
    HFRect bbox; // x, y, width, height
    float confidence = 0.0f;
    
    // Landmarks — runtime-defined count, not hardcoded 68 or 106 without evidence
    std::vector<HFPoint2D> landmarks; // 2D
    std::vector<HFPoint3D> landmarks3D; // 3D
    int landmarkCount = 0; // runtime-defined
    
    // Pose
    HFRotation rotation; // pitch, yaw, roll (quaternion or Euler)
    HFVec3 translation;
    HFVec3 scale;
    
    // Mesh (proposed, not FaceUnity mesh which is PROTECTED)
    HFFaceMesh mesh; // vertices, indices, UV, normals, landmarks mapping
    
    // Expression (optional)
    std::vector<float> expression; // blendshape weights, count runtime-defined (47 or 51 from FUAI ConvertExpression47To51)
    int expressionCount = 0;
    
    // Additional (optional)
    float eyeOpenLeft = 0.0f;
    float eyeOpenRight = 0.0f;
    float mouthOpen = 0.0f;
    // etc.
};

struct HFTrackingData {
    std::vector<HFFaceData> faces;
    int64_t timestampNanos = 0;
};

struct HFFaceMesh {
    std::vector<HFMeshVertex> vertices; // pos, normal, UV
    std::vector<int> indices; // triangles
    std::vector<HFVec2> uv;
    std::vector<HFVec3> normals;
    // Landmark to mesh mapping (which vertex corresponds to which landmark)
    std::map<int, int> landmarkToVertex;
};
```

**Landmark count:**
- From FUAI: `ConvertExpression47To51` suggests 47 and 51 expression blendshapes, but landmark count not directly observed
- Common open models: MediaPipe Face Mesh 468 points, dlib 68 points, FaceUnity might use 75 or 106 (from public docs, but not observed in repo, so we mark as UNKNOWN)
- **HuanFace:** `landmark_count = runtime-defined`, not hardcoded 68 or 106 without evidence, from `IFaceTracker` implementation

**Implementations (PROPOSED, not implemented in Phase 2):**
- `MediaPipeFaceTracker` — uses MediaPipe Face Mesh (468 points) + Face Detection, Apache 2.0
- `ONNXFaceTracker` — uses ONNX Runtime + custom models (e.g., SCRFD for detection, PFLD for landmarks)
- `DlibFaceTracker` — uses dlib 68 points (fallback, slower)

**See:** `docs/FACE_ENGINE_DESIGN.md` for full spec

---

## 7. Makeup Architecture

**Based on Phase 1 makeup_parameter_catalog.json (391 params, OBSERVED from strings + INI + logs):**
- `makeup_intensity_*`, `tex_*`, `blend_type_*`, `makeup_*_color`, `is_makeup_on`, etc.
- `HeavyBlur`, `ColorLevel`, etc. (beauty, but also makeup? Actually beauty)
- From logs: `tex_lip_mask_zz`, `tex_blusher`, `tex_eyeLash`, `tex_eyeLiner`, `tex_eye`, `tex_brow`, `eyepupil.png`, `eyeliner.png`, etc.

**HuanFace Makeup Architecture (PROPOSED, generic system, not 391 implementations at once):**

```cpp
enum class HFParamType {
    FLOAT,
    COLOR, // RGBA
    TEXTURE, // IGpuTexture*
    BOOL,
    ENUM, // int for blend mode etc.
    VEC2,
    VEC3,
    VEC4,
};

struct HFMakeupParameter {
    std::string name; // e.g., "lip_intensity", "tex_lip", "lip_color"
    HFParamType type;
    // Value union
    float floatValue = 0.0f;
    HFColor colorValue = {1,1,1,1};
    IGpuTexture* textureValue = nullptr;
    bool boolValue = false;
    int enumValue = 0;
    // Range/default only if evidence available, otherwise not set
    bool hasMin = false;
    float minValue = 0.0f;
    bool hasMax = false;
    float maxValue = 1.0f;
    bool hasDefault = false;
    float defaultFloat = 0.0f;
    HFColor defaultColor = {1,1,1,1};
};

class HFMakeupEffect {
public:
    virtual ~HFMakeupEffect() = default;
    virtual bool Init(const HFEngineConfig& config) = 0;
    virtual void SetParameter(const HFMakeupParameter& param) = 0;
    virtual HFMakeupParameter GetParameter(const std::string& name) const = 0;
    virtual void Render(const HFFaceData& face, const HFFrame& input, HFFrame& output, IRenderBackend* backend) = 0;
    virtual void Shutdown() = 0;
};

// Grouped by category (from Phase 1):
// Lip, Eye, Eyebrow, Eyelash, Eyeliner, Blush, Foundation, Pupil, Highlight, Shadow, Texture, Blend, Beauty

// Specific effects (PROPOSED, not implemented in Phase 2):
class HFLipMakeup : public HFMakeupEffect { ... };
class HFBlushMakeup : public HFMakeupEffect { ... };
class HFEyeShadowMakeup : public HFMakeupEffect { ... };
class HFEyebrowMakeup : public HFMakeupEffect { ... };
class HFEyelinerMakeup : public HFMakeupEffect { ... };
class HFEyelashMakeup : public HFMakeupEffect { ... };
class HFPupilMakeup : public HFMakeupEffect { ... };
class HFFoundationMakeup : public HFMakeupEffect { ... };
```

**Generic system, not 391 implementations at once — start with Lip, Blush, EyeShadow, Foundation as examples.**

**See:** `docs/MAKEUP_RUNTIME_DESIGN.md` for full spec

---

## 8. Makeup Pipeline (Generic, Configurable Render Graph)

**PROPOSED pipeline:**

```
Camera/Image (HFFrame)
      │
      ▼
Face Tracking (IFaceTracker → HFTrackingData)
      │
      ▼
Face Mesh (HFFaceMesh from HFFaceData)
      │
      ▼
Mask Generation (for each makeup category)
      ├── Lip: lip_polygon from landmarks → lip_mask (blur for soft edge) + occu mask (mouth, teeth)
      ├── Eye: eye mask from landmarks
      ├── Eyebrow: brow mask
      ├── Blush: cheek region
      └── etc.
      │
      ▼
Texture Sampling (from Bundle: textures/lip.png, eye.png, etc.)
      │
      ▼
Makeup Shader (GLSL/HLSL clean-room)
      ├── Vertex: transform face mesh
      └── Fragment: sample base image, makeup texture, mask, apply color, intensity, opacity, blend mode
      │
      ▼
Blend (Normal, Multiply, Screen, Overlay, etc. — from blend_type_* evidence)
      │
      ▼
Composite (MakeupPipeline — composite all makeup layers)
      │
      ▼
Output Frame (HFFrame with makeup)
```

**Makeup layers order (configurable render graph, since final order UNKNOWN from Phase 1):**

```
Proposed order (can be configured via manifest.json passes):
1. Foundation (base)
2. Blush
3. Eye Shadow
4. Eyebrow
5. Eyeliner
6. Eyelash
7. Pupil
8. Lip
9. Highlight
10. Shadow

But final order UNKNOWN from Phase 1, so use configurable render graph via manifest.json passes:
{
  "passes": [
    {"name": "foundation", "effect": "foundation", "order": 0},
    {"name": "blush", "effect": "blush", "order": 1},
    ...
  ]
}
```

**See:** `docs/MAKEUP_RUNTIME_DESIGN.md` for full spec

---

## 9. Beauty Pipeline (Separate from Makeup)

**BeautyEngine separate from MakeupEngine:**

```
Input (HFFrame)
 │
 ├── BeautyEngine
 │   ├── Skin Smooth (HeavyBlur, BlurLevel) — bilateral filter (proposed clean-room)
 │   ├── Whitening (ColorLevel) — color adjustment
 │   ├── Sharpen, Clarity
 │   ├── Tone (Brightness, Saturation, RedLevel)
 │   ├── Face Shape (FaceThreed, FaceSize) — mesh warp
 │   ├── Eye Adjustment (EyeBright, eye enlarge, eye distance)
 │   ├── Nose Adjustment (nose slim)
 │   ├── Chin Adjustment (chin slim)
 │   ├── Blemish Removal (DelspotLevel)
 │   ├── Tooth Whiten, Dark Circle Removal, Nasolabial Fold Removal
 │   └── etc.
 │
 └── MakeupEngine (as above)
       │
       ▼
   Composite (Beauty + Makeup)
       │
       ▼
   Output Frame
```

**Beauty parameters from Phase 1 (391 catalog, beauty category):**
- `HeavyBlur`, `BlurLevel`, `ColorLevel`, `ColorLevelType`, `DelspotLevel`, `RedLevel`, `Clarity`, `Sharpen`, `FaceThreed`, `EyeBright`, `ToothWhiten`, `RemovePouchStrength`, `RemoveNasolabialFoldsStrength`, `Brightness`, `Saturation`, `FaceSize`, `FaceLandmarkQuality`, `BodyNum`, `BeautyProtection`, `BeautyRenderFPS`, `RotateClockwise`, etc. (140 from INI)

**Algorithms PROTECTED/UNKNOWN from Phase 1 (original FaceUnity), so HuanFace uses clean-room proposed:**
- Skin smooth: bilateral filter (not copying FaceUnity)
- Whitening: color adjustment (RGB curve)
- Sharpen: unsharp mask
- Face shape: mesh warp based on landmarks (move vertices)
- Eye/nose/chin: similar mesh warp

**Config:**
```cpp
struct HFBeautyConfig {
    float skinSmooth = 0.5f; // HeavyBlur
    float whitening = 0.3f; // ColorLevel
    float sharpen = 0.2f;
    float clarity = 0.2f;
    float rosiness = 0.1f; // RedLevel
    float blemishRemoval = 0.0f; // DelspotLevel
    // ... plus face shape etc.
};
```

**See:** `docs/BEAUTY_RUNTIME_DESIGN.md` for full spec

---

## 10. Performance Design

**Target desktop:**
- 1080p
- 30 FPS minimum
- 60 FPS target

**Design considerations (documented, not implemented in Phase 2):**
- GPU texture reuse (avoid per-frame allocation, pool)
- Async processing (face tracking in separate thread, double-buffered)
- Resource cache (textures, shaders, meshes cached via ResourceManager, LRU)
- Shader cache (compile once, cache binary)
- Frame synchronization (CPU/GPU sync, avoid stall)
- Memory reuse (pool allocators for face data, mesh vertices)
- Format handling (BGRA8 for D3D11 to avoid conversion, NV12 for camera direct)

**See:** `docs/WINDOWS_SDK_ARCHITECTURE.md` (this file) and `docs/RENDER_BACKEND.md`

---

## 11. OBS Exclusion Rule (CRITICAL)

**Added to docs/ARCHITECTURE.md:**

```
OBS Studio is NOT a runtime dependency of HuanFace SDK.

OBS integration only:
- research reference
- validation environment
- optional integration example (e.g., examples/obs_integration/ as separate, not core)

Core SDK dependency list MUST NOT include:
- obs.dll
- obsplus.dll
- Spout.dll, SpoutDX.dll, SpoutLibrary.dll, win-spout.dll
- OBS effects (default.effect, format_conversion.effect)
- OBS shader-cache .v2

All OBS information is labeled EXTERNAL REFERENCE.

HuanFace SDK dependencies (clean-room, open):
- Windows 10+ SDK (Win32, D3D11, DXGI)
- OpenGL 4.6 (optional)
- Standard C++ library
- Open third-party: nlohmann/json (MIT), miniz (MIT), stb_image (MIT), glm (MIT), MediaPipe (Apache 2.0) or ONNX Runtime (MIT)
```

---

## 12. SDK Directory Structure (PROPOSED, not implemented in Phase 2)

```
HuanFace/
│
├── sdk/
│   ├── include/
│   │   ├── huanface/
│   │   │   ├── huanface_c_api.h (C ABI)
│   │   │   ├── huanface.hpp (C++ RAII)
│   │   │   ├── huanface_core.h
│   │   │   ├── huanface_face.h
│   │   │   ├── huanface_makeup.h
│   │   │   ├── huanface_beauty.h
│   │   │   ├── huanface_bundle.h
│   │   │   ├── huanface_rendering.h
│   │   │   └── huanface_platform.h
│   │   └── huanface.h (main include)
│   │
│   ├── src/
│   │   ├── core/
│   │   │   ├── runtime.cpp
│   │   │   ├── resource_manager.cpp
│   │   │   ├── config.cpp
│   │   │   └── logging.cpp
│   │   ├── face/
│   │   │   ├── face_tracker.cpp (interface)
│   │   │   ├── mediapipe_tracker.cpp (implementation)
│   │   │   ├── onnx_tracker.cpp
│   │   │   ├── face_mesh.cpp
│   │   │   └── landmark.cpp
│   │   ├── makeup/
│   │   │   ├── makeup_effect.cpp (base)
│   │   │   ├── lip_makeup.cpp
│   │   │   ├── blush_makeup.cpp
│   │   │   ├── eyeshadow_makeup.cpp
│   │   │   ├── eyebrow_makeup.cpp
│   │   │   ├── eyeliner_makeup.cpp
│   │   │   ├── eyelash_makeup.cpp
│   │   │   ├── pupil_makeup.cpp
│   │   │   ├── foundation_makeup.cpp
│   │   │   └── makeup_pipeline.cpp
│   │   ├── beauty/
│   │   │   ├── beauty_effect.cpp (base)
│   │   │   ├── skin_smooth.cpp
│   │   │   ├── whitening.cpp
│   │   │   ├── sharpen.cpp
│   │   │   ├── face_shape.cpp
│   │   │   └── beauty_pipeline.cpp
│   │   ├── bundle/
│   │   │   ├── bundle_parser.cpp
│   │   │   ├── bundle_loader.cpp
│   │   │   ├── resource_resolver.cpp
│   │   │   └── bundle_runtime.cpp
│   │   ├── rendering/
│   │   │   ├── render_backend.cpp (interface)
│   │   │   ├── texture.cpp
│   │   │   ├── shader.cpp
│   │   │   ├── mesh.cpp
│   │   │   ├── compositor.cpp
│   │   │   ├── d3d11/
│   │   │   │   ├── d3d11_backend.cpp
│   │   │   │   ├── d3d11_texture.cpp
│   │   │   │   ├── d3d11_shader.cpp
│   │   │   │   ├── d3d11_mesh.cpp
│   │   │   │   └── d3d11_render_target.cpp
│   │   │   └── opengl/
│   │   │       ├── opengl_backend.cpp
│   │   │       ├── opengl_texture.cpp
│   │   │       ├── opengl_shader.cpp
│   │   │       ├── opengl_mesh.cpp
│   │   │       └── opengl_render_target.cpp
│   │   └── platform/
│   │       └── windows/
│   │           ├── windows_camera_mf.cpp
│   │           ├── windows_camera_dshow.cpp
│   │           ├── windows_filesystem.cpp
│   │           ├── windows_clock.cpp
│   │           └── windows_threadpool.cpp
│   │
│   └── CMakeLists.txt (build SDK DLL)
│
├── tools/
│   ├── bundle_inspector.py (existing, for FaceUnity encrypted header only)
│   ├── asset_inspector.py
│   ├── texture_inspector.py
│   ├── metadata_inspector.py
│   ├── huanface_bundle_packer.py (NEW, for HuanFace .hfbundle ZIP)
│   ├── huanface_bundle_unpacker.py (NEW)
│   └── huanface_bundle_inspector.py (NEW, for HuanFace .hfbundle)
│
├── examples/
│   ├── bundles/
│   │   ├── simple_lip/ (clean-room example)
│   │   ├── simple_blush/
│   │   ├── simple_eyeshadow/
│   │   └── simple_foundation/
│   ├── basic_face_demo/ (future, Phase 4)
│   ├── makeup_demo/ (future, Phase 6)
│   └── obs_integration/ (optional, EXTERNAL REFERENCE, not core)
│
├── tests/
│   ├── core/
│   ├── face/
│   ├── makeup/
│   ├── beauty/
│   ├── bundle/
│   └── integration/
│
├── docs/
│   ├── WINDOWS_SDK_ARCHITECTURE.md (this file)
│   ├── PLATFORM_ABSTRACTION.md
│   ├── RENDER_BACKEND.md
│   ├── FRAME_PIPELINE.md
│   ├── FACE_ENGINE_DESIGN.md
│   ├── MAKEUP_RUNTIME_DESIGN.md
│   ├── BEAUTY_RUNTIME_DESIGN.md
│   ├── HUANFACE_BUNDLE_SPEC.md
│   ├── SDK_API_SPEC.md
│   ├── BUNDLE_FORMAT.md (updated with OBSERVED/INFERRED/PROTECTED/PROPOSED)
│   ├── ARCHITECTURE.md (updated with OBS exclusion)
│   └── etc.
│
└── analysis/
    ├── asset_catalog.json
    ├── binary_catalog.json
    ├── dependency_graph.json (updated)
    ├── string_catalog.json
    ├── makeup_parameter_catalog.json
    └── etc.
```

**Note:** In Phase 2, we only create directory structure and specification, NOT full implementation of engine. Implementation starts Phase 4.

---

## 13. Summary

**Windows SDK boundary defined:** YES — Application → HuanFace SDK (C ABI + C++ RAII) → Core, Face, Makeup, Beauty, Bundle, Rendering, Platform → GPU Backend (D3D11 P0, OpenGL P1)

**OBS removed as runtime dependency:** YES — OBS is EXTERNAL REFERENCE only, not in core SDK dependency list, explicitly marked in ARCHITECTURE.md

**D3D11 backend architecture defined:** YES — D3D11RenderBackend, D3D11Texture, D3D11Shader, etc., HLSL, ID3D11Device etc., P0 priority for Windows

**OpenGL backend architecture defined:** YES — OpenGLRenderBackend, OpenGLTexture, etc., GLSL, evidence CNamaSDK uses OpenGL 4.6 (GLLoader log, imports OPENGL32), P1, clean-room not copying FaceUnity's GLProgramNew

**Frame abstraction defined:** YES — HFFrame with width, height, format (RGBA8, BGRA8, RGB8, NV12, R8, R32F), timestamp, CPU data (data, stride) and GPU data (gpuTexture, nativeHandle), ownership flags, CPU vs GPU separation for performance

**Face engine interface defined:** YES — IFaceTracker interface swappable, HFFaceData with id, bbox, confidence, landmarks (runtime-defined count, not hardcoded 68/106), rotation, translation, scale, mesh (HFFaceMesh vertices, indices, UV, normals), expression (runtime-defined count), implementations MediaPipeFaceTracker, ONNXFaceTracker proposed

**Makeup architecture defined:** YES — generic HFMakeupParameter with name, type (float, color, texture, bool, enum), value, hasMin/hasMax/hasDefault only if evidence, HFMakeupEffect base, grouped Lip, Eye, Eyebrow, Eyelash, Eyeliner, Blush, Foundation, Pupil, Highlight, Shadow, Texture, Blend, Beauty, 391 params catalog used as evidence but not 391 implementations at once

**Beauty architecture defined:** YES — BeautyEngine separate from MakeupEngine, BeautyConfig with skinSmooth (HeavyBlur), whitening (ColorLevel), sharpen, clarity, rosiness, blemishRemoval, eyeBright, toothWhiten, etc., plus face shape Eye, Nose, Chin, clean-room algorithms (bilateral filter etc.) not copying FaceUnity

**Resource system defined:** YES — ResourceManager with cache for Texture, Shader, Mesh, Mask, Material, Parameter, Bundle→ResourceManager→GPU Resource, avoids loading texture repeatedly

**Performance design:** 1080p 30 FPS min 60 FPS target, GPU texture reuse, async processing, resource cache, shader cache, frame sync, memory reuse, format handling BGRA8 for D3D11, NV12 for camera

**OBS exclusion rule added to ARCHITECTURE.md:** YES

---

**End of Windows SDK Architecture**
