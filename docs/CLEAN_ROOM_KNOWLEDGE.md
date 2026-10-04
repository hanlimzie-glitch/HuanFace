# Clean-Room Knowledge Model — HuanFace

**Purpose:** Separate knowledge into OBSERVED, INFERRED, UNKNOWN, PROTECTED, PROPOSED to avoid mixing fact, hypothesis, and design decision.

**Date:** 2026-09-28 (Phase 1)

---

## OBSERVED — Facts directly from repository

**Repository Structure:**
- 965 files, ~655 MB, excluding .git
- File types: .png 341, .bundle 267, .v2 223, .dll 37, .log 31, .ini 19, .exe 11, .svg 8, .dat 6, .json 5, .effect 4, .caffemodel 2, .prototxt 2, .bin 1, .data 3, .meta 2, .gif 2
- Directory structure: `AppData/Roaming/plugins/obs-cam-beauty/` main plugin, `ProgramData/obsplus/beauty/` presets, `ProgramData/obs-studio/` OBS host + shader-cache, `bin/`, `common/`, `data/` duplicates at root
- `bin/64bit/CNamaSDK.dll` 19.5 MB, `bin/64bit/fuai.dll` 29 MB, `bin/64bit/obs-cam-beauty.dll` 63 KB, `obsplus.dll` 456 KB, `Spout.dll`, `store` DLLs, `beauty.exe` 1.4 MB, `camera-tool.exe` 176 KB
- `common/assets/graphics/` has `face_beautification.bundle` 5.2 MB, `face_makeup.bundle` 702 KB, `body_slim.bundle` 16.5 KB
- `common/assets/items/BackgroundSegmentation/background_blur.bundle` 125 KB
- `common/assets/model/` has `ai_face_processor_pc.bundle` 23 MB, `ai_human_processor_pc.bundle` 42 MB
- `ProgramData/obsplus/beauty/makeup/8.10.0/custom/models/` 60+ bundles with 32-char hex names
- `ProgramData/obsplus/beauty/makeup/8.10.0/style/{1..28}/*.bundle` 24 bundles
- `ProgramData/obsplus/beauty/beauty-hair/models/` 2 bundles + `thumbs/` 22 PNG
- `ProgramData/obsplus/beauty/filters/thumbs/` 100+ PNG with hex names
- `ProgramData/obsplus/beauty/special-effects/{id}/*.bundle` 150+ bundles
- `data/` has `default.effect` 9.3 KB, `format_conversion.effect` 13 KB, `model/MobileNetSSD_deploy.caffemodel` 22 MB, `locale/en-US.ini` 263 keys, `themes/` SVG icons
- `ProgramData/obs-studio/shader-cache/*.v2` 223 files
- `AppData/Roaming/proxy/gateway.json` 2 proxies, `locale/allinfo-en-US.json` 69 plugins marketplace metadata, `token/token.bin`, `notice/config.json`
- `AppData/Roaming/logs/camera_beauty/main/*.log` 7 files, `nama/*.log` 7 files, `obsplus/host/*.log` 7 files, `store/*.log` 7 files

**Bundle Files (Observed):**
- 267 .bundle files
- Magic bytes: `F3 5B 06 12` in 70% (187 files), variant magics `7C D3 33 56`, `10 11 D2 D5`, `B2 AA 43 33`, `3D E6 15 6D`, `35 59 CD 0B`, etc. in 30% (80 files)
- Entropy: 7.7-7.85 for all bundles (first 1K)
- No `PK` (zip), no JSON, no PNG header in first 256 bytes
- Sizes: 4.2 KB smallest, 42 MB largest, core bundles 16KB-5.2MB-23MB-42MB
- `background_blur.bundle` magic `35 59 CD 0B` variant, not primary

**Config.dat Files (Observed):**
- 5 files: `beauty-hair/config.dat` 11,458 bytes, `filters/config.dat` 16,014 bytes, `makeup/8.10.0/custom_config.dat` 41,962 bytes, `style_config.dat` 14,290 bytes, `special-effects/config.dat` 80,023 bytes
- Magic: `1a1a` or `3a43` first 2 bytes, not standard
- Entropy: 6.27-6.58 (1K) — less than bundle 7.8, more than text 4-5
- Printable ratio: 0.43-0.54
- Zero count: 14-23 per 1K
- ASCII strings: short random 4-8 chars, no readable keys like "id", "name"
- Not JSON, not INI, not ZIP, not PNG

**Binary Exports (Observed via PE parsing):**
- `CNamaSDK.dll`: 582 total exports, 373 `fu*` — list in `analysis/cnamasdk_fu_exports.json` and `cnamasdk_exports_full.json`
  - Examples: `fuSetup`, `fuCreateItemFromPackage`, `fuDestroyItem`, `fuBindItems`, `fuItemSetParamd`, `fuRenderBundles`, `fuTrackFace`, `fuFaceProcessorGetNumResults`, `fuHumanProcessorGetNumResults`, `fuHandDetectorGetResultNumHands`, `fuImageBeautyCreateTexture`, etc.
- `fuai.dll`: 597 total exports, all `FUAI_*` — list in `analysis/fuai_exports_full.json`
  - Examples: `FUAI_NewHumanDriverFromBundle`, `FUAI_FaceProcessorGetAllLandmarksFromResult`, `FUAI_FaceBeautyProcessorBeauty`, `FUAI_BackgroundSegmenterInference`, `FUAI_FaceParsingProcess2Result`, `FUAI_FaceAttributeProcessorGetAge`, etc.
- `obs-cam-beauty.dll`: 9 exports, all `obs_module_*` — `obs_module_load`, `unload`, `name`, `description`, `ver`, etc.

**Binary Imports (Observed via PE parsing):**
- `CNamaSDK.dll` imports: `fuai.dll`, `OPENGL32.dll`, `KERNEL32.dll`, `USER32.dll`, `GDI32.dll`, `ADVAPI32.dll`, `WS2_32.dll`
- `fuai.dll` imports: `WS2_32.dll`, `KERNEL32.dll`, `ADVAPI32.dll`, `dbghelp.dll`
- `obs-cam-beauty.dll` imports: `obs.dll`, `KERNEL32.dll`, `MSVCP140.dll`, `VCRUNTIME140.dll`
- `obsplus.dll` imports: `libcurl.dll`, `obs.dll`, `SHELL32.dll`, `ole32.dll`, `WS2_32.dll`, `WINHTTP.dll`, `MSVCP140.dll`
- `beauty.exe` imports: `CRYPT32.dll`, `WS2_32.dll`, `bcrypt.dll`, `VERSION.dll`, `ole32.dll`, `SHELL32.dll`, `KERNEL32.dll`, `w32-pthreads.dll`, `USER32.dll`, `GDI32.dll`, `ADVAPI32.dll`, `OPENGL32.dll`, `CNamaSDK.dll`
- `camera-tool.exe` imports: `SetupDiEnumDeviceInfo`, `SetupDiDestroyDeviceInfoList`, `SetupDiGetDevicePropertyW`, `SetupDiGetDeviceRegistryPropertyW`, `QueryDosDeviceW`, manifest `requireAdministrator`

**Strings (Observed via static strings extraction, 10,386 unique relevant):**
- Makeup: `makeup_intensity`, `makeup_intensity_lip`, `makeup_intensity_pupil`, `makeup_intensity_eye`, `makeup_intensity_eyeLiner`, `makeup_intensity_eyelash`, `makeup_intensity_eyeBrow`, `makeup_intensity_blusher`, `makeup_intensity_foundation`, `makeup_intensity_highlight`, `makeup_intensity_shadow`, `makeup_lip_color`, `makeup_eye_color`, `is_makeup_on`, `makeup_occlusion`, `makeup_lip_mask`, etc. (275 unique makeup params from strings, see `analysis/makeup_params_from_strings.json`)
- Textures: `tex_brow`, `tex_eye`, `tex_eye2`, `tex_eye3`, `tex_eye4`, `tex_pupil`, `tex_eyeLash`, `tex_lip`, `tex_eyeLiner`, `tex_blusher`, `tex_blusher2`, `tex_foundation`, `tex_shadow`, `tex_lip_highlight`, `tex_lip_mask_bz`, `tex_lip_mask_zz`, `tex_lip_mask_bite_bz`, `tex_lip_mask_bite_zz`, `tex_lip_mask_highlight_bz`, `tex_lip_mask_highlight_zz`, `tex_makeup`, `tex_lut`, `tex_lut2`, `tex_lipstick_median`, `tex_blend_weight_and_level_map`, `tex_face_occu_blur`, etc. (40+)
- Blend: `blend_type_tex_eye`, `blend_type_tex_brown`, `blend_type_tex_eyeLash`, `blend_type_tex_eyeLiner`, `blend_type_tex_blusher`, `blend_type_tex_lip`, `blend_type_tex_pupil`, etc.
- Beauty: `HeavyBlur`, `ColorLevel`, `DelspotLevel`, `RedLevel`, `Clarity`, `Sharpen`, `FaceThreed`, `EyeBright`, `ToothWhiten`, `RemovePouchStrength`, `RemoveNasolabialFoldsStrength`, `Brightness`, `Saturation`, `FaceSize`, `BeautyProtection`, etc. (from INI 140 beauty-related keys, 263 total)
- Face: `face_meshV2`, `face_meshV2_point_smooth_h`, `use_face_meshV2`, `armesh_vertex_num`, `arMesh`, `FaceMeshV2Interface`, `FaceMeshV2`, `BMesh::triangulate_face`, `FUAI_FaceProcessorGetAllLandmarksFromResult`, `FUAI_FaceProcessorGetArmeshVerticesFromResult`, etc.
- Bundle: `is_controller_resource_bundle`, `bundles`, `CNamaSDK::BundleHelper::DecryptObfuscatedPackage`, `CNamaSDK::BundleHelper::VerifySignature`, `Decrypt bundle failed, error:{}`, `enter DecryptObfuscatedPackage size:{}`, `DecryptObfuscatedPackage Failed!`, `is_editor_debug_bundle`, `DoRender bundle name = {}`, `Background segmentation bundle is corrupted.`, `Hair segmentation bundle is corrupted.`, `Face Processor bundle is corrupted.`, `FUAI_NewFaceProcessorFromBundleWithConfig`, `Human Processor bundle is corrupted.`, `Controller::FAvatarSystem::GetBundleBodyPartType`, `bundle_resource_name`, `bundle_id`, etc.
- Shader: `g_makeup_vbo`, `g_makeup_ebo`, `MakeupFilterPassNAMA`, `MakeupWarpNAMA`, `MakeupPipeline2`, `lip_makeup`, `face_makeup`, `eye_makeup`, `brow_makeup`, `LipMaskGetTexture2`, `makeup_lip_gloss_blur`, `makeup_lip_gloss_highpass`, `makeup_lip_gloss_final`, `lip_occu_mask_vbo`, `lip_occu_mask_dilation_tech`, `LIP_MASK_SIZE`, `lip_highlight_mask`, `lip_polygon_shader`, `GLProgramNew`, `GLTechnique`, `GLTechniqueBase`, `Material`, `CreateProgram`, `CreateBinaryProgram`, `DeclareUniform`, `SetFloat`, `Draw`, `DrawScreenQuad`, etc.
- AI: `use_mesh_deform`, `use_face_meshV2`, `ProcessFacemesh`, `HumanProcessor`, `HandDetector`, `TFLite`, `Quantization parameters has non-null scale but null zero_point`, `reference_ops::AveragePool`, `FaceMeshV2`, `BMesh`, `use_motion_controller`, `motion_controller.cc`, `avatar_to_mocap_map_file`, `SetAvatarAnimFilterParams`, etc.
- JS: `[js] liufei init stbline in`, `[js] liufei statesArray init,len: 1`, `[js] liufei show makeup: Group_1_13`, `NamaContext.cpp:98 [js]`, `SpriteClip`
- Encryption: `timer_Decrypt`, `Decrypt exception:{}`, `Decrypt and Verify error!`, `fu_mbedtls_ssl_decrypt_buf`, `fu_mbedtls_cipher_crypt`, `AES-128-CBC`, `AES-192-CBC`, `AES-256-CBC`, `AES-128-ECB`, `PKCS#1 encryption`, `SOFTWARE\Microsoft\Cryptography`, `CryptAcquireContextA`, `CryptGenRandom`, `ENCRYPTED PRIVATE KEY`
- OBS: `obs_module_load`, `obs_module_unload`, etc., `default.effect` techniques `Draw`, `DrawAlphaDivide`, `DrawNonlinearAlpha`, `DrawSrgbDecompress`, `DrawMultiply`, `DrawTonemap`, `DrawPQ`, etc., color space functions `srgb_linear_to_nonlinear`, `rec709_to_rec2020`, `linear_to_st2084`, `reinhard`, `linear_to_hlg`, etc.

**Logs (Observed):**
- `AppData/Roaming/logs/camera_beauty/nama/*.log` 7 files:
  - `created item name: face_makeup`, `body_beautify`, `new_greensegment_fucreator_1.0.5_release`, `hair_normal`, `dummy`
  - `fuCreateItemFromPackage: handle = 2, item_name =`
  - `load texture failed!:eyepupil.png`, `eyeliner.png`, `eyelash.png`, `brow.png`, `eye.png`, `eye2.png`, `eye3.png`, `eye4.png`, `lip_top2.png`, `lip_bz1.png`, `zhuangrong_sh.png`, `zhuangrong_sh2.png`, `zhuangrong_fd.png`, `zhuangrong_gg.png`, `zhuangrong_yy.png`, `gloss_lut.png` (16 unique)
  - `SetParamTex called tex_lip_mask_zz`, `tex_blusher`, `tex_eyeLash`, `tex_eyeLiner`, `tex_eye`, `tex_brow` (6 unique)
  - `[js] liufei init stbline in`, `statesArray init,len: 1`, `init stbline ok`, `RegiterStb in`, `show makeup: Group_1_13`
  - `GLLoader.cc:212 initialGLExtentions: glversion max = 4, min = 6`
- `main/*.log` less detailed
- `obsplus/host/*.log` plugin loading

**INI Locale (Observed):**
- `data/locale/en-US.ini` 263 keys, 140 beauty-related, parsed via `metadata_inspector.py`
- Keys: `obs-cam-beauty`, `ParameterSetting`, `BaseBeauty`, `SkinDect`, `HeavyBlur`, `BalanceSmooth`, `BlurLevel`, `ColorLevel`, `DelspotLevel`, `RedLevel`, `Clarity`, `Sharpen`, `FaceThreed`, `EyeBright`, `ToothWhiten`, `RemovePouchStrength`, `RemoveNasolabialFoldsStrength`, `Brightness`, `Saturation`, `Filters`, `FaceSize`, `FaceLandmarkQuality`, `BodyNum`, `BeautyProtection`, `BeautyRenderFPS`, `RotateClockwise`, etc.

**JSON Marketplace (Observed):**
- `AppData/Roaming/locale/allinfo-en-US.json` 69 plugins, including `obs-cam-beauty` with briefDescription "Professional beauty plugin for OBS with HD high-frame-rate effects, makeup, stickers, body shaping, background blur, multi-camera support, and more.", previewImages `https://resource.obsworks.com/plugininfo/beautyLogo.png`, etc.
- `proxy/gateway.json` 2 proxies: `85vk:85vku@183.131.35.94:20252` and `obsproxy:nCzPVD4B@gateway.obshelp.com:10080`
- `notice/config.json` `{"1":"2026-09-10 08:35:00","pluginRenew":"2026-09-26"}`

**PNG Thumbnails (Observed):**
- 341 PNG, dimensions via IHDR parsing, most 200x200 or similar, has_alpha true for many
- Categories by path: hair 22, filter 100+, makeup 0 (no thumbs in repo for makeup, maybe remote), special-effects 0 (no thumbs), UI 8+ (download, favorite)
- Hair: `hair_gradient_01.png`, `hair_normal_01.png` etc., descriptive
- Filters: `04BC983D74C989ADD8B0C1CB844953BF.png` hex names, 32-char, matches bundle hash pattern

**Shader Effects (Observed):**
- `data/default.effect` 9.3 KB text, 11 techniques, color space functions, copyright `Copyright (C) 2014 by Hugh Bailey <obs.jim@gmail.com>` — OBS Studio
- `data/format_conversion.effect` 13 KB, similar, YUV conversion
- `ProgramData/obs-studio/shader-cache/*.v2` 223 files, binary, few KB each, magic random, D3D11 bytecode cache (from OBS docs)

**AI Models (Observed):**
- `data/model/MobileNetSSD_deploy.caffemodel` 22 MB + `prototxt` — Caffe model, standard
- `ai_face_processor_pc.bundle` 23 MB, `ai_human_processor_pc.bundle` 42 MB — names suggest AI processors, but internal format PROTECTED

---

## INFERRED — Conclusions based on multiple evidence

**Bundle Encryption:**
- Bundles ARE encrypted + signed — from magic `F3 5B 06 12`, entropy 7.7-7.85, strings `DecryptObfuscatedPackage`, `VerifySignature`, `fu_mbedtls_cipher_crypt`, `AES-128-CBC`, `CryptAcquireContextA`
- Uses mbedtls for crypto, likely AES-CBC, with RSA signature
- **Confidence: HIGH**

**Bundle Types:**
- Primary magic `F3 5B 06 12` = obfuscated bundle v1, 70% of bundles
- Variant magics = different bundle types (controller resource vs model) or different encryption versions — from `is_controller_resource_bundle` string, different locations (custom makeup vs background_blur)
- **Confidence: MEDIUM for existence of multiple types, LOW for exact reason**

**Bundle Internal Structure (Inferred, not observed decrypted):**
- Contains: manifest JSON (item name, type, version), JavaScript controller (`liufei init stbline`, `Group_1_13`), textures (PNG, from `load texture failed` logs), shaders (GLSL/HLSL, from `MakeupFilterPassNAMA` strings), meshes (VBO/EBO, from `g_makeup_vbo`), parameters (`makeup_intensity_*`, `tex_*`, `blend_type_*`), AI models (TFLite, from fuai strings)
- **Confidence: MEDIUM for existence of these sections, LOW for exact structure**

**Config.dat Purpose:**
- Encrypted/obfuscated index mapping bundle hash (32-char hex) to metadata (name, thumbnail, category, download/favorite state) — from directory structure (each config.dat alongside bundles and/or thumbs), file sizes proportional to preset count (0.16-0.7KB per preset), UI icons `download.png`, `favorite.png`, locale Shop messages
- **Confidence: MEDIUM for purpose, LOW for internal structure**

**Makeup Textures Inside Bundles:**
- Textures like `eyepupil.png`, `eyeliner.png`, etc. ARE inside bundles (encrypted) — from logs `load texture failed` (SDK expects them but fails, suggesting they should be inside bundle or external), `tex_*` params from logs and DLL strings, no matching PNGs in repo except thumbs
- **Confidence: HIGH for existence inside bundles, HIGH for tex_* param names**

**JavaScript Inside Bundles:**
- Bundles contain JS controller logic — from logs `[js] liufei init stbline`, `[js] show makeup: Group_1_13`, `NamaContext.cpp:98 [js]`, `SpriteClip`, FaceUnity public docs mention JS
- **Confidence: HIGH for JS existence, MEDIUM for purpose (stbline = stroke line for eyeliner)**

**Beauty Features:**
- Supported beauty features from INI: skin smooth (HeavyBlur, BlurLevel), whitening (ColorLevel), blemish removal (DelspotLevel), rosiness (RedLevel), clarity, sharpen, face contour (FaceThreed), eye bright, tooth whiten, dark circle removal, nasolabial fold removal, brightness, saturation, filters (White, Pink, Fresh, CoolTone, WarmTone), face size, detect mode, max faces, beauty guard (mosaic), rendering FPS, orientation
- **Confidence: HIGH for feature list, MEDIUM for internal param mapping**

**OBS Integration:**
- `obs-cam-beauty.dll` is OBS plugin bridge (9 exports `obs_module_*`, imports `obs.dll`, size 63KB, build path OBS Studio 29.0)
- `obsplus.dll` is framework hosting multiple plugins, with marketplace (allinfo-en-US.json 69 plugins), proxy (gateway.json), store (core.dll, websocket.dll), token
- Flow: OBS → obsplus.dll → obs-cam-beauty.dll → CNamaSDK.dll → fuai.dll → GPU (OpenGL 4.6, D3D11, Spout) → output texture → OBS composite via default.effect → preview/program + Spout sharing
- **Confidence: HIGH for individual dependencies with logs/imports, MEDIUM for overall flow**

**FUAI Components:**
- Face Processor: face detection, landmarks, mesh V2, DDE, expression, eyes rotation, Disney face, occlusion, hair/head mask, del spot, tongue — from 30+ exports `FUAI_FaceProcessor*`, strings `face_meshV2`, `ProcessFacemesh`, `BMesh::triangulate_face`
- Face Beauty Processor: del spot, wrinkle, even skin, bright eye, teeth, skin white mask, blur mask, etc. — from 50+ exports `FUAI_FaceBeautyProcessor*`
- Background Segmenter: background segmentation (blur, green) — from `background_blur.bundle` 125KB, log `new_greensegment_fucreator_1.0.5_release`, strings `Background segmentation bundle is corrupted.`
- Face Parsing: skin, hair, eye, lip categories — from `FUAI_FaceParsing*` exports
- Face Attribute: age, gender, skin color — from `FUAI_FaceAttributeProcessor*`
- Hand Processor: hand detection, gesture — from `FUAI_HandProcessor*`, CNamaSDK `fuHandDetector*` logs
- Human Processor/Driver: human detection, pose, human mask, action, BVH, human state, gesture, track ID, FOV, global RTS — from `FUAI_HumanDriver*`, `FUAI_HumanProcessor*`, CNamaSDK `fuHumanProcessor*` logs
- Human Mocap Transfer/Collision: transfer human motion to avatar — from `FUAI_HumanMocapTransfer*`, `FUAI_HumanMocapCollision*`
- BVH Retargeter: BVH retargeting with foot contact — from `FUAI_BVHRetargeter*`
- Uses TFLite for inference — from quantization strings, `reference_ops::AveragePool`
- Uses FaceMeshV2 and BMesh for triangulation — from `face_meshV2`, `BMesh::triangulate_face`, `bmesh error`
- **Confidence: HIGH for all components (exports + strings)**

**Shader Origins:**
- `default.effect` and `format_conversion.effect` are OBS Studio (copyright Hugh Bailey, techniques Draw, color space functions) — NOT FaceUnity
- `.v2` shader cache is OBS D3D11 bytecode cache — some are cached beauty-related shaders, but also other OBS plugins, cannot determine which is beauty without OBS source mapping
- FaceUnity shaders inside encrypted bundles: `MakeupFilterPassNAMA`, `MakeupWarpNAMA`, `MakeupPipeline2`, `lip_mask`, etc. — from DLL strings, but implementation PROTECTED
- **Confidence: HIGH for OBS origin, MEDIUM for relationship to beauty, HIGH for FaceUnity shader existence, MEDIUM for purpose, PROTECTED for implementation**

**Executable Purpose:**
- `beauty.exe`: Standalone beauty test harness for `fuImageBeauty*` (image-based beauty) — from name, imports `CNamaSDK.dll` + `OPENGL32.dll` + `libcurl`, build path `.../obs-cam-beauty/nama/`, `fuImageBeauty*` APIs exist, no --help strings (GUI)
- `camera-tool.exe`: Camera occupation detection tool — from name, imports `SetupDi*`, `QueryDosDeviceW`, manifest `requireAdministrator`, locale `CheckWhereDeviceUsed.DlgTitle="Camera Occupation Detection Tool"`
- **Confidence: MEDIUM for beauty.exe purpose, HIGH for camera-tool.exe purpose, LOW for dynamic behavior (no Wine, no execution)**

**Thumbnail Mapping:**
- PNG thumbs are mapped to bundles via `config.dat` index (likely) — from co-location (hair: models + thumbs + config.dat, filters: thumbs + config.dat, makeup: bundles + custom_config.dat/style_config.dat, special-effects: bundles + config.dat), file sizes proportional to preset count
- **Confidence: MEDIUM for mapping via config.dat, LOW for exact mapping**

---

## UNKNOWN — Not yet known

- Exact bundle internal structure (JSON manifest schema, JS controller API, texture list format, shader code, mesh format, param defaults, dependencies)
- Exact config.dat internal structure (per-preset entry fields, version, timestamp, language handling, download/favorite state encoding)
- Exact mapping between thumbnail PNG and bundle hash (which thumb corresponds to which bundle)
- Exact shader implementation for FaceUnity (MakeupFilterPassNAMA, MakeupWarpNAMA, etc. code)
- Exact face tracking model architecture (TFLite model layers, landmark count 68 vs 106, mesh vertex count)
- Exact beauty algorithm (skin smooth bilateral filter params, whitening curve, face shape warp)
- Exact API signatures (params, return) for most fu* and FUAI_* — only names and some logs with `{}` placeholders
- Exact rendering pipeline order (does beauty happen before or after makeup? Logs show face_makeup and body_beautify both created, but order not clear)
- Exact dependency between face_makeup.bundle and custom/style makeup bundles (does face_makeup load custom bundles, or does obs-cam-beauty.dll load both and bind via fuBindItems?)
- Exact purpose of `dummy` item from logs
- Exact meaning of `zhuangrong_sh`, `sh2`, `fd`, `gg`, `yy`, `bz`, `zz`, `bite` texture names (Chinese makeup terms)
- Exact purpose of `beauty.exe` CLI/GUI behavior (no --help, no Wine execution)
- Exact behavior of `camera-tool.exe` (requires admin, no execution)
- Exact content of `token.bin`
- Exact proxy usage (gateway.json proxies for marketplace download?)
- Exact relationship between MobileNetSSD Caffe model and AI processor bundles (fallback or legacy?)
- Exact count of special-effects presets (150+ folders, but each folder has one bundle, but no thumbs in repo — maybe remote)

---

## PROTECTED — Not analyzed because requires bypass/protected access

**Original FaceUnity bundles:**
- 267 .bundle files — encrypted + signed, magic `F3 5B 06 12` or variant, entropy 7.7-7.85, strings `DecryptObfuscatedPackage`, `VerifySignature`, `AES-128-CBC`
- Internal structure: NOT fully established, only inferred from DLL strings and logs
- Content: NOT analyzed — would require decryption key or bypass of VerifySignature, which we do NOT do
- Status: PROTECTED / NOT ANALYZED for internal content

**Config.dat:**
- 5 files, 11-80KB, entropy 6.27-6.58, magic `1a1a` or `3a43`, no readable JSON/INI
- Internal structure: NOT analyzed — would require decryption or bypass
- Status: PROTECTED / NOT ANALYZED for internal structure

**FaceUnity shaders:**
- Inside encrypted bundles: `MakeupFilterPassNAMA`, `MakeupWarpNAMA`, `MakeupPipeline2`, `lip_mask`, etc.
- Implementation: NOT analyzed — would require decrypted bundle
- Status: PROTECTED / NOT ANALYZED for implementation, but existence and likely purpose documented from DLL strings (HIGH for existence, MEDIUM for purpose)

**AI models inside bundles:**
- `ai_face_processor_pc.bundle` 23MB, `ai_human_processor_pc.bundle` 42MB, plus other models
- Weights/architecture: NOT analyzed — would require decryption
- Status: PROTECTED / NOT ANALYZED for weights, but purpose documented (HIGH)

**Executable dynamic behavior:**
- `beauty.exe`, `camera-tool.exe`, `obs-plugin-console.exe` — Windows PE, requires Windows/Wine, some require admin
- No Wine available in sandbox, no execution attempted beyond static strings/imports
- Status: PROTECTED / NOT ANALYZED for dynamic behavior, but purpose inferred from name, imports, locale (MEDIUM-HIGH)

**Token.bin:**
- `AppData/Roaming/token/token.bin` — auth token for store
- Content: NOT analyzed — binary, possibly encrypted
- Status: PROTECTED / NOT ANALYZED

**Private keys, secret keys, license keys:**
- Strings show `ENCRYPTED PRIVATE KEY`, `SOFTWARE\Microsoft\Cryptography`, `CryptAcquireContextA`, `CryptGenRandom`, `PKCS#1 encryption`
- Keys: NOT searched, NOT extracted — would be bypass
- Status: PROTECTED / NOT ANALYZED

**Runtime memory dump:**
- No memory dump attempted
- Status: PROTECTED / NOT ANALYZED

---

## PROPOSED — New design for HuanFace SDK (clean-room, not copying original)

**HuanFace Bundle Format (ZIP-based open format):**

```
my_makeup.hfbundle (ZIP)
├── manifest.json
│   ├── name: "natural_lip"
│   ├── version: "1.0"
│   ├── type: "makeup/lip"
│   ├── description: "Natural lip"
│   ├── author: "HuanFace"
│   ├── params: {intensity: {type: float, default: 1.0, min: 0, max: 1}, color: {type: color, default: "#FF0000"}}
│   ├── textures: {tex_lip: "textures/lip.png", tex_lip_mask: "textures/lip_mask.png"}
│   ├── shaders: {vertex: "shaders/vertex.glsl", fragment: "shaders/fragment.glsl"}
│   └── dependencies: []
├── textures/
│   ├── lip.png (RGBA, with alpha)
│   └── lip_mask.png
├── shaders/
│   ├── vertex.glsl (GLSL)
│   └── fragment.glsl (GLSL)
├── meshes/
│   └── face_mask.json (optional, VBO/EBO or OBJ)
└── controller.js (optional, JS state machine)
```

**This is PROPOSED OPEN FORMAT, not reverse-engineered original. Original is PROTECTED.**

**HuanFace Preset Index (JSON, open):**

```json
{
  "version": "1.0",
  "category": "makeup",
  "presets": [
    {
      "id": "natural_lip",
      "name": "Natural Lip",
      "description": "Everyday natural lip color",
      "category": "lip",
      "thumbnail": "thumbs/natural_lip.png",
      "bundle": "bundles/natural_lip.hfbundle",
      "params": {"intensity": 0.8, "color": "#FF0000"},
      "downloaded": true,
      "favorite": false,
      "language": {"en-US": "Natural Lip", "zh-CN": "自然唇色"}
    }
  ]
}
```

**This is PROPOSED OPEN FORMAT, not reverse-engineered config.dat. Original config.dat is PROTECTED.**

**HuanFace Shaders (GLSL, clean-room):**

```glsl
// makeup_fragment.glsl (proposed, not copying FaceUnity)
uniform sampler2D u_inputTexture;
uniform sampler2D u_makeupTexture;
uniform sampler2D u_maskTexture;
uniform vec3 u_makeupColor;
uniform float u_intensity;
uniform float u_opacity;
uniform int u_blendMode;
varying vec2 v_uv;

void main() {
    vec4 base = texture2D(u_inputTexture, v_uv);
    vec4 makeup = texture2D(u_makeupTexture, v_uv);
    float mask = texture2D(u_maskTexture, v_uv).r;
    vec3 coloredMakeup = makeup.rgb * u_makeupColor;
    float alpha = makeup.a * mask * u_intensity * u_opacity;
    vec3 blended = mix(base.rgb, coloredMakeup, alpha);
    gl_FragColor = vec4(blended, base.a);
}
```

**This is PROPOSED OPEN FORMAT, not reverse-engineered FaceUnity shader. Original FaceUnity shaders are PROTECTED.**

**HuanFace Face Tracking (using open models):**

- Use MediaPipe (Apache 2.0) or ONNX Runtime (MIT) + custom models for face detection, landmarks, mesh
- Abstraction `IFaceTracker` with implementations `MediaPipeFaceTracker`, `ONNXFaceTracker`, `DlibFaceTracker`
- Data structures `FaceData`, `TrackingData` with bbox, landmarks, rotation, scale, translation, confidence, mesh vertices/triangles/UV
- **This is PROPOSED, not copying FUAI models (which are PROTECTED)**

**HuanFace Beauty Engine (clean-room):**

- Skin smooth: bilateral filter
- Whitening: color adjustment
- Sharpen, clarity, rosiness, blemish removal, etc.
- Face shape: mesh warp
- **Proposed algorithms, not copying FaceUnity**

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

**This is PROPOSED, not copying fu* signatures exactly, but similar in spirit for compatibility.**

---

## Summary Table

| Knowledge | Count / Size | Status | Confidence |
|-----------|--------------|--------|------------|
| OBSERVED files | 965 files, 655 MB | OBSERVED | HIGH |
| OBSERVED bundles | 267 files, magic F3 5B 06 12 primary, entropy 7.7-7.85 | OBSERVED (header only) | HIGH |
| OBSERVED config.dat | 5 files, 11-80KB, entropy 6.27-6.58, magic 1a1a/3a43 | OBSERVED (header only) | HIGH |
| OBSERVED exports | CNamaSDK 582 (373 fu*), FUAI 597 (all FUAI_*), obs-cam-beauty 9 obs_module_* | OBSERVED | HIGH |
| OBSERVED imports | CNamaSDK→fuai.dll, OPENGL32, etc. | OBSERVED | HIGH |
| OBSERVED strings | 10,386 unique relevant, 275 makeup params, 40+ tex_*, etc. | OBSERVED | HIGH |
| OBSERVED logs | face_makeup, body_beautify, hair_normal, dummy, load texture failed 16, SetParamTex 6, JS liufei, GL 4.6 | OBSERVED | HIGH |
| OBSERVED INI | en-US.ini 263 keys, 140 beauty-related | OBSERVED | HIGH |
| OBSERVED JSON | allinfo-en-US.json 69 plugins, gateway.json 2 proxies | OBSERVED | HIGH |
| OBSERVED PNG | 341 PNG, 200x200, alpha, categories hair/filter/UI | OBSERVED | HIGH |
| OBSERVED .effect | default.effect 9.3KB 11 techniques color space, format_conversion.effect 13KB, copyright Hugh Bailey | OBSERVED | HIGH |
| OBSERVED .v2 | 223 files, D3D11 cache, few KB | OBSERVED | HIGH |
| INFERRED bundle encryption | AES-CBC via mbedtls, signed, uses CryptAcquireContextA | INFERRED | HIGH |
| INFERRED bundle types | Primary vs variant magics, controller vs model, from is_controller_resource_bundle | INFERRED | MEDIUM/LOW |
| INFERRED bundle internal | JSON manifest, JS controller, textures, shaders, meshes, params, AI models | INFERRED | MEDIUM/LOW |
| INFERRED config.dat purpose | Index mapping bundle hash to metadata (name, thumbnail, category, download/favorite) | INFERRED | MEDIUM |
| INFERRED textures inside bundles | eyepupil.png etc. inside bundles, tex_* binding points | INFERRED | HIGH |
| INFERRED JS inside bundles | JS controller logic, liufei stbline, Group_1_13 | INFERRED | HIGH |
| INFERRED beauty features | HeavyBlur, ColorLevel, etc. from INI | INFERRED | HIGH |
| INFERRED OBS integration | OBS → obsplus → obs-cam-beauty → CNamaSDK → fuai → GPU → output → OBS composite → Spout | INFERRED | HIGH/MEDIUM |
| INFERRED FUAI components | Face Processor, Beauty Processor, Background Segmenter, Face Parsing, Attribute, Hand, Human, Mocap, BVH, TFLite, FaceMeshV2, BMesh | INFERRED | HIGH |
| INFERRED shader origins | OBS effects are OBS, not FaceUnity; .v2 is OBS cache; FaceUnity shaders inside bundles PROTECTED | INFERRED | HIGH/MEDIUM |
| INFERRED executable purpose | beauty.exe = image beauty test harness, camera-tool.exe = camera occupation detection | INFERRED | MEDIUM/HIGH |
| INFERRED thumbnail mapping | Via config.dat index | INFERRED | MEDIUM |
| UNKNOWN | Exact bundle internal structure, config.dat structure, thumbnail→bundle mapping, shader code, model weights, API signatures, rendering order, zhuangrong meaning, executable dynamic behavior, token.bin, proxy usage, MobileNetSSD relationship, special-effects count | UNKNOWN | - |
| PROTECTED | Original bundles internal content, config.dat internal, FaceUnity shaders implementation, AI model weights, executable dynamic behavior, token.bin, private keys, runtime memory | PROTECTED / NOT ANALYZED | - |
| PROPOSED | HuanFace bundle ZIP format, preset index JSON, shaders GLSL clean-room, face tracking MediaPipe/ONNX, beauty engine bilateral filter etc., API C ABI | PROPOSED OPEN FORMAT | - |

---

**End of Clean-Room Knowledge Model**
