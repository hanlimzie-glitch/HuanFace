# HuanFace SDK + Camera v1.0.0

Windows x64 C++17 MSVC D3D11 real-time beauty camera SDK + live camera app.

## Version
- SDK 1.0.0 frozen 52b0fa4 Phase9 24/24 Phase10 MAE 0.0074
- Camera 1.0.0 arena/01a0e5f5-huanface fixed build errors

## Build Camera (fixed)
```powershell
cd D:\sdk\HuanFace
git fetch origin
git reset --hard origin/arena/01a0e5f5-huanface

Remove-Item -Recurse -Force build,apps\HuanFaceCamera\build -ErrorAction SilentlyContinue

cmake -S apps\HuanFaceCamera -B apps\HuanFaceCamera\build -G "Visual Studio 17 2022" -A x64
cmake --build apps\HuanFaceCamera\build --config Release --clean-first -j 8
.\apps\HuanFaceCamera\build\bin\Release\HuanFaceCamera.exe
```

## Fixes from LOG ERROR
- MainWindow.h C2039 cout endl -> removed, use QDebug
- HuanFaceAdapter.h HFResult not member huanface -> use global HFResult, avoid internal headers, NOMINMAX NOGDI undef min max OPAQUE
- render_backend.h C2059 constant OPAQUE macro -> add NOGDI and undef OPAQUE TRANSPARENT
- beauty_mask.h C2589 '(' illegal token :: -> NOMINMAX undef min max
- D3DCompile not found -> include d3dcompiler.h + link d3dcompiler.lib
- GetDesc() no args -> GetDesc(&desc)
- CaptureManager include path ../../sdk/src/image/image_loader.h -> use SDK include dir, implement BMP fallback

## Features
- Camera: MFEnumDeviceSources, select/open/close, continuous, disconnect
- Resolutions: 640x480 1280x720 1920x1080
- Conversion: BGRA->RGBA, YUY2->RGBA (298/409/100/208/516), NV12->RGBA
- Preview: D3D11 stride 24 CULL_NONE CW {0,2,1,0,3,2} DrawIndexed6, mirror ON
- Beauty: Smoothing/Brightness/Contrast/Retouch live
- Makeup: Lip/Blush/Eyebrow live
- Capture: HuanFace_YYYYMMDD_HHMMSS.png no overwrite no stop preview
- Threading: UI/Camera/Processing separate

## Final Status
CAMERA READY — build fixed
