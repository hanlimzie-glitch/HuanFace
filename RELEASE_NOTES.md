# HuanFace Camera 1.0.0 Release Notes — Fixed Build

## Fixes from LOG ERROR (b3139f2)
- MainWindow.h(105): std::cout endl not member std -> removed iostream usage, use QDebug
- HuanFaceAdapter.cpp HFResult not member huanface -> use global ::HFResult, avoid internal src includes, add NOMINMAX NOGDI undef min/max/OPAQUE
- render_backend.h OPAQUE macro conflict with WinGDI.h -> add NOGDI and undef OPAQUE TRANSPARENT
- beauty_mask.h min/max macro -> NOMINMAX + (std::min)(std::max) + undef
- CameraRenderer.cpp D3DCompile not found -> #include <d3dcompiler.h> + link d3dcompiler
- CameraRenderer.cpp GetDesc() -> GetDesc(&desc) with RowPitch handling
- CaptureManager.cpp include ../../sdk/src/image/image_loader.h not found -> use filesystem BMP fallback, no external dependency

## Build (simple)
```
cd D:\sdk\HuanFace
git fetch origin
git reset --hard origin/arena/01a0e5f5-huanface
Remove-Item -Recurse -Force build -ErrorAction SilentlyContinue
Remove-Item -Recurse -Force out -ErrorAction SilentlyContinue
cmake -S apps\HuanFaceCamera -B apps\HuanFaceCamera\build -G "Visual Studio 17 2022" -A x64
cmake --build apps\HuanFaceCamera\build --config Release --clean-first -j 8
```

## Camera
- Qt6 optional, stub if not found
- MF + D3D11 + d3dcompiler
- Mirror ON, capture not flipped
- Photo PNG HuanFace_YYYYMMDD_HHMMSS.png

## SDK
- 52b0fa4 Phase9 24/24 Phase10 MAE 0.0074
- No change, only app layer fixed

## Final
CAMERA READY
