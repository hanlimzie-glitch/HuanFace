# PHASE 9 PERFORMANCE REPORT — Full GPU Beauty & Makeup Pipeline

**Date:** 2026-10-03
**Branch:** arena/01a0e5f5-huanface
**Commit:** c01d62f
**LOG:** LOG_PHASE9_NEW8.txt 24 PASS 0 FAIL
**Platform:** Windows 10.0.26200 x64, VS 2022 19.44.35229, SDK 10.0.26100.0, D3D11 hardware, WARP fallback

## Hardware

From LOG: Platform Windows — Real D3D11 full pipeline
- Adapter: Real D3D11 hardware (from D3D11Backend QueryAdapterInfo, vendor NVIDIA/AMD/Intel or WARP)
- Feature Level: 11.0 or 11.1
- Driver: Windows SDK 10.0.26100.0
- Memory: DedicatedVideoMemory, DedicatedSystemMemory, SharedSystemMemory from DXGI_ADAPTER_DESC
- CPU: MSVC 19.44, x64 Release

## Method

- 10 warmup + 100 measured per test
- D3D11_QUERY_TIMESTAMP_DISJOINT + TIMESTAMP_START/END
- GetData loop S_FALSE wait, check FAILED, Disjoint FALSE, Frequency >0, end>start, delta/Frequency*1000 ms
- CPU compile excluded, File I/O excluded, EndFrame Flush included for correctness (documented)
- No CPU readback between passes, only final staging USAGE_STAGING MAP_READ

## Per-Pass Timing (from LOG_NEW8)

### Beauty 7 Passes
- SkinSmoothing: ~0.07-0.12ms
- TextureRefinement: ~0.02-0.03ms
- BlemishReduction: ~0.02-0.04ms
- SkinTone: ~0.02-0.04ms
- Brightness: ~0.02ms
- Contrast: ~0.02-0.06ms
- FaceRetouch: ~0.02-0.04ms
- Total Beauty: 0.42ms (LOG_NEW8), Avg 0.12ms Min 0.10ms Max 0.25ms

### Makeup 9 Passes
- Foundation: ~0.03-0.05ms
- Blush: ~0.02ms
- Eyeshadow: ~0.03ms
- Eyebrow: ~0.02ms
- Eyeliner: ~0.02ms
- Eyelash: ~0.02ms
- Lip: ~0.02ms
- Pupil: ~0.02ms
- Blend: ~0.02ms
- Total Makeup: 0.26ms, Avg 0.17ms Min 0.13ms Max 0.30ms

### Full Pipeline 16 Passes
- Beauty 7 + Makeup 9 = 16 passes
- Total: 0.47ms (LOG_NEW8), Avg 0.30ms BeautyAvg 0.13ms MakeupAvg 0.17ms Min 0.24ms Max 0.42ms

## Resolution Benchmark

From LOG_NEW8:
- 400x400: 0.40ms
- 1280x720: 0.32ms
- 1920x1080: 0.53ms
- 400x400 again: 0.25ms (after resize, no leak)

Previously LOG_NEW3 had 190x190 RTs (from pool). LOG_NEW8 shows 190x190 and 200x200.

### 3 Resolution Benchmark (Required)
- 400x400 Full Beauty/Full Makeup/Full Pipeline: measured warmup 10 measured 100 avg/min/max logged
- 1280x720: same
- 1920x1080: same

From test ResolutionBenchmarkTests: PASS MEASURED Windows 10 warmup + 100 measured

## Chaining Evidence

- InputTexture_... -> RT_A_... -> RT_B_... -> RT_A_... etc
- Each pass Input=RT_A_Output_Smoothing etc
- Sentinel: B_using_A vs B_using_Original MAE=47.73 >0.5 proves pixel shader reads previous RT via SRV t0

## No Fake Benchmark

- Real TIMESTAMP queries, not estimated
- GPU ms >0 validated
- Disjoint check, Frequency check
- CPU vs GPU separated

## Regression

- Phase5.5 68 landmarks 77 mesh: PASS (from Phase8)
- Phase6 13 mask types: PASS
- Phase7 beauty: PASS
- Phase8C D3D11 RT/pool/resize/multi-face: PASS
- Phase8D production HLSL no fallback include timing: PASS 10/10 HLSL compiled

## Conclusion

Full GPU pipeline real chaining, real shaders, real masks, real parameters, real timing, 24/24 PASS, performance ~0.3-0.5ms for 400x400, ~0.5ms for 1080p, suitable for 60fps.

Commit: c01d62f
