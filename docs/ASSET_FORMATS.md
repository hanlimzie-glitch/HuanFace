# Asset Formats — HuanFace

## 1. Image Assets (PNG, SVG, GIF)

### PNG Thumbnails (341 files)

**Location:**
- `ProgramData/obsplus/beauty/beauty-hair/thumbs/` (22 files)
- `ProgramData/obsplus/beauty/filters/thumbs/` (100+ files)
- `AppData/.../data/*.png` (UI icons)

**Format:** Standard PNG, 8-bit RGBA, ~200x200 for thumbs, small for UI icons

**Naming:**
- Hair: `hair_gradient_01.png`, `hair_normal_01.png` — descriptive
- Filters: `04BC983D74C989ADD8B0C1CB844953BF.png` — MD5-like hash, matches bundle hash or filter ID
- UI: `download.png`, `favorite.png` — fixed names

**Extractable:** YES — direct read via any image library

**Mapping to bundles:** Likely via `config.dat` index. For hair, thumb name `hair_normal_01.png` may map to model bundle `7F6145EBA25E794E874664152A6F1917.bundle`? Need config.dat decryption to confirm. **Confidence: MEDIUM**

**Example inspection (Python):**
```python
from PIL import Image
im = Image.open("ProgramData/obsplus/beauty/beauty-hair/thumbs/hair_normal_01.png")
print(im.size, im.mode)
```

### SVG Icons (8 files)

**Location:** `data/themes/Dark|Light/download.svg`, `favorite.svg`

**Format:** Standard SVG XML, vector icons for UI

**Extractable:** YES

### GIF (2 files)

**Location:** `data/loading.gif`

**Format:** Standard GIF animation, loading spinner

**Extractable:** YES

---

## 2. Shader / Effect Assets

### OBS Effects (.effect) — 4 files

**Files:**
- `data/default.effect` (9.3 KB)
- `data/format_conversion.effect` (13 KB)
- Duplicates in `AppData/.../data/` and `bin/` root copies

**Format:** OBS effect language — HLSL-like with techniques, passes, vertex/pixel shaders, sampler states, uniforms

**Example structure:**
```hlsl
uniform float4x4 ViewProj;
uniform texture2d image;
uniform float multiplier;

sampler_state def_sampler {
    Filter = Linear;
    AddressU = Clamp;
    AddressV = Clamp;
};

struct VertInOut {
    float4 pos : POSITION;
    float2 uv  : TEXCOORD0;
};

VertInOut VSDefault(VertInOut vert_in) {
    VertInOut vert_out;
    vert_out.pos = mul(float4(vert_in.pos.xyz, 1.0), ViewProj);
    vert_out.uv  = vert_in.uv;
    return vert_out;
}

float4 PSDrawBare(VertInOut vert_in) : TARGET {
    return image.Sample(def_sampler, vert_in.uv);
}

technique Draw {
    pass {
        vertex_shader = VSDefault(vert_in);
        pixel_shader  = PSDrawBare(vert_in);
    }
}
```

**Techniques in default.effect:**
- `Draw` — bare draw
- `DrawAlphaDivide` — divide RGB by alpha
- `DrawNonlinearAlpha` — sRGB nonlinear alpha handling
- `DrawSrgbDecompress` — sRGB to linear
- `DrawMultiply` — multiply by uniform
- `DrawTonemap` — Rec709->Rec2020->Reinhard->Rec2020->Rec709 tonemap
- `DrawPQ` — ST2084 PQ to linear
- `DrawTonemapPQ` — PQ with tonemap

**Purpose:** OBS uses these to composite final output with correct color space handling (SDR, HDR, PQ, HLG)

**FaceUnity shaders:** NOT in these files. FaceUnity shaders are inside encrypted bundles.

**Extractable:** YES — text files

### Shader Cache (.v2) — 223 files

**Location:** `ProgramData/obs-studio/shader-cache/*.v2`

**Format:** Binary D3D11 compiled shader bytecode cache — OBS caches compiled shaders for faster startup

**Example:** File `10abd9496ef2f029.v2` — first bytes not readable, size few KB

**Extractable:** PARTIAL — binary, need D3D11 disassembler, but not essential for HuanFace SDK

**Confidence:** MEDIUM that it's D3D11 cache, based on OBS source code knowledge

---

## 3. Model Assets

### MobileNetSSD (Caffe)

**Files:**
- `data/model/MobileNetSSD_deploy.caffemodel` (22 MB)
- `data/model/MobileNetSSD_deploy.prototxt` (text)

**Format:** Caffe model — prototxt defines network, caffemodel contains weights

**Prototxt preview:**
```prototxt
# Usually contains layers: convolution, relu, pooling, detection_output, etc.
# For MobileNetSSD: 300x300 input, 20 classes (person, etc.)
```

**Purpose:** Person/object detection — fallback for background segmentation when AI processor bundles not loaded, or for initial person detection

**Extractable:** YES — standard Caffe, can be loaded with OpenCV DNN or converted to ONNX

### AI Face Processor Bundle (23 MB)

**File:** `common/assets/model/ai_face_processor_pc.bundle`

**Format:** Encrypted FaceUnity bundle — contains CNN model for face detection, landmarks, face mesh

**Internal (hypothesized):**
- TFLite model or custom CNN
- Config for landmark quality, min face ratio, etc.
- Possibly contains multiple models: detection, landmark, occlusion, hair mask, head mask

**Extractable:** NO without decryption

### AI Human Processor Bundle (42 MB)

**File:** `common/assets/model/ai_human_processor_pc.bundle`

**Format:** Encrypted FaceUnity bundle — contains models for human pose, segmentation, hand detection

**Internal (hypothesized):**
- Human detection, 2D/3D joints, action recognition, BVH motion
- Hand detector, gesture types
- Human mask, hair mask

**Extractable:** NO without decryption

---

## 4. Config / Metadata Assets

### Locale INI (19 files)

**Location:** `data/locale/en-US.ini`, `zh-CN.ini`, `ja-JP.ini`, `ko-KR.ini`, `zh-TW.ini` + duplicates

**Format:** INI — key=value, with sections implied by key prefix

**Example:**
```ini
obs-cam-beauty="OBS Beauty Camera"
ParameterSetting="Beauty"
BaseBeauty="Base"
SkinDect="Skin Precise"
HeavyBlur="Skin Smooth"
BalanceSmooth="Balance Smooth"
BlurLevel="Smooth Level"
ColorLevel="Whitening"
DelspotLevel="Blemish Removal"
RedLevel="Rosiness"
Clarity="Enhance Details"
Sharpen="Sharpen"
FaceThreed="Facial Contour"
EyeBright="Brighten Eyes"
ToothWhiten="Whiten Teeth"
RemovePouchStrength="Remove Dark Circles"
RemoveNasolabialFoldsStrength="Nasolabial Folds"
Brightness="Fill Light"
Saturation="Saturation"
Filters="Filters"
...
```

**Purpose:** UI translations + parameter names — reveals supported beauty features

**Full param list extracted (from en-US.ini, partial):**
- Base: `HeavyBlur` (skin smooth), `ColorLevel` (whitening), `DelspotLevel`, `RedLevel`, `Clarity`, `Sharpen`, `FaceThreed`, `EyeBright`, `ToothWhiten`, `RemovePouchStrength`, `RemoveNasolabialFoldsStrength`, `Brightness`, `Saturation`
- Filters: `White`, `Pink`, `Fresh`, `CoolTone`, `WarmTone`, `Clear`
- Face Size: `Big`, `Medium`, `Small`
- Detect Mode: `Standard`, `High Precision`
- Beauty Guard: `BeautyProtection`, `Mosaic Size`
- FPS: `Rendering Frame Rate`, `Match OBS video output frame rate`
- Orientation: `Up`, `Right`, `Down`, `Left`
- Plus many more for face shape, eye, nose, chin (need full INI parse)

**Extractable:** YES — text INI, parse with Python `configparser`

### JSON Configs (5 files)

**Files:**
- `AppData/Roaming/proxy/gateway.json` — proxy list
- `AppData/Roaming/locale/allinfo-en-US.json` — marketplace metadata for all plugins
- `AppData/Roaming/locale/ui-en-US.json` — UI strings for store
- `AppData/Roaming/notice/config.json` — notice timing
- `AppData/Roaming/notice/report.json` — report

**Example gateway.json:**
```json
{
  "proxies": [
    {"username": "85vk", "password": "85vku", "ip": "183.131.35.94", "port": 20252},
    {"username": "obsproxy", "password": "nCzPVD4B", "ip": "gateway.obshelp.com", "port": 10080}
  ]
}
```

**Extractable:** YES

### Config.dat (6 files, encrypted)

**Files:**
- `ProgramData/obsplus/beauty/beauty-hair/config.dat` (11 KB)
- `ProgramData/obsplus/beauty/filters/config.dat` (16 KB)
- `ProgramData/obsplus/beauty/makeup/8.10.0/custom_config.dat` (41 KB)
- `ProgramData/obsplus/beauty/makeup/8.10.0/style_config.dat` (14 KB)
- `ProgramData/obsplus/beauty/special-effects/config.dat` (80 KB)

**Format:** Encrypted/obfuscated — not JSON, not INI, high entropy

**Hypothesis:** Index mapping bundle hash to metadata:
```json
// Hypothetical decrypted structure
{
  "bundles": [
    {
      "id": "026B94DC68A10EB6992B94333E856A1F",
      "name": "Natural Lip",
      "category": "lip",
      "thumb": "026B94DC...png or internal",
      "params": {"intensity": 0.8, "color": "#FF0000"},
      "dependencies": []
    }
  ]
}
```

**Evidence:** Located alongside bundles and thumbs, named config.dat, typical for FaceUnity store to have encrypted index

**Extractable:** NO without decryption — but we can attempt to understand via observing running plugin or via legitimate SDK API if available

**Confidence:** MEDIUM for purpose, LOW for internal structure

### Token.bin (1 file)

**File:** `AppData/Roaming/token/token.bin` (small)

**Format:** Binary token — likely auth token for store/marketplace

**Extractable:** NO — binary, possibly encrypted

---

## 5. Bundle Assets (267 files, encrypted)

**Already covered in BINARY_ANALYSIS.md and BUNDLE_FORMAT.md, but summary:**

- **Format:** Encrypted container with magic `F3 5B 06 12` (primary)
- **Internal (hypothesized):** JSON manifest, JS controller, textures (PNG), shaders (GLSL), meshes (VBO/EBO), parameters, AI models
- **Textures inside:** Expected `eyepupil.png`, `eyeliner.png`, `eyelash.png`, `brow.png`, `eye.png`, etc. — from logs `load texture failed`
- **Params inside:** `tex_eye`, `tex_brow`, `makeup_intensity_*`, `blend_type_*`
- **Extractable:** NO without decryption via official SDK (no bypass)

**For HuanFace SDK, we will design our own bundle format:**

```
HuanFace Bundle (proposed, clean-room, not copying FaceUnity)
├── manifest.json
│   ├── name: "natural_lip"
│   ├── version: "1.0"
│   ├── type: "makeup/lip"
│   ├── params: {intensity, color, opacity, blend_mode}
│   └── dependencies: []
├── textures/
│   ├── lip.png
│   ├── lip_mask.png
│   └── gloss_lut.png
├── shaders/
│   ├── vertex.glsl
│   └── fragment.glsl
├── meshes/
│   ├── lip_mesh.obj or custom
│   └── mask
└── js/ (optional)
    └── controller.js
```

This would be **ZIP or custom binary** but **not encrypted with FaceUnity's key** — we will use open format.

---

## 6. Summary Table

| Asset Type | Count | Format | Encrypted? | Extractable? | Confidence |
|------------|-------|--------|------------|--------------|------------|
| PNG thumbs | 341 | PNG | No | Yes | HIGH |
| SVG icons | 8 | SVG | No | Yes | HIGH |
| GIF | 2 | GIF | No | Yes | HIGH |
| OBS .effect | 4 | HLSL-like text | No | Yes | HIGH |
| Shader cache .v2 | 223 | D3D11 bytecode | No (but binary) | Partial | MEDIUM |
| Caffe model | 2 | Caffe | No | Yes | HIGH |
| AI bundles | 2 | FaceUnity encrypted | Yes | No | HIGH |
| Core bundles | 4 | FaceUnity encrypted | Yes | No | HIGH |
| Makeup bundles | 84+ | FaceUnity encrypted | Yes | No | HIGH |
| Hair bundles | 2 | FaceUnity encrypted | Yes | No | HIGH |
| Special-effects | 150+ | FaceUnity encrypted | Yes | No | HIGH |
| INI locale | 19 | INI text | No | Yes | HIGH |
| JSON config | 5 | JSON | No | Yes | HIGH |
| Config.dat | 6 | Encrypted | Yes | No | MEDIUM |
| Token.bin | 1 | Binary | Likely | No | LOW |

**Total extractable without decryption:** ~400 files  
**Total encrypted:** 273 files

---

## 7. Tools to Build (Phase 1)

- `tools/texture_inspector.py` — catalog PNGs, sizes, map to bundles
- `tools/metadata_inspector.py` — parse INI and JSON, extract param list
- `tools/bundle_inspector.py` — header, entropy, magic analysis
- `tools/asset_inspector.py` — overall asset catalog with purpose

---

**End of Asset Formats**
