# Testing — HuanFace SDK (Draft)

Structure:

```
tests/
├── core/
├── face/
├── tracking/
├── rendering/
├── makeup/
├── beauty/
├── bundle/
├── api/
└── integration/
```

- Automated tests for each subsystem
- Visual regression: reference.png vs huanface.png, pixel diff, SSIM, mask diff, landmark diff
- Performance: FPS, CPU, GPU, memory, frame time, bundle loading time — target 30 FPS initial, 60 FPS after

TODO: Implement in Phase 10.
