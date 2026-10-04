#pragma once
#include <string>
#include <vector>
#include <cstdint>

class CaptureManager {
public:
    CaptureManager();
    ~CaptureManager();

    // Save RGBA as PNG, returns path, no overwrite
    std::string SavePhoto(const uint8_t* rgba, int width, int height, std::string& outError);
    static std::string GetCaptureDir();
};
