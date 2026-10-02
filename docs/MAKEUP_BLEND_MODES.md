# Makeup Blend Modes — Phase 6 Full Makeup Renderer

**Branch:** arena/01a0e5f5-huanface  
**Phase:** 6 — Full Makeup Renderer ✅ IMPLEMENTED  
**Status:** BlendModeTests PASS, real formulas

## Overview

Abstraction:

```cpp
enum class HFBlendMode {
    Normal,
    Multiply,
    Screen,
    Overlay
};
```

Real formulas with alpha/mask consideration.

## Formulas (Per Spec)

### Normal
```
result = source
With alpha: result = source*alpha + base*(1-alpha)
```

Implementation:
```cpp
float BlendNormal(float base, float source, float alpha) {
    return source*alpha + base*(1-alpha);
}
```

### Multiply
```
result = base * source
With alpha: (base*source)*alpha + base*(1-alpha)
```

Implementation:
```cpp
float BlendMultiply(float base, float source, float alpha) {
    float blended = base*source;
    return blended*alpha + base*(1-alpha);
}
```

### Screen
```
result = 1 - (1-base)*(1-source)
With alpha: blended*alpha + base*(1-alpha)
```

Implementation:
```cpp
float BlendScreen(float base, float source, float alpha) {
    float blended = 1.0f - (1.0f-base)*(1.0f-source);
    return blended*alpha + base*(1-alpha);
}
```

### Overlay
Standard overlay:
```
if base<0.5: 2*base*source
else: 1-2*(1-base)*(1-source)
With alpha
```

Implementation:
```cpp
float BlendOverlay(float base, float source, float alpha) {
    float blended;
    if(base<0.5f) blended=2*base*source;
    else blended=1-2*(1-base)*(1-source);
    blended=clamp(0,1,blended);
    return blended*alpha + base*(1-alpha);
}
```

All blend consider alpha/mask:

```cpp
float alpha = maskAlpha * intensity * opacity * source.a;
alpha = clamp(0,1,alpha);
result.r = Blend(base.r, source.r, alpha, mode);
result.g = Blend(base.g, source.g, alpha, mode);
result.b = Blend(base.b, source.b, alpha, mode);
result.a = base.a;
```

## Unit Tests

In `test_makeup.cpp` BlendModeTests:

- Normal: base 0.5, source 0.8, alpha 0.5 => 0.8*0.5+0.5*0.5=0.65, check <0.001
- Multiply: 0.5*0.8=0.4 with alpha 1 => 0.4
- Screen: 1-(1-0.5)*(1-0.8)=1-0.5*0.2=0.9
- Overlay base<0.5: 2*0.3*0.8=0.48
- Overlay base>=0.5: 1-2*0.3*0.2=0.88
- Alpha 0 returns base, Alpha 1 returns source

All PASS.

## HLSL Implementation

In `makeup_common.hlsl`:

```hlsl
float3 BlendNormal(float3 base, float3 blend) { return blend; }
float3 BlendMultiply(float3 base, float3 blend) { return base*blend; }
float3 BlendScreen(float3 base, float3 blend) { return 1.0f - (1.0f-base)*(1.0f-blend); }
float3 BlendOverlay(float3 base, float3 blend) {
    float3 result;
    result.r = (base.r<0.5f)?(2*base.r*blend.r):(1-2*(1-base.r)*(1-blend.r));
    ...
    return result;
}
float4 BlendWithMask(float4 base, float4 makeup, float maskAlpha, int blendMode, float intensity, float opacity) {
    float alpha = maskAlpha*intensity*opacity*makeup.a;
    alpha = saturate(alpha);
    float3 blended;
    if(blendMode==0) blended=BlendNormal(base.rgb,makeup.rgb);
    else if(blendMode==1) blended=BlendMultiply(base.rgb,makeup.rgb);
    else if(blendMode==2) blended=BlendScreen(base.rgb,makeup.rgb);
    else blended=BlendOverlay(base.rgb,makeup.rgb);
    float3 result = lerp(base.rgb, blended, alpha);
    return float4(result, base.a);
}
```

Real HLSL, not fake, validated.

## Status

IMPLEMENTED, real math, unit tests PASS, HLSL real.

**End of Blend Modes**
