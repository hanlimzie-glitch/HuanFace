
> **IMPORTANT CORRECTION — Phase 1**
> 
> This document contains three distinct types of information:
> - **OBSERVED FORMAT**: Facts directly observable from repository (magic bytes, entropy, file sizes, strings)
> - **INFERRED FORMAT**: Hypotheses based on evidence (encryption via mbedtls, likely internal sections)
> - **PROPOSED HUANFACE FORMAT**: New open format for HuanFace SDK (ZIP-based), NOT reverse-engineered original
>
> **Original FaceUnity bundle:**
> - Status: PROTECTED / partially understood
> - Observed: encrypted/signed binary package, magic F3 5B 06 12 primary (70%) or variant (30%), entropy 7.7-7.85, size 4KB-42MB
> - Internal structure: NOT fully established (would require decryption, which we do NOT do)
> - Protected content: NOT analyzed (no bypass)
>
> **HuanFace bundle:**
> - Status: PROPOSED OPEN FORMAT
> - ZIP-based with manifest.json, textures PNG, shaders GLSL, meshes JSON/OBJ, optional JS
> - This is clean-room design, not reverse-engineered original

---

# Bundle Format Analysis — HuanFace

## 1. Overview

FaceUnity bundles (`.bundle`) are **encrypted, signed containers** that hold all resources for a specific effect (beauty, makeup, body slim, background blur, AI models, hair, stickers, etc.).

- **Count in repo:** 267
- **Total size:** ~150 MB (excluding AI processors: 23+42 MB extra)
- **Encryption:** YES — evidence from `DecryptObfuscatedPackage`, `VerifySignature`, mbedtls strings, high entropy (7.7-7.85)
- **Signature:** YES — `VerifySignature` prevents tampering
- **Compression:** Likely — high entropy could be compressed + encrypted

**We MUST NOT bypass encryption or signature verification.** Our analysis is static only.

---

## 2. Header Structure (Observed)

### 2.1 Primary Magic `F3 5B 06 12`

**Appears in:** ~70% of bundles (all core bundles + most makeup)

**Sample hexdump (first 64 bytes) from `face_beautification.bundle`:**

```
F3 5B 06 12 AC 5E 00 E1 B8 96 66 80 18 3E 3A 25 65 B9 58 29 98 85 6F B8 93 F1 DE 81 C1 E1 E6 71 C6 2E E2 BE 2D 04 46 DB 66 DB 10 19 BD 95 C1 5E 54 22 43 EF 54 54 AA 08 74 6B D5 1B 99 3A 59 09
```

**Breakdown hypothesis:**

| Offset | Size | Observed | Hypothesis | Confidence |
|--------|------|----------|------------|------------|
| 0x00 | 4 | `F3 5B 06 12` | Magic number for obfuscated bundle v1 | HIGH |
| 0x04 | 4-8 | `AC 5E 00 E1` ... | Version / flags / IV / checksum? High entropy, varies per file | MEDIUM |
| 0x08 | 8-16 | `B8 96 66 80 18 3E 3A 25` ... | Possibly file size, timestamp, or part of IV | LOW |
| 0x10+ | ... | High entropy | Encrypted payload starts early | MEDIUM |

**No readable strings in first 256 bytes** — entire header encrypted or obfuscated.

**File sizes:**
- Smallest: 4.2 KB (`81ED4D77A8EE259D091955F14985EE36.bundle` — hair model)
- Largest: 42 MB (`ai_human_processor_pc.bundle`)
- Core: `body_slim` 16.5 KB, `face_makeup` 702 KB, `face_beautification` 5.2 MB, `background_blur` 125 KB

### 2.2 Variant Magics

**Appears in:** ~30% of bundles, mostly custom makeup

**Examples:**
- `7C D3 33 56 25 FA EC C7` — `1BAD21287324521074F56AD7388A225D.bundle` (30 KB)
- `10 11 D2 D5 D3 F1 C1 2D` — `23626C7408C1F88BA95030F6BF6E16FE.bundle` (24 KB)
- `B2 AA 43 33 D1 0B B5 01` — `306D2F1D540BC5FE88180DF5D19064CB.bundle` (87 KB)
- `3D E6 15 6D 28 32 D7 B7` — `33BB1C630C223AD83F9EB02E3822DC3E.bundle` (104 KB)
- `35 59 CD 0B 20 23 4F 26` — `background_blur.bundle` (125 KB) — **different from primary!**

**Hypotheses for variants:**

1. **Different bundle type:**
   - String `is_controller_resource_bundle` suggests at least 2 types: controller resource vs model
   - Controller bundles contain JS + shaders + textures
   - Model bundles contain AI models
   - Background blur is segmentation model — maybe different type, hence different magic

2. **Different encryption version:**
   - FaceUnity may have updated encryption over time
   - Older bundles use variant magic, newer use `F3 5B 06 12`
   - Check file timestamps? Repo doesn't have original timestamps, but we can check config.dat dates

3. **Different obfuscation level:**
   - Some bundles may be only obfuscated, not encrypted (lower security for free presets)
   - But entropy still high (7.8), so likely still encrypted

**Validation needed:** Compare variant magic bundles' sizes, locations, and whether they load via same API. Logs show both types loading via `fuCreateItemFromPackage` — same API, so SDK handles both magics.

**Confidence:** LOW for exact reason, MEDIUM that variants are valid bundle types.

---

## 3. Encryption & Signature

### Evidence from CNamaSDK.dll

```
enter DecryptObfuscatedPackage size:{}
CNamaSDK::BundleHelper::DecryptObfuscatedPackage
DecryptObfuscatedPackage Failed!
Decrypt bundle failed, error:{}
CNamaSDK::BundleHelper::VerifySignature
is_controller_resource_bundle
fu_mbedtls_cipher_crypt
AES-128-CBC, AES-192-CBC, AES-256-CBC
AES-128-ECB, AES-256-GCM
PKCS#1 encryption
SOFTWARE\Microsoft\Cryptography
CryptAcquireContextA, CryptGenRandom
```

### Hypothesis: Encryption Flow

```
Original Bundle (JSON + textures + shaders + models)
  ↓
  Compress (zlib? - need to check)
  ↓
  Encrypt with AES-CBC (key from SDK or license)
  ↓
  Obfuscate (XOR or custom)
  ↓
  Sign (RSA or HMAC) for VerifySignature
  ↓
  Prepend header with magic F3 5B 06 12 + IV + size + signature
  ↓
  Save as .bundle file
```

**Decryption flow in SDK:**

```cpp
// Pseudo-code from strings
bool BundleHelper::DecryptObfuscatedPackage(data, size) {
    // 1. Read header, get magic, IV, size
    // 2. VerifySignature
    // 3. Decrypt via mbedtls AES-CBC
    // 4. Decompress
    // 5. Return decrypted payload
}
```

**Key storage:** Likely in CNamaSDK.dll or derived from device ID / license. We **will not attempt to extract key**.

**Compliance:** We will design HuanFace SDK with **our own open bundle format** (e.g., ZIP with JSON manifest) that does not use FaceUnity's encryption. Users can provide their own assets legally.

---

## 4. Decrypted Payload Hypothesis

Based on logs, strings, and FaceUnity public documentation (not from decrypted file):

### 4.1 Manifest / Metadata

**Possible JSON:**
```json
{
  "name": "face_makeup",
  "version": "1.0",
  "type": "controller",
  "dependencies": [],
  "params": {
    "is_makeup_on": true,
    "makeup_intensity": 1.0,
    "makeup_intensity_lip": 1.0,
    "makeup_intensity_eye": 1.0,
    "makeup_intensity_eyeBrow": 1.0,
    "makeup_intensity_eyeLiner": 1.0,
    "makeup_intensity_eyelash": 1.0,
    "makeup_intensity_blusher": 1.0,
    "makeup_intensity_foundation": 1.0,
    "makeup_intensity_pupil": 1.0
  }
}
```

### 4.2 JavaScript Controller

**Evidence from logs:**
```
[js] liufei init stbline in
[js] liufei statesArray init,len: 1
[js] liufei init stbline ok
[js] liufei RegiterStb in
[js] liufei show makeup: Group_1_13
```

- FaceUnity supports JS for controller logic (state machine, animation, param binding)
- `liufei` is developer name, `stbline` maybe stroke-based line rendering for eyeliner?
- `Group_1_13` suggests makeup grouping (group 1, item 13)

**Hypothetical JS:**
```javascript
function init() {
    // init stroke line for eyeliner
}
function showMakeup(group) {
    // show makeup group
}
```

### 4.3 Textures

**Expected files (from logs `load texture failed`):**
- `eyepupil.png` — pupil color
- `eyeliner.png` — eyeliner style
- `eyelash.png` — eyelash
- `brow.png` — eyebrow
- `eye.png`, `eye2.png`, `eye3.png`, `eye4.png` — eyeshadow layers?
- `lip_top2.png`, `lip_bz1.png` — lip textures
- `zhuangrong_sh.png`, `zhuangrong_sh2.png`, `zhuangrong_fd.png`, `zhuangrong_gg.png`, `zhuangrong_yy.png` — Chinese names for makeup? `zhuangrong` = makeup in Chinese, `sh` = ?, `fd` = foundation?, `gg` = ?, `yy` = eyeshadow?
- `gloss_lut.png` — LUT for lip gloss

**Binding via `SetParamTex`:**
- `tex_brow`, `tex_eye`, `tex_eyeLash`, `tex_eyeLiner`, `tex_blusher`, `tex_lip_mask_zz`, `tex_lip`, `tex_pupil`, `tex_foundation`, `tex_shadow`, etc.

**Format:** Likely PNG with alpha, possibly compressed inside bundle

### 4.4 Shaders

**From strings:**
- `MakeupFilterPassNAMA`, `MakeupWarpNAMA`, `MakeupPipeline2`, `MakeupPipeline`, `lip_mask`, `lip_highlight_mask`, `lip_polygon_shader`, `lip_mask_preprocess_shader`, `lip_mask_blur_shader`, `makeup_lip_gloss_blur`, `makeup_lip_gloss_highpass`, `makeup_lip_gloss_final`

**Hypothesis:** GLSL shaders for:
- Face warp (for makeup alignment)
- Lip mask generation
- Eye, brow, blush rendering
- Gloss effect (blur, highpass, final)

**Example (hypothetical):**
```glsl
uniform sampler2D tex_input;
uniform sampler2D tex_brow;
uniform float makeup_intensity_eyeBrow;
varying vec2 uv;
void main() {
    vec4 base = texture2D(tex_input, uv);
    vec4 brow = texture2D(tex_brow, uv);
    // blend based on mask and intensity
    gl_FragColor = mix(base, brow, brow.a * makeup_intensity_eyeBrow);
}
```

### 4.5 Meshes

**From strings:**
- `g_makeup_vbo`, `g_makeup_ebo` — vertex buffer objects for makeup
- `armesh_vertex_num`, `face_meshV2`, `arMesh`

**Hypothesis:** Face mesh with vertices for makeup placement, lip polygon, eye regions

### 4.6 Parameters

**Full list from strings and logs:**

**Makeup intensity:**
- `makeup_intensity`, `makeup_intensity_lip`, `makeup_intensity_pupil`, `makeup_intensity_eye`, `makeup_intensity_eyeLiner`, `makeup_intensity_eyelash`, `makeup_intensity_eyeBrow`, `makeup_intensity_blusher`, `makeup_intensity_foundation`, `makeup_intensity_highlight`, `makeup_intensity_shadow`, `makeup_intensity1`, `makeup_intensity2`, `makeup_intensity3`

**Makeup color:**
- `makeup_lip_color`, `makeup_lip_color2`, `makeup_eye_color`, `makeup_eye_color2`, `makeup_eye_color3`, `makeup_eye_color4`, `makeup_eyeLiner_color`, `makeup_eyelash_color`, `makeup_eyeBrow_color`

**Makeup flags:**
- `is_makeup_on`, `is_clear_makeup`, `makeup_occlusion`, `makeup_occlusion_type`, `makeup_lip_occlusion`, `makeup_lip_highlight`, `makeup_lip_highlight_enable`, `makeup_lip_highlight_strength`, `makeup_lip_color_mix_strength`, `makeup_lip_mask`

**Textures:**
- `tex_brow`, `tex_eye`, `tex_eye2`, `tex_eye3`, `tex_eye4`, `tex_pupil`, `tex_eyeLash`, `tex_lip`, `tex_eyeLiner`, `tex_blusher`, `tex_blusher2`, `tex_foundation`, `tex_shadow`, `tex_lip_highlight`, `tex_lip_mask_bz`, `tex_lip_mask_zz`, `tex_lip_mask_bite_bz`, `tex_lip_mask_bite_zz`, `tex_lip_mask_highlight_bz`, `tex_lip_mask_highlight_zz`, `tex_makeup`, `tex_lut`, `tex_lut2`, `tex_lipstick_median`, `tex_blend_weight_and_level_map`, `tex_face_occu_blur`, `tex_occudebug`

**Blend types:**
- `blend_type_tex_eye`, `blend_type_tex_eye2`, `blend_type_tex_eye3`, `blend_type_tex_eye4`, `blend_type_tex_brown`, `blend_type_tex_eyeLash`, `blend_type_tex_eyeLiner`, `blend_type_tex_blusher`, `blend_type_tex_blusher2`, `blend_type_tex_highlight`, `blend_type_tex_shadow`, `blend_type_tex_pupil`, `blend_type_tex_lip`

**Beauty (from INI and strings):**
- `HeavyBlur`, `ColorLevel`, `DelspotLevel`, `RedLevel`, `Clarity`, `Sharpen`, `FaceThreed`, `EyeBright`, `ToothWhiten`, `RemovePouchStrength`, `RemoveNasolabialFoldsStrength`, `Brightness`, `Saturation`, `BlurLevel`, `ColorLevelType`, `SkinDect`, etc.
- Plus face shape: `FaceSize`, `Eye`, `Nose`, `Chin` adjustments (inferred)

**Confidence:** HIGH for param names (from strings), LOW for exact usage.

### 4.7 AI Models

**For `ai_face_processor_pc.bundle`:**
- Face detection model
- Landmark model (maybe 75 or 106 points)
- Face mesh V2
- Occlusion, hair mask, head mask
- Eye, lip, etc. segmentation

**For `ai_human_processor_pc.bundle`:**
- Human detection
- 2D/3D joints
- Human mask
- Action recognition
- Hand detection, gesture
- BVH motion

**Format:** Likely TFLite or custom CNN, based on `fuai.dll` containing TFLite quantization strings

---

## 5. Bundle Types

Based on file locations and names:

| Type | Example | Size | Magic | Purpose |
|------|---------|------|-------|---------|
| Face Beautification | `face_beautification.bundle` | 5.2 MB | F3 5B 06 12 | Skin smooth, whitening, etc. |
| Face Makeup Controller | `face_makeup.bundle` | 702 KB | F3 5B 06 12 | Base makeup controller |
| Body Slim | `body_slim.bundle` | 16 KB | F3 5B 06 12 | Body shaping |
| Background Blur | `background_blur.bundle` | 125 KB | 35 59 CD 0B | Background segmentation |
| AI Face Processor | `ai_face_processor_pc.bundle` | 23 MB | F3 5B 06 12 | Face detection/tracking |
| AI Human Processor | `ai_human_processor_pc.bundle` | 42 MB | F3 5B 06 12 | Human pose/segmentation |
| Custom Makeup | `026B94DC...bundle` | 20-100 KB | F3 5B 06 12 or variant | Lip, eye, brow, etc. presets |
| Style Makeup | `35852020...bundle` | 2.4 MB | F3 5B 06 12 | Curated makeup look (multiple effects) |
| Hair | `7F6145EB...bundle` | 611 KB | F3 5B 06 12 | Hair dye/segmentation |
| Special Effects | `86C46C3F...bundle` | Varies | F3 5B 06 12 | Stickers, props, filters |

---

## 6. Bundle Loading Flow (from logs)

```
1. fuSetup() — init SDK, check license, init GL context
2. fuCreateItemFromPackage(data, size) — load bundle from file/memory
   - Internally calls DecryptObfuscatedPackage
   - VerifySignature
   - Parse manifest, JS, textures, shaders, meshes, params
   - Create item handle (int)
   - Log: "created item name: face_makeup", "handle = 2, item_name ="
3. fuBindItems(target, items) — bind makeup items to face beautification item?
4. fuItemSetParamd(handle, "makeup_intensity_lip", 0.8)
   fuCreateTexForItem(handle, "tex_lip", data, width, height)
   - Set params and textures
5. fuRenderBundles() or fuRenderItems() — render with current face data
6. fuDestroyItem(handle) — unload
7. fuDestroyAllItems(), fuOnDeviceLost(), fuDestroyLibData() — cleanup
```

**Evidence from logs:**
```
[info][NamaContext.cpp:1534] created item name: face_makeup
[info][CNamaSDK.cpp:890] fuCreateItemFromPackage: handle = 2, item_name =
[info][makeupController.cpp:2037] debug++ SetParamTex called tex_lip_mask_zz
[info][CNamaSDK.cpp:1030] fuDestroyItem: handle = 2
[info][CNamaSDK.cpp:1039] fuDestroyAllItems called
```

---

## 7. HuanFace SDK Bundle Format (Proposed, Clean-Room)

Since we cannot and will not use FaceUnity's encrypted format, we propose an **open, compatible-but-independent format** for HuanFace SDK:

### 7.1 Option A: ZIP-based (Recommended for simplicity)

```
my_makeup.hfbundle (ZIP)
├── manifest.json
├── textures/
│   ├── lip.png
│   ├── eye.png
│   └── brow.png
├── shaders/
│   ├── vertex.glsl
│   └── fragment.glsl
├── meshes/
│   └── face_mask.json
└── controller.js (optional)
```

**manifest.json:**
```json
{
  "name": "natural_makeup",
  "version": "1.0",
  "type": "makeup",
  "description": "Natural everyday makeup",
  "author": "HuanFace",
  "params": {
    "intensity": {"type": "float", "default": 1.0, "min": 0, "max": 1},
    "lip_color": {"type": "color", "default": "#FF0000"},
    "eye_color": {"type": "color", "default": "#000000"}
  },
  "textures": {
    "tex_lip": "textures/lip.png",
    "tex_eye": "textures/eye.png",
    "tex_brow": "textures/brow.png"
  },
  "shaders": {
    "vertex": "shaders/vertex.glsl",
    "fragment": "shaders/fragment.glsl"
  },
  "dependencies": []
}
```

**Pros:**
- Simple, standard ZIP
- Easy to create and parse
- Textures are plain PNG
- Shaders are plain GLSL
- JSON manifest human-readable

**Cons:**
- Not encrypted (but we can add optional AES if user wants)
- Larger than binary format

### 7.2 Option B: Custom Binary (More efficient)

```
HuanFace Bundle Binary
├── Header (32 bytes)
│   ├── Magic: "HFBN" (48 46 42 4E)
│   ├── Version: uint32
│   ├── Manifest offset: uint32
│   ├── Manifest size: uint32
│   ├── Texture count: uint32
│   ├── Shader count: uint32
│   └── Flags: uint32
├── Manifest JSON (variable)
├── Texture Table
│   ├── [name, offset, size, width, height, format] * count
│   └── Texture data (PNG or raw)
├── Shader Table
│   ├── [name, offset, size] * count
│   └── Shader data (GLSL text)
└── Mesh Table (optional)
```

**Pros:**
- More efficient, single file
- Can add optional compression (zlib)
- Can add optional encryption (AES) with user-provided key (not bypassing FaceUnity)

**Cons:**
- More complex to implement
- Need custom parser

### 7.3 Decision

**Start with Option A (ZIP) for Phase 2-4 prototype**, then optimize to Option B for production if needed.

**Compatibility with FaceUnity bundles:** We will **not** attempt to load FaceUnity's encrypted bundles directly in HuanFace SDK unless user provides decrypted resources legally (e.g., they own the assets and have license to use them). Instead, we provide a **conversion tool** that, given decrypted resources (via official FaceUnity SDK API with valid license), can pack them into HuanFace format.

**This avoids any DRM bypass and respects FaceUnity's IP.**

---

## 8. Tools to Build

### bundle_inspector (Python)

**Features:**
- Read bundle file, check magic, size, entropy
- Detect encryption (entropy > 7.5)
- Try to list param names from known list (from DLL strings)
- Output:
  ```
  Bundle: makeup_xxx
  Path: ProgramData/.../026B94DC...bundle
  Size: 51695
  Magic: f35b0612090dcbfa (primary)
  Entropy: 7.79 (encrypted)
  Type: custom makeup (hypothesis)
  Textures (expected): eyepupil.png, eyeliner.png, etc. (from logs)
  Params (from strings): tex_eye, tex_brow, makeup_intensity_*, etc.
  Dependencies: face_makeup.bundle (hypothesis)
  Confidence: MEDIUM
  ```

### asset_inspector

- Catalog PNGs, map to bundles via name similarity or config.dat if possible

### texture_inspector

- Load PNG, check size, format, alpha

### metadata_inspector

- Parse INI locale, extract beauty/makeup param list
- Parse JSON marketplace metadata

---

## 9. Validation Steps

1. **Header validation:** Confirm magic `F3 5B 06 12` appears in 70% of bundles, variant magics in 30%
2. **Entropy validation:** All bundles entropy 7.7-7.85 — encrypted
3. **String validation:** `DecryptObfuscatedPackage` and `VerifySignature` exist in CNamaSDK.dll — proves encryption + signature
4. **Log validation:** `fuCreateItemFromPackage` creates items named `face_makeup`, `body_beautify`, etc. — proves bundle loading flow
5. **Param validation:** `SetParamTex` logs show `tex_*` params — proves texture binding
6. **JS validation:** `[js] liufei` logs prove JS inside bundles

**All validations passed with HIGH confidence for encryption existence, MEDIUM for internal structure.**

---

## 10. Legal Notes

- **No decryption attempted.** No key extraction.
- **No signature bypass.** We respect VerifySignature.
- **Clean-room design.** HuanFace bundle format is independent, open, and does not use FaceUnity's encryption or proprietary structure.
- **Conversion tool:** If user has legal access to decrypted FaceUnity resources via official SDK with valid license, they can convert to HuanFace format using our tool — this is legal as long as they own the assets and comply with FaceUnity license.

---

**End of Bundle Format Analysis**


---

## 11. Clean-Room Distinction (Phase 1 Correction)

### OBSERVED FORMAT (Facts)

- 267 .bundle files, all encrypted
- Magic: F3 5B 06 12 in 70% (187 files), variant magics in 30% (80 files): 7C D3 33 56, 10 11 D2 D5, B2 AA 43 33, 3D E6 15 6D, 35 59 CD 0B etc.
- Entropy: 7.7-7.85 (first 1K) — close to 8.0 = random, encrypted
- No PK (zip), no JSON, no PNG header in first 256 bytes
- Sizes: 4.2KB smallest, 42MB largest
- File locations: core bundles in common/assets/graphics/ and model/, presets in ProgramData/obsplus/beauty/
- Strings in CNamaSDK.dll: DecryptObfuscatedPackage, VerifySignature, AES-128-CBC, fu_mbedtls_cipher_crypt, CryptAcquireContextA
- Logs: fuCreateItemFromPackage creates items named face_makeup, body_beautify, new_greensegment_fucreator_1.0.5_release, hair_normal, dummy; SetParamTex tex_*; load texture failed 16 names; JS liufei; GL 4.6

### INFERRED FORMAT (Hypotheses with evidence)

- Bundles ARE encrypted + signed — from magic, entropy, DecryptObfuscatedPackage, VerifySignature, AES strings — Confidence HIGH
- Uses mbedtls AES-CBC, with RSA signature, key from SDK or license, uses Windows Crypto API for random — Confidence MEDIUM-HIGH (from strings, but not confirmed mode)
- Internal structure likely contains: manifest JSON (item name, type, version), JavaScript controller (liufei stbline, Group_1_13), textures (PNG, from load texture failed logs), shaders (GLSL/HLSL, from MakeupFilterPassNAMA strings), meshes (VBO/EBO, from g_makeup_vbo), parameters (makeup_intensity_*, tex_*, blend_type_*), AI models (TFLite, from fuai strings) — Confidence MEDIUM for existence, LOW for exact structure
- Variant magics possibly different bundle types (controller resource vs model) or different encryption versions — from is_controller_resource_bundle string, different locations — Confidence LOW-MEDIUM
- Config.dat (5 files, 11-80KB, entropy 6.27-6.58, magic 1a1a/3a43) is encrypted index mapping bundle hash to metadata — from directory co-location, size proportional to preset count — Confidence MEDIUM for purpose, LOW for structure

### PROTECTED (Not analyzed)

- Original FaceUnity bundle internal content: NOT analyzed — would require decryption key or bypass of VerifySignature, which we do NOT do
- Original FaceUnity shaders: MakeupFilterPassNAMA, MakeupWarpNAMA, etc. — existence observed from DLL strings (HIGH), purpose inferred (MEDIUM), implementation PROTECTED (no bypass)
- Original AI models inside ai_face_processor and ai_human_processor bundles: weights and architecture PROTECTED
- Config.dat internal structure: PROTECTED (encrypted)
- Private keys, secret keys, license keys: PROTECTED (not searched, not extracted)

### PROPOSED HUANFACE FORMAT (Clean-room, open, NOT reverse-engineered original)

**HuanFace bundle is a proposed clean-room target format, ZIP-based:**

```
my_makeup.hfbundle (ZIP)
├── manifest.json
│   ├── name, version, type, description, author
│   ├── params: {intensity: {type: float, default: 1.0}, color: {type: color, default: "#FF0000"}}
│   ├── textures: {tex_lip: "textures/lip.png"}
│   ├── shaders: {vertex: "shaders/vertex.glsl", fragment: "shaders/fragment.glsl"}
│   └── dependencies: []
├── textures/ (PNG, RGBA)
├── shaders/ (GLSL vertex/fragment, clean-room)
├── meshes/ (JSON/OBJ, optional)
└── controller.js (optional, JS state machine, clean-room)
```

**This is PROPOSED OPEN FORMAT, not reverse-engineered original. Original is PROTECTED.**

**Conversion tool (future, legal only):** If user has legal access to decrypted FaceUnity resources via official SDK with valid license, they can convert to HuanFace format using our tool — legal as long as they own assets and comply with FaceUnity license. No bypass.

---
