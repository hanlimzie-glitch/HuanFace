#pragma once
// Media Foundation camera enumeration — fixed NOMINMAX/NOGDI before windows.h
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#define NOGDI
#include <windows.h>
#include <mfapi.h>
#include <mfidl.h>
#include <mfreadwrite.h>
#include <string>
#include <vector>

struct CameraInfo {
    std::string name;
    std::string symbolicLink;
    int id;
};

struct Resolution {
    int width;
    int height;
};

class CameraManager {
public:
    CameraManager();
    ~CameraManager();

    bool Initialize();
    void Shutdown();
    std::vector<CameraInfo> EnumerateDevices();
    std::vector<Resolution> GetSupportedResolutions() const;

private:
    bool initialized_ = false;
};
