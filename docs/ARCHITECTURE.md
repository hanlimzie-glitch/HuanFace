# HuanFace SDK — Architecture Design (Phase 2 Updated)

**Status:** Phase 2 — Windows SDK Architecture & Bundle Format defined, no implementation yet  
**Target:** Windows 10+, Windows 11+, x64, Native C/C++ DLL  
**OBS Role:** EXTERNAL REFERENCE only, NOT runtime dependency (critical rule)

---

## 0. OBS Exclusion Rule (CRITICAL — Added Phase 2)

**OBS Studio is NOT a runtime dependency of HuanFace SDK.**

OBS integration in Phase 0/1 was research evidence only:

- Frame flow clues (camera → OBS source → texture → FaceUnity → output → composite)
- Integration clues (obs-cam-beauty.dll 63KB bridge, 9 exports obs_module_*, imports obs.dll)
- Parameter usage (SetParamTex tex_*, makeupController, INI beauty params)
- Logs (face_makeup, body_beautify, GL 4.6, JS liufei)
- Texture sharing (Spout) and D3D11/OpenGL usage

**All OBS information is labeled EXTERNAL REFERENCE and NOT runtime dependency.**

**Core SDK dependency list MUST NOT include:**

- obs.dll
- obsplus.dll
- Spout.dll, SpoutDX.dll, SpoutLibrary.dll, win-spout.dll
- OBS effects (default.effect, format_conversion.effect) — OBS built-in, not FaceUnity
- OBS shader-cache .v2 — D3D11 bytecode cache from OBS environment

**HuanFace SDK dependencies (clean-room, open):**

- Windows 10+ SDK (Win32, D3D11, DXGI, Media Foundation)
- OpenGL 4.6 (optional, for OpenGL backend P1, evidence CNamaSDK uses OpenGL 4.6 from log GLLoader.cc:212 and OPENGL32 imports)
- Standard C++ library
- Open third-party: nlohmann/json (MIT) for manifest, miniz (MIT) for ZIP, stb_image (MIT) for PNG, glm (MIT) for math, MediaPipe (Apache 2.0) or ONNX Runtime (MIT) for face tracking
- No proprietary FaceUnity, no OBS

**If OBS integration needed in future, it must be separate example (e.g., examples/obs_integration/) not core SDK.**

---

## 1. Overview (Phase 2)

HuanFace SDK is modular, clean-room beauty camera engine for Windows 10+/11+ x64, Native C/C++ DLL, NO dependency on OBS/obs.dll/obsplus/Spout/OBS shaders.

Goals:
- No proprietary code copying
- No DRM bypass
- Open bundle format .hfbundle ZIP-based (manifest.json + PNG + GLSL + JSON)
- Modular, swappable components (face tracking IFaceTracker, rendering IRenderBackend)
- C ABI for FFI to C++, C#, Python, Unity, Unreal + C++ RAII wrapper
- 30 FPS min, 60 FPS target at 1080p
- D3D11 P0, OpenGL P1 (evidence CNamaSDK uses GL 4.6)

See `docs/WINDOWS_SDK_ARCHITECTURE.md` for full Windows SDK boundary spec.

---

## 2. High-Level Architecture (Phase 2 Updated)

```
Application (C++, C#, Python, Unity, Unreal, custom desktop app)
        │
        ▼
HuanFace SDK Public API (C ABI huanface_c_api.h + C++ RAII huanface.hpp)
        │
        ▼
┌─────────────────────────────────────┐
│         HuanFace Runtime            │
│  ┌──────────┐  ┌──────────────────┐ │
│  │   Core   │  │      Face        │ │
│  │ Runtime  │  │  IFaceTracker    │ │
│  │ Resource │  │  Detection       │ │
│  │ Manager  │  │  Landmark (runtime-defined count) │ │
│  │ Config   │  │  Mesh (HFFaceMesh) │ │
│  │ Logging  │  │  Expression      │ │
│  └──────────┘  └──────────────────┘ │
│  ┌──────────┐  ┌──────────────────┐ │
│  │  Makeup  │  │     Beauty       │ │
│  │  Lip etc │  │  Skin Smooth (HeavyBlur) │ │
│  │  Generic │  │  Whitening (ColorLevel)  │ │
│  │  HFMakeup│  │  Face Shape etc  │ │
│  │  Param   │  │  Separate Engine │ │
│  └──────────┘  └──────────────────┘ │
│  ┌──────────┐  ┌──────────────────┐ │
│  │  Bundle  │  │    Rendering     │ │
│  │  Parser  │  │  IRenderBackend  │ │
│  │  Resolver│  │  D3D11 P0        │ │
│  │  Manager │  │  OpenGL P1       │ │
│  │  .hfbundle ZIP │ Texture/Shader/Mesh/Compositor │ │
│  └──────────┘  └──────────────────┘ │
│  ┌──────────────────────────────┐   │
│  │      Platform Layer          │   │
│  │  Windows: Camera (MF/DShow)  │   │
│  │  FileSystem (miniz ZIP)      │   │
│  │  Threading, GPU, Timing (QPC)│   │
│  │  IFrameSource, IGpuDevice etc│   │
│  └──────────────────────────────┘   │
└─────────────────────────────────────┘
        │
        ▼
   GPU Backend
   - D3D11 P0 (Windows priority, ID3D11Device/Context/SwapChain)
   - OpenGL P1 (evidence CNamaSDK GL 4.6, GLLoader.cc log, OPENGL32 imports)
   - Future: Vulkan, D3D12, Metal
        │
        ▼
   CPU (Face tracking MediaPipe/ONNX, image processing)
```

**SDK Boundary (Phase 2):**

```
Application
  │
  ▼
HuanFace SDK (Windows)
  ├── Public C ABI (huanface_c_api.h) + C++ RAII (huanface.hpp)
  ├── Core Runtime (Runtime, ResourceManager, Config, Logging, IClock, IFileSystem, IThreading)
  ├── Face Engine (IFaceTracker interface swappable MediaPipe/ONNX, HFFaceData bbox/landmarks runtime-defined not hardcoded 68/106, rotation/translation/scale/confidence/mesh/expression)
  ├── Makeup Engine (Lip/Eye/Eyebrow/Eyelash/Eyeliner/Blush/Foundation/Pupil/Highlight/Shadow/Texture/Blend/Beauty groups, generic HFMakeupParameter name/type/value, range/default only if evidence, render graph Foundation→Blush→EyeShadow→Eyebrow→Eyeliner→Eyelash→Pupil→Lip→Highlight→Shadow configurable)
  ├── Beauty Engine (separate from MakeupEngine, params HeavyBlur/ColorLevel/DelspotLevel/RedLevel/Clarity/Sharpen/FaceThreed/EyeBright/ToothWhiten/RemovePouchStrength etc. from catalog, clean-room bilateral filter etc.)
  ├── Bundle Runtime (BundleParser manifest.json, ResourceResolver, ResourceManager cache Bundle→ResourceManager→GPU, .hfbundle ZIP open format NOT reverse-engineered FaceUnity encrypted)
  ├── Rendering Engine (IRenderBackend abstraction, D3D11Backend P0 HLSL clean-room, OpenGLBackend P1 GLSL 4.6 clean-room, texture/shader/mesh/compositor, no OBS shaders)
  └── Platform Layer (Windows: Camera via Media Foundation/DirectShow IFrameSource, FileSystem via miniz ZIP IFileSystem, Threading IThreading, GPU IGpuDevice/IGpuTexture/IRenderTarget/IShader/IMesh D3D11/OpenGL, Timing via QueryPerformanceCounter IClock)
  │
  ▼
GPU Backend D3D11 P0, OpenGL P1
```

**Detailed specs:**

- `docs/WINDOWS_SDK_ARCHITECTURE.md` — SDK boundary, public API, platform abstraction, render backend, frame abstraction, face engine, makeup, beauty, resource system, performance, directory structure
- `docs/PLATFORM_ABSTRACTION.md` — IFrameSource, IGpuDevice, IGpuTexture, IRenderTarget, IShader, IMesh, IClock, IFileSystem, IThreading interfaces + Windows impls MF/DShow/D3D11/OpenGL/QPC
- `docs/RENDER_BACKEND.md` — IRenderBackend Init/Shutdown/CreateTexture/RenderTarget/Shader/Mesh/DrawMesh/Blit/Present/SetBlendMode, D3D11Backend classes HLSL example clean-room, OpenGLBackend 4.6 evidence GLLoader log OPENGL32 imports GLSL example, backend selection AUTO/D3D11/OPENGL, no OBS dependency
- `docs/FRAME_PIPELINE.md` — HFFrame width/height/format/timestamp/data/stride/GPU texture, RGBA8/BGRA8/RGB8/NV12/R8/R32F, CPU vs GPU separation, rotation handling from OBS RotateClockwise evidence, pipeline Webcam→Capture→HFFrame→Face Tracking→Beauty→Makeup→Render Backend→Output, also Image/Video/GPU Texture support, performance GPU texture reuse async resource cache
- `docs/FACE_ENGINE_DESIGN.md` — IFaceTracker swappable, HFFaceData bbox/landmarks runtime-defined not hardcoded 68/106, rotation/translation/scale/confidence/mesh/expression, HFFaceMesh vertices/indices/UV/normals/landmarkToVertex, MediaPipe P0 468 points Apache 2.0, ONNX P1 SCRFD+PFLD+3DDFA_V2 MIT, Dlib fallback, FaceMeshV2 BMesh triangulation from FUAI OBSERVED but PROTECTED exact structure UNKNOWN, HuanFace mesh PROPOSED clean-room
- `docs/MAKEUP_RUNTIME_DESIGN.md` — 391 params grouped Lip/Eye/Brow/Blush/Foundation/Highlight/Shadow/Texture/Blend/Beauty, generic HFMakeupParameter name/type/value range/default only if evidence, pipeline Camera→Face Tracking→Face Mesh→Mask Generation→Texture Sampling→Makeup Shader→Blend→Composite, layers configurable render graph manifest.json passes, resource system BundleParser→Manifest→ResourceResolver→Runtime→Render Graph Bundle→ResourceManager→GPU
- `docs/BEAUTY_RUNTIME_DESIGN.md` — BeautyEngine separate vs MakeupEngine, params HeavyBlur/ColorLevel/DelspotLevel/RedLevel/Clarity/Sharpen/FaceThreed/EyeBright/ToothWhiten/RemovePouchStrength/RemoveNasolabialFoldsStrength etc. from INI 263 keys 140 beauty, pipeline Skin Processing bilateral filter + face mask, Face Shape Warp mesh warp, Eye/Teeth/Dark Circle localized, Body/Background optional segmentation + blur, clean-room GLSL bilateral example not FaceUnity PROTECTED
- `docs/HUANFACE_BUNDLE_SPEC.md` — .hfbundle ZIP open format PROPOSED NOT reverse-engineered FaceUnity encrypted (magic F3 5B 06 12 entropy 7.7-7.85 PROTECTED), structure manifest.json + textures/ PNG + masks/ + shaders/ GLSL/HLSL clean-room + meshes/ JSON/OBJ + metadata/thumbnail.png, manifest schema format HuanFaceBundle version type name author dependencies textures masks shaders meshes parameters passes extensible, preset types makeup/beauty/filter/effect/hair/face/combined, parameter schema generic float/color/texture/bool/enum min/max/default only if known, resource system ResourceManager cache, bundle to runtime flow, tools packer/unpacker/inspector only for HuanFace .hfbundle not FaceUnity encrypted, example bundles simple_lip/blush/eyeshadow/foundation clean-room assets no protected extraction
- `docs/SDK_API_SPEC.md` — Public API C ABI huanface_c_api.h + C++ RAII huanface.hpp, types HFEngine/HFBundle/HFTexture opaque handles HFResult HFRenderBackendType HFFormat HFParamType HFFrameC HFFaceDataC HFTrackingDataC HFEngineConfigC HFColorC, functions HF_Init/Shutdown/CreateEngine/Destroy/LoadBundle/LoadBundleFromMemory/Unload/SetParameterFloat/Int/Bool/Color/Vec2/3/4/Texture/Enum/GetParameter/ProcessFrame/ProcessFrameWithBundle/GetFaceData/FreeFaceData/FreeFrame/GetVersion/GetResultString, C++ wrapper namespace huanface Engine/Bundle/Frame/FaceData/TrackingData/EngineConfig with RAII CheckResult exceptions, usage examples C and C++, SDK directory structure sdk/include/src/core/face/makeup/beauty/bundle/rendering/d3d11/opengl/platform/windows/tools/examples/tests/docs/analysis

---

## 3. Module Details (Updated Phase 2)

### 3.1 Core

See WINDOWS_SDK_ARCHITECTURE.md section 1-2 and PLATFORM_ABSTRACTION.md

Responsibilities: Runtime lifecycle, Memory, ResourceManager cache, Config, Logging, IClock, IFileSystem, IThreading

No OBS dependency.

### 3.2 Face

See FACE_ENGINE_DESIGN.md

IFaceTracker swappable, HFFaceData runtime-defined landmark count not hardcoded 68/106, HFFaceMesh clean-room not FaceUnity PROTECTED, MediaPipe P0, ONNX P1.

### 3.3 Makeup

See MAKEUP_RUNTIME_DESIGN.md

Generic HFMakeupParameter, grouped 391 params, pipeline mask generation texture sampling shader blend composite, configurable render graph.

### 3.4 Beauty

See BEAUTY_RUNTIME_DESIGN.md

BeautyEngine separate, params from catalog, pipeline skin smooth bilateral filter whitening face shape warp etc., clean-room not FaceUnity PROTECTED.

### 3.5 Bundle

See HUANFACE_BUNDLE_SPEC.md

.hfbundle ZIP open format, manifest.json schema, textures PNG, shaders GLSL/HLSL clean-room, meshes JSON/OBJ, tools packer/unpacker/inspector only for HuanFace .hfbundle not FaceUnity encrypted, example bundles clean-room.

### 3.6 Rendering

See RENDER_BACKEND.md

IRenderBackend abstraction, D3D11Backend P0 HLSL clean-room, OpenGLBackend P1 GLSL 4.6 evidence CNamaSDK uses GL 4.6 but clean-room not copying GLProgramNew, backend selection AUTO/D3D11/OPENGL, no OBS shaders.

### 3.7 Platform

See PLATFORM_ABSTRACTION.md

IFrameSource (WindowsCamera_MF P0 Media Foundation MFCreateMediaSource IMFSourceReader, DShow fallback), IGpuDevice/IGpuTexture/IRenderTarget/IShader/IMesh D3D11/OpenGL, IClock QPC, IFileSystem UTF-8→UTF-16 + miniz ZIP, IThreading Thread/Mutex/CondVar/ThreadPool.

### 3.8 API

See SDK_API_SPEC.md

C ABI huanface_c_api.h + C++ RAII huanface.hpp, HF_Init/CreateEngine/LoadBundle/SetParameter/ProcessFrame/GetFaceData/Destroy/Shutdown, desktop camera pipeline Webcam→Capture→HFFrame→Face Tracking→Beauty→Makeup→Render Backend→Output + Image/Video/GPU Texture.

---

## 4. Data Flow (Phase 2)

### 4.1 Initialization

```
HF_Init()
  → Core::Runtime::Init()
  → IClock::NowNanos() QPC
  → IFileSystem (WindowsFileSystem)
  → ResourceManager cache init
  → IRenderBackend factory (AUTO → try D3D11, fallback OpenGL)

HF_CreateEngine(config)
  → Create IFaceTracker (MediaPipe or ONNX via factory)
  → Create IRenderBackend (D3D11 or OpenGL)
  → Create BeautyEngine, MakeupEngine
  → Create BundleLoader, ResourceResolver, BundleRuntime
```

### 4.2 Bundle Loading

```
HF_LoadBundle("simple_lip.hfbundle")
  → BundleParser (miniz ZIP + nlohmann/json)
    → Open ZIP, Read manifest.json, Validate schema format==HuanFaceBundle
    → Parse textures/masks/shaders/meshes/parameters/passes
  → ResourceResolver (resolve paths inside ZIP)
  → ResourceManager
    → Load textures via stb_image PNG → IGpuTexture via IGpuDevice::CreateTexture → cache LRU
    → Load shaders via ReadFile → IRenderBackend::CreateShader compile GLSL/HLSL → cache
    → Load meshes via JSON/OBJ parser → IGpuDevice::CreateMesh → cache
  → BundleRuntime::CreateObjects()
    → Create MakeupEffect instances based on manifest type/passes
    → Bind params from manifest defaults
```

### 4.3 Frame Processing

```
HF_ProcessFrame(input HFFrame, output HFFrame)
  → IFrameSource::Read (if camera) → HFFrame CPU (RGBA8) or GPU (BGRA8 D3D11 texture zero-copy from MF)
  → IFaceTracker::Process (CPU data) → HFTrackingData faces bbox landmarks runtime-defined mesh rotation translation
  → BeautyEngine::Process (input + faceData + beautyParams HeavyBlur etc.)
    → Skin mask from landmarks/mesh → bilateral filter shader (GPU) masked → whitening etc. → face shape warp mesh warp shader
  → MakeupEngine::Process (beautified + faceData + bundle + makeupParams)
    → For each pass in manifest passes order:
      → Mask Generation (lip polygon from landmarks → rasterize R8 → blur soft edge)
      → Texture Sampling from ResourceManager cache
      → Makeup Shader (GLSL/HLSL clean-room) set uniforms intensity color textures masks blend mode
      → DrawMesh/Blit via IRenderBackend
    → Composite output
  → IRenderBackend::Present() if windowHandle
  → Output HFFrame (GPU texture or CPU data)
```

---

## 5. Threading Model

- Main thread: API calls, bundle loading, resource management, rendering (D3D11/OpenGL context)
- Face tracking thread: Async face detection optional for performance, double-buffered face data
- Camera thread: IFrameSource Read in separate thread
- Resource loading thread: async texture/shader loading via IThreading ThreadPool

Synchronization: mutex for face data double buffer, ResourceManager thread-safe

---

## 6. Memory Model

- Pool allocators for face data HFFaceData/HFTrackingData/HFFaceMesh avoid per-frame alloc
- Texture cache LRU eviction (e.g., 100 MB textures, 50 shaders) via ResourceManager
- Bundle resources ref-counted, unloaded when not used
- HFFrame data pool allocate once for max size 1080p RGBA 8.29 MB reuse
- GPU texture reuse pool triple buffering avoid per-frame allocation

---

## 7. Platform Support (Phase 2 Focus Windows)

- Windows 10+, 11+, x64, Native C/C++ DLL — P0
- D3D11 backend P0, OpenGL P1 (evidence CNamaSDK GL 4.6)
- Linux OpenGL/Vulkan future, macOS Metal future, Android/iOS future but not Phase 2

Initial target: Windows + D3D11 (for desktop camera app) + OpenGL (for cross-platform evidence)

---

## 8. Dependencies (Third-Party Open)

- Face tracking: MediaPipe Apache 2.0 or ONNX Runtime MIT + open models SCRFD/PFLD/3DDFA_V2
- Image: stb_image MIT for PNG
- ZIP: miniz MIT
- JSON: nlohmann/json MIT
- Math: glm MIT
- Rendering: D3D11 system, OpenGL 4.6 system, glad MIT loader optional
- No proprietary FaceUnity, no OBS

---

## 9. Security & Compliance

- No DRM bypass, no license verification bypass, no encryption protection bypass
- No proprietary source code copying
- Open bundle format .hfbundle ZIP
- Clean-room implementation all shaders GLSL/HLSL clean-room examples not FaceUnity PROTECTED
- FaceUnity bundles 267 encrypted magic F3 5B 06 12 entropy 7.7-7.85 PROTECTED not decrypted
- Tools only for HuanFace .hfbundle open ZIP not FaceUnity encrypted (bundle_inspector in Phase 1 only did header/entropy analysis not decryption)

---

## 10. SDK Directory Structure (Phase 2 Proposed)

```
sdk/
├── include/
│   ├── huanface_c_api.h (C ABI)
│   ├── huanface.hpp (C++ RAII)
│   ├── huanface_frame.h
│   ├── huanface_face.h
│   ├── huanface_makeup.h
│   ├── huanface_beauty.h
│   ├── huanface_bundle.h
│   ├── huanface_rendering.h
│   ├── huanface_platform.h
│   └── huanface_version.h
├── src/
│   ├── core/
│   ├── face/
│   ├── makeup/
│   ├── beauty/
│   ├── bundle/
│   ├── rendering/
│   │   ├── d3d11/
│   │   └── opengl/
│   └── platform/
│       └── windows/
├── lib/
├── tools/
│   ├── huanface_bundle_packer.py (NEW Phase 2 for HuanFace .hfbundle ZIP)
│   ├── huanface_bundle_unpacker.py (NEW)
│   └── huanface_bundle_inspector.py (NEW)
├── examples/
│   ├── bundles/
│   │   ├── simple_lip/ (clean-room)
│   │   ├── simple_blush/
│   │   ├── simple_eyeshadow/
│   │   └── simple_foundation/
│   └── desktop_camera/ (future Phase 4)
└── tests/
```

Phase 2 only creates structure and spec placeholder files, no full implementation.

See docs/WINDOWS_SDK_ARCHITECTURE.md section 12 for full structure.

---

## 11. Next Steps

- Phase 2 remaining: create tools packer/unpacker/inspector for .hfbundle ZIP, example bundles clean-room assets, sdk directory structure placeholder, update ROADMAP, then commit architecture: define Windows SDK and HuanFace bundle format and STOP
- Phase 3+ (future): Minimal Prototype, Face Tracking, Makeup Renderer, Bundle Runtime, Beauty Engine, Compatibility API, Testing, SDK Packaging — but Phase 2 STOP says do NOT implement full SDK/renderer/tracker/makeup/API/UI before understanding repo, only ARCHITECTURE SPEC TOOLING EXAMPLES DOCS

---

**End of Architecture (Phase 2 Updated)**
