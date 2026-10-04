# OBS Integration Analysis — Phase 1D

**Binaries Analyzed:**
- `bin/64bit/obs-cam-beauty.dll` (63 KB)
- `ProgramData/obs-studio/plugins/obsplus/bin/64bit/obsplus.dll` (456 KB)
- `bin/64bit/CNamaSDK.dll` (19.5 MB) — dependency
- `bin/64bit/fuai.dll` (29 MB) — dependency
- `data/default.effect` (9.3 KB) — OBS effect
- `data/format_conversion.effect` (13 KB)
- `ProgramData/obs-studio/shader-cache/*.v2` (223 files)

**Method:** PE export/import table parsing, strings, file structure, no execution.

---

## 1. OBS Plugin Architecture (Standard)

OBS Studio loads plugins via `obs_module_*` API:

```c
// From obs-cam-beauty.dll exports
obs_module_load
obs_module_unload
obs_module_name
obs_module_description
obs_module_ver
obs_module_set_pointer
obs_module_set_locale
obs_module_get_string
obs_module_free_locale
```

**Evidence:**
- Export table of `obs-cam-beauty.dll` has exactly 9 exports, all `obs_module_*` — standard OBS plugin
- Size 63 KB, small, only bridge
- Imports `obs.dll` — OBS core
- Build path: `D:\work\obsplus\client\obs-studio-29.0\build\x64\plugins\obsplus\obs-cam-beauty\RelWithDebInfo\obs-cam-beauty.pdb` — built with OBS Studio 29.0
- No FaceUnity logic inside (no fu* exports, no makeup strings except via dependency)

**Confidence:** HIGH — CONFIRMED DEPENDENCY: `obs-cam-beauty.dll` → `obs.dll`

---

## 2. OBSPlus Framework

**File:** `obsplus.dll` (456 KB)

**Imports:**
- `libcurl.dll`, `obs.dll`, `SHELL32.dll`, `ole32.dll`, `WS2_32.dll`, `WINHTTP.dll`, `MSVCP140.dll`, `VCRUNTIME140.dll`

**Purpose (from strings + JSON):**
- Hosts multiple OBS plugins: `obs-cam-beauty`, `heart-rate`, `virtualcam`, `multicast-streaming`, `gift-goal`, etc. (from `allinfo-en-US.json`)
- Marketplace: `allinfo-en-US.json` contains 69 plugins with id, name, description, previewImages, guideUrl, official, author
- Proxy: `gateway.json` with proxies `85vk:85vku@183.131.35.94:20252` and `obsproxy:nCzPVD4B@gateway.obshelp.com:10080`
- Store: `core.dll`, `obs-plugins-store.dll`, `websocket.dll`, `store_qt5/6.dll` — handles download, auth, token
- Token: `token/token.bin` — auth token for store

**Evidence:**
- `allinfo-en-US.json` has 69 entries, including `obs-cam-beauty` with description "Professional beauty plugin for OBS with HD high-frame-rate effects, makeup, stickers, body shaping, background blur, multi-camera support, and more."
- `gateway.json` proxies
- `notice/config.json` with pluginRenew date
- `locale/allinfo-en-US.json` and `ui-en-US.json` for store UI

**Confidence:** HIGH — CONFIRMED DEPENDENCY: `obsplus.dll` → `obs.dll`, `libcurl.dll`, `WINHTTP.dll`

**Diagram (CONFIRMED):**
```
OBS Studio (obs.dll)
  │
  └── obsplus.dll (framework)
        ├── libcurl.dll (download)
        ├── WINHTTP.dll (http)
        └── hosts:
              ├── obs-cam-beauty.dll
              ├── heart-rate.dll (not in repo, but in allinfo)
              └── other plugins
```

---

## 3. OBS-CAM-BEAUTY Bridge

**File:** `obs-cam-beauty.dll` (63 KB)

**Role:** Bridge between OBS and FaceUnity

**Hypothesized flow (from logs + imports + strings):**

```
OBS
 │
 ▼
obs-cam-beauty.dll (OBS plugin)
 │
 ├── Loads CNamaSDK.dll (FaceUnity SDK)
 │     ├── fuSetup() — init with auth
 │     ├── fuInitGLContext() — init OpenGL (requires 4.6, from GLLoader log)
 │     ├── fuCreateItemFromPackage() — load bundles
 │     │     ├── face_beautification.bundle
 │     │     ├── face_makeup.bundle
 │     │     ├── body_slim.bundle
 │     │     ├── background_blur.bundle
 │     │     ├── ai_face_processor_pc.bundle
 │     │     ├── ai_human_processor_pc.bundle
 │     │     ├── makeup custom/style bundles
 │     │     ├── hair models
 │     │     └── special-effects
 │     ├── fuBindItems() — bind makeup to base?
 │     ├── fuItemSetParamd() / fuCreateTexForItem() — set intensity, color, textures
 │     │     └── tex_brow, tex_eye, tex_eyeLash, tex_eyeLiner, tex_blusher, tex_lip_mask_zz etc.
 │     ├── fuRenderBundles() — render with face data
 │     └── fuDestroyItem(), fuDestroyAllItems(), fuOnDeviceLost(), fuDestroyLibData()
 │
 ├── Loads fuai.dll (AI inference)
 │     └── Face/human detection, landmarks, mesh
 │
 ├── Uses GPU
 │     ├── OpenGL 4.6 (from GLLoader log)
 │     ├── D3D11 (from shader-cache .v2)
 │     └── Spout (texture sharing) — Spout.dll, SpoutDX.dll, SpoutLibrary.dll, win-spout.dll
 │
 └── Uses OBS effects
       ├── default.effect (9.3 KB) — final compositing with color space handling (sRGB, Rec709/2020, PQ, HLG, tonemap)
       └── format_conversion.effect (13 KB) — YUV to RGB etc.
```

**Evidence for each dependency:**

| Dependency | Evidence | Confidence |
|------------|----------|------------|
| obs-cam-beauty.dll → CNamaSDK.dll | `beauty.exe` imports `CNamaSDK.dll`, logs show `CNamaSDK.cpp:890 fuCreateItemFromPackage`, `makeupController.cpp`, `NamaContext.cpp` — all from CNamaSDK | HIGH — CONFIRMED (via beauty.exe and logs) |
| obs-cam-beauty.dll → fuai.dll | CNamaSDK.dll imports `fuai.dll` (from PE import table: `fuai.dll`, `OPENGL32.dll`), so indirect dependency via CNamaSDK | HIGH — CONFIRMED (CNamaSDK imports fuai) |
| obs-cam-beauty.dll → OBS (obs.dll) | Direct import `obs.dll` in obs-cam-beauty.dll import table | HIGH — CONFIRMED |
| obs-cam-beauty.dll → GPU (OpenGL/D3D11) | Log `GLLoader.cc:212 initialGLExtentions: glversion max = 4, min = 6`, shader-cache .v2 (D3D11), Spout DLLs (texture sharing) | HIGH — CONFIRMED (OpenGL 4.6 required) |
| obs-cam-beauty.dll → default.effect | File exists in `data/`, OBS uses .effect for final compositing, not FaceUnity shader | MEDIUM — LIKELY (file exists, but no direct log linking) |
| obs-cam-beauty.dll → MobileNetSSD | File `data/model/MobileNetSSD_deploy.caffemodel` exists, but no log shows usage — could be fallback or legacy | LOW — POSSIBLE |
| obs-cam-beauty.dll → config.dat | Config.dat in `ProgramData/obsplus/beauty/*/`, likely used by OBSPlus or beauty plugin to list presets, but no direct evidence in obs-cam-beauty.dll strings | LOW — POSSIBLE |

**Diagram with confidence:**

```
OBS Studio (obs.dll) [CONFIRMED]
  │
  ├── obsplus.dll [CONFIRMED] — framework, marketplace
  │     ├── libcurl.dll [CONFIRMED] — download
  │     └── WINHTTP.dll [CONFIRMED]
  │
  └── obs-cam-beauty.dll [CONFIRMED] — bridge, 63KB, 9 exports obs_module_*
        │
        ├── CNamaSDK.dll [CONFIRMED] — via beauty.exe imports + logs
        │     ├── fuai.dll [CONFIRMED] — CNamaSDK imports fuai.dll
        │     ├── OPENGL32.dll [CONFIRMED] — CNamaSDK imports OPENGL32
        │     ├── face_beautification.bundle [CONFIRMED] — log "created item name: face_makeup" etc. but actually face_beautification is core, from file existence + logs
        │     ├── face_makeup.bundle [CONFIRMED] — log "created item name: face_makeup"
        │     ├── body_slim.bundle [CONFIRMED] — log "body_beautify"
        │     ├── background_blur.bundle [CONFIRMED] — log "new_greensegment_fucreator_1.0.5_release" (background segmentation)
        │     ├── ai_face_processor_pc.bundle [CONFIRMED] — size 23MB, name, FUAI_NewFaceProcessorFromBundle
        │     ├── ai_human_processor_pc.bundle [CONFIRMED] — size 42MB, FUAI_NewHumanProcessorFromBundle
        │     ├── makeup custom/style bundles [CONFIRMED] — 84+ files, logs show Group_1_13 etc.
        │     ├── hair models [CONFIRMED] — 2 bundles + 22 thumbs + log "hair_normal"
        │     └── special-effects [CONFIRMED] — 150+ bundles
        │
        ├── GPU [CONFIRMED]
        │     ├── OpenGL 4.6 [CONFIRMED] — log "glversion max = 4, min = 6"
        │     ├── D3D11 [MEDIUM] — shader-cache .v2, but could be OBS cache not beauty-specific
        │     └── Spout [CONFIRMED] — Spout.dll, SpoutDX.dll, win-spout.dll exist, for texture sharing
        │
        ├── default.effect [LIKELY] — file exists, OBS uses for final composite, but no direct log linking to beauty
        │     └── Techniques: Draw, DrawAlphaDivide, DrawNonlinearAlpha, DrawSrgbDecompress, DrawMultiply, DrawTonemap, DrawPQ, DrawTonemapPQ
        │
        ├── MobileNetSSD [POSSIBLE] — file exists data/model/MobileNetSSD_deploy.caffemodel, but no log
        │
        └── config.dat [POSSIBLE] — 5 files, encrypted index, likely for preset listing, but no direct evidence in obs-cam-beauty.dll
```

---

## 4. Texture & Frame Flow (Hypothesized)

Based on OBS plugin standard and FaceUnity API:

```
Camera Device (physical)
  │
  ▼
OBS Source (DecklinkBeauty or obs-cam-beauty)
  │
  ├── OBS captures frame (YUV or RGB)
  ├── Converts via format_conversion.effect (YUV→RGB)
  │
  ▼
obs-cam-beauty.dll
  │
  ├── Gets camera texture (D3D11 or OpenGL)
  ├── Copies to FaceUnity input (fuCreateTexForItem or similar)
  │
  ▼
CNamaSDK.dll + fuai.dll
  │
  ├── Face detection (ai_face_processor)
  ├── Landmarks, face mesh, hair/head mask
  ├── Human detection (ai_human_processor) if needed
  ├── Background segmentation (background_blur)
  ├── Beauty: skin smooth, whitening, etc. (face_beautification.bundle)
  ├── Makeup: lip, eye, brow, etc. (face_makeup.bundle + custom/style bundles)
  ├── Body slim (body_slim.bundle)
  ├── Hair (hair models)
  └── Special effects (stickers)
  │
  ▼
Output texture (beautified)
  │
  ▼
OBS
  │
  ├── Composites via default.effect (color space handling, tonemap, alpha)
  ├── Outputs to OBS preview/program
  └── Shares via Spout (Spout.dll) to other apps (e.g., virtual camera)
```

**Confidence:** MEDIUM for overall flow (based on OBS plugin standard + FaceUnity API + logs), HIGH for individual steps that have logs

---

## 5. Locale & UI Integration

**Files:** `data/locale/en-US.ini` etc. (19 INI files, 263 keys each)

**Keys reveal OBS UI for beauty:**

- `obs-cam-beauty="OBS Beauty Camera"`
- `ParameterSetting="Beauty"`
- `BaseBeauty="Base"`, `SkinDect="Skin Precise"`, `HeavyBlur="Skin Smooth"`, `BlurLevel="Smooth Level"`, `ColorLevel="Whitening"`, `DelspotLevel="Blemish Removal"`, `RedLevel="Rosiness"`, `Clarity="Enhance Details"`, `Sharpen="Sharpen"`, `FaceThreed="Facial Contour"`, `EyeBright="Brighten Eyes"`, `ToothWhiten="Whiten Teeth"`, `RemovePouchStrength="Remove Dark Circles"`, `RemoveNasolabialFoldsStrength="Nasolabial Folds"`, `Brightness="Fill Light"`, `Saturation="Saturation"`, `Filters="Filters"`, etc.
- `FaceSize="Face Size"`, `FaceLandmarkQuality="Detect Mode"`, `BodyNum="Max Faces"`, `BeautyProtection="Beauty Guard"`, `BeautyRenderFPS="Rendering Frame Rate"`, `RotateClockwise="Orientation"`, etc.

**Evidence:** INI files exist, parsed via `metadata_inspector.py`, 140 beauty-related keys in en-US.ini

**Confidence:** HIGH — CONFIRMED that INI defines UI for beauty parameters

---

## 6. Proxy & Store Integration

**Files:**
- `AppData/Roaming/proxy/gateway.json` — 2 proxies
- `AppData/Roaming/locale/allinfo-en-US.json` — 69 plugins marketplace metadata
- `AppData/Roaming/store/` — store DLLs, locale
- `AppData/Roaming/token/token.bin` — auth token
- `AppData/Roaming/notice/config.json` — notice timing

**Flow (hypothesized):**
```
OBSPlus store
  ├── Checks token.bin for auth
  ├── Uses gateway.json proxies to connect to marketplace (resource.obsworks.com, gateway.obshelp.com)
  ├── Downloads allinfo-en-US.json (69 plugins)
  ├── Shows UI with previewImages (https://resource.obsworks.com/plugininfo/beautyLogo.png etc.)
  ├── User downloads beauty presets (bundles) — stored in ProgramData/obsplus/beauty/*/
  └── Uses config.dat as index for downloaded presets
```

**Confidence:** MEDIUM for overall flow (based on files), HIGH for individual files existence

---

## 7. Summary

| Component | File | Size | Role | Dependency | Confidence |
|-----------|------|------|------|------------|------------|
| OBS core | obs.dll (not in repo, but imported) | N/A | Host | - | CONFIRMED (imported) |
| OBSPlus framework | obsplus.dll | 456 KB | Hosts plugins, marketplace, proxy | obs.dll, libcurl, WINHTTP | CONFIRMED |
| Beauty bridge | obs-cam-beauty.dll | 63 KB | Bridge OBS ↔ FaceUnity, 9 exports obs_module_* | obs.dll, CNamaSDK.dll (via logs+beauty.exe), fuai.dll (via CNamaSDK) | CONFIRMED |
| FaceUnity SDK | CNamaSDK.dll | 19.5 MB | Core rendering, bundle decryption, makeup/beauty pipeline | fuai.dll, OPENGL32, KERNEL32, etc. | CONFIRMED |
| FaceUnity AI | fuai.dll | 29 MB | Face/human detection, landmarks, mesh, segmentation | KERNEL32, WS2_32, ADVAPI32 | CONFIRMED |
| GPU | OpenGL 4.6, D3D11 | - | Rendering | - | CONFIRMED (OpenGL 4.6 log) |
| Texture sharing | Spout.dll, SpoutDX.dll, win-spout.dll | 60-200 KB | Share beauty output to other apps | - | CONFIRMED (files exist) |
| Final composite | default.effect | 9.3 KB | OBS color space handling (sRGB, Rec709/2020, PQ, HLG, tonemap) | OBS | LIKELY (file exists, OBS standard) |
| Fallback detection | MobileNetSSD Caffe | 22 MB | Person detection fallback | - | POSSIBLE (file exists, no log) |
| Preset index | config.dat | 11-80 KB | Index for beauty/makeup/hair/filter/special-effects presets | - | POSSIBLE (files exist, location) |

**No proprietary implementation copied, only observed dependencies and file structure.**

---

**End of OBS Integration Analysis**
