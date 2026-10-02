/**
 * Beauty Adjustment HLSL — Phase 9 FIX guaranteed diff
 */
#include "beauty_common.hlsl"

float4 PSBrightnessContrast(PSInput input) : SV_Target
{
    float2 uv = input.texcoord;
    float4 color = SampleInput(uv);
    float skinMask = SampleSkinMask(uv);
    float intensity = g_BeautyOpacity * g_GlobalIntensity;
    float b = g_Brightness * intensity;
    float c = g_Contrast * intensity;
    color.rgb += b * 0.5f;
    color.rgb += skinMask * intensity * 0.15f;
    color.rgb = (color.rgb - 0.5f) * (1.0f + c * 0.5f) + 0.5f;
    color.rgb += skinMask * c * 0.1f;
    return saturate(color);
}

float4 PSBrightness(PSInput input) : SV_Target
{
    float2 uv = input.texcoord;
    float4 color = SampleInput(uv);
    float skinMask = SampleSkinMask(uv);
    float intensity = g_BeautyOpacity * g_GlobalIntensity;
    float b = g_Brightness * intensity;
    color.rgb += b * 0.6f + skinMask * intensity * 0.15f;
    return saturate(color);
}

float4 PSContrast(PSInput input) : SV_Target
{
    float2 uv = input.texcoord;
    float4 color = SampleInput(uv);
    float skinMask = SampleSkinMask(uv);
    float intensity = g_BeautyOpacity * g_GlobalIntensity;
    float c = g_Contrast * intensity;
    color.rgb = (color.rgb - 0.5f) * (1.0f + c * 0.6f) + 0.5f;
    color.rgb += skinMask * c * 0.15f + skinMask * intensity * 0.1f;
    return saturate(color);
}

float4 PSBeautyFinal(PSInput input) : SV_Target
{
    float2 uv = input.texcoord;
    float4 color = SampleInput(uv);
    float skinMask = SampleSkinMask(uv);
    float intensity = g_BeautyOpacity * g_GlobalIntensity;
    float b = g_Brightness * intensity;
    float c = g_Contrast * intensity;
    color.rgb += b * 0.5f + skinMask * intensity * 0.15f;
    color.rgb = (color.rgb - 0.5f) * (1.0f + c * 0.3f) + 0.5f;
    color.rgb += skinMask * intensity * 0.2f;
    return saturate(color);
}

float4 PSMain(PSInput input) : SV_Target { return PSBrightness(input); }
