# HuanFace — Repository Audit (Phase 0)

**Date:** 2026-09-28  
**Branch:** arena/01a0e5f5-huanface  
**Total Files (excluding .git):** 965  
**Total Size:** ~655 MB  
**Audit Confidence:** HIGH for structure, MEDIUM for bundle internals (encrypted)

---

## 1. Repository Structure Overview

This repository is **not a source code repository** in the traditional sense. It is a **snapshot of installed OBS plugin data** for `obs-cam-beauty` (OBS Beauty Camera) based on FaceUnity (NamaSDK).

```
HuanFace/
├── AppData/
│   └── Roaming/
│       ├── plugins/obs-cam-beauty/      # Main plugin installation
│       │   ├── bin/32bit,64bit          # DLLs + EXEs (CNamaSDK, fuai, OBS bridge)
│       │   ├── common/assets/
│       │   │   ├── graphics/            # Core bundles: face_beautification, face_makeup, body_slim
│       │   │   ├── items/BackgroundSegmentation/
│       │   │   └── model/               # AI processors: ai_face_processor_pc, ai_human_processor_pc
│       │   └── data/                    # OBS effects, locale INI, MobileNetSSD model, UI icons
│       ├── store/                       # OBS Store marketplace plugin (core.dll, websocket.dll)
│       ├── locale/                      # Marketplace metadata (allinfo-en-US.json, ui-en-US.json)
│       ├── proxy/                       # Proxy config (gateway.json)
│       └── logs/                        # Runtime logs: camera_beauty/main, camera_beauty/nama, obsplus/host
│
├── ProgramData/
│   ├── obs-studio/
│   │   ├── plugins/obsplus, win-spout   # OBSPlus host + Spout texture sharing
│   │   └── shader-cache/*.v2            # 223 compiled OBS shaders (binary cache)
│   └── obsplus/beauty/
│       ├── beauty-hair/                 # Hair beautification: models/*.bundle + thumbs/*.png + config.dat
│       ├── filters/                     # Filter presets: thumbs/*.png + config.dat
│       ├── makeup/8.10.0/
│       │   ├── custom/models/*.bundle   # 60+ custom makeup bundles (hashed names)
│       │   ├── style/{1..28}/*.bundle   # Curated style bundles (one per style)
│       │   ├── custom_config.dat
│       │   └── style_config.dat
│       └── special-effects/             # 150+ sticker/prop bundles, each in numbered folder
│
├── bin/                                 # Duplicate of AppData/.../bin (root copy)
├── common/                              # Duplicate of AppData/.../common
└── data/                                # Duplicate of AppData/.../data
```

> **Observation:** `bin/`, `common/`, `data/` at root are exact duplicates of `AppData/Roaming/plugins/obs-cam-beauty/`. This suggests the repo was created by copying the OBS plugin folder to root for convenience. **Confidence: HIGH**

---

## 2. File Type Statistics

| Extension | Count | Suspected Category |
|-----------|-------|--------------------|
| .png | 341 | Thumbnails for makeup/hair/filters/special-effects + UI icons |
| .bundle | 267 | FaceUnity encrypted bundles (core + presets) |
| .v2 | 223 | OBS shader cache (compiled GPU shaders) |
| .dll | 37 | Native libraries: CNamaSDK, fuai, OBS bridges, Spout, Store |
| .log | 31 | Runtime logs (NamaSDK, camera_beauty, obsplus) |
| .ini | 19 | Locale translations (en-US, zh-CN, ja-JP, etc) |
| .exe | 11 | Helper tools: beauty.exe, camera-tool.exe, obs-plugin-console.exe |
| .svg | 8 | UI vector icons (Dark/Light themes) |
| .dat | 6 | Encrypted preset indexes (config.dat) |
| .json | 5 | Proxy gateway, marketplace metadata, notice config |
| .effect | 4 | OBS shader effects (HLSL-like) for color conversion |
| .caffemodel/.prototxt | 4 | MobileNetSSD Caffe model (fallback detection) |
| .gif | 2 | Loading animation |
| .bin | 1 | Token binary |

**Generated file:** `analysis/file_type_statistics.json`

---

## 3. Binary Inventory (Critical)

### 3.1 Core SDK DLLs

| File | Size | Purpose | Confidence |
|------|------|---------|------------|
| `bin/64bit/CNamaSDK.dll` | 19.5 MB | FaceUnity NamaSDK wrapper — main rendering, bundle decryption, makeup/beauty pipeline | HIGH |
| `bin/64bit/fuai.dll` | 29 MB | FaceUnity AI inference — face/human detection, landmarks, segmentation | HIGH |
| `bin/64bit/obs-cam-beauty.dll` | 63 KB | OBS plugin entry point (obs-studio plugin API) | HIGH |
| `bin/64bit/obs-cam-beauty_qt5.dll` / `_qt6.dll` | 2.4 MB | Qt UI for beauty panel | MEDIUM |
| `bin/64bit/beauty.exe` | 1.4 MB | Standalone beauty helper / test harness | MEDIUM |
| `bin/64bit/camera-tool.exe` | 176 KB | Camera occupation detection tool (from logs: "Check Usage") | HIGH |

**Evidence from strings:**
- `CNamaSDK.dll` contains 200+ `fu*` APIs: `fuCreateItemFromPackage`, `fuRenderBundles`, `fuItemSetParamd`, `fuBindItems`, `fuFaceProcessorSetFaceLandmarkQuality`, etc.
- `CNamaSDK.dll` contains `DecryptObfuscatedPackage`, `VerifySignature`, `Decrypt bundle failed`, `is_controller_resource_bundle` — **proves bundles are encrypted/obfuscated**.
- `CNamaSDK.dll` contains mbedtls strings: `AES-128-CBC`, `fu_mbedtls_cipher_crypt`, `PKCS#1 encryption` — encryption library is mbedtls.
- Makeup pipeline strings: `makeupController.cpp`, `tex_brow`, `tex_eye`, `tex_eyeLiner`, `tex_eyeLash`, `tex_blusher`, `tex_lip`, `tex_pupil`, `makeup_intensity_*`, `lip_mask`, `MakeupFilterPassNAMA`.
- `fuai.dll` contains face mesh, human processor, avatar, BVH motion strings.

**Generated file:** `analysis/binary_catalog.json`

### 3.2 Other DLLs

- `obsplus.dll` (456 KB): OBSPlus host framework — loads obs-cam-beauty.
- `Spout.dll`, `SpoutDX.dll`, `SpoutLibrary.dll`, `win-spout.dll`: Spout texture sharing for OBS (allow beauty output to other apps).
- `core.dll`, `obs-plugins-store.dll`, `websocket.dll`, `store_qt5/6.dll`: OBS Store marketplace.

---

## 4. Bundle Inventory (267 files)

### 4.1 Core Bundles (AppData/.../common/assets)

| Bundle | Size | Purpose | Magic | Entropy |
|--------|------|---------|-------|---------|
| `face_beautification.bundle` | 5.27 MB | Base beauty: skin smooth, whitening, sharpen, tone | `f35b0612` | 7.79 |
| `face_makeup.bundle` | 702 KB | Base makeup controller logic + shaders | `f35b0612` | 7.79 |
| `body_slim.bundle` | 16.5 KB | Body shaping (slim face/legs) | `f35b0612` | 7.80 |
| `ai_face_processor_pc.bundle` | 23.3 MB | AI face processor — detection, landmarks, face mesh | `f35b0612` | 7.83 |
| `ai_human_processor_pc.bundle` | 42.3 MB | AI human processor — human pose, segmentation | `f35b0612` | 7.81 |
| `background_blur.bundle` | 125 KB | Background segmentation (green) | `3559cd0b` | 7.82 |

**Magic Byte Analysis:**
- **Primary magic:** `f3 5b 06 12` appears in ~70% of bundles (all core + most makeup). **Hypothesis:** FaceUnity obfuscated bundle v1.
- **Secondary magics:** `7c d3 33 56`, `10 11 d2 d5`, `b2 aa 43 33`, etc. appear in ~30% of bundles (some custom makeup). **Hypothesis:** Either different encryption version, or different bundle type (controller resource vs model), or older format. **Confidence: MEDIUM, needs validation.**

**Entropy:** All bundles show entropy 7.7-7.85 (close to 8.0 = random) — **strong evidence of encryption/compression**. No bundle starts with `PK` (zip) or readable JSON. **Confidence: HIGH that bundles are encrypted.**

### 4.2 Preset Bundles (ProgramData/obsplus/beauty)

| Category | Count | Path Pattern | Thumbnail |
|----------|-------|--------------|-----------|
| Custom Makeup | ~60 | `makeup/8.10.0/custom/models/*.bundle` | No thumbs, but `custom_config.dat` indexes them |
| Style Makeup | 24 | `makeup/8.10.0/style/{1..28}/*.bundle` | No thumbs, but `style_config.dat` |
| Hair | 2 models + 22 thumbs | `beauty-hair/models/*.bundle` + `thumbs/hair_*.png` | Yes — gradient + normal |
| Filters | 0 bundles + 100+ thumbs | `filters/thumbs/*.png` + `config.dat` | Yes — color filter previews |
| Special Effects | 150+ | `special-effects/{id}/*.bundle` | No thumbs in repo? Possibly remote |

**Naming:** Custom/style bundles use **MD5-like hex names** (32-char uppercase hex, e.g., `026B94DC68A10EB6992B94333E856A1F.bundle`). This suggests content-addressable storage or remote download with hash verification.

**Config.dat files:** 5 files (`beauty-hair/config.dat`, `filters/config.dat`, `makeup/.../custom_config.dat`, `style_config.dat`, `special-effects/config.dat`) — sizes 11KB-80KB, **not readable as text**, high entropy, first bytes not JSON. **Hypothesis:** Encrypted/obfuscated index mapping bundle hash to metadata (name, preview, parameters). **Confidence: MEDIUM.**

---

## 5. Asset Inventory

### 5.1 Images (341 PNG)

- **Hair thumbs:** 22 files `hair_gradient_*.png`, `hair_normal_*.png` — 200x200 approx, preview of hair dye.
- **Filter thumbs:** 100+ files with hex names (e.g., `04BC983D74C989ADD8B0C1CB844953BF.png`) — color filter previews.
- **UI icons:** `download.png`, `favorite.png`, `dowloading.png`, `favorited.png` — store UI.
- **SVG icons:** 4 files in `data/themes/Dark|Light/download.svg`, `favorite.svg` — vector UI.

**Extractable:** PNGs are standard and viewable. **Confidence: HIGH.**

### 5.2 Shaders / Effects

- `data/default.effect`: **OBS built-in effect** — contains 11 techniques for drawing textures with various color space handling (sRGB linear/nonlinear, Rec709/Rec2020, ST2084 PQ, HLG, alpha divide, tonemap). **Not FaceUnity shader.** It's OBS's final compositing shader.
- `data/format_conversion.effect`: OBS format conversion (not fully read yet, but similar).
- `ProgramData/obs-studio/shader-cache/*.v2`: 223 files — **OBS compiled shader cache**, binary, not human-readable, size ~ few KB each. Likely D3D11 bytecode cache.

**Suspected FaceUnity shaders are INSIDE bundles** (encrypted), not exposed as separate files. **Confidence: HIGH.**

### 5.3 Models

- `data/model/MobileNetSSD_deploy.caffemodel` (22 MB) + `.prototxt`: Standard Caffe MobileNetSSD for object detection. Used as fallback for background segmentation or person detection when AI processor not loaded? Logs don't show direct usage but file exists.
- `ai_face_processor_pc.bundle` and `ai_human_processor_pc.bundle` likely contain **TFLite or custom CNN models** for face/human.

### 5.4 Locale / Config

- `data/locale/en-US.ini` etc (5 languages): Contains 200+ keys for beauty parameters — evidence of supported features:
  - **BaseBeauty:** `HeavyBlur` (skin smooth), `ColorLevel` (whitening), `DelspotLevel` (blemish removal), `RedLevel` (rosiness), `Clarity`, `Sharpen`, `FaceThreed`, `EyeBright`, `ToothWhiten`, `RemovePouchStrength`, `RemoveNasolabialFoldsStrength`, `Brightness`, `Saturation`
  - **Filters:** `White`, `Pink`, `Fresh`, `CoolTone`, `WarmTone`, etc.
  - **Face Shape:** From other INI sections (not yet fully parsed but likely `FaceSize`, `Eye`, `Nose`, `Chin` adjustments)
  - **Makeup:** Lip, eye, brow, etc. (implied by logs)
  - **Background:** Blur
  - **Body:** Slim

---

## 6. Logs Analysis (31 logs)

### 6.1 NamaSDK Logs (`logs/camera_beauty/nama/*.log`)

Critical findings:

```
[info][NamaContext.cpp:1534] created item name: face_makeup
[info][CNamaSDK.cpp:890] fuCreateItemFromPackage: handle = 2
[info][NamaContext.cpp:1534] created item name: body_beautify
[info][NamaContext.cpp:1534] created item name: new_greensegment_fucreator_1.0.5_release
[info][NamaContext.cpp:1534] created item name: hair_normal
[info][NamaContext.cpp:1534] created item name: dummy

[warning][makeupController.cpp:2706] load texture failed!:eyepupil.png
[warning][makeupController.cpp:2706] load texture failed!:eyeliner.png
[warning][makeupController.cpp:2706] load texture failed!:eyelash.png
[warning][makeupController.cpp:2706] load texture failed!:brow.png
[warning][makeupController.cpp:2706] load texture failed!:eye.png, eye2.png, eye3.png, eye4.png
[warning][makeupController.cpp:2706] load texture failed!:lip_top2.png, zhuangrong_*.png, lip_bz1.png, gloss_lut.png

[info][makeupController.cpp:2037] debug++ SetParamTex called tex_lip_mask_zz
[info][makeupController.cpp:2037] debug++ SetParamTex called tex_blusher, tex_eyeLash, tex_eyeLiner, tex_eye, tex_brow

[info][NamaContext.cpp:98] [js] liufei init stbline in
[info][NamaContext.cpp:98] [js] liufei show makeup: Group_1_13
```

**Interpretation:**
- **Item creation:** FaceUnity uses `fuCreateItemFromPackage` to create items from bundles. Items have names like `face_makeup`, `body_beautify`, `hair_normal`, `new_greensegment_fucreator_...`, `dummy`.
- **MakeupController** expects external textures (`eyepupil.png`, etc.) but fails to load them — suggests some makeup bundles reference external resources not included, or textures are supposed to be inside bundle but extraction failed.
- **SetParamTex** with names `tex_*` shows how makeup textures are bound: `tex_lip_mask_zz`, `tex_blusher`, `tex_eyeLash`, `tex_eyeLiner`, `tex_eye`, `tex_brow` — **these are the actual parameter names for makeup engine**.
- **JS context:** `NamaContext.cpp:98 [js]` indicates bundles contain **JavaScript logic** (FaceUnity supports JS for controller logic). `liufei` is likely developer name, `stbline` maybe stroke-based line? `Group_1_13` suggests makeup grouping.

**Confidence: HIGH for API usage, MEDIUM for JS logic.**

### 6.2 Main Logs (`logs/camera_beauty/main/*.log`)

Less detailed, but show initialization and errors.

### 6.3 OBSPlus Host Logs

Show plugin loading, proxy, etc.

---

## 7. Dependency Graph

```
OBS Studio
  ├── obsplus.dll (host)
  │     └── obs-cam-beauty.dll (plugin entry)
  │           ├── CNamaSDK.dll (core FaceUnity SDK)
  │           │     ├── fuai.dll (AI inference)
  │           │     ├── ai_face_processor_pc.bundle (face detection model)
  │           │     ├── ai_human_processor_pc.bundle (human detection model)
  │           │     ├── face_beautification.bundle (beauty filters)
  │           │     ├── face_makeup.bundle (makeup controller)
  │           │     ├── body_slim.bundle (body shape)
  │           │     ├── background_blur.bundle (background seg)
  │           │     ├── makeup custom/style bundles (60+)
  │           │     ├── hair models (2)
  │           │     └── special-effects (150+)
  │           ├── default.effect (OBS final composite)
  │           └── MobileNetSSD (fallback detection)
  └── win-spout (texture sharing)
```

**Generated file:** `analysis/dependency_graph.json`

---

## 8. Bundle Format Hypothesis (Preliminary)

Based on magic bytes, entropy, and strings:

```
Bundle File (encrypted)
├── Header (16-64 bytes)
│   ├── Magic: F3 5B 06 12 (primary) or variant
│   ├── Version / flags? (next 4-8 bytes, high entropy)
│   └── Possibly signature / checksum
├── Encrypted Payload (AES-128/256 CBC? - evidence from mbedtls)
│   ├── Decompressed via DecryptObfuscatedPackage
│   ├── Contains:
│   │   ├── Metadata JSON (item name, controller type)
│   │   ├── JavaScript (controller logic, e.g., "liufei init stbline")
│   │   ├── Textures (PNG/JPG, e.g., eyepupil.png)
│   │   ├── Shaders (GLSL/HLSL, e.g., MakeupFilterPassNAMA)
│   │   ├── Meshes / VBOs (g_makeup_vbo, g_makeup_ebo)
│   │   ├── Parameters (makeup_intensity_*, blend_type_*)
│   │   └── AI Models (TFLite)
│   └── May be further compressed (zlib?)
└── Footer / Signature (for VerifySignature)
```

**Evidence:**
- `DecryptObfuscatedPackage` + `VerifySignature` strings prove encryption + signature verification.
- `fu_mbedtls_cipher_crypt` indicates AES.
- `is_controller_resource_bundle` suggests bundle type differentiation.
- Logs show JS inside bundles.

**Confidence:**
- Encryption existence: HIGH
- AES usage: MEDIUM-HIGH (based on mbedtls strings, but not confirmed mode)
- Internal structure (JSON, JS, textures, shaders): MEDIUM (based on logs and FaceUnity public docs, not direct extraction)
- Need further analysis with tools that attempt to parse decrypted content (if we can obtain key legally or via SDK API — **but we must NOT bypass DRM**).

**Next Steps for Bundle Analysis:**
1. Build `bundle_inspector` tool that reads header, entropy, tries to detect compression.
2. Document all param names from strings (`makeup_intensity_*`, `tex_*`, `blend_type_*`).
3. Attempt to load bundle via official FaceUnity SDK API (if license available) to observe decrypted structure — **only via legitimate API, no bypass**.
4. If decryption key not available, treat bundle as opaque and design HuanFace SDK to support **compatible but independent format**.

---

## 9. Potential Extractable Resources

| Resource | Count | Extractable? | Method |
|----------|-------|--------------|--------|
| PNG thumbnails | 341 | YES | Direct file read |
| SVG icons | 8 | YES | Direct |
| INI locale | 19 | YES | Direct |
| JSON configs | 5 | YES | Direct |
| OBS .effect | 4 | YES | Direct text |
| Caffe model | 2 | YES | Direct |
| DLL strings | 37 | YES | `strings` tool |
| Bundles internal | 267 | NO (encrypted) | Requires decryption via SDK API (no bypass) |
| Config.dat | 6 | NO (encrypted) | Same as bundles |
| Shader cache .v2 | 223 | PARTIAL | Binary D3D11 cache, not easily extractable |

**Total extractable without decryption:** ~400 files  
**Total requiring decryption:** 273 files (267 bundles + 6 dat)

---

## 10. Major Dependencies

- **FaceUnity NamaSDK** (proprietary) — core
- **FaceUnity FUAI** (proprietary) — AI
- **OBS Studio** (GPL) — host
- **Qt5/Qt6** — UI
- **mbedTLS** (embedded in CNamaSDK) — crypto
- **MobileNetSSD Caffe** — fallback detection
- **Spout** (BSD) — texture sharing
- **OpenGL / D3D11** — rendering (inferred from shader cache and `fuInitGLContext`)

---

## 11. Confidence Assessment

| Area | Confidence | Reason |
|------|------------|--------|
| Repository structure | HIGH | Direct file listing, duplicates identified |
| Binary purpose | HIGH | Strings, logs, file names, sizes |
| Bundle encryption | HIGH | Magic `f35b0612`, entropy 7.8, Decrypt strings |
| Bundle internal structure | MEDIUM | Inferred from logs + FaceUnity public knowledge, not direct extraction |
| Makeup params | HIGH | From `makeupController.cpp` strings and `SetParamTex` logs: `tex_eye`, `tex_brow`, `tex_lip`, etc. |
| Beauty params | HIGH | From `en-US.ini`: skin smooth, whitening, etc. |
| Config.dat format | LOW | No readable content, need further analysis |
| Shader format | MEDIUM-HIGH | OBS effects are clear, FaceUnity shaders inside bundles unknown |

---

## 12. Recommended Next Phase

**Phase 1 — Asset & Binary Analysis (Deep Dive)**

- Build `tools/bundle_inspector.py` to:
  - Parse bundle header (magic, size, entropy)
  - Attempt to list internal file names from decrypted payload if possible via legitimate API
  - Catalog all `tex_*`, `makeup_*`, `blend_type_*` params
- Build `tools/texture_inspector.py` to catalog PNG thumbs and map to bundles via `config.dat` (if decryptable)
- Build `tools/metadata_inspector.py` to parse INI locale and extract beauty/makeup parameter list
- Document all `fu*` APIs found in CNamaSDK.dll (200+ functions) into `docs/API_REFERENCE.md`
- Analyze `config.dat` encryption — check if XOR or same AES as bundles (compare entropy, header patterns)
- Attempt to run `beauty.exe` in sandbox to observe behavior (without bypassing license)
- Create `docs/ASSET_FORMATS.md`, `docs/BINARY_ANALYSIS.md`, `docs/BUNDLE_FORMAT.md` with detailed findings

**Do NOT proceed to SDK implementation until bundle format is at least 70% understood and param list is documented.**

---

## 13. Files Generated

- `analysis/asset_catalog.json` — 965 entries with path, type, size, hash, purpose, confidence
- `analysis/binary_catalog.json` — 315 entries (DLLs, EXEs, bundles) with dependencies
- `analysis/file_type_statistics.json` — counts by extension and category
- `analysis/dependency_graph.json` — nodes and edges for high-level dependencies

---

## 14. Assumptions & Hypotheses Log

| # | Hypothesis | Evidence | Confidence | Next Validation |
|---|------------|----------|------------|-----------------|
| H1 | Bundles encrypted with AES-128/256 CBC via mbedtls | `fu_mbedtls_cipher_crypt`, `AES-128-CBC` strings, high entropy | MEDIUM-HIGH | Check CNamaSDK for key storage, attempt to find key via legitimate API documentation |
| H2 | Bundles contain JSON + JS + textures + shaders + meshes | Logs show JS (`[js] liufei`), `SetParamTex`, `g_makeup_vbo`, `MakeupFilterPassNAMA` | MEDIUM | Need decrypted sample (via official SDK if licensed) |
| H3 | `config.dat` uses same encryption as bundles | High entropy, similar file size patterns, located alongside bundles | LOW-MEDIUM | Compare header entropy, try to see if CNamaSDK has `config.dat` decryption function |
| H4 | `face_beautification.bundle` contains beauty pipeline (skin smooth, whitening, etc) | Name, size 5.2MB (largest among graphics), locale INI lists beauty params | HIGH | Search for beauty param strings inside decrypted payload (future) |
| H5 | `face_makeup.bundle` is controller that loads other makeup bundles | Name, logs show `face_makeup` item created, `makeupController.cpp` references | HIGH | Check if `face_makeup` bundle's JS references other bundles |
| H6 | PNG thumbs are mapped to bundles via `config.dat` | Hair and filter folders have both bundles/models and thumbs, plus config.dat index | MEDIUM | Parse config.dat if decrypted, or observe via running plugin |
| H7 | MobileNetSSD is fallback when AI processor bundles fail to load | File exists in `data/model`, but no log shows its usage; could be legacy | LOW | Search for MobileNetSSD strings in DLLs |

---

## 15. Legal & Compliance Notes

- **No DRM bypass attempted.** All analysis used static strings, file headers, logs, and entropy — no decryption without key.
- **No proprietary source code copied.** Only observed public API names (`fu*`) and param names from logs.
- **No license verification bypass.** Not attempting to run `fuSetup` with fake auth.
- **Focus remains on format analysis, behavioral analysis, and compatible clean-room SDK design.**

---

**End of Phase 0 Audit**
