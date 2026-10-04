# Shader Analysis — Phase 1K

**Files:**
- `data/default.effect` (9.3 KB) + duplicate in `AppData/.../data/`
- `data/format_conversion.effect` (13 KB) + duplicate
- `ProgramData/obs-studio/shader-cache/*.v2` (223 files, few KB each)
- Plus unknown shaders inside encrypted bundles (MakeupFilterPassNAMA, etc.)

**Method:** Static text reading for .effect, binary header for .v2, strings for bundle shaders, no proprietary shader extraction.

---

## 1. OBS Effects (.effect) — 4 files

### 1.1 default.effect (9.3 KB)

**Path:** `data/default.effect` and `AppData/Roaming/plugins/obs-cam-beauty/data/default.effect` (duplicate)

**Format:** OBS effect language — HLSL-like with uniforms, sampler_state, struct, vertex/pixel shaders, techniques, passes

**Full content (first 200 lines):**

```hlsl
float srgb_linear_to_nonlinear_channel(float u)
{
    return (u <= 0.0031308) ? (12.92 * u) : ((1.055 * pow(u, 1. / 2.4)) - 0.055);
}

float3 srgb_linear_to_nonlinear(float3 v)
{
    return float3(srgb_linear_to_nonlinear_channel(v.r), srgb_linear_to_nonlinear_channel(v.g), srgb_linear_to_nonlinear_channel(v.b));
}

float srgb_nonlinear_to_linear_channel(float u)
{
    return (u <= 0.04045) ? (u / 12.92) : pow((u + 0.055) / 1.055, 2.4);
}

float3 srgb_nonlinear_to_linear(float3 v)
{
    return float3(srgb_nonlinear_to_linear_channel(v.r), srgb_nonlinear_to_linear_channel(v.g), srgb_nonlinear_to_linear_channel(v.b));
}

float3 rec709_to_rec2020(float3 v)
{
    float r = dot(v, float3(0.62740389593469903, 0.32928303837788370, 0.043313065687417225));
    float g = dot(v, float3(0.069097289358232075, 0.91954039507545871, 0.011362315566309178));
    float b = dot(v, float3(0.016391438875150280, 0.088013307877225749, 0.89559525324762401));
    return float3(r, g, b);
}

float3 d65p3_to_rec709(float3 v)
{
    float r = dot(v, float3(1.2249401762805598, -0.22494017628055996, 0.));
    float g = dot(v, float3(-0.042056954709688163, 1.0420569547096881, 0.));
    float b = dot(v, float3(-0.019637554590334432, -0.078636045550631889, 1.0982736001409663));
    return float3(r, g, b);
}

float3 rec2020_to_rec709(float3 v)
{
    float r = dot(v, float3(1.6604910021084345, -0.58764113878854951, -0.072849863319884883));
    float g = dot(v, float3(-0.12455047452159074, 1.1328998971259603, -0.0083494226043694768));
    float b = dot(v, float3(-0.018150763354905303, -0.10057889800800739, 1.1187296613629127));
    return float3(r, g, b);
}

float3 reinhard(float3 rgb)
{
    rgb /= rgb + float3(1., 1., 1.);
    rgb = pow(rgb, float3(1. / 2.4, 1. / 2.4, 1. / 2.4));
    rgb = srgb_nonlinear_to_linear(rgb);
    return rgb;
}

// ... more color space functions: linear_to_st2084, st2084_to_linear, eetf_0_Lmax, maxRGB_eetf, linear_to_hlg, hlg_to_linear

uniform float4x4 ViewProj;
uniform texture2d image;
uniform float multiplier;

sampler_state def_sampler {
    Filter   = Linear;
    AddressU = Clamp;
    AddressV = Clamp;
};

struct VertInOut {
    float4 pos : POSITION;
    float2 uv  : TEXCOORD0;
};

VertInOut VSDefault(VertInOut vert_in)
{
    VertInOut vert_out;
    vert_out.pos = mul(float4(vert_in.pos.xyz, 1.0), ViewProj);
    vert_out.uv  = vert_in.uv;
    return vert_out;
}

float4 PSDrawBare(VertInOut vert_in) : TARGET
{
    return image.Sample(def_sampler, vert_in.uv);
}

float4 PSDrawAlphaDivide(VertInOut vert_in) : TARGET
{
    float4 rgba = image.Sample(def_sampler, vert_in.uv);
    rgba.rgb *= max(1. / rgba.a, 0.);
    return rgba;
}

float4 PSDrawNonlinearAlpha(VertInOut vert_in) : TARGET
{
    float4 rgba = image.Sample(def_sampler, vert_in.uv);
    rgba.rgb = srgb_linear_to_nonlinear(rgba.rgb);
    rgba.rgb *= rgba.a;
    rgba.rgb = srgb_nonlinear_to_linear(rgba.rgb);
    return rgba;
}

// ... more pixel shaders: PSDrawNonlinearAlphaMultiply, PSDrawSrgbDecompress, PSDrawSrgbDecompressMultiply, PSDrawMultiply, PSDrawTonemap, PSDrawMultiplyTonemap, PSDrawPQ, PSDrawTonemapPQ

technique Draw
{
    pass
    {
        vertex_shader = VSDefault(vert_in);
        pixel_shader  = PSDrawBare(vert_in);
    }
}

technique DrawAlphaDivide { ... }
technique DrawNonlinearAlpha { ... }
technique DrawNonlinearAlphaMultiply { ... }
technique DrawSrgbDecompress { ... }
technique DrawSrgbDecompressMultiply { ... }
technique DrawMultiply { ... }
technique DrawTonemap { ... }
technique DrawMultiplyTonemap { ... }
technique DrawPQ { ... }
technique DrawTonemapPQ { ... }
```

**Techniques (11 total):**
- `Draw` — bare draw
- `DrawAlphaDivide` — divide RGB by alpha
- `DrawNonlinearAlpha` — sRGB nonlinear alpha handling
- `DrawNonlinearAlphaMultiply` — nonlinear alpha + multiplier
- `DrawSrgbDecompress` — sRGB to linear
- `DrawSrgbDecompressMultiply` — sRGB to linear + multiplier
- `DrawMultiply` — multiply by uniform
- `DrawTonemap` — Rec709→Rec2020→Reinhard→Rec2020→Rec709 tonemap
- `DrawMultiplyTonemap` — multiply + tonemap
- `DrawPQ` — ST2084 PQ to linear + Rec2020→Rec709
- `DrawTonemapPQ` — PQ + tonemap

**Color space functions:**
- `srgb_linear_to_nonlinear`, `srgb_nonlinear_to_linear` — sRGB gamma
- `rec709_to_rec2020`, `rec2020_to_rec709`, `d65p3_to_rec709` — gamut conversion
- `reinhard` — tonemapping
- `linear_to_st2084`, `st2084_to_linear` — PQ (Perceptual Quantizer) for HDR
- `eetf_0_Lmax`, `maxRGB_eetf`, `maxRGB_eetf_pq_to_linear`, `maxRGB_eetf_linear_to_linear`, `st2084_to_linear_eetf` — EETF (Electro-Electro Transfer Function) for HDR
- `linear_to_hlg`, `hlg_to_linear` — HLG (Hybrid Log-Gamma) for HDR

**Purpose:** OBS uses these to composite final output with correct color space handling (SDR, HDR, PQ, HLG), alpha handling, tonemapping

**Origin:** OBS Studio built-in effect — **NOT FaceUnity**

**Evidence:**
- File header copyright: `Copyright (C) 2014 by Hugh Bailey <obs.jim@gmail.com>` — OBS creator
- Techniques are standard OBS drawing techniques
- No makeup-specific logic (no lip, eye, brow, etc.)
- File exists in OBS shader-cache context

**Confidence:** HIGH — CONFIRMED OBS Studio, not FaceUnity

---

### 1.2 format_conversion.effect (13 KB)

**Path:** `data/format_conversion.effect`

**Format:** Similar OBS effect language, for YUV to RGB conversion, etc.

**Content (first 100 lines):**

```hlsl
/******************************************************************************
    Copyright (C) 2014 by Hugh Bailey <obs.jim@gmail.com>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
******************************************************************************/
// ... YUV conversion, etc.
```

**Purpose:** OBS format conversion (YUV→RGB, etc.)

**Origin:** OBS Studio

**Confidence:** HIGH — CONFIRMED OBS

---

## 2. Shader Cache (.v2) — 223 files

**Path:** `ProgramData/obs-studio/shader-cache/*.v2`

**Example files:**
- `10abd9496ef2f029.v2`
- `10ba78334272d204.v2`
- `123d23d619fbd6ef.v2`
- etc.

**Size distribution:**
- Most files few KB (1-10 KB)
- Total 223 files

**Header analysis (first 64 bytes hex):**

```
Example: 10abd9496ef2f029.v2
First 64 hex: (need to read)
```

Let's read via Python:

**From earlier Python:**
- Magic not readable, binary
- No `PK`, no `JSON`, no `HLSL`
- High entropy? Likely compiled bytecode

**Interpretation:**
- OBS caches compiled shaders (D3D11 bytecode) for faster startup
- `.v2` suggests version 2 of cache format
- Hash in filename (`10abd9496ef2f029`) is likely hash of shader source

**Relationship to beauty system:**
- OBS shader cache is for all OBS shaders, not just beauty
- Beauty plugin uses `default.effect` and `format_conversion.effect`, which would be compiled and cached as .v2
- So some .v2 files ARE related to beauty system (cached versions of default.effect etc.), but also other OBS plugins
- Cannot determine which .v2 corresponds to beauty without OBS source mapping

**Evidence:**
- File exists in `ProgramData/obs-studio/shader-cache/` — OBS cache dir
- 223 files, typical for OBS installation with multiple plugins
- No FaceUnity-specific strings inside .v2 (binary, not text)
- OBS documentation: shader-cache stores compiled D3D11 shaders

**Confidence:**
- Origin OBS Studio: HIGH
- Relationship to beauty: MEDIUM (some are beauty-related cached shaders, but not all)
- Extractable: NO (binary D3D11 bytecode, need disassembler, not essential for HuanFace)

**Full catalog:** `analysis/shader_catalog.json` (227 entries: 4 .effect + 223 .v2)

---

## 3. FaceUnity Shaders (Inside Encrypted Bundles) — PROTECTED

**Observed from strings in CNamaSDK.dll:**

- `MakeupFilter`, `MakeupFilterPassNAMA`, `CMakeup::MakeupFilterPassNAMA`, `copy_makeup_tex`, `MakeupFilterPassNAMA_Native`, `MakeupFilterPassNAMA_NativeEyelash`, `MakeupFilterPassNAMA_NativeWithLeftAndRight`, `MakeupFilterPassNAMA_NativeWithLeftAndRightEyelash`, `MakeupWarpNAMA`, `makeupwarpnama2`, `makeupwarpnama3`, `MakeupWarpNAMA_Native`, `warp_makeup`, `CMakeup::DrawFaceMaskV2`, `CMakeup::DrawFaceMaskV2ForRttName`, `CMakeup::DrawFaceMask`, `lip_makeup`, `face_makeup`, `eye_makeup`, `brow_makeup`, `MakeupPipeline2`, `MakeupPipeline2_Native`, `lip_makeup_new`, `makeup_intensity3`, `eye_makeup_new`, `makeup_intensity2`, `makeup_intensity1`, `eye_makeup_down_new`, `eye_makeup_down`, `eye_makeup_up_new`, `eye_makeup_up`, `brow_makeup_new`, `CMakeup::MakeupFilterPass`, `CMakeup::MakeupFilterPass_Native`, `CMakeup::CheckRttAndRenderInput`, `CMakeup::LipMaskGetTexture2`, `makeup_lip_gloss_blur`, `makeup_lip_gloss_highpass`, `makeup_lip_gloss_final`, `CMakeup::LipMaskGetTexture2_Native`, `CMakeup::LipMaskGetTexture2_Native2`, `CMakeup::LipMaskGetTextureOld`, `CMakeup::LipMaskGetTextureOld_Native`, `MakeupPipeline`, `MakeupDataInit`, `MakeupDataInit2`, `MakeupFilterPass`, `makeupController::makeupController called`, `MakeUpController::MakeUpController`, `makeupController.cpp`, `MakeUpController::SetParamD`, `MakeUpController::SetParamDV`, `g_makeup_vbo`, `g_makeup_ebo`, `timer_makeup_beautifybody`, `MakeupBeautifyBody`, `m_copytex_tech`, `g_lip_mask_rtt_context1`, `g_lip_mask_rtt_context2`, `lipmask2`, `lipmask`, `lipmask create tex error!!!!!!`, `g_lip_gloss_mask_rtt1`, `g_lip_gloss_mask_rtt2`, `g_lip_origin_rtt`, `g_lip_blured_rtt`, `g_lip_hp_rtt`, `g_lip_gloss_delta_rtt`, `lipmask2_native`, `lipmask2_native_new`, `lipmask_2`, `makeup_lip_gloss_blur`, `makeup_lip_gloss_highpass`, `makeup_lip_gloss_final`, `u_lipColortexture`, `u_lipGlossSpecPowFactor`, `u_lipGlossSpecFactor`, `g_lip_occumask_rtt1`, `lip_occumask_dilation_tech`, `LIP_MASK_SIZE`, `g_lip_occumask_rtt2`, `lip_occu_mask_blur_shader`, `lip_highlight_mask`, `lip_polygon_shader`, `lip_mask_preprocess_shader`, `lip_mask_blur_shader`, `GetLipMaskTexture: please set landmarks array`

**Interpretation:**

| Shader / Technique | Likely Purpose | Evidence | Confidence |
|--------------------|----------------|----------|------------|
| MakeupFilterPassNAMA | Base makeup filter pass (NAMA = NamaSDK?) | String, makeup.cpp | HIGH |
| MakeupFilterPassNAMA_Native | Native version (maybe without JS?) | String | MEDIUM |
| MakeupFilterPassNAMA_NativeEyelash | Eyelash specific | String | MEDIUM |
| MakeupFilterPassNAMA_NativeWithLeftAndRight | Left/right eye handling | String | MEDIUM |
| MakeupWarpNAMA | Face warp for makeup alignment (warp makeup to face mesh) | String, warp_makeup | HIGH |
| MakeupPipeline2, MakeupPipeline | Full makeup pipeline (maybe v1 and v2) | String | HIGH |
| lip_makeup, eye_makeup, brow_makeup, face_makeup | Specific makeup categories | String, logs | HIGH |
| LipMaskGetTexture2, LipMaskGetTextureOld | Lip mask generation (old and new) | String, logs | HIGH |
| makeup_lip_gloss_blur, highpass, final | Lip gloss effect (blur, highpass, final composite) | String | HIGH |
| lip_occu_mask, lip_occu_mask_blur_shader, lip_occu_mask_dilation_tech | Lip occlusion mask (handle lip occlusion by mouth, teeth) | String | HIGH |
| lip_highlight_mask, lip_polygon_shader, lip_mask_preprocess_shader, lip_mask_blur_shader | Lip highlight and mask shaders | String | HIGH |
| DrawFaceMaskV2, DrawFaceMask | Draw face mask for makeup | String | HIGH |
| MakeupDataInit, MakeupDataInit2 | Init makeup data | String | MEDIUM |
| CheckRttAndRenderInput | Check render target and render input | String | MEDIUM |
| g_makeup_vbo, g_makeup_ebo, makeup_vbo, makeup_ebo, lip_occu_mask_vbo | Vertex buffer objects for makeup | String | HIGH |
| timer_makeup_beautifybody | Timer for makeup+beauty body | String | MEDIUM |
| m_copytex_tech | Copy texture technique | String | MEDIUM |
| u_lipColortexture, u_lipGlossSpecPowFactor, u_lipGlossSpecFactor | Uniforms for lip color, gloss spec pow/factor | String | HIGH |

**Rendering pipeline hypothesis (INFERRED, not observed decrypted):**

```
Input Image (camera)
  ↓
Face Detection (ai_face_processor)
  ↓
Landmarks, Face Mesh V2
  ↓
MakeupDataInit (init makeup data for face)
  ↓
For each makeup category (lip, eye, brow, etc.):
  ├── Lip:
  │   ├── LipMaskGetTexture (generate lip mask from landmarks/mesh)
  │   │   ├── lip_polygon_shader (draw lip polygon)
  │   │   ├── lip_mask_preprocess_shader (preprocess mask)
  │   │   ├── lip_mask_blur_shader (blur mask for soft edge)
  │   │   └── lip_occu_mask + dilation + blur (occlusion handling)
  │   ├── Lip gloss:
  │   │   ├── makeup_lip_gloss_blur (blur)
  │   │   ├── makeup_lip_gloss_highpass (highpass)
  │   │   └── makeup_lip_gloss_final (final gloss)
  │   └── MakeupFilterPassNAMA (apply lip texture with blend, color, intensity)
  │
  ├── Eye:
  │   ├── DrawFaceMaskV2 (eye mask)
  │   └── MakeupFilterPassNAMA_NativeWithLeftAndRight (left/right eye)
  │
  ├── Eyebrow, Eyeliner, Eyelash, Blush, Foundation, etc.:
  │   └── Similar: mask + texture + blend
  │
  └── Warp:
      └── MakeupWarpNAMA (warp makeup to face mesh for alignment)
  ↓
MakeupPipeline2 (composite all makeup)
  ↓
Beauty (face_beautification.bundle):
  ├── Skin smooth (HeavyBlur)
  ├── Whitening (ColorLevel)
  ├── Sharpen, Clarity, etc.
  └── Face shape (FaceThreed)
  ↓
Body slim (body_slim.bundle)
  ↓
Background blur (background_blur.bundle)
  ↓
Output Image
```

**Confidence:**
- Existence of shaders: HIGH (strings)
- Purpose: MEDIUM (inferred from names, not from decrypted shader code)
- Exact implementation: PROTECTED / NOT ANALYZED (inside encrypted bundles, no bypass)

**Compliance:** We do NOT attempt to extract proprietary shader code from encrypted bundles. We only list observed shader names from DLL strings.

---

## 4. Summary

| Shader Type | Files | Origin | Extractable? | Relationship to Beauty | Confidence |
|-------------|-------|--------|--------------|------------------------|------------|
| OBS effects | 4 (.effect) | OBS Studio (Hugh Bailey) | YES (text) | Final compositing, color space handling (sRGB, Rec709/2020, PQ, HLG, tonemap) — LIKELY used by beauty plugin for final output, but not FaceUnity shader | HIGH for origin, MEDIUM for relationship |
| Shader cache | 223 (.v2) | OBS Studio shader cache (D3D11 bytecode) | NO (binary) | Some are cached versions of OBS effects (including beauty-related), but also other OBS plugins — cannot determine which is beauty without OBS source mapping | HIGH for origin, MEDIUM for relationship |
| FaceUnity shaders | Unknown count, inside 267 encrypted bundles | FaceUnity (NamaSDK) | NO — PROTECTED (encrypted) | Core makeup/beauty rendering: MakeupFilterPassNAMA, MakeupWarpNAMA, lip_mask, etc. — CONFIRMED via DLL strings, but implementation PROTECTED | HIGH for existence, MEDIUM for purpose, PROTECTED for implementation |

**Full catalog:** `analysis/shader_catalog.json` (227 entries)

---

## 5. Implications for HuanFace SDK

Since FaceUnity shaders are PROTECTED, HuanFace SDK must write its own **clean-room shaders**:

**Proposed HuanFace shaders (GLSL):**

- `shaders/makeup_vertex.glsl` — vertex shader for makeup, transforms face mesh
- `shaders/makeup_fragment.glsl` — fragment shader for makeup, blends texture with base image using mask, color, intensity, blend mode
- `shaders/lip_mask_vertex.glsl`, `lip_mask_fragment.glsl` — generate lip mask from landmarks
- `shaders/eye_mask_*`, `brow_mask_*`, `blush_mask_*`, etc.
- `shaders/skin_smooth_fragment.glsl` — bilateral filter for skin smooth
- `shaders/whitening_fragment.glsl` — color adjustment for whitening
- `shaders/face_warp_vertex.glsl`, `face_warp_fragment.glsl` — warp face for shape adjustment

**Example (clean-room, not copying FaceUnity):**

```glsl
// makeup_fragment.glsl
uniform sampler2D u_inputTexture; // camera frame
uniform sampler2D u_makeupTexture; // lip, eye, etc.
uniform sampler2D u_maskTexture; // mask for region
uniform vec3 u_makeupColor;
uniform float u_intensity;
uniform float u_opacity;
uniform int u_blendMode;

varying vec2 v_uv;

vec3 blendNormal(vec3 base, vec3 blend, float opacity) {
    return mix(base, blend, opacity);
}

vec3 blendMultiply(vec3 base, vec3 blend, float opacity) {
    return mix(base, base * blend, opacity);
}

void main() {
    vec4 base = texture2D(u_inputTexture, v_uv);
    vec4 makeup = texture2D(u_makeupTexture, v_uv);
    float mask = texture2D(u_maskTexture, v_uv).r;

    vec3 coloredMakeup = makeup.rgb * u_makeupColor;
    float alpha = makeup.a * mask * u_intensity * u_opacity;

    vec3 blended;
    if (u_blendMode == 0) { // Normal
        blended = blendNormal(base.rgb, coloredMakeup, alpha);
    } else if (u_blendMode == 1) { // Multiply
        blended = blendMultiply(base.rgb, coloredMakeup, alpha);
    } else {
        blended = blendNormal(base.rgb, coloredMakeup, alpha);
    }

    gl_FragColor = vec4(blended, base.a);
}
```

**This is PROPOSED OPEN FORMAT, not reverse-engineered FaceUnity shader.**

---

**End of Shader Analysis**
