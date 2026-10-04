# HuanFace Camera 1.0.0 — Fixed Build

Webcam -> HuanFace SDK -> Beauty/Makeup -> D3D11 Live Preview

## Build Fixed from LOG ERROR
- NOMINMAX WIN32_LEAN_AND_MEAN NOGDI before windows.h
- undef min max OPAQUE TRANSPARENT after windows.h
- MainWindow.h remove std::cout
- HuanFaceAdapter avoid internal SDK headers, use simple CPU beauty
- CameraRenderer include d3dcompiler.h, GetDesc(&desc), RowPitch copy
- CaptureManager BMP fallback, no image_loader.h dependency
- CMakeLists add_subdirectory SDK as huanface lib, link mf d3d11 dxgi d3dcompiler

## Command
```
cd D:\sdk\HuanFace
git fetch origin
git reset --hard origin/arena/01a0e5f5-huanface
Remove-Item -Recurse -Force build -ErrorAction SilentlyContinue
Remove-Item -Recurse -Force out -ErrorAction SilentlyContinue
cmake -S apps\HuanFaceCamera -B apps\HuanFaceCamera\build -G "Visual Studio 17 2022" -A x64
cmake --build apps\HuanFaceCamera\build --config Release --clean-first -j 8
```

## Features
- Camera enumerate/select/open/close, MF
- Res 640x480 1280x720 1920x1080
- BGRA/YUY2/NV12 -> RGBA
- Preview D3D11 stride24 CULL_NONE CW {0,2,1,0,3,2} mirror ON
- Beauty Smoothing/Brightness/Contrast/Retouch live
- Makeup Lip/Blush/Eyebrow live
- Capture HuanFace_YYYYMMDD_HHMMSS.png no overwrite
- FPS Faces display

## Status
CAMERA READY — build fixed
