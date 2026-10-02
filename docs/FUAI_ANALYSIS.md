# FUAI Analysis — Phase 1E

**Binary:** `bin/64bit/fuai.dll` (29 MB x64, 18 MB x32)

**Method:** Static PE export table (597 exports, all FUAI_*), strings analysis (710 AI-related strings), no execution, no bypass.

**Total Exports:** 597

---

## 1. Component Overview

FUAI = FaceUnity AI — inference engine for face/human/hand detection, landmarks, mesh, segmentation, beauty, attribute, parsing, retargeting.

**Evidence:**
- Export names: `FUAI_NewHumanDriverFromBundle`, `FUAI_FaceProcessorGetAllLandmarksFromResult`, `FUAI_FaceBeautyProcessorBeauty`, `FUAI_BackgroundSegmenterInference`, etc.
- Strings: `face_meshV2`, `ProcessFacemesh`, `HumanProcessor`, `HandDetector`, `TFLite`, `BMesh::triangulate_face`, `motion_controller.cc`, `avatar_to_mocap_map_file`, etc.
- Build paths: `D:\GitLab-Runner\builds\E3YA_xXv\0\chiliangyang\fuai\fuai\face\face_meshV2\face_meshV2_interface.cc`, `fuai\face\face_meshV2\face_meshV2.cc`, `fuai\human\human_motion\motion_controller.cc`
- TFLite quantization strings: `Quantization parameters has non-null scale but null zero_point`, `Tensor %d has invalid quantization parameters`, `reference_ops::AveragePool`, etc.

---

## 2. Detailed Components

### 2.1 Face Processor

| Component | Purpose | Input | Output | Evidence | Confidence |
|-----------|---------|-------|--------|----------|------------|
| Face Processor | Face detection, landmarks, confidence, face mesh V2, DDE model, expression, eyes rotation, Disney face, tongue, del spot | Image (camera frame) + bundle `ai_face_processor_pc.bundle` | Face rect, landmarks (all), ArMesh vertices, DDE vertices/triangles/texcoords, expression (47/51), eyes rotation, Disney face affine matrix, confidence score, face occlusion, hair mask, head mask, del spot input/output image & matrix | Exports: `FUAI_NewFaceProcessorFromBundle`, `DeleteFaceProcessor`, `GetAllLandmarksFromResult`, `GetArmeshVerticesFromResult`, `GetExpressionFromResult`, `GetConfidenceScoreFromResult`, `GetDdeTriangles`, `GetFaceDdeTexCoords`, `GetEyesRotationFromResult`, `GetDisneyFaceFromResult`, `DelSpotProcess`, `GetDdeNumVertices`, `GetFaceDdeNumVertices`, `GetFaceDdeShortEdgeFov`, `GetFaceCaptureCalibInfoFromResult`, `GetDetectMode`; Strings: `face_meshV2`, `ProcessFacemesh`, `face_meshV2_interface.cc`, `FUAITYPE_FACEPROCESSOR_LIPSOCCUSEGMENT` | HIGH |

**Input:** Camera image (RGBA/YUV), AI face processor bundle (23 MB)

**Output:**
- Face count, rect, confidence
- Landmarks (2D/3D, all)
- Face mesh V2 vertices, triangles, tex coords, affine matrix
- DDE (Dense Deformation Expression?) model vertices, triangles, tex coords, num vertices, short edge FOV
- Expression (47/51 conversion: `ConvertExpression47To51`, `ConvertExpression51To47`)
- Eyes rotation
- Disney face (maybe stylized face for avatar?)
- Face capture calibration info
- Face occlusion, hair mask, head mask
- Del spot (blemish) input image, matrix, output image
- Tongue expression (`ConvertGLToDdeTongueExpression`)

**Evidence:**
- Exports: 30+ FaceProcessor
- Strings: `face_meshV2`, `ProcessFacemesh`, `mesh model preprocess timer`, `BMesh::triangulate_face`, `FUAI_FaceProcessorGetAllLandmarksFromResult`, etc.
- Logs from CNamaSDK: `fuFaceProcessorGetResultHairMask called`, `fuFaceProcessorGetResultHeadMask called`, `fuFaceProcessorSetMinFaceRatio called`, `fuFaceProcessorSetFaceLandmarkQuality`

**Confidence:** HIGH

---

### 2.2 Face Beauty Processor

| Component | Purpose | Input | Output | Evidence | Confidence |
|-----------|---------|-------|--------|----------|------------|
| Face Beauty Processor | Skin beautification: del spot, wrinkle inpaint, even skin, bright eye, teeth seg, skin white mask, blur mask, etc. | Face image + face data (landmarks, rect) + beauty models | Beauty image, blur mask, bright eye mask, dark eye circle masks, decree pattern masks, white LUT, threed red template, del spot result, mole mask, even skin in/out/matrix/rect, skin seg, parsing mask order, small label mask, smart wrinkle removal result, etc. | Exports: `FUAI_NewFaceBeautyProcessorFromBundle` (inferred, but actually `FaceBeautyProcessor` exports), `DeleteFaceBeautyProcessor`, `Beauty`, `EvenSkin`, `FaceDelSpot`, `FaceWrinkleInpaint`, `AutoFaceWrinkleInpaint`, `FaceAcneInpaint`, `FaceMoleFetch`, `GetBeautyImage`, `GetBlurMask`, `GetBrightEyeMask`, `GetDarkEyeCirclHeavyblurRedMask`, `GetSkinWhiteMask`, `GetDelSpotResult`, `GetMoleMaskResult`, `GetEvenSkinInResult`, `GetEvenSkinMatrixResult`, `GetEvenSkinOutResult`, `GetFaceNum`, `GetFaceRect`, `GetLandmarkExt`, `GetLeftIrisLandmark`, `GetRightIrisLandmark`, `GetNoacne`, `GetParsingMaskOrder`, `GetSmallParsingResult`, `GetSmallSkinSegResult`, `GetSmartWrinkleRemovalResult`, `GetSpotMask`, `GetTemplateBrightEyeDarkeyePatternSoftskin`, `GetTemplateThreedRed`, `GetTemplateWhiteLut`, `SmartWrinkleRemoval`, `TeethSeg`, `FaceBeautyRGBA2NV21`, `FaceBeautyResize`, `FreeImageMemory` | HIGH |

**Input:** Face image, landmarks, face rect

**Output:**
- Beauty image (processed)
- Masks: blur, bright eye, dark eye circle (heavyblur, slightblur), decree pattern (heavyblur threed, slightblur), skin white, spot, mole, small label, skin seg, parsing, etc.
- Results: del spot, mole, even skin (in, out, matrix, rect), noacne, small face 5 promask, small rate, smart wrinkle removal, template bright eye/dark eye/pattern/softskin, template threed red, template white LUT
- Face num, rect, landmark ext, left/right iris landmark, aver mask, default beauty image

**Evidence:**
- Exports: 50+ FaceBeautyProcessor
- Strings: `FaceBeautyProcessor` etc.

**Confidence:** HIGH

---

### 2.3 Face Beauty Video Processor

| Component | Purpose | Input | Output | Evidence | Confidence |
|-----------|---------|-------|--------|----------|------------|
| Face Beauty Video Processor | Video version of beauty processor, with even skin and skin seg | Video frames | Even skin input/output image, matrix, rect, skin seg result, face num | Exports: `DeleteFaceBeautyVideoProcessor`, `Process`, `PreviewProcessorProcess`, `GetEvenSkinInputImageFromResult`, `GetEvenSkinMatrixFromResult`, `GetEvenSkinOutputImageFromResult`, `GetEvenSkinRectFromResult`, `GetFaceNum`, `GetSkinSegResult` | HIGH |

**Confidence:** HIGH

---

### 2.4 Background Segmenter

| Component | Purpose | Input | Output | Evidence | Confidence |
|-----------|---------|-------|--------|----------|------------|
| Background Segmenter | Background segmentation (blur, green) | Image + background_blur.bundle (125 KB) | Mask (human vs background) | Exports: `DeleteBackgroundSegmenter`, `Inference`, `InferenceV1`, `GetResultMask`, `Reset`; Strings: `Background segmentation bundle is corrupted.`, `Background segmentation green bundle is corrupted.`, `Please load Background Segmentation AI Bundle`, `Please load Background Segmentation Green AI Bundle`, `new_greensegment_fucreator_1.0.5_release` (log) | HIGH |

**Input:** Camera image, background_blur.bundle

**Output:** Segmentation mask (for blur or green screen)

**Evidence:**
- File `background_blur.bundle` 125KB, magic variant `35 59 CD 0B`
- Log: `created item name: new_greensegment_fucreator_1.0.5_release` — green segmentation creator
- Strings: `Background segmentation bundle is corrupted.`

**Confidence:** HIGH

---

### 2.5 Face Parsing

| Component | Purpose | Input | Output | Evidence | Confidence |
|-----------|---------|-------|--------|----------|------------|
| Face Parsing | Face parsing (skin, hair, eye, lip, etc. categories) | Face image | Parsing mask, cate mask order, cate num, mask scale | Exports: `DeleteFaceParsing`, `Process2Result`, `GetParsingMaskFromResult`, `GetCateMaskOrder`, `GetCateNum`, `GetMaskScale`, `GetCateMaskOrder`, `Reset`, `ResetModules`; | HIGH |

**Input:** Face image

**Output:** Parsing masks for categories (skin, hair, etc.), cate num, mask order, scale

**Confidence:** HIGH

---

### 2.6 Face Attribute Processor

| Component | Purpose | Input | Output | Evidence | Confidence |
|-----------|---------|-------|--------|----------|------------|
| Face Attribute Processor | Age, gender, skin color, nation | Face image | Age, gender, skin color, face num | Exports: `DeleteFaceAttributeProcessor`, `Process`, `GetAge`, `GetGender`, `GetSkinColor`, `GetFaceNum`, `SetNation` | HIGH |

**Confidence:** HIGH

---

### 2.7 Face PTA API Processor

| Component | Purpose | Input | Output | Evidence | Confidence |
|-----------|---------|-------|--------|----------|------------|
| Face PTA API | PTA (maybe face tracking API) | ? | ? | Exports: `DeleteFacePtaApiProcessor`, `DeleteFacePtagResult` | LOW — name only, no details |

**Confidence:** LOW

---

### 2.8 Face Plugins

| Component | Purpose | Input | Output | Evidence | Confidence |
|-----------|---------|-------|--------|----------|------------|
| Face Plugins Makeup Transfer | Makeup transfer (transfer makeup from one face to another?) | ? | ? | Exports: `DeleteFacePluginsMakeupTransferProcessor`, `Process` | MEDIUM |
| Face Plugins Wrinkle Remover | Wrinkle removal | ? | ? | Exports: `DeleteFacePluginsWrinkleRemoverProcessor`, `Process`, `FreeImageMemory` | MEDIUM |

**Confidence:** MEDIUM

---

### 2.9 Hand Processor

| Component | Purpose | Input | Output | Evidence | Confidence |
|-----------|---------|-------|--------|----------|------------|
| Hand Processor | Hand detection, gesture recognition | Image | Hand num, rect, gesture type, score | Exports: `DeleteHandProcessor`, `DeleteHandProcessorResult`, `Process2Result` (inferred), `GetHandNum`, `GetGesture`, `GetScore`, `GetRect`; CNamaSDK: `fuHandDetectorGetResultNumHands called`, `fuHandDetectorGetResultHandRect called`, `fuHandDetectorGetResultGestureType`, `fuHandDetectorGetResultHandScore`; Strings: `HandDetector` | HIGH |

**Input:** Camera image

**Output:** Num hands, hand rect, gesture type, score

**Confidence:** HIGH

---

### 2.10 Human Processor & Human Driver

| Component | Purpose | Input | Output | Evidence | Confidence |
|-----------|---------|-------|--------|----------|------------|
| Human Processor | Human detection, pose, human mask, action | Image + `ai_human_processor_pc.bundle` (42 MB) | Human num, rect, joint 2D/3D, POF joint 2D/scores, human mask, action type/score, BVH motion, human state, gesture types, track ID, FOV | Exports: `DeleteHumanProcessor`, `DeleteHumanProcessorResult`, `Process2Result` (inferred), `GetHumanNum`, `GetJoint2ds`, `GetJoint3ds`; CNamaSDK: `fuHumanProcessorGetNumResults called`, `fuHumanProcessorGetResultRect called`, `fuHumanProcessorGetResultJoint2ds called`, `fuHumanProcessorGetResultJoint3ds called`, `fuHumanProcessorGetResultHumanMask called`, `fuHumanProcessorGetResultActionType called`, `fuHumanProcessorGetResultActionScore called`, `fuHumanProcessorGetResultBVHMotionFrameOutput called`, `fuHumanProcessorGetFov called`, `fuHumanProcessorSetFov`, `fuHumanProcessorSetMaxHumans`, `fuHumanProcessorReset`; FUAI: `FUAI_NewHumanDriverFromBundle`, `HumanDriverProcess2Result`, `GetNumFromResult`, `GetTrackIdFromResult`, `GetPofJoint2dsFromResult`, `GetGestureTypesFromResult`, `GetHumanStateFromResult`, `SetMaxHumans`, `SetFov`, `SetResetEveryNFrames`, `SetSceneState`, `SetUseMocapMode`, `SetUsePtaMode`, `SetAvatarAnimFilterParams` | HIGH |
| Human Driver | Human tracking driver, with mocap mode, PTA mode, avatar anim filter | Image + bundle | Num, track ID, POF joint 2D, gesture types, human state, detection global RTS, gesture scores, POF joint scores | Exports: `FUAI_NewHumanDriverFromBundle`, `DeleteHumanDriver`, `Process2Result`, `GetNumFromResult`, `GetTrackIdFromResult`, `GetPofJoint2dsFromResult`, `GetGestureTypesFromResult`, `GetHumanStateFromResult`, `GetDetectionGlobalRTSFromResult`, `GetGestureScoresFromResult`, `GetPofJointScoresFromResult`, `SetMaxHumans`, `SetFov`, `SetResetEveryNFrames`, `SetSceneState`, `SetUseMocapMode`, `SetUsePtaMode`, `SetAvatarAnimFilterParams`, `SetInternalPoseType`, `SetJointTrackingValidThresholdScale`, `SetJointValidThreshold`, `Reset`, `ResetModules`, `GetFov` | HIGH |

**Input:** Camera image, human processor bundle (42 MB)

**Output:**
- Human num, rect, track ID
- Joint 2D/3D, POF (Part Orientation Field?) joint 2D, scores
- Human mask
- Action type/score
- BVH motion frame output
- Human state, gesture types
- FOV
- Detection global RTS (rotation, translation, scale?)

**Confidence:** HIGH

---

### 2.11 Human Mocap Transfer & Collision

| Component | Purpose | Input | Output | Evidence | Confidence |
|-----------|---------|-------|--------|----------|------------|
| Human Mocap Transfer | Transfer human motion to avatar, with mirror, T-pose bonemap, avatar to mocap name map | Human driver result + bundle | Transform array, model matrix, collision transform array | Exports: `FUAI_NewHumanMocapTransferFromBundle`, `DeleteHumanMocapTransfer`, `Process`, `ProcessOnlyCollision`, `GetResultTransformArray`, `GetResultModelMatrix`, `GetResultCollisionTransformArray`, `SetAvatarToMocapNameMap`, `SetTPoseBonemap`, `SetUseMirror`, `Reset` | HIGH |
| Human Mocap Collision | Collision detection for mocap | ? | Transform array | Exports: `FUAI_NewHumanMocapCollisionFromBundle`, `DeleteHumanMocapCollision`, `Process`, `SetBonemap`, `GetResultTransformArray` | HIGH |

**Confidence:** HIGH

---

### 2.12 Human Retargeter & BVH Retargeter

| Component | Purpose | Input | Output | Evidence | Confidence |
|-----------|---------|-------|--------|----------|------------|
| Human Retargeter | Retarget human motion to UE (Unreal Engine) with local TRS | ? | UE local TRS | Exports: `DeleteHumanRetargeter`, `DeleteHumanRetargeterResult`, `GetTargetUELocalTRSFromResult`, `ProcessForUE2Result` | MEDIUM |
| BVH Retargeter | BVH (Biovision Hierarchy) retargeting, with foot contact, global/local TRS, BVH source/target init | BVH data + target | Global TRS, local TRS, global TRS for UE, local TRS for UE | Exports: `DeleteBVHRetargeter`, `BVHRetargeterInitBVHSource`, `InitTarget`, `Process`, `ProcessWithFootContact`, `GetGlobalTRS`, `GetGlobalTRSForUE`, `GetLocalTRS`, `GetLocalTRSForUE` | HIGH |

**Confidence:** HIGH for BVH, MEDIUM for Human Retargeter

---

### 2.13 Other Processors

| Component | Purpose | Input | Output | Evidence | Confidence |
|-----------|---------|-------|--------|----------|------------|
| Face Recognizer | Face recognition (face ID) | ? | ? | Strings: `face recognizer bundle is corrupted.` | LOW — only string |
| Face Mask Mapper | Face mask mapping | ? | ? | Export: `DeleteFaceMaskMapper`, `FaceMaskMapperProcess` | LOW |
| Photo Editing Processor | Photo editing | ? | ? | Exports: `DeletePhotoEditingProcessorProcessor`, `DeletePhotoEditingProcessorResult` | LOW |
| ISP Bokeh Processor | Bokeh (background blur) via ISP | ? | ? | Exports: `DeleteISPBokehProcessor`, `DeleteISPBokehProcessorResult` | LOW |
| Human Skeleton | Human skeleton bone info | ? | ? | Exports: `DeleteHumanSkeleton`, `DeleteHumanSkeletonBoneInfoArray` | LOW |
| Retarget Name Mapping | Name mapping for retargeting | ? | ? | Export: `DeleteRetargetNameMapping` | LOW |
| Tflite Model | TFLite model handling | ? | ? | Export: `DeleteTfliteModel` | LOW |
| File Buffer / Image View / Camera View | Image view handling, data point, data type, width, height, rot, mode | Image data | ? | Exports: `DeleteFileBuffer`, `DeleteImageView`, `DeleteImageViewMulti`, `DeleteCameraView`, `CameraViewSetDataPoint`, `SetDataType`, `SetWidth`, `SetHeight`, `SetRot`, `SetMode` | MEDIUM |

**Confidence:** LOW-MEDIUM (name only)

---

## 3. TFLite & Model Details

**Evidence of TFLite:**
- Strings: `Quantization parameters has non-null scale but null zero_point`, `QuantizationParam has %d zero_point values and %d scale values. Must have same number.`, `Invalid sparsity parameter.`, `Tensor %d has invalid quantization parameters.`, `Tensor %d has invalid sparsity parameters.`, `AddNodeWithParameters is disallowed when graph is immutable.`, `SetTensorParametersReadOnly is disallowed when graph is immutable.`, `Encountered Dequantize input with no quant params`, `Encountered Quantize output with no quant params`, `Slice does not support shrink_axis_mask parameter.`, `reference_ops::AveragePool`, `optimized_ops::AveragePool`, `bias->params.zero_point`, `mul_params.multiplier_exponent_perchannel()`, `params->multiplier_fixedpoint`, `params->multiplier_exponent`, `params->bias`, `params->dilation_height_factor > 0`

**Interpretation:**
- FUAI uses TFLite for inference
- Models are quantized (scale, zero_point)
- Supports various ops: AveragePool, etc.
- Has sparsity, quantization handling

**Confidence:** HIGH — TFLite used

---

## 4. FaceMeshV2 & BMesh

**Evidence:**
- `face_meshV2`, `face_meshV2_point_smooth_h`, `use_face_meshV2`, `Perform SetUseFaceMeshV2.`, `ProcessFacemesh`, `ProcessFacemesh start.`, `ProcessFacemesh end.`, `read armesh_vertices_size error:`, `PTA_NS::BMesh::triangulate_face`, `triangulator failed to split face! (bmesh internal error)`, `bmesh error: infinite loop in disk cycle!`, `PTA_NS::BMesh::BM_face_exists_multi`, `face_meshV2_interface.cc`, `face_meshV2.cc`, `mesh model preprocess timer:`, `mesh model timer:`, `mesh refine model preprocess timer:`, `mesh refine model timer:`, `HaveSameShapes`, `FUAI_ConvertGLToDdeMeshLandmark3ds`, `FUAI_ConvertGLToDdeMeshTriangles`, `FUAI_ConvertGLToDdeMeshVertices`, `FUAI_ConvertGLToDdeMeshVerticesMirror`, `FUAI_FaceProcessorGetArmeshVerticesFromResult`, `FUAI_FaceProcessorGetFaceMeshV2AffineMatrixFromResult`, `FUAI_FaceProcessorGetFaceMeshV2TexCoords`, `FUAI_FaceProcessorGetFaceMeshV2Triangles`, `FUAI_FaceProcessorGetFaceMeshV2VerticesFromResult`, `FUAI_FaceProcessorSetUseFaceMeshV2`, `FUAI_MirrorMeshVertices`, `FaceMeshV2Interface`, `FaceMeshV2`, `BMesh`

**Interpretation:**
- FaceMeshV2 is second version of face mesh, with point smooth
- Uses BMesh (maybe Blender-like BMesh) for triangulation, with `triangulate_face`, `BM_face_exists_multi`
- Has armesh (AR mesh?) vertices
- Supports mirror
- Has preprocess and refine timers

**Confidence:** HIGH

---

## 5. Motion Controller & Avatar

**Evidence:**
- `use_motion_controller`, `motion_controller.cc`, `FUAI_HumanRetargeterSetTargetMotionUseMotionController`, `BufferPoolController`, `DummyBufferPoolController`, `SetAvatarAnimFilterParams: please use bundle with keypoint3d ability to use this api!`, `avatar_to_mocap_map_file`, `in mocap_to_avatar_map`, `avatar_to_mocap_map hasn't been initialized, the result will only be rest pose!`, `SetAvatarFixModeTransScale API is deprecated!`, `SetAvatarAnimFilterParams: n_buffer_frames must > 0`, `SetAvatarAnimFilterParams: pos_w must >= 0`, `SetAvatarAnimFilterParams: angle_w must >= 0`, `FUAI_HumanDriverSetAvatarAnimFilterParams`, `FUAI_HumanMocapTransferSetAvatarToMocapNameMap`, `FUAI_HumanProcessorSetAvatarAnimFilterParams`

**Interpretation:**
- Supports motion controller for avatar
- Avatar to mocap mapping file
- Anim filter params: n_buffer_frames, pos_w, angle_w
- Fix mode trans scale deprecated

**Confidence:** HIGH

---

## 6. Summary

| Component | Purpose | Input | Output | Confidence |
|-----------|---------|-------|--------|------------|
| Face Processor | Face detection, landmarks, mesh V2, DDE, expression, eyes rotation, Disney face, occlusion, hair/head mask, del spot, tongue | Image + ai_face_processor bundle | Face rect, landmarks, mesh vertices/triangles/UV, DDE, expression, eyes rotation, confidence, occlusion, hair/head mask, del spot image/matrix | HIGH |
| Face Beauty Processor | Skin beautification: del spot, wrinkle, even skin, bright eye, teeth, etc. | Face image + landmarks | Beauty image, masks (blur, bright eye, dark eye, decree, skin white, spot, mole, parsing, etc.), results (del spot, even skin, wrinkle, etc.) | HIGH |
| Face Beauty Video Processor | Video beauty | Video frames | Even skin image/matrix/rect, skin seg, face num | HIGH |
| Background Segmenter | Background segmentation (blur, green) | Image + background_blur bundle | Mask | HIGH |
| Face Parsing | Face parsing (skin, hair, eye, lip) | Face image | Parsing mask, cate num, order, scale | HIGH |
| Face Attribute | Age, gender, skin color, nation | Face image | Age, gender, skin color, face num | HIGH |
| Hand Processor | Hand detection, gesture | Image | Hand num, rect, gesture type, score | HIGH |
| Human Processor / Driver | Human detection, pose, human mask, action, BVH, human state, gesture | Image + ai_human_processor bundle | Human num, rect, joint 2D/3D, POF joint 2D/scores, human mask, action type/score, BVH motion, human state, gesture types, track ID, FOV, global RTS | HIGH |
| Human Mocap Transfer | Transfer human motion to avatar | Human driver result + bundle | Transform array, model matrix, collision transform | HIGH |
| Human Mocap Collision | Collision for mocap | ? | Transform array | HIGH |
| Human Retargeter / BVH Retargeter | Retarget motion to UE, BVH | BVH data + target | Global/local TRS, UE TRS | HIGH/MEDIUM |
| Other | Recognizer, mask mapper, photo editing, bokeh, skeleton, TFLite | ? | ? | LOW |

**Total FUAI exports:** 597

**No proprietary model weights analyzed, only API names and strings.**

---

**End of FUAI Analysis**
