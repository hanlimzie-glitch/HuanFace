# Platform Abstraction — HuanFace (Phase 2)

**Status:** SPECIFICATION — no implementation in Phase 2  
**Target:** Windows 10+, Windows 11+, x64

---

## 1. Goal

Core engine MUST NOT directly depend on Win32, D3D11, OpenGL, Media Foundation, etc. Use interfaces.

**Why:**
- Swappable implementations (e.g., Media Foundation vs DirectShow for camera)
- Testable (mock implementations for unit tests)
- Portable (future Linux, macOS, Android, iOS — only platform layer changes)
- Clean boundary: Application → HuanFace SDK → Platform → OS/GPU

---

## 2. Interfaces (PROPOSED)

### 2.1 IFrameSource — Camera, Video, Image

```cpp
enum class HFResult {
    OK = 0,
    FAIL = -1,
    NOT_INITIALIZED = -2,
    INVALID_PARAM = -3,
    NOT_SUPPORTED = -4,
    DEVICE_NOT_FOUND = -5,
    DEVICE_BUSY = -6,
};

class IFrameSource {
public:
    virtual ~IFrameSource() = default;
    virtual HFResult Open(int deviceId, int width, int height, HFFormat format) = 0;
    virtual HFResult OpenFile(const std::string& path) = 0; // video file
    virtual HFResult Read(HFFrame& outFrame) = 0;
    virtual HFResult Close() = 0;
    virtual bool IsOpened() const = 0;
    virtual int GetWidth() const = 0;
    virtual int GetHeight() const = 0;
    virtual HFFormat GetFormat() const = 0;
};
```

**Windows Implementations (PROPOSED, not implemented in Phase 2):**
- `WindowsCamera_MF` — Media Foundation (Windows 10+ recommended, supports 1080p, hardware acceleration)
  - Uses `MFCreateMediaSource`, `IMFMediaSource`, `IMFSourceReader`, `MFCreateSourceReaderFromMediaSource`
  - Pros: Modern, hardware accel, 1080p, efficient
  - Cons: Windows 10+ only, more complex
- `WindowsCamera_DShow` — DirectShow (fallback for Windows 7/8, or if MF fails)
  - Uses `CoCreateInstance(CLSID_FilterGraph)`, `ICaptureGraphBuilder2`, `IBaseFilter`, `ISampleGrabber`
  - Pros: Works on older Windows, simpler
  - Cons: Deprecated, less efficient, no hardware accel
- `ImageFileSource` — load image file (PNG, JPG via stb_image) for testing
- `VideoFileSource` — load video file via Media Foundation or FFmpeg (future)

**See:** `docs/FRAME_PIPELINE.md` for HFFrame definition

---

### 2.2 IGpuDevice, IGpuTexture, IRenderTarget, IShader, IMesh

```cpp
enum class HFFormat {
    UNKNOWN,
    RGBA8,
    BGRA8,
    RGB8,
    NV12,
    YUV420P,
    R8,
    R32F,
};

struct HFColor {
    float r, g, b, a;
};

struct HFVec2 {
    float x, y;
};

struct HFVec3 {
    float x, y, z;
};

struct HFVec4 {
    float x, y, z, w;
};

struct HFMeshVertex {
    HFVec3 position;
    HFVec3 normal;
    HFVec2 texCoord;
};

class IGpuTexture {
public:
    virtual ~IGpuTexture() = default;
    virtual int GetWidth() const = 0;
    virtual int GetHeight() const = 0;
    virtual HFFormat GetFormat() const = 0;
    virtual void* GetNativeHandle() const = 0; // ID3D11Texture2D* or GLuint
    virtual void* GetShaderResourceView() const = 0; // ID3D11ShaderResourceView* or GLuint
};

class IRenderTarget {
public:
    virtual ~IRenderTarget() = default;
    virtual IGpuTexture* GetTexture() = 0;
    virtual void* GetRenderTargetView() const = 0; // ID3D11RenderTargetView* or GLuint FBO
    virtual void Bind() = 0;
    virtual void Unbind() = 0;
};

class IShader {
public:
    virtual ~IShader() = default;
    virtual void* GetNativeHandle() const = 0; // ID3D11VertexShader* + PixelShader* or GLuint program
    virtual bool SetUniform(const std::string& name, float value) = 0;
    virtual bool SetUniform(const std::string& name, const HFVec2& value) = 0;
    virtual bool SetUniform(const std::string& name, const HFVec3& value) = 0;
    virtual bool SetUniform(const std::string& name, const HFVec4& value) = 0;
    virtual bool SetUniform(const std::string& name, const HFColor& value) = 0;
    virtual bool SetTexture(const std::string& name, IGpuTexture* texture, int slot) = 0;
};

class IMesh {
public:
    virtual ~IMesh() = default;
    virtual void* GetNativeHandle() const = 0; // ID3D11Buffer* or GLuint VBO/EBO
    virtual int GetVertexCount() const = 0;
    virtual int GetIndexCount() const = 0;
};

class IGpuDevice {
public:
    virtual ~IGpuDevice() = default;
    virtual bool Init(void* windowHandle = nullptr) = 0;
    virtual IGpuTexture* CreateTexture(int w, int h, HFFormat format, const void* data, int stride) = 0;
    virtual IRenderTarget* CreateRenderTarget(int w, int h, HFFormat format) = 0;
    virtual IShader* CreateShader(const std::string& vsSrc, const std::string& fsSrc) = 0;
    virtual IMesh* CreateMesh(const std::vector<HFMeshVertex>& vertices, const std::vector<int>& indices) = 0;
    virtual void DestroyTexture(IGpuTexture* tex) = 0;
    virtual void DestroyRenderTarget(IRenderTarget* rt) = 0;
    virtual void DestroyShader(IShader* shader) = 0;
    virtual void DestroyMesh(IMesh* mesh) = 0;
    virtual void Shutdown() = 0;
};
```

**Windows Implementations (PROPOSED):**
- `D3D11GpuDevice`, `D3D11Texture`, `D3D11RenderTarget`, `D3D11Shader`, `D3D11Mesh`
- `OpenGLGpuDevice`, `OpenGLTexture`, `OpenGLRenderTarget`, `OpenGLShader`, `OpenGLMesh`

**See:** `docs/RENDER_BACKEND.md` for details

---

### 2.3 IClock — Timing

```cpp
class IClock {
public:
    virtual ~IClock() = default;
    virtual int64_t NowNanos() = 0; // nanoseconds since epoch or since start
    virtual double NowMillis() = 0; // milliseconds
    virtual double NowSeconds() = 0;
    virtual void SleepMillis(int millis) = 0;
};
```

**Windows Implementations:**
- `WindowsClock` — uses `QueryPerformanceCounter`, `QueryPerformanceFrequency` for high-res timing
- `StdChronoClock` — uses `std::chrono::high_resolution_clock` (portable fallback)

---

### 2.4 IFileSystem — File I/O

```cpp
class IFileSystem {
public:
    virtual ~IFileSystem() = default;
    virtual bool ReadFile(const std::string& path, std::vector<uint8_t>& outData) = 0;
    virtual bool WriteFile(const std::string& path, const void* data, size_t size) = 0;
    virtual bool Exists(const std::string& path) = 0;
    virtual bool IsDirectory(const std::string& path) = 0;
    virtual bool CreateDirectory(const std::string& path) = 0;
    virtual std::vector<std::string> ListFiles(const std::string& dir, const std::string& extension = "") = 0;
};
```

**Windows Implementations:**
- `WindowsFileSystem` — uses `CreateFileW`, `ReadFile`, `WriteFile`, `GetFileAttributesW`, `FindFirstFileW`, etc., with UTF-8 to UTF-16 conversion
- `StdFileSystem` — uses `std::ifstream`, `std::ofstream`, `std::filesystem` (C++17) for portable fallback

**For ZIP (.hfbundle):**
- Uses `miniz` (MIT) — single header ZIP library, no dependency on Windows ZIP API
- `IFileSystem` can read ZIP via miniz: `mz_zip_reader_extract_file_to_mem`, etc.

---

### 2.5 IThreading — Threading & Thread Pool

```cpp
class IThread {
public:
    virtual ~IThread() = default;
    virtual void Start(std::function<void()> func) = 0;
    virtual void Join() = 0;
    virtual bool IsRunning() const = 0;
};

class IMutex {
public:
    virtual ~IMutex() = default;
    virtual void Lock() = 0;
    virtual void Unlock() = 0;
};

class IConditionVariable {
public:
    virtual ~IConditionVariable() = default;
    virtual void Wait(IMutex* mutex) = 0;
    virtual void NotifyOne() = 0;
    virtual void NotifyAll() = 0;
};

class IThreadPool {
public:
    virtual ~IThreadPool() = default;
    virtual void Init(int numThreads) = 0;
    virtual void Submit(std::function<void()> task) = 0;
    virtual void Shutdown() = 0;
};
```

**Windows Implementations:**
- `StdThread`, `StdMutex`, `StdConditionVariable` — uses `std::thread`, `std::mutex`, `std::condition_variable` (portable)
- `WindowsThreadPool` — uses `CreateThreadpool`, `SubmitThreadpoolWork`, or custom thread pool with `std::thread`

**For HuanFace:**
- Face tracking in separate thread (async)
- Rendering in main thread (GPU context)
- Thread pool for parallel makeup/beauty processing (optional)

---

## 3. Platform Layer Diagram

```
Application
    │
    ▼
HuanFace SDK (Core, Face, Makeup, Beauty, Bundle, Rendering)
    │
    ├── IFrameSource
    │     ├── WindowsCamera_MF (Media Foundation) [P0]
    │     ├── WindowsCamera_DShow (DirectShow) [fallback]
    │     ├── ImageFileSource (stb_image)
    │     └── VideoFileSource (MF or FFmpeg)
    │
    ├── IGpuDevice, IGpuTexture, IRenderTarget, IShader, IMesh
    │     ├── D3D11GpuDevice etc. [P0 Windows]
    │     └── OpenGLGpuDevice etc. [P1, evidence CNamaSDK uses OpenGL 4.6]
    │
    ├── IClock
    │     ├── WindowsClock (QueryPerformanceCounter)
    │     └── StdChronoClock
    │
    ├── IFileSystem
    │     ├── WindowsFileSystem (Win32)
    │     └── StdFileSystem (std::filesystem)
    │     └── miniz for ZIP (.hfbundle)
    │
    └── IThreading
          ├── StdThread, StdMutex, StdConditionVariable
          └── WindowsThreadPool / StdThreadPool
    │
    ▼
OS / GPU
- Windows 10+ SDK (Win32, D3D11, DXGI, Media Foundation)
- OpenGL 4.6 (opengl32.dll)
- Standard C++ library
```

---

## 4. Windows-Specific Considerations

**Camera:**
- Media Foundation requires Windows 10+ (MFPlat, MFReadWrite, MFSrcSn)
- DirectShow fallback uses quartz.dll, strmiids.lib
- Need to handle camera permission (Windows 10 privacy settings)
- Camera-tool.exe evidence: SetupDi* for device enumeration, requireAdministrator — for diagnosing camera occupation, not for normal capture

**GPU:**
- D3D11: d3d11.lib, dxgi.lib, D3DCompiler.lib for HLSL compilation
- OpenGL: opengl32.lib, glad (MIT) for loader, wgl for context creation
- Need to handle GPU device lost (like fuOnDeviceLost) — D3D11 device removed, OpenGL context lost

**FileSystem:**
- UTF-8 to UTF-16 conversion for Win32 (Windows uses UTF-16 for file paths)
- Long path support (\\?\ prefix)
- ZIP via miniz, no Windows ZIP dependency

**Threading:**
- std::thread is portable and sufficient for most cases
- For thread pool, use std::thread + queue + mutex + condition_variable

**Timing:**
- QueryPerformanceCounter for high-res, fallback to std::chrono

---

## 5. No OBS Dependency

**Explicitly NOT in platform layer:**
- obs.dll
- obsplus.dll
- Spout.dll
- OBS effects
- OBS shader-cache

**OBS is EXTERNAL REFERENCE only, not platform dependency.**

---

**End of Platform Abstraction**
