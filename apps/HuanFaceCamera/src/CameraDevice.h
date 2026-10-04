#pragma once
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#define NOGDI
#include <windows.h>
#include <mfapi.h>
#include <mfidl.h>
#include <mfreadwrite.h>
#include <atomic>
#include <thread>
#include <mutex>
#include <queue>
#include <vector>
#include <functional>

enum class CameraPixelFormat {
    BGRA = 0,
    RGBA = 1,
    YUY2 = 2,
    NV12 = 3
};

struct CameraFrame {
    std::vector<uint8_t> data;
    int width = 0;
    int height = 0;
    CameraPixelFormat format = CameraPixelFormat::BGRA;
    int64_t timestampMs = 0;
};

class CameraDevice {
public:
    CameraDevice();
    ~CameraDevice();

    bool Open(int deviceId, int width, int height);
    void Close();
    bool IsOpen() const { return isOpen_; }

    // Callback when frame captured
    void SetFrameCallback(std::function<void(const CameraFrame&)> cb);

    double GetFPS() const { return fps_; }

private:
    void CaptureThreadFunc();
    void SyntheticThreadFunc();

    bool isOpen_ = false;
    int deviceId_ = 0;
    int width_ = 1280;
    int height_ = 720;

    IMFMediaSource* pSource_ = nullptr;
    IMFSourceReader* pReader_ = nullptr;

    std::thread captureThread_;
    std::atomic<bool> running_{false};
    std::function<void(const CameraFrame&)> frameCallback_;

    std::atomic<double> fps_{0.0};
    int64_t frameCount_ = 0;
    int64_t lastFpsTime_ = 0;
};
