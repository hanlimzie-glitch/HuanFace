#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#define NOGDI
#include "CameraDevice.h"
#include <chrono>
#include <cstring>

#ifdef _WIN32
#include <mfapi.h>
#include <mfidl.h>
#include <mfreadwrite.h>
#endif

CameraDevice::CameraDevice() {}
CameraDevice::~CameraDevice() { Close(); }

void CameraDevice::SetFrameCallback(std::function<void(const CameraFrame&)> cb) {
    frameCallback_ = cb;
}

bool CameraDevice::Open(int deviceId, int width, int height) {
    Close();
    deviceId_ = deviceId;
    width_ = width;
    height_ = height;

#ifdef _WIN32
    IMFAttributes* pAttributes = nullptr;
    HRESULT hr = MFCreateAttributes(&pAttributes, 1);
    if (FAILED(hr)) return false;
    hr = pAttributes->SetGUID(MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE, MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE_VIDCAP_GUID);
    if (FAILED(hr)) { pAttributes->Release(); return false; }

    IMFActivate** ppDevices = nullptr;
    UINT32 count = 0;
    hr = MFEnumDeviceSources(pAttributes, &ppDevices, &count);
    pAttributes->Release();
    if (FAILED(hr) || count == 0) return false;
    if (deviceId >= (int)count) deviceId = 0;

    hr = ppDevices[deviceId]->ActivateObject(IID_PPV_ARGS(&pSource_));
    for (UINT32 i = 0; i < count; ++i) ppDevices[i]->Release();
    CoTaskMemFree(ppDevices);
    if (FAILED(hr) || !pSource_) return false;

    hr = MFCreateSourceReaderFromMediaSource(pSource_, nullptr, &pReader_);
    if (FAILED(hr)) { pSource_->Release(); pSource_=nullptr; return false; }

    // Try set RGB32 preferred
    IMFMediaType* pType = nullptr;
    hr = MFCreateMediaType(&pType);
    if (SUCCEEDED(hr)) {
        pType->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Video);
        pType->SetGUID(MF_MT_SUBTYPE, MFVideoFormat_RGB32);
        pType->SetUINT64(MF_MT_FRAME_SIZE, ((UINT64)width<<32)| (UINT64)height);
        // Try set, ignore fail -> will get native YUY2/NV12
        pReader_->SetCurrentMediaType((DWORD)MF_SOURCE_READER_FIRST_VIDEO_STREAM, nullptr, pType);
        pType->Release();
    }

    running_ = true;
    isOpen_ = true;
    captureThread_ = std::thread(&CameraDevice::CaptureThreadFunc, this);
#else
    running_ = true;
    isOpen_ = true;
    captureThread_ = std::thread(&CameraDevice::SyntheticThreadFunc, this);
#endif
    return true;
}

void CameraDevice::Close() {
    running_ = false;
    if (captureThread_.joinable()) captureThread_.join();

#ifdef _WIN32
    if (pReader_) { pReader_->Release(); pReader_=nullptr; }
    if (pSource_) { pSource_->Release(); pSource_=nullptr; }
#endif
    isOpen_ = false;
}

void CameraDevice::CaptureThreadFunc() {
#ifdef _WIN32
    auto lastTime = std::chrono::steady_clock::now();
    int frames = 0;
    while (running_) {
        IMFSample* pSample = nullptr;
        DWORD streamIndex, flags;
        LONGLONG timestamp;
        HRESULT hr = pReader_->ReadSample((DWORD)MF_SOURCE_READER_FIRST_VIDEO_STREAM, 0, &streamIndex, &flags, &timestamp, &pSample);
        if (FAILED(hr) || !pSample) {
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
            continue;
        }
        if (flags & MF_SOURCE_READERF_ENDOFSTREAM) { pSample->Release(); break; }

        IMFMediaBuffer* pBuffer = nullptr;
        hr = pSample->ConvertToContiguousBuffer(&pBuffer);
        if (SUCCEEDED(hr) && pBuffer) {
            BYTE* pData = nullptr;
            DWORD maxLen, curLen;
            hr = pBuffer->Lock(&pData, &maxLen, &curLen);
            if (SUCCEEDED(hr) && pData) {
                CameraFrame frame;
                frame.width = width_;
                frame.height = height_;
                frame.format = CameraPixelFormat::BGRA;
                frame.data.assign(pData, pData + curLen);
                frame.timestampMs = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();

                // Detect format from media type if possible — for now assume BGRA, fallback handled in adapter
                if (frameCallback_) frameCallback_(frame);

                frames++;
                auto now = std::chrono::steady_clock::now();
                auto diff = std::chrono::duration_cast<std::chrono::milliseconds>(now - lastTime).count();
                if (diff >= 1000) {
                    fps_ = frames * 1000.0 / diff;
                    frames = 0;
                    lastTime = now;
                }

                pBuffer->Unlock();
            }
            pBuffer->Release();
        }
        pSample->Release();
    }
#endif
}

void CameraDevice::SyntheticThreadFunc() {
    // Checker pattern 30 FPS for Linux CI
    auto lastTime = std::chrono::steady_clock::now();
    int frames = 0;
    int counter = 0;
    while (running_) {
        CameraFrame frame;
        frame.width = width_;
        frame.height = height_;
        frame.format = CameraPixelFormat::BGRA;
        frame.data.resize(width_*height_*4);
        for (int y=0;y<height_;++y){
            for (int x=0;x<width_;++x){
                int idx=(y*width_+x)*4;
                bool checker = ((x/40 + y/40 + counter/10)%2)==0;
                frame.data[idx+0]= checker? 35:220; // B
                frame.data[idx+1]= checker? 220:35; // G
                frame.data[idx+2]= 220; // R
                frame.data[idx+3]= 255;
            }
        }
        frame.timestampMs = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
        if (frameCallback_) frameCallback_(frame);
        counter++;
        frames++;
        auto now = std::chrono::steady_clock::now();
        auto diff = std::chrono::duration_cast<std::chrono::milliseconds>(now - lastTime).count();
        if (diff >= 1000) {
            fps_ = frames*1000.0/diff;
            frames=0;
            lastTime=now;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(33));
    }
}
