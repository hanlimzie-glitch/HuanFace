# Tracking Pipeline — Phase 5 Production

## Overview
Temporal tracking with ID persistence and smoothing.

## Pipeline

```
Frame N
  ↓
ProductionFaceDetector.Detect -> detections with confidence
  ↓
For each detection:
  LandmarkEstimator + PoseEstimator + MeshGenerator -> faceData
  ↓
TemporalTracker.Update
  - Match detections to previous tracked faces via IoU
  - Assign persistent ID
  - EMA smoothing for landmarks and pose
  - Handle lost/reappearance
  ↓
HFTrackingData with trackingState
```

## Temporal Tracker

### State
```cpp
struct TrackedFace {
    int id;
    FaceDetection lastDetection;
    HFFaceData lastFaceData;
    int64_t lastSeenTimestamp;
    int lostFrames;
    int totalFramesTracked;
    float smoothedConfidence;
    std::vector<HFVec2> smoothedLandmarks;
    HFFacePose smoothedPose;
};

std::map<int, TrackedFace> trackedFaces;
int nextId = 0;
float smoothingAlpha = 0.6;
int maxLostFrames = 10;
float iouThreshold = 0.3;
```

### Matching
- For each new detection, find best previous tracked face with IoU > threshold
- IoU = intersection / union of bboxes
- Best IoU >0.3 matches same ID
- If no match, new ID = nextId++

### Smoothing
- EMA: smoothed = alpha*current + (1-alpha)*previous, alpha 0.6
- Landmarks: each point smoothed
- Pose: yaw/pitch/roll/tx/ty/tz/scale smoothed
- Confidence: smoothedConfidence = alpha*currentConf + (1-alpha)*prevConf
- 3D landmarks: x,y,z smoothed similarly

### States
```cpp
enum class HFTrackingState {
    DETECTED,   // newly detected
    TRACKED,    // tracked from previous
    LOST,       // lost this frame (internal)
    REAPPEARED  // reappeared after lost
};
```

- First frame: DETECTED
- Subsequent matching: TRACKED
- Lost: internal lostFrames increment, if >maxLostFrames removed
- Reappearance: if lostFrames>0 and matched again, REAPPEARED

### Multi-face
- Supports N faces up to maxFaces
- Each face has independent ID and tracking
- IDs unique across frames
- No fake second face, only real detections

### Example
```
Frame 1: Face A bbox (40,40,120,120) -> ID 0 DETECTED
Frame 2: Face A bbox (42,41,118,119) IoU 0.9 with ID 0 -> ID 0 TRACKED, smoothed landmarks
Frame 3: No face -> ID 0 lostFrames=1 kept internally
Frame 4: Face A bbox (41,40,119,120) IoU 0.85 with ID 0 (lostFrames 1 <10) -> ID 0 REAPPEARED
```

### Testing
- ID persistence: same face twice -> same ID
- Landmark smoothing: tracked landmarks not exactly current nor previous, but between
- Pose smoothing: similar
- Disappearance: no face -> 0 tracked, but internal keeps for maxLostFrames
- Reappearance: after 1 frame lost, same ID and REAPPEARED state
- Multi-face: two far faces -> two IDs unique

### Limitations
- Simple IoU matching, not appearance-based
- No Kalman filter, just EMA
- No handling of ID switch when faces cross
- No face recognition, only position

### Future Work
- Kalman filter for motion prediction
- Appearance embedding for re-identification
- Hungarian algorithm for multi-face matching
- Face quality based tracking

### Status
- IMPLEMENTED: TemporalTracker with IoU matching, EMA smoothing, ID persistence, lost/reappearance
- PARTIAL: Simple, not Kalman
- NOT IMPLEMENTED: Appearance re-ID, Kalman
