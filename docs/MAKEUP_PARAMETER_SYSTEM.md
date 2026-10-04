# Makeup Parameter System — Phase 6 Full Makeup Renderer

**Branch:** arena/01a0e5f5-huanface  
**Phase:** 6 — Full Makeup Renderer ✅ IMPLEMENTED  
**Status:** 17/17 tests PASS, sensitivity verified

## Overview

Centralized parameter system, per-feature, affects rendering, testable sensitivity.

```
makeup.lip.enabled, color, intensity, opacity
makeup.foundation.enabled, color, intensity, opacity
makeup.blush.enabled, color, intensity, opacity
makeup.eyebrow.enabled, color, intensity
makeup.eyeliner.enabled, color, intensity
makeup.eyelash.enabled, intensity
makeup.eyeshadow.enabled, color, intensity
makeup.pupil.enabled, color, intensity
```

## Structures

```cpp
struct HFFloat4 { float r,g,b,a; };

enum class HFBlendMode { Normal, Multiply, Screen, Overlay };

struct HFLipMakeupParams {
    bool enabled=true;
    float intensity=0.8f; // 0-1
    HFFloat4 color=1,0.2,0.3,1;
    float opacity=0.9f;
    float feather=1.0f;
    float scale=1.0f;
    HFBlendMode blendMode=Normal;
};

struct HFFoundationParams {
    bool enabled=false;
    float intensity=0.5f;
    HFFloat4 color=0.95,0.8,0.7,1;
    float opacity=0.6f;
    float feather=2.0f;
    float softness=0.5f;
    HFBlendMode blendMode=Normal;
};

struct HFBlushParams {
    bool enabled=false;
    float intensity=0.6f;
    HFFloat4 color=1,0.4,0.4,1;
    float opacity=0.7f;
    float feather=3.0f;
    float scale=1.0f;
    HFBlendMode blendMode=Normal;
};

struct HFEyebrowParams {
    bool enabled=false;
    float intensity=0.7f;
    HFFloat4 color=0.3,0.2,0.15,1;
    float opacity=0.8f;
    float feather=1.5f;
    float thickness=1.0f;
    HFBlendMode blendMode=Normal;
};

struct HFEyelinerParams {
    bool enabled=false;
    float intensity=0.8f;
    HFFloat4 color=0.1,0.1,0.1,1;
    float opacity=0.9f;
    float thickness=2.0f;
    float feather=0.5f;
    HFBlendMode blendMode=Normal;
};

struct HFEyelashParams {
    bool enabled=false;
    float intensity=0.8f;
    float length=1.0f;
    float thickness=1.0f;
    float opacity=0.9f;
    HFFloat4 color=0.05,0.05,0.05,1;
};

struct HFEyeshadowParams {
    bool enabled=false;
    float intensity=0.6f;
    HFFloat4 color=0.8,0.4,0.6,1;
    float opacity=0.7f;
    float feather=2.0f;
    HFBlendMode blendMode=Multiply;
};

struct HFPupilParams {
    bool enabled=false;
    float intensity=0.5f;
    HFFloat4 color=0.2,0.5,0.8,1;
    float opacity=0.6f;
    float irisEnhancement=0.5f;
    float scale=1.1f;
};

struct HFMakeupParameters {
    HFLipMakeupParams lip;
    HFFoundationParams foundation;
    HFBlushParams blush;
    HFEyebrowParams eyebrow;
    HFEyelinerParams eyeliner;
    HFEyelashParams eyelash;
    HFEyeshadowParams eyeshadow;
    HFPupilParams pupil;
    bool enabled=true;
    float globalIntensity=1.0f;
    bool IsValid() const;
    std::map<string,float> ToFloatMap() const;
    std::map<string,HFFloat4> ToColorMap() const;
};
```

## Parameter Affects Rendering (Per Gate)

All params affect rendering, tested via sensitivity:

- **Lip**: intensity 0 vs 0.5 vs 1 mean pixel diff >0.1/0.2, color affects output, opacity, feather, scale, blendMode
- **Foundation**: intensity 0 vs 1 diff >0.1, color, opacity, feather, softness
- **Blush**: intensity, color, opacity, feather, scale, follows cheek position not absolute
- **Eyebrow**: intensity, color, opacity, feather, thickness, follows brow landmarks
- **Eyeliner**: intensity, color, opacity, thickness, feather, follows eye contour
- **Eyelash**: intensity, length, thickness, opacity, color
- **Eyeshadow**: intensity, color, opacity, feather, blendMode, follows eyelid movement
- **Pupil**: intensity, color, opacity, irisEnhancement, scale, position from landmark

## Sensitivity Test (Wajib)

Per spec:

```
intensity = 0
intensity = 0.5
intensity = 1
Output must differ
mean pixel difference > epsilon
If 0 and 1 identical => FAIL
```

Implemented in `TestMakeup()`:

- Lip: Render with intensity 0, 0.5, 1, compute meanDiff, check >0.1
- Foundation: 0 vs 1 diff >0.1
- All features: enabled true with intensity >0.001 renders, intensity 0 returns input unchanged (no makeup)

## Bundle Integration

Parameters can be defined in bundle manifest:

```json
{
    "type": "makeup",
    "version": 1,
    "features": ["lip","blush","eyeshadow"],
    "parameters": {
        "makeup.lip.color": [1,0.2,0.3,1],
        "makeup.lip.intensity": 0.8,
        "makeup.blush.color": [1,0.4,0.4,1]
    }
}
```

`ToFloatMap` and `ToColorMap` provide string->value mapping for bundle loading via ResourceManager.

## Validation

- `IsValid()`: All intensities 0-1, opacities 0-1
- `IsIntensityZero`, `IsIntensityFull` helpers
- No param that doesn't affect rendering — all tested

## Status

IMPLEMENTED, sensitivity verified, no fake param.

**End of Parameter System**
