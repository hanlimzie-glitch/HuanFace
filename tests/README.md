# HuanFace Tests — Phase 2 Placeholder

**Status:** Placeholder, no implementation in Phase 2

**Future structure (Phase 10 Testing & Validation):**

```
tests/
├── core/ (runtime, resource_manager, config, clock)
├── face/ (face_tracker, mediapipe_tracker, onnx_tracker, face_mesh)
├── makeup/ (lip, blush, eyeshadow, foundation, pipeline, mask_generator)
├── beauty/ (skin_filter, face_shape, etc.)
├── bundle/ (bundle_parser, resource_resolver, packer/unpacker/inspector)
├── rendering/ (d3d11_backend, opengl_backend, texture, shader, mesh)
├── platform/ (windows_camera_mf, filesystem, etc.)
├── api/ (c_api, hpp wrapper)
└── integration/ (full pipeline)
```

**Phase 2:** Only architecture spec, tooling, example bundles, docs, no tests implementation.

**Implementation starts Phase 4+ minimal prototype, Phase 10 full testing.**

**End**
