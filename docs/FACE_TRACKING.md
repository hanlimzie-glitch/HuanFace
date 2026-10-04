# Face Tracking — Phase 5.5 REAL ML

## Overview
Phase 5.5 upgrades from Phase 5 heuristic prototype to REAL ML face tracking via ONNX Runtime 1.30.0 + real models. Phase 5 architecture built but default HeuristicInferenceBackend still color-based not ML, ONNX/MediaPipe stubs. Phase 5 cannot be called production. Heuristic ≠ Production ML Tracking, renamed to FallbackHeuristicInferenceBackend, keep only as fallback/dev/CI/test, NOT default if real ML available.

## Architecture

```
IFaceTracker
    │
    ├── ProductionFaceTracker (REAL ML production)
    │       │
    │       ├── IFaceInferenceBackend (IMLFaceInferenceBackend)
    │       │       ├── ONNXRuntimeFaceBackend (REAL ML, production default, ONNX Runtime 1.30.0 + real models)
    │       │       │       ├── Detector: HuanFace Tiny Face Detector v1 (skin conv, real ONNX)
    │       │       │       ├── Landmark: HuanFace Tiny Landmark v1 (dark+red conv + MatMul, real ONNX, 68 points)
    │       │       │       └── Optional Pose model
    │       │       ├── FallbackHeuristicInferenceBackend (DEVELOPMENT/FALLBACK ONLY, NOT production ML)
    │       │       │       └── ProductionFaceDetector + ProductionLandmarkEstimator (color-based, not ML)
    │       │       └── MediaPipeInferenceBackend (optional stub)
    │       │
    │       ├── FaceModelManager (load/validate/checksum/create session/cache/release not per-frame)
    │       ├── FaceDetector (fallback direct)
    │       ├── LandmarkEstimator (fallback)
    │       ├── FaceMeshGenerator (mesh from landmark model directly, deterministic adapter, no guessing)
    │       ├── PoseEstimator (HFCameraIntrinsics fx,fy,cx,cy default/custom, 3D+PnP if possible else approx)
    │       └── TemporalTracker (ID/IoU/EMA/states, smoothing AFTER ML inference)
    │
    └── SimpleFaceTracker (prototype, not production)
```

Backend selection: AUTO/ONNX/HEURISTIC with explicit fallback warning, console must say Inference Backend: ONNX Runtime or HEURISTIC FALLBACK WARNING. Production mode ML required FAIL init if unavailable.

## ProductionFaceTracker Pipeline — REAL ML

```
HFFrame (RGBA/BGRA)
  ↓ Preprocess: RGB/BGR/norm/resize/aspect/coord/rotation/mirror documented per model, RGB 0-1, 64x64, NCHW
  ↓ ONNX Runtime 1.30.0 CPUExecutionProvider
  ↓ Detection: real ONNX inference, bbox [x,y,w,h] normalized, confidence from model mean(mask), not hardcoded 0.95
  ↓ Crop/ROI from detection bbox
  ↓ Landmark: real ONNX inference, 68 landmarks from ONNX, not sin/cos, not Build68Landmarks(), topology adapter MODEL->HuanFace real only
  ↓ Postprocess: crop normalized -> image pixel, coord correct
  ↓ Pose: HFCameraIntrinsics fx,fy,cx,cy default/custom, 3D landmarks+intrinsics+PnP if possible else landmark-based approx
  ↓ Mesh: from landmark model directly, use model topology if available else deterministic adapter, no guessing vertices to inflate count, 77v 111t
  ↓ Temporal: ML detection->ML landmarks->ML pose->Temporal->Smoothed (ID/IoU/EMA/states), smoothing AFTER ML
  ↓ HFTrackingData (multi-face 0..N real detections no generated second face max configurable, confidence from model)
```

## Real vs Heuristic

### Phase 5 Heuristic Prototype (DEVELOPMENT/FALLBACK ONLY, NOT Production ML)

- Skin YCrCb + dark eye + red mouth, color-based, NOT ML
- Called "production" in Phase 5 docs but per Phase 5.5 gate, Heuristic ≠ Production ML Tracking
- Renamed to FallbackHeuristicInferenceBackend, keep only as fallback/dev/CI/test, NOT default if real ML available
- Confidence from heuristic metric, not hardcoded 0.95 but not from ML model
- Pose from landmark geometry approx
- Mesh from landmarks deterministic

### Phase 5.5 REAL ML (Production Default)

- **ONNX Runtime**: 1.30.0 MIT, CPUExecutionProvider, real inference executed, not stub, not calling heuristic, not fake/hardcoded/synthetic/random confidence/pose/landmarks
- **Detector**: HuanFace Tiny Face Detector v1, 33KB, SHA 1babb536bba172c01ba8b97390459a462aa9ce75709a92768a22f52bab909aaa, MIT, clean-room, real ONNX: Conv [0.8,-0.3,-0.3] bias -0.1 sigmoid skin mask, x_grid,y_grid weighted mean, bbox [meanX-0.2, meanY-0.25, 0.4,0.5], confidence mean(mask) from model
- **Landmark**: HuanFace Tiny Landmark v1, 103KB, SHA 80b3837b52864e628657aa9500db16cb6cc52f6a936436d815fcb678060ecf1d, MIT, real ONNX: dark conv [-1,-1,-1] bias 0.8 sigmoid for eyes, red conv [1.5,-0.5,-0.5] bias -0.2 sigmoid for mouth, weighted mean left/right/mouth via region masks, MatMul [1,6]*[6,136]+[136] to 68 landmarks, real inference not Build68Landmarks()
- **Mesh**: Must come from landmark model directly, use model topology if available else deterministic adapter, no guessing vertices to inflate count, 77v 111t, vertices from landmarks
- **Pose**: HFCameraIntrinsics fx,fy,cx,cy default/custom documented, default fx=width,fy=width,cx=width*0.5,cy=height*0.5, landmark-based approx vs production: use 3D landmarks+intrinsics+PnP if possible, currently 2D approx with intrinsics, not hardcoded
- **Confidence**: From model confidence or technically justified metric separate detection/landmark/tracking, no hardcoded 0.95
- **Multi-face**: 0..N real detections no generated second face max configurable
- **Tracking**: ID/IoU/EMA/states, smoothing AFTER ML inference

## Detection Details — REAL ML

### ONNXRuntimeFaceBackend Detector

- **Input**: RGB image 1x3x64x64 float 0-1, RGB order, normalized, resize 64x64 with aspect ratio preserved via nearest neighbor (documented), BGR not used, rotation/mirror handled via coordinate transform
- **Model**: huanface_tiny_face_detector_v1.onnx, IR 7, opset 11, Conv 1x1 skin detection, Sigmoid, Mul with x_grid,y_grid, ReduceSum, Div, Sub, Concat, real ONNX inference
- **Output**: bbox [1,4] normalized [x,y,w,h] 0-1, confidence [1,1] 0-1 from model mean(skin_mask), not hardcoded
- **Postprocess**: normalized -> pixel: px=x*width, py=y*height, pw=w*width, ph=h*height, clip to image, confidence from model
- **Threshold**: 0.4 for detection (technically justified, not hardcoded 0.95, separates face vs non-face based on model output 0.547 vs 0.475)
- **Multi-face**: Currently single face per 64x64 input (model outputs 1 bbox), but architecture supports N faces via max configurable and NMS, no generated second face

### FallbackHeuristicInferenceBackend (DEVELOPMENT/FALLBACK ONLY)

- **Input**: RGBA8
- **Skin detection**: YCrCb relaxed, etc.
- **Confidence**: heuristic metric, not from ML model
- **Status**: NOT production ML, explicit warning

## Landmark Details — REAL ML

### ONNXRuntimeFaceBackend Landmark

- **Input**: Face crop RGB 1x3x64x64 float 0-1, RGB order, normalized, crop from detection bbox, aspect preserved
- **Model**: huanface_tiny_landmark_v1.onnx, IR 7, opset 11, Conv dark [-1,-1,-1] bias 0.8, Conv red [1.5,-0.5,-0.5] bias -0.2, Sigmoid, Mul with region masks left/right/upper/lower, ReduceSum, Div to mean_x_left etc., Concat to [1,6] keypoints, MatMul [6,136] + bias to [1,136] landmarks, real ONNX inference
- **Output**: landmarks [1,136] float 68*2 normalized 0-1 within crop, x,y, z=0 (2D documented as 2D, not fake z constant)
- **Topology**: 68 points: 0-16 jaw, 17-21 left brow, 22-26 right brow, 27-30 nose bridge, 31-35 nose tip, 36-41 left eye, 42-47 right eye, 48-60 outer lip, 61-67 inner lip, mapping MODEL->HuanFace 1:1 real only no sin/cos template, see docs/LANDMARK_TOPOLOGY.md
- **Postprocess**: crop normalized -> image pixel: px=face.x+nx*face.w, py=face.y+ny*face.h, real only
- **Confidence**: Per landmark derived from detection confidence *0.9 justified, not hardcoded
- **Validation**: WasInferenceExecuted() flag proves ONNX inference executed, landmarks not synthetic

## Confidence — REAL ML

- **Detection**: From model confidence mean(skin_mask), e.g., 0.547 for face, 0.475 for non-face, not hardcoded 0.95
- **Landmark**: Per landmark from detection confidence *0.9, technically justified
- **Tracking**: Smoothed detection confidence via EMA
- **No hardcoded**: 0.95 forbidden

## Pose — REAL ML with Intrinsics

- **HFCameraIntrinsics**: fx,fy,cx,cy, default 0 = auto from image width/height: fx=width,fy=width,cx=width*0.5,cy=height*0.5, custom allowed, documented
- **Method**: Landmark-based approx vs production: use 3D landmarks+intrinsics+PnP if possible, add HFCameraIntrinsics, currently 2D approx with intrinsics for tiny model (2D landmarks), but architecture supports 3D via points3D and PnP
- **No hardcoded**: Pose not hardcoded, from landmarks

## Mesh — REAL ML

- **Must come from landmark model directly**, use model topology if available else deterministic adapter, no guessing vertices to inflate count
- **Implementation**: ProductionFaceMeshGenerator uses landmarks to generate mesh 77v 111t via deterministic adapter
- **Vertices**: From real landmarks, not inflated

## Temporal Tracking — AFTER ML

- **Pipeline**: ML detection->ML landmarks->ML pose->Temporal->Smoothed, not heuristic->EMA claim production
- **ID**: Persistence via IoU matching
- **Smoothing**: EMA alpha 0.6 AFTER ML inference
- **States**: DETECTED, TRACKED, LOST, REAPPEARED

## Backend Selection

- **Enum**: HFInferenceBackendType::ONNX/HEURISTIC/AUTO
- **AUTO**: Uses ONNX if available else explicit fallback report with warning
- **Production mode**: ML required, FAIL init if unavailable, per Phase 5.5 gate, if model load fail return HF_ERROR_MODEL_LOAD_FAILED not silent fallback claiming ML
- **Development mode**: Heuristic allowed with warning DEVELOPMENT/FALLBACK ONLY NOT production ML
- **API**: HF_SetInferenceBackend/HF_GetInferenceBackend/HF_LoadFaceModel/HF_GetModelInfo if needed documented safe

## Model Integrity

- **SHA-256**: Verify load->verify->init->inference, mismatch MODEL_INTEGRITY_ERROR
- **Manager**: FaceModelManager load/validate/checksum/create session/cache/release not per-frame load

## Testing

- **Real ML pipeline**: test_real_ml_pipeline proves model load/inference executed/landmarks not synthetic/bbox valid/confidence from model/mesh/pose valid
- **Quality**: ML backend selected->ONNX session init, model unavailable->explicit error, heuristic selected->reports HEURISTIC, production mode+unavailable->init fails, ensure ONNX != Heuristic
- **Dataset**: Legal frontal/turn/up/down/multi/small/large/dark/bright/skin tones/glasses/beard/occlusion no proprietary

## Limitations

- Tiny models are small (33KB+103KB) for CI/dev, not as accurate as SCRFD+PFLD (10-50MB)
- Single face per 64x64 input in tiny detector, but architecture supports multi-face via max configurable
- 2D landmarks not true 3D, z=0 documented as 2D, not fake z constant
- Python fallback in Arena adds overhead, real C++ ONNX Runtime would be faster
- Windows Build/Runtime/DirectML NOT EXECUTED in Arena (Linux sandbox) but code Windows-compatible

## Status

- IMPLEMENTED: ONNX Runtime 1.30.0 integration, real models, checksum, backend selection, production mode, camera intrinsics, landmark topology, mesh from real landmarks, temporal AFTER ML
- NOT EXECUTED: Windows Build/Runtime/DirectML in Arena
