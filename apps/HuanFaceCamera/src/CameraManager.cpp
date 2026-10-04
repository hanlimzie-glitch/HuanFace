#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#define NOGDI
#include "CameraManager.h"
#include <mfapi.h>
#include <mfidl.h>
#include <mfreadwrite.h>
#include <comdef.h>
#include <string>
#include <vector>

#pragma comment(lib, "mf")
#pragma comment(lib, "mfplat")
#pragma comment(lib, "mfreadwrite")
#pragma comment(lib, "mfuuid")

CameraManager::CameraManager() {}
CameraManager::~CameraManager() { Shutdown(); }

bool CameraManager::Initialize() {
    if (initialized_) return true;
    HRESULT hr = MFStartup(MF_VERSION);
    if (FAILED(hr)) return false;
    initialized_ = true;
    return true;
}

void CameraManager::Shutdown() {
    if (initialized_) {
        MFShutdown();
        initialized_ = false;
    }
}

std::vector<CameraInfo> CameraManager::EnumerateDevices() {
    std::vector<CameraInfo> devices;
#ifdef _WIN32
    if (!initialized_) {
        if (!Initialize()) return devices;
    }

    IMFAttributes* pAttributes = nullptr;
    HRESULT hr = MFCreateAttributes(&pAttributes, 1);
    if (FAILED(hr)) return devices;

    hr = pAttributes->SetGUID(MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE, MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE_VIDCAP_GUID);
    if (FAILED(hr)) { pAttributes->Release(); return devices; }

    IMFActivate** ppDevices = nullptr;
    UINT32 count = 0;
    hr = MFEnumDeviceSources(pAttributes, &ppDevices, &count);
    pAttributes->Release();
    if (FAILED(hr)) return devices;

    for (UINT32 i = 0; i < count; ++i) {
        CameraInfo info;
        info.id = (int)i;

        WCHAR* szFriendlyName = nullptr;
        UINT32 cchName = 0;
        hr = ppDevices[i]->GetAllocatedString(MF_DEVSOURCE_ATTRIBUTE_FRIENDLY_NAME, &szFriendlyName, &cchName);
        if (SUCCEEDED(hr) && szFriendlyName) {
            char szName[512] = {0};
            WideCharToMultiByte(CP_UTF8, 0, szFriendlyName, -1, szName, 512, nullptr, nullptr);
            info.name = szName;
            CoTaskMemFree(szFriendlyName);
        } else {
            info.name = "Camera " + std::to_string(i);
        }

        WCHAR* szSymbolicLink = nullptr;
        UINT32 cchLink = 0;
        hr = ppDevices[i]->GetAllocatedString(MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE_VIDCAP_SYMBOLIC_LINK, &szSymbolicLink, &cchLink);
        if (SUCCEEDED(hr) && szSymbolicLink) {
            char szLink[1024] = {0};
            WideCharToMultiByte(CP_UTF8, 0, szSymbolicLink, -1, szLink, 1024, nullptr, nullptr);
            info.symbolicLink = szLink;
            CoTaskMemFree(szSymbolicLink);
        }

        devices.push_back(info);
        ppDevices[i]->Release();
    }
    CoTaskMemFree(ppDevices);
#else
    // Linux stub synthetic
    devices.push_back({ "Synthetic Camera", "synthetic", 0 });
#endif
    return devices;
}

std::vector<Resolution> CameraManager::GetSupportedResolutions() const {
    return { {640,480}, {1280,720}, {1920,1080} };
}
