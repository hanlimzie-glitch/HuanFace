# Reverse Engineering Log — HuanFace

This document follows the mandatory process:

```
Observe
↓
Hypothesis
↓
Experiment
↓
Evidence
↓
Conclusion
```

All entries must have confidence level: HIGH, MEDIUM, LOW, UNKNOWN.

---

## Entry 1: Bundle Magic F3 5B 06 12

**Observe:**  
- 70% of .bundle files start with bytes `F3 5B 06 12`
- Next bytes high entropy, no readable strings
- File sizes vary from 4KB to 42MB
- Entropy of first 1024 bytes = 7.7-7.85 (close to 8.0 = random)

**Hypothesis:**  
- `F3 5B 06 12` is magic for FaceUnity obfuscated bundle v1
- Entire file encrypted (AES) + signed
- Header contains IV, size, signature, but encrypted so not readable

**Experiment:**  
- Read first 64 bytes of 267 bundles via Python
- Calculate entropy
- Check for PK (zip) or JSON or PNG headers — none found
- Search CNamaSDK.dll strings for "DecryptObfuscatedPackage", "VerifySignature", "AES-128-CBC"

**Evidence:**  
- Magic `F3 5B 06 12` consistent across 187 bundles (70%)
- Entropy 7.7-7.85 for all bundles
- Strings in CNamaSDK.dll: `DecryptObfuscatedPackage`, `VerifySignature`, `fu_mbedtls_cipher_crypt`, `AES-128-CBC`
- Logs: `fuCreateItemFromPackage` creates items from bundles, so SDK decrypts them internally

**Conclusion:**  
- Bundles ARE encrypted with AES (likely CBC) via mbedtls, with signature verification
- Magic `F3 5B 06 12` is for obfuscated bundle
- **Confidence: HIGH**

**Next Validation:**  
- Try to find if CNamaSDK has key storage (without extracting key — just observe existence)
- Check if different magic variants correspond to different bundle types (controller vs model)

---

## Entry 2: Variant Magics (7C D3 33 56, 10 11 D2 D5, etc.)

**Observe:**  
- 30% of bundles have different first 4 bytes: `7C D3 33 56`, `10 11 D2 D5`, `B2 AA 43 33`, `3D E6 15 6D`, `35 59 CD 0B`, etc.
- These appear mostly in custom makeup bundles (`ProgramData/obsplus/beauty/makeup/8.10.0/custom/models/`)
- `background_blur.bundle` also has variant magic `35 59 CD 0B`
- Entropy still high (7.8)

**Hypothesis A:** Different bundle type (controller resource vs model) — string `is_controller_resource_bundle` suggests type differentiation  
**Hypothesis B:** Different encryption version (older bundles not re-encrypted)  
**Hypothesis C:** Different obfuscation level (free presets less protected but still encrypted)

**Experiment:**  
- List all variant magic bundles, check their paths and sizes
- Check if they load via same API `fuCreateItemFromPackage` (from logs, yes)
- Check CNamaSDK strings for bundle type handling

**Evidence:**  
- Variant magics appear in custom makeup (small sizes 20-100KB) and background_blur (125KB)
- Same API loads both primary and variant magics
- String `is_controller_resource_bundle` exists, suggesting SDK distinguishes bundle types internally
- No clear correlation between magic and size or location alone

**Conclusion:**  
- Variant magics ARE valid bundle types, handled by same SDK
- Most likely different bundle type or encryption version, but not proven which
- **Confidence: MEDIUM for existence of multiple types, LOW for exact reason**

**Next Validation:**  
- Compare file creation times if available (not in repo)
- Try to see if variant magic bundles have different internal structure via official SDK API (with valid license, no bypass)

---

## Entry 3: Config.dat Encryption

**Observe:**  
- 5 files named `config.dat` in `ProgramData/obsplus/beauty/*/`
- Sizes 11KB-80KB
- First bytes not JSON, not readable, e.g., `:C\u0017YU\u0016_...` or `\x1a\x1aWQXX...`
- High entropy, similar to bundles
- Located alongside bundles and PNG thumbs

**Hypothesis:**  
- `config.dat` is encrypted index mapping bundle hash to metadata (name, preview, params, category)
- Uses same encryption as bundles (AES via mbedtls)
- Contains list of available presets for UI

**Experiment:**  
- Read first 500 bytes of each config.dat, check entropy, check for readable strings
- Compare header patterns to bundle headers
- Search CNamaSDK.dll for "config.dat" handling

**Evidence:**  
- High entropy, not readable
- Located in same folders as bundles and thumbs, typical for store index
- No "config.dat" string found in CNamaSDK.dll (searched, not found) — maybe handled by obsplus.dll or store?
- But marketplace metadata JSON exists separately (`allinfo-en-US.json`), so config.dat might be for beauty presets only

**Conclusion:**  
- Config.dat IS encrypted/obfuscated index, likely for beauty presets
- Encryption method probably same as bundles, but not confirmed
- **Confidence: MEDIUM for purpose, LOW for encryption method**

**Next Validation:**  
- Check obsplus.dll strings for config.dat handling
- Observe running plugin behavior: does it read config.dat to list presets?

---

## Entry 4: Makeup Textures Inside Bundles

**Observe:**  
- Logs show `load texture failed!:eyepupil.png`, `eyeliner.png`, `eyelash.png`, `brow.png`, `eye.png`, etc.
- Logs show `SetParamTex called tex_lip_mask_zz`, `tex_blusher`, `tex_eyeLash`, `tex_eyeLiner`, `tex_eye`, `tex_brow`
- CNamaSDK.dll strings contain `tex_brow`, `tex_eye`, `tex_eyeLiner`, `tex_eyeLash`, `tex_blusher`, `tex_lip`, etc.

**Hypothesis:**  
- Bundles contain textures like `eyepupil.png`, `eyeliner.png`, etc. but SDK expects them as external files or as internal resources bound via `SetParamTex`
- `tex_*` params are the actual texture binding points for makeup engine
- Failed to load textures because bundles reference external files not included, or internal extraction failed

**Experiment:**  
- Search all logs for "load texture failed" — found 16 unique texture names
- Search CNamaSDK.dll for `tex_*` — found 40+ texture params
- Check if any PNG in repo matches those names — none, only thumbs and UI icons

**Evidence:**  
- 16 texture names from logs: `eyepupil.png`, `eyeliner.png`, `eyelash.png`, `brow.png`, `eye.png`, `eye2.png`, `eye3.png`, `eye4.png`, `lip_top2.png`, `lip_bz1.png`, `zhuangrong_sh.png`, `zhuangrong_sh2.png`, `zhuangrong_fd.png`, `zhuangrong_gg.png`, `zhuangrong_yy.png`, `gloss_lut.png`
- `tex_*` params: `tex_brow`, `tex_eye`, `tex_eye2`, `tex_eye3`, `tex_eye4`, `tex_pupil`, `tex_eyeLash`, `tex_lip`, `tex_eyeLiner`, `tex_blusher`, `tex_blusher2`, `tex_foundation`, `tex_shadow`, `tex_lip_highlight`, `tex_lip_mask_bz`, `tex_lip_mask_zz`, etc.
- No matching PNGs in repo — suggests textures ARE inside bundles (encrypted)

**Conclusion:**  
- Makeup bundles DO contain textures, but they are encrypted inside bundles
- `tex_*` params are the binding points for HuanFace SDK to implement
- **Confidence: HIGH for existence of textures inside bundles, HIGH for tex_* param names**

**Next Validation:**  
- If we can get decrypted bundle via official SDK with valid license, check if textures are PNG inside
- For HuanFace SDK, design our own texture system using `tex_*` naming convention for compatibility

---

## Entry 5: JavaScript Inside Bundles

**Observe:**  
- Logs show `[js] liufei init stbline in`, `[js] liufei statesArray init,len: 1`, `[js] liufei init stbline ok`, `[js] liufei RegiterStb in`, `[js] liufei show makeup: Group_1_13`
- CNamaSDK.dll contains `NamaContext.cpp:98 [js]` and `SpriteClip`

**Hypothesis:**  
- Bundles contain JavaScript controller logic for makeup application, state machine, animation
- `liufei` is developer name, `stbline` is stroke-based line rendering for eyeliner
- `Group_1_13` suggests makeup grouping (group 1, item 13)

**Experiment:**  
- Search CNamaSDK.dll for "js" and "javascript" — found JS context
- Search for "stbline" — not found outside logs, so likely inside bundle JS
- Check FaceUnity public docs: FaceUnity supports JS for controller logic

**Evidence:**  
- Logs prove JS execution inside bundles
- FaceUnity public documentation mentions JS controller for bundles
- `SpriteClip` suggests animation or sprite handling

**Conclusion:**  
- Bundles DO contain JavaScript controller logic
- **Confidence: HIGH for JS existence, MEDIUM for purpose (stbline = stroke line for eyeliner)**

**Next Validation:**  
- If decrypted bundle available via official SDK, check for .js files inside
- For HuanFace SDK, we can support optional JS controller or use JSON-based state machine

---

## Entry 6: Beauty Parameters from INI

**Observe:**  
- `data/locale/en-US.ini` contains 263 keys, many beauty-related
- Examples: `HeavyBlur`, `ColorLevel`, `DelspotLevel`, `RedLevel`, `Clarity`, `Sharpen`, `FaceThreed`, `EyeBright`, `ToothWhiten`, etc.

**Hypothesis:**  
- INI file reveals supported beauty features of FaceUnity SDK
- Each key corresponds to a beauty effect that can be controlled via `fuItemSetParamd`

**Experiment:**  
- Parse en-US.ini, extract all keys, filter beauty-related
- Cross-reference with CNamaSDK.dll strings for param names

**Evidence:**  
- 140 beauty-related keys in en-US.ini
- CNamaSDK.dll contains similar param names but not exact INI keys — INI is UI translation, not internal param
- But internal params likely similar: `HeavyBlur` -> `heavy_blur` or `skin_smooth`?

**Conclusion:**  
- INI reveals high-level beauty features supported
- Internal param names need to be derived from DLL strings and logs, not just INI
- **Confidence: HIGH for feature list, MEDIUM for internal param mapping**

**Next Validation:**  
- Build metadata_inspector to parse INI fully and map to known makeup/beauty params from DLL strings
- For HuanFace SDK, use INI feature list as requirement for beauty engine

---

## Entry 7: OBS Effects vs FaceUnity Shaders

**Observe:**  
- `data/default.effect` is 9.3KB text file with HLSL-like code, contains color space conversion (sRGB, Rec709, Rec2020, PQ, HLG, tonemap)
- `data/format_conversion.effect` similar
- CNamaSDK.dll contains strings like `MakeupFilterPassNAMA`, `MakeupWarpNAMA`, `lip_mask`, `g_makeup_vbo`

**Hypothesis:**  
- OBS .effect files are for OBS final compositing, NOT FaceUnity shaders
- FaceUnity shaders are inside encrypted bundles (GLSL/HLSL for makeup warp, lip mask, etc.)

**Experiment:**  
- Read default.effect — confirms it's OBS built-in effect with 11 techniques for drawing textures with various color space handling
- Search CNamaSDK.dll for shader names — found FaceUnity shader names
- Check if any .effect file contains makeup logic — none, only color space conversion

**Evidence:**  
- default.effect contains `srgb_linear_to_nonlinear`, `rec709_to_rec2020`, `linear_to_st2084`, `reinhard`, etc. — standard OBS color handling
- No makeup-specific logic in .effect files
- FaceUnity shader names in DLL: `MakeupFilterPassNAMA`, `MakeupWarpNAMA`, `lip_polygon_shader`, etc.

**Conclusion:**  
- OBS .effect files are NOT FaceUnity shaders, they are OBS's own
- FaceUnity shaders are inside bundles, encrypted
- **Confidence: HIGH**

**Next Validation:**  
- For HuanFace SDK, we need to write our own GLSL shaders for makeup rendering, not reuse OBS effects
- OBS effects can be used for final compositing if HuanFace SDK is used as OBS plugin

---

## Entry 8: AI Processor Bundles

**Observe:**  
- `ai_face_processor_pc.bundle` 23MB, `ai_human_processor_pc.bundle` 42MB
- Magic `F3 5B 06 12`, entropy 7.8
- CNamaSDK.dll contains `FUAI_NewFaceProcessorFromBundleWithConfig`, `FaceMeshV2`, `ProcessFacemesh`, `HumanProcessor`, `HandDetector`, etc.
- fuai.dll contains TFLite quantization strings, `FaceMeshV2Interface`, `BMesh::triangulate_face`

**Hypothesis:**  
- AI bundles contain CNN models for face/human detection, landmarks, mesh, segmentation
- Format is TFLite or custom CNN, encrypted inside bundle
- fuai.dll is inference engine that loads these models

**Experiment:**  
- Check file sizes: 23MB and 42MB reasonable for CNN models
- Search fuai.dll for model-related strings — found FaceMeshV2, BMesh, TFLite
- Check if any other AI model exists: MobileNetSSD Caffe model 22MB in `data/model/`

**Evidence:**  
- Sizes match typical face detection models
- Strings prove FaceMeshV2, human pose, hand detection
- MobileNetSSD exists as fallback, but AI bundles are primary

**Conclusion:**  
- AI bundles DO contain CNN models for face/human processing
- **Confidence: HIGH for purpose, MEDIUM for format (TFLite vs custom)**

**Next Validation:**  
- For HuanFace SDK, we can use MediaPipe or ONNX models for face detection as replacement, not need to decrypt FaceUnity models
- Design face tracking abstraction so engine can be swapped

---

## Summary of Confidence Levels

| Entry | Topic | Confidence | Reason |
|-------|-------|------------|--------|
| 1 | Bundle magic F3 5B 06 12 = encrypted | HIGH | Magic consistent, entropy high, Decrypt strings |
| 2 | Variant magics = different bundle types/versions | MEDIUM/LOW | Exists, but exact reason unknown |
| 3 | Config.dat = encrypted index | MEDIUM/LOW | High entropy, location, but no direct evidence |
| 4 | Textures inside bundles, tex_* params | HIGH | Logs + DLL strings |
| 5 | JS inside bundles | HIGH | Logs prove JS execution |
| 6 | Beauty params from INI | HIGH for features, MEDIUM for internal mapping | INI parsed, but internal param mapping needs DLL strings |
| 7 | OBS .effect vs FaceUnity shaders | HIGH | .effect is color space conversion, FaceUnity shaders inside bundles |
| 8 | AI bundles contain CNN models | HIGH for purpose, MEDIUM for format | Sizes + strings |

---

## Next Steps

- Build inspector tools (DONE)
- Document all fu* APIs into API_REFERENCE.md (TODO)
- Analyze config.dat more (TODO)
- Attempt to run beauty.exe with --help (TODO, no bypass)
- Design HuanFace bundle format (TODO, Phase 2)

---

**End of Reverse Engineering Log**

---

# Phase 1 Experiments (Additional)

## Experiment 9: CNamaSDK Export Table

### Question
What is the public API surface of CNamaSDK.dll? How many fu* functions exported?

### Observation
- File `bin/64bit/CNamaSDK.dll` 19.5 MB
- PE header shows 582 total exports, 373 fu*
- Previous strings analysis found 200+ fu* but export table gives exact count

### Hypothesis
- Export table contains all public APIs, including fu* and internal C++ classes (GLProgramNew, GLTechnique, Material, Timer)
- fu* are the public C API for FaceUnity, while C++ classes are internal

### Experiment
- Parse PE file manually: read e_lfanew, PE sig, COFF header, optional header, data directory for export table, section headers to map RVA to file offset, read IMAGE_EXPORT_DIRECTORY, read AddressOfNames array, read each name string
- Save to `analysis/cnamasdk_exports_full.json` and `cnamasdk_fu_exports.json`
- Classify into categories: Initialization, GL Context, Item/Bundle, Instance/Scene, Parameters, Rendering, Face Tracking, Human Tracking, Hand Tracking, Face Processor, Human Processor, Beauty/ImageBeauty, Camera, Avatar/Bind, Animation, Physics, Profile/Debug, AI Model, Other

### Evidence
- Total exports 582, fu* 373
- Categories: Initialization 21, GL Context 2, Item/Bundle 2, Instance/Scene 95, Parameters 23, Rendering 11, Face Tracking 12, Human Tracking 2, Hand Tracking 7, Face Processor 16, Human Processor 22, Beauty/ImageBeauty 18, Camera 31, Avatar/Bind 4, Animation 3, Physics 1, Profile/Debug 20, AI Model 8, Other 75
- Logs show usage for many: fuCreateItemFromPackage handle, fuDestroyItem, fuBindItems, fuItemSetParamd, fuRenderBundlesSplitView, fuTrackFace, fuFaceProcessorGetResultHairMask, fuHumanProcessorGetNumResults, etc.
- File `analysis/api_classification.json` contains classification

### Conclusion
- CNamaSDK API surface is 373 fu* functions, plus 209 C++ internal exports
- Categories cover full beauty camera pipeline: init, GL, bundle loading, params, rendering, face/human/hand tracking, beauty, camera, avatar, animation, AI model
- **Confidence: HIGH for existence, MEDIUM for category, LOW for signature (only names, not params/return)**

### Next Step
- Document each API in API_REFERENCE.md with observed signature if available from logs, otherwise UNKNOWN
- For HuanFace SDK, design similar but clean-room API with C ABI

---

## Experiment 10: FUAI Export Table

### Question
What is the AI pipeline in fuai.dll? What components exist?

### Observation
- File `bin/64bit/fuai.dll` 29 MB
- PE export table 597 exports, all FUAI_*
- Strings contain FaceMeshV2, HumanProcessor, HandDetector, TFLite, BMesh, motion_controller, avatar_to_mocap_map

### Hypothesis
- FUAI is FaceUnity AI inference engine, with components: Face Processor, Face Beauty Processor, Background Segmenter, Face Parsing, Face Attribute, Hand Processor, Human Processor/Driver, Human Mocap Transfer/Collision, Human Retargeter, BVH Retargeter, etc.
- Uses TFLite for inference, FaceMeshV2 and BMesh for mesh

### Experiment
- Parse PE export table for fuai.dll similarly
- Save to `analysis/fuai_exports_full.json` and `fuai_fu_exports.json`
- Classify by name: Human Driver, Human Mocap Transfer, Human Mocap Collision, Human Retargeter, BVH Retargeter, Face Processor, Face Beauty Processor, Face Beauty Video Processor, Background Segmenter, Face Parsing, Face Attribute, Hand Processor, etc.
- Extract strings for TFLite, FaceMeshV2, BMesh, motion_controller, avatar

### Evidence
- Total exports 597, all FUAI_*
- Categories: Human Driver 20+, Human Mocap Transfer 10+, Human Mocap Collision 5+, Human Retargeter 5+, BVH Retargeter 10+, Face Processor 30+, Face Beauty Processor 50+, Face Beauty Video Processor 10+, Background Segmenter 5+, Face Parsing 10+, Face Attribute 10+, Hand Processor 10+, etc.
- Strings: `face_meshV2`, `ProcessFacemesh`, `mesh model preprocess timer`, `BMesh::triangulate_face`, `use_motion_controller`, `avatar_to_mocap_map_file`, `Quantization parameters has non-null scale but null zero_point`, `reference_ops::AveragePool`
- Build paths: `face_meshV2_interface.cc`, `face_meshV2.cc`, `human_motion/motion_controller.cc`

### Conclusion
- FUAI contains full AI pipeline for face/human/hand, beauty, parsing, attribute, retargeting, BVH
- Uses TFLite quantized models, FaceMeshV2, BMesh triangulation, motion controller, avatar mapping
- **Confidence: HIGH for components, HIGH for TFLite usage, HIGH for FaceMeshV2/BMesh**

### Next Step
- Document in FUAI_ANALYSIS.md with Component/Purpose/Input/Output/Evidence/Confidence
- For HuanFace SDK, use open models (MediaPipe, ONNX) with abstraction IFaceTracker, so engine swappable

---

## Experiment 11: OBS-CAM-BEAUTY Bridge

### Question
What is the boundary between OBS and FaceUnity? How does obs-cam-beauty.dll integrate?

### Observation
- File `bin/64bit/obs-cam-beauty.dll` 63 KB, small
- Exports 9: all obs_module_* — standard OBS plugin API
- Imports obs.dll — OBS core
- beauty.exe imports CNamaSDK.dll and OPENGL32.dll
- CNamaSDK.dll imports fuai.dll and OPENGL32.dll
- Logs: face_makeup, body_beautify, background_blur, hair_normal, GL 4.6, SetParamTex, load texture failed, JS liufei
- Files: default.effect 9.3KB 11 techniques color space, format_conversion.effect 13KB, shader-cache 223 .v2, Spout DLLs, MobileNetSSD Caffe 22MB, config.dat 5 files

### Hypothesis
- obs-cam-beauty.dll is bridge: OBS loads it via obs_module_load, it loads CNamaSDK.dll and fuai.dll, creates items from bundles, sets params via tex_* and makeup_intensity_*, renders via fuRenderBundles, outputs texture to OBS, OBS composites via default.effect, shares via Spout
- OBSPlus framework (obsplus.dll 456KB) hosts multiple plugins, with marketplace (allinfo-en-US.json 69 plugins), proxy (gateway.json 2 proxies), store (core.dll, websocket.dll), token

### Experiment
- Parse PE exports for obs-cam-beauty.dll: 9 obs_module_*
- Parse PE imports: obs.dll for obs-cam-beauty, fuai.dll and OPENGL32 for CNamaSDK, KERNEL32 etc for fuai, libcurl and WINHTTP for obsplus, CNamaSDK and OPENGL32 and CRYPT32 and libcurl for beauty.exe, SetupDi* and QueryDosDeviceW for camera-tool.exe with requireAdministrator manifest
- Read default.effect: contains srgb, rec709, rec2020, PQ, HLG, tonemap, 11 techniques — OBS built-in, copyright Hugh Bailey, not FaceUnity
- Read allinfo-en-US.json: 69 plugins, obs-cam-beauty description
- Read gateway.json: 2 proxies
- Analyze directory structure: each config.dat alongside bundles and/or thumbs

### Evidence
- obs-cam-beauty.dll 9 exports obs_module_*, imports obs.dll — CONFIRMED OBS plugin
- obsplus.dll imports libcurl, obs.dll, WINHTTP — CONFIRMED framework
- CNamaSDK imports fuai.dll and OPENGL32 — CONFIRMED dependency
- beauty.exe imports CNamaSDK and OPENGL32 and libcurl — CONFIRMED uses FaceUnity + OpenGL + curl
- camera-tool.exe imports SetupDi* and QueryDosDeviceW, manifest requireAdministrator — CONFIRMED camera occupation detection
- Logs: face_makeup, body_beautify, hair_normal, GL 4.6, SetParamTex, load texture failed, JS — CONFIRMED bundle loading flow
- default.effect 9.3KB 11 techniques, copyright Hugh Bailey — CONFIRMED OBS, not FaceUnity
- shader-cache 223 .v2 — CONFIRMED OBS D3D11 cache
- Spout DLLs exist — CONFIRMED texture sharing
- MobileNetSSD Caffe 22MB exists — POSSIBLE fallback
- config.dat 5 files 11-80KB entropy 6.27-6.58 — LIKELY index

### Conclusion
- OBS integration boundary documented with CONFIRMED/LIKELY/POSSIBLE/UNKNOWN confidence
- Diagram with confidence levels in OBS_INTEGRATION.md
- **Confidence: HIGH for CONFIRMED dependencies (OBS, CNamaSDK→fuai, GPU OpenGL 4.6, Spout), MEDIUM for LIKELY (default.effect final composite, config.dat indexing), LOW for POSSIBLE (MobileNetSSD fallback)**

### Next Step
- Update dependency_graph.json with detailed nodes and edges with confidence
- For HuanFace SDK, design similar OBS plugin bridge but with open bundle format

---

## Experiment 12: Config.dat Structural Analysis

### Question
What is the structural purpose of config.dat? What does it contain?

### Observation
- 5 files: beauty-hair 11,458 bytes, filters 16,014 bytes, custom_config 41,962 bytes, style_config 14,290 bytes, special-effects 80,023 bytes
- Magic: 1a1a or 3a43 first 2 bytes, not standard
- Entropy 6.27-6.58 (1K) — less than bundle 7.8, more than text 4-5
- Printable ratio 0.43-0.54, zero count 14-23 per 1K
- ASCII strings short random 4-8 chars, no readable keys like "id", "name"
- Directory: each config.dat alongside bundles and/or thumbs in same category folder
- File sizes proportional to preset count: hair 24 items 11KB ~0.5KB per item, filters 100+ thumbs 16KB ~0.16KB per item, makeup custom 60+ bundles 41KB ~0.7KB per item, style 24 bundles 14KB ~0.6KB per item, special-effects 150+ bundles 80KB ~0.5KB per item

### Hypothesis
- Config.dat is encrypted/obfuscated index mapping bundle hash (32-char hex) to metadata (name, thumbnail, category, download/favorite state)
- Uses light obfuscation (XOR) not full AES, or compressed then obfuscated, since entropy 6.27-6.58 less than bundle 7.8
- Contains per-preset entry with id, name, thumbnail path, category

### Experiment
- Read first 64 bytes hex, first 1K entropy, printable ratio, zero count, ASCII strings sample, first uints LE
- Check for PK, JSON, INI, PNG headers — none
- Check for UTF-16 BOM — none
- Check for repetition, offset table — none obvious
- Check directory structure and file sizes vs preset count
- Check UI icons download.png, favorite.png and locale Shop messages for download/favorite state

### Evidence
- Entropy 6.27-6.58, printable 0.43-0.54, zero low — obfuscated text, not plain
- No JSON/INI/PK/PNG headers
- Short random ASCII strings, not readable keys
- Each config.dat alongside bundles and/or thumbs in same category
- File sizes proportional to preset count (0.16-0.7KB per preset) — plausible for index
- UI icons download.png, favorite.png and locale Message.Shop.* suggest download/favorite state stored
- Bundle filenames 32-char hex (MD5-like) — likely ID used in config.dat
- No direct evidence in CNamaSDK.dll for config.dat handling (searched, not found) — maybe handled by obsplus.dll or store

### Conclusion
- Config.dat is PROTECTED / encrypted or obfuscated binary, not JSON/INI — Observed
- Inferred purpose: Index mapping bundle hash to metadata (name, thumbnail, category, download/favorite state) — Inferred, Confidence MEDIUM for purpose, LOW for internal structure
- **Status: PROTECTED / NOT ANALYZED for internal structure (no decryption, no brute-force)**

### Next Step
- Document in CONFIG_DAT_ANALYSIS.md with Observed/Inferred/Confidence
- For HuanFace SDK, propose open preset index JSON format (proposed, not reverse-engineered original)

---

## Experiment 13: Thumbnail Semantic Analysis

### Question
What are the 341 PNG thumbnails? How are they categorized and mapped to bundles?

### Observation
- 341 PNG files
- Locations: beauty-hair/thumbs/ 22 files hair_gradient_*.png hair_normal_*.png descriptive, filters/thumbs/ 100+ files hex names 32-char, data/download.png etc UI icons, themes/Dark|Light SVG icons
- Dimensions via IHDR parsing: most 200x200 or similar, has_alpha true for many
- No makeup thumbs in repo for custom/style (maybe remote), no special-effects thumbs (maybe remote)

### Hypothesis
- Thumbnails are previews for beauty presets (hair, filters, makeup, special-effects) and UI icons
- Mapped to bundles via config.dat index (likely) — from co-location and size proportional
- Hair: descriptive names map to models? e.g., hair_normal_01.png to 7F6145EB...bundle?
- Filters: hex names match bundle hash pattern, maybe filter id is hash and thumb is preview
- UI: download.png, favorite.png for store UI

### Experiment
- Parse PNG IHDR for width, height, bit_depth, color_type, has_alpha
- Categorize by path: hair, filter, makeup, special-effects, UI, beauty, unknown
- Check neighboring bundles in same directory
- Check config.dat proximity
- Visual inspection not possible in sandbox without image viewer, but filename and dimensions give clues

### Evidence
- 341 PNG, dimensions parsed, has_alpha
- Categories: hair 22, filter 100+, UI 8+, other
- Hair: descriptive names, 22 files, alongside 2 bundles and config.dat 11KB
- Filters: 100+ hex names, alongside config.dat 16KB, no bundles (filters maybe just LUTs)
- Makeup: 60+ bundles custom, 24 style, but no thumbs in repo — maybe remote download
- Special-effects: 150+ bundles in numbered folders, no thumbs — maybe remote
- UI: download.png, favorite.png, etc.
- Config.dat in each category folder

### Conclusion
- Thumbnails ARE previews for presets and UI icons — Observed, Confidence HIGH
- Categories by path: hair, filter, UI, etc. — Observed, Confidence HIGH
- Mapping to bundles via config.dat index — Inferred, Confidence MEDIUM for existence, LOW for exact mapping
- **Full catalog in analysis/thumbnail_catalog.json (341 entries with path, size, width, height, has_alpha, category, probable_function, neighboring_bundles, confidence)**

### Next Step
- For HuanFace SDK, design open thumbnail system with manifest.json mapping thumbnail to bundle

---

## Experiment 14: Makeup Parameter Inventory

### Question
What are all makeup and beauty parameters? How are they categorized?

### Observation
- From CNamaSDK.dll strings: 275 makeup params (makeup_intensity_*, tex_*, blend_type_*, makeup_*_color, is_makeup_on, etc.)
- From logs: 6 SetParamTex (tex_lip_mask_zz, tex_blusher, tex_eyeLash, tex_eyeLiner, tex_eye, tex_brow) and 16 load texture failed (eyepupil.png, eyeliner.png, etc.)
- From INI locale: 140 beauty-related keys (HeavyBlur, ColorLevel, DelspotLevel, RedLevel, Clarity, Sharpen, FaceThreed, EyeBright, ToothWhiten, RemovePouchStrength, RemoveNasolabialFoldsStrength, Brightness, Saturation, Filters, FaceSize, etc.)
- From analysis: known lists from Phase 0

### Hypothesis
- Makeup params include intensity (float), color (color), texture (tex_*), blend type (enum), flags (bool)
- Beauty params include skin smooth, whitening, sharpen, tone, face shape, eye, nose, chin, etc.
- Params can be categorized: lip, eye, eyebrow, eyelash, eyeliner, blush, foundation, pupil, highlight_shadow, texture, blend, beauty, other

### Experiment
- Extract all makeup params from strings via regex (makeup_*, tex_*, blend_type_*, is_makeup_*, ColorLevel, HeavyBlur, etc.) — 275 unique
- Parse INI files for beauty params — 118 unique beauty keys
- Combine with known lists from Phase 0
- Categorize by keyword: lip, eye, brow, lash, liner, blush, foundation, pupil, highlight_shadow, texture, blend, beauty, other
- Guess type by name: color → color, intensity/level/strength/brightness/saturation → float, tex_/texture → texture, is_/enable → bool, blend → enum
- Collect evidence: CNamaSDK strings, known list, INI locale, runtime log SetParamTex
- Assign confidence HIGH if >=2 evidence, MEDIUM if 1, LOW if 0

### Evidence
- 275 params from strings, 118 from INI, 7 known from Phase 0, total 391 unique after deduplication
- Categories: lip, eye, eyebrow, eyelash, eyeliner, blush, foundation, pupil, highlight_shadow, texture, blend, beauty, other
- Types: float, color, texture, bool, enum, unknown
- Evidence per param: e.g., makeup_intensity_lip has CNamaSDK strings + known list + log? Actually log has tex_* not intensity, but intensity from strings
- Full catalog in analysis/makeup_parameter_catalog.json (391 entries with name, category, type, range null, default null, evidence list, confidence)

### Conclusion
- Makeup parameter inventory complete with 391 entries — Observed from strings, logs, INI
- Categories and types guessed from name, not from actual range/default (range and default null, not invented)
- **Confidence: HIGH for existence (from strings), MEDIUM for category/type (from name), LOW for range/default (not found, set null)**

### Next Step
- For HuanFace SDK, use this catalog as requirement for makeup engine and beauty engine params, with actual range/default to be determined via testing or open models

---

## Experiment 15: Shader Cache Analysis

### Question
What are the 223 .v2 files? Are they related to beauty system or just OBS cache?

### Observation
- 223 files in ProgramData/obs-studio/shader-cache/*.v2, few KB each
- Magic random, binary, no PK/JSON/HLSL
- default.effect 9.3KB text 11 techniques color space, copyright Hugh Bailey, OBS Studio
- format_conversion.effect 13KB similar
- CNamaSDK.dll strings contain FaceUnity shaders: MakeupFilterPassNAMA, MakeupWarpNAMA, MakeupPipeline2, lip_mask, etc., plus GLProgramNew, GLTechnique, g_makeup_vbo

### Hypothesis
- .v2 files are OBS D3D11 compiled shader bytecode cache — from OBS docs, shader-cache stores compiled shaders for faster startup, .v2 version 2
- Hash in filename is hash of shader source
- Some .v2 are cached versions of default.effect and format_conversion.effect, which would be used by beauty plugin for final compositing, but also other OBS plugins
- FaceUnity shaders are inside encrypted bundles, not in .effect or .v2

### Experiment
- Read first 64 bytes hex for .v2 — binary, not readable
- Read default.effect and format_conversion.effect — text, OBS built-in, copyright Hugh Bailey, techniques Draw, color space functions srgb, rec709, rec2020, PQ, HLG, tonemap
- Search CNamaSDK.dll for FaceUnity shader names — found MakeupFilterPassNAMA etc.
- Check if any .effect contains makeup logic — none, only color space
- Check OBS documentation for shader-cache

### Evidence
- .v2 binary, few KB, 223 files, in ProgramData/obs-studio/shader-cache/ — OBS cache dir
- default.effect 9.3KB 11 techniques, copyright Hugh Bailey — OBS Studio, not FaceUnity
- format_conversion.effect 13KB similar — OBS
- FaceUnity shader names in DLL: MakeupFilterPassNAMA, MakeupWarpNAMA, MakeupPipeline2, lip_mask, etc. — existence HIGH, but implementation PROTECTED
- No FaceUnity shader code in .effect files

### Conclusion
- OBS effects (.effect) are OBS Studio, NOT FaceUnity — CONFIRMED, Confidence HIGH
- Shader cache (.v2) is OBS D3D11 bytecode cache — CONFIRMED, Confidence HIGH, relationship to beauty: MEDIUM (some are beauty-related cached shaders, but not all, cannot determine which without OBS source mapping)
- FaceUnity shaders inside encrypted bundles — existence HIGH from DLL strings, purpose MEDIUM from names, implementation PROTECTED (no bypass)
- **Full catalog in analysis/shader_catalog.json (227 entries: 4 .effect + 223 .v2 with path, size, type, origin, extractable, confidence)**

### Next Step
- For HuanFace SDK, write clean-room GLSL shaders for makeup/beauty, not reuse OBS effects (OBS effects can be used for final compositing if HuanFace used as OBS plugin)

---

## Experiment 16: Executable Observation

### Question
What do beauty.exe and camera-tool.exe do? Can we observe their behavior non-invasively?

### Observation
- beauty.exe 1.4MB, imports CNamaSDK.dll, OPENGL32.dll, CRYPT32.dll, WS2_32.dll, libcurl, build PDB .../obs-cam-beauty/nama/RelWithDebInfo/beauty.pdb
- camera-tool.exe 176KB, imports SetupDiEnumDeviceInfo, SetupDiDestroyDeviceInfoList, SetupDiGetDevicePropertyW, SetupDiGetDeviceRegistryPropertyW, QueryDosDeviceW, manifest requireAdministrator
- No --help, -h, /?, usage, version strings — likely GUI apps, not CLI
- No Wine available in sandbox (which wine not found)

### Hypothesis
- beauty.exe is standalone beauty test harness for fuImageBeauty (image-based beauty) — from name, imports CNamaSDK + OPENGL32 + libcurl, build path in nama, fuImageBeauty* APIs exist
- camera-tool.exe is camera occupation detection tool — from name, SetupDi* imports, requireAdministrator, locale CheckWhereDeviceUsed.DlgTitle="Camera Occupation Detection Tool"

### Experiment
- Try which wine, wine --version — not found
- Try file command — not found
- Try strings for help, usage, version, --, /, \, error, camera, beauty, fu, nama, bundle, obs, device, init, render, texture, frame — found version.dll, curl_easy_init, CNamaSDK.dll, SetupDi*, requireAdministrator, but no help
- Parse PE imports for both exes
- Check locale INI for CheckWhereDeviceUsed keys

### Evidence
- beauty.exe imports CNamaSDK.dll, OPENGL32.dll, CRYPT32.dll, WS2_32.dll, libcurl — uses FaceUnity + OpenGL + curl
- No help strings
- Build PDB in obs-cam-beauty/nama/
- fuImageBeauty* APIs in CNamaSDK.dll
- camera-tool.exe imports SetupDi*, QueryDosDeviceW, manifest requireAdministrator
- Locale INI: CheckWhereDeviceUsed.ButtonText="Check Usage", Info="Checking camera device usage...", DlgTitle="Camera Occupation Detection Tool", etc.

### Conclusion
- beauty.exe purpose: standalone beauty test harness for image beauty — Inferred, Confidence MEDIUM for purpose, LOW for dynamic behavior (no execution)
- camera-tool.exe purpose: camera occupation detection — Inferred, Confidence HIGH for purpose, LOW for dynamic behavior
- **Status: PROTECTED / NOT ANALYZED for dynamic behavior (requires Windows/Wine and/or admin, no bypass, no execution beyond static)**
- **Full observations in analysis/executable_observations.json**

### Next Step
- For HuanFace SDK, we don't need beauty.exe or camera-tool.exe, but we can create our own test harness and camera detection tool as open source

---

