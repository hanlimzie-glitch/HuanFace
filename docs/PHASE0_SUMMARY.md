# Phase 0 Summary — Repository Audit

**Date:** 2026-09-28
**Status:** DONE

## Repository analyzed: 965 files

- Total size: ~655 MB
- File types: .png 341, .bundle 267, .v2 223, .dll 37, .log 31, .ini 19, .exe 11, .svg 8, .dat 6, .json 5, .effect 4, etc.

## Potential bundles: 267

- All encrypted, magic F3 5B 06 12 primary (70%) or variant (30%)
- Entropy 7.7-7.85 = encrypted
- Types: face_beautification 5.2MB, face_makeup 702KB, body_slim 16KB, background_blur 125KB, ai_face_processor 23MB, ai_human_processor 42MB, custom makeup ~60, style makeup 24, hair 2, special-effects 150+
- No DRM bypass, clean-room approach

## Potential textures: 351 + unknown inside bundles

- 341 PNG thumbs (hair, filters, UI)
- 8 SVG icons
- 2 GIF loading
- Plus unknown inside bundles: eyepupil.png, eyeliner.png, eyelash.png, brow.png, eye.png, lip.png, gloss_lut.png, etc. (from logs)

## Potential shaders: 4 OBS .effect + 223 .v2 cache + unknown inside bundles

- OBS .effect: default.effect 9.3KB, format_conversion.effect 13KB — color space conversion (sRGB, Rec709, Rec2020, PQ, HLG, tonemap), NOT FaceUnity shaders
- .v2: OBS D3D11 shader cache, binary
- Inside bundles: MakeupFilterPassNAMA, MakeupWarpNAMA, lip_mask, etc. (from DLL strings)

## Potential binaries: 48

- 37 DLL + 11 EXE
- Core: CNamaSDK.dll 19.5MB (FaceUnity SDK, 200+ fu* APIs, DecryptObfuscatedPackage, VerifySignature, mbedtls AES)
- fuai.dll 29MB (AI inference, FaceMeshV2, HumanProcessor, HandDetector, TFLite)
- obs-cam-beauty.dll 63KB (OBS plugin bridge)
- obsplus.dll 456KB (OBSPlus host)
- Spout DLLs (texture sharing)
- Store DLLs (marketplace)

## Unknown formats: 273

- 267 bundles + 6 config.dat encrypted
- Config.dat: 11KB-80KB, encrypted index for beauty presets
- Need decryption via legitimate SDK API (no bypass) or clean-room replacement

## Extractable resources: ~400

- PNG, SVG, GIF, INI, JSON, .effect, Caffe model (MobileNetSSD 22MB + prototxt)
- DLL strings, logs
- No bundle internal without decryption

## Major dependencies: 8

- FaceUnity NamaSDK (proprietary)
- FaceUnity FUAI (proprietary)
- OBS Studio (GPL)
- Qt5/Qt6 (UI)
- mbedTLS (embedded crypto)
- MobileNetSSD Caffe (fallback detection)
- Spout (BSD, texture sharing)
- OpenGL/D3D11 (rendering)

## Confidence: HIGH / MEDIUM / LOW

- Repository structure: HIGH
- Binary purpose: HIGH
- Bundle encryption existence: HIGH
- Bundle internal structure: MEDIUM (inferred from logs + FaceUnity docs, not direct extraction)
- Makeup params (tex_*, makeup_intensity_*): HIGH (from DLL strings + logs)
- Beauty params (HeavyBlur, ColorLevel, etc.): HIGH (from INI + DLL)
- Config.dat format: LOW-MEDIUM
- Shader format: MEDIUM-HIGH (OBS effects clear, FaceUnity shaders inside bundles unknown)

## Recommended next phase: Phase 1 — Asset & Binary Analysis

- Build inspector tools (DONE: bundle_inspector, asset_inspector, texture_inspector, metadata_inspector)
- Document all fu* APIs into API_REFERENCE.md (TODO)
- Parse INI fully, extract beauty/makeup param list (DONE via metadata_inspector)
- Analyze config.dat encryption (TODO)
- Attempt to run beauty.exe --help in sandbox (TODO)
- Create REVERSE_ENGINEERING.md with Observe->Hypothesis->Experiment->Evidence->Conclusion (DONE)

## Files Generated

- docs/REPOSITORY_AUDIT.md (22KB)
- docs/BINARY_ANALYSIS.md (14KB)
- docs/ASSET_FORMATS.md (12KB)
- docs/BUNDLE_FORMAT.md (18KB)
- docs/ROADMAP.md (17KB)
- docs/REVERSE_ENGINEERING.md (log)
- docs/ARCHITECTURE.md (draft)
- docs/API_DESIGN.md, RUNTIME_DESIGN.md, RENDERING.md, MAKEUP_ENGINE.md, BEAUTY_ENGINE.md, TESTING.md (placeholders)
- analysis/asset_catalog.json (177KB, 965 entries)
- analysis/binary_catalog.json (107KB, 315 entries)
- analysis/file_type_statistics.json
- analysis/dependency_graph.json
- tools/bundle_inspector.py, asset_inspector.py, texture_inspector.py, metadata_inspector.py

## Legal & Compliance

- No DRM bypass, no license bypass, no encryption bypass
- No proprietary source copying
- Clean-room design for HuanFace bundle format (ZIP-based open format)
- All analysis via static strings, headers, entropy, logs

## Next Steps

- Phase 1: Finish inspector tools, document fu* APIs, analyze config.dat
- Phase 2: Design HuanFace bundle format (ZIP-based), build packer/unpacker
- Phase 3: Runtime architecture final design
- Phase 4: Minimal prototype (Input Image -> Face Detection -> Landmark -> Face Mesh -> Basic Texture Overlay -> Output Image)

END OF PHASE 0
