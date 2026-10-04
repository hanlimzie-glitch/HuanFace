# Face Engine Design — HuanFace (Phase 2)

**Status:** SPECIFICATION — no implementation in Phase 2  
**Based on Phase 1 FUAI Analysis:** Face Processor, FaceMeshV2, ProcessFacemesh, BMesh, Face Beauty Processor, Background Segmenter, Face Parsing, Face Attribute, Hand Processor, Human Processor/Driver, etc., but internal model architecture PROTECTED

---

## 1. IFaceTracker Interface (Swappable)

```cpp
enum class HFResult { OK, FAIL, NOT_INITIALIZED, INVALID_PARAM, NOT_SUPPORTED };

struct HFEngineConfig {
    HFRenderBackendType backendType = HFRenderBackendType::AUTO; // D3D11, OPENGL
    void* windowHandle = nullptr; // HWND optional
    int width = 1280;
    int height = 720;
    bool enableDebug = false;
    
    // Face tracking config
    std::string faceTrackerType = "mediapipe"; // "mediapipe", "onnx", "dlib"
    int maxFaces = 4; // from INI BodyNum "Max Faces" — max faces to track
    bool detectSmallFace = false; // from FUAI fuFaceProcessorSetDetectSmallFace
    float minFaceRatio = 0.0f; // from fuFaceProcessorSetMinFaceRatio
    int faceLandmarkQuality = 0; // from fuFaceProcessorSetFaceLandmarkQuality — 0=Standard, 1=High Precision (from INI FaceLandmarkQuality)
    int faceDetectMode = 0; // from fuSetFaceProcessorDetectMode
    bool useAsyncAIInference = false; // from fuSetUseAsyncAIInference
    bool enableFaceMeshV2 = true; // from FUAI use_face_meshV2, face_meshV2_point_smooth_h
};

class IFaceTracker {
public:
    virtual ~IFaceTracker() = default;
    virtual HFResult Init(const HFEngineConfig& config) = 0;
    virtual HFTrackingData Process(const HFFrame& frame) = 0; // frame is CPU data RGBA/RGB
    virtual HFResult Shutdown() = 0;
    
    // Optional: set params
    virtual HFResult SetParam(const std::string& name, float value) = 0;
    virtual HFResult GetParam(const std::string& name, float& outValue) = 0;
};
```

**Why swappable:**
- Different implementations have pros/cons, can be chosen based on platform, performance, accuracy
- FaceUnity's internal implementation PROTECTED, so HuanFace uses open models

---

## 2. Output Structures (PROPOSED, clean-room)

### 2.1 HFFaceData

```cpp
struct HFRect {
    float x, y, width, height; // x,y top-left, width,height size
};

struct HFPoint2D {
    float x, y;
};

struct HFPoint3D {
    float x, y, z;
};

struct HFVec2 {
    float x, y;
};

struct HFVec3 {
    float x, y, z;
};

struct HFVec4 {
    float x, y, z, w;
};

struct HFRotation {
    // Can be quaternion or Euler — use quaternion for stability
    float pitch, yaw, roll; // Euler in degrees or radians? Use radians
    // Or quaternion:
    float qx, qy, qz, qw;
};

struct HFFaceData {
    int id = 0; // tracking ID, consistent across frames for same face
    HFRect bbox; // bounding box
    float confidence = 0.0f; // detection confidence 0-1
    
    // Landmarks — runtime-defined count, not hardcoded 68 or 106 without evidence
    std::vector<HFPoint2D> landmarks; // 2D landmarks in image coordinates (x,y in pixels or normalized 0-1?)
    std::vector<HFPoint3D> landmarks3D; // 3D landmarks (x,y,z) — from FaceMeshV2
    int landmarkCount = 0; // runtime-defined, from tracker implementation
    // Common counts: dlib 68, MediaPipe Face Mesh 468, FaceUnity maybe 75 or 106 (from public docs but not observed in repo, so UNKNOWN, we use runtime-defined)
    
    // Pose
    HFRotation rotation; // pitch, yaw, roll
    HFVec3 translation; // x,y,z translation (maybe in camera space)
    HFVec3 scale; // x,y,z scale
    
    // Mesh (proposed, not FaceUnity mesh which is PROTECTED)
    HFFaceMesh mesh; // vertices, indices, UV, normals, landmark mapping
    
    // Expression (optional, blendshape weights)
    std::vector<float> expression; // blendshape weights, count runtime-defined (47 or 51 from FUAI ConvertExpression47To51)
    int expressionCount = 0;
    
    // Additional (optional, for beauty/makeup)
    float eyeOpenLeft = 0.0f; // eye open ratio 0-1
    float eyeOpenRight = 0.0f;
    float mouthOpen = 0.0f;
    float eyeDistance = 0.0f; // inter-ocular distance
    // etc.
    
    // Face occlusion, hair mask, head mask (from FUAI Face Processor)
    // For HuanFace, we can have masks as HFFrame R8
    HFFrame* faceOcclusionMask = nullptr; // R8 mask, optional
    HFFrame* hairMask = nullptr; // R8
    HFFrame* headMask = nullptr; // R8
};

struct HFTrackingData {
    std::vector<HFFaceData> faces;
    int64_t timestampNanos = 0;
    int faceCount = 0; // faces.size()
};
```

**Landmark count — why runtime-defined:**
- From FUAI: `ConvertExpression47To51` suggests 47 and 51 expression blendshapes, but landmark count not directly observed in repo
- Common open models: MediaPipe Face Mesh 468 points, dlib 68 points, PFLD 98 points, FaceUnity public docs mention 75 or 106 (but not observed in repo, so we mark as UNKNOWN from Phase 1, and use runtime-defined for HuanFace)
- **HuanFace:** `landmarkCount = runtime-defined`, from `IFaceTracker` implementation, not hardcoded 68 or 106 without evidence

**Face occlusion, hair mask, head mask:**
- From FUAI: `fuFaceProcessorGetResultFaceOcclusion`, `GetResultHairMask`, `GetResultHeadMask`, `FUAI_FaceProcessorGetResult` etc.
- For HuanFace: optional `HFFrame*` R8 masks, can be null if not supported by tracker implementation

---

### 2.2 HFFaceMesh (PROPOSED, not FaceUnity mesh which is PROTECTED)

```cpp
struct HFMeshVertex {
    HFVec3 position; // x,y,z
    HFVec3 normal; // nx,ny,nz
    HFVec2 texCoord; // u,v
};

struct HFFaceMesh {
    std::vector<HFMeshVertex> vertices; // positions, normals, UV
    std::vector<int> indices; // triangles, 3 indices per triangle
    std::vector<HFVec2> uv; // alternative UV array if not in vertex
    std::vector<HFVec3> normals; // alternative normals array
    
    // Landmark to mesh mapping: which vertex corresponds to which landmark
    // e.g., landmark 0 (chin) → vertex 123
    std::map<int, int> landmarkToVertex;
    
    // Additional: face mesh V2 with point smooth? From FUAI face_meshV2_point_smooth_h
    // For HuanFace, we can have smoothing param
    bool hasSmooth = false;
    float smoothFactor = 0.0f;
};
```

**FaceUnity mesh: PROTECTED / UNKNOWN — original FaceUnity mesh structure not known, would require decrypted bundle or bypass, which we do NOT do**

**HuanFace mesh: PROPOSED — clean-room design, using open models (MediaPipe Face Mesh 468 points with triangulation) or custom**

**Why mesh needed:**
- Makeup needs mesh to place textures accurately (warp makeup to face)
- Beauty face shape needs mesh to warp face (move vertices)
- From strings: `g_makeup_vbo`, `g_makeup_ebo`, `armesh_vertex_num`, `face_meshV2`, `BMesh::triangulate_face`

---

## 3. Implementations (PROPOSED, not implemented in Phase 2)

### 3.1 MediaPipeFaceTracker (P0 for cross-platform)

**Pros:**
- Apache 2.0 license (open)
- Face Detection (BlazeFace) + Face Mesh (468 points) + Iris (5 points per eye) + Face Geometry
- 468 landmarks, with triangulation (from MediaPipe)
- GPU acceleration (OpenGL, Metal, D3D11 via delegates)
- Well-documented, maintained by Google
- Supports max faces, small face detection, etc.

**Cons:**
- Large model size (~10-15 MB for face mesh)
- Requires MediaPipe framework (C++), more complex integration
- 468 points may be more than needed for makeup (but can be used)

**Input:** HFFrame CPU RGBA/RGB

**Output:** HFFaceData with bbox, confidence, landmarks 468, landmarks3D, mesh vertices 468 + triangulation (from MediaPipe's canonical face model), rotation, translation, scale, expression? MediaPipe has face geometry with pose, but not blendshapes 47/51 — need additional model for expression

**Implementation sketch (not actual code in Phase 2):**

```cpp
class MediaPipeFaceTracker : public IFaceTracker {
    mediapipe::CalculatorGraph graph;
    mediapipe::CalculatorGraphConfig config;
    
    HFResult Init(const HFEngineConfig& config) override {
        // Load MediaPipe graph: face_detection + face_landmark + face_geometry
        // From MediaPipe examples: face_detection, face_landmark, face_geometry calculators
        // Set maxFaces from config.maxFaces
    }
    
    HFTrackingData Process(const HFFrame& frame) override {
        // Convert HFFrame RGBA to MediaPipe ImageFrame
        // Feed to graph, get output: detection + landmarks + geometry
        // Convert to HFFaceData: bbox, landmarks 468, mesh, rotation, etc.
    }
    
    HFResult Shutdown() override {
        // Close graph
    }
};
```

---

### 3.2 ONNXFaceTracker (P1 for Windows, efficient)

**Pros:**
- MIT license (ONNX Runtime)
- Efficient, hardware acceleration (DML for D3D11, CUDA, TensorRT)
- Custom models: SCRFD for face detection (fast, accurate), PFLD or 2D106 for landmarks (98 or 106 points), 3DDFA_V2 for 3D mesh
- Smaller model size than MediaPipe (SCRFD 1-2 MB, PFLD 1-2 MB)
- Can support expression 47/51 via additional models (e.g., 3DDFA_V2 has expression)

**Cons:**
- Need to find/train open models for landmarks and mesh
- More integration work for multiple models (detection + landmark + mesh)
- ONNX Runtime dependency

**Input:** HFFrame CPU RGBA/RGB

**Output:** HFFaceData with bbox, landmarks 98/106, mesh (from 3DDFA_V2 or similar), rotation, expression

**Implementation sketch:**

```cpp
class ONNXFaceTracker : public IFaceTracker {
    Ort::Env env;
    Ort::Session* detectionSession = nullptr; // SCRFD
    Ort::Session* landmarkSession = nullptr; // PFLD
    Ort::Session* meshSession = nullptr; // 3DDFA_V2
    
    HFResult Init(const HFEngineConfig& config) override {
        // Load ONNX models: scrfd.onnx, pfld.onnx, 3ddfa_v2.onnx
        // Create sessions with DML execution provider for D3D11 GPU acceleration
    }
    
    HFTrackingData Process(const HFFrame& frame) override {
        // Detection: run SCRFD on frame, get bboxes
        // For each bbox: crop face, run PFLD for landmarks 98, run 3DDFA_V2 for mesh and pose
        // Convert to HFFaceData
    }
};
```

---

### 3.3 DlibFaceTracker (Fallback, slower)

**Pros:**
- 68 landmarks, well-known, easy integration (single header + cpp)
- No heavy dependencies

**Cons:**
- Slower (HOG + SVM detection, not CNN, or CNN detection but slower than SCRFD/MediaPipe)
- 68 points less than 468 or 106, less accurate for makeup
- No 3D mesh, no expression
- Not GPU accelerated

**Input:** HFFrame CPU RGBA/RGB

**Output:** HFFaceData with bbox, landmarks 68, no mesh (or simple triangulation from 68)

**Use case:** Fallback if MediaPipe and ONNX not available, or for testing

---

### 3.4 Comparison

| Tracker | License | Landmarks | Mesh | Expression | GPU Accel | Model Size | Speed | Accuracy | Pros | Cons |
|---------|---------|-----------|------|------------|-----------|------------|-------|----------|------|------|
| MediaPipe | Apache 2.0 | 468 | Yes (468 + triangulation) | No (need extra) | Yes (OpenGL, Metal, D3D11 via delegates) | ~10-15 MB | Fast (30+ FPS on CPU, 60+ on GPU) | High | Open, well-documented, 468 points, GPU accel | Large, complex integration, no expression |
| ONNX (SCRFD+PFLD+3DDFA_V2) | MIT (ONNX Runtime) + open models | 98/106 | Yes (3DDFA_V2) | Yes (47/51 via 3DDFA_V2) | Yes (DML for D3D11, CUDA) | ~5-10 MB total | Fast (30+ FPS CPU, 60+ GPU) | High | Efficient, small models, DML for D3D11, expression | Need open models, multiple models integration |
| Dlib | Boost | 68 | No (or simple) | No | No | ~100 MB (shape predictor) | Slow (5-10 FPS) | Medium | Easy integration, 68 well-known | Slow, no GPU, no mesh, less accurate |

**Decision for HuanFace (PROPOSED):**
- **P0: MediaPipe** for cross-platform and 468 points (good for makeup)
- **P1: ONNX** for Windows with D3D11 zero-copy and expression support
- **Fallback: Dlib** for testing

**Interface IFaceTracker allows swappable, so application can choose.**

---

## 4. FaceMeshV2 & BMesh (From FUAI Analysis, EXTERNAL REFERENCE)

**From Phase 1 FUAI Analysis (OBSERVED):**
- `face_meshV2`, `face_meshV2_point_smooth_h`, `use_face_meshV2`, `Perform SetUseFaceMeshV2.`, `ProcessFacemesh`, `ProcessFacemesh start.`, `ProcessFacemesh end.`, `read armesh_vertices_size error:`, `PTA_NS::BMesh::triangulate_face`, `triangulator failed to split face! (bmesh internal error)`, `bmesh error: infinite loop in disk cycle!`, `PTA_NS::BMesh::BM_face_exists_multi`, `face_meshV2_interface.cc`, `face_meshV2.cc`, `mesh model preprocess timer:`, `mesh model timer:`, `mesh refine model preprocess timer:`, `mesh refine model timer:`, `HaveSameShapes`, `FUAI_ConvertGLToDdeMeshLandmark3ds`, `FUAI_FaceProcessorGetArmeshVerticesFromResult`, `FUAI_FaceProcessorGetFaceMeshV2AffineMatrixFromResult`, `FUAI_FaceProcessorGetFaceMeshV2TexCoords`, `FUAI_FaceProcessorGetFaceMeshV2Triangles`, `FUAI_FaceProcessorGetFaceMeshV2VerticesFromResult`, `FUAI_FaceProcessorSetUseFaceMeshV2`, `FUAI_MirrorMeshVertices`, `FaceMeshV2Interface`, `FaceMeshV2`, `BMesh`

**Interpretation (INFERRED):**
- FaceMeshV2 is second version of face mesh, with point smooth (face_meshV2_point_smooth_h)
- Uses BMesh (maybe Blender-like BMesh) for triangulation, with triangulate_face, BM_face_exists_multi
- Has armesh (AR mesh?) vertices
- Supports mirror
- Has preprocess and refine timers

**For HuanFace (PROPOSED):**
- HFFaceMesh is clean-room, not FaceUnity mesh (PROTECTED)
- Use MediaPipe's canonical face model triangulation (468 points with 900+ triangles) or custom triangulation from landmarks via BMesh-like algorithm (but clean-room, not copying FaceUnity's BMesh)
- Smoothing via face_meshV2_point_smooth_h evidence — we can have smoothFactor param

**Confidence:** HIGH for FaceMeshV2 existence, MEDIUM for BMesh usage, PROTECTED for exact mesh structure

---

## 5. No OBS Dependency

**Explicitly NOT using:**
- obs.dll
- obsplus.dll
- FaceUnity's FaceMeshV2 implementation (PROTECTED)

**OBS is EXTERNAL REFERENCE only for research (frame flow, etc.), not runtime dependency.**

**HuanFace face tracking dependencies (clean-room, open):**
- MediaPipe (Apache 2.0) or ONNX Runtime (MIT) + open models (SCRFD, PFLD, 3DDFA_V2)
- No proprietary FaceUnity, no OBS

---

---

## Phase 4 Implementation Status

**IMPLEMENTED:**
- IFaceTracker interface real with SimpleFaceTracker heuristic backend
- HFFaceData bbox/landmarks/confidence/rotation/translation/scale/mesh HFFaceMesh vertices/indices/UV
- Runtime-defined landmarks not hardcoded 68/106 (API allows any count, implementation generates 68 for Phase 4 minimal)
- Coordinate system documented origin top-left X right Y down Z out normalized/pixel/UV + landmark->pixel/UV/D3D11 NDC transform + unit test
- FaceMesh 5x5 grid 25 vertices 32 triangles depth variation
- SimpleFaceTracker: skin YCrCb + largest contour BFS + bbox heuristics, confidence 0.3-0.95

**PARTIAL:**
- No MediaPipe/ONNX production, only heuristic
- Only first face used, no tracking ID persistence
- No expression, no 47/51 blendshapes

**STUB:**
- MediaPipeTracker, ONNXTracker still placeholders

**NOT IMPLEMENTED:**
- MediaPipe 468 points, ONNX SCRFD+PFLD+3DDFA_V2, Dlib 68, Kalman tracker, FaceMeshV2 BMesh triangulation production, async inference, GPU accel DML

---

## Phase 5 Implementation Status — Production Tracking & Mesh

**IMPLEMENTED:**
- ProductionFaceTracker architecture IFaceTracker -> ProductionFaceTracker with FaceDetector, LandmarkEstimator, FaceMeshGenerator, PoseEstimator, TemporalTracker + SimpleFaceTracker as prototype/fallback
- IFaceInferenceBackend abstraction HeuristicInferenceBackend (production default real image analysis), ONNXInferenceBackend stub, MediaPipeInferenceBackend stub
- ProductionFaceDetector multi-face YCrCb relaxed + eye/mouth verification + NMS IoU 0.3 + confidence scoring skin*0.3+eye*0.35+mouth*0.2+center*0.1+size*0.05, up to maxFaces 5, multi-face API vector<HFFaceData>
- ProductionLandmarkEstimator real eye darkest weighted 5x5, mouth reddest R-(G+B)/2, nose brightness+center preference, brow dark above eyes, Build68Landmarks anchored to real detections not sin/cos, 68 landmarks + 3D depth + per-landmark confidence
- ProductionFaceMeshGenerator 77 vertices (68 landmarks + 5 forehead +2 cheeks+1 chin+1 nose bridge), indices ~112 triangles following face shape, regions FACE/FOREHEAD/LEFT_EYE/RIGHT_EYE/LEFT_BROW/RIGHT_BROW/NOSE/NOSE_BRIDGE/NOSE_TIP/LIP/OUTER_LIP/INNER_LIP/LEFT_CHEEK/RIGHT_CHEEK/CHIN/JAW, IsValid no NaN indices bounds UV [0,1]
- ProductionPoseEstimator real yaw from nose vs eyes asymmetry ratio*60 + face center offset, pitch from eye-nose vs eye-mouth ratio clamped, roll from eye angle atan2, not hardcoded, ranges yaw [-90,90] pitch [-90,90] roll [-180,180]
- Confidence detection 0.15-0.98, landmark per region, tracking smoothed EMA
- TemporalTracker ID persistence via IoU threshold 0.3, EMA smoothing alpha 0.6 landmarks/pose/confidence, lostFrames max 10, states DETECTED/TRACKED/REAPPEARED/LOST
- Multi-face architecture vector<HFFaceData> 0..N faces, no fake faces, IDs unique
- Mirror/Rotation handling CoordinateTransform Pixel<->Normalized, UV->D3D11 NDC, MirrorHorizontal, RotatePoint 0/90/180/270, TransformLandmarks, RotateBBox, tests normal/rotated/mirrored/rotated+mirrored
- FaceData extended HFFacePose yaw/pitch/roll/tx/ty/tz/scale, HFTrackingState, detectionConfidence/landmarkConfidence/trackingConfidence, landmarkConfidences, landmarksNormalized, pose, trackingState, faceId persistence, coordinate conversions LandmarkToUV, NormalizedToPixel, ApplyMirror, ApplyRotation, UVToD3D11NDC
- Debug visualization --debug-face bbox red landmarks color by region, --debug-landmarks real landmarks, --debug-mesh wireframe white real mesh, --debug-pose yaw yellow pitch cyan + console Face ID/Confidence/Yaw/Pitch/Roll
- basic_face_demo updated to ProductionFaceTracker, console Tracker: ProductionFaceTracker Faces: N Face #0 BBox/Landmarks/Detection/Landmark/Tracking Confidence Pose Yaw/Pitch/Roll Mesh Vertices/Triangles Processing time
- Tests 15/15 PASS: Production Tracker 30 checks, Face Mesh 17, Pose 14, Tracking 24, Coordinate 22, plus existing 10 suites
- Performance measured 256x256 total ~2.8ms ~350 FPS, 512x512 ~6.9ms, 720p ~15ms, 1080p ~32ms, harness exists
- Windows requirement CPU heuristic works Windows, ONNX DirectML optional, Windows runtime validation NOT EXECUTED in Arena Linux sandbox, code Windows-compatible

**PARTIAL:**
- Heuristic not ML, may fail complex backgrounds, no ONNX model bundled, no MediaPipe, no Kalman, no appearance re-ID

**NOT IMPLEMENTED:**
- ONNX Runtime with SCRFD+PFLD+3DDFA_V2 models bundled, MediaPipe 468, 3DMM, SolvePnP, Kalman filter, face recognition re-ID, GPU DirectML acceleration in sandbox

**End of Face Engine Design**
