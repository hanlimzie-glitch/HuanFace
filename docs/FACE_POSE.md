# Face Pose — Phase 5 Production

## Overview
Real face pose estimation from landmarks geometry, not hardcoded.

## Previous (Phase 4)
- rotationPitch/Yaw/Roll = 0 hardcoded
- translation = bbox center, tz=0
- scale = w/200

## Production (Phase 5)

### Pose Structure
```cpp
struct HFFacePose {
    float yaw;   // Y axis, degrees, [-90,90], positive right turn
    float pitch; // X axis, degrees, [-90,90], positive down
    float roll;  // Z axis, degrees, [-180,180], positive clockwise
    float tx, ty, tz; // translation pixel + depth
    float scale; // uniform scale relative to 200px reference
};
```

### Conventions
- **Coordinate system**: Right-handed, origin top-left image, X right, Y down, Z out
- **Yaw**: Rotation around Y (vertical). Positive = face turns right, left side more visible, nose moves toward right eye. Computed from nose vs left/right eye asymmetry.
- **Pitch**: Rotation around X (horizontal). Positive = face looks down, nose moves toward mouth. Computed from nose vs eye-mouth ratio.
- **Roll**: Rotation around Z (depth). Positive = clockwise tilt. Computed from eye angle atan2(dy,dx).
- **Translation**: tx,ty = face center (bbox center), tz = depth estimated from face size 500/(scale+0.2)
- **Scale**: bboxW / 200.0f
- **Units**: Degrees for rotation, pixels for tx,ty, arbitrary depth for tz, unitless for scale
- **Ranges**: yaw [-90,90], pitch [-90,90], roll [-180,180]

### Computation

#### Roll
- Left eye center = average 42-47
- Right eye center = average 36-41
- dx = right.x - left.x, dy = right.y - left.y
- roll = atan2(dy,dx) * 180/pi

#### Yaw
- distToLeft = noseTip.x - leftEye.x
- distToRight = rightEye.x - noseTip.x
- total = distToLeft + distToRight
- ratio = (distToLeft - distToRight)/total in [-1,1]
- yaw = ratio*60
- Plus offset from nose vs face center: (nose.x - faceCenterX)/(face.w*0.5)*30, blended 0.7/0.3
- Clamped [-90,90]

#### Pitch
- eyeMid = (leftEye+rightEye)/2
- mouthCenter = average 48-60
- eyeToMouth = mouth.y - eyeMid.y
- eyeToNose = noseTip.y - eyeMid.y
- ratio = eyeToNose / eyeToMouth, typical 0.5-0.6 frontal
- If eyeToMouth <=5 or ratio <0.2 or >0.9, clamp ratio to [0.3,0.75] and fallback to near 0
- pitch = (ratio - 0.52)*80, clamped [-90,90]

#### Translation & Scale
- tx = face.x + w*0.5, ty = face.y + h*0.5
- scale = w/200
- tz = 500/(scale+0.2)

### Validation
- IsValid(): finite yaw/pitch/roll/tx/ty/tz
- Tests: yaw/pitch/roll in range, translation finite, scale>0, frontal face yaw/pitch/roll near 0 (<30/<20), yaw changes with nose position (not hardcoded)

### Example
- Frontal face: yaw~0, pitch~0, roll~0
- Right turn: nose closer to right eye, distToLeft > distToRight, ratio positive, yaw positive
- Looking down: nose closer to mouth, ratio higher, pitch positive

### Debug Visualization
- `output_pose.png`: draws yaw line yellow horizontal from nose tip, pitch line cyan vertical, plus console Face ID, Confidence, Yaw/Pitch/Roll

### Limitations
- Heuristic based on 2D landmarks, not true 3D head pose from 3DMM
- Sensitive to landmark noise, especially nose detection
- No real depth sensor, tz estimated from size
- Extreme poses >60 deg may be inaccurate

### Future Work
- SolvePnP with 3D model points for true 6DOF pose
- Use 3DDFA_V2 or similar for 3DMM pose
- Add Kalman filter for pose smoothing
- Add head pose quality metric

### Status
- IMPLEMENTED: ProductionPoseEstimator with real yaw/pitch/roll from geometry, not hardcoded
- PARTIAL: Heuristic 2D, not 3DMM
- NOT IMPLEMENTED: SolvePnP, 3DMM, Kalman
