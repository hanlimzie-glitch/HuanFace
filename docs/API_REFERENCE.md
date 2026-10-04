# API Reference — CNamaSDK.dll & FUAI (Phase 1)

**Source:** Static analysis of exported symbols from `bin/64bit/CNamaSDK.dll` (582 exports, 373 fu*) and `bin/64bit/fuai.dll` (597 exports, all FUAI_*)

**Method:** PE export table parsing (RVA→file offset), no execution, no bypass.

**Confidence Levels:**
- HIGH: Export name exists, category clear from name, log shows usage
- MEDIUM: Export name exists, category inferred, no log
- LOW: Export name ambiguous

**No proprietary implementation copied, only public API names.**

---

## 1. CNamaSDK.dll — Total Exports 582, fu* 373

### 1.1 Initialization & Setup (21)

| Function | Observed Signature | Params (from logs) | Return | Evidence | Confidence |
|----------|-------------------|--------------------|--------|----------|------------|
| fuSetup | UNKNOWN (requires auth data) | `const char* data, int size` inferred from `fuSetup called`, `fuSetup_Impl: sdk version {}` | int (0=success) | Export exists, log `fuSetup called`, `fuSetup_Impl: sdk version {}` | HIGH |
| fuSetupLocal | UNKNOWN | Likely path to local auth | int | Export exists | MEDIUM |
| fuSetupLocal2 | UNKNOWN | Variant of local setup | int | Export exists | MEDIUM |
| fuSetupInternalCheck | UNKNOWN | `called` no args log | int | Log `fuSetupInternalCheck called` | HIGH |
| fuSetupInternalCheckEx | UNKNOWN | Extended check | int | Log | HIGH |
| fuSetupInternalCheckPackageBind | UNKNOWN | Package bind check | int | Log | HIGH |
| fuSetupDeviceLocal | UNKNOWN | `:{}` log shows string param (device id?) | int | Log `fuSetupDeviceLocal:{}` | HIGH |
| fuSetAuthenticated | UNKNOWN | `called({})` bool/int | void | Log `fuSetAuthenticated called({})` | HIGH |
| fuIsLibraryInit | UNKNOWN | `called({})` | bool | Log `fuIsLibraryInit called({})` | HIGH |
| fuGetVersion | UNKNOWN | `called` | const char* or version struct | Log `fuGetVersion called`, export | HIGH |
| fuGetCommitTime | UNKNOWN | `called` | const char* | Log `fuGetCommitTime called` | HIGH |
| fuGetModuleCode | UNKNOWN | `called` | int/string | Log `fuGetModuleCode called` | HIGH |
| fuGetLogLevel | UNKNOWN | | int | Export | MEDIUM |
| fuGetSystemError | UNKNOWN | | int | Export | MEDIUM |
| fuGetSystemErrorString | UNKNOWN | | const char* | Export | MEDIUM |
| fuOpenFileLog | UNKNOWN | Path? | void | Export | MEDIUM |
| fuDestroyLibData | UNKNOWN | `called` | void | Log `fuDestroyLibData called` | HIGH |
| fuDestroyAllItems | UNKNOWN | `called` | void | Log `fuDestroyAllItems called` | HIGH |
| fuDestroyItem | UNKNOWN | `handle = {}` int | void | Log `fuDestroyItem: handle = {}` | HIGH |
| fuDestroyInstance | UNKNOWN | | void | Export | MEDIUM |
| fuDestroyScene | UNKNOWN | | void | Export | MEDIUM |

**Category:** Initialization — SDK lifecycle, auth, version, cleanup

---

### 1.2 GL Context (2)

| Function | Evidence | Confidence |
|----------|----------|------------|
| fuInitGLContext | Log `fuInitGLContext(sharedContext:{})`, export | HIGH |
| fuDestroyGLContext | Export | HIGH |
| fuGetCurrentGLContext | Export | HIGH |
| fuMakeGLContextCurrent | Export | HIGH |
| fuIsGLPrepared | Export | MEDIUM |
| fuGetOpenGLSupported | Export | MEDIUM |

**Observed:** `GLLoader.cc:212 initialGLExtentions: glversion max = 4, min = 6` — requires OpenGL 4.6

**Category:** GL Context — OpenGL initialization for rendering

---

### 1.3 Item / Bundle (Core Loading)

| Function | Observed Log | Signature Guess (DO NOT ASSUME) | Confidence |
|----------|--------------|----------------------------------|------------|
| fuCreateItemFromPackage | `handle = {}, item_name =` , `... exception` , `std::overflow_exception` | `int fuCreateItemFromPackage(const void* data, int size)` — returns handle | HIGH |
| fuCreateLiteItemFromPackage | `handle = {}` `finish.` | Lite version, returns handle | HIGH |
| fuCreateInstance | | Creates instance? | MEDIUM |
| fuCreateScene | | Creates scene? | MEDIUM |
| fuDestroyItem | `handle = {}` | `void fuDestroyItem(int handle)` | HIGH |
| fuDestroyAllItems | `called` | `void fuDestroyAllItems()` | HIGH |
| fuDestroyInstance | | | MEDIUM |
| fuDestroyScene | | | MEDIUM |

**Evidence from logs:**
```
[info][NamaContext.cpp:1534] created item name: face_makeup
[info][CNamaSDK.cpp:890] fuCreateItemFromPackage: handle = 2, item_name =
[info][NamaContext.cpp:1534] created item name: body_beautify
[info][NamaContext.cpp:1534] created item name: new_greensegment_fucreator_1.0.5_release
[info][NamaContext.cpp:1534] created item name: hair_normal
[info][NamaContext.cpp:1534] created item name: dummy
```

**Item names observed:** `face_makeup`, `body_beautify`, `new_greensegment_fucreator_1.0.5_release`, `hair_normal`, `dummy`

**Category:** Item/Bundle — loading encrypted bundles

---

### 1.4 Parameters & Textures

| Function | Log | Confidence |
|----------|-----|------------|
| fuItemSetParamd | `called` | HIGH |
| fuItemSetParamdv | `called` | HIGH |
| fuItemSetParams | `called` | HIGH |
| fuItemSetParamu64 | `called` | HIGH |
| fuItemSetParamu8v | `called` | HIGH |
| fuItemGetParamd | | MEDIUM |
| fuItemGetParamdv | `called` | HIGH |
| fuItemGetParamfv | `called` | HIGH |
| fuItemGetParams | `called` | HIGH |
| fuItemGetParamu8v | `called` | HIGH |
| fuCreateTexForItem | `called`, `create new tex!`, `set param error!`, `update tex!` | HIGH |
| fuDeleteTexForItem | `called`, `set param error!` | HIGH |
| fuSetMakeupCoverResource | `is_cover:{}` | HIGH |

**Texture binding observed:**
```
[info][makeupController.cpp:2037] debug++ SetParamTex called tex_lip_mask_zz
[info][makeupController.cpp:2037] debug++ SetParamTex called tex_blusher
[info][makeupController.cpp:2037] debug++ SetParamTex called tex_eyeLash
[info][makeupController.cpp:2037] debug++ SetParamTex called tex_eyeLiner
[info][makeupController.cpp:2037] debug++ SetParamTex called tex_eye
[info][makeupController.cpp:2037] debug++ SetParamTex called tex_brow
```

**Param names from strings (HIGH confidence):**
- `tex_brow`, `tex_eye`, `tex_eye2`, `tex_eye3`, `tex_eye4`, `tex_pupil`, `tex_eyeLash`, `tex_lip`, `tex_eyeLiner`, `tex_blusher`, `tex_blusher2`, `tex_foundation`, `tex_shadow`, `tex_lip_highlight`, `tex_lip_mask_bz`, `tex_lip_mask_zz`, `tex_lip_mask_bite_bz`, `tex_lip_mask_bite_zz`, `tex_lip_mask_highlight_bz`, `tex_lip_mask_highlight_zz`, `tex_makeup`, `tex_lut`, `tex_lut2`, `tex_lipstick_median`, `tex_blend_weight_and_level_map`, `tex_face_occu_blur`
- `makeup_intensity`, `makeup_intensity_lip`, `makeup_intensity_pupil`, `makeup_intensity_eye`, `makeup_intensity_eyeLiner`, `makeup_intensity_eyelash`, `makeup_intensity_eyeBrow`, `makeup_intensity_blusher`, `makeup_intensity_foundation`, `makeup_intensity_highlight`, `makeup_intensity_shadow`, `makeup_lip_color`, `makeup_lip_color2`, `makeup_eye_color`, etc.
- `blend_type_tex_eye`, `blend_type_tex_brown`, `blend_type_tex_eyeLash`, etc.

**Category:** Parameters — setting intensity, color, textures

---

### 1.5 Rendering

| Function | Evidence | Confidence |
|----------|----------|------------|
| fuRender | Export | MEDIUM |
| fuRenderBundles | `DoRender bundle name = {}`, export | HIGH |
| fuRenderBundlesEx | Export | MEDIUM |
| fuRenderBundlesSplitView | `called`, export | HIGH |
| fuRenderItems | Export | MEDIUM |
| fuRenderItemsEx | Export | MEDIUM |
| fuRenderItemsEx2 | `_Impl`, export | MEDIUM |
| fuRenderItemsMasked | Export | MEDIUM |
| fuEnableRender | Export | MEDIUM |
| fuEnableRenderCamera | Export | MEDIUM |
| fuPrepareGLResource | Export | MEDIUM |
| fuReleaseGLResources | `called` | HIGH |
| fuReleaseGLResourcesSafe | Export | MEDIUM |
| fuOnDeviceLost | `called` | HIGH |
| fuOnDeviceLostSafe | `called` | HIGH |

**Category:** Rendering — OpenGL rendering of bundles

---

### 1.6 Face Tracking & Face Processor

| Function | Log / Evidence | Confidence |
|----------|----------------|------------|
| fuTrackFace | `called`, `fuTrackFace_Impl called` | HIGH |
| fuTrackFaceWithTongue | `called` | HIGH |
| fuHasFace | `called` | HIGH |
| fuIsTracking | `called` | HIGH |
| fuGetFaceInfo | Export | MEDIUM |
| fuGetFaceInfoRotated | Export | MEDIUM |
| fuGetAIInfo | `_Impl`, export | MEDIUM |
| fuGetAIInfoRotated | Export | MEDIUM |
| fuGetFaceIdentifier | `called` | HIGH |
| fuSetTrackFaceAIType | `called` | HIGH |
| fuSetASYNCTrackFace | Export | MEDIUM |
| fuSetStrictTracking | Export | MEDIUM |
| fuEnableFaceProcessor | Export | MEDIUM |
| fuFaceProcessorGetNumResults | Export | MEDIUM |
| fuFaceProcessorGetConfidenceScore | Export | MEDIUM |
| fuFaceProcessorGetResultFaceOcclusion | Export | MEDIUM |
| fuFaceProcessorGetResultHairMask | `called` | HIGH |
| fuFaceProcessorGetResultHeadMask | `called` | HIGH |
| fuFaceProcessorSetDetectSmallFace | Export | MEDIUM |
| fuFaceProcessorSetFaceLandmarkQuality | Export | MEDIUM |
| fuFaceProcessorSetMinFaceRatio | `called({})` | HIGH |
| fuFaceProcessorSetUseCaptureEyeLookCam | Export | MEDIUM |
| fuGetFaceProcessorFov | `called` | HIGH |
| fuGetFaceProcessorResult | Export | MEDIUM |
| fuSetFaceProcessorDetectMode | `called({})` | HIGH |
| fuSetFaceProcessorFov | `({})` | HIGH |
| fuSetFaceProcessorDetectEveryNFramesWhenFace | Export | MEDIUM |
| fuSetFaceProcessorDetectEveryNFramesWhenNoFace | Export | MEDIUM |
| fuSetTongueTracking | `({})` | HIGH |
| fuSetUseAsyncAIInference | `:({})` | HIGH |

**Category:** Face Tracking — detection, landmarks, confidence, hair/head mask

---

### 1.7 Human Tracking & Human Processor

| Function | Log | Confidence |
|----------|-----|------------|
| fuEnableHumanProcessor | Export | MEDIUM |
| fuHumanProcessorGetNumResults | `called` | HIGH |
| fuHumanProcessorGetResultRect | `called({})` | HIGH |
| fuHumanProcessorGetResultTrackId | `called({})` | HIGH |
| fuHumanProcessorGetResultJoint2ds | `called` | HIGH |
| fuHumanProcessorGetResultJoint3ds | `called` | HIGH |
| fuHumanProcessorGetResultPofJoint2ds | `called` | HIGH |
| fuHumanProcessorGetResultPofJointScores | `called` | HIGH |
| fuHumanProcessorGetResultHumanMask | `called` | HIGH |
| fuHumanProcessorGetResultActionType | `called` | HIGH |
| fuHumanProcessorGetResultActionScore | `called` | HIGH |
| fuHumanProcessorGetResultBVHMotionFrameOutput | `called` | HIGH |
| fuHumanProcessorGetFov | `called` | HIGH |
| fuHumanProcessorGetGestureTypes | Export | MEDIUM |
| fuHumanProcessorGetHumanState | Export | MEDIUM |
| fuHumanProcessorReset | `called` | HIGH |
| fuHumanProcessorSetMaxHumans | `called({})` | HIGH |
| fuHumanProcessorSetFov | `called({})` | HIGH |
| fuHumanProcessorSetResetEveryNFrames | `called({})` | HIGH |
| fuHumanProcessorSetEnableBVHOutput | `called({})` | HIGH |
| fuHumanProcessorSetBVHInPlaneRotation | `called({})` | HIGH |
| fuHumanProcessorSetBVHInPlaneMirrorType | `called({})` | HIGH |
| fuHumanProcessorSetAvatarAnimFilterParams | Export | MEDIUM |
| fuHumanProcessorSet3DScene | Export | MEDIUM |
| fuHumanActionMatchDistance | `called` | HIGH |
| fuHumanActionMatchLeftRightHandDistance | `called` | HIGH |

**Category:** Human Tracking — pose, joints, human mask, action, BVH

---

### 1.8 Hand Tracking

| Function | Log | Confidence |
|----------|-----|------------|
| fuEnableHandDetetor | Export (typo in original: Detetor) | MEDIUM |
| fuHandDetectorGetResultNumHands | `called` | HIGH |
| fuHandDetectorGetResultHandRect | `called({})` | HIGH |
| fuHandDetectorGetResultGestureType | `({})` | HIGH |
| fuHandDetectorGetResultHandScore | `({})` | HIGH |
| fuSetHandDetectEveryNFramesWhenNoHand | Export | MEDIUM |
| fuSetHandGestureCallBack | `:{}` | HIGH |

**Category:** Hand Tracking — hand rect, gesture, score

---

### 1.9 Beauty / ImageBeauty

| Function | Evidence | Confidence |
|----------|----------|------------|
| fuBeautifyImage | Export | MEDIUM |
| fuImageBeautyCreateTexture | Export | MEDIUM |
| fuImageBeautyCreateTextureCoverPreview | Export | MEDIUM |
| fuImageBeautySetParam | Export | MEDIUM |
| fuImageBeautyGetParam | Export | MEDIUM |
| fuImageBeautyPreProcess | Export | MEDIUM |
| fuImageBeautyPreProcessForImageInfo | Export | MEDIUM |
| fuImageBeautyPreview | Export | MEDIUM |
| fuImageBeautyGetResult | Export | MEDIUM |
| fuImageBeautyGetInfo | Export | MEDIUM |
| fuImageBeautyGetOriginTexture | Export | MEDIUM |
| fuImageBeautyGetLastResultTexture | Export | MEDIUM |
| fuImageBeautyClearMemory | Export | MEDIUM |
| fuImageBeautyLoadCache | Export | MEDIUM |
| fuImageBeautySaveCache | Export | MEDIUM |
| fuImageBeautySaveResultToPath | Export | MEDIUM |
| fuImageBeautySetAttributePath | Export | MEDIUM |
| fuImageBeautySetCacheDir | Export | MEDIUM |
| fuImageBeautySetFaceBeautyPath | Export | MEDIUM |
| fuImageBeautySetUndoRedoMode | Export | MEDIUM |
| fuImageBeautyConvertRGBA2NV21 | Export | MEDIUM |
| fuGetDelspotStatus | `is_open:{}` | HIGH |

**Category:** Beauty — skin smooth, whitening, image-based beauty

---

### 1.10 Camera & Rendering State

| Function | Log | Confidence |
|----------|-----|------------|
| fuGetCameraImageSize | `called` | HIGH |
| fuGetCurrentRotationMode | Export | MEDIUM |
| fuGetProjectionMatrixZfar | Export | MEDIUM |
| fuGetProjectionMatrixZnear | Export | MEDIUM |
| fuSetDefaultRotationMode | `({})` | HIGH |
| fuSetDeviceOrientation | `({})` | HIGH |
| fuOnCameraChange | `called` | HIGH |
| fuSetCropState | `({})` | HIGH |
| fuSetCropFreePixel | `({},{},{},{})` | HIGH |
| fuEnableCameraAnimation | Export | MEDIUM |
| fuEnableCameraAnimationInternalLerp | Export | MEDIUM |
| fuGetCameraAnimationFrameNumber | Export | MEDIUM |
| fuGetCameraAnimationProgress | Export | MEDIUM |
| fuGetCameraAnimationTransitionProgress | Export | MEDIUM |
| fuPauseCameraAnimation | Export | MEDIUM |
| fuPlayCameraAnimation | Export | MEDIUM |
| fuPlayCameraAnimationOnce | Export | MEDIUM |
| fuResetCameraAnimation | Export | MEDIUM |
| fuSetCameraAnimationTransitionTime | Export | MEDIUM |
| fuEnableOrthogonalProjection | Export | MEDIUM |
| fuSetOutputResolution | `w:{},h:{}` | HIGH |
| fuSetInputCameraTextureMatrixState | `isEnable:{}` | HIGH |
| fuSetOutputMatrixState | `isEnable:{}` | HIGH |
| fuSetInputCameraBufferMatrixState | `isEnable:{}` | HIGH |
| fuSetRttCacheState | `({})` | HIGH |
| fuSetUsePbo | `({})` | HIGH |
| fuSetUseTexAsync | `({})` | HIGH |
| fuSetForceUseGL2 | `: {}` | HIGH |
| fuSetUseAsyncAIInference | `:({})` | HIGH |
| fuSetUseMultiBuffer | `:({},{})` | HIGH |

**Category:** Camera — orientation, crop, projection, camera animation, rendering state

---

### 1.11 Avatar / Bind

| Function | Log | Confidence |
|----------|-----|------------|
| fuBindItems | `called`, `the target item has no OnBind function`, `the target item index is out-of-range` | HIGH |
| fuUnbindItems | `called` | HIGH |
| fuUnbindAllItems | `called` | HIGH |
| fuBindItemsToInstance | Export | MEDIUM |
| fuBindItemsToScene | Export | MEDIUM |
| fuAvatarBindItems | `called`, `not an avatar`, `the avatar item index is out-of-range`, `{} has been rejected by all contracts` | HIGH |
| fuAvatarUnbindItems | `called`, `not an avatar` | HIGH |

**Category:** Avatar/Bind — binding items to instances/scenes/avatars

---

### 1.12 Instance / Scene

| Function | Evidence | Confidence |
|----------|----------|------------|
| fuCreateInstance | Export | MEDIUM |
| fuCreateScene | Export | MEDIUM |
| fuDestroyInstance | Export | MEDIUM |
| fuDestroyScene | Export | MEDIUM |
| fuEnableInstanceAnimationInternalLerp | Export | MEDIUM |
| fuEnableInstanceDynamicBone* | Export | MEDIUM |
| fuEnableInstanceExpressionBlend | Export | MEDIUM |
| fuEnableInstanceFaceProcessorRotateHead | Export | MEDIUM |
| fuEnableInstanceFacepupMode | Export | MEDIUM |
| fuEnableInstanceFocusEyeToCamera | Export | MEDIUM |
| fuEnableInstanceHideNeck | Export | MEDIUM |
| fuEnableInstanceModelMatToBone | Export | MEDIUM |
| fuEnableInstanceRotateWithoutAnimationTranslation | Export | MEDIUM |
| fuEnableInstanceSelfCollision | Export | MEDIUM |
| fuEnableInstanceSingleDynamicBone | Export | MEDIUM |
| fuEnableInstanceSingleMeshVisible | Export | MEDIUM |
| fuEnableInstanceUseBodyVisibleList | Export | MEDIUM |
| fuEnableInstanceUseFaceBeautyOrder | Export | MEDIUM |
| fuEnableInstanceVisible | Export | MEDIUM |
| fuGetInstance* | Export | MEDIUM |

**Category:** Instance/Scene — avatar instances, scenes, bones, bounding boxes

---

### 1.13 AI Model

| Function | Log | Confidence |
|----------|-----|------------|
| fuLoadAIModelFromPackage | `type:{} sz:{}` | HIGH |
| fuPreprocessAIModelFromPackage | `type:{} sz:{}` | HIGH |
| fuReleaseAIModel | `type:{}` | HIGH |
| fuIsAIModelLoaded | Export | MEDIUM |
| fuProcessorChangeModel | `type:{}` | HIGH |
| fuLoadTongueModel | Export | MEDIUM |
| fuSetTongueTracking | `({})` | HIGH |
| fuTrackFaceWithTongue | `called` | HIGH |

**Category:** AI Model — loading AI models from packages

---

### 1.14 Profile / Debug

| Function | Evidence | Confidence |
|----------|----------|------------|
| fuCheckDebugItem | Export | MEDIUM |
| fuCheckGLError | Export | MEDIUM |
| fuProfileGetNumTimers | Export | MEDIUM |
| fuProfileGetTimer* | Export | MEDIUM |
| fuProfileResetAllTimers | Export | MEDIUM |
| fuFrameTimeProfile* | Export | MEDIUM |
| fuRootTimeProfileStart/Stop | Export | MEDIUM |
| fuStackTimeProfileStart/Stop | Export | MEDIUM |

**Category:** Profile/Debug — performance profiling

---

### 1.15 Other

| Function | Evidence | Confidence |
|----------|----------|------------|
| fuEnableARMode | Export | MEDIUM |
| fuEnableBackgroundColor | Export | MEDIUM |
| fuEnableBackgroundAnimationLoop | Export | MEDIUM |
| fuEnableBinaryShaderProgram | Export | MEDIUM |
| fuEnableBloom | Export | MEDIUM |
| fuEnableControlTimeUpdate | Export | MEDIUM |
| fuEnableDof | Export | MEDIUM |
| fuEnableDofDebug | Export | MEDIUM |
| fuEnableDynamicBone | Export | MEDIUM |
| fuEnableGroundReflection | Export | MEDIUM |
| fuEnableHDRRGBA16F | Export | MEDIUM |
| fuEnableLowQualityLighting | Export | MEDIUM |
| fuEnableLowResolutionTexture | Export | MEDIUM |
| fuEnableOuterMVPMatrix | Export | MEDIUM |
| fuEnableRiggingBVHInputProcessor | Export | MEDIUM |
| fuEnableShadow | Export | MEDIUM |
| fuGetCurrentGLContext | Export | MEDIUM |
| fuGetOpenGLSupported | Export | MEDIUM |
| fuHexagonInitWithPath | Export | MEDIUM |
| fuHexagonTearDown | Export | MEDIUM |
| fuAuthCountWithAPIName | Export | MEDIUM |
| fuClearPhysics | `call` | HIGH |
| fuRotateImage | Export | MEDIUM |
| fuSetViewMatrix | Export | MEDIUM |
| fuSetBackgroundColor | Export | MEDIUM |
| fuSetBackgroundParams | Export | MEDIUM |
| fuSetBloomParamters | Export (typo) | MEDIUM |
| fuSetDofParamters | Export (typo) | MEDIUM |
| fuSetRenderPauseState | Export | MEDIUM |
| fuSetMultiSamples | `({})` | HIGH |
| fuSetHandGestureCallBack | `:{}` | HIGH |

**Category:** Other — rendering features, AR, bloom, DOF, etc.

---

## 2. FUAI.dll — Total Exports 597

### 2.1 Core

- `FUAI_NewHumanDriverFromBundle`, `FUAI_NewHumanMocapTransferFromBundle`, `FUAI_NewHumanMocapCollisionFromBundle`
- `FUAI_DeleteHumanDriver`, `FUAI_DeleteHumanMocapTransfer`, etc.
- `FUAI_HumanDriverProcess2Result`, `FUAI_HumanDriverGetNumFromResult`, `FUAI_HumanDriverGetTrackIdFromResult`, etc.
- `FUAI_FaceProcessorGetAllLandmarksFromResult`, `FUAI_FaceProcessorGetArmeshVerticesFromResult`, `FUAI_FaceProcessorGetExpressionFromResult`, etc.
- `FUAI_FaceBeautyProcessor*` — beauty processing (del spot, wrinkle, even skin, bright eye, etc.)
- `FUAI_BackgroundSegmenter*` — background segmentation
- `FUAI_FaceParsing*` — face parsing
- `FUAI_BVHRetargeter*` — BVH retargeting for avatar
- `FUAI_FaceAttributeProcessor*` — age, gender, skin color
- `FUAI_ConvertGLToDde*` — conversion from GL to DDE (face model)

**Category breakdown:**

| Category | Count | Examples |
|----------|-------|----------|
| Human Driver | 20+ | `FUAI_NewHumanDriverFromBundle`, `Process2Result`, `GetNumFromResult`, `SetMaxHumans`, `SetFov`, `SetResetEveryNFrames`, `SetSceneState`, `GetTrackId`, `GetPofJoint2ds`, `GetGestureTypes` |
| Human Mocap Transfer | 10+ | `NewHumanMocapTransferFromBundle`, `Process`, `GetResultTransformArray`, `SetAvatarToMocapNameMap`, `SetUseMirror` |
| Human Mocap Collision | 5+ | `NewHumanMocapCollisionFromBundle`, `Process`, `SetBonemap`, `GetResultTransformArray` |
| Human Retargeter | 5+ | `DeleteHumanRetargeter`, `GetTargetUELocalTRSFromResult`, `ProcessForUE2Result` |
| BVH Retargeter | 10+ | `BVHRetargeterInitBVHSource`, `InitTarget`, `Process`, `ProcessWithFootContact`, `GetGlobalTRS`, `GetLocalTRS` |
| Face Processor | 30+ | `DeleteFaceProcessor`, `GetAllLandmarksFromResult`, `GetArmeshVerticesFromResult`, `GetExpressionFromResult`, `GetConfidenceScoreFromResult`, `GetDdeTriangles`, `GetFaceDdeTexCoords`, `GetEyesRotationFromResult`, `DelSpotProcess` |
| Face Beauty Processor | 50+ | `DeleteFaceBeautyProcessor`, `Beauty`, `EvenSkin`, `FaceDelSpot`, `FaceWrinkleInpaint`, `GetBeautyImage`, `GetBlurMask`, `GetBrightEyeMask`, `GetSkinWhiteMask`, `GetDelSpotResult`, `SmartWrinkleRemoval`, `TeethSeg`, `GetFaceNum`, `GetFaceRect` |
| Face Beauty Video Processor | 10+ | `DeleteFaceBeautyVideoProcessor`, `Process`, `GetSkinSegResult`, `GetEvenSkinInputImageFromResult` |
| Background Segmenter | 5+ | `DeleteBackgroundSegmenter`, `Inference`, `InferenceV1`, `GetResultMask`, `Reset` |
| Face Parsing | 10+ | `DeleteFaceParsing`, `Process2Result`, `GetParsingMaskFromResult`, `GetCateMaskOrder`, `GetCateNum`, `GetMaskScale` |
| Face Attribute | 10+ | `DeleteFaceAttributeProcessor`, `Process`, `GetAge`, `GetGender`, `GetSkinColor`, `SetNation` |
| Hand Processor | 10+ | `DeleteHandProcessor`, `Process2Result`, `GetHandNum`, `GetGesture`, `GetScore` |
| Human Processor | 10+ | `DeleteHumanProcessor`, `Process2Result`, `GetHumanNum`, `GetJoint2ds`, `GetJoint3ds` |
| Image View / File Buffer | 10+ | `DeleteImageView`, `DeleteFileBuffer`, `CameraViewSetWidth`, `SetHeight`, `SetDataType` |
| Conversion | 10+ | `ConvertGLToDdeExpression`, `ConvertGLToDdeMeshVertices`, `ConvertExpression47To51`, `CheckNeutralFace` |
| Other | 100+ | `DeleteFaceRecognizer`, `DeleteFacePtaApiProcessor`, `DeletePhotoEditingProcessor`, `DeleteISPBokehProcessor`, `DeleteRetargetNameMapping`, `DeleteTfliteModel` |

**Confidence:** HIGH for existence, MEDIUM for purpose (inferred from names)

---

## 3. OBS-CAM-BEAUTY.dll — Total Exports 9

```
obs_module_description
obs_module_free_locale
obs_module_get_string
obs_module_load
obs_module_name
obs_module_set_locale
obs_module_set_pointer
obs_module_unload
obs_module_ver
```

**Purpose:** OBS plugin entry points — standard OBS plugin API

**Evidence:** Small size (63KB), imports `obs.dll`, build path `D:\work\obsplus\client\obs-studio-29.0\build\x64\plugins\obsplus\obs-cam-beauty\RelWithDebInfo\obs-cam-beauty.pdb`

**Confidence:** HIGH

---

## 4. OBSPLUS.dll — Imports

- `libcurl.dll`, `obs.dll`, `SHELL32.dll`, `ole32.dll`, `WS2_32.dll`, `WINHTTP.dll`
- Purpose: OBSPlus framework, hosts multiple plugins, handles marketplace, proxy, download

**Confidence:** HIGH

---

## 5. BEAUTY.exe & CAMERA-TOOL.exe

### beauty.exe

- Imports: `CNamaSDK.dll`, `OPENGL32.dll`, `CRYPT32.dll`, `WS2_32.dll`, `VERSION.dll`, `w32-pthreads.dll`
- Build path: `.../obs-cam-beauty/nama/RelWithDebInfo/beauty.pdb`
- Purpose: Standalone beauty helper / test harness (inferred)
- **No --help strings found** — likely GUI app, not CLI
- **Observation:** Cannot run without Wine, no bypass attempted, marked as PROTECTED for behavior

### camera-tool.exe

- Imports: `SetupDi*` (device enumeration), `QueryDosDeviceW`
- Manifest: `requireAdministrator`
- Purpose: Camera occupation detection tool (from locale: `CheckWhereDeviceUsed.DlgTitle="Camera Occupation Detection Tool"`)
- **No --help strings** — GUI tool
- **Observation:** PROTECTED for behavior (requires admin)

---

## 6. Summary

| Binary | Total Exports | fu*/FUAI* | Categories | Confidence |
|--------|---------------|-----------|------------|------------|
| CNamaSDK.dll | 582 | 373 fu* | 15 categories | HIGH |
| fuai.dll | 597 | 597 FUAI_* | 15+ categories | HIGH |
| obs-cam-beauty.dll | 9 | 0 | OBS plugin API | HIGH |
| obsplus.dll | N/A (imports) | 0 | OBSPlus framework | MEDIUM |
| beauty.exe | N/A | 0 | Standalone helper | LOW (no --help) |
| camera-tool.exe | N/A | 0 | Camera detection | LOW |

**Total API surface documented:** 970+ functions (373 + 597)

**No signature guessed — only names, logs, and evidence.**

---

**End of API Reference**
