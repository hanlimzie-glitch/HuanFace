# Face Mesh — Phase 5 Production

## Overview
Phase 5 upgrades from 5x5 synthetic grid mesh to real face mesh following face shape, with topology documented and regions.

## Previous (Phase 4)
- 5x5 grid =25 vertices inside bbox
- 32 triangles (2 per quad)
- Vertices pixel + depth center higher (1-dist)*10
- UV normalized
- Not following face shape, just grid

## Production (Phase 5)

### Mesh Structure
```cpp
struct HFFaceMesh {
    std::vector<HFVec3> vertices; // pixel space + depth
    std::vector<int> indices; // triangles
    std::vector<HFVec2UV> uv; // normalized [0,1]
    std::vector<FaceRegion> vertexRegions; // region per vertex
    int width, height;
};
```

### Vertex Count
- 68 landmarks + 9 additional = 77 vertices
  - 68 from landmarks (jaw 17, brows 10, nose 9, eyes 12, lips 20)
  - 5 forehead above brows
  - 2 cheeks left/right
  - 1 chin center
  - 1 nose bridge additional
- Each vertex has depth: nose tip 12, bridge 8, eye 2, brow 1, lip 3, chin -2, cheek 0.5, forehead 1

### Topology
Indices generated following face shape, not grid:

- Jaw chain 0-16 to nose bridge 27 and tip 30: triangles fan from jaw to nose
- Eyebrow to eye: brow 17-21 to eye 36-41, 22-26 to 42-47
- Nose bridge 27-30 to tip 31-35 to outer lip 48
- Right eye fan: center 36, perimeter 37-41
- Left eye fan: center 42, perimeter 43-47
- Outer lip fan: center 48, perimeter 49-59, plus connections to inner lip 60-67
- Inner lip fan: center 60, perimeter 61-67
- Forehead 68-72 to brows 19,24
- Cheeks 73,74 to eyes and jaw and lips
- Chin 75 to jaw 6,8,10
- Nose additional 76 to bridge 27-29

Total triangles ~112 (336 indices), validated no degenerate (distinct indices), no NaN, indices in bounds.

### Regions
```cpp
enum class FaceRegion {
    FACE, FOREHEAD, LEFT_EYE, RIGHT_EYE, LEFT_BROW, RIGHT_BROW,
    NOSE, NOSE_BRIDGE, NOSE_TIP, LIP, OUTER_LIP, INNER_LIP,
    LEFT_CHEEK, RIGHT_CHEEK, CHIN, JAW
};
```

Mapping:
- 0-16 JAW
- 17-21 RIGHT_BROW
- 22-26 LEFT_BROW
- 27-30 NOSE_BRIDGE
- 31-35 NOSE_TIP
- 36-41 RIGHT_EYE
- 42-47 LEFT_EYE
- 48-60 OUTER_LIP
- 61-67 INNER_LIP
- 68-72 FOREHEAD
- 73 LEFT_CHEEK, 74 RIGHT_CHEEK, 75 CHIN, 76 NOSE_BRIDGE

Minimal regions required: FACE, FOREHEAD, LEFT_EYE, RIGHT_EYE, LEFT_BROW, RIGHT_BROW, NOSE, LIP, OUTER_LIP, INNER_LIP, LEFT_CHEEK, RIGHT_CHEEK, CHIN all present.

### UV
- UV = pixel / imageWidth, pixel / imageHeight, normalized [0,1]
- Validated in [0,1] approx (-0.1 to 1.1 allowed for slight out of bounds)

### Validation
- IsValid(): vertices not empty, indices not empty, vertices.size()==uv.size(), indices%3==0, no NaN, indices in bounds
- Tests: vertex count >=68, triangle count >0, no NaN, indices in bounds, UV in [0,1], 70% vertices inside bbox (follows face), regions present, no degenerate triangles

### Coordinate System
- Vertices in pixel space (x,y) + depth z
- Origin top-left, X right, Y down, Z out
- UV normalized top-left origin
- D3D11 NDC conversion: (u*2-1, 1-v*2)

### Debug Visualization
- `output_mesh.png`: wireframe white lines over input image, mesh following face shape, not grid
- Shows real mesh actual

### Future Work
- Denser mesh 468 points from MediaPipe
- 3DMM morphable model
- Subdivision surface
- Soft edge with blur
- GPU mesh generation

### Status
- IMPLEMENTED: ProductionFaceMeshGenerator with 77 vertices, real topology, regions, validation
- PARTIAL: Still heuristic based on landmarks, not ML 468
- NOT IMPLEMENTED: MediaPipe 468, 3DMM
