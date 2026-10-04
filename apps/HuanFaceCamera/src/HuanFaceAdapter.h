#pragma once
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#define NOGDI
#include <windows.h>
#undef min
#undef max
#undef OPAQUE
#undef TRANSPARENT

#include <vector>
#include <mutex>
#include <string>
#include <cstdint>

// Forward declare simple params — no internal SDK headers to avoid macro conflicts
struct BeautyParams {
    float smoothing = 0.5f;   // 0-1
    float brightness = 0.0f;  // -1 to 1
    float contrast = 0.0f;    // -1 to 1
    float retouch = 0.5f;     // 0-1
};

struct MakeupParams {
    float lip = 0.0f;
    float blush = 0.0f;
    float eyebrow = 0.0f;
    float eyeliner = 0.0f;
    float eyelash = 0.0f;
    float eyeshadow = 0.0f;
    float pupil = 0.0f;
};

struct ProcessedFrame {
    std::vector<uint8_t> rgba; // RGBA8
    int width = 0;
    int height = 0;
    int faceCount = 0;
};

class HuanFaceAdapter {
public:
    HuanFaceAdapter();
    ~HuanFaceAdapter();

    bool Initialize();
    void Shutdown();

    // Conversion helpers
    static std::vector<uint8_t> ConvertBGRAtoRGBA(const uint8_t* data, int w, int h);
    static std::vector<uint8_t> ConvertYUY2toRGBA(const uint8_t* data, int w, int h);
    static std::vector<uint8_t> ConvertNV12toRGBA(const uint8_t* yPlane, const uint8_t* uvPlane, int w, int h, int strideY, int strideUV);

    // Process frame: input BGRA/YUY2/NV12 -> RGBA -> beauty/makeup -> output RGBA
    ProcessedFrame ProcessFrame(const uint8_t* data, int w, int h, int format, bool mirror);

    void SetBeautyParams(const BeautyParams& p);
    void SetMakeupParams(const MakeupParams& p);
    void SetMirror(bool m) { mirror_ = m; }

    int GetFaceCount() const { return faceCount_; }

private:
    std::vector<uint8_t> ApplyBeauty(const std::vector<uint8_t>& rgba, int w, int h);
    std::vector<uint8_t> ApplyMakeup(const std::vector<uint8_t>& rgba, int w, int h);

    BeautyParams beauty_;
    MakeupParams makeup_;
    bool mirror_ = true;
    std::mutex mutex_;
    int faceCount_ = 0;
    bool initialized_ = false;
};
