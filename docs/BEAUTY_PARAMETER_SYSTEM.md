# Beauty Parameter System — Phase 7 Full Beauty Engine

**Branch:** arena/01a0e5f5-huanface  
**Phase:** 7 — Beauty Parameter System ✅ IMPLEMENTED

## Overview

Centralized parameter system for beauty features, runtime controllable, affects rendering, testable sensitivity.

## Structures

```cpp
struct HFSkinSmoothingParams {
    bool enabled = true;
    float intensity = 0.5f; // 0-1, 0=no smoothing, 1=full
    float radius = 2.0f; // 0.5-5.0
    float opacity = 0.8f; // 0-1 blend with original
    float edgePreservation = 0.6f; // 0-1, 0=gaussian, 1=edge-aware bilateral-like
};

struct HFSkinTextureParams {
    bool enabled = false;
    float intensity = 0.5f;
    float preservation = 0.7f; // structure preservation, avoid plastic
    float opacity = 0.7f;
    float detailThreshold = 0.3f; // threshold for small noise vs structure
};

struct HFBlemishReductionParams {
    bool enabled = false;
    float intensity = 0.5f;
    float radius = 2.5f;
    float opacity = 0.8f;
    // NOT AI blemish detection, just skin blemish reduction
};

struct HFSkinToneParams {
    bool enabled = false;
    float intensity = 0.5f;
    float temperature = 0.0f; // -1..1 warm/cool
    float tint = 0.0f; // -1..1 green/magenta
    float saturation = 0.0f; // -1..1 desat/sat
    float opacity = 0.7f;
};

struct HFBrightnessParams {
    bool enabled = false;
    float intensity = 0.0f; // -1..1 neutral 0, range documented
    float opacity = 0.8f;
    bool skinOnly = true; // default skin-only per spec
};

struct HFContrastParams {
    bool enabled = false;
    float intensity = 0.0f; // -1..1 neutral 0
    float opacity = 0.8f;
    bool skinOnly = true;
};

struct HFBeautyRetouchParams {
    bool enabled = true;
    float intensity = 0.8f; // global
    float smoothing = 0.5f; // maps to smoothingIntensity
    float texture = 0.3f;
    float blemish = 0.4f;
    float tone = 0.3f;
    float brightness = 0.0f;
    float contrast = 0.0f;
    float opacity = 0.9f;
};

struct HFBeautyParameters {
    bool enabled = true;
    float globalIntensity = 1.0f;
    float opacity = 0.9f;
    HFSkinSmoothingParams smoothing;
    HFSkinTextureParams texture;
    HFBlemishReductionParams blemish;
    HFSkinToneParams tone;
    HFBrightnessParams brightness;
    HFContrastParams contrast;
    HFBeautyRetouchParams retouch;
    bool IsValid() const; // checks ranges
    std::map<std::string,float> ToFloatMap() const;
};
```

## Ranges and Documentation

- Smoothing intensity 0..1, radius 0.5-5.0, opacity 0..1, edgePreservation 0..1
- Texture intensity 0..1, preservation 0..1, opacity 0..1, detailThreshold 0..1
- Blemish intensity 0..1, radius 0.5-5, opacity 0..1
- Tone intensity 0..1, temperature -1..1, tint -1..1, saturation -1..1, opacity 0..1
- Brightness -1..1 neutral 0, opacity 0..1, skinOnly default true
- Contrast -1..1 neutral 0, opacity 0..1, skinOnly default true
- Retouch intensity 0..1, smoothing/texture/blemish/tone 0..1, brightness/contrast -1..1, opacity 0..1
- Global intensity 0..1, opacity 0..1

All ranges documented in header and in docs/BEAUTY_RENDER_PIPELINE.md.

## Parameter to CPU/GPU Mapping

```
parameter
    ↓
CPU reference (HFImage processing)
    ↓
GPU constant buffer (BeautyConstants cbuffer)
```

Same parameters used for both paths:

- CPU: uses params.smoothing.intensity, radius, opacity, edgePreservation directly in BilateralLikeBlur
- GPU: same values in cbuffer BeautyConstants g_SmoothingIntensity, g_SmoothingRadius, etc., used in HLSL shaders

Example:

```cpp
cbuffer BeautyConstants : register(b0) {
    float g_SmoothingIntensity;
    float g_SmoothingRadius;
    float g_SmoothingOpacity;
    float g_EdgePreservation;
    // ...
};
```

CPU and GPU use same parameter, ensuring consistency.

## Sensitivity Tests

Per spec, for each feature 0, 0.5, 1 must produce different output, pixel difference > epsilon, FAIL if equal.

Implemented in test_beauty.cpp BeautyParameterSensitivityTests:

- Smoothing 0 vs 0.5 diff>0.1, 0.5 vs 1 diff>0.1, 0 vs 1 diff>0.2
- Texture 0 vs 1 diff>0.1
- Blemish 0 vs 1 diff>0.1
- Tone 0 vs 1 (with temperature 0.5, tint 0.2, saturation 0.3) diff>0.1
- Brightness 0 vs 0.8 diff>0.1
- Contrast 0 vs 0.8 diff>0.1, -0.8 vs 0.8 diff>0.1

Mean pixel difference computed via ComputeMeanDiff (average absolute per channel).

If diff <= epsilon, test FAIL.

Not just testing setter accepts value, but that rendering actually changes.

## Retouch Abstraction

Retouch is NOT hardcoded filter, it calls feature pipeline:

```cpp
HFResult RenderRetouch(input, face, masks, params, out, err) {
    // Map retouch params to feature params with global intensity
    float global = retouch.intensity * globalIntensity;
    if(smoothing) RenderSmoothing(current, face, skinMask, mappedSmoothing, tmp, err);
    if(texture) RenderTextureRefinement(...);
    if(blemish) RenderBlemishReduction(...);
    if(tone) RenderSkinTone(...);
    if(brightness) RenderBrightness(...);
    if(contrast) RenderContrast(...);
}
```

Retouch parameters: enabled, intensity, smoothing, texture, blemish, tone, brightness, contrast, opacity — minimal per spec, implemented.

## Bundle Integration

ToFloatMap includes all beauty params with keys like "beauty.smoothing.intensity", "beauty.brightness.intensity", etc., for bundle manifest integration.

## Thread Safety

Parameters are per-engine instance, not global mutable, document thread-safe/single-threaded/externally synchronized per spec.

- HFEngine_ has mutex for loadedBundles and params, but beautyParams is per-engine and should be externally synchronized if accessed from multiple threads.
- CPU renderer is stateless except lastPerf, which is per-instance.
- No global mutable without reason.

## NO FAKE GATE

- Parameter affects output? YES (sensitivity tests diff>epsilon)
- Runtime controllable? YES (SetParameters, EnableFeature)
- CPU and GPU use same params? YES (cbuffer mapping)
- Not just setter? YES (rendering changes)
