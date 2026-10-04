# API Design — HuanFace SDK (Phase 2 Updated)

**Status:** SPECIFICATION — Phase 2, no implementation, architecture only  
**Target:** Windows 10+, x64, Native C/C++ DLL, NO OBS dependency

See `docs/SDK_API_SPEC.md` for full C ABI + C++ RAII spec.

---

## 1. OBS Exclusion Rule

OBS is EXTERNAL REFERENCE only, NOT runtime dependency. Core SDK MUST NOT include obs.dll, obsplus.dll, Spout, OBS effects.

---

## 2. Public API Overview (Phase 2)

**C ABI (huanface_c_api.h) — stable for FFI:**

```
HF_Init()
HF_Shutdown()
HF_CreateEngine(config, outEngine)
HF_DestroyEngine(engine)
HF_LoadBundle(engine, path, outBundle)
HF_LoadBundleFromMemory(engine, data, size, outBundle)
HF_UnloadBundle(engine, bundle)
HF_SetParameterFloat(engine, name, value)
HF_SetParameterInt(engine, name, value)
HF_SetParameterBool(engine, name, value)
HF_SetParameterColor(engine, name, color)
HF_SetParameterVec2/3/4
HF_SetParameterTexture
HF_SetParameterEnum
HF_GetParameterFloat/Int/Bool/Color/etc.
HF_ProcessFrame(engine, inputFrame, outFrame)
HF_ProcessFrameWithBundle(engine, inputFrame, bundle, outFrame)
HF_GetFaceData(engine, outTrackingData)
HF_FreeFaceData(trackingData)
HF_FreeFrame(frame)
HF_GetVersion()
HF_GetResultString(result)
```

**C++ RAII (huanface.hpp) — header-only wrapper:**

```
namespace huanface {
  class Engine { Engine(config); LoadBundle(path); SetParameter(name,value); ProcessFrame(input); GetFaceData(); }
  class Bundle { }
  struct Frame { ToC(); }
  struct FaceData { }
  struct TrackingData { }
  struct EngineConfig { ToC(); }
  Init(); Shutdown(); GetVersion();
}
```

**Frame abstraction:** HFFrame width/height/format/timestamp/data/stride/GPU texture/nativeHandle/ownership/rotation/mirrored, formats RGBA8/BGRA8/RGB8/NV12/R8/R32F, CPU vs GPU separation for performance

**Face abstraction:** IFaceTracker swappable MediaPipe/ONNX, HFFaceData bbox/landmarks runtime-defined not hardcoded 68/106, rotation/translation/scale/confidence/mesh/expression, HFFaceMesh vertices/indices/UV/normals/landmarkToVertex

**Makeup abstraction:** generic HFMakeupParameter name/type/value min/max/default only if evidence, grouped Lip/Eye/Brow/Blush/Foundation/etc., render graph configurable passes

**Beauty abstraction:** BeautyEngine separate, HFBeautyParameter HeavyBlur/ColorLevel/etc. 0-100 from INI evidence, pipeline skin smooth bilateral filter whitening face shape warp etc.

**Bundle abstraction:** .hfbundle ZIP open format manifest.json + textures PNG + shaders GLSL/HLSL clean-room + meshes JSON/OBJ + metadata thumbnail, tools packer/unpacker/inspector only for HuanFace .hfbundle not FaceUnity encrypted

**Rendering abstraction:** IRenderBackend Init/Shutdown/CreateTexture/RenderTarget/Shader/Mesh/DrawMesh/Blit/Present/SetBlendMode, D3D11 P0 HLSL, OpenGL P1 GLSL 4.6 evidence CNamaSDK uses GL 4.6

**Platform abstraction:** IFrameSource Open/Read/Close, IGpuDevice CreateTexture/RenderTarget/Shader/Mesh Destroy, IGpuTexture, IRenderTarget, IShader, IMesh, IClock NowNanos, IFileSystem Read/Write/Exists/List, IThreading Thread/Mutex/CondVar/ThreadPool, Windows impls MF/DShow/D3D11/OpenGL/QPC

---

## 3. Desktop Camera Pipeline Example

```
Webcam → IFrameSource (WindowsCamera_MF) → HFFrame → IFaceTracker → HFTrackingData → BeautyEngine → MakeupEngine → IRenderBackend → Output HFFrame → Display/File/Interop
Also support Image/Video/GPU Texture
```

See `docs/FRAME_PIPELINE.md` and `docs/SDK_API_SPEC.md` section 5.

---

## 4. SDK Directory Structure

See `docs/WINDOWS_SDK_ARCHITECTURE.md` section 12 and `docs/SDK_API_SPEC.md` section 4.

```
sdk/include/huanface_c_api.h, huanface.hpp, etc.
sdk/src/core/face/makeup/beauty/bundle/rendering/d3d11/opengl/platform/windows/
tools/huanface_bundle_packer/unpacker/inspector.py (Phase 2 NEW for HuanFace .hfbundle ZIP)
examples/bundles/simple_lip/blush/eyeshadow/foundation (Phase 2 clean-room assets)
```

Phase 2 only spec + tooling + examples + placeholder structure, no full implementation.

---

## 5. No OBS Dependency

Explicitly NOT in SDK API: obs.dll, obsplus.dll, Spout, OBS effects.

OBS is EXTERNAL REFERENCE only for research.

SDK dependencies open: Windows SDK, D3D11, OpenGL, stb_image MIT, miniz MIT, nlohmann/json MIT, glm MIT, MediaPipe Apache 2.0 or ONNX Runtime MIT.

---

**End of API Design (Phase 2 Updated)**
