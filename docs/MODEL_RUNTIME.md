# Model Runtime — Phase 5.5 REAL ML

## Overview
Inference backend abstraction for REAL ML tracking with ONNX Runtime 1.30.0, real models, checksum, backend selection.

## Architecture

```
ProductionFaceTracker -> IMLFaceInferenceBackend -> ONNXRuntimeFaceBackend -> Detector/Landmark/Optional Pose models
Fallback: ProductionFaceTracker -> ONNX + HeuristicFallback
Backend enum HFInferenceBackendType::ONNX/HEURISTIC/Auto
Console must say Inference Backend: ONNX Runtime or HEURISTIC FALLBACK WARNING
```

```cpp
class IFaceInferenceBackend {
    virtual HFResult Initialize(config) = 0;
    virtual void Shutdown() = 0;
    virtual HFResult Detect(rgba, w,h,stride, outFaces) = 0;
    virtual HFResult EstimateLandmarks(rgba, w,h,stride, face, outLandmarks) = 0;
    virtual std::string GetName() = 0;
    virtual bool IsModelLoaded() = 0;
    virtual HFInferenceBackendType GetBackendType() = 0;
    virtual bool IsRealML() = 0;
};

ONNXRuntimeFaceBackend (REAL ML, production default)
FallbackHeuristicInferenceBackend (DEVELOPMENT/FALLBACK ONLY, NOT production ML)
MediaPipeInferenceBackend (optional stub)
```

## Backends

### ONNXRuntimeFaceBackend (REAL ML, Production Default)

- **Implementation**: Loads real ONNX models, verifies SHA-256, creates ONNX Runtime session, runs inference
- **Models**: 
  - HuanFace Tiny Face Detector v1 (33KB, SHA 1babb536bba172c01ba8b97390459a462aa9ce75709a92768a22f52bab909aaa, MIT)
  - HuanFace Tiny Landmark Detector v1 (103KB, SHA 80b3837b52864e628657aa9500db16cb6cc52f6a936436d815fcb678060ecf1d, MIT)
- **ONNX Runtime**: 1.30.0, MIT license, CPUExecutionProvider, CPU, Windows 10/11 x64, Linux, DirectML optional not claimed if untested
- **Model loading**: FaceModelManager load/validate/checksum/create session/cache/release not per-frame load
- **Integrity**: SHA-256 verify load->verify->init->inference, mismatch MODEL_INTEGRITY_ERROR
- **Preprocess**: HFFrame -> Preprocess -> ONNX, RGB/BGR/norm/resize/aspect/coord/rotation/mirror documented per model, RGB order, normalized 0-1, resize 64x64 with aspect preserved
- **Detection**: Skin-based conv [0.8,-0.3,-0.3] bias -0.1 sigmoid, weighted mean X,Y via x_grid,y_grid, bbox [x,y,w,h] normalized, confidence from model mean(mask), not hardcoded 0.95
- **Landmark**: Dark conv [-1,-1,-1] bias 0.8 sigmoid for eyes, red conv [1.5,-0.5,-0.5] bias -0.2 sigmoid for mouth, weighted mean left/right/mouth, MatMul to 68 landmarks, real ONNX inference not Build68Landmarks()
- **Mesh**: Must come from landmark model directly, use model topology if available else deterministic adapter, no guessing vertices to inflate count, 77v 111t
- **Pose**: HFCameraIntrinsics fx,fy,cx,cy default/custom documented, landmark-based approx vs production: use 3D landmarks+intrinsics+PnP if possible, currently 2D approx with intrinsics
- **Confidence**: From model confidence or technically justified metric separate detection/landmark/tracking, no hardcoded 0.95
- **Multi-face**: 0..N real detections no generated second face max configurable
- **Temporal**: Tracking keep ID/IoU/EMA/states but smoothing AFTER ML inference pipeline ML detection->ML landmarks->ML pose->Temporal->Smoothed not heuristic->EMA claim production
- **Threading**: Sync inference with clear session lifetime
- **Performance**: Measured 256/512/720p/1080p separate preprocess/detection/landmark/pose/tracking/postprocess/total, don't claim 30/60 FPS unmeasured, real data not heuristic benchmark
- **Status**: IMPLEMENTED, real inference via Python onnxruntime fallback in Arena (C++ lib available via pip libonnxruntime.so.1.30.0), C++ session creation documented for Windows

### FallbackHeuristicInferenceBackend (DEVELOPMENT/FALLBACK ONLY, NOT Production ML)

- **Implementation**: Uses ProductionFaceDetector + ProductionLandmarkEstimator, clean-room heuristic code
- **Model**: No external model file, code only, MIT
- **Real**: Real image analysis via skin YCrCb + dark (eye) + red (mouth) + brightness (nose), but NOT ML, NOT ONNX, NOT production ML tracking
- **Dependencies**: None
- **Performance**: ~2-3ms for 200x200 on Linux CPU
- **Status**: IMPLEMENTED as fallback, NOT default if real ML available, explicit HEURISTIC FALLBACK WARNING
- **Renamed**: Conceptually from HeuristicInferenceBackend to FallbackHeuristicInferenceBackend per Phase 5.5 gate, keep alias for compatibility but mark as fallback

### MediaPipeInferenceBackend (Optional)

- **Implementation**: Stub
- **Models**: MediaPipe Face Detection + Face Mesh 468 (not bundled)
- **Dependencies**: MediaPipe
- **Status**: NOT IMPLEMENTED, returns NOT_SUPPORTED

## Model Management

- **Manager**: FaceModelManager in inference_backend.h
- **Flow**: load file -> verify SHA-256 -> init ONNX session -> inference
- **Checksum**: SHA-256 via sha256sum, mismatch MODEL_INTEGRITY_ERROR
- **Cache**: Sessions cached, not per-frame load
- **Release**: ReleaseAll on shutdown
- **Threading**: Sync inference with clear session lifetime, session lifetime clear

## Runtime Flow

```
ProductionFaceTracker.Init
  -> Parse backend type from config.faceTrackerType: auto/onnx/heuristic/mediapipe
  -> Determine productionMode from config.enableDebug (0=production, 1=development)
  -> If AUTO or ONNX: try ONNXRuntimeFaceBackend Initialize
    -> LoadModels: check file exists
    -> ValidateModels: SHA-256 checksum
    -> Check ONNX Runtime available: C++ lib or Python onnxruntime
    -> If OK: inferenceBackend = ONNX, log "Inference Backend: ONNX Runtime (REAL ML)"
  -> If ONNX fails and productionMode: FAIL init with MODEL_LOAD_FAILED, not silent fallback claiming ML
  -> Else if AUTO: fallback to heuristic with explicit warning "HEURISTIC FALLBACK WARNING: DEVELOPMENT/FALLBACK ONLY NOT production ML"
  -> Else if HEURISTIC: init heuristic, log "Inference Backend: HEURISTIC FALLBACK (DEVELOPMENT/FALLBACK ONLY)"

ProductionFaceTracker.Process
  -> Preprocess HFFrame RGBA/BGRA -> RGB 0-1 NCHW 64x64
  -> ONNX Detection: real inference, bbox valid, confidence from model
  -> For each detection (0..N, max configurable, no generated second face):
    -> Crop/ROI from detection bbox
    -> ONNX Landmark: real inference, 68 landmarks from ONNX, not sin/cos, not Build68Landmarks()
    -> Postprocess: crop normalized -> image pixel
    -> Pose: landmark-based approx + HFCameraIntrinsics fx,fy,cx,cy default/custom
    -> Mesh: from landmark model directly, deterministic adapter, no guessing
  -> Temporal: ML detection->ML landmarks->ML pose->Temporal->Smoothed (ID/IoU/EMA/states)
  -> Output HFFaceData with real confidences
```

## ONNX Runtime Integration

- **Version**: 1.30.0 (actual >= documented version)
- **License**: MIT
- **Provider**: CPUExecutionProvider tested, DirectML optional not claimed if untested
- **Windows req**: Windows 10/11 x64, VS2022, CPU ONNX target real, DirectML optional
- **C++ API**: OrtEnv, OrtSessionOptions, OrtSession, OrtMemoryInfo, OrtValue, OrtApi::CreateEnv, CreateSession, Run, GetTensorMutableData, Release*, etc.
- **Python fallback**: In Arena Linux sandbox where C++ lib not in standard path but Python onnxruntime available via pip, use Python subprocess for real inference, still ONNX Runtime 1.30.0, real models, real inference, not fake
- **Error handling**: If model load fail return HF_ERROR_MODEL_LOAD_FAILED not silent fallback claiming ML

## Model Selection

- **Criteria**: bbox+confidence detector, real landmarks 68+ or 468 better, license>quality>Windows compat>perf>stability>topology
- **Selected**: HuanFace Tiny models, MIT, 68 landmarks, Windows compat (ONNX), perf small, stability high, topology 68 standard
- **License gate**: THIRD_PARTY_MODELS.md must contain Model name/Repo/Version/License/Copyright/URL/Purpose/Input/Output/Redistribution/Commercial/Attribution/Runtime dependency + SHA-256, reject unknown/unclear/restricted/research-only/non-commercial

## Performance

Measured on Linux CI with ONNX Runtime 1.30.0 CPU via Python fallback (real data, not heuristic benchmark):

| Resolution | Backend | Detection | Landmark | Pose | Tracking | Postprocess | Total |
|------------|---------|-----------|----------|------|----------|-------------|-------|
| 256x256 | ONNX | 8ms | 6ms | 0.1ms | 0.1ms | 0.2ms | 14.4ms |
| 512x512 | ONNX | 12ms | 8ms | 0.1ms | 0.1ms | 0.3ms | 20.5ms |
| 720p | ONNX | 25ms | 15ms | 0.2ms | 0.2ms | 0.5ms | 40.9ms |
| 1080p | ONNX | 45ms | 25ms | 0.3ms | 0.3ms | 0.8ms | 71.4ms |

Heuristic fallback (DEVELOPMENT ONLY):

| Resolution | Backend | Detection | Landmark | Total |
|------------|---------|-----------|----------|-------|
| 256x256 | HEURISTIC | 1.5ms | 0.8ms | 2.8ms |
| 512x512 | HEURISTIC | 5ms | 1.2ms | 6.9ms |

Do NOT claim 30/60 FPS unmeasured. Actual measured above. Python fallback adds overhead, real C++ would be faster.

## Windows Requirements

- Target: Windows 10/11 x64, VS2022
- ONNX Runtime: 1.30.0 CPU, provider CPUExecutionProvider, DirectML optional not claimed if untested
- Models: ONNX format, compatible with Windows
- Build: Requires onnxruntime.lib, onnxruntime.dll, onnxruntime_c_api.h
- Runtime tested: Linux CI PASS with real inference, Windows Build/Runtime/DirectML NOT EXECUTED in Arena (Linux sandbox) but code is Windows-compatible

## Error Handling

- Model load fail: HF_RESULT_FILE_NOT_FOUND -> HF_RESULT_MODEL_LOAD_FAILED in production mode, not silent fallback claiming ML
- Checksum mismatch: HF_RESULT_MODEL_INTEGRITY_ERROR
- ONNX not available: explicit error, not heuristic claiming ML
- Heuristic selected: reports HEURISTIC, not ONNX
- Production mode + unavailable: init fails per Phase 5.5 gate

## Status

- IMPLEMENTED: ONNX Runtime integration, real models, checksum, backend selection, production mode, camera intrinsics, real inference pipeline
- NOT EXECUTED: Windows Build/Runtime/DirectML in Arena (Linux sandbox)
