#pragma once
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#define NOGDI
#include <windows.h>
#undef min
#undef max
#undef OPAQUE
#undef TRANSPARENT

#include <d3d11.h>
#include <dxgi.h>
#include <vector>
#include <cstdint>

class CameraRenderer {
public:
    CameraRenderer();
    ~CameraRenderer();

    bool Initialize(void* hwnd, int width, int height);
    void Shutdown();

    bool CreateInputTexture(int width, int height);
    bool UpdateInputTexture(const uint8_t* rgba, int width, int height);
    bool RenderFrame(bool mirror);
    void OnResize(int width, int height);

private:
    bool CreateShaders();
    bool CreateQuad();

    HWND hwnd_ = nullptr;
    int width_ = 0;
    int height_ = 0;

    ID3D11Device* device_ = nullptr;
    ID3D11DeviceContext* context_ = nullptr;
    IDXGISwapChain* swapChain_ = nullptr;
    ID3D11RenderTargetView* rtv_ = nullptr;
    ID3D11Texture2D* inputTexture_ = nullptr;
    ID3D11ShaderResourceView* inputSRV_ = nullptr;
    ID3D11SamplerState* sampler_ = nullptr;
    ID3D11VertexShader* vs_ = nullptr;
    ID3D11PixelShader* ps_ = nullptr;
    ID3D11InputLayout* inputLayout_ = nullptr;
    ID3D11Buffer* vertexBuffer_ = nullptr;
    ID3D11Buffer* indexBuffer_ = nullptr;
    ID3D11Buffer* psConstantBuffer_ = nullptr;
    ID3D11RasterizerState* rasterState_ = nullptr;

    bool initialized_ = false;
};
