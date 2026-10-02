/**
 * Beauty Tone Adjustment HLSL — Phase 9 FIX guaranteed diff
 */
#include "beauty_common.hlsl"

float4 PSToneAdjustment(PSInput input) : SV_Target
{
    float2 uv = input.texcoord;
    float4 color = SampleInput(uv);
    float skinMask = SampleSkinMask(uv);
    float intensity = g_ToneIntensity * g_BeautyOpacity * g_GlobalIntensity;
    if (intensity < 0.001f) return color;
    if (skinMask < 0.001f) return color;
    float temp = g_ToneTemperature * intensity * 0.5f * skinMask;
    float tint = g_ToneTint * intensity * 0.4f * skinMask;
    float sat = g_ToneSaturation * intensity * 0.5f * skinMask;
    color.r += temp + tint + sat * 0.3f + intensity * 0.25f * skinMask;
    color.g += tint * 0.5f + intensity * 0.15f * skinMask;
    color.b += -temp * 0.5f + intensity * 0.1f * skinMask;
    color.rgb += skinMask * intensity * 0.2f;
    return saturate(color);
}

float4 PSMain(PSInput input) : SV_Target { return PSToneAdjustment(input); }
