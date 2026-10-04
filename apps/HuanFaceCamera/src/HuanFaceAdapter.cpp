#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#define NOGDI
#include "HuanFaceAdapter.h"
#undef min
#undef max
#undef OPAQUE
#undef TRANSPARENT

#include <algorithm>
#include <cmath>
#include <cstring>

HuanFaceAdapter::HuanFaceAdapter() {}
HuanFaceAdapter::~HuanFaceAdapter() { Shutdown(); }

bool HuanFaceAdapter::Initialize() {
    initialized_ = true;
    faceCount_ = 1; // synthetic face for preview
    return true;
}

void HuanFaceAdapter::Shutdown() {
    initialized_ = false;
}

std::vector<uint8_t> HuanFaceAdapter::ConvertBGRAtoRGBA(const uint8_t* data, int w, int h) {
    std::vector<uint8_t> out(w*h*4);
    for (int i=0;i<w*h;++i){
        out[i*4+0]=data[i*4+2]; // R
        out[i*4+1]=data[i*4+1]; // G
        out[i*4+2]=data[i*4+0]; // B
        out[i*4+3]=data[i*4+3]; // A
    }
    return out;
}

std::vector<uint8_t> HuanFaceAdapter::ConvertYUY2toRGBA(const uint8_t* data, int w, int h) {
    std::vector<uint8_t> out(w*h*4);
    for (int y=0;y<h;++y){
        for (int x=0;x<w;x+=2){
            int idx = (y*w + x)*2;
            uint8_t y0 = data[idx+0];
            uint8_t u  = data[idx+1];
            uint8_t y1 = data[idx+2];
            uint8_t v  = data[idx+3];
            auto convert = [&](uint8_t Y, uint8_t U, uint8_t V, uint8_t* rgba){
                int c = Y - 16;
                int d = U - 128;
                int e = V - 128;
                int r = (298*c + 409*e + 128)>>8;
                int g = (298*c - 100*d - 208*e + 128)>>8;
                int b = (298*c + 516*d + 128)>>8;
                r = std::max(0, std::min(255, r));
                g = std::max(0, std::min(255, g));
                b = std::max(0, std::min(255, b));
                rgba[0]= (uint8_t)r;
                rgba[1]= (uint8_t)g;
                rgba[2]= (uint8_t)b;
                rgba[3]= 255;
            };
            convert(y0,u,v,&out[(y*w+x)*4]);
            if (x+1<w) convert(y1,u,v,&out[(y*w+x+1)*4]);
        }
    }
    return out;
}

std::vector<uint8_t> HuanFaceAdapter::ConvertNV12toRGBA(const uint8_t* yPlane, const uint8_t* uvPlane, int w, int h, int strideY, int strideUV) {
    std::vector<uint8_t> out(w*h*4);
    for (int y=0;y<h;++y){
        for (int x=0;x<w;++x){
            int yIdx = y*strideY + x;
            int uvIdx = (y/2)*strideUV + (x/2)*2;
            uint8_t Y = yPlane[yIdx];
            uint8_t U = uvPlane[uvIdx];
            uint8_t V = uvPlane[uvIdx+1];
            int c = Y - 16;
            int d = U - 128;
            int e = V - 128;
            int r = (298*c + 409*e + 128)>>8;
            int g = (298*c - 100*d - 208*e + 128)>>8;
            int b = (298*c + 516*d + 128)>>8;
            r = std::max(0, std::min(255, r));
            g = std::max(0, std::min(255, g));
            b = std::max(0, std::min(255, b));
            int outIdx = (y*w + x)*4;
            out[outIdx+0]= (uint8_t)r;
            out[outIdx+1]= (uint8_t)g;
            out[outIdx+2]= (uint8_t)b;
            out[outIdx+3]= 255;
        }
    }
    return out;
}

std::vector<uint8_t> HuanFaceAdapter::ApplyBeauty(const std::vector<uint8_t>& rgba, int w, int h) {
    std::vector<uint8_t> out = rgba;
    std::lock_guard<std::mutex> lock(mutex_);
    float smooth = beauty_.smoothing;
    float bright = beauty_.brightness;
    float contrast = beauty_.contrast;
    float retouch = beauty_.retouch;

    // Simple smoothing: box blur 3x3 weighted by smooth param (0=no blur, 1=full blur)
    if (smooth > 0.01f) {
        std::vector<uint8_t> blurred = out;
        int radius = 1;
        for (int y=1;y<h-1;++y){
            for (int x=1;x<w-1;++x){
                int r=0,g=0,b=0;
                for (int dy=-radius; dy<=radius; ++dy){
                    for (int dx=-radius; dx<=radius; ++dx){
                        int idx = ((y+dy)*w + (x+dx))*4;
                        r+= out[idx+0];
                        g+= out[idx+1];
                        b+= out[idx+2];
                    }
                }
                r/=9; g/=9; b/=9;
                int idx = (y*w+x)*4;
                out[idx+0] = (uint8_t)(out[idx+0]*(1.0f-smooth) + r*smooth);
                out[idx+1] = (uint8_t)(out[idx+1]*(1.0f-smooth) + g*smooth);
                out[idx+2] = (uint8_t)(out[idx+2]*(1.0f-smooth) + b*smooth);
            }
        }
    }

    // Brightness + Contrast + Retouch (retouch = extra brightness on skin tone area center)
    for (int i=0;i<w*h;++i){
        int idx=i*4;
        float rf = out[idx+0];
        float gf = out[idx+1];
        float bf = out[idx+2];

        // Brightness: -1 to 1 -> -100 to +100
        rf += bright*80.0f;
        gf += bright*80.0f;
        bf += bright*80.0f;

        // Contrast: -1 to 1 -> factor 0.5 to 1.5
        float cFactor = 1.0f + contrast*0.5f;
        rf = (rf-128.0f)*cFactor + 128.0f;
        gf = (gf-128.0f)*cFactor + 128.0f;
        bf = (bf-128.0f)*cFactor + 128.0f;

        // Retouch: simple skin brighten in center region (face area)
        if (retouch > 0.01f) {
            int x = i % w;
            int y = i / w;
            float cx = w*0.5f, cy = h*0.5f;
            float dist = sqrtf((x-cx)*(x-cx) + (y-cy)*(y-cy)) / (w*0.5f);
            if (dist < 0.6f) {
                float mask = (0.6f - dist)/0.6f * retouch * 0.3f;
                rf += 20.0f*mask;
                gf += 15.0f*mask;
                bf += 10.0f*mask;
            }
        }

        rf = std::max(0.0f, std::min(255.0f, rf));
        gf = std::max(0.0f, std::min(255.0f, gf));
        bf = std::max(0.0f, std::min(255.0f, bf));
        out[idx+0]=(uint8_t)rf;
        out[idx+1]=(uint8_t)gf;
        out[idx+2]=(uint8_t)bf;
    }
    return out;
}

std::vector<uint8_t> HuanFaceAdapter::ApplyMakeup(const std::vector<uint8_t>& rgba, int w, int h) {
    std::vector<uint8_t> out = rgba;
    std::lock_guard<std::mutex> lock(mutex_);
    // Simple makeup: lip color in bottom center, blush, etc. via rect masks
    // Lip: bottom 60-75% y, 35-65% x
    if (makeup_.lip > 0.01f) {
        for (int y=(int)(h*0.60f); y<(int)(h*0.75f); ++y){
            for (int x=(int)(w*0.35f); x<(int)(w*0.65f); ++x){
                if (y<0||y>=h||x<0||x>=w) continue;
                int idx=(y*w+x)*4;
                // pink lip
                out[idx+0] = (uint8_t)(out[idx+0]*(1.0f-makeup_.lip*0.6f) + 220*makeup_.lip*0.6f);
                out[idx+1] = (uint8_t)(out[idx+1]*(1.0f-makeup_.lip*0.6f) + 50*makeup_.lip*0.6f);
                out[idx+2] = (uint8_t)(out[idx+2]*(1.0f-makeup_.lip*0.6f) + 80*makeup_.lip*0.6f);
            }
        }
    }
    if (makeup_.blush > 0.01f) {
        // left and right cheek
        for (int side=0; side<2; ++side){
            int cx = side==0 ? (int)(w*0.25f) : (int)(w*0.75f);
            int cy = (int)(h*0.45f);
            int rad = (int)(w*0.08f);
            for (int y=cy-rad; y<cy+rad; ++y){
                for (int x=cx-rad; x<cx+rad; ++x){
                    if (y<0||y>=h||x<0||x>=w) continue;
                    float dist = sqrtf((x-cx)*(x-cx)+(y-cy)*(y-cy));
                    if (dist>rad) continue;
                    float alpha = (1.0f - dist/rad)*makeup_.blush*0.4f;
                    int idx=(y*w+x)*4;
                    out[idx+0] = (uint8_t)(out[idx+0]*(1-alpha) + 255*alpha);
                    out[idx+1] = (uint8_t)(out[idx+1]*(1-alpha) + 120*alpha);
                    out[idx+2] = (uint8_t)(out[idx+2]*(1-alpha) + 150*alpha);
                }
            }
        }
    }
    // Eyebrow, eyeliner etc simplified as darker lines
    // For brevity, only lip+blush implemented, others can be added similarly
    return out;
}

ProcessedFrame HuanFaceAdapter::ProcessFrame(const uint8_t* data, int w, int h, int format, bool mirror) {
    std::vector<uint8_t> rgba;
    if (format==0) { // BGRA
        rgba = ConvertBGRAtoRGBA(data,w,h);
    } else if (format==2) { // YUY2
        rgba = ConvertYUY2toRGBA(data,w,h);
    } else if (format==3) { // NV12 - assume data is Y plane followed by UV, need split
        // For simplicity, treat data as Y plane + UV plane contiguous
        const uint8_t* yPlane = data;
        const uint8_t* uvPlane = data + w*h;
        rgba = ConvertNV12toRGBA(yPlane, uvPlane, w, h, w, w);
    } else { // RGBA already
        rgba.assign(data, data + w*h*4);
    }

    // Apply beauty + makeup (CPU reference, GPU path via CameraRenderer D3D11 would be similar)
    rgba = ApplyBeauty(rgba,w,h);
    rgba = ApplyMakeup(rgba,w,h);

    // Mirror handling — preview mirror ON, but we keep data as is and let renderer mirror via shader
    // For face count, synthetic 1
    faceCount_ = 1;

    ProcessedFrame out;
    out.rgba = std::move(rgba);
    out.width = w;
    out.height = h;
    out.faceCount = faceCount_;
    return out;
}

void HuanFaceAdapter::SetBeautyParams(const BeautyParams& p) {
    std::lock_guard<std::mutex> lock(mutex_);
    beauty_ = p;
}

void HuanFaceAdapter::SetMakeupParams(const MakeupParams& p) {
    std::lock_guard<std::mutex> lock(mutex_);
    makeup_ = p;
}
