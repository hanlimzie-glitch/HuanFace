# Symbol & String Analysis — Phase 1C

**Binaries Analyzed:**
- `bin/64bit/CNamaSDK.dll` (19.5 MB)
- `bin/64bit/fuai.dll` (29 MB)
- `bin/64bit/obs-cam-beauty.dll` (63 KB)
- `ProgramData/obs-studio/plugins/obsplus/bin/64bit/obsplus.dll` (456 KB)
- `bin/64bit/beauty.exe` (1.4 MB)
- `bin/64bit/camera-tool.exe` (176 KB)

**Method:** Static strings extraction (min length 4, ASCII), no execution, no bypass.

**Total Unique Relevant Strings:** 10,386 (filtered by keywords)

**By Category:**
- other: 3296
- face: 1210
- filter: 976
- avatar/animation: 834
- ai: 710
- texture: 638
- makeup_part: 486
- shader: 384
- mask: 359
- beauty: 307
- js: 284
- warp/mesh: 272
- bundle: 232
- parameter: 226
- makeup: 150
- rendering: 22

**Full catalog:** `analysis/string_catalog.json` (10,386 entries with binary, offset, string, category, keyword)

---

## 1. Makeup Symbols

### From CNamaSDK.dll

**Makeup intensity:**
- `makeup_intensity`, `makeup_intensity_lip`, `makeup_intensity_pupil`, `makeup_intensity_eye`, `makeup_intensity_eyeLiner`, `makeup_intensity_eyelash`, `makeup_intensity_eyeBrow`, `makeup_intensity_blusher`, `makeup_intensity_foundation`, `makeup_intensity_highlight`, `makeup_intensity_shadow`, `makeup_intensity1`, `makeup_intensity2`, `makeup_intensity3`, `makeup_intensity4`

**Makeup colors:**
- `makeup_lip_color`, `makeup_lip_color2`, `makeup_lip_color_v2`, `makeup_eye_color`, `makeup_eye_color2`, `makeup_eye_color3`, `makeup_eye_color4`, `makeup_eyeLiner_color`, `makeup_eyelash_color`, `makeup_eyeBrow_color`, `makeup_blusher_color`, `makeup_blusher_color2`, `makeup_foundation_color`, `makeup_highlight_color`, `makeup_shadow_color`, `makeup_pupil_color`, `makeup_color1`, `makeup_color2`, `makeup_color3`, `makeup_color4`, `makeup_color12`, `makeup_color13`, `makeup_color14`

**Makeup flags:**
- `is_makeup_on`, `is_clear_makeup`, `makeup_occlusion`, `makeup_occlusion_type`, `makeup_lip_occlusion`, `makeup_lip_highlight`, `makeup_lip_highlight_enable`, `makeup_lip_highlight_strength`, `makeup_lip_color_mix_strength`, `makeup_lip_mask`, `makeup_cover_resource`, `makeup_current_index`, `makeup_index`, `makeup_data`, `fix_makeup_data`

**Makeup textures (tex_*):**
- `tex_brow`, `tex_eye`, `tex_eye2`, `tex_eye3`, `tex_eye4`, `tex_pupil`, `tex_eyeLash`, `tex_lip`, `tex_eyeLiner`, `tex_blusher`, `tex_blusher2`, `tex_foundation`, `tex_shadow`, `tex_lip_highlight`, `tex_lip_mask_bz`, `tex_lip_mask_zz`, `tex_lip_mask_bite_bz`, `tex_lip_mask_bite_zz`, `tex_lip_mask_highlight_bz`, `tex_lip_mask_highlight_zz`, `tex_makeup`, `tex_lut`, `tex_lut2`, `tex_lipstick_median`, `tex_blend_weight_and_level_map`, `tex_face_occu_blur`, `tex_occudebug`

**Blend types:**
- `blend_type_tex_eye`, `blend_type_tex_eye2`, `blend_type_tex_eye3`, `blend_type_tex_eye4`, `blend_type_tex_brown`, `blend_type_tex_eyeLash`, `blend_type_tex_eyeLiner`, `blend_type_tex_blusher`, `blend_type_tex_blusher2`, `blend_type_tex_highlight`, `blend_type_tex_shadow`, `blend_type_tex_pupil`, `blend_type_tex_lip`

**Makeup pipeline:**
- `MakeupFilter`, `MakeupFilterPassNAMA`, `CMakeup::MakeupFilterPassNAMA`, `copy_makeup_tex`, `MakeupFilterPassNAMA_Native`, `MakeupFilterPassNAMA_NativeEyelash`, `MakeupFilterPassNAMA_NativeWithLeftAndRight`, `MakeupWarpNAMA`, `makeupwarpnama2`, `makeupwarpnama3`, `MakeupWarpNAMA_Native`, `warp_makeup`, `CMakeup::DrawFaceMaskV2`, `CMakeup::DrawFaceMask`, `lip_makeup`, `face_makeup`, `eye_makeup`, `brow_makeup`, `MakeupPipeline2`, `MakeupPipeline2_Native`, `lip_makeup_new`, `eye_makeup_new`, `brow_makeup_new`, `MakeupPipeline`, `MakeupDataInit`, `MakeupDataInit2`, `MakeupFilterPass`, `makeupController::makeupController called`, `MakeUpController::SetParamD`, `MakeUpController::SetParamDV`

**Lip-specific:**
- `lip_mask.cpp`, `LipMaskGetTexture2`, `g_lip_mask_rtt_context1`, `g_lip_mask_rtt_context2`, `lipmask2`, `lipmask`, `lipmask create tex error!!!!!!`, `g_lip_gloss_mask_rtt1`, `g_lip_gloss_mask_rtt2`, `g_lip_origin_rtt`, `g_lip_blured_rtt`, `g_lip_hp_rtt`, `g_lip_gloss_delta_rtt`, `lipmask2_native`, `lipmask_2`, `makeup_lip_gloss_blur`, `makeup_lip_gloss_highpass`, `makeup_lip_gloss_final`, `u_lipColortexture`, `u_lipGlossSpecPowFactor`, `u_lipGlossSpecFactor`, `g_lip_occumask_rtt1`, `lip_occu_mask_vbo`, `lip_occumask_dilation_tech`, `LIP_MASK_SIZE`, `g_lip_occumask_rtt2`, `lip_occu_mask_blur_shader`, `lip_highlight_mask`, `lip_polygon_shader`, `lip_mask_preprocess_shader`, `lip_mask_blur_shader`, `GetLipMaskTexture: please set landmarks array`

**Evidence:** CNamaSDK.dll strings, offset 0x... (varies), category makeup, confidence HIGH

---

## 2. Beauty Symbols

### From CNamaSDK.dll + INI

**From INI locale (en-US.ini, 263 keys, 140 beauty-related):**
- `HeavyBlur` (skin smooth), `BalanceSmooth`, `FineSmooth`, `ClearSmooth`, `HazySmooth`, `BlurLevel`, `ColorLevel`, `ColorLevelType`, `DelspotLevel`, `RedLevel`, `Clarity`, `Sharpen`, `FaceThreed`, `EyeBright`, `ToothWhiten`, `RemovePouchStrength`, `RemoveNasolabialFoldsStrength`, `Brightness`, `Saturation`, `Filters`, `BeautyFilterLevel`, `Original`, `White`, `Pink`, `Fresh`, `CoolTone`, `WarmTone`, `BeautyFilter.Clear`, `FaceSize`, `FaceLandmarkQuality`, `BodyNum`, `BeautyProtection`, `BeautyRenderFPS`, `FPSSameOBS`, `BigFaceSize`, `MediumFaceSize`, `SmallFaceSize`, `BaseBeauty`, `SkinDect`, `HeavyBlurDescription`, `ColorLevelType.EnableSkinseg`, `ColorLevelType.DisableSkinseg`, `DelspotLevelDescription`, `Clarity.Description`, `SharpenDescription`, `FaceThreed`, `EyeBright`, `ToothWhiten`, `RemovePouchStrength`, `RemoveNasolabialFoldsStrength`, `Brightness`, `Saturation`

**From DLL strings:**
- `MakeupBeautifyBody`, `timer_makeup_beautifybody`, `g_intensity_lip_thick`, `face_beautification`, `body_beautify`, `body_slim`, `face_beautification.bundle`, `face_makeup.bundle`, `body_slim.bundle`

**Category:** beauty, confidence HIGH

---

## 3. Face & Landmark Symbols

**From CNamaSDK.dll:**
- `face_rect_flipy`, `landmarks_flipy`, `face_meshV2`, `face_meshV2_point_smooth_h`, `use_face_meshV2`, `face_meshV2_interface.cc`, `face_meshV2.cc`, `armesh_vertex_num`, `arMesh`, `face_makeup`, `face_beautification`, `face_beautify`, `FaceSize`, `FaceLandmarkQuality`, `FaceThreed`, `FaceProcessor`, `FUAITYPE_FACEPROCESSOR_LIPSOCCUSEGMENT`, `FUAI_FaceProcessorSetUseLipsOccuSegmenter`, `FUAI_FaceProcessorGetArmeshVerticesFromResult`, `FUAI_FaceProcessorGetFaceMeshV2VerticesFromResult`, `FUAI_FaceProcessorGetFaceMeshV2Triangles`, `FUAI_FaceProcessorGetFaceMeshV2TexCoords`, `FUAI_FaceProcessorGetFaceMeshV2AffineMatrixFromResult`, `FUAI_ConvertGLToDdeMeshVertices`, `FUAI_ConvertGLToDdeMeshLandmark3ds`, `FUAI_ConvertGLToDdeMeshTriangles`, `FUAI_MirrorMeshVertices`

**From fuai.dll:**
- `face_meshV2`, `ProcessFacemesh`, `mesh model preprocess timer`, `mesh refine model timer`, `HaveSameShapes`, `FUAI_ConvertGLToDdeMeshLandmark3ds`, `FUAI_FaceProcessorGetArmeshVerticesFromResult`, `FUAI_FaceProcessorGetFaceMeshV2VerticesFromResult`, `FaceMeshV2Interface`, `FaceMeshV2`, `BMesh::triangulate_face`, `PTA_NS::BMesh::BM_face_exists_multi`, `bmesh error: infinite loop in disk cycle!`

**Category:** face, confidence HIGH

---

## 4. Eye, Lip, Brow, etc.

**From logs + DLL:**
- `eyepupil.png`, `eyeliner.png`, `eyelash.png`, `brow.png`, `eye.png`, `eye2.png`, `eye3.png`, `eye4.png`, `lip_top2.png`, `lip_bz1.png`, `zhuangrong_sh.png`, `zhuangrong_sh2.png`, `zhuangrong_fd.png`, `zhuangrong_gg.png`, `zhuangrong_yy.png`, `gloss_lut.png`
- `tex_eye`, `tex_brow`, `tex_eyeLash`, `tex_eyeLiner`, `tex_blusher`, `tex_lip_mask_zz`, `tex_lip`, `tex_pupil`, etc.

**Interpretation:**
- `zhuangrong` = makeup in Chinese
- `sh` = maybe eye shadow? `fd` = foundation? `gg` = bone? `yy` = eye shadow?
- `bz` = bite? `zz` = ?
- Need Chinese translation but not essential for SDK

**Category:** makeup_part, confidence HIGH for existence, MEDIUM for meaning

---

## 5. Rendering & Shader Symbols

**From CNamaSDK.dll:**
- `g_makeup_vbo`, `g_makeup_ebo`, `timer_makeup_beautifybody`, `timer_copy_tex_mat`, `timer_copy_tex_warp`, `m_copytex_tech`, `vertex_groups`, `tex_cube`, `2d_bigtex_desc.json`, `tex_yuv`, `tex_segmentation`, `tex_mask`, `tex_segment_result`, `tex_src`, `tex_input`, `tex_ori`, `tex_occu`, `tex_mouthoccu`, `tex_origin`, `tex_gloss`, `tex_color`, `tex_highlight`, `tex_s`, `tex_vtf`, `tex_capture`, `CURRENT_VERTEX_ATTRIB`, `VERTEX_SHADER`, `MAX_VERTEX_ATTRIBS`, `MAX_VERTEX_UNIFORM_VECTORS`, `MAX_VERTEX_TEXTURE_IMAGE_UNITS`, `VERTEX_ATTRIB_ARRAY_ENABLED`, `VERTEX_ATTRIB_ARRAY_SIZE`, `VERTEX_ATTRIB_ARRAY_STRIDE`, `VERTEX_ATTRIB_ARRAY_TYPE`, `VERTEX_ATTRIB_ARRAY_NORMALIZED`, `VERTEX_ATTRIB_ARRAY_POINTER`, `VERTEX_ATTRIB_ARRAY_BUFFER_BINDING`, `armesh_vertex_num`, `GLProgramNew`, `GLTechnique`, `GLTechniqueBase`, `Material`, `Timer`, `TimerManager`, `CreateProgram`, `CreateBinaryProgram`, `DeclareUniform`, `SetFloat`, `SetFloat2`, `SetFloat3`, `SetFloat4`, `SetFloatArray`, `Draw`, `DrawScreenQuad`, `DrawBuffer`, `DrawElements`, `EnableLazyFree`, `GetProgramID`, `IsSupportCubemapLod`, `NeedCreateProgram`, `ClearGLResource`, `ReportAllTimer`, `ReportTimer`, `ResetTimer`, `RigisterTimer`

**From fuai.dll:**
- `GL_EXT_shader_explicit_arithmetic_types_float16`, `GL_EXT_shader_16bit_storage`, `VK_KHR_shader_float16_int8`, `GL_EXT_shader_16`, etc. — shader extensions

**Interpretation:**
- FaceUnity uses OpenGL with custom `GLProgramNew`, `GLTechnique`, `Material` classes
- Supports binary shader programs (`fuEnableBinaryShaderProgram`)
- Has VBO/EBO for makeup (`g_makeup_vbo`, `g_makeup_ebo`)
- Has timers for profiling (`timer_makeup_beautifybody`, `timer_copy_tex_mat`)

**Category:** shader, rendering, warp/mesh, confidence HIGH

---

## 6. Bundle & Package Symbols

**From CNamaSDK.dll:**
- `is_controller_resource_bundle`, `bundles`, `CNamaSDK::BundleHelper::DecryptObfuscatedPackage`, `CNamaSDK::BundleHelper::VerifySignature`, `Decrypt bundle failed, error:{}`, `enter DecryptObfuscatedPackage size:{}`, `DecryptObfuscatedPackage Failed!`, `Decrypt and Verify error!`, `is_editor_debug_bundle({})`, `fuRenderBundles_Impl`, `fuRenderBundlesSplitView`, `DoRender bundle name = {}`, `Background segmentation bundle is corrupted.`, `Hair segmentation bundle is corrupted.`, `face recognizer bundle is corrupted.`, `Please load Hair Segmentation AI Bundle`, `Please load Background Segmentation AI Bundle`, `Face Processor bundle is corrupted.`, `FUAI_NewFaceProcessorFromBundleWithConfig`, `Hand Gesture Detection bundle is corrupted.`, `Human Processor bundle is corrupted.`, `FUAI_NewHumanProcessorFromBundleWithConfig`, `Controller::FAvatarSystem::GetBundleBodyPartType`, `Controller::FAvatarSystem::GetBundleResourceType`, `Add DeformationConfig, bundle handle = {}`, `remove DeformationConfig, bundle handle = {}`, `Don't Create controller_cpp.bundle again!!!`, `please create controller_cpp.bundle first`, `Controller::QueryBundleInfo`, `bundle is not controller resource`, `bundle has no name`, `bundle has no handle`, `bundle has no handle`, `bundle( handle = {}, name = {} ), no find in allComponentList`, `please unbind first before destroy, for bundle( handle = {}, name = {}, resource_name = {} )`, `there is a componnet(handle = {}, name = {}, resource_name = {}) in allComponentList having same handle with current bundle`, `please bind Head bundle first`, `get_bundle_type`, `bundle_id`, `bundle_resource_type_map`, `bundle_resource_body_part_type_map`, `no bind head bundle`, `invalid bundle_handle = {}`, `bundle`, `bundle_resource_name`, `invalid json, events::{} has no bundle_resource_name or state`, `invalid json, events::{} invalid bundle_state = {}`, `Controller::FUVAnimationManager::AddBundleUVConfig`, `add bundle, handle = {}`, `same bundle has been added, handle = {}`

**Interpretation:**
- Bundles are encrypted and signed, decrypted via `DecryptObfuscatedPackage`
- Verified via `VerifySignature`
- Types: controller resource bundle vs model bundle
- Controller system with `controller_cpp.bundle`, `DeformationConfig`, `BundleBodyPartType`, `BundleResourceType`
- Avatar system with `FAvatarSystem`
- UV animation with `FUVAnimationManager`
- Resource management with `allComponentList`, handle tracking

**Category:** bundle, confidence HIGH

---

## 7. FUAI & AI Symbols

**From fuai.dll:**
- `use_mesh_deform`, `use_face_meshV2`, `face_meshV2`, `face_meshV2_point_smooth_h`, `Perform SetUseFaceMeshV2.`, `ProcessFacemesh`, `ProcessFacemesh start.`, `ProcessFacemesh end.`, `read armesh_vertices_size error:`, `PTA_NS::BMesh::triangulate_face`, `triangulator failed to split face! (bmesh internal error)`, `bmesh error: infinite loop in disk cycle!`, `PTA_NS::BMesh::BM_face_exists_multi`, `face_meshV2_interface.cc`, `face_meshV2.cc`, `mesh model preprocess timer:`, `mesh model timer:`, `mesh refine model preprocess timer:`, `mesh refine model timer:`, `HaveSameShapes`, `FUAI_ConvertGLToDdeMeshLandmark3ds`, `FUAI_ConvertGLToDdeMeshTriangles`, `FUAI_ConvertGLToDdeMeshVertices`, `FUAI_ConvertGLToDdeMeshVerticesMirror`, `FUAI_FaceProcessorGetArmeshVerticesFromResult`, `FUAI_FaceProcessorGetFaceMeshV2AffineMatrixFromResult`, `FUAI_FaceProcessorGetFaceMeshV2TexCoords`, `FUAI_FaceProcessorGetFaceMeshV2Triangles`, `FUAI_FaceProcessorGetFaceMeshV2VerticesFromResult`, `FUAI_FaceProcessorSetUseFaceMeshV2`, `FUAI_MirrorMeshVertices`, `FaceMeshV2Interface`, `FaceMeshV2`, `BMesh`, `use_motion_controller`, `motion_controller.cc`, `FUAI_HumanRetargeterSetTargetMotionUseMotionController`, `BufferPoolController`, `DummyBufferPoolController`, `SetAvatarAnimFilterParams: please use bundle with keypoint3d ability to use this api!`, `avatar_to_mocap_map_file`, `in mocap_to_avatar_map`, `avatar_to_mocap_map hasn't been initialized, the result will only be rest pose!`, `SetAvatarFixModeTransScale API is deprecated!`, `SetAvatarAnimFilterParams: n_buffer_frames must > 0`, `SetAvatarAnimFilterParams: pos_w must >= 0`, `SetAvatarAnimFilterParams: angle_w must >= 0`, `FUAI_HumanDriverSetAvatarAnimFilterParams`, `FUAI_HumanMocapTransferSetAvatarToMocapNameMap`, `FUAI_HumanProcessorSetAvatarAnimFilterParams`

**Also TFLite related:**
- `Quantization parameters has non-null scale but null zero_point`, `QuantizationParam has %d zero_point values and %d scale values. Must have same number.`, `Invalid sparsity parameter.`, `Tensor %d has invalid quantization parameters.`, `Tensor %d has invalid sparsity parameters.`, `AddNodeWithParameters is disallowed when graph is immutable.`, `SetTensorParametersReadOnly is disallowed when graph is immutable.`, `Encountered Dequantize input with no quant params`, `Encountered Quantize output with no quant params`, `Slice does not support shrink_axis_mask parameter.`, `input_params != nullptr`, `input_params->scale != nullptr`, `reference_ops::AveragePool`, `optimized_ops::AveragePool`, `bias->params.zero_point`, `mul_params.multiplier_exponent_perchannel()`, `params->multiplier_fixedpoint`, `params->multiplier_exponent`, `params->bias`, `params->dilation_height_factor > 0`

**Interpretation:**
- FUAI uses FaceMeshV2 for face mesh generation
- Uses BMesh for triangulation (PTA_NS namespace)
- Supports motion controller, avatar retargeting, BVH
- Uses TFLite for inference (quantization params)
- Has human driver, mocap transfer, mocap collision, retargeter

**Category:** ai, confidence HIGH

---

## 8. OBS Integration Symbols

**From obs-cam-beauty.dll:**
- `obs_module_load`, `obs_module_unload`, `obs_module_name`, `obs_module_description`, `obs_module_ver`, `obs_module_set_pointer`, `obs_module_set_locale`, `obs_module_get_string`, `obs_module_free_locale`
- Small size, imports `obs.dll`, `MSVCP140.dll`, `VCRUNTIME140.dll`
- Build path: `D:\work\obsplus\client\obs-studio-29.0\build\x64\plugins\obsplus\obs-cam-beauty\RelWithDebInfo\obs-cam-beauty.pdb`

**From obsplus.dll:**
- Imports `libcurl.dll`, `obs.dll`, `SHELL32.dll`, `ole32.dll`, `WS2_32.dll`, `WINHTTP.dll`
- Purpose: OBSPlus framework, marketplace, proxy, download

**Category:** bundle? Actually OBS integration, confidence HIGH

---

## 9. Encryption & Security Symbols

**From CNamaSDK.dll:**
- `timer_Decrypt`, `decrypt`, `Decrypt exception:{}`, `Decrypt exception:...`, `Decrypt and Verify error!`, `Decrypt bundle failed, error:{}`, `enter DecryptObfuscatedPackage size:{}`, `CNamaSDK::BundleHelper::DecryptObfuscatedPackage`, `SOFTWARE\Microsoft\Cryptography`, `-----END ENCRYPTED PRIVATE KEY-----`, `-----BEGIN ENCRYPTED PRIVATE KEY-----`, `fu_mbedtls_ssl_decrypt_buf`, `=> encrypt buf`, `no transform provided to encrypt_buf`, `bad record structure provided to encrypt_buf`, `before encrypt: output payload`, `Buffer provided for encrypted record not large enough`, `before encrypt: msglen = %zu, including 0 bytes of padding`, `fu_mbedtls_cipher_auth_encrypt_ext`, `after encrypt: tag`, `No PRNG provided to encrypt_record routine`, `before encrypt: msglen = %zu, including %zu bytes of IV and %zu bytes of padding`, `fu_mbedtls_cipher_crypt`, `using encrypt then mac`, `<= encrypt buf`, `=> decrypt buf`, `bad record structure provided to decrypt_buf`, `fu_mbedtls_cipher_auth_decrypt_ext`, `<= decrypt buf`, `ssl_encrypt_buf`, `ssl_decrypt_buf`, `record type after decrypt (before %d): %d`, `input payload after decrypt`, `DecryptObfuscatedPackage Failed!`, `PKCS#1 encryption :`, `PKCS#1 decryption :`, `md5WithRSAEncryption`, `sha-1WithRSAEncryption`, `sha224WithRSAEncryption`, `sha256WithRSAEncryption`, `sha384WithRSAEncryption`, `sha512WithRSAEncryption`, `rsaEncryption`, `Proc-Type: 4,ENCRYPTED`, `DEK-Info: AES-`, `DEK-Info: AES-128-CBC,`, `DEK-Info: AES-192-CBC,`, `DEK-Info: AES-256-CBC,`, `TLS-ECDHE-ECDSA-WITH-AES-128-CBC-SHA`, etc., `AES-128-ECB`, `AES-192-ECB`, `AES-256-ECB`, `AES-128-CBC`, `AES-192-CBC`, `AES-256-CBC`, `AES-128-CFB128`, `AES-192-CFB128`, `AES-256-CFB128`, `AES-128-OFB`, `AES-192-OFB`, `AES-256-OFB`, `AES-128-CTR`, `AES-192-CTR`, `AES-256-CTR`, `AES-128-XTS`, `AES-256-XTS`, `AES-128-GCM`, `AES-192-GCM`, `AES-256-GCM`, `AES-128-CCM`, `AES-192-CCM`, `AES-256-CCM`, `CryptAcquireContextA`, `CryptReleaseContext`, `CryptGenRandom`

**Interpretation:**
- Uses mbedtls for crypto (AES-CBC, AES-GCM, RSA, etc.)
- Uses Windows Crypto API for random
- Bundle decryption via `DecryptObfuscatedPackage`
- Signature verification
- Private key handling (`ENCRYPTED PRIVATE KEY`)

**Category:** other (security), confidence HIGH for existence, but we do NOT attempt to extract keys

**Compliance:** Marked as PROTECTED for key extraction, we only observe existence

---

## 10. Summary

| Category | Count | Examples | Confidence |
|----------|-------|----------|------------|
| makeup | 150 | MakeupFilterPassNAMA, MakeupWarpNAMA, makeup_intensity, lip_makeup | HIGH |
| beauty | 307 | HeavyBlur, ColorLevel, face_beautification, body_beautify | HIGH |
| face | 1210 | FaceMeshV2, landmark, face_makeup, armesh | HIGH |
| makeup_part | 486 | eye, lip, brow, eyelash, eyeliner, pupil, blush | HIGH |
| texture | 638 | tex_brow, tex_eye, tex_lip, tex_capture, tex_yuv | HIGH |
| mask | 359 | lip_mask, tex_mask, segmentation mask | HIGH |
| warp/mesh | 272 | warp_makeup, g_makeup_vbo, arm mesh | HIGH |
| shader | 384 | GLProgramNew, GLTechnique, CreateProgram, DrawScreenQuad | HIGH |
| bundle | 232 | DecryptObfuscatedPackage, VerifySignature, bundle_resource_name | HIGH |
| parameter | 226 | makeup_intensity, blend_type, is_makeup_on | HIGH |
| ai | 710 | FaceMeshV2, HumanProcessor, HandDetector, TFLite, BMesh | HIGH |
| avatar/animation | 834 | Avatar, animation, controller, DeformationConfig | HIGH |
| filter | 976 | filter, blur, etc. | MEDIUM |
| rendering | 22 | OpenGL, D3D11, D3D12, DirectX | HIGH |
| js | 284 | [js] liufei, stbline, SpriteClip | HIGH |
| other | 3296 | Various | LOW-MEDIUM |

**Total unique relevant strings:** 10,386

**Evidence files:**
- `analysis/string_catalog.json` — full list with binary, offset, string, category, keyword
- `analysis/makeup_params_from_strings.json` — 275 makeup params from strings
- `analysis/cnamasdk_exports_full.json` — 582 exports
- `analysis/cnamasdk_fu_exports.json` — 373 fu*
- `analysis/fuai_exports_full.json` — 597 exports

**No proprietary implementation copied, only names and observed logs.**

---

**End of Symbol Analysis**
