# Frame Pipeline — HuanFace (Phase 2)

**Status:** SPECIFICATION — no implementation in Phase 2  
**Target:** Windows 10+, x64, 1080p 30 FPS min 60 FPS target

---

## 1. HFFrame Abstraction

```cpp
enum class HFFormat {
    UNKNOWN = 0,
    RGBA8,      // 8-bit RGBA, 4 bytes per pixel, R in lowest byte? Actually RGBA order
    BGRA8,      // 8-bit BGRA, Windows D3D11 default (B,G,R,A)
    RGB8,       // 8-bit RGB, 3 bytes
    BGR8,       // 8-bit BGR
    NV12,       // YUV 4:2:0, 2 planes: Y plane (W*H) + UV interleaved plane (W*H/2)
    YUV420P,    // YUV 4:2:0, 3 planes: Y (W*H), U (W*H/4), V (W*H/4)
    R8,         // 8-bit single channel (mask, e.g., lip mask, eye mask)
    R32F,       // 32-bit float single channel (for advanced processing)
};

struct HFFrame {
    int width = 0;
    int height = 0;
    HFFormat format = HFFormat::UNKNOWN;
    int64_t timestampNanos = 0; // from IClock::NowNanos()
    
    // CPU data (optional, for CPU processing like face tracking)
    uint8_t* data = nullptr; // For RGBA8/BGRA8/RGB8/BGR8: main data; For NV12/YUV420P: Y plane; For R8/R32F: single channel
    uint8_t* dataU = nullptr; // For NV12: UV interleaved plane; For YUV420P: U plane
    uint8_t* dataV = nullptr; // For YUV420P: V plane
    int stride = 0; // bytes per row for data (may be > width*bytesPerPixel for alignment)
    int strideU = 0;
    int strideV = 0;
    
    // GPU data (optional, for GPU processing like makeup/beauty rendering, avoids CPU→GPU copy)
    IGpuTexture* gpuTexture = nullptr; // D3D11Texture or OpenGLTexture
    void* nativeHandle = nullptr; // ID3D11Texture2D* or GLuint for interop with external (e.g., OBS, Unity)
    
    // Ownership flags
    bool ownsData = false; // if true, HuanFace will free data via delete[] or custom allocator
    bool ownsGpuTexture = false; // if true, HuanFace will destroy gpuTexture
    
    // Metadata (optional)
    int rotation = 0; // 0, 90, 180, 270 — from OBS RotateClockwise evidence (Up, Right, Down, Left)
    bool isMirrored = false; // mirrored (front camera)
};
```

**Why separate CPU and GPU:**
- Face tracking (MediaPipe, ONNX) needs CPU image (data not null) — runs on CPU, needs RGBA or RGB
- Rendering (makeup, beauty) needs GPU texture (gpuTexture not null) — runs on GPU, needs D3D11 or OpenGL texture, avoids CPU→GPU copy if camera already outputs GPU texture (e.g., Media Foundation can output D3D11 texture directly, zero-copy)
- If both needed, copy once: CPU→GPU or GPU→CPU, but avoid per-frame alloc via pool

**Supported formats (from OBS evidence + Windows standard):**
- RGBA8, BGRA8, RGB8, BGR8 — common RGB formats, BGRA8 is D3D11 default (Windows)
- NV12 — Media Foundation camera default (YUV 4:2:0, 2 planes), efficient, hardware accelerated, OBS format_conversion.effect converts YUV→RGB
- YUV420P — alternative YUV, 3 planes
- R8 — for masks (lip mask, eye mask, blush mask) — single channel 8-bit
- R32F — for advanced (e.g., depth, or float mask)

**Rotation handling (from OBS evidence):**
- INI `RotateClockwise` with values `Up`, `Right`, `Down`, `Left` — corresponds to 0, 90, 180, 270
- `RotateClockwiseTips`: "If effects like slimming or background blur are not working properly, please try adjusting this setting to set the camera's original orientation."
- So camera may have orientation, and HuanFace should handle rotation via `HFFrame::rotation` field, not by rotating image data (expensive), but by adjusting face tracking and rendering (e.g., rotate landmarks, or set shader uniform for rotation)

---

## 2. Frame Flow

### 2.1 Camera Capture → Face Tracking → Beauty → Makeup → Output

```
Webcam (physical device)
  │
  ▼
IFrameSource (WindowsCamera_MF or DShow)
  ├── Open(deviceId, width, height, format)
  │     └── Media Foundation: MFCreateMediaSource, IMFSourceReader, supports NV12 and RGBA
  │     └── DirectShow: Filter Graph, Sample Grabber, supports RGB
  ├── Read(HFFrame& outFrame)
  │     ├── For MF: if GPU backend D3D11, try to get D3D11 texture directly from MF (zero-copy) → HFFrame with gpuTexture not null, data null
  │     └── For DShow or if CPU needed: get CPU data (RGBA or NV12) → HFFrame with data not null, gpuTexture null
  └── Close()
  │
  ▼
HFFrame (input)
  │
  ├── If GPU texture: for face tracking, need CPU data → copy GPU→CPU via Map (D3D11) or glReadPixels (OpenGL) → HFFrame with both data and gpuTexture, or convert to CPU-only for tracking
  └── If CPU data: for rendering, need GPU texture → copy CPU→GPU via CreateTexture or UpdateTexture → HFFrame with both or GPU-only
  │
  ▼
Face Engine (IFaceTracker)
  ├── Input: HFFrame (CPU data, RGBA or RGB)
  ├── Process: face detection, landmarks, mesh, expression, pose
  └── Output: HFTrackingData (faces with bbox, landmarks, rotation, translation, scale, confidence, mesh)
  │
  ▼
Beauty Engine (BeautyEngine)
  ├── Input: HFFrame (input) + HFTrackingData (face data)
  ├── Process:
  │   ├── Skin Smooth (HeavyBlur) — bilateral filter on GPU, using face mask (skin region from landmarks/mesh) to only smooth skin
  │   ├── Whitening (ColorLevel) — color adjustment (RGB curve) on GPU
  │   ├── Sharpen, Clarity, Tone (Brightness, Saturation, RedLevel)
  │   ├── Face Shape (FaceThreed, FaceSize) — mesh warp on GPU (move vertices based on landmarks)
  │   ├── Eye, Nose, Chin adjustments — mesh warp
  │   └── Blemish Removal (DelspotLevel), Eye Bright, Tooth Whiten, Dark Circle, Nasolabial Fold, etc.
  └── Output: HFFrame (beautified, CPU or GPU)
  │
  ▼
Makeup Engine (MakeupEngine)
  ├── Input: HFFrame (beautified or original) + HFTrackingData + Bundles (textures, masks, shaders, params)
  ├── Process (for each makeup category, configurable render graph):
  │   ├── Mask Generation:
  │   │   ├── Lip: lip polygon from landmarks → lip_mask (R8, blur for soft edge) + occu mask (mouth, teeth) + highlight mask
  │   │   ├── Eye: eye mask from landmarks
  │   │   ├── Eyebrow: brow mask
  │   │   ├── Blush: cheek region mask
  │   │   ├── Foundation: face mask (skin region)
  │   │   └── etc.
  │   ├── Texture Sampling: from Bundle (textures/lip.png, eye.png, etc. via ResourceManager cache)
  │   ├── Makeup Shader (GLSL/HLSL clean-room):
  │   │   ├── Vertex: transform face mesh (mvp uniform)
  │   │   └── Fragment: sample base image, makeup texture, mask, apply color (makeup_*_color), intensity (makeup_intensity_*), opacity, blend mode (blend_type_*)
  │   ├── Blend: Normal, Multiply, Screen, Overlay, etc.
  │   └── Composite: MakeupPipeline composite all layers (foundation → blush → eyeshadow → eyebrow → eyeliner → eyelash → pupil → lip → highlight → shadow, but order configurable via manifest.json passes)
  └── Output: HFFrame (with makeup)
  │
  ▼
Rendering Engine (IRenderBackend)
  ├── Input: HFFrame (with makeup/beauty) + HFTrackingData
  ├── Process:
  │   ├── SetRenderTarget (output texture or backbuffer)
  │   ├── Clear
  │   ├── DrawMesh (face mesh, makeup meshes) with shaders and uniforms (intensity, color, textures, masks)
  │   ├── Blit (full-screen quad for beauty effects like skin smooth which is full-screen filter)
  │   └── Present (if windowed)
  └── Output: HFFrame (final output, GPU texture or CPU data)
  │
  ▼
Output Frame (HFFrame)
  ├── For display: if windowHandle provided, Present() to window
  ├── For file: if CPU data, save to PNG/JPG via stb_image_write
  ├── For interop: if nativeHandle (ID3D11Texture2D* or GLuint), share with external (e.g., Unity, Unreal, custom app) — NOT OBS Spout (OBS is EXTERNAL REFERENCE, not dependency, but HuanFace can support Spout as optional platform extension, not core)
  └── For further processing: pass to next pipeline stage
```

### 2.2 Image Input (Not Just Camera)

SDK must also accept:

```cpp
// Image file (PNG, JPG)
HFFrame frame;
frame.width = 1280;
frame.height = 720;
frame.format = HFFormat::RGBA8;
frame.data = stbi_load("input.png", &w, &h, &channels, 4); // via stb_image
frame.stride = w * 4;
frame.ownsData = true;
engine.ProcessFrame(frame, outputFrame);

// Video file
IFrameSource* videoSource = new VideoFileSource(); // MF or FFmpeg
videoSource->OpenFile("input.mp4");
while (videoSource->Read(frame)) {
    engine.ProcessFrame(frame, outputFrame);
}

// GPU Texture (from Unity, Unreal, custom app)
HFFrame frame;
frame.width = 1280;
frame.height = 720;
frame.format = HFFormat::BGRA8; // D3D11 default
frame.gpuTexture = gpuDevice->CreateTexture(1280, 720, HFFormat::BGRA8, nullptr); // or wrap existing ID3D11Texture2D*
frame.nativeHandle = existingD3D11Texture; // ID3D11Texture2D* from Unity
frame.ownsGpuTexture = false; // external owns
engine.ProcessFrame(frame, outputFrame);
```

**Why:** SDK should not only depend on webcam, but also support image, video, GPU texture for flexibility (e.g., Unity plugin, Unreal plugin, custom app, testing with image files)

---

## 3. Performance Design

**Target:**
- 1080p (1920x1080)
- 30 FPS minimum
- 60 FPS target

**Design considerations (documented, not implemented in Phase 2):**

**GPU texture reuse:**
- Pool of textures (e.g., 3 textures for triple buffering) to avoid per-frame allocation
- `ResourceManager` caches textures, shaders, meshes with LRU eviction
- `HFFrame` with `ownsGpuTexture=false` for external textures (zero-copy)

**Async processing:**
- Face tracking in separate thread (IFrameSource Read in camera thread, face tracking in tracking thread, rendering in main thread)
- Double-buffered face data: tracking thread writes to back buffer, rendering thread reads from front buffer, swap via mutex
- Example: camera 30 FPS, face tracking 30 FPS, rendering 60 FPS (interpolate face data if needed)

**Resource cache:**
- Textures: cache PNG loaded via stb_image, upload to GPU once, reuse
- Shaders: compile once (HLSL via D3DCompile, GLSL via glCompileShader), cache program/binary, reuse
- Meshes: face mesh from landmarks, but mesh topology (indices) constant, only vertices change per frame, so create mesh once with dynamic vertex buffer (D3D11_USAGE_DYNAMIC, GL_DYNAMIC_DRAW)

**Shader cache:**
- For D3D11: cache compiled bytecode (ID3DBlob) to file (like OBS shader-cache .v2 but clean-room)
- For OpenGL: cache program binary via glGetProgramBinary, glProgramBinary

**Frame synchronization:**
- CPU/GPU sync: avoid stall (e.g., don't Map D3D11 texture with D3D11_MAP_FLAG_DO_NOT_WAIT? Actually use D3D11_MAP_WRITE_DISCARD for dynamic)
- For GPU→CPU copy (for face tracking if camera outputs GPU texture), use async readback with 1-frame delay (read previous frame's GPU texture while rendering current)

**Memory reuse:**
- Pool allocators for face data (HFFaceData, HFTrackingData, HFFaceMesh) — avoid per-frame new/delete
- HFFrame data pool: allocate once for max size (1080p RGBA = 1920*1080*4 = 8.29 MB), reuse

**Format handling:**
- BGRA8 for D3D11 to avoid conversion (D3D11 default is BGRA8)
- NV12 for Media Foundation camera to avoid conversion (MF default NV12, can be directly used for face tracking if tracking supports NV12, or convert to RGBA via shader for rendering)
- R8 for masks (single channel, efficient)

---

## 4. No OBS Dependency

**Explicitly NOT in frame pipeline:**
- obs.dll
- obsplus.dll
- Spout.dll (texture sharing) — OBS uses Spout for sharing beauty output to other apps, but HuanFace SDK core does NOT depend on Spout, Spout can be optional platform extension (e.g., `SpoutFrameSource` or `SpoutOutput`) but not core
- OBS effects (default.effect, format_conversion.effect) — OBS built-in, not FaceUnity, HuanFace shaders are clean-room GLSL/HLSL
- OBS shader-cache .v2

**OBS is EXTERNAL REFERENCE only for research (frame flow, integration clues, etc.), not runtime dependency.**

**HuanFace frame pipeline dependencies (clean-room, open):**
- Windows: Media Foundation (MFPlat, MFReadWrite) for camera, D3D11 for GPU, OpenGL for alternative
- Standard C++ library
- Open third-party: stb_image (MIT) for PNG/JPG loading, miniz (MIT) for ZIP, nlohmann/json (MIT) for manifest, glm (MIT) for math, MediaPipe (Apache 2.0) or ONNX Runtime (MIT) for face tracking

---

---

## Phase 4 Implementation Status

**IMPLEMENTED:**
- HFFrameC width/height/format/timestamp/stride/CPU data/GPU texture/rotation/mirrored, formats RGBA8/BGRA8/RGB8/NV12/R8/R32F, CPU vs GPU separation, rotation 0/90/180/270 validated
- FrameValidator Validate checks null, dimensions, format, rotation, has CPU or GPU, stride >= width*bpp, NV12 dataU required, YUV420P dataU+V required
- ImageLoader LoadImage/SaveImage RGBA8 PNG via zlib + BMP, EncodePNGToMemory, IsPNG/BMP/JPG
- Engine pipeline HF_ProcessFrame Validate->FaceTracker (SimpleFaceTracker real) -> FaceMaskGenerator (face+lip R8 triangle rasterization) -> Beauty pass-through -> Makeup (lip lerp) -> RenderBackend (Null CPU on Linux, D3D11 texture real PNG + shader compile on Windows) -> Output owned data allocation
- Memory ownership Borrowed (input not owned) / Owned (output new[] ownsData=1, free via HF_FreeFrame) / GPU (ComPtr, ownsGpuTexture), no double free/use-after-free/dangling, ResourceManager load once reuse
- basic_face_demo CLI input.jpg output.png with debug outputs output_face/mask/makeup/final.png, console Input/Faces/bbox/landmarks/confidence/Bundle/Makeup/Output/Backend/Status
- Tests for image, face, mask, shader, texture, bundle, integration, perf measure ms FPS baseline 3.2ms avg 312 FPS for 200x200, 28ms for 512x512

**PARTIAL:**
- JPG not supported, only PNG/BMP
- NV12/RGB8/BGR8/YUV420P/R8/R32F validated but makeup only supports RGBA8/BGRA8, others fallback pass-through
- D3D11 backend only Windows, Linux CI Null + CPU
- No webcam, no video file source, no GPU texture input wrapping yet

**STUB:**
- BeautyEngine pass-through
- OpenGL backend stub
- Platform camera MF/DShow placeholders

**NOT IMPLEMENTED:**
- Webcam capture Media Foundation IMFSourceReader / DirectShow, VideoFileSource, GPU texture wrapping Unity/Unreal, image file source via stb_image (PNG via zlib only), performance 1080p 60 FPS with GPU reuse pool

**End of Frame Pipeline**
