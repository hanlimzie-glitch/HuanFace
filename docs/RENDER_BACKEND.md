# Render Backend — HuanFace (Phase 2)

**Status:** SPECIFICATION — no implementation in Phase 2  
**Target:** Windows 10+, x64, D3D11 P0, OpenGL P1

---

## 1. IRenderBackend Abstraction

```cpp
enum class HFResult { OK, FAIL, NOT_INITIALIZED, INVALID_PARAM, NOT_SUPPORTED };

class IRenderBackend {
public:
    virtual ~IRenderBackend() = default;
    virtual HFResult Init(void* windowHandle = nullptr) = 0;
    virtual HFResult Shutdown() = 0;
    
    // Texture
    virtual IGpuTexture* CreateTexture(int w, int h, HFFormat format, const void* data, int stride) = 0;
    virtual IGpuTexture* CreateTextureFromFile(const std::string& path) = 0; // PNG via stb_image
    virtual void DestroyTexture(IGpuTexture* tex) = 0;
    
    // Render Target
    virtual IRenderTarget* CreateRenderTarget(int w, int h, HFFormat format) = 0;
    virtual void DestroyRenderTarget(IRenderTarget* rt) = 0;
    virtual void SetRenderTarget(IRenderTarget* rt) = 0; // null = default backbuffer
    virtual void Clear(float r, float g, float b, float a) = 0;
    
    // Shader
    virtual IShader* CreateShader(const std::string& vsSrc, const std::string& fsSrc) = 0;
    virtual IShader* CreateShaderFromFile(const std::string& vsPath, const std::string& fsPath) = 0;
    virtual void DestroyShader(IShader* shader) = 0;
    
    // Mesh
    virtual IMesh* CreateMesh(const std::vector<HFMeshVertex>& vertices, const std::vector<int>& indices) = 0;
    virtual void DestroyMesh(IMesh* mesh) = 0;
    
    // Drawing
    virtual void DrawMesh(IMesh* mesh, IShader* shader, const std::map<std::string, Uniform>& uniforms) = 0;
    virtual void Blit(IGpuTexture* src, IRenderTarget* dst) = 0; // full-screen quad blit
    virtual void Present() = 0; // for windowed rendering
    
    // State
    virtual void SetBlendMode(HFBlendMode mode) = 0;
    virtual void SetDepthTest(bool enable) = 0;
    virtual void SetCullMode(HFCullMode mode) = 0;
};
```

**Uniform variant:**
```cpp
struct Uniform {
    enum Type { FLOAT, VEC2, VEC3, VEC4, COLOR, INT, TEXTURE };
    Type type;
    float f;
    HFVec2 v2;
    HFVec3 v3;
    HFVec4 v4;
    HFColor color;
    int i;
    IGpuTexture* texture;
    int textureSlot;
};

enum class HFBlendMode {
    OPAQUE,
    ALPHA_BLEND, // srcAlpha, invSrcAlpha
    ADDITIVE,
    MULTIPLY,
};

enum class HFCullMode {
    NONE,
    FRONT,
    BACK,
};
```

---

## 2. D3D11 Backend (P0, Windows Priority)

**Why P0:**
- Windows 10+ native, efficient, hardware acceleration
- OBS shader-cache .v2 is D3D11 bytecode cache (223 files) — evidence D3D11 used in OBS environment (EXTERNAL REFERENCE)
- Better integration with Media Foundation camera (which can output D3D11 texture directly, zero-copy)
- No need for glad loader, uses system d3d11.dll

**Classes:**

```cpp
class D3D11Texture : public IGpuTexture {
    ID3D11Texture2D* texture = nullptr;
    ID3D11ShaderResourceView* srv = nullptr;
    int width, height;
    HFFormat format;
    void* GetNativeHandle() const override { return texture; }
    void* GetShaderResourceView() const override { return srv; }
};

class D3D11RenderTarget : public IRenderTarget {
    ID3D11Texture2D* texture = nullptr;
    ID3D11RenderTargetView* rtv = nullptr;
    ID3D11ShaderResourceView* srv = nullptr;
    D3D11Texture* hfTexture = nullptr; // wrapper
    void Bind() override { deviceContext->OMSetRenderTargets(1, &rtv, nullptr); }
    void Unbind() override { /* set null */ }
};

class D3D11Shader : public IShader {
    ID3D11VertexShader* vs = nullptr;
    ID3D11PixelShader* ps = nullptr;
    ID3D11InputLayout* inputLayout = nullptr;
    ID3D11Buffer* constantBuffer = nullptr; // for uniforms
    std::map<std::string, int> uniformToSlot;
    bool SetUniform(const std::string& name, float value) override { /* update constant buffer */ }
    bool SetTexture(const std::string& name, IGpuTexture* tex, int slot) override { /* PSSetShaderResources */ }
};

class D3D11Mesh : public IMesh {
    ID3D11Buffer* vertexBuffer = nullptr;
    ID3D11Buffer* indexBuffer = nullptr;
    int vertexCount, indexCount;
};

class D3D11Backend : public IRenderBackend {
    ID3D11Device* device = nullptr;
    ID3D11DeviceContext* context = nullptr;
    IDXGISwapChain* swapChain = nullptr; // optional, for windowed
    ID3D11RenderTargetView* backbufferRTV = nullptr;
    
    HFResult Init(void* windowHandle) override {
        // windowHandle is HWND
        // D3D11CreateDeviceAndSwapChain
        // Create backbuffer RTV if windowHandle not null
        // For offscreen rendering (no window), only device and context, no swapchain
    }
    
    IGpuTexture* CreateTexture(int w, int h, HFFormat format, const void* data, int stride) override {
        // D3D11_TEXTURE2D_DESC
        // format mapping: RGBA8 → DXGI_FORMAT_R8G8B8A8_UNORM, BGRA8 → DXGI_FORMAT_B8G8R8A8_UNORM, R8 → DXGI_FORMAT_R8_UNORM, R32F → DXGI_FORMAT_R32_FLOAT
        // CreateTexture2D, CreateShaderResourceView
    }
    
    IShader* CreateShader(const std::string& vsSrc, const std::string& fsSrc) override {
        // vsSrc is HLSL vertex shader source, fsSrc is HLSL pixel shader source
        // Compile via D3DCompile (D3DCompiler.lib)
        // CreateVertexShader, CreatePixelShader, CreateInputLayout
    }
    
    IMesh* CreateMesh(const std::vector<HFMeshVertex>& vertices, const std::vector<int>& indices) override {
        // Create vertex buffer (D3D11_BIND_VERTEX_BUFFER)
        // Create index buffer (D3D11_BIND_INDEX_BUFFER)
    }
    
    void DrawMesh(IMesh* mesh, IShader* shader, const std::map<std::string, Uniform>& uniforms) override {
        // Set shaders: VSSetShader, PSSetShader
        // Set uniforms: update constant buffer, PSSetConstantBuffers
        // Set textures: PSSetShaderResources
        // Set vertex buffer, index buffer, input layout
        // DrawIndexed
    }
    
    void Blit(IGpuTexture* src, IRenderTarget* dst) override {
        // Full-screen quad blit via DrawMesh with quad mesh and blit shader
    }
};
```

**HLSL Shader Example (clean-room, not copying FaceUnity or OBS):**

```hlsl
// vs.hlsl
cbuffer Uniforms : register(b0) {
    float4x4 u_mvp;
};

struct VSInput {
    float3 pos : POSITION;
    float3 normal : NORMAL;
    float2 uv : TEXCOORD0;
};

struct PSInput {
    float4 pos : SV_POSITION;
    float3 normal : NORMAL;
    float2 uv : TEXCOORD0;
};

PSInput main(VSInput input) {
    PSInput output;
    output.pos = mul(float4(input.pos, 1.0), u_mvp);
    output.normal = input.normal;
    output.uv = input.uv;
    return output;
}

// ps.hlsl (makeup)
Texture2D u_inputTexture : register(t0);
Texture2D u_makeupTexture : register(t1);
Texture2D u_maskTexture : register(t2);
SamplerState u_sampler : register(s0);

cbuffer Uniforms : register(b0) {
    float4 u_makeupColor;
    float u_intensity;
    float u_opacity;
    int u_blendMode;
};

struct PSInput {
    float4 pos : SV_POSITION;
    float3 normal : NORMAL;
    float2 uv : TEXCOORD0;
};

float4 main(PSInput input) : SV_TARGET {
    float4 base = u_inputTexture.Sample(u_sampler, input.uv);
    float4 makeup = u_makeupTexture.Sample(u_sampler, input.uv);
    float mask = u_maskTexture.Sample(u_sampler, input.uv).r;
    
    float3 coloredMakeup = makeup.rgb * u_makeupColor.rgb;
    float alpha = makeup.a * mask * u_intensity * u_opacity;
    
    float3 blended = lerp(base.rgb, coloredMakeup, alpha);
    
    return float4(blended, base.a);
}
```

**Pros:**
- Native Windows, efficient
- Media Foundation can output D3D11 texture directly (zero-copy from camera to GPU)
- Hardware acceleration, 1080p 60 FPS achievable
- No glad loader needed

**Cons:**
- Windows only
- HLSL compilation requires D3DCompiler.lib
- More verbose than OpenGL

**Confidence:** Design based on D3D11 standard, not copying FaceUnity or OBS

---

## 3. OpenGL Backend (P1, Evidence CNamaSDK Uses OpenGL 4.6)

**Why P1:**
- Evidence: Log `GLLoader.cc:212 initialGLExtentions: glversion max = 4, min = 6` from CNamaSDK, CNamaSDK imports OPENGL32.dll, beauty.exe imports OPENGL32.dll, strings GLProgramNew, GLTechnique, Material, etc.
- But HuanFace OpenGL backend is clean-room, not copying FaceUnity's GLProgramNew
- OpenGL is cross-platform (Windows, Linux, macOS), good for future portability
- OBS also supports OpenGL (but HuanFace does not depend on OBS)

**Classes:**

```cpp
class OpenGLTexture : public IGpuTexture {
    GLuint textureId = 0;
    int width, height;
    HFFormat format;
    void* GetNativeHandle() const override { return (void*)(uintptr_t)textureId; }
    void* GetShaderResourceView() const override { return (void*)(uintptr_t)textureId; }
};

class OpenGLRenderTarget : public IRenderTarget {
    GLuint fbo = 0;
    OpenGLTexture* texture = nullptr;
    void Bind() override { glBindFramebuffer(GL_FRAMEBUFFER, fbo); glViewport(0,0,width,height); }
    void Unbind() override { glBindFramebuffer(GL_FRAMEBUFFER, 0); }
};

class OpenGLShader : public IShader {
    GLuint programId = 0;
    GLuint vsId = 0;
    GLuint fsId = 0;
    std::map<std::string, GLint> uniformLocations;
    bool SetUniform(const std::string& name, float value) override { glUniform1f(uniformLocations[name], value); }
    bool SetTexture(const std::string& name, IGpuTexture* tex, int slot) override { glActiveTexture(GL_TEXTURE0+slot); glBindTexture(GL_TEXTURE_2D, (GLuint)(uintptr_t)tex->GetNativeHandle()); glUniform1i(uniformLocations[name], slot); }
};

class OpenGLMesh : public IMesh {
    GLuint vao = 0;
    GLuint vbo = 0;
    GLuint ebo = 0;
    int vertexCount, indexCount;
};

class OpenGLBackend : public IRenderBackend {
    HGLRC glContext = nullptr;
    HDC hdc = nullptr;
    HWND hwnd = nullptr;
    
    HFResult Init(void* windowHandle) override {
        // windowHandle is HWND
        // Get DC, choose pixel format, create WGL context via wglCreateContext, wglMakeCurrent
        // Load OpenGL functions via glad (MIT) or wglGetProcAddress
        // Check version >= 4.6 (from CNamaSDK evidence)
    }
    
    IGpuTexture* CreateTexture(int w, int h, HFFormat format, const void* data, int stride) override {
        // glGenTextures, glBindTexture, glTexImage2D
        // format mapping: RGBA8 → GL_RGBA8, BGRA8 → GL_RGBA8 with swizzle or convert, R8 → GL_R8, R32F → GL_R32F
        // glTexParameteri GL_TEXTURE_MIN_FILTER, MAG_FILTER, WRAP_S, WRAP_T
    }
    
    IShader* CreateShader(const std::string& vsSrc, const std::string& fsSrc) override {
        // vsSrc is GLSL vertex shader source, fsSrc is GLSL fragment shader source
        // glCreateShader, glShaderSource, glCompileShader, check compile status
        // glCreateProgram, glAttachShader, glLinkProgram, check link status
    }
    
    IMesh* CreateMesh(const std::vector<HFMeshVertex>& vertices, const std::vector<int>& indices) override {
        // glGenVertexArrays, glGenBuffers
        // glBindVertexArray, glBindBuffer GL_ARRAY_BUFFER, glBufferData vertices
        // glBindBuffer GL_ELEMENT_ARRAY_BUFFER, glBufferData indices
        // glVertexAttribPointer for pos, normal, uv
    }
    
    void DrawMesh(IMesh* mesh, IShader* shader, const std::map<std::string, Uniform>& uniforms) override {
        // glUseProgram
        // Set uniforms: glUniform1f, glUniform3f, etc.
        // Set textures: glActiveTexture, glBindTexture, glUniform1i
        // glBindVertexArray, glDrawElements
    }
    
    void Blit(IGpuTexture* src, IRenderTarget* dst) override {
        // Full-screen quad blit
    }
};
```

**GLSL Shader Example (clean-room, not copying FaceUnity or OBS):**

```glsl
// vs.glsl
#version 460 core

layout(location = 0) in vec3 a_pos;
layout(location = 1) in vec3 a_normal;
layout(location = 2) in vec2 a_uv;

uniform mat4 u_mvp;

out vec3 v_normal;
out vec2 v_uv;

void main() {
    gl_Position = u_mvp * vec4(a_pos, 1.0);
    v_normal = a_normal;
    v_uv = a_uv;
}

// fs.glsl (makeup)
#version 460 core

in vec3 v_normal;
in vec2 v_uv;

uniform sampler2D u_inputTexture;
uniform sampler2D u_makeupTexture;
uniform sampler2D u_maskTexture;
uniform vec4 u_makeupColor;
uniform float u_intensity;
uniform float u_opacity;
uniform int u_blendMode;

out vec4 fragColor;

vec3 blendNormal(vec3 base, vec3 blend, float opacity) {
    return mix(base, blend, opacity);
}

vec3 blendMultiply(vec3 base, vec3 blend, float opacity) {
    return mix(base, base * blend, opacity);
}

void main() {
    vec4 base = texture(u_inputTexture, v_uv);
    vec4 makeup = texture(u_makeupTexture, v_uv);
    float mask = texture(u_maskTexture, v_uv).r;
    
    vec3 coloredMakeup = makeup.rgb * u_makeupColor.rgb;
    float alpha = makeup.a * mask * u_intensity * u_opacity;
    
    vec3 blended;
    if (u_blendMode == 0) {
        blended = blendNormal(base.rgb, coloredMakeup, alpha);
    } else if (u_blendMode == 1) {
        blended = blendMultiply(base.rgb, coloredMakeup, alpha);
    } else {
        blended = blendNormal(base.rgb, coloredMakeup, alpha);
    }
    
    fragColor = vec4(blended, base.a);
}
```

**Pros:**
- Cross-platform (Windows, Linux, macOS)
- Evidence CNamaSDK uses OpenGL 4.6, so HuanFace OpenGL backend aligns with research
- GLSL is simpler than HLSL
- Good for future portability

**Cons:**
- On Windows, D3D11 is more efficient and better integrated with Media Foundation
- Requires glad loader (MIT) for function loading
- WGL context creation more complex than D3D11

**Confidence:** Design based on OpenGL 4.6 standard, not copying FaceUnity's GLProgramNew

---

## 4. Backend Selection

**Runtime selection (PROPOSED):**

```cpp
enum class HFRenderBackendType {
    AUTO, // auto-detect: try D3D11 first, fallback to OpenGL
    D3D11,
    OPENGL,
};

struct HFEngineConfig {
    HFRenderBackendType backendType = HFRenderBackendType::AUTO;
    void* windowHandle = nullptr; // HWND, optional for offscreen
    int width = 1280;
    int height = 720;
    bool enableDebug = false;
};

HF_Engine HF_CreateEngineWithConfig(const HFEngineConfig& config);
```

- **AUTO:** Try D3D11 first (P0 Windows), if fails (e.g., old GPU, no D3D11), fallback to OpenGL
- **D3D11:** Force D3D11
- **OPENGL:** Force OpenGL

**Evidence for D3D11 P0, OpenGL P1:**
- Windows 10+ D3D11 is native and efficient, Media Foundation zero-copy
- OpenGL evidence from CNamaSDK (GL 4.6 log, OPENGL32 imports) suggests FaceUnity uses OpenGL, so HuanFace should support OpenGL for compatibility and research alignment, but D3D11 is priority for Windows

---

## 5. No OBS Dependency

**Explicitly NOT using:**
- obs.dll
- obsplus.dll
- Spout.dll
- OBS effects (default.effect, format_conversion.effect) — these are OBS Studio, not FaceUnity, and HuanFace shaders are clean-room GLSL/HLSL, not copying OBS
- OBS shader-cache .v2

**OBS is EXTERNAL REFERENCE only for research (frame flow, integration clues, etc.), not runtime dependency.**

**HuanFace rendering dependencies (clean-room, open):**
- Windows: d3d11.lib, dxgi.lib, D3DCompiler.lib (for D3D11), opengl32.lib (for OpenGL), glad (MIT) for OpenGL loader
- No OBS

---

---

## Phase 4 Implementation Status

**IMPLEMENTED:**
- IRenderBackend interface real with NullBackend software for Linux CI and D3D11Backend minimal for Windows
- NullBackend: CreateTexture CPU buffer, CreateRenderTarget, Clear, CreateShader, CreateMesh fullscreen quad, DrawMesh no-op, Blit CPU memcpy, Present no-op
- D3D11Backend P0: CreateDeviceAndContext HARDWARE then WARP fallback, with/without swapchain, ToDXGIFormat, CreateTexture Texture2D BIND_SHADER_RESOURCE with init data SysMemPitch, CreateTextureFromFile real PNG->CPU RGBA via ImageLoader->Texture2D SRV correct width/height/format/stride, CreateTextureFromMemory for bundle PNG bytes, CreateRenderTarget BIND_RENDER_TARGET|SHADER_RESOURCE RTV+SRV, SetRenderTarget OMSetRenderTargets RSSetViewports, Clear ClearRenderTargetView, CreateShader real HLSL compile via D3DCompile with error log no crash (vs_5_0 ps_5_0), CreateShaderFromFile loads file then compiles, CreateMesh vertex+index buffers, DrawMesh sets shaders vertex buffer index buffer TRIANGLELIST DrawIndexed/Draw, Blit CopyResource, Present swapChain Present, SetBlendMode/DepthTest/CullMode stub
- Bundle integration: simple_lip.hfbundle texture lip.png real 512x512 not dummy 256, shader lip.hlsl clean-room lerp
- ShaderLoader: D3DCompile with error blob to stderr, no crash
- ResourceManager: load once reuse

**PARTIAL:**
- D3D11 only Windows #ifdef _WIN32, Linux fallback NullBackend + CPU makeup
- Blend modes, depth test, cull mode stubs
- Shader compilation uses passthrough fallback if source empty, real HLSL from bundle compiled but not used in HF_ProcessFrameWithBundle yet (CPU path)
- No OpenGL production, only NullBackend stub P1

**STUB:**
- OpenGLBackend returns NullBackend

**NOT IMPLEMENTED:**
- OpenGL 4.6 production with glad loader GLSL 460 core, evidence CNamaSDK uses GL 4.6 but not implemented
- Advanced blend modes, depth test, cull, constant buffers, input layouts from manifest
- GPU texture reuse pool, shader cache, async loading

**End of Render Backend**
