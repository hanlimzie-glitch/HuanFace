# HuanFace Bundle Spec — .hfbundle (Phase 2)

**Status:** SPECIFICATION — PROPOSED OPEN FORMAT, NOT reverse-engineered FaceUnity encrypted format  
**Important Distinction:** FaceUnity .bundle files (267 observed, magic F3 5B 06 12, entropy 7.7-7.85) are PROTECTED / ENCRYPTED, internal format UNKNOWN without bypass. HuanFace .hfbundle is PROPOSED OPEN ZIP format, clean-room, with PNG + GLSL + JSON + OBJ.

---

## 1. Format Overview

**Extension:** `.hfbundle`

**Container:** ZIP (via miniz, MIT) — open, no encryption

**Structure:**

```
example.hfbundle (ZIP)
├── manifest.json
├── textures/
│   ├── lip.png (RGBA8 PNG, 512x512)
│   ├── blush.png
│   ├── eyeshadow.png
│   └── ...
├── masks/
│   ├── lip_mask.png (R8 PNG, optional, or JSON polygon)
│   └── ...
├── shaders/
│   ├── lip.glsl (GLSL 460, or HLSL for D3D11)
│   ├── lip.hlsl
│   ├── blush.glsl
│   └── ...
├── meshes/
│   ├── face.json (optional, custom mesh)
│   └── ...
└── metadata/
    ├── thumbnail.png (256x256 preview)
    ├── preview.png
    └── description.txt (optional)
```

**All assets clean-room, no extraction from FaceUnity bundles.**

---

## 2. Manifest Schema

**File:** `manifest.json`

**Required fields:**

```json
{
  "format": "HuanFaceBundle",
  "version": "1.0",
  "type": "makeup",
  "name": "Simple Lip",
  "author": "HuanFace Example",
  "description": "Simple lipstick example",
  "created": "2026-03-31",
  "dependencies": [],
  "textures": [
    {"name": "lip", "path": "textures/lip.png", "format": "RGBA8", "width": 512, "height": 512}
  ],
  "masks": [
    {"name": "lip_mask", "path": "masks/lip_mask.png", "type": "r8"}
  ],
  "shaders": [
    {"name": "lip", "path": "shaders/lip.glsl", "type": "glsl", "stage": "fragment"}
  ],
  "meshes": [],
  "parameters": [
    {
      "name": "intensity_lip",
      "type": "float",
      "min": 0.0,
      "max": 1.0,
      "default": 0.8,
      "group": "Lip",
      "description": "Lipstick intensity"
    },
    {
      "name": "lip_color",
      "type": "color",
      "default": [1.0, 0.2, 0.3, 1.0],
      "group": "Lip"
    },
    {
      "name": "lip_type",
      "type": "enum",
      "options": ["matte", "gloss", "shimmer"],
      "default": "matte",
      "group": "Lip"
    }
  ],
  "passes": [
    {
      "name": "lip",
      "shader": "shaders/lip.glsl",
      "textures": ["textures/lip.png"],
      "masks": ["lip_mask"],
      "blend": "normal",
      "order": 0
    }
  ]
}
```

**Field definitions:**

| Field | Type | Required | Description |
|-------|------|----------|-------------|
| format | string | Yes | Must be "HuanFaceBundle" |
| version | string | Yes | Semver, e.g., "1.0" |
| type | string | Yes | Preset type: makeup, beauty, filter, effect, hair, face, combined |
| name | string | Yes | Bundle name, e.g., "Simple Lip" |
| author | string | No | Author name |
| description | string | No | Description |
| created | string | No | Date ISO 8601 |
| dependencies | array | No | Array of other bundle names this bundle depends on (for combined) |
| textures | array | No | Array of texture descriptors |
| masks | array | No | Array of mask descriptors |
| shaders | array | No | Array of shader descriptors |
| meshes | array | No | Array of mesh descriptors |
| parameters | array | No | Array of parameter descriptors (generic HFMakeupParameter) |
| passes | array | No | Array of render pass descriptors (render graph) |

**Texture descriptor:**

```json
{
  "name": "lip",
  "path": "textures/lip.png",
  "format": "RGBA8",
  "width": 512,
  "height": 512
}
```

| Field | Type | Required | Description |
|-------|------|----------|-------------|
| name | string | Yes | Logical name, e.g., "lip" |
| path | string | Yes | Relative path inside ZIP, e.g., "textures/lip.png" |
| format | string | No | RGBA8, BGRA8, R8, etc., default RGBA8 |
| width | int | No | Width, optional |
| height | int | No | Height, optional |

**Mask descriptor:**

```json
{
  "name": "lip_mask",
  "path": "masks/lip_mask.png",
  "type": "r8"
}
```

| Field | Type | Required | Description |
|-------|------|----------|-------------|
| name | string | Yes | Logical name |
| path | string | Yes | Relative path |
| type | string | No | r8, rgba8, json (polygon) |

**Shader descriptor:**

```json
{
  "name": "lip",
  "path": "shaders/lip.glsl",
  "type": "glsl",
  "stage": "fragment"
}
```

| Field | Type | Required | Description |
|-------|------|----------|-------------|
| name | string | Yes | Logical name |
| path | string | Yes | Relative path |
| type | string | No | glsl, hlsl, default glsl |
| stage | string | No | vertex, fragment, compute, default fragment |

**Mesh descriptor:**

```json
{
  "name": "face",
  "path": "meshes/face.json",
  "format": "json"
}
```

**Parameter descriptor (generic, extensible):**

```json
{
  "name": "intensity_lip",
  "type": "float",
  "min": 0.0,
  "max": 1.0,
  "default": 0.8,
  "group": "Lip",
  "description": "Lipstick intensity"
}
```

| Field | Type | Required | Description |
|-------|------|----------|-------------|
| name | string | Yes | Param name, e.g., "intensity_lip" |
| type | string | Yes | float, int, bool, color, vec2, vec3, vec4, texture, enum |
| min | float | No | Min value (for float/int) — only if known |
| max | float | No | Max value — only if known |
| default | any | No | Default value — type depends on type field, only if known |
| options | array | No | For enum type, array of strings |
| group | string | No | Group, e.g., Lip, Eye, Blush, etc. |
| description | string | No | Description |

**Pass descriptor (render graph):**

```json
{
  "name": "lip",
  "shader": "shaders/lip.glsl",
  "textures": ["textures/lip.png"],
  "masks": ["lip_mask"],
  "blend": "normal",
  "order": 0
}
```

| Field | Type | Required | Description |
|-------|------|----------|-------------|
| name | string | Yes | Pass name |
| shader | string | Yes | Shader path (logical or relative) |
| textures | array | No | Array of texture names or paths |
| masks | array | No | Array of mask names |
| blend | string | No | Blend mode: normal, multiply, screen, overlay, additive, etc., default normal |
| order | int | No | Render order, lower first |

**Preset types:**

| Type | Description | Example |
|------|-------------|---------|
| makeup | Makeup bundle (lip, eye, blush, etc.) | simple_lip.hfbundle |
| beauty | Beauty preset (skin smooth, whitening params) | natural_beauty.hfbundle |
| filter | Filter (color grading, LUT) | vintage_filter.hfbundle |
| effect | Special effect (e.g., face warp, sticker) | cat_ears.hfbundle |
| hair | Hair color / style | hair_color.hfbundle |
| face | Face shape / morph | small_face.hfbundle |
| combined | Combined (makeup+beauty+filter) | full_makeup.hfbundle |

---

## 3. Textures

**Format:** PNG (RGBA8, 8-bit per channel, lossless, open)

**Why PNG:**
- Open, lossless, supports alpha
- Loadable via stb_image (MIT)
- No proprietary format

**Size:** 512x512 or 1024x1024 typical, but any size allowed

**Content (clean-room):**
- Solid color with soft edge (e.g., lip.png is solid red with alpha gradient)
- No extraction from FaceUnity bundles (PROTECTED)
- Generated via Python script without PIL (struct+zlib) or via image editor

**Example simple_lip texture:**
- 512x512 PNG, solid color RGB (255, 50, 80) with alpha 255, or with gradient
- For testing, even 256x256 solid color is okay

---

## 4. Shaders

**Format:** GLSL (OpenGL 4.6 core) and HLSL (D3D11) — both optional, at least one required

**Why GLSL/HLSL:**
- Open, standard
- FaceUnity shaders (MakeupFilterPassNAMA etc.) are PROTECTED, not used
- Clean-room examples in RENDER_BACKEND.md and MAKEUP_RUNTIME_DESIGN.md

**Example lip.glsl:**

```glsl
#version 460 core
in vec2 v_uv;
uniform sampler2D u_inputTexture;
uniform sampler2D u_makeupTexture;
uniform sampler2D u_maskTexture;
uniform vec4 u_lip_color;
uniform float u_intensity_lip;
out vec4 fragColor;
void main() {
    vec4 base = texture(u_inputTexture, v_uv);
    vec4 makeup = texture(u_makeupTexture, v_uv) * u_lip_color;
    float mask = texture(u_maskTexture, v_uv).r;
    float alpha = mask * u_intensity_lip;
    fragColor = mix(base, makeup, alpha);
}
```

**Example lip.hlsl:**

```hlsl
Texture2D inputTexture : register(t0);
Texture2D makeupTexture : register(t1);
Texture2D maskTexture : register(t2);
SamplerState samLinear : register(s0);
cbuffer Params : register(b0) {
    float4 lip_color;
    float intensity_lip;
};
struct PS_INPUT { float4 pos : SV_POSITION; float2 uv : TEXCOORD0; };
float4 main(PS_INPUT input) : SV_TARGET {
    float4 base = inputTexture.Sample(samLinear, input.uv);
    float4 makeup = makeupTexture.Sample(samLinear, input.uv) * lip_color;
    float mask = maskTexture.Sample(samLinear, input.uv).r;
    float alpha = mask * intensity_lip;
    return lerp(base, makeup, alpha);
}
```

---

## 5. Meshes

**Format:** JSON or OBJ (open)

**JSON example (face.json):**

```json
{
  "vertices": [
    {"pos": [0.0, 0.0, 0.0], "normal": [0,0,1], "uv": [0.5,0.5]},
    ...
  ],
  "indices": [0,1,2, ...],
  "uv": [[0.5,0.5], ...],
  "normals": [[0,0,1], ...]
}
```

**OBJ (standard Wavefront OBJ):**

```
v 0.0 0.0 0.0
vt 0.5 0.5
vn 0 0 1
f 1/1/1 2/2/2 3/3/3
```

**For HuanFace example bundles, meshes optional — simple bundles may not need mesh, using face mesh from tracking instead**

---

## 6. Thumbnail / Metadata

**thumbnail.png:** 256x256 PNG preview of bundle effect, for UI

**description.txt:** optional text description

**Clean-room, generated without PIL via struct+zlib or image editor**

---

## 7. Bundle Tools (Only for HuanFace .hfbundle, NOT FaceUnity Encrypted)

**Tools location:** `tools/`

### 7.1 huanface_bundle_packer

**Purpose:** Pack directory into .hfbundle ZIP

**Input:** Directory with manifest.json + textures/ + shaders/ + etc.

**Output:** .hfbundle file (ZIP)

**Usage:**

```bash
python tools/huanface_bundle_packer.py examples/bundles/simple_lip/ examples/bundles/simple_lip.hfbundle
```

**Implementation (Python, no proprietary):**
- Use zipfile module (standard library)
- Read manifest.json, validate
- Add all files to ZIP
- No encryption

### 7.2 huanface_bundle_unpacker

**Purpose:** Unpack .hfbundle ZIP to directory

**Input:** .hfbundle file

**Output:** Directory

**Usage:**

```bash
python tools/huanface_bundle_unpacker.py examples/bundles/simple_lip.hfbundle /tmp/simple_lip_unpacked/
```

**Implementation:**
- Use zipfile module
- Extract all files
- No decryption (because HuanFace bundles are open, not encrypted)

### 7.3 huanface_bundle_inspector

**Purpose:** Inspect .hfbundle manifest and list contents

**Input:** .hfbundle file

**Output:** Manifest JSON pretty-printed + file list + validation

**Usage:**

```bash
python tools/huanface_bundle_inspector.py examples/bundles/simple_lip.hfbundle
```

**Implementation:**
- Use zipfile module
- Read manifest.json, parse with json module
- List files in ZIP
- Validate schema (required fields)
- Print summary

**These tools ONLY for HuanFace .hfbundle (ZIP, open), NOT for FaceUnity encrypted bundles (magic F3 5B 06 12, entropy 7.7-7.85, PROTECTED). Inspector for FaceUnity bundles in Phase 1 only did header/entropy analysis, not decryption.**

---

## 8. Bundle to Runtime Flow

```
.hfbundle file (ZIP)
  │
  ▼
BundleParser (IFileSystem + miniz + nlohmann/json)
  ├── Open ZIP (via miniz or IFileSystem::ListFiles if ZIP filesystem)
  ├── Read manifest.json (nlohmann/json)
  ├── Validate (format == HuanFaceBundle, version, type, name)
  ├── Parse textures array → list of texture paths
  ├── Parse masks array
  ├── Parse shaders array
  ├── Parse parameters array → HFMakeupParameter list
  └── Parse passes array → render graph
  │
  ▼
ResourceResolver
  ├── Resolve texture path: "textures/lip.png" → ZIP entry "textures/lip.png" → extract to temp or read directly
  ├── Resolve shader path
  └── Resolve mask path
  │
  ▼
ResourceManager (cache)
  ├── For each texture: load via stb_image (PNG) → CPU data → IGpuDevice::CreateTexture → IGpuTexture → cache (key=path)
  ├── For each shader: read file → IRenderBackend::CreateShader (compile GLSL/HLSL) → IShader → cache
  ├── For each mesh: read JSON/OBJ → parse → IGpuDevice::CreateMesh → IMesh → cache
  └── Cache with LRU eviction
  │
  ▼
Runtime (MakeupEngine)
  ├── For each pass in order:
  │   ├── Get shader from cache
  │   ├── Get textures from cache
  │   ├── Get masks (generated or from cache)
  │   ├── Set uniforms: for each param in parameters, set uniform (e.g., intensity_lip → u_intensity_lip)
  │   ├── Set blend mode
  │   └── Draw: IRenderBackend::DrawMesh or Blit
  └── Output HFFrame
```

**Documented, not implemented in Phase 2 (only spec + tooling for ZIP handling)**

---

## 9. Example Bundles (Clean-Room Assets)

**Location:** `examples/bundles/`

**Each example bundle directory:**

```
examples/bundles/simple_lip/
├── manifest.json
├── textures/
│   └── lip.png (256x256 solid color, generated via Python struct+zlib, no PIL, no protected extraction)
├── shaders/
│   ├── lip.glsl
│   └── lip.hlsl
└── metadata/
    └── thumbnail.png (256x256 solid, generated via Python)
```

**Bundles:**

| Bundle | Type | Description | Textures | Shaders | Params |
|--------|------|-------------|----------|---------|--------|
| simple_lip | makeup | Simple lipstick | lip.png (solid red) | lip.glsl/hlsl | intensity_lip, lip_color |
| simple_blush | makeup | Simple blush | blush.png (solid pink, soft circle) | blush.glsl/hlsl | intensity_blush, blush_color |
| simple_eyeshadow | makeup | Simple eyeshadow | eyeshadow.png (gradient) | eyeshadow.glsl/hlsl | intensity_eye, eye_color |
| simple_foundation | makeup | Simple foundation | foundation.png (skin tone) | foundation.glsl/hlsl | intensity_foundation, foundation_color |

**All assets clean-room:**
- PNG generated via Python script using struct+zlib (no PIL, no external lib, no protected extraction)
- GLSL/HLSL clean-room examples (not FaceUnity shaders)
- manifest.json hand-written, generic params

**Packing:**

```bash
python tools/huanface_bundle_packer.py examples/bundles/simple_lip examples/bundles/simple_lip.hfbundle
```

**Generates .hfbundle ZIP files in examples/bundles/*.hfbundle**

---

## 10. Comparison with FaceUnity Bundles (PROTECTED)

| Aspect | FaceUnity .bundle (Observed) | HuanFace .hfbundle (Proposed) |
|--------|------------------------------|-------------------------------|
| Magic | F3 5B 06 12 (70%) or variant (30%) | PK (ZIP magic) |
| Entropy | 7.7-7.85 (encrypted) | ~7.0-7.5 for PNG (not encrypted, PNG compressed) but manifest JSON low entropy |
| Size | 4KB-42MB | 10KB-5MB typical (PNG+GLSL) |
| Container | Custom encrypted, maybe AES-CBC (INFERRED from mbedtls) | ZIP (open, miniz) |
| Content | UNKNOWN / PROTECTED (would need decryption) | Open: PNG, GLSL/HLSL, JSON, OBJ |
| Encryption | Likely AES-CBC + signature (HIGH confidence from mbedtls strings) | None (open) |
| Manifest | INFERRED internal manifest, but PROTECTED | Open manifest.json with schema defined in this doc |
| Textures | INFERRED inside, but PROTECTED | Open textures/*.png |
| Shaders | INFERRED FaceUnity shaders (MakeupFilterPassNAMA etc.), PROTECTED | Open clean-room GLSL/HLSL |
| Tooling | No open tooling (would need bypass) | Packer/unpacker/inspector for open ZIP (tools/) |
| License | Proprietary, protected | Open, MIT (proposed) |

**HuanFace does NOT attempt to decrypt FaceUnity bundles, only defines open format.**

---

**End of HuanFace Bundle Spec**
