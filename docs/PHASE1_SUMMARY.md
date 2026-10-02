# PHASE 1 SUMMARY — Asset & Binary Analysis

**Date:** 2026-09-28  
**Branch:** arena/01a0e5f5-huanface  
**Previous Commit:** audit: complete Phase 0 repository audit  
**Status:** Phase 1 COMPLETE (documentation and evidence, no SDK implementation)

---

## API Surface

**Total exported APIs:**
- CNamaSDK.dll: 582 total exports, 373 fu* (C API)
- fuai.dll: 597 total exports, all FUAI_* (C++ mangled + C)
- obs-cam-beauty.dll: 9 exports, all obs_module_*
- **Total: 1,188 exports, 970+ relevant (373 + 597)**

**Known categories (CNamaSDK 373 fu* classified):**
- Initialization: 21 (fuSetup, fuDestroyLibData, fuGetVersion, fuIsLibraryInit, etc.)
- GL Context: 2 (fuInitGLContext, fuDestroyGLContext) + fuGetCurrentGLContext, fuMakeGLContextCurrent, fuIsGLPrepared, fuGetOpenGLSupported
- Item/Bundle: 2 (fuCreateItemFromPackage, fuCreateLiteItemFromPackage) + fuDestroyItem, fuDestroyAllItems
- Instance/Scene: 95 (fuCreateInstance, fuCreateScene, fuEnableInstance*, fuGetInstance*, etc.)
- Parameters: 23 (fuItemSetParamd, fuItemGetParamd, fuCreateTexForItem, fuDeleteTexForItem, fuSetMakeupCoverResource, etc.)
- Rendering: 11 (fuRender, fuRenderBundles, fuRenderBundlesEx, fuRenderBundlesSplitView, fuRenderItems, etc.)
- Face Tracking: 12 (fuTrackFace, fuHasFace, fuIsTracking, fuGetFaceInfo, fuGetAIInfo, etc.)
- Human Tracking: 2 (fuHumanActionMatchDistance, fuHumanActionMatchLeftRightHandDistance)
- Hand Tracking: 7 (fuEnableHandDetetor, fuHandDetectorGetResultNumHands, etc.)
- Face Processor: 16 (fuEnableFaceProcessor, fuFaceProcessorGetNumResults, fuFaceProcessorGetResultHairMask, fuFaceProcessorSetMinFaceRatio, etc.)
- Human Processor: 22 (fuEnableHumanProcessor, fuHumanProcessorGetNumResults, fuHumanProcessorGetResultRect, Joint2ds, Joint3ds, HumanMask, etc.)
- Beauty/ImageBeauty: 18 (fuBeautifyImage, fuImageBeautyCreateTexture, fuImageBeautySetParam, fuImageBeautyPreProcess, etc.)
- Camera: 31 (fuGetCameraImageSize, fuSetDefaultRotationMode, fuSetDeviceOrientation, fuOnCameraChange, fuSetCropState, fuEnableCameraAnimation, etc.)
- Avatar/Bind: 4 (fuBindItems, fuUnbindItems, fuAvatarBindItems, fuAvatarUnbindItems)
- Animation: 3 (fuEnableBackgroundAnimationLoop, fuResetBackgroundAnimation, fuResetLightAnimation)
- Physics: 1 (fuClearPhysics)
- Profile/Debug: 20 (fuCheckDebugItem, fuCheckGLError, fuProfileGetNumTimers, fuFrameTimeProfile*, etc.)
- AI Model: 8 (fuLoadAIModelFromPackage, fuPreprocessAIModelFromPackage, fuReleaseAIModel, fuIsAIModelLoaded, etc.)
- Other: 75 (fuEnableARMode, fuEnableBloom, fuEnableDof, fuEnableShadow, fuGetCurrentGLContext, fuHexagonInitWithPath, etc.)

**FUAI categories (597 exports):**
- Human Driver: 20+ (NewHumanDriverFromBundle, Process2Result, GetNumFromResult, SetMaxHumans, SetFov, etc.)
- Human Mocap Transfer: 10+ (NewHumanMocapTransferFromBundle, Process, GetResultTransformArray, SetAvatarToMocapNameMap, etc.)
- Human Mocap Collision: 5+
- Human Retargeter: 5+ (DeleteHumanRetargeter, GetTargetUELocalTRSFromResult, ProcessForUE2Result)
- BVH Retargeter: 10+ (InitBVHSource, InitTarget, Process, ProcessWithFootContact, GetGlobalTRS, GetLocalTRS)
- Face Processor: 30+ (DeleteFaceProcessor, GetAllLandmarksFromResult, GetArmeshVerticesFromResult, GetExpressionFromResult, GetDdeTriangles, DelSpotProcess, etc.)
- Face Beauty Processor: 50+ (DeleteFaceBeautyProcessor, Beauty, EvenSkin, FaceDelSpot, FaceWrinkleInpaint, GetBeautyImage, GetBlurMask, GetBrightEyeMask, GetSkinWhiteMask, GetDelSpotResult, SmartWrinkleRemoval, TeethSeg, etc.)
- Face Beauty Video Processor: 10+
- Background Segmenter: 5+ (DeleteBackgroundSegmenter, Inference, GetResultMask, Reset)
- Face Parsing: 10+ (DeleteFaceParsing, Process2Result, GetParsingMaskFromResult, GetCateMaskOrder, etc.)
- Face Attribute: 10+ (DeleteFaceAttributeProcessor, Process, GetAge, GetGender, GetSkinColor, SetNation)
- Hand Processor: 10+
- Human Processor: 10+
- Image View / File Buffer / Camera View: 10+
- Conversion: 10+ (ConvertGLToDde*, ConvertExpression47To51, CheckNeutralFace)
- Other: 100+ (DeleteFaceRecognizer, DeleteFaceMaskMapper, DeletePhotoEditingProcessor, DeleteISPBokehProcessor, DeleteHumanSkeleton, DeleteRetargetNameMapping, DeleteTfliteModel)

**Confidence:**
- HIGH for existence (export table parsing)
- MEDIUM for category (inferred from name)
- LOW for signature (only names, logs show `called({})` or `handle = {}` but not full signature, marked UNKNOWN if not sure)

**Evidence files:**
- `analysis/cnamasdk_exports_full.json` (582)
- `analysis/cnamasdk_fu_exports.json` (373)
- `analysis/fuai_exports_full.json` (597)
- `analysis/cnamasdk_fu_logs.json` (113 call logs with params like `handle = {}, item_name =`, `called({})`, `type:{} sz:{}`)
- `analysis/api_classification.json` (15 categories)

---

## Binary Findings

### CNamaSDK.dll (19.5 MB x64, 15.2 MB x32)

**Observed:**
- 582 exports, 373 fu*
- Imports: fuai.dll, OPENGL32.dll, KERNEL32.dll, USER32.dll, GDI32.dll, ADVAPI32.dll, WS2_32.dll
- Build path: `C:\CI12\builds\zx2dyZvQ\0\ruitaocai\CNamaSDK\src\core\CNamaSDK_FUAI.cc`, `src\modules\global\MakeupAndFacewarp.cpp`, `src\modules\makeup\lip_mask.cpp`, `makeup.cpp`, `makeupController.cpp`, `common\BundleHelper.cpp`, `core\ecs\SpriteClip.h`, `GLLoader.cc`
- Strings: 10,386 relevant filtered, 275 makeup params, 40+ tex_*, 13 blend_type_*, beauty params HeavyBlur etc., FaceMeshV2, BMesh, DecryptObfuscatedPackage, VerifySignature, AES-128-CBC, mbedtls, ENCRYPTED PRIVATE KEY, GLProgramNew, GLTechnique, g_makeup_vbo, MakeupFilterPassNAMA, MakeupWarpNAMA, etc.
- Logs: face_makeup, body_beautify, new_greensegment_fucreator_1.0.5_release, hair_normal, dummy, load texture failed 16, SetParamTex 6, JS liufei, GL 4.6

**Purpose:** FaceUnity NamaSDK — core rendering, bundle decryption, makeup/beauty pipeline, GL context, item management, params, rendering, face/human/hand tracking, beauty, camera, avatar, animation, AI model, profile/debug

**Confidence:** HIGH

**Status:** OBSERVED for exports/imports/strings/logs, INFERRED for purpose, PROTECTED for proprietary implementation

---

### FUAI.dll (29 MB x64, 18 MB x32)

**Observed:**
- 597 exports, all FUAI_*
- Imports: WS2_32.dll, KERNEL32.dll, ADVAPI32.dll, dbghelp.dll
- Build path: `D:\GitLab-Runner\builds\E3YA_xXv\0\chiliangyang\fuai\fuai\face\face_meshV2\face_meshV2_interface.cc`, `face_meshV2.cc`, `human\human_motion\motion_controller.cc`
- Strings: 710 AI-related, FaceMeshV2, ProcessFacemesh, mesh model timers, BMesh::triangulate_face, bmesh error, HaveSameShapes, ConvertGLToDde, HumanDriver, HumanMocapTransfer, HumanMocapCollision, HumanRetargeter, BVHRetargeter, FaceBeautyProcessor, BackgroundSegmenter, FaceParsing, FaceAttribute, HandProcessor, TFLite quantization, reference_ops::AveragePool, use_motion_controller, avatar_to_mocap_map, SetAvatarAnimFilterParams

**Purpose:** FaceUnity AI — inference engine for face/human/hand detection, landmarks, mesh V2, DDE, expression, eyes rotation, Disney face, occlusion, hair/head mask, del spot, tongue, beauty (del spot, wrinkle, even skin, bright eye, teeth, skin white mask, blur mask, etc.), background segmentation, face parsing, attribute (age, gender, skin color), hand, human (pose, human mask, action, BVH, human state, gesture, track ID, FOV, global RTS), mocap transfer/collision, retargeter, BVH retargeting, TFLite quantized models, FaceMeshV2, BMesh triangulation, motion controller, avatar mapping

**Confidence:** HIGH

**Status:** OBSERVED for exports/strings, INFERRED for purpose, PROTECTED for model weights

---

### OBS Bridge (obs-cam-beauty.dll 63 KB)

**Observed:**
- 9 exports: obs_module_load, unload, name, description, ver, set_pointer, set_locale, get_string, free_locale — standard OBS plugin API
- Imports: obs.dll, KERNEL32, MSVCP140, VCRUNTIME140
- Build PDB: `D:\work\obsplus\client\obs-studio\build\x64\plugins\obsplus\obs-cam-beauty\RelWithDebInfo\obs-cam-beauty.pdb` — OBS Studio 29.0
- Size small, only bridge, no fu* or makeup strings directly (via dependency)

**Purpose:** OBS plugin bridge — OBS loads via obs_module_load, loads CNamaSDK.dll and fuai.dll, creates items from bundles, sets params via tex_* and makeup_intensity_*, renders via fuRenderBundles, outputs texture to OBS, OBS composites via default.effect, shares via Spout

**Confidence:** HIGH

**Status:** OBSERVED, CONFIRMED DEPENDENCY: obs-cam-beauty.dll → obs.dll

---

### OBSPlus Framework (obsplus.dll 456 KB)

**Observed:**
- Imports: libcurl.dll, obs.dll, SHELL32, ole32, WS2_32, WINHTTP, MSVCP140, VCRUNTIME140
- JSON: allinfo-en-US.json 69 plugins marketplace metadata, including obs-cam-beauty with description "Professional beauty plugin for OBS with HD high-frame-rate effects, makeup, stickers, body shaping, background blur, multi-camera support, and more.", previewImages, guideUrl, official, author
- Proxy: gateway.json 2 proxies
- Store: core.dll, obs-plugins-store.dll, websocket.dll, store_qt5/6.dll, token.bin, notice/config.json

**Purpose:** OBSPlus framework hosting multiple plugins, marketplace, proxy, download, auth, token, notice

**Confidence:** HIGH

**Status:** OBSERVED, CONFIRMED

---

### Executables

**beauty.exe (1.4 MB x64, 1.3 MB x32):**
- Imports: CNamaSDK.dll, OPENGL32.dll, CRYPT32.dll, WS2_32.dll, bcrypt, VERSION, ole32, SHELL32, w32-pthreads, USER32, GDI32, ADVAPI32
- Build PDB: `.../obs-cam-beauty/nama/RelWithDebInfo/beauty.pdb`
- Strings: VERSION.dll, curl_easy_init, fuGetSystemError, beauty.exe, CNamaSDK.dll, XML manifest
- No --help strings — GUI app likely
- Hypothesized purpose: Standalone beauty test harness for fuImageBeauty (image-based beauty, not video) — from name, imports CNamaSDK+OPENGL32+libcurl, build path in nama, fuImageBeauty* APIs exist
- **Dynamic behavior: PROTECTED / NOT ANALYZED (no Wine, Windows PE, no execution beyond static)**

**camera-tool.exe (176 KB x64, 138 KB x32):**
- Imports: SetupDiEnumDeviceInfo, SetupDiDestroyDeviceInfoList, SetupDiGetDevicePropertyW, SetupDiGetDeviceRegistryPropertyW, QueryDosDeviceW, GetLastError, SetLastError
- Manifest: requireAdministrator
- Strings: SetupDi*, QueryDosDeviceW, requireAdministrator
- Locale INI: CheckWhereDeviceUsed.ButtonText="Check Usage", Info="Checking camera device usage...", DlgTitle="Camera Occupation Detection Tool", CheckError="Unable to detect camera status...", OpenDir="Locate File", NotUseed="No device occupation detected...", FindCameraSourceInOBS="The camera device is occupied by another source..."
- Purpose: Camera occupation detection tool — checks which app or OBS source is using camera device, when beauty camera fails to activate
- **Dynamic behavior: PROTECTED / NOT ANALYZED (no Wine, requires admin)**

**obs-plugin-console.exe:**
- Path store/64bit/, alongside core.dll, obs-plugins-store.dll
- Purpose: OBS Store console helper
- **Status: PROTECTED / NOT ANALYZED**

**Confidence:** MEDIUM for beauty.exe purpose, HIGH for camera-tool.exe purpose, LOW for dynamic behavior

**Evidence files:** `analysis/executable_observations.json`

---

## Configuration

### config.dat (5 files, 11-80 KB)

**Observed:**
- Files: beauty-hair 11,458 bytes, filters 16,014 bytes, custom_config 41,962 bytes, style_config 14,290 bytes, special-effects 80,023 bytes
- Magic: 1a1a or 3a43 first 2 bytes, not standard
- Entropy: 6.27-6.58 (1K) — less than bundle 7.8, more than text 4-5
- Printable ratio: 0.43-0.54, zero count 14-23 per 1K
- ASCII strings: short random 4-8 chars, no readable keys like "id", "name"
- Not JSON, not INI, not ZIP, not PNG, no UTF-16 BOM, no repetition, no offset table obvious
- Directory: each config.dat alongside bundles and/or thumbs in same category folder
- File sizes proportional to preset count: hair 24 items 11KB ~0.5KB per item, filters 100+ thumbs 16KB ~0.16KB per item, makeup custom 60+ bundles 41KB ~0.7KB per item, style 24 bundles 14KB ~0.6KB per item, special-effects 150+ bundles 80KB ~0.5KB per item

**Inferred:**
- Purpose: Encrypted/obfuscated index mapping bundle hash (32-char hex) to metadata (name, thumbnail, category, download/favorite state) — from co-location, size proportional, UI icons download.png/favorite.png, locale Shop messages
- Uses light obfuscation (XOR) not full AES, or compressed then obfuscated, since entropy 6.27-6.58 less than bundle 7.8
- Contains per-preset entry with id, name, thumbnail path, category

**Confidence:** MEDIUM for purpose, LOW for internal structure

**Status:** PROTECTED / NOT ANALYZED for internal structure (no decryption, no brute-force)

**Evidence file:** `analysis/config_dat_catalog.json` (5 entries with path, size, magic_hex, entropy, printable_ratio, zero_count, ascii_strings_sample, first_uints_le, first_64_hex, observed_format, inferred_purpose, confidence)

**Proposed HuanFace format (open, not reverse-engineered original):**
```json
{
  "version": "1.0",
  "category": "makeup",
  "presets": [
    {
      "id": "natural_lip",
      "name": "Natural Lip",
      "category": "lip",
      "thumbnail": "thumbs/natural_lip.png",
      "bundle": "bundles/natural_lip.hfbundle",
      "params": {"intensity": 0.8, "color": "#FF0000"},
      "downloaded": true,
      "favorite": false
    }
  ]
}
```

---

### INI (19 files)

**Observed:**
- Files: data/locale/en-US.ini etc. 5 languages + duplicates, 263 keys each, 140 beauty-related in en-US.ini, plus store locale 19 keys, win-spout locale 15-20 keys
- Keys: obs-cam-beauty, ParameterSetting, BaseBeauty, SkinDect, HeavyBlur, BalanceSmooth, FineSmooth, ClearSmooth, HazySmooth, BlurLevel, ColorLevel, ColorLevelType, DelspotLevel, RedLevel, Clarity, Sharpen, FaceThreed, EyeBright, ToothWhiten, RemovePouchStrength, RemoveNasolabialFoldsStrength, Brightness, Saturation, Filters, BeautyFilterLevel, Original, White, Pink, Fresh, CoolTone, WarmTone, FaceSize, FaceLandmarkQuality, BodyNum, BeautyProtection, BeautyRenderFPS, RotateClockwise, etc.
- Plus: NotFindBeautySource, WaitBeautyRenderReady, BeautyRenderInitializedError, CheckWhereDeviceUsed.ButtonText, etc.

**Purpose:** UI translations + parameter names — reveals supported beauty features

**Confidence:** HIGH

**Evidence file:** Parsed via `tools/metadata_inspector.py`, `analysis/preset_metadata.json` includes INI beauty params

---

### JSON (5 + analysis JSONs)

**Observed:**
- `AppData/Roaming/proxy/gateway.json`: 2 proxies `85vk:85vku@183.131.35.94:20252` and `obsproxy:nCzPVD4B@gateway.obshelp.com:10080`
- `AppData/Roaming/locale/allinfo-en-US.json`: 69 plugins, including obs-cam-beauty with briefDescription, description, keyWords, previewImages, guideUrl, official, author
- `AppData/Roaming/locale/ui-en-US.json`: UI strings for store
- `AppData/Roaming/notice/config.json`: `{"1":"2026-09-10 08:35:00","pluginRenew":"2026-09-26"}`
- `AppData/Roaming/notice/report.json`
- `analysis/*.json`: asset_catalog 965 entries, binary_catalog 315, file_type_statistics, dependency_graph, string_catalog 10,386, config_dat_catalog 5, preset_metadata 171, thumbnail_catalog 341, makeup_parameter_catalog 391, shader_catalog 227, executable_observations, api_classification, cnamasdk_exports, fuai_exports, etc.

**Purpose:** Proxy gateway, marketplace metadata, UI strings, notice, plus analysis catalogs

**Confidence:** HIGH

---

## Makeup Parameters

**Known: 391 unique params (from strings + INI + known lists)**

**Sources:**
- CNamaSDK.dll strings: 275 makeup params (makeup_intensity_*, tex_*, blend_type_*, makeup_*_color, is_makeup_on, etc.) — `analysis/makeup_params_from_strings.json`
- INI locale: 118 beauty params (HeavyBlur, ColorLevel, etc.)
- Known lists from Phase 0: 7+ (tex_brow, tex_eye, etc.)
- Logs: 6 SetParamTex (tex_lip_mask_zz, tex_blusher, tex_eyeLash, tex_eyeLiner, tex_eye, tex_brow) + 16 load texture failed (eyepupil.png, eyeliner.png, etc.)

**Categories:**
- lip: makeup_intensity_lip, makeup_lip_color, tex_lip, etc.
- eye: makeup_intensity_eye, makeup_eye_color, tex_eye, eye.png
- eyebrow: makeup_intensity_eyeBrow, makeup_eyeBrow_color, tex_brow, brow.png
- eyelash: makeup_intensity_eyelash, makeup_eyelash_color, tex_eyeLash, eyelash.png
- eyeliner: makeup_intensity_eyeLiner, makeup_eyeLiner_color, tex_eyeLiner, eyeliner.png
- blush: makeup_intensity_blusher, makeup_blusher_color, tex_blusher, blush
- foundation: makeup_intensity_foundation, makeup_foundation_color, tex_foundation, zhuangrong_fd
- pupil: makeup_intensity_pupil, makeup_pupil_color, tex_pupil, eyepupil.png
- highlight_shadow: makeup_intensity_highlight, makeup_intensity_shadow, tex_shadow, highlight
- texture: tex_* (40+)
- blend: blend_type_tex_* (13)
- beauty: HeavyBlur, ColorLevel, DelspotLevel, RedLevel, Clarity, Sharpen, FaceThreed, EyeBright, ToothWhiten, RemovePouchStrength, RemoveNasolabialFoldsStrength, Brightness, Saturation, FaceSize, BeautyProtection, etc. (140 from INI)
- other: is_makeup_on, is_clear_makeup, makeup_occlusion, makeup_lip_mask, etc.

**Types guessed from name:**
- color: makeup_*_color
- float: intensity, level, strength, brightness, saturation
- texture: tex_*, texture
- bool: is_*, enable
- enum: blend_type

**Range/Default:**
- NOT found, set null — we do NOT invent range/default

**Evidence per param:**
- e.g., makeup_intensity_lip: CNamaSDK.dll strings + known list + runtime log? Actually log has tex_* not intensity, but intensity from strings
- Full evidence list in `analysis/makeup_parameter_catalog.json` (391 entries with name, category, type, range null, default null, evidence list, confidence HIGH if >=2 evidence, MEDIUM if 1, LOW if 0)

**Confidence:**
- HIGH for existence (from strings)
- MEDIUM for category/type (from name)
- LOW for range/default (not found, null)

**Evidence file:** `analysis/makeup_parameter_catalog.json` (391 entries)

**Unknown:**
- Exact range (min/max) for each param
- Exact default value
- Exact meaning of zhuangrong_sh, sh2, fd, gg, yy, bz, zz, bite texture names (Chinese makeup terms, need translation but not essential)
- Exact mapping between texture file (eyepupil.png) and param (tex_pupil)

---

## Rendering

**Known:**

**OBS Effects (.effect) — 4 files, OBS Studio, NOT FaceUnity:**
- default.effect 9.3KB, 11 techniques: Draw, DrawAlphaDivide, DrawNonlinearAlpha, DrawNonlinearAlphaMultiply, DrawSrgbDecompress, DrawSrgbDecompressMultiply, DrawMultiply, DrawTonemap, DrawMultiplyTonemap, DrawPQ, DrawTonemapPQ
- Color space functions: srgb_linear_to_nonlinear, srgb_nonlinear_to_linear, rec709_to_rec2020, rec2020_to_rec709, d65p3_to_rec709, reinhard tonemap, linear_to_st2084 (PQ), st2084_to_linear, eetf_0_Lmax, maxRGB_eetf, linear_to_hlg, hlg_to_linear
- Purpose: OBS final compositing with color space handling (SDR, HDR, PQ, HLG, tonemap, alpha)
- Origin: OBS Studio, copyright Hugh Bailey, CONFIRMED, Confidence HIGH
- Relationship to beauty: LIKELY used by beauty plugin for final output, but no direct log linking, Confidence MEDIUM

**Shader Cache (.v2) — 223 files, OBS Studio D3D11 bytecode cache:**
- Few KB each, binary, magic random, no PK/JSON/HLSL
- Hash in filename is hash of shader source
- OBS caches compiled shaders for faster startup, .v2 version 2
- Some .v2 are cached versions of default.effect and format_conversion.effect, which would be used by beauty plugin, but also other OBS plugins, cannot determine which is beauty without OBS source mapping
- Origin: OBS Studio, CONFIRMED, Confidence HIGH
- Relationship to beauty: MEDIUM (some are beauty-related cached shaders, but not all)

**FaceUnity Shaders (Inside Encrypted Bundles) — PROTECTED:**
- Observed from strings in CNamaSDK.dll: MakeupFilterPassNAMA, MakeupWarpNAMA, MakeupPipeline2, MakeupPipeline, lip_makeup, face_makeup, eye_makeup, brow_makeup, LipMaskGetTexture2, makeup_lip_gloss_blur, highpass, final, lip_occu_mask, lip_occu_mask_blur_shader, dilation_tech, LIP_MASK_SIZE, lip_highlight_mask, lip_polygon_shader, lip_mask_preprocess_shader, lip_mask_blur_shader, DrawFaceMaskV2, MakeupDataInit, CheckRttAndRenderInput, g_makeup_vbo, g_makeup_ebo, timer_makeup_beautifybody, m_copytex_tech, u_lipColortexture, etc.
- Likely purpose:
  - MakeupFilterPassNAMA: base makeup filter pass
  - MakeupWarpNAMA: face warp for makeup alignment
  - MakeupPipeline2: full makeup pipeline composite
  - lip_makeup, eye_makeup, brow_makeup: specific categories
  - LipMaskGetTexture: lip mask generation
  - makeup_lip_gloss_blur/highpass/final: lip gloss effect
  - lip_occu_mask: lip occlusion handling
  - DrawFaceMaskV2: draw face mask
  - g_makeup_vbo/ebo: vertex buffer objects for makeup
- **Existence: HIGH from DLL strings, Purpose: MEDIUM from names, Implementation: PROTECTED (inside encrypted bundles, no bypass)**

**Rendering Pipeline Hypothesis (INFERRED, not observed decrypted):**
```
Input Image (camera)
  ↓
Face Detection (ai_face_processor)
  ↓
Landmarks, Face Mesh V2
  ↓
MakeupDataInit
  ↓
For each makeup category (lip, eye, brow, etc.):
  ├── Lip: LipMaskGetTexture (lip_polygon_shader, preprocess, blur, occu mask) + gloss (blur, highpass, final) + MakeupFilterPassNAMA
  ├── Eye: DrawFaceMaskV2 + MakeupFilterPassNAMA_NativeWithLeftAndRight
  ├── Eyebrow, Eyeliner, Eyelash, Blush, Foundation, etc.: mask + texture + blend
  └── Warp: MakeupWarpNAMA
  ↓
MakeupPipeline2 (composite all makeup)
  ↓
Beauty (face_beautification.bundle): skin smooth (HeavyBlur), whitening (ColorLevel), sharpen, clarity, face contour, eye bright, tooth whiten, etc.
  ↓
Body slim (body_slim.bundle)
  ↓
Background blur (background_blur.bundle)
  ↓
Output Image
```

**Confidence:** HIGH for OBS effects origin, MEDIUM for relationship to beauty, HIGH for FaceUnity shader existence, MEDIUM for purpose, PROTECTED for implementation

**Evidence files:**
- `analysis/shader_catalog.json` (227 entries: 4 .effect + 223 .v2 with path, size, type, origin, technique_count, color_spaces, magic, extractable, confidence)
- `docs/SHADER_ANALYSIS.md`

---

## Protected Information

**Original FaceUnity bundles:**
- 267 .bundle files, encrypted + signed, magic F3 5B 06 12 primary (70%) or variant (30%), entropy 7.7-7.85, strings DecryptObfuscatedPackage, VerifySignature, AES-128-CBC, mbedtls, CryptAcquireContextA
- Internal structure: NOT fully established, only inferred from DLL strings and logs (manifest JSON, JS controller, textures, shaders, meshes, params, AI models) — Confidence MEDIUM for existence, LOW for exact structure
- Content: NOT analyzed — would require decryption key or bypass of VerifySignature, which we do NOT do
- Status: PROTECTED / NOT ANALYZED for internal content

**Config.dat:**
- 5 files, 11-80KB, entropy 6.27-6.58, magic 1a1a/3a43, no readable JSON/INI
- Internal structure: NOT analyzed — would require decryption or bypass
- Status: PROTECTED / NOT ANALYZED for internal structure, but purpose inferred as index mapping bundle hash to metadata (MEDIUM for purpose, LOW for structure)

**FaceUnity shaders:**
- Inside encrypted bundles: MakeupFilterPassNAMA, MakeupWarpNAMA, MakeupPipeline2, lip_mask, etc.
- Implementation: NOT analyzed — would require decrypted bundle
- Status: PROTECTED / NOT ANALYZED for implementation, but existence and likely purpose documented from DLL strings (HIGH for existence, MEDIUM for purpose)

**AI models inside bundles:**
- ai_face_processor_pc.bundle 23MB, ai_human_processor_pc.bundle 42MB, plus other models
- Weights/architecture: NOT analyzed — would require decryption
- Status: PROTECTED / NOT ANALYZED for weights, but purpose documented (HIGH)

**Executable dynamic behavior:**
- beauty.exe, camera-tool.exe, obs-plugin-console.exe — Windows PE, requires Windows/Wine, some require admin
- No Wine available in sandbox, no execution attempted beyond static strings/imports
- Status: PROTECTED / NOT ANALYZED for dynamic behavior, but purpose inferred from name, imports, locale (MEDIUM-HIGH)

**Token.bin:**
- AppData/Roaming/token/token.bin — auth token for store
- Content: NOT analyzed — binary, possibly encrypted
- Status: PROTECTED / NOT ANALYZED

**Private keys, secret keys, license keys:**
- Strings show ENCRYPTED PRIVATE KEY, SOFTWARE\Microsoft\Cryptography, CryptAcquireContextA, CryptGenRandom, PKCS#1 encryption
- Keys: NOT searched, NOT extracted — would be bypass
- Status: PROTECTED / NOT ANALYZED

**Runtime memory dump:**
- No memory dump attempted
- Status: PROTECTED / NOT ANALYZED

---

## Clean-room Knowledge

### Observed (Facts directly from repo)

- 965 files, 655 MB, file types, directory structure, duplicate bin/common/data at root
- 267 bundles, magic F3 5B 06 12 primary 70% or variant 30%, entropy 7.7-7.85, sizes 4KB-42MB, locations
- 5 config.dat, 11-80KB, entropy 6.27-6.58, magic 1a1a/3a43, not JSON/INI/ZIP/PNG
- 582 exports CNamaSDK (373 fu*), 597 exports fuai (all FUAI_*), 9 exports obs-cam-beauty (obs_module_*)
- Imports: CNamaSDK→fuai.dll, OPENGL32, etc.
- 10,386 unique relevant strings, 275 makeup params, 40+ tex_*, 13 blend_type_*, 140 beauty keys from INI, FaceMeshV2, BMesh, DecryptObfuscatedPackage, VerifySignature, AES-128-CBC, GLProgramNew, MakeupFilterPassNAMA, etc.
- Logs: face_makeup, body_beautify, new_greensegment, hair_normal, dummy, load texture failed 16, SetParamTex 6, JS liufei, GL 4.6
- INI: en-US.ini 263 keys, 140 beauty-related
- JSON: allinfo-en-US.json 69 plugins, gateway.json 2 proxies
- PNG: 341 PNG, 200x200, alpha, categories hair 22, filter 100+, UI
- .effect: default.effect 9.3KB 11 techniques color space, copyright Hugh Bailey, OBS Studio; format_conversion.effect 13KB
- .v2: 223 files, D3D11 cache, few KB
- Caffe: MobileNetSSD 22MB
- AI bundles: ai_face_processor 23MB, ai_human_processor 42MB

### Inferred (Conclusions based on evidence)

- Bundles ARE encrypted + signed — from magic, entropy, DecryptObfuscatedPackage, VerifySignature, AES strings — HIGH
- Variant magics = different bundle types or versions — from is_controller_resource_bundle, different locations — MEDIUM/LOW
- Bundle internal likely contains manifest JSON, JS controller, textures, shaders, meshes, params, AI models — from logs and DLL strings — MEDIUM/LOW
- Config.dat is encrypted index mapping bundle hash to metadata — from co-location, size proportional, UI icons, locale Shop messages — MEDIUM for purpose, LOW for structure
- Textures inside bundles (eyepupil.png etc.) with tex_* binding points — from logs and DLL strings — HIGH
- JS inside bundles (liufei stbline, Group_1_13) — from logs — HIGH
- Beauty features from INI: HeavyBlur, ColorLevel, etc. — HIGH for list, MEDIUM for internal mapping
- OBS integration: OBS → obsplus → obs-cam-beauty → CNamaSDK → fuai → GPU → output → OBS composite → Spout — from imports, logs, files — HIGH/MEDIUM
- FUAI components: Face Processor, Beauty Processor, Background Segmenter, Face Parsing, Attribute, Hand, Human, Mocap, BVH, TFLite, FaceMeshV2, BMesh — from exports and strings — HIGH
- Shader origins: OBS effects are OBS, not FaceUnity; .v2 is OBS cache; FaceUnity shaders inside bundles PROTECTED — HIGH/MEDIUM
- Executable purpose: beauty.exe = image beauty test harness, camera-tool.exe = camera occupation detection — from name, imports, locale — MEDIUM/HIGH
- Thumbnail mapping via config.dat index — MEDIUM/LOW

### Unknown (Not yet known)

- Exact bundle internal structure (manifest schema, JS API, texture list format, shader code, mesh format, param defaults, dependencies)
- Exact config.dat internal structure (per-preset entry fields, version, timestamp, language, download/favorite encoding)
- Exact thumbnail→bundle mapping
- Exact shader implementation for FaceUnity
- Exact face tracking model architecture (landmark count, mesh vertex count)
- Exact beauty algorithm (bilateral filter params, whitening curve, face shape warp)
- Exact API signatures (params, return) for most fu* and FUAI_* — only names and some logs
- Exact rendering pipeline order
- Exact dependency between face_makeup.bundle and custom/style bundles
- Exact purpose of dummy item
- Exact meaning of zhuangrong_sh, sh2, fd, gg, yy, bz, zz, bite texture names
- Exact purpose of beauty.exe CLI/GUI behavior
- Exact behavior of camera-tool.exe
- Exact content of token.bin
- Exact proxy usage
- Exact relationship between MobileNetSSD and AI processor bundles
- Exact count of special-effects presets

### Protected (Not analyzed, requires bypass)

- Original FaceUnity bundles internal content (encrypted + signed, would require key or bypass VerifySignature)
- Config.dat internal structure (encrypted)
- FaceUnity shaders implementation (inside encrypted bundles)
- AI model weights inside bundles
- Executable dynamic behavior (requires Windows/Wine, admin)
- Token.bin content
- Private keys, secret keys, license keys
- Runtime memory dump

### Proposed (New design for HuanFace SDK, clean-room, NOT copying original)

**HuanFace Bundle Format (ZIP-based open):**
```
my_makeup.hfbundle (ZIP)
├── manifest.json (name, version, type, description, author, params, textures, shaders, dependencies)
├── textures/ (PNG, RGBA)
├── shaders/ (GLSL vertex/fragment, clean-room)
├── meshes/ (JSON/OBJ, optional)
└── controller.js (optional, JS state machine, clean-room)
```
This is PROPOSED OPEN FORMAT, not reverse-engineered original. Original is PROTECTED.

**HuanFace Preset Index (JSON, open):**
```json
{
  "version": "1.0",
  "category": "makeup",
  "presets": [
    {
      "id": "natural_lip",
      "name": "Natural Lip",
      "category": "lip",
      "thumbnail": "thumbs/natural_lip.png",
      "bundle": "bundles/natural_lip.hfbundle",
      "params": {"intensity": 0.8, "color": "#FF0000"},
      "downloaded": true,
      "favorite": false
    }
  ]
}
```
This is PROPOSED OPEN FORMAT, not reverse-engineered config.dat. Original config.dat is PROTECTED.

**HuanFace Shaders (GLSL, clean-room):**
- makeup_vertex.glsl, makeup_fragment.glsl with blendNormal, blendMultiply, mask, color, intensity, opacity, blendMode
- lip_mask, eye_mask, etc.
- skin_smooth (bilateral filter), whitening (color adjustment), face_warp
- This is PROPOSED OPEN FORMAT, not reverse-engineered FaceUnity shader. Original is PROTECTED.

**HuanFace Face Tracking (open models):**
- MediaPipe or ONNX Runtime + custom models, abstraction IFaceTracker with MediaPipeFaceTracker, ONNXFaceTracker, DlibFaceTracker, data structures FaceData, TrackingData
- This is PROPOSED, not copying FUAI models (PROTECTED)

**HuanFace Beauty Engine (clean-room):**
- Skin smooth bilateral filter, whitening color adjustment, sharpen, clarity, rosiness, blemish removal, face contour, eye bright, tooth whiten, etc., face shape mesh warp
- Proposed algorithms, not copying FaceUnity

**HuanFace API (C ABI, clean-room, compatible in spirit but not copying proprietary implementation):**
```c
HF_Init();
HF_CreateEngine();
HF_LoadBundle("preset.hfbundle");
HF_SetParamFloat("intensity", 0.8f);
HF_SetParamColor("lip_color", color);
HF_SetTexture("tex_lip", texId);
HF_ProcessFrame(inputTex, outputTex, width, height);
HF_Destroy();
```
This is PROPOSED, not copying fu* signatures exactly, but similar in spirit.

---

## Confidence

**HIGH: ~800+ items**
- Repository structure (965 files, directory layout, duplicates)
- Binary exports (582 CNamaSDK, 597 FUAI, 9 OBS)
- Binary imports (CNamaSDK→fuai, OPENGL32, etc.)
- Bundle magic F3 5B 06 12 primary, entropy 7.7-7.85, encrypted existence
- Bundle types core (face_beautification 5.2MB, face_makeup 702KB, body_slim 16KB, background_blur 125KB, ai_face_processor 23MB, ai_human_processor 42MB)
- String existence (10,386 relevant, 275 makeup params, 40+ tex_*, beauty params from INI 140)
- Logs (face_makeup, body_beautify, hair_normal, dummy, load texture failed 16, SetParamTex 6, JS liufei, GL 4.6)
- INI beauty feature list (HeavyBlur, ColorLevel, etc.)
- JSON marketplace (69 plugins)
- PNG thumbs existence and categories (hair 22, filter 100+, UI)
- OBS effects origin (default.effect 9.3KB 11 techniques, copyright Hugh Bailey, OBS Studio)
- Shader cache origin (223 .v2, OBS D3D11 cache)
- FUAI components existence (Face Processor, Beauty Processor, Background Segmenter, etc. from 597 exports)
- Executable purpose for camera-tool.exe (HIGH from SetupDi* and locale)
- Dependency CONFIRMED edges (OBS→obsplus, obs-cam-beauty→obs.dll, CNamaSDK→fuai.dll, GPU OpenGL 4.6, Spout)

**MEDIUM: ~200+ items**
- Bundle variant magics = different types/versions (existence MEDIUM, reason LOW)
- Bundle internal structure inferred (manifest, JS, textures, shaders, meshes, params, AI models) — existence MEDIUM, exact structure LOW
- Config.dat purpose as index mapping bundle hash to metadata — MEDIUM for purpose, LOW for structure
- Textures inside bundles with tex_* binding — HIGH for existence, MEDIUM for mapping
- JS inside bundles purpose (stbline = stroke line) — HIGH for existence, MEDIUM for purpose
- Beauty internal param mapping from INI to DLL — MEDIUM
- OBS integration overall flow — MEDIUM for flow, HIGH for individual CONFIRMED dependencies
- FUAI purpose for each component — HIGH for existence, MEDIUM for input/output details
- Shader cache relationship to beauty — MEDIUM (some are beauty-related cached, but not all)
- Thumbnail mapping via config.dat — MEDIUM for existence, LOW for exact mapping
- Beauty.exe purpose — MEDIUM
- Makeup param categories/types — MEDIUM (from name), LOW for range/default
- Dependency LIKELY edges (default.effect final composite, config.dat indexing)

**LOW: ~100+ items**
- Exact bundle internal structure (manifest schema, JS API, texture list format, shader code, mesh format, param defaults, dependencies)
- Exact config.dat internal structure (per-preset fields, version, timestamp, language, download/favorite encoding)
- Exact thumbnail→bundle mapping
- Exact shader implementation for FaceUnity
- Exact face tracking model architecture
- Exact beauty algorithm
- Exact API signatures for most fu*/FUAI_*
- Exact rendering pipeline order
- Exact dependency between face_makeup.bundle and custom/style bundles
- Exact purpose of dummy item
- Exact meaning of zhuangrong_* texture names
- Exact purpose of beauty.exe CLI/GUI behavior
- Exact behavior of camera-tool.exe dynamic
- Exact content of token.bin
- Exact proxy usage
- Exact relationship MobileNetSSD vs AI processor bundles
- Exact count of special-effects presets
- Dependency POSSIBLE edges (MobileNetSSD fallback, Store manages download)

**PROTECTED: 273 + other**
- Original bundles internal content (267 files)
- Config.dat internal (5 files)
- FaceUnity shaders implementation
- AI model weights
- Executable dynamic behavior (beauty.exe, camera-tool.exe, obs-plugin-console.exe)
- Token.bin
- Private keys, secret keys, license keys
- Runtime memory

---

## Remaining Questions

1. **Exact bundle internal structure:** What is the JSON manifest schema? What is the JS controller API? What is the texture list format? What is the shader code? What is the mesh format (VBO/EBO vs OBJ)? What are param defaults and ranges? What are dependencies between bundles? — Requires decrypted bundle via official SDK with valid license (no bypass), or clean-room design for HuanFace.

2. **Exact config.dat internal structure:** What are per-preset entry fields? Is it JSON obfuscated with XOR? What is version, timestamp, language handling, download/favorite state encoding? — Requires decryption (PROTECTED) or observation of running plugin (with valid license, no bypass).

3. **Exact thumbnail→bundle mapping:** Which PNG thumb corresponds to which bundle hash? Is it via config.dat index? What is the mapping for makeup custom/style where no thumbs in repo (maybe remote)? — Requires config.dat decryption or running plugin observation.

4. **Exact shader implementation for FaceUnity:** What is the GLSL/HLSL code for MakeupFilterPassNAMA, MakeupWarpNAMA, MakeupPipeline2, lip_mask, etc.? — PROTECTED inside encrypted bundles, no bypass. For HuanFace, we need clean-room shaders.

5. **Exact face tracking model architecture:** What is the TFLite model architecture for face detection, landmark count (68 vs 106), mesh vertex count, expression count (47 vs 51), etc.? — PROTECTED inside ai_face_processor bundle, but we can use open models (MediaPipe) for HuanFace.

6. **Exact beauty algorithm:** What are the algorithms for skin smooth (bilateral filter params), whitening (color curve), sharpen, clarity, face shape warp, eye/nose/chin adjustments? — Inferred from INI names but not exact implementation, need clean-room design for HuanFace.

7. **Exact API signatures:** What are the full signatures (params, return) for 373 fu* and 597 FUAI_*? Only names and some logs with `{}` placeholders observed. Need official FaceUnity documentation or header files (if available legally) or clean-room design for HuanFace.

8. **Exact rendering pipeline order:** Does beauty happen before or after makeup? Logs show face_makeup and body_beautify both created, but order not clear. What is the order of body slim, background blur, hair, special-effects? — Requires running plugin observation or documentation.

9. **Exact dependency between face_makeup.bundle and custom/style makeup bundles:** Does face_makeup.bundle load custom bundles, or does obs-cam-beauty.dll load both and bind via fuBindItems? Logs show Group_1_13 but not clear.

10. **Exact meaning of zhuangrong_* texture names:** zhuangrong = makeup in Chinese, but sh, sh2, fd, gg, yy, bz, zz, bite — need Chinese translation and makeup domain knowledge. Could be: sh = eye shadow? fd = foundation? gg = contour? yy = eye shadow? bz = bite lip? zz = ? — Need translation but not essential for SDK.

11. **Exact purpose and behavior of beauty.exe and camera-tool.exe:** No --help strings, no Wine execution, requires admin for camera-tool. Need Windows environment to observe, but no bypass.

12. **Exact content of token.bin and proxy usage:** token.bin binary, possibly encrypted auth token for store. gateway.json 2 proxies for marketplace download. Need running store observation.

13. **Exact relationship between MobileNetSSD Caffe model and AI processor bundles:** MobileNetSSD 22MB exists in data/model/, but no log shows usage. Could be fallback when AI processor bundles fail to load, or legacy. Need documentation or code.

14. **Exact count and metadata for special-effects presets:** 150+ numbered folders each with bundle, but no thumbs in repo, maybe remote download. Need config.dat decryption or marketplace API.

15. **How to handle protected resources legally for HuanFace SDK:** Since original bundles are PROTECTED, HuanFace SDK must use open format (ZIP with manifest.json, PNG, GLSL) and provide conversion tool that, given decrypted resources via official FaceUnity SDK with valid license, can pack into HuanFace format — legal as long as user owns assets and complies with FaceUnity license. No bypass.

---

## Recommended Phase 2

**Phase 2 — Bundle Format Analysis (with clean-room distinction)**

**Goal:** Understand how preset makeup/beauty bundle is structured (metadata, texture, mask, mesh, shader, param, animation, dependencies) based on OBSERVED and INFERRED, and design PROPOSED HuanFace open format.

**Tasks:**

- [ ] Based on Phase 1, propose HuanFace bundle format (ZIP-based first, then custom binary) — clearly marked as PROPOSED OPEN FORMAT, not reverse-engineered original
- [ ] Design manifest.json schema for HuanFace bundles with name, version, type, author, params (intensity, color, opacity, blend mode), textures (tex_*), shaders (vertex/fragment GLSL), meshes, dependencies
- [ ] Document bundle structure with OBSERVED vs INFERRED vs PROPOSED:
  ```
  Original FaceUnity bundle:
    Observed: encrypted/signed binary package, magic F3 5B 06 12 primary or variant, entropy 7.7-7.85, size 4KB-42MB
    Inferred: likely contains manifest JSON, JS controller, textures PNG, shaders GLSL/HLSL, meshes VBO/EBO, params, AI models TFLite
    Protected: internal content NOT analyzed (no bypass)
  
  HuanFace bundle:
    Proposed: ZIP-based open format with manifest.json, textures PNG, shaders GLSL clean-room, meshes JSON/OBJ, optional JS
  ```
- [ ] Build tools (Python) for HuanFace format (not for decrypting original):
  - `tools/huanface_bundle_packer.py` — pack directory into .hfbundle (ZIP)
  - `tools/huanface_bundle_unpacker.py` — unpack .hfbundle
  - `tools/bundle_inspector.py` — already exists, update to support HuanFace format (ZIP) as well as FaceUnity encrypted (header only)
- [ ] Create example bundles (e.g., simple lip color) for testing — using open textures and clean-room shaders
- [ ] Document in `docs/BUNDLE_FORMAT.md` (update with final HuanFace format, with clear OBSERVED/INFERRED/PROPOSED/PROTECTED sections) — already updated in Phase 1 with disclaimer, but need final version in Phase 2
- [ ] Create `examples/bundles/` with example HuanFace bundles

**Outputs:**
- `docs/BUNDLE_FORMAT.md` (final, with OBSERVED/INFERRED/PROPOSED/PROTECTED)
- `tools/bundle_inspector.py` (updated to support HuanFace ZIP)
- `tools/huanface_bundle_packer.py`
- `tools/huanface_bundle_unpacker.py`
- `examples/bundles/` (example HuanFace bundles)

**Exit Criteria:**
- HuanFace bundle format documented and agreed, clearly marked as PROPOSED OPEN FORMAT
- Original FaceUnity bundle documented as PROTECTED / partially understood (observed header only, internal PROTECTED)
- Packer/unpacker works for HuanFace format (ZIP)
- Example bundles can be loaded by future runtime (Phase 7)
- No DRM/encryption bypass performed

**Estimated Effort:** 1 week

**Legal:** No decryption of original bundles, no bypass, clean-room design for HuanFace format, conversion tool only for legally obtained decrypted resources via official SDK with valid license

---

**End of Phase 1 Summary**
