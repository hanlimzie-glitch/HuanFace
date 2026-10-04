# Binary Analysis — HuanFace (Phase 0/1)

## 1. Core Binaries

### CNamaSDK.dll (19.5 MB x64, 15.2 MB x32)
- **Path:** `bin/64bit/CNamaSDK.dll`
- **Purpose:** FaceUnity NamaSDK — main SDK
- **Evidence:**
  - Strings: `fuCreateItemFromPackage`, `fuRenderBundles`, `fuItemSetParamd`, `fuBindItems`, `fuFaceProcessorSetFaceLandmarkQuality`, `fuFaceProcessorSetDetectSmallFace`, `fuSetFaceProcessorDetectMode`, etc. (200+ APIs)
  - `C:\CI12\builds\zx2dyZvQ\0\ruitaocai\CNamaSDK\src\...` — build path reveals source structure: `core`, `modules/global`, `modules/makeup`, `modules/makeup/makeupController.cpp`, `common/BundleHelper.cpp`
  - `BundleHelper::DecryptObfuscatedPackage`, `VerifySignature`, `Decrypt bundle failed`
  - `mbedtls` crypto: `AES-128-CBC`, `AES-256-CBC`, `fu_mbedtls_cipher_crypt`, `PKCS#1 encryption`
  - Makeup: `MakeupFilterPassNAMA`, `MakeupWarpNAMA`, `lip_mask.cpp`, `makeup.cpp`, `g_makeup_vbo`, `g_makeup_ebo`, `tex_brow`, `tex_eye`, `tex_eyeLiner`, `tex_eyeLash`, `tex_blusher`, `tex_lip`, `tex_pupil`, `tex_foundation`, `tex_shadow`, `tex_lip_mask_zz`, `tex_lip_mask_bz`, etc.
  - Beauty: `MakeupBeautifyBody`, `face_beautification`
  - JS: `NamaContext.cpp:98 [js]`
  - GL: `fuInitGLContext`, `fuMakeGLContextCurrent`, `initialGLExtentions: glversion max = 4, min = 6`, `GLLoader.cc`
- **Dependencies:** OpenGL, D3D, mbedtls (static linked), FUAI
- **Confidence:** HIGH

### fuai.dll (29 MB x64, 18 MB x32)
- **Path:** `bin/64bit/fuai.dll`
- **Purpose:** FaceUnity AI — inference engine for face/human detection, landmarks, mesh, segmentation
- **Evidence:**
  - Strings: `FUAI_NewFaceProcessorFromBundleWithConfig`, `FUAI_NewHumanProcessorFromBundleWithConfig`, `FaceMeshV2`, `face_meshV2_interface.cc`, `ProcessFacemesh`, `BMesh::triangulate_face`, `FUAI_FaceProcessorGetArmeshVerticesFromResult`, `FUAI_FaceProcessorGetFaceMeshV2VerticesFromResult`, `FUAI_FaceProcessorGetFaceMeshV2Triangles`, `FUAI_ConvertGLToDdeMeshLandmark3ds`, `HumanProcessor`, `HandDetector`, `HumanRetargeter`, `MotionController`, `Avatar`, `BVH`, `TFLite` related quantization params (`Quantization parameters has non-null scale but null zero_point`)
  - Build path: `D:\GitLab-Runner\builds\E3YA_xXv\0\chiliangyang\fuai\fuai\face\...`
  - Also contains `unsupported encryption`
- **Dependencies:** TFLite, OpenCV-like, math libs
- **Confidence:** HIGH

### obs-cam-beauty.dll (63 KB x64, 53 KB x32)
- **Path:** `bin/64bit/obs-cam-beauty.dll`
- **Purpose:** OBS Studio plugin — entry point that OBS loads
- **Evidence:**
  - Small size, imports OBS API
  - Strings: `obs-cam-beauty`, `obs-cam-beauty-loader`, `plugin module file not exists`, `can not get qt version info`, build path `D:\work\obsplus\client\obs-studio-29.0\build\x64\plugins\obsplus\obs-cam-beauty\RelWithDebInfo\obs-cam-beauty.pdb`
  - No FaceUnity logic inside, just bridge
- **Confidence:** HIGH

### obs-cam-beauty_qt5.dll / qt6.dll (2.4 MB)
- **Purpose:** Qt UI for beauty panel in OBS
- **Evidence:** Qt5/6, contains UI strings from locale INI
- **Confidence:** MEDIUM

### beauty.exe (1.4 MB)
- **Purpose:** Standalone test harness or helper
- **Evidence:** Build path `.../obs-studio/plugins/obsplus/obs-cam-beauty/nama/RelWithDebInfo/beauty.pdb`, small console app
- **Confidence:** MEDIUM

### camera-tool.exe (176 KB)
- **Purpose:** Camera occupation detection — helps user find which app uses camera
- **Evidence:** From locale: `CheckWhereDeviceUsed.ButtonText="Check Usage"`, `CheckWhereDeviceUsed.DlgTitle="Camera Occupation Detection Tool"`
- **Confidence:** HIGH

### obsplus.dll (456 KB)
- **Path:** `ProgramData/obs-studio/plugins/obsplus/bin/64bit/obsplus.dll`
- **Purpose:** OBSPlus framework — hosts multiple plugins (beauty, heart-rate, virtualcam, etc.)
- **Confidence:** MEDIUM

### Spout DLLs
- **Spout.dll, SpoutDX.dll, SpoutLibrary.dll, win-spout.dll**
- **Purpose:** Spout texture sharing — allows OBS texture to be shared with other apps (e.g., use beauty camera in other software)
- **Evidence:** Known open-source project Spout (BSD)
- **Confidence:** HIGH

### Store DLLs
- **core.dll, obs-plugins-store.dll, websocket.dll, store_qt5/6.dll**
- **Purpose:** OBS Store marketplace — download plugins, handle auth, proxy
- **Evidence:** `gateway.json` with proxies, `allinfo-en-US.json` marketplace metadata, `token.bin`
- **Confidence:** HIGH

---

## 2. Bundle Binary Format (Encrypted)

### 2.1 Header Analysis

**Sample:** `face_beautification.bundle` (5.2 MB)

```
Offset 0x00: F3 5B 06 12 AC 5E 00 E1 B8 96 66 80 18 3E 3A 25 ...
```

- **Magic:** `F3 5B 06 12` — 4 bytes, consistent across 70% of bundles
- **Next 12-60 bytes:** High entropy, no readable strings, no zeros — likely encrypted header containing version, size, checksum, or IV
- **No PK, no JSON, no PNG header** — not zip, not plain
- **Entropy:** 7.79-7.85 (close to 8.0 = random) — encrypted/compressed

**Variant Magics (30% of bundles):**
- `7C D3 33 56`, `10 11 D2 D5`, `B2 AA 43 33`, `3D E6 15 6D`, `35 59 CD 0B`, etc.
- These appear in some custom makeup bundles
- **Hypothesis A:** Different bundle type (controller resource vs model) — string `is_controller_resource_bundle` suggests type differentiation
- **Hypothesis B:** Different encryption key or version
- **Hypothesis C:** Older format that wasn't re-encrypted
- **Confidence:** LOW, needs more samples

### 2.2 Encryption Evidence

From CNamaSDK.dll strings:

```
enter DecryptObfuscatedPackage size:{}
CNamaSDK::BundleHelper::DecryptObfuscatedPackage
DecryptObfuscatedPackage Failed!
Decrypt bundle failed, error:{}
VerifySignature
CNamaSDK::BundleHelper::VerifySignature
fu_mbedtls_cipher_crypt
AES-128-CBC, AES-192-CBC, AES-256-CBC
AES-128-ECB, AES-256-GCM, etc.
SOFTWARE\Microsoft\Cryptography
CryptAcquireContextA, CryptGenRandom
```

**Conclusion:**
- Bundles are **obfuscated + encrypted + signed**
- Decryption function is `DecryptObfuscatedPackage`
- Uses **mbedtls** for crypto, likely AES-CBC
- Signature verification via `VerifySignature` — prevents tampering
- Key likely stored in CNamaSDK.dll or derived from hardware / license

**We MUST NOT attempt to extract key or bypass verification.** Our SDK will need to design a **compatible but independent bundle format** that can hold similar resources (textures, params, shaders) without using FaceUnity's encrypted format.

### 2.3 Internal Structure Hypothesis (from logs + strings)

Based on logs and FaceUnity public docs (not from decrypted payload):

```
Decrypted Bundle (hypothetical)
├── manifest.json / metadata
│   ├── item_name: "face_makeup", "body_beautify", "hair_normal", "new_greensegment_..."
│   ├── type: controller, model, texture, etc.
│   └── version
├── JavaScript controller
│   ├── Example: "liufei init stbline in", "liufei statesArray init,len: 1", "liufei show makeup: Group_1_13"
│   └── Contains logic for makeup application, state machine
├── Textures
│   ├── Expected: eyepupil.png, eyeliner.png, eyelash.png, brow.png, eye.png, eye2.png, eye3.png, eye4.png, lip_top2.png, zhuangrong_sh.png, etc.
│   ├── Actual binding: tex_eye, tex_brow, tex_eyeLash, tex_eyeLiner, tex_blusher, tex_lip_mask_zz, etc.
│   └── Format: PNG, possibly with alpha
├── Shaders
│   ├── MakeupFilterPassNAMA, MakeupWarpNAMA, MakeupPipeline2, lip_mask, etc.
│   └── GLSL/HLSL for face warp and makeup blend
├── Meshes
│   ├── g_makeup_vbo, g_makeup_ebo
│   └── Face mesh, lip mask mesh, eye mesh
├── Parameters
│   ├── makeup_intensity, makeup_intensity_lip, makeup_intensity_pupil, makeup_intensity_eye, makeup_intensity_eyeLiner, makeup_intensity_eyelash, makeup_intensity_eyeBrow, makeup_intensity_blusher, makeup_intensity_foundation, makeup_intensity_highlight, makeup_intensity_shadow
│   ├── makeup_lip_color, makeup_lip_color2, makeup_eye_color, etc.
│   ├── blend_type_tex_eye, blend_type_tex_brown, etc.
│   ├── is_makeup_on, is_clear_makeup, makeup_occlusion, makeup_lip_occlusion, etc.
│   └── Beauty params: HeavyBlur, ColorLevel, DelspotLevel, RedLevel, Clarity, Sharpen, etc.
└── AI Models (for ai_face_processor, ai_human_processor)
    ├── TFLite models
    └── Config for face landmark quality, detect small face, etc.
```

**Confidence:** MEDIUM for existence of these sections, LOW for exact structure.

---

## 3. Config.dat Format

- **Files:** 5 files, sizes 11KB-80KB
- **Header:** Not readable, first bytes e.g., `:C\u0017YU\u0016_...` — not JSON, high entropy
- **Hypothesis:** Encrypted index mapping bundle hash to metadata (name, preview image, parameters, category)
- **Evidence:** Located in same folders as bundles and thumbs, named `config.dat`, typical for FaceUnity store
- **Confidence:** MEDIUM that it's encrypted, LOW for internal structure

---

## 4. OBS Effect Files

### default.effect (9.3 KB, text)
- **Purpose:** OBS final compositing shader
- **Contains:**
  - Color space conversion: `srgb_linear_to_nonlinear`, `srgb_nonlinear_to_linear`, `rec709_to_rec2020`, `rec2020_to_rec709`, `d65p3_to_rec709`, `linear_to_st2084` (PQ), `st2084_to_linear`, `linear_to_hlg`, `hlg_to_linear`, `reinhard` tonemap, `eetf` (electro-electro transfer function)
  - Techniques: `Draw`, `DrawAlphaDivide`, `DrawNonlinearAlpha`, `DrawSrgbDecompress`, `DrawMultiply`, `DrawTonemap`, `DrawPQ`, `DrawTonemapPQ`, etc.
- **Not FaceUnity shader** — it's OBS's own shader for handling different video formats (SDR, HDR, PQ, HLG)
- **Confidence:** HIGH

### format_conversion.effect (13 KB, text)
- **Purpose:** OBS format conversion (YUV to RGB, etc.)
- **Confidence:** MEDIUM-HIGH (not fully read)

---

## 5. AI Models

### MobileNetSSD (Caffe)
- **Files:** `MobileNetSSD_deploy.caffemodel` (22 MB) + `prototxt`
- **Purpose:** Object detection — likely fallback for background segmentation or person detection
- **Evidence:** Standard Caffe model, known for person detection
- **Confidence:** MEDIUM (usage not seen in logs)

### AI Face Processor Bundle (23 MB)
- **Purpose:** Face detection, landmarks, face mesh, head pose
- **Evidence:** Name, size, string `FUAI_NewFaceProcessorFromBundleWithConfig`, `FaceMeshV2`
- **Confidence:** HIGH

### AI Human Processor Bundle (42 MB)
- **Purpose:** Human pose, segmentation, hand detection
- **Evidence:** Name, size, `FUAI_NewHumanProcessorFromBundleWithConfig`, `HumanProcessor`, `HandDetector`
- **Confidence:** HIGH

---

## 6. API Surface (from CNamaSDK.dll)

### Core Lifecycle
- `fuSetup`, `fuSetupInternalCheck`, `fuSetupLocal`, `fuSetupDeviceLocal`, `fuSetupInternalCheckPackageBind`
- `fuInitGLContext`, `fuMakeGLContextCurrent`, `fuDestroyGLContext`, `fuIsLibraryInit`, `fuDestroyLibData`, `fuOnDeviceLost`, `fuDestroyAllItems`

### Item Management
- `fuCreateItemFromPackage`, `fuCreateLiteItemFromPackage`, `fuDestroyItem`, `fuCreateInstance`, `fuDestroyInstance`, `fuCreateScene`, `fuDestroyScene`, `fuBindItems`, `fuBindItemsToInstance`, `fuBindItemsToScene`, `fuAvatarBindItems`, `fuAvatarUnbindItems`

### Rendering
- `fuRender`, `fuRenderBundles`, `fuRenderBundlesEx`, `fuRenderBundlesSplitView`, `fuRenderItems`, `fuRenderItemsEx`, `fuRenderItemsEx2`, `fuRenderItemsMasked`, `fuCreateTexForItem`, `fuDeleteTexForItem`

### Parameters
- `fuItemSetParamd`, `fuItemSetParamdv`, `fuItemSetParams`, `fuItemSetParamu64`, `fuItemSetParamu8v`, `fuItemGetParamd`, `fuItemGetParamdv`, `fuItemGetParamfv`, `fuItemGetParams`, `fuItemGetParamu8v`
- `fuSetMakeupCoverResource`, `fuSetFaceProcessorDetectMode`, `fuFaceProcessorSetFaceLandmarkQuality`, `fuFaceProcessorSetDetectSmallFace`, `fuFaceProcessorSetMinFaceRatio`, `fuHumanProcessorSetMaxHumans`, `fuHumanProcessorSetFov`, etc.

### Face/Human Tracking
- `fuFaceProcessorGetNumResults`, `fuFaceProcessorGetConfidenceScore`, `fuFaceProcessorGetResultFaceOcclusion`, `fuFaceProcessorGetResultHairMask`, `fuFaceProcessorGetResultHeadMask`, `fuHumanProcessorGetNumResults`, `fuHumanProcessorGetResultRect`, `fuHumanProcessorGetResultJoint2ds`, `fuHumanProcessorGetResultJoint3ds`, `fuHandDetectorGetResultNumHands`, etc.

### Image Beauty
- `fuImageBeautyCreateTexture`, `fuImageBeautySetParam`, `fuImageBeautyGetParam`, `fuImageBeautyPreProcess`, `fuImageBeautyPreview`, `fuImageBeautyGetResult`, etc.

**Total:** 200+ APIs

**Implication for HuanFace SDK:** We should design a **similar but clean-room API** that is compatible in spirit but not copying proprietary implementation. For example:

```cpp
// HuanFace proposed C API (clean-room, not copying FaceUnity)
HF_CreateEngine();
HF_LoadBundle("preset.bundle");
HF_SetParamFloat("intensity", 0.8f);
HF_SetParamTexture("tex_eye", texId);
HF_ProcessFrame(input, output);
HF_Destroy();
```

---

## 7. Security & Compliance Notes

- **No decryption attempted.** All analysis is static (strings, headers, entropy, logs).
- **No key extraction.** We observed that mbedtls is used, but did not attempt to find key in binary (which would be reverse engineering for bypass).
- **No bypass of VerifySignature.** We respect signature verification.
- **Clean-room design:** For HuanFace SDK, we will create our own bundle format (e.g., JSON + PNG + GLSL) that can hold similar resources but is **not** compatible with FaceUnity's encrypted format unless user provides decrypted resources legally.

---

## 8. Next Steps for Phase 1

1. Build `bundle_inspector` tool (Python) that:
   - Reads magic, size, entropy
   - Tries to detect if file is encrypted (entropy > 7.5)
   - Lists all param names from DLL strings that could be inside bundles
2. Build `asset_inspector` to map PNG thumbs to bundle hashes via file name similarity or config.dat (if possible)
3. Build `metadata_inspector` to parse INI locale and extract full beauty/makeup param list
4. Document all `fu*` APIs into `docs/API_REFERENCE.md`
5. Attempt to run `beauty.exe` with `--help` or in sandbox to see if it loads bundles (without bypassing auth)
6. Create `docs/BUNDLE_FORMAT.md` with detailed header structure hypothesis and validation steps

---

**End of Binary Analysis**
