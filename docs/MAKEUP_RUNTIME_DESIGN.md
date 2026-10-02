# Makeup Runtime Design — HuanFace (Phase 2)

**Status:** SPECIFICATION — no renderer implementation in Phase 2  
**Evidence Source:** Phase 1 observed 275+ makeup strings (`makeup_*`), 40+ `tex_*` bindings, INI 263 keys, logs `face_makeup`, `Render Makeup Filter Count`

---

## 1. Makeup Parameter Catalog — Grouped (391 params from Phase 1)

From Phase 1 analysis, 391 makeup params observed in strings. Grouped for HuanFace:

### 1.1 Lip (20+ params)
```
makeup_intensity_lip (0-1) — lipstick intensity
makeup_lip_color (color vec3/vec4) — lip color
makeup_lip_color2 — gradient second color
makeup_intensity_lip_highlight — lip highlight intensity
makeup_lip_highlight_color
makeup_intensity_lip_shadow — lip shadow
makeup_lip_type (enum 0-N) — lipstick type (matte, gloss, etc.)
makeup_lip_mask (texture) — lip mask
tex_lip_mask, tex_lip, tex_mouth_occu_mask, tex_lip_highlight_mask
makeup_lip_occlusion (bool)
```

### 1.2 Eye (30+)
```
makeup_intensity_eye (eyeshadow intensity)
makeup_eye_color, makeup_eye_color2
makeup_intensity_eye_highlight
makeup_eye_type (enum)
makeup_intensity_eye_liner, makeup_eye_liner_color, makeup_eye_liner_type
makeup_intensity_eye_lash, makeup_eye_lash_type
makeup_intensity_pupil, makeup_pupil_color, makeup_pupil_type
tex_eye_mask, tex_eye_highlight, tex_eyelash, tex_eyeliner, tex_pupil
makeup_intensity_eye_light, makeup_eye_light_type
```

### 1.3 Eyebrow (10+)
```
makeup_intensity_brow, makeup_brow_color, makeup_brow_type
tex_brow_mask, tex_brow
makeup_brow_warp (bool)
```

### 1.4 Blush (10+)
```
makeup_intensity_blush, makeup_blush_color, makeup_blush_type
tex_blush_mask, tex_blush
makeup_blush_position (vec2), makeup_blush_scale
```

### 1.5 Foundation (10+)
```
makeup_intensity_foundation, makeup_foundation_color, makeup_foundation_type
tex_foundation_mask, tex_foundation
makeup_foundation_alpha
```

### 1.6 Highlight / Shadow / Contour (15+)
```
makeup_intensity_highlight, makeup_highlight_color, makeup_highlight_type
makeup_intensity_shadow, makeup_shadow_color, makeup_shadow_type
tex_highlight_mask, tex_shadow_mask
```

### 1.7 Texture / Blend (Generic)
```
makeup_*_texture (texture binding)
makeup_*_mask (texture)
blend_type_* (enum: 0=Normal, 1=Multiply, 2=Screen, 3=Overlay, 4=Additive, 5=SoftLight)
makeup_*_opacity (0-1)
makeup_*_saturation, makeup_*_brightness
```

### 1.8 Beauty (Separate — see BEAUTY_RUNTIME_DESIGN.md)
```
makeup_intensity_* are MAKEUP, not beauty
Beauty params: HeavyBlur, ColorLevel, etc. separate engine
```

**Total grouped:** Lip 20 + Eye 30 + Brow 10 + Blush 10 + Foundation 10 + Highlight/Shadow 15 + Texture/Blend generic 50 + others (eyelash, eyeliner, pupil, etc.) 30 + misc 216 = 391 observed

---

## 2. Generic Parameter System (PROPOSED — Clean-Room)

**Not copying FaceUnity's exact param names implementation (PROTECTED), but using generic HFMakeupParameter for HuanFace.**

```cpp
enum class HFParamType {
    FLOAT = 0,
    INT,
    BOOL,
    COLOR,   // vec3 or vec4, RGB or RGBA 0-1 or 0-255
    VEC2,
    VEC3,
    VEC4,
    TEXTURE, // string path or IGpuTexture*
    ENUM,
};

struct HFColor {
    float r, g, b, a; // 0-1
};

struct HFMakeupParameter {
    std::string name; // e.g., "intensity_lip", "lip_color", "blush_type"
    HFParamType type = HFParamType::FLOAT;
    // Value — variant
    float floatValue = 0.0f;
    int intValue = 0;
    bool boolValue = false;
    HFColor colorValue = {0,0,0,1};
    float vec2Value[2] = {0,0};
    float vec3Value[3] = {0,0,0};
    float vec4Value[4] = {0,0,0,0};
    std::string texturePath; // for TEXTURE type, relative to bundle textures/
    IGpuTexture* textureValue = nullptr; // resolved at runtime via ResourceManager
    
    // Range — only if evidence, otherwise default
    float minValue = 0.0f;
    float maxValue = 1.0f;
    float defaultValue = 0.0f;
    std::vector<std::string> enumOptions; // for ENUM
    
    // Metadata
    std::string group; // Lip, Eye, Brow, Blush, Foundation, Highlight, Shadow, Texture, Blend, Beauty
    std::string description;
};

struct HFMakeupPreset {
    std::string name; // e.g., "Natural", "Smoky Eye"
    std::map<std::string, HFMakeupParameter> parameters; // name → param
};
```

**Range/default only if evidence:**
- From INI: Beauty params have ranges (e.g., HeavyBlur 0-100, ColorLevel 0-100)
- From strings: makeup_intensity_* likely 0-1 (intensity)
- But we do NOT invent ranges without evidence — in manifest.json we mark min/max/default optional

**Why generic:**
- FaceUnity has 391 hardcoded params, but HuanFace should be extensible
- Bundle can define custom params via manifest.json parameters array
- Engine resolves param name to shader uniform (e.g., "intensity_lip" → uniform float u_intensity_lip in shader)

---

## 3. Makeup Pipeline (Camera → Face Tracking → Face Mesh → Mask Generation → Texture Sampling → Makeup Shader → Blend → Composite)

### 3.1 Steps

```
1. Camera → HFFrame (input)
2. Face Tracking → HFTrackingData (bbox, landmarks, mesh)
3. Face Mesh → HFFaceMesh (vertices, indices, UV, normals) — from tracking, with landmarkToVertex mapping
4. Mask Generation (CPU or GPU):
   - Lip: polygon from landmarks (e.g., landmarks 48-67 for 68-point, or 0-... for 468-point lip outer/inner) → rasterize to R8 texture (256x256 or full-res) → blur for soft edge (Gaussian blur shader) → lip_mask + mouth_occu_mask (teeth/mouth interior) + lip_highlight_mask
   - Eye: eye mask from landmarks (eye contour)
   - Brow: brow mask
   - Blush: cheek region (from landmarks, e.g., cheek points)
   - Foundation: face skin mask (face contour minus eyes, lips, eyebrows)
   - Highlight: highlight region (T-zone, etc.)
   - Shadow: contour region (jaw, nose side)
   - Implementation: CPU rasterization (scanline fill) for initial mask, then GPU blur, or full GPU via mesh rendering to mask texture (render face mesh with mask color to R8 render target)
5. Texture Sampling:
   - From Bundle textures/ via ResourceManager cache
   - Example: textures/lip.png, textures/eyeshadow.png, textures/blush.png
   - Load via stb_image (PNG) → CPU data → upload to GPU texture (IGpuTexture) → cache
   - Texture format: RGBA8, size: 512x512 or 1024x1024 typical
6. Makeup Shader (GLSL/HLSL clean-room):
   - Vertex shader: transform mesh vertex with mvp uniform (from face pose), pass UV, pass world pos
   - Fragment shader: sample base image (input HFFrame GPU texture), sample makeup texture (from bundle), sample mask (from step 4), sample color (uniform), apply intensity, blend
   - Example GLSL (clean-room, not FaceUnity):
     ```glsl
     #version 460 core
     in vec2 v_uv;
     uniform sampler2D u_inputTexture; // base image
     uniform sampler2D u_makeupTexture; // from bundle
     uniform sampler2D u_maskTexture; // from mask generation
     uniform vec4 u_makeupColor; // color param
     uniform float u_intensity; // 0-1
     uniform int u_blendMode; // 0=Normal,1=Multiply,etc.
     out vec4 fragColor;
     vec4 blendNormal(vec4 base, vec4 blend, float opacity) { return mix(base, blend, opacity); }
     vec4 blendMultiply(vec4 base, vec4 blend, float opacity) { return mix(base, base*blend, opacity); }
     void main() {
         vec4 base = texture(u_inputTexture, v_uv);
         vec4 makeup = texture(u_makeupTexture, v_uv) * u_makeupColor;
         float mask = texture(u_maskTexture, v_uv).r;
         float alpha = mask * u_intensity;
         vec4 result;
         if (u_blendMode == 0) result = blendNormal(base, makeup, alpha);
         else if (u_blendMode == 1) result = blendMultiply(base, makeup, alpha);
         else result = blendNormal(base, makeup, alpha);
         fragColor = result;
     }
     ```
   - HLSL equivalent for D3D11
7. Blend:
   - Blend modes: Normal (lerp), Multiply (base*blend), Screen (1-(1-base)*(1-blend)), Overlay, Additive, SoftLight, etc.
   - Opacity from mask * intensity
8. Composite:
   - Render graph: Foundation → Blush → EyeShadow → Eyebrow → Eyeliner → Eyelash → Pupil → Lip → Highlight → Shadow
   - But order configurable via manifest.json passes array
   - Each pass: input = previous pass output, output = new render target or same (ping-pong)
   - Final composite output = HFFrame with makeup
```

### 3.2 Layers / Render Graph (Configurable)

From manifest.json:

```json
{
  "passes": [
    {"name": "foundation", "shader": "shaders/foundation.glsl", "textures": ["textures/foundation.png"], "masks": ["foundation_mask"], "blend": "normal", "order": 0},
    {"name": "blush", "shader": "shaders/blush.glsl", "textures": ["textures/blush.png"], "masks": ["blush_mask"], "blend": "normal", "order": 1},
    {"name": "eyeshadow", "shader": "shaders/eyeshadow.glsl", "textures": ["textures/eyeshadow.png"], "masks": ["eye_mask"], "blend": "multiply", "order": 2},
    {"name": "eyebrow", "shader": "shaders/eyebrow.glsl", "textures": ["textures/eyebrow.png"], "masks": ["brow_mask"], "blend": "normal", "order": 3},
    {"name": "eyeliner", "shader": "shaders/eyeliner.glsl", "textures": ["textures/eyeliner.png"], "masks": ["eye_mask"], "blend": "normal", "order": 4},
    {"name": "eyelash", "shader": "shaders/eyelash.glsl", "textures": ["textures/eyelash.png"], "masks": ["eye_mask"], "blend": "normal", "order": 5},
    {"name": "pupil", "shader": "shaders/pupil.glsl", "textures": ["textures/pupil.png"], "masks": ["pupil_mask"], "blend": "normal", "order": 6},
    {"name": "lip", "shader": "shaders/lip.glsl", "textures": ["textures/lip.png"], "masks": ["lip_mask", "mouth_occu_mask"], "blend": "normal", "order": 7},
    {"name": "highlight", "shader": "shaders/highlight.glsl", "textures": ["textures/highlight.png"], "masks": ["highlight_mask"], "blend": "additive", "order": 8},
    {"name": "shadow", "shader": "shaders/shadow.glsl", "textures": ["textures/shadow.png"], "masks": ["shadow_mask"], "blend": "multiply", "order": 9}
  ]
}
```

**Order configurable, not hardcoded.**

**For HuanFace example bundles (simple_lip, simple_blush, etc.):**
- simple_lip: only lip pass
- simple_blush: only blush pass
- simple_eyeshadow: only eyeshadow pass
- simple_foundation: only foundation pass

---

## 4. Resource System

```
Bundle (.hfbundle ZIP)
  ├── manifest.json
  ├── textures/ (PNG)
  ├── masks/ (PNG or JSON for polygon)
  ├── shaders/ (GLSL/HLSL)
  ├── meshes/ (OBJ/JSON)
  └── metadata/
      └── thumbnail.png
  │
  ▼
BundleParser (IFileSystem + miniz)
  ├── Read manifest.json (nlohmann/json)
  ├── Validate schema (version, type, name, etc.)
  └── List resources (textures, masks, shaders, meshes)
  │
  ▼
ResourceResolver
  ├── Resolve texture path (textures/lip.png → absolute path in ZIP)
  ├── Resolve shader path
  └── Resolve mask path
  │
  ▼
ResourceManager (cache)
  ├── Load texture via stb_image → IGpuTexture (via IGpuDevice) → cache (LRU, key=path)
  ├── Load shader via IFileSystem::ReadFile → compile (IRenderBackend::CreateShader) → cache
  ├── Load mesh via OBJ parser → IMesh (via IGpuDevice::CreateMesh) → cache
  └── Eviction: LRU when cache size > max (e.g., 100 MB textures, 50 shaders)
  │
  ▼
Runtime (MakeupEngine)
  ├── For each pass in manifest passes:
  │   ├── Get shader from ResourceManager (or compile if not cached)
  │   ├── Get textures from ResourceManager
  │   ├── Get masks (generated or from bundle)
  │   ├── Set uniforms (from HFMakeupParameter)
  │   └── Draw (IRenderBackend::DrawMesh or Blit)
  └── Composite output
```

**Bundle → ResourceManager → GPU flow documented, not implemented in Phase 2 (only spec + tooling for .hfbundle ZIP)**

---

## 5. Performance

**Same as FRAME_PIPELINE.md:**
- GPU texture reuse (pool)
- Async resource loading (textures, shaders load in background thread, not blocking render)
- Mask generation GPU-accelerated (render mesh to R8 target, not CPU rasterization per frame for all masks — CPU for initial, GPU for blur and final)
- Shader cache (D3D11 bytecode, OpenGL binary)
- Render graph optimization: if only lip changed, only re-render lip pass, not all (dirty flag per pass)

**Target 1080p 30 FPS min 60 FPS target**

---

## 6. No OBS / FaceUnity Dependency

**NOT using:**
- obs.dll, obsplus.dll, Spout, OBS effects
- FaceUnity encrypted bundles (267 .bundle files with magic F3 5B 06 12 entropy 7.7-7.85 PROTECTED)
- FaceUnity shaders (MakeupFilterPassNAMA etc. PROTECTED)
- FaceUnity param implementation (PROTECTED)

**OBS is EXTERNAL REFERENCE only**

**HuanFace makeup dependencies (clean-room, open):**
- stb_image (MIT) for PNG
- miniz (MIT) for ZIP
- nlohmann/json (MIT) for manifest
- glm (MIT) for math
- Custom GLSL/HLSL shaders clean-room (example in RENDER_BACKEND.md and this doc)

---

---

## Phase 4 Implementation Status

**IMPLEMENTED:**
- MakeupEnginePrototype Input+Mask+Texture+Params->Shader->Composite lip color lerp(original, makeupColor, lipMask*intensity) clean-room
- FaceMaskGenerator HFFaceMesh->R8 mask triangle rasterization CPU barycentric
- FeatureMask architecture FaceMask/LipMask/EyeMask etc Phase 4 only Face+Lip with data/face_regions/ mapping open knowledge 68-landmark layout not proprietary
- D3D11 pipeline HFFrame->InputTexture->FaceMask->LipMask->MakeupShader->RT->OutputTexture no OBS renderer/shader (CPU path on Linux, D3D11 texture real PNG + shader compile on Windows)
- Bundle integration simple_lip.hfbundle Load manifest->texture->shader->GPU resources->makeup pass HLSL clean-room lerp
- ShaderLoader D3DCompile with error log no crash
- PNG texture CreateTextureFromFile real PNG->CPU RGBA->D3D11 Texture2D SRV correct width/height/format/stride, bundle texture simple_lip textures/lip.png real 512x512 not dummy 256
- ResourceManager load once reuse release no duplicate per frame

**PARTIAL:**
- Only lip makeup, not blush/eyeshadow/foundation
- Only FACE and LIP masks, others return error
- CPU lerp on Linux, D3D11 shader compile on Windows but not executed in HF_ProcessFrameWithBundle (still CPU path)
- No soft edge blur, no blend modes Normal/Multiply/Screen/Overlay yet, only normal lerp

**STUB:**
- BlushMakeup, EyeshadowMakeup, FoundationMakeup still placeholders

**NOT IMPLEMENTED:**
- Full makeup renderer with texture sampling, blend modes, render graph from manifest passes order, foundation/blush/eyebrow/eyeliner/eyelash/pupil/highlight/shadow, mask blur, GPU texture reuse pool, shader cache, async loading, 60 FPS opt

**End of Makeup Runtime Design**
