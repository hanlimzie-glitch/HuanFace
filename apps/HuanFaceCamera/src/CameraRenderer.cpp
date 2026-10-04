#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#define NOGDI
#include "CameraRenderer.h"
#undef min
#undef max
#undef OPAQUE
#undef TRANSPARENT

#include <d3d11.h>
#include <d3dcompiler.h>
#include <dxgi.h>
#include <iostream>

#pragma comment(lib, "d3d11")
#pragma comment(lib, "dxgi")
#pragma comment(lib, "d3dcompiler")

struct Vertex {
    float x,y,z,w;
    float u,v;
};

struct PSConstants {
    int mirror;
    float pad[3];
};

CameraRenderer::CameraRenderer() {}
CameraRenderer::~CameraRenderer() { Shutdown(); }

bool CameraRenderer::Initialize(void* hwnd, int width, int height) {
    hwnd_ = (HWND)hwnd;
    width_ = width;
    height_ = height;

    DXGI_SWAP_CHAIN_DESC scDesc = {};
    scDesc.BufferCount = 2;
    scDesc.BufferDesc.Width = width;
    scDesc.BufferDesc.Height = height;
    scDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    scDesc.BufferDesc.RefreshRate.Numerator = 60;
    scDesc.BufferDesc.RefreshRate.Denominator = 1;
    scDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    scDesc.OutputWindow = hwnd_;
    scDesc.SampleDesc.Count = 1;
    scDesc.Windowed = TRUE;
    scDesc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

    D3D_FEATURE_LEVEL featureLevel;
    HRESULT hr = D3D11CreateDeviceAndSwapChain(
        nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, 0,
        nullptr, 0, D3D11_SDK_VERSION,
        &scDesc, &swapChain_, &device_, &featureLevel, &context_
    );
    if (FAILED(hr)) {
        // WARP fallback
        hr = D3D11CreateDeviceAndSwapChain(
            nullptr, D3D_DRIVER_TYPE_WARP, nullptr, 0,
            nullptr, 0, D3D11_SDK_VERSION,
            &scDesc, &swapChain_, &device_, &featureLevel, &context_
        );
        if (FAILED(hr)) return false;
    }

    ID3D11Texture2D* pBackBuffer = nullptr;
    hr = swapChain_->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&pBackBuffer);
    if (FAILED(hr)) return false;
    hr = device_->CreateRenderTargetView(pBackBuffer, nullptr, &rtv_);
    pBackBuffer->Release();
    if (FAILED(hr)) return false;

    // Rasterizer CULL_NONE
    D3D11_RASTERIZER_DESC rastDesc = {};
    rastDesc.FillMode = D3D11_FILL_SOLID;
    rastDesc.CullMode = D3D11_CULL_NONE;
    rastDesc.FrontCounterClockwise = FALSE;
    rastDesc.DepthClipEnable = TRUE;
    device_->CreateRasterizerState(&rastDesc, &rasterState_);
    if (rasterState_) context_->RSSetState(rasterState_);

    // Sampler
    D3D11_SAMPLER_DESC sampDesc = {};
    sampDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
    sampDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
    sampDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
    sampDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
    device_->CreateSamplerState(&sampDesc, &sampler_);

    if (!CreateShaders()) return false;
    if (!CreateQuad()) return false;
    if (!CreateInputTexture(width, height)) return false;

    // Viewport
    D3D11_VIEWPORT vp = {};
    vp.Width = (float)width;
    vp.Height = (float)height;
    vp.MinDepth = 0.0f;
    vp.MaxDepth = 1.0f;
    context_->RSSetViewports(1, &vp);

    initialized_ = true;
    return true;
}

void CameraRenderer::Shutdown() {
    if (context_) context_->ClearState();
    if (psConstantBuffer_) { psConstantBuffer_->Release(); psConstantBuffer_=nullptr; }
    if (indexBuffer_) { indexBuffer_->Release(); indexBuffer_=nullptr; }
    if (vertexBuffer_) { vertexBuffer_->Release(); vertexBuffer_=nullptr; }
    if (inputLayout_) { inputLayout_->Release(); inputLayout_=nullptr; }
    if (ps_) { ps_->Release(); ps_=nullptr; }
    if (vs_) { vs_->Release(); vs_=nullptr; }
    if (sampler_) { sampler_->Release(); sampler_=nullptr; }
    if (inputSRV_) { inputSRV_->Release(); inputSRV_=nullptr; }
    if (inputTexture_) { inputTexture_->Release(); inputTexture_=nullptr; }
    if (rasterState_) { rasterState_->Release(); rasterState_=nullptr; }
    if (rtv_) { rtv_->Release(); rtv_=nullptr; }
    if (swapChain_) { swapChain_->Release(); swapChain_=nullptr; }
    if (context_) { context_->Release(); context_=nullptr; }
    if (device_) { device_->Release(); device_=nullptr; }
    initialized_ = false;
}

bool CameraRenderer::CreateShaders() {
    const char* vsSrc = R"(
        struct VS_IN {
            float4 pos : POSITION;
            float2 uv : TEXCOORD0;
        };
        struct PS_IN {
            float4 pos : SV_POSITION;
            float2 uv : TEXCOORD0;
        };
        PS_IN main(VS_IN input) {
            PS_IN output;
            output.pos = input.pos;
            output.uv = input.uv;
            return output;
        }
    )";

    const char* psSrc = R"(
        Texture2D tex : register(t0);
        SamplerState samp : register(s0);
        cbuffer PSConstants : register(b0) {
            int mirror;
            float3 pad;
        };
        struct PS_IN {
            float4 pos : SV_POSITION;
            float2 uv : TEXCOORD0;
        };
        float4 main(PS_IN input) : SV_TARGET {
            float2 uv = input.uv;
            if (mirror != 0) uv.x = 1.0 - uv.x;
            return tex.Sample(samp, uv);
        }
    )";

    ID3DBlob* vsBlob = nullptr;
    ID3DBlob* psBlob = nullptr;
    ID3DBlob* errorBlob = nullptr;

    HRESULT hr = D3DCompile(vsSrc, strlen(vsSrc), nullptr, nullptr, nullptr, "main", "vs_5_0", 0, 0, &vsBlob, &errorBlob);
    if (FAILED(hr)) {
        if (errorBlob) { std::cout << (char*)errorBlob->GetBufferPointer() << std::endl; errorBlob->Release(); }
        return false;
    }
    hr = device_->CreateVertexShader(vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), nullptr, &vs_);
    if (FAILED(hr)) { vsBlob->Release(); return false; }

    // Input layout stride 24 POSITION float4 offset0 TEXCOORD float2 offset16
    D3D11_INPUT_ELEMENT_DESC layout[] = {
        {"POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 16, D3D11_INPUT_PER_VERTEX_DATA, 0}
    };
    hr = device_->CreateInputLayout(layout, 2, vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), &inputLayout_);
    vsBlob->Release();
    if (FAILED(hr)) return false;

    hr = D3DCompile(psSrc, strlen(psSrc), nullptr, nullptr, nullptr, "main", "ps_5_0", 0, 0, &psBlob, &errorBlob);
    if (FAILED(hr)) {
        if (errorBlob) { std::cout << (char*)errorBlob->GetBufferPointer() << std::endl; errorBlob->Release(); }
        return false;
    }
    hr = device_->CreatePixelShader(psBlob->GetBufferPointer(), psBlob->GetBufferSize(), nullptr, &ps_);
    psBlob->Release();
    if (FAILED(hr)) return false;

    // Constant buffer for mirror
    D3D11_BUFFER_DESC cbDesc = {};
    cbDesc.ByteWidth = sizeof(PSConstants);
    cbDesc.Usage = D3D11_USAGE_DYNAMIC;
    cbDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    cbDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    device_->CreateBuffer(&cbDesc, nullptr, &psConstantBuffer_);

    return true;
}

bool CameraRenderer::CreateQuad() {
    // Fullscreen quad CW {0,2,1,0,3,2} for CULL_NONE, stride 24
    Vertex verts[4] = {
        {-1.0f,  1.0f, 0.0f, 1.0f, 0.0f, 0.0f},
        { 1.0f,  1.0f, 0.0f, 1.0f, 1.0f, 0.0f},
        { 1.0f, -1.0f, 0.0f, 1.0f, 1.0f, 1.0f},
        {-1.0f, -1.0f, 0.0f, 1.0f, 0.0f, 1.0f}
    };
    // CW winding
    uint32_t indices[6] = {0,2,1, 0,3,2};

    D3D11_BUFFER_DESC vbDesc = {};
    vbDesc.ByteWidth = sizeof(verts);
    vbDesc.Usage = D3D11_USAGE_DEFAULT;
    vbDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    D3D11_SUBRESOURCE_DATA vbData = {};
    vbData.pSysMem = verts;
    HRESULT hr = device_->CreateBuffer(&vbDesc, &vbData, &vertexBuffer_);
    if (FAILED(hr)) return false;

    D3D11_BUFFER_DESC ibDesc = {};
    ibDesc.ByteWidth = sizeof(indices);
    ibDesc.Usage = D3D11_USAGE_DEFAULT;
    ibDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
    D3D11_SUBRESOURCE_DATA ibData = {};
    ibData.pSysMem = indices;
    hr = device_->CreateBuffer(&ibDesc, &ibData, &indexBuffer_);
    if (FAILED(hr)) return false;

    return true;
}

bool CameraRenderer::CreateInputTexture(int width, int height) {
    if (inputTexture_) { inputTexture_->Release(); inputTexture_=nullptr; }
    if (inputSRV_) { inputSRV_->Release(); inputSRV_=nullptr; }

    D3D11_TEXTURE2D_DESC desc = {};
    desc.Width = width;
    desc.Height = height;
    desc.MipLevels = 1;
    desc.ArraySize = 1;
    desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    desc.SampleDesc.Count = 1;
    desc.Usage = D3D11_USAGE_DYNAMIC;
    desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

    HRESULT hr = device_->CreateTexture2D(&desc, nullptr, &inputTexture_);
    if (FAILED(hr)) return false;

    D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Format = desc.Format;
    srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MipLevels = 1;
    hr = device_->CreateShaderResourceView(inputTexture_, &srvDesc, &inputSRV_);
    if (FAILED(hr)) return false;

    return true;
}

bool CameraRenderer::UpdateInputTexture(const uint8_t* rgba, int width, int height) {
    if (!inputTexture_ || !rgba) return false;

    // Recreate if size changed
    D3D11_TEXTURE2D_DESC desc = {};
    inputTexture_->GetDesc(&desc);
    if ((int)desc.Width != width || (int)desc.Height != height) {
        if (!CreateInputTexture(width, height)) return false;
    }

    D3D11_MAPPED_SUBRESOURCE mapped = {};
    HRESULT hr = context_->Map(inputTexture_, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
    if (FAILED(hr)) return false;

    // Copy with RowPitch
    for (int y=0; y<height; ++y) {
        memcpy((uint8_t*)mapped.pData + y*mapped.RowPitch, rgba + y*width*4, width*4);
    }
    context_->Unmap(inputTexture_, 0);
    return true;
}

bool CameraRenderer::RenderFrame(bool mirror) {
    if (!initialized_ || !rtv_) return false;

    // Update constant buffer mirror
    if (psConstantBuffer_) {
        D3D11_MAPPED_SUBRESOURCE mapped = {};
        if (SUCCEEDED(context_->Map(psConstantBuffer_, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped))) {
            PSConstants* c = (PSConstants*)mapped.pData;
            c->mirror = mirror ? 1 : 0;
            c->pad[0]=c->pad[1]=c->pad[2]=0;
            context_->Unmap(psConstantBuffer_, 0);
        }
    }

    float clearColor[4] = {0.1f,0.1f,0.1f,1.0f};
    context_->OMSetRenderTargets(1, &rtv_, nullptr);
    context_->ClearRenderTargetView(rtv_, clearColor);

    UINT stride = 24; // POSITION float4 + TEXCOORD float2 = 24 bytes
    UINT offset = 0;
    context_->IASetVertexBuffers(0, 1, &vertexBuffer_, &stride, &offset);
    context_->IASetIndexBuffer(indexBuffer_, DXGI_FORMAT_R32_UINT, 0);
    context_->IASetInputLayout(inputLayout_);
    context_->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    context_->VSSetShader(vs_, nullptr, 0);
    context_->PSSetShader(ps_, nullptr, 0);
    context_->PSSetShaderResources(0, 1, &inputSRV_);
    context_->PSSetSamplers(0, 1, &sampler_);
    context_->PSSetConstantBuffers(0, 1, &psConstantBuffer_);
    if (rasterState_) context_->RSSetState(rasterState_);

    context_->DrawIndexed(6, 0, 0);

    HRESULT hr = swapChain_->Present(1, 0);
    return SUCCEEDED(hr);
}

void CameraRenderer::OnResize(int width, int height) {
    if (!swapChain_ || width==0 || height==0) return;
    if (rtv_) { rtv_->Release(); rtv_=nullptr; }
    context_->OMSetRenderTargets(0, nullptr, nullptr);
    HRESULT hr = swapChain_->ResizeBuffers(0, width, height, DXGI_FORMAT_UNKNOWN, 0);
    if (FAILED(hr)) return;
    ID3D11Texture2D* pBackBuffer = nullptr;
    hr = swapChain_->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&pBackBuffer);
    if (FAILED(hr)) return;
    device_->CreateRenderTargetView(pBackBuffer, nullptr, &rtv_);
    pBackBuffer->Release();

    D3D11_VIEWPORT vp = {};
    vp.Width = (float)width;
    vp.Height = (float)height;
    vp.MinDepth = 0.0f;
    vp.MaxDepth = 1.0f;
    context_->RSSetViewports(1, &vp);
    width_ = width;
    height_ = height;
}
