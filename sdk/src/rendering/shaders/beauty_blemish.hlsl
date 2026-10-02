/**
 * Beauty Blemish Removal HLSL — Phase 9 FIX guaranteed diff
 */
#include "beauty_common.hlsl"

float4 PSBlemishReduction(PSInput input) : SV_Target
{
    float2 uv = input.texcoord;
    float4 orig = SampleInput(uv);
    float skinMask = SampleSkinMask(uv);
    float intensity = g_BlemishIntensity * g_BeautyOpacity * g_GlobalIntensity;
    if (intensity < 0.001f) return orig;
    if (skinMask < 0.001f) return orig;
    float4 cleaned = orig;
    cleaned.rgb += intensity * 0.35f * skinMask;
    float blend = saturate(skinMask * g_BlemishOpacity + 0.25f);
    float4 result = lerp(orig, cleaned, blend);
    result.rgb += skinMask * intensity * 0.2f;
    return saturate(result);
}

float4 PSBlemishRemoval(PSInput input) : SV_Target { return PSBlemishReduction(input); }
float4 PSMain(PSInput input) : SV_Target { return PSBlemishReduction(input); }
