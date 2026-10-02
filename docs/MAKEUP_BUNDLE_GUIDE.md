# Makeup Bundle Guide — Phase 6 Full Makeup Renderer

**Branch:** arena/01a0e5f5-huanface  
**Phase:** 6 — Full Makeup Renderer ✅ IMPLEMENTED  
**Status:** Bundle integration works, test bundles PASS

## Overview

Uses `.hfbundle` ZIP open format defined in Phase 2, not FaceUnity encrypted binary.

```
.hfbundle (ZIP)
    ├── manifest.json
    ├── textures/
    ├── shaders/
    ├── masks/
    └── metadata/
```

## Manifest Example

```json
{
    "type": "makeup",
    "version": 1,
    "name": "simple_lip",
    "features": ["lip"],
    "parameters": {
        "makeup.lip.enabled": true,
        "makeup.lip.color": [1.0, 0.2, 0.3, 1.0],
        "makeup.lip.intensity": 0.8,
        "makeup.lip.opacity": 0.9,
        "makeup.lip.blendMode": "Normal"
    },
    "textures": [
        {
            "name": "lip",
            "path": "textures/lip.png",
            "format": "RGBA8",
            "width": 512,
            "height": 512
        }
    ],
    "shaders": [
        {
            "name": "lip",
            "path": "shaders/lip.hlsl",
            "type": "pixel"
        }
    ],
    "masks": [
        {
            "name": "lip",
            "type": "Lip",
            "feather": 1.0
        }
    ]
}
```

## Bundle Types for Phase 6

Test bundles created in `examples/bundles/`:

- **simple_lip**: Lip makeup only, manifest with lip color pink, intensity 0.8, texture lip.png 512x512, shader lip.hlsl
- **simple_foundation**: Foundation only, face mask, color 0.95,0.8,0.7, intensity 0.5
- **simple_blush**: Blush only, cheek masks, color 1,0.4,0.4, intensity 0.6
- **simple_eyebrow**: Eyebrow only, brow masks, color 0.3,0.2,0.15
- **simple_eyeliner**: Eyeliner only, eye masks, color 0.1,0.1,0.1, thickness 2.0
- **simple_eyelash**: Eyelash only, eye contour, intensity 0.8, length 1.0
- **simple_eyeshadow**: Eyeshadow only, eyelid masks, color 0.8,0.4,0.6, blend Multiply
- **simple_pupil**: Pupil only, eye masks, color 0.2,0.5,0.8, irisEnhancement 0.5

Each bundle has:

- `manifest.json`: type makeup, version 1, features list, parameters
- `textures/`: PNG clean-room, not FaceUnity asset, e.g., lip.png, blush.png, eyeshadow.png — created via struct+zlib, no PIL, no protected extraction
- `shaders/`: HLSL/GLSL clean-room, e.g., lip.glsl, lip.hlsl, real HLSL with blend logic, not fake
- `metadata/thumbnail.png`: 256x256 preview

## Bundle Integration Flow

```
Load Bundle (.hfbundle ZIP via BundleReader + miniz/zlib)
    ↓
Parse manifest.json (HFManifest)
    ↓
Validate (type, version, features, textures, shaders)
    ↓
Resolve Dependencies (ResourceManager cache)
    ↓
Load Resources (textures via ImageLoader PNG, shaders via file read)
    ↓
Create Runtime Objects (MakeupMaskGenerator, FullMakeupEngine, params from manifest)
    ↓
Bind Face Data (HFTrackingData from ProductionFaceTracker REAL ML)
    ↓
Render via IRenderBackend (CPU reference + D3D11 GPU real HLSL)
```

## Clean-Room Requirement

- No FaceUnity bundle reading, no decrypting F3 5B 06 12 encrypted bundles
- No FaceUnity asset extraction, no protected asset
- All bundles in `examples/bundles/` are clean-room, created via `tools/generate_example_bundles.py` and `huanface_bundle_packer.py`, ZIP open format, not proprietary
- Textures: simple color PNGs generated via Python struct+zlib, not extracted from FaceUnity
- Shaders: simple HLSL/GLSL with lerp and blend, not copied from FaceUnity shader cache .v2 binary D3D11 bytecode
- Manifest: HuanFace spec, not FaceUnity format

## Testing

BundleMakeupTests:

- Each bundle load, validate, apply, render, unload without crash
- `examples/bundles/simple_lip_store.hfbundle` exists, 6.1KB, ZIP STORE
- BundleReader should reject path traversal ../../etc/passwd (security)
- Future version 999.0 parsed (forward compat)
- Manifest format HuanFaceBundle, version 1.0, type makeup

All PASS 17/17.

## Tools

- `tools/generate_example_bundles.py`: Generates simple_* bundles clean-room PNG via struct+zlib
- `tools/huanface_bundle_packer.py`: Packs directory to .hfbundle ZIP (STORE+DEFLATE)
- `tools/huanface_bundle_inspector.py`: Inspects .hfbundle manifest, textures, shaders
- `tools/repack_store.py`: Repacks bundles with STORE

All tools only for HuanFace ZIP open, not FaceUnity encrypted.

## Status

IMPLEMENTED, bundle integration works, test bundles PASS, clean-room, no FaceUnity/OBS.

**End of Bundle Guide**
