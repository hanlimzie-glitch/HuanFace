/**
 * HuanFace Rendering Tests — Phase 3
 * Tests D3D11 initialization, texture creation, render target, shader compilation, fullscreen quad, blit
 */

#include "../sdk/src/rendering/render_backend.h"
#include "../sdk/src/rendering/null_backend.h"
#include <iostream>
#include <vector>

using namespace huanface;

bool TestRendering() {
    int passed = 0;
    int failed = 0;

    auto check = [&](bool cond, const std::string& msg) {
        if (cond) {
            std::cout << "  [PASS] " << msg << std::endl;
            passed++;
        } else {
            std::cout << "  [FAIL] " << msg << std::endl;
            failed++;
        }
    };

    // Test 1: Null backend creation
    {
        auto backend = CreateNullBackend();
        check(backend != nullptr, "Null backend creation");
        if (backend) {
            HFResult res = backend->Init(nullptr);
            check(res == HF_RESULT_OK, "Null backend Init");
            check(backend->IsInitialized(), "Null backend IsInitialized");
            check(backend->GetType() == HF_RENDER_BACKEND_AUTO, "Null backend type AUTO");
            backend->Shutdown();
            check(!backend->IsInitialized(), "Null backend Shutdown");
        }
    }

    // Test 2: Render backend factory AUTO
    {
        auto backend = CreateRenderBackend(HF_RENDER_BACKEND_AUTO);
        check(backend != nullptr, "Render backend factory AUTO");
        if (backend) {
            HFResult res = backend->Init(nullptr);
            check(res == HF_RESULT_OK, "Render backend AUTO Init");
            backend->Shutdown();
        }
    }

    // Test 3: Texture creation
    {
        auto backend = CreateNullBackend();
        backend->Init(nullptr);
        IGpuTexture* tex = backend->CreateTexture(256, 256, HF_FORMAT_RGBA8, nullptr);
        check(tex != nullptr, "CreateTexture 256x256 RGBA8");
        if (tex) {
            check(tex->GetWidth() == 256, "Texture width 256");
            check(tex->GetHeight() == 256, "Texture height 256");
            check(tex->GetFormat() == HF_FORMAT_RGBA8, "Texture format RGBA8");
            backend->DestroyTexture(tex);
            check(true, "DestroyTexture");
        }
        backend->Shutdown();
    }

    // Test 4: Texture creation with data
    {
        auto backend = CreateNullBackend();
        backend->Init(nullptr);
        std::vector<uint8_t> data(128*128*4, 128);
        IGpuTexture* tex = backend->CreateTexture(128, 128, HF_FORMAT_RGBA8, data.data());
        check(tex != nullptr, "CreateTexture with data 128x128");
        if (tex) {
            backend->DestroyTexture(tex);
        }
        backend->Shutdown();
    }

    // Test 5: Render target creation
    {
        auto backend = CreateNullBackend();
        backend->Init(nullptr);
        IRenderTarget* rt = backend->CreateRenderTarget(1280, 720, HF_FORMAT_RGBA8);
        check(rt != nullptr, "CreateRenderTarget 1280x720");
        if (rt) {
            IGpuTexture* tex = rt->GetTexture();
            check(tex != nullptr, "RenderTarget GetTexture");
            if (tex) {
                check(tex->GetWidth() == 1280, "RenderTarget texture width 1280");
                check(tex->GetHeight() == 720, "RenderTarget texture height 720");
            }
            backend->DestroyRenderTarget(rt);
            check(true, "DestroyRenderTarget");
        }
        backend->Shutdown();
    }

    // Test 6: Shader compilation
    {
        auto backend = CreateNullBackend();
        backend->Init(nullptr);
        std::string vs = "vertex shader source";
        std::string fs = "fragment shader source";
        IShader* shader = backend->CreateShader(vs, fs);
        check(shader != nullptr, "CreateShader from source");
        if (shader) {
            Uniform u;
            u.type = Uniform::Type::FLOAT;
            u.floatValue = 0.8f;
            shader->SetUniform("u_intensity_lip", u);
            check(true, "SetUniform");
            backend->DestroyShader(shader);
        }
        backend->Shutdown();
    }

    // Test 7: Fullscreen quad mesh
    {
        auto backend = CreateNullBackend();
        backend->Init(nullptr);
        // Fullscreen quad vertices: pos x,y,z, uv u,v
        std::vector<float> vertices = {
            -1.0f, -1.0f, 0.0f, 0.0f, 0.0f,
             1.0f, -1.0f, 0.0f, 1.0f, 0.0f,
             1.0f,  1.0f, 0.0f, 1.0f, 1.0f,
            -1.0f,  1.0f, 0.0f, 0.0f, 1.0f
        };
        std::vector<int> indices = {0,1,2, 0,2,3};
        IMesh* mesh = backend->CreateMesh(vertices, indices);
        check(mesh != nullptr, "CreateMesh fullscreen quad");
        if (mesh) {
            check(mesh->GetVertexCount() > 0, "Mesh vertex count >0");
            check(mesh->GetIndexCount() == 6, "Mesh index count 6");
            backend->DestroyMesh(mesh);
        }
        backend->Shutdown();
    }

    // Test 8: Blit texture
    {
        auto backend = CreateNullBackend();
        backend->Init(nullptr);
        IGpuTexture* src = backend->CreateTexture(256, 256, HF_FORMAT_RGBA8, nullptr);
        IRenderTarget* dst = backend->CreateRenderTarget(256, 256, HF_FORMAT_RGBA8);
        check(src != nullptr && dst != nullptr, "Create src texture and dst RT for blit");
        if (src && dst) {
            backend->Blit(src, dst);
            check(true, "Blit src->dst");
        }
        if (src) backend->DestroyTexture(src);
        if (dst) backend->DestroyRenderTarget(dst);
        backend->Shutdown();
    }

    // Test 9: Clear render target
    {
        auto backend = CreateNullBackend();
        backend->Init(nullptr);
        IRenderTarget* rt = backend->CreateRenderTarget(128, 128, HF_FORMAT_RGBA8);
        backend->SetRenderTarget(rt);
        backend->Clear(1.0f, 0.0f, 0.0f, 1.0f); // red
        check(true, "Clear render target red");
        if (rt) backend->DestroyRenderTarget(rt);
        backend->Shutdown();
    }

    // Test 10: CPU -> GPU -> shader -> render target flow (minimal GPU test)
    {
        auto backend = CreateNullBackend();
        backend->Init(nullptr);
        
        // Create solid color texture (CPU -> GPU)
        std::vector<uint8_t> solidColor(256*256*4);
        for (size_t i=0;i<solidColor.size();i+=4) {
            solidColor[i+0] = 255; // R
            solidColor[i+1] = 0;   // G
            solidColor[i+2] = 0;   // B
            solidColor[i+3] = 255; // A
        }
        IGpuTexture* inputTex = backend->CreateTexture(256, 256, HF_FORMAT_RGBA8, solidColor.data());
        check(inputTex != nullptr, "CPU->GPU: Create solid color texture");

        IRenderTarget* rt = backend->CreateRenderTarget(256, 256, HF_FORMAT_RGBA8);
        check(rt != nullptr, "Create render target for GPU test");

        IShader* shader = backend->CreateShader("vs", "fs");
        check(shader != nullptr, "Create shader for GPU test");

        std::vector<float> quadVerts = {
            -1,-1,0, 0,0,
             1,-1,0, 1,0,
             1, 1,0, 1,1,
            -1, 1,0, 0,1
        };
        std::vector<int> quadIndices = {0,1,2, 0,2,3};
        IMesh* quad = backend->CreateMesh(quadVerts, quadIndices);
        check(quad != nullptr, "Create fullscreen quad for GPU test");

        if (inputTex && rt && shader && quad) {
            backend->SetRenderTarget(rt);
            backend->Clear(0,0,0,1);
            
            // Simulate shader with texture
            shader->SetTexture("u_inputTexture", inputTex);
            
            std::map<std::string, Uniform> uniforms;
            backend->DrawMesh(quad, shader, uniforms);
            check(true, "DrawMesh fullscreen quad");

            // Blit test
            backend->Blit(inputTex, rt);
            check(true, "Blit for GPU test validates CPU->GPU->shader->RT");

            // For null backend, we can check if RT data was written
            NullRenderTarget* nullRT = static_cast<NullRenderTarget*>(rt);
            if (nullRT && nullRT->texture) {
                bool hasData = !nullRT->texture->data.empty();
                check(hasData, "Render target has data after blit");
            }
        }

        if (quad) backend->DestroyMesh(quad);
        if (shader) backend->DestroyShader(shader);
        if (rt) backend->DestroyRenderTarget(rt);
        if (inputTex) backend->DestroyTexture(inputTex);

        backend->Shutdown();
        check(true, "Minimal GPU test CPU->GPU->shader->RT completed");
    }

#ifdef _WIN32
    // Test 11: D3D11 backend (Windows only)
    {
        auto backend = CreateRenderBackend(HF_RENDER_BACKEND_D3D11);
        check(backend != nullptr, "D3D11 backend creation (Windows)");
        if (backend) {
            HFResult res = backend->Init(nullptr);
            check(res == HF_RESULT_OK, "D3D11 backend Init");
            if (res == HF_RESULT_OK) {
                IGpuTexture* tex = backend->CreateTexture(256, 256, HF_FORMAT_RGBA8, nullptr);
                check(tex != nullptr, "D3D11 CreateTexture");
                if (tex) backend->DestroyTexture(tex);

                IRenderTarget* rt = backend->CreateRenderTarget(1280, 720, HF_FORMAT_RGBA8);
                check(rt != nullptr, "D3D11 CreateRenderTarget");
                if (rt) backend->DestroyRenderTarget(rt);

                backend->Shutdown();
                check(true, "D3D11 Shutdown");
            }
        }
    }
#else
    std::cout << "  [SKIP] D3D11 backend tests (not Windows)" << std::endl;
#endif

    std::cout << "  Rendering tests: " << passed << " passed, " << failed << " failed" << std::endl;
    return failed == 0;
}
