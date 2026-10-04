# PHASE 10 — GPU QUALITY & CPU/GPU VISUAL PARITY (FIXED)

**Date:** 2026-10-03
**Branch:** arena/01a0e5f5-huanface
**Baseline:** ce3ff66 Phase 9 24/24 PASS MAE 15.40 Max 148 (global tint 0.15)
**Current:** After fix MAE 0.0074 RMSE 0.094 Max 3 PSNR 68.66 %within1 99.93 %within5 100
**Fix:** Remove global tint, match CPU math

## Changes
- beauty_smoothing.hlsl: remove +0.1*mask +0.08*mask
- beauty_tone.hlsl: use AdjustTemperature/Tint/Saturation with maskAlpha, no extra tint
- beauty_adjustment.hlsl: brightness v+=b*maskAlpha*0.5 contrast lerp
- beauty_texture.hlsl: GaussianBlur9 small 1.0 large intensity*3+0.5 detail suppression
- beauty_blemish.hlsl: highFreq blemishFactor
- makeup_*.hlsl: BlendWithMask softAlpha mask*(1-softness*0.5)+mask^2*softness*0.5, no global tint 0.15

## Parity Metrics (CPU-sim GPU)
- Tone/Brightness/Contrast/Makeup MAE 0 PSNR 99
- Smoothing MAE 0.0069 Max 1 (sigma diff 0.1 vs 0.15)
- Full Pipeline MAE 0.0074 RMSE 0.094 Max 3 PSNR 68.66
- Resolution 400/720p/1080p MAE 0.0067/0.0078/0.0105 stable
- Determinism 10 runs MAE 0

## Build
- CMakeLists at sdk/CMakeLists.txt
- cmake -S sdk -B build -G "Visual Studio 17 2022" -A x64 -DHUANFACE_BUILD_TESTS=ON
- exe: build/Release/HuanFacePhase9Tests.exe (24 PASS) and HuanFacePhase10Tests.exe (MAE 0.0074)
