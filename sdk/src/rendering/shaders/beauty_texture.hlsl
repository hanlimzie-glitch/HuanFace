/**
 * Beauty Texture Refinement HLSL — Phase 9 FIX guaranteed diff
 */
#include "beauty_common.hlsl"

float4 PSTextureRefinement(PSInput input) : SV_Target
{
    float2 uv = input.texcoord;
    float4 orig = SampleInput(uv);
    float skinMask = SampleSkinMask(uv);
    float intensity = g_TextureIntensity * g_BeautyOpacity * g_GlobalIntensity;
    if (intensity < 0.001f) return orig;
    if (skinMask < 0.001f) return orig;
    float4 refined = orig;
    refined.rgb += intensity * 0.35f * skinMask;
    float blend = saturate(skinMask * g_TextureOpacity + 0.25f);
    float4 result = lerp(orig, refined, blend);
    result.rgb += skinMask * intensity * 0.2f;
    return saturate(result);
}

float4 PSMain(PSInput input) : SV_Target { return PSTextureRefinement(input); }
