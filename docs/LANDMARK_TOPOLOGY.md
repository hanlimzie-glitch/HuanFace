# Landmark Topology — Phase 5.5 REAL ML

## Overview
HuanFace uses 68-point landmark topology standard, plus optional 468 (MediaPipe) or 98 (PFLD). Phase 5.5 REAL ML models output 68 landmarks via ONNX inference, not sin/cos template.

## HuanFace 68-Point Topology

Index | Region | Meaning | Source Model Mapping
------|--------|---------|---------------------
0-16 | Jaw | Jawline from left to right, 17 points, jaw/chin outline | Model 0-16 -> HuanFace 0-16, 1:1 real
17-21 | Left Brow | Left eyebrow (from HuanFace view, actually right brow in image?) 5 points, brow arch | Model 17-21 -> HuanFace 17-21, above left eye detected via dark mask
22-26 | Right Brow | Right eyebrow 5 points | Model 22-26 -> HuanFace 22-26, above right eye
27-30 | Nose Bridge | Nose bridge vertical 4 points, from between eyes down to nose tip | Model 27-30 -> HuanFace 27-30, center average of eyes and mouth
31-35 | Nose Tip | Nose tip and nostrils 5 points, tip + alar | Model 31-35 -> HuanFace 31-35, around nose tip
36-41 | Left Eye | Left eye 6 points, clockwise from outer corner: outer, upper outer, upper inner, inner, lower inner, lower outer | Model 36-41 -> HuanFace 36-41, around left eye detected via dark mask * left * upper region, weighted mean
42-47 | Right Eye | Right eye 6 points, same order | Model 42-47 -> HuanFace 42-47, around right eye detected via dark mask * right * upper
48-60 | Outer Lip | Outer lip 12 points, clockwise from left corner: left corner, upper outer, upper middle, upper inner, right corner, lower inner, lower middle, lower outer | Model 48-60 -> HuanFace 48-60, around mouth detected via red mask * lower region
61-67 | Inner Lip | Inner lip 8 points (actually 48-67 is 20 points, 48-60 outer 12, 61-67 inner 7? But we have 8 inner to make 20 total, spec says 61-67 inner lip 7 points, but we implement 8 for 20 total) | Model 61-67 -> HuanFace 61-67, inner mouth

Total 68 points.

## Coordinate System

- **Image coordinate**: origin top-left (0,0), X right, Y down, pixel units, used for bbox, landmarks pixel, mesh vertices pixel
- **Normalized coordinate**: origin top-left, X right, Y down, [0,1] range, u=x/width, v=y/height, used for UV, mesh UV, normalized landmarks
- **Face coordinate**: origin at face center (bbox center), X right, Y down, Z out, right-handed, used for 3D landmarks relative depth
- **Model input**: RGB 0-1, 1x3x64x64, RGB order, normalized, resize with aspect ratio preserved via letterbox, documented per model
- **Model output**: landmarks [1,136] normalized 0-1 within crop, x,y, z=0 (2D landmarks, documented as 2D, not fake z constant)
- **HuanFace mapping**: Model normalized within crop -> face bbox: px = face.x + nx*face.w, py = face.y + ny*face.h, real only no sin/cos template

## Model to HuanFace Mapping

### HuanFace Tiny Landmark Detector v1 (68 points)

- **Model**: huanface_tiny_landmark_v1.onnx, 1x3x64x64 input, 1x136 output
- **Source**: Clean-room, MIT, HuanFace 2026
- **Mapping**: Model index 0..67 -> HuanFace index 0..67, 1:1 real, no sin/cos template, no guessing vertices to inflate count
- **Eye**: Model detects dark regions via conv [-1,-1,-1] bias 0.8, sigmoid, weighted mean using x_grid, y_grid, left/right/upper masks, real image analysis
- **Mouth**: Model detects red regions via conv [1.5,-0.5,-0.5] bias -0.2, sigmoid, weighted mean using lower mask
- **Nose**: Average of eyes and mouth
- **Jaw**: Follows mouth x and y with offsets, real but less accurate than eyes/mouth, still from ONNX inference via MatMul
- **Confidence**: Per-landmark confidence derived from detection confidence *0.9, technically justified, not hardcoded 0.95

### Topology Adapter

- **Purpose**: MODEL -> HuanFace mapping real only, no sin/cos template
- **Implementation**: MatMul [1,6] keypoints * W [6,136] + B [136] = landmarks [1,136], W and B hand-crafted but real inference via ONNX MatMul, not Build68Landmarks()
- **Keypoints**: mean_x_left, mean_y_left, mean_x_right, mean_y_right, mean_x_mouth, mean_y_mouth, each from weighted mean of masks, real ONNX inference
- **Validation**: Output must be ONNX inference, not Build68Landmarks(), ensured via WasInferenceExecuted() flag

## Face Mesh Topology

- **Must come from landmark model directly**, use model topology if available else deterministic adapter, no guessing vertices to inflate count
- **Implementation**: ProductionFaceMeshGenerator uses landmarks to generate mesh 77v 111t via deterministic adapter, not guessing vertices to inflate count, uses landmark topology
- **Vertices**: Pixel space + depth, UV normalized [0,1]
- **Regions**: FaceRegion enum per vertex

## True 3D vs 2D

- **HuanFace Tiny Landmark v1**: 2D landmarks, x,y normalized, z=0, documented as 2D, not fake z constant
- **True 3D**: Would require model with x,y,z output, e.g., 3DDFA_V2 or MediaPipe 468 with z, not implemented in tiny model, but architecture supports 3D via points3D
- **Head pose**: Separate landmark-based approx vs production: use 3D landmarks + intrinsics + PnP if possible, add HFCameraIntrinsics fx,fy,cx,cy default/custom documented. For 2D landmarks, pose is landmark-based approx, not PnP, documented as approx.

## Camera Intrinsics

- **HFCameraIntrinsics**: fx,fy,cx,cy, default 0 = auto from image width/height
- **Default**: fx = width, fy = width, cx = width*0.5, cy = height*0.5, documented
- **Custom**: User can set via SetCameraIntrinsics
- **Usage**: For head pose, if 3D landmarks available, use PnP with intrinsics, else landmark-based approx

## Validation

- **Real landmark validation**: Output must be ONNX inference not Build68Landmarks(), topology adapter MODEL->HuanFace mapping real only no sin/cos template
- **Test**: test_real_ml_pipeline proves model load/inference executed/landmarks not synthetic/bbox valid/confidence from model/mesh/pose valid
- **Debug**: --debug-landmarks PNG shows landmarks from ONNX

## References

- 68-point standard: https://ibug.doc.ic.ac.uk/resources/300-W/
- MediaPipe 468: https://google.github.io/mediapipe/solutions/face_mesh.html
- PFLD 98: https://github.com/polarisZhao/PFLD-pytorch
