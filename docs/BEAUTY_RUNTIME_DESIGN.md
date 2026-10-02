# Beauty Runtime Design — HuanFace (Phase 2)

**Status:** SPECIFICATION — no beauty algorithm implementation in Phase 2  
**Evidence Source:** INI 263 keys (140 beauty), FUAI exports Face Beauty Processor, logs body_beautify/background_blur, catalog from strings

---

## 1. Beauty vs Makeup — Separate Engines

**From Phase 1:**
- Logs: `face_makeup` vs `body_beautify`, `background_blur`
- INI: `[CamBeauty]` 263 keys, many beauty params, separate from makeup params
- FUAI: `FUAI_FaceProcessorSetUseBeauty`, `FUAI_FaceBeautyProcessor`, `FUAI_BodyBeautyProcessor`, `FUAI_BackgroundBlurProcessor`
- Strings: beauty params distinct from makeup_* (HeavyBlur, ColorLevel, etc. vs makeup_intensity_*)

**HuanFace Decision: Separate Engines**

```cpp
class BeautyEngine {
public:
    HFResult Init(const HFEngineConfig& config);
    HFFrame Process(const HFFrame& input, const HFTrackingData& faceData, const HFBeautyParams& params);
    HFResult SetParam(const std::string& name, float value);
    HFResult Shutdown();
};

class MakeupEngine {
public:
    HFResult Init(const HFEngineConfig& config);
    HFFrame Process(const HFFrame& input, const HFTrackingData& faceData, const HFMakeupBundle& bundle, const std::map<std::string, HFMakeupParameter>& params);
    HFResult Shutdown();
};

class HuanFaceEngine { // Main engine composes both
    IFaceTracker* faceTracker;
    BeautyEngine* beautyEngine;
    MakeupEngine* makeupEngine;
    IRenderBackend* renderBackend;
    
    HFFrame ProcessFrame(const HFFrame& input) {
        auto tracking = faceTracker->Process(input);
        auto beautified = beautyEngine->Process(input, tracking, beautyParams);
        auto withMakeup = makeupEngine->Process(beautified, tracking, bundle, makeupParams);
        return withMakeup;
    }
};
```

**Why separate:**
- Beauty is full-face/body/background processing (skin smooth, whitening, face shape warp, background blur)
- Makeup is localized texture + mask + shader (lip, eye, blush, etc.)
- Different pipelines: Beauty uses bilateral filter, color adjustment, mesh warp; Makeup uses mask generation + texture sampling + blend
- Can be enabled/disabled independently: beauty only, makeup only, or both (beauty first, then makeup — because makeup should be applied on beautified skin)

---

## 2. Beauty Parameter Catalog (From INI + Strings)

From Phase 1 INI 263 keys (140 beauty) and strings:

### 2.1 Skin

```
HeavyBlur (0-100) — skin smooth intensity, bilateral filter strength
ColorLevel (0-100) — whitening / color level
DelspotLevel (0-100) — blemish removal / delspot
RedLevel (0-100) — redness removal?
Clarity (0-100) — clarity / sharpness?
Sharpen (0-100) — sharpen level
Brightness (0-100) — brightness
Saturation (0-100) — saturation
```

### 2.2 Face Shape

```
FaceThreed (0-100) — 3D face / face lift?
FaceSize (0-100) — face slimming (small face)
EyeEnlarging (0-100) — eye enlarging
EyeBright (0-100) — eye brightening
NoseSlim (0-100) — nose slimming
ChinSlim (0-100) — chin slimming
MouthSlim? — mouth adjustment
ForeheadSlim?
```

### 2.3 Eye / Brow / Nose / Mouth

```
EyeBrowShape? — eyebrow shape adjustment (not makeup, but shape warp)
EyeDistance? — eye distance
NoseLength?
MouthWidth?
ChinLength?
```

### 2.4 Teeth / Eye / Dark Circle / Etc.

```
ToothWhiten (0-100) — tooth whitening
RemovePouchStrength (0-100) — eye pouch removal (dark circle)
RemoveNasolabialFoldsStrength (0-100) — nasolabial fold removal (smile lines)
RemoveForeheadWrinkles? — forehead wrinkle removal
SmileLines?
```

### 2.5 Body / Background

```
BodySlim? — body slimming (from log body_beautify)
BackgroundBlur (0-100) — background blur intensity (from log background_blur, FUAI BackgroundBlurProcessor, Background Segmenter)
BackgroundSegmentation (bool) — enable background segmentation
```

### 2.6 Generic Beauty Param

```cpp
struct HFBeautyParameter {
    std::string name; // e.g., "HeavyBlur", "ColorLevel"
    float value = 0.0f;
    float minValue = 0.0f;
    float maxValue = 100.0f; // most beauty params 0-100 from INI
    float defaultValue = 0.0f;
    std::string group; // Skin, FaceShape, Eye, Nose, Mouth, Teeth, Body, Background
};

struct HFBeautyParams {
    std::map<std::string, HFBeautyParameter> params;
    // Or specific fields for P0:
    float heavyBlur = 0.0f; // 0-100
    float colorLevel = 0.0f; // 0-100
    float delspotLevel = 0.0f;
    float redLevel = 0.0f;
    float clarity = 0.0f;
    float sharpen = 0.0f;
    float faceThreed = 0.0f;
    float eyeBright = 0.0f;
    float toothWhiten = 0.0f;
    float removePouchStrength = 0.0f;
    // ... others
};
```

**Range only if evidence — INI suggests 0-100 for most beauty params, but we mark as observed from INI, not invented.**

---

## 3. Beauty Pipeline (Proposed)

```
Input HFFrame (from camera or image) + HFTrackingData
  │
  ├── 1. Skin Processing (GPU, full-screen filter + face mask)
  │   ├── Skin Mask: from landmarks/mesh (skin region = face contour minus eyes, lips, brows, hair) → R8 mask
  │   ├── HeavyBlur (Skin Smooth): bilateral filter shader on GPU, only on skin region (masked)
  │   │   ├── Bilateral filter: edge-preserving smooth, params: sigma_spatial, sigma_range, from HeavyBlur value
  │   │   ├── Implementation: 2-pass (horizontal + vertical) or 1-pass with sampling, GLSL/HLSL
  │   │   └── Example: for each pixel in skin mask, sample neighbors, weight by spatial distance and color distance, average
  │   ├── ColorLevel (Whitening): RGB curve adjustment, e.g., increase Y (luminance) in YUV or RGB
  │   ├── DelspotLevel (Blemish Removal): maybe inpainting or additional blur on small regions (detected via face parsing?)
  │   ├── RedLevel: reduce red channel in skin region
  │   ├── Clarity: local contrast enhancement
  │   ├── Sharpen: unsharp mask
  │   └── Output: beautified skin (still HFFrame)
  │
  ├── 2. Face Shape Warp (GPU, mesh warp)
  │   ├── Face Mesh: from tracking (HFFaceMesh)
  │   ├── Warp Params: FaceThreed, FaceSize, EyeEnlarging, NoseSlim, ChinSlim, etc. → adjust mesh vertices
  │   │   ├── Example: FaceSize (slimming) → move jaw vertices inward based on landmarks
  │   │   ├── EyeEnlarging → move eye contour vertices outward, scale eye region
  │   │   └── NoseSlim → move nose side vertices inward
  │   ├── Mesh Warp Shader: vertex shader moves vertices based on uniform params, fragment shader samples input texture with warped UV
  │   └── Output: warped face (HFFrame)
  │
  ├── 3. Eye / Teeth / Dark Circle (GPU, localized)
  │   ├── EyeBright: brighten eye white region (eye mask, increase brightness)
  │   ├── ToothWhiten: whiten teeth region (mouth interior mask, from landmarks or face parsing, increase brightness/reduce yellow)
  │   ├── RemovePouchStrength: eye pouch / dark circle removal — maybe inpainting or blur + color adjustment on under-eye region
  │   ├── RemoveNasolabialFoldsStrength: nasolabial fold removal — similar
  │   └── Output: with eye/teeth enhancements
  │
  ├── 4. Body / Background (optional, GPU)
  │   ├── BodySlim: body slimming (if body tracking available, from FUAI Human Processor/Driver)
  │   ├── BackgroundBlur: background segmentation (from FUAI Background Segmenter) → background mask (R8) → blur background (Gaussian blur shader) → composite foreground (face/body) sharp + background blurred
  │   │   ├── Segmentation: maybe MediaPipe Selfie Segmentation or BodyPix, or ONNX model for background matting
  │   │   └── Blur: Gaussian blur on background region
  │   └── Output: with body/background effects
  │
  └── Output HFFrame (beautified)
```

**Steps 1-3 are P0 for face beauty, Step 4 body/background P1**

**GPU vs CPU:**
- Skin smooth (bilateral) is heavy, must be GPU (fragment shader, full-screen)
- Face shape warp is GPU (vertex shader warp)
- Eye/teeth localized also GPU
- Background segmentation may be CPU (ONNX model) then blur GPU

---

## 4. Bilateral Filter Example (Clean-Room, Not FaceUnity)

**From INI HeavyBlur, and known beauty technique:**

GLSL (clean-room):

```glsl
#version 460 core
in vec2 v_uv;
uniform sampler2D u_inputTexture;
uniform sampler2D u_skinMask; // R8, 1=skin, 0=non-skin
uniform float u_heavyBlur; // 0-100, mapped to sigma
uniform vec2 u_texelSize; // 1/width, 1/height
out vec4 fragColor;

void main() {
    vec4 center = texture(u_inputTexture, v_uv);
    float mask = texture(u_skinMask, v_uv).r;
    if (mask < 0.1) { fragColor = center; return; } // not skin, no blur
    
    float sigma_s = mix(1.0, 10.0, u_heavyBlur / 100.0); // spatial sigma 1-10
    float sigma_r = mix(0.1, 0.5, u_heavyBlur / 100.0); // range sigma 0.1-0.5
    
    vec4 sum = vec4(0.0);
    float wsum = 0.0;
    // 7x7 kernel example, could be larger for stronger blur
    for (int x = -3; x <= 3; ++x) {
        for (int y = -3; y <= 3; ++y) {
            vec2 offset = vec2(float(x), float(y)) * u_texelSize;
            vec4 sampleCol = texture(u_inputTexture, v_uv + offset);
            float spatial = exp(-float(x*x + y*y) / (2.0 * sigma_s * sigma_s));
            float range = exp(-dot(sampleCol.rgb - center.rgb, sampleCol.rgb - center.rgb) / (2.0 * sigma_r * sigma_r));
            float weight = spatial * range;
            sum += sampleCol * weight;
            wsum += weight;
        }
    }
    vec4 blurred = sum / wsum;
    // Mix based on mask and intensity
    fragColor = mix(center, blurred, mask * (u_heavyBlur / 100.0));
}
```

**This is clean-room example, not FaceUnity's implementation (PROTECTED)**

**Other beauty shaders similarly clean-room**

---

## 5. No OBS / FaceUnity Dependency

**NOT using:**
- FaceUnity beauty implementation (PROTECTED)
- OBS beauty effects

**OBS is EXTERNAL REFERENCE only**

**HuanFace beauty dependencies (clean-room, open):**
- Custom GLSL/HLSL shaders clean-room
- ONNX or MediaPipe for segmentation (background) if needed
- No proprietary

---

**End of Beauty Runtime Design**
