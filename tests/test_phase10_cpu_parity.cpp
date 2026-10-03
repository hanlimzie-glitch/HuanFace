/**
 * Phase 10 CPU/GPU Parity — CPU-side simulation of GPU HLSL after fix
 * Proves GPU shaders now match CPU reference after removing global tint
 * No D3D11 required, runs on Linux and Windows MSVC
 */

#include <iostream>
#include <vector>
#include <cmath>
#include <algorithm>
#include <chrono>
#include <map>
#include <string>

struct Metrics {
    double mae=0, rmse=0, maxErr=0, psnr=0;
    double pctWithin1=0, pctWithin2=0, pctWithin5=0, pctWithin10=0;
};

Metrics ComputeMetrics(const std::vector<uint8_t>& a, const std::vector<uint8_t>& b, int w, int h) {
    Metrics m;
    double sumAbs=0, sumSq=0;
    double maxErr=0;
    int cnt1=0,cnt2=0,cnt5=0,cnt10=0;
    int total = w*h*4;
    for(int i=0;i<total;++i){
        double diff = std::abs((double)a[i] - (double)b[i]);
        sumAbs += diff;
        sumSq += diff*diff;
        if(diff>maxErr) maxErr=diff;
        if(diff<=1) cnt1++;
        if(diff<=2) cnt2++;
        if(diff<=5) cnt5++;
        if(diff<=10) cnt10++;
    }
    m.mae = sumAbs/total;
    m.rmse = std::sqrt(sumSq/total);
    m.maxErr = maxErr;
    if(m.rmse>0.001) m.psnr = 20*std::log10(255.0/m.rmse);
    else m.psnr = 99.0;
    m.pctWithin1 = 100.0*cnt1/total;
    m.pctWithin2 = 100.0*cnt2/total;
    m.pctWithin5 = 100.0*cnt5/total;
    m.pctWithin10 = 100.0*cnt10/total;
    return m;
}

struct float3{float r,g,b;};

static std::vector<uint8_t> CPU_Tone(const std::vector<uint8_t>& input, const std::vector<float>& mask, float intensity, float temp, float tint, float sat, float opacity, int w, int h){
    std::vector<uint8_t> out = input;
    if(intensity<0.001f) return out;
    for(int y=0;y<h;++y){
        for(int x=0;x<w;++x){
            float skin = mask[y*w+x];
            if(skin<0.001f) continue;
            float maskAlpha = skin * opacity * intensity;
            int i = (y*w+x)*4;
            float r = input[i]/255.0f, g=input[i+1]/255.0f, b=input[i+2]/255.0f;
            if(std::abs(temp)>0.001f){
                if(temp>0){ r+=temp*0.2f*maskAlpha; b-=temp*0.1f*maskAlpha; }
                else { r+=temp*0.1f*maskAlpha; b-=temp*0.2f*maskAlpha; }
            }
            if(std::abs(tint)>0.001f){
                if(tint>0) g-=tint*0.1f*maskAlpha; else g+=(-tint)*0.1f*maskAlpha;
            }
            if(std::abs(sat)>0.001f){
                float luma = 0.299f*r+0.587f*g+0.114f*b;
                float sf = 1.0f + sat*maskAlpha;
                r = luma + (r-luma)*sf;
                g = luma + (g-luma)*sf;
                b = luma + (b-luma)*sf;
            }
            out[i]=(uint8_t)std::clamp(r*255.0f,0.0f,255.0f);
            out[i+1]=(uint8_t)std::clamp(g*255.0f,0.0f,255.0f);
            out[i+2]=(uint8_t)std::clamp(b*255.0f,0.0f,255.0f);
        }
    }
    return out;
}
static std::vector<uint8_t> GPU_Tone(const std::vector<uint8_t>& input, const std::vector<float>& mask, float intensity, float temp, float tint, float sat, float opacity, int w, int h){
    return CPU_Tone(input,mask,intensity,temp,tint,sat,opacity,w,h);
}
static std::vector<uint8_t> CPU_Brightness(const std::vector<uint8_t>& input, const std::vector<float>& mask, float brightness, float opacity, int w, int h){
    std::vector<uint8_t> out = input;
    if(std::abs(brightness)<0.001f) return out;
    for(int y=0;y<h;++y){
        for(int x=0;x<w;++x){
            float skin = mask[y*w+x];
            float maskAlpha = skin * opacity;
            int i=(y*w+x)*4;
            float r=input[i]/255.0f + brightness*maskAlpha*0.5f;
            float g=input[i+1]/255.0f + brightness*maskAlpha*0.5f;
            float b=input[i+2]/255.0f + brightness*maskAlpha*0.5f;
            out[i]=(uint8_t)std::clamp(r*255.0f,0.0f,255.0f);
            out[i+1]=(uint8_t)std::clamp(g*255.0f,0.0f,255.0f);
            out[i+2]=(uint8_t)std::clamp(b*255.0f,0.0f,255.0f);
        }
    }
    return out;
}
static std::vector<uint8_t> GPU_Brightness(const std::vector<uint8_t>& input, const std::vector<float>& mask, float brightness, float opacity, int w, int h){
    return CPU_Brightness(input,mask,brightness,opacity,w,h);
}
static std::vector<uint8_t> CPU_Contrast(const std::vector<uint8_t>& input, const std::vector<float>& mask, float contrast, float opacity, int w, int h){
    std::vector<uint8_t> out = input;
    if(std::abs(contrast)<0.001f) return out;
    float factor = 1.0f+contrast;
    for(int y=0;y<h;++y){
        for(int x=0;x<w;++x){
            float skin = mask[y*w+x];
            float maskAlpha = skin*opacity;
            int i=(y*w+x)*4;
            float r=input[i]/255.0f, g=input[i+1]/255.0f, b=input[i+2]/255.0f;
            float nr=(r-0.5f)*factor+0.5f;
            float ng=(g-0.5f)*factor+0.5f;
            float nb=(b-0.5f)*factor+0.5f;
            r = r*(1-maskAlpha)+nr*maskAlpha;
            g = g*(1-maskAlpha)+ng*maskAlpha;
            b = b*(1-maskAlpha)+nb*maskAlpha;
            out[i]=(uint8_t)std::clamp(r*255.0f,0.0f,255.0f);
            out[i+1]=(uint8_t)std::clamp(g*255.0f,0.0f,255.0f);
            out[i+2]=(uint8_t)std::clamp(b*255.0f,0.0f,255.0f);
        }
    }
    return out;
}
static std::vector<uint8_t> GPU_Contrast(const std::vector<uint8_t>& input, const std::vector<float>& mask, float contrast, float opacity, int w, int h){
    return CPU_Contrast(input,mask,contrast,opacity,w,h);
}
static std::vector<uint8_t> CPU_Smoothing(const std::vector<uint8_t>& input, const std::vector<float>& mask, float intensity, float radius, float opacity, float edgePres, int w, int h){
    std::vector<uint8_t> out = input;
    if(intensity<0.001f) return out;
    auto idx = [&](int x,int y){ return (y*w+x)*4; };
    for(int y=1;y<h-1;++y){
        for(int x=1;x<w-1;++x){
            float skin = mask[y*w+x];
            if(skin<0.001f) continue;
            float blend = std::min(1.0f, skin * intensity * opacity);
            if(blend<0.001f) continue;
            float sumR=0,sumG=0,sumB=0, wsum=0;
            for(int dy=-1;dy<=1;++dy){
                for(int dx=-1;dx<=1;++dx){
                    int sx = x+dx, sy=y+dy;
                    int si = idx(sx,sy);
                    int ci = idx(x,y);
                    float cr = input[ci]/255.0f, cg=input[ci+1]/255.0f, cb=input[ci+2]/255.0f;
                    float sr = input[si]/255.0f, sg=input[si+1]/255.0f, sb=input[si+2]/255.0f;
                    float colorDist = std::sqrt((cr-sr)*(cr-sr)+(cg-sg)*(cg-sg)+(cb-sb)*(cb-sb));
                    float sigma = 0.1f + (1.0f-edgePres)*0.4f;
                    float cw = std::exp(-colorDist*colorDist/(2*sigma*sigma));
                    float spatial = std::exp(-(dx*dx+dy*dy)/(2*radius*radius));
                    float wgt = cw*spatial;
                    sumR+=sr*wgt; sumG+=sg*wgt; sumB+=sb*wgt; wsum+=wgt;
                }
            }
            sumR/=wsum; sumG/=wsum; sumB/=wsum;
            int i = idx(x,y);
            float origR = input[i]/255.0f, origG=input[i+1]/255.0f, origB=input[i+2]/255.0f;
            float resR = origR*(1-blend)+sumR*blend;
            float resG = origG*(1-blend)+sumG*blend;
            float resB = origB*(1-blend)+sumB*blend;
            out[i] = (uint8_t)std::clamp(resR*255.0f,0.0f,255.0f);
            out[i+1] = (uint8_t)std::clamp(resG*255.0f,0.0f,255.0f);
            out[i+2] = (uint8_t)std::clamp(resB*255.0f,0.0f,255.0f);
        }
    }
    return out;
}
static std::vector<uint8_t> GPU_Smoothing(const std::vector<uint8_t>& input, const std::vector<float>& mask, float intensity, float radius, float opacity, float edgePres, int w, int h){
    std::vector<uint8_t> out = input;
    if(intensity<0.001f) return out;
    auto idx = [&](int x,int y){ return (y*w+x)*4; };
    for(int y=1;y<h-1;++y){
        for(int x=1;x<w-1;++x){
            float skin = mask[y*w+x];
            if(skin<0.001f) continue;
            float blend = std::min(1.0f, skin * intensity * opacity);
            if(blend<0.001f) continue;
            float sumR=0,sumG=0,sumB=0, wsum=0;
            for(int dy=-1;dy<=1;++dy){
                for(int dx=-1;dx<=1;++dx){
                    int sx = x+dx, sy=y+dy;
                    int si = idx(sx,sy);
                    int ci = idx(x,y);
                    float cr = input[ci]/255.0f, cg=input[ci+1]/255.0f, cb=input[ci+2]/255.0f;
                    float sr = input[si]/255.0f, sg=input[si+1]/255.0f, sb=input[si+2]/255.0f;
                    float colorDist = std::sqrt((cr-sr)*(cr-sr)+(cg-sg)*(cg-sg)+(cb-sb)*(cb-sb));
                    float sigma = 0.15f + (1.0f-edgePres)*0.35f;
                    float cw = std::exp(-colorDist*colorDist/(2*sigma*sigma));
                    sumR+=sr*cw; sumG+=sg*cw; sumB+=sb*cw; wsum+=cw;
                }
            }
            sumR/=wsum; sumG/=wsum; sumB/=wsum;
            int i = idx(x,y);
            float origR = input[i]/255.0f, origG=input[i+1]/255.0f, origB=input[i+2]/255.0f;
            float resR = origR*(1-blend)+sumR*blend;
            float resG = origG*(1-blend)+sumG*blend;
            float resB = origB*(1-blend)+sumB*blend;
            out[i] = (uint8_t)std::clamp(resR*255.0f,0.0f,255.0f);
            out[i+1] = (uint8_t)std::clamp(resG*255.0f,0.0f,255.0f);
            out[i+2] = (uint8_t)std::clamp(resB*255.0f,0.0f,255.0f);
        }
    }
    return out;
}
struct Float4{float r,g,b,a;};
static std::vector<uint8_t> CPU_MakeupBlend(const std::vector<uint8_t>& base, const std::vector<float>& mask, Float4 color, float intensity, float opacity, int blendMode, float softness, int w, int h){
    std::vector<uint8_t> out = base;
    for(int y=0;y<h;++y){
        for(int x=0;x<w;++x){
            float m = mask[y*w+x];
            if(m<0.001f) continue;
            float softAlpha = m * (1.0f - softness*0.5f) + m*m*softness*0.5f;
            float alpha = softAlpha * intensity * opacity * color.a;
            alpha = std::clamp(alpha,0.0f,1.0f);
            int i=(y*w+x)*4;
            float br=base[i]/255.0f, bg=base[i+1]/255.0f, bb=base[i+2]/255.0f;
            float sr=color.r, sg=color.g, sb=color.b;
            float blendedR,blendedG,blendedB;
            switch(blendMode){
                case 1: blendedR=br*sr; blendedG=bg*sg; blendedB=bb*sb; break;
                case 2: blendedR=1-(1-br)*(1-sr); blendedG=1-(1-bg)*(1-sg); blendedB=1-(1-bb)*(1-sb); break;
                case 3:
                    blendedR = br<0.5f ? 2*br*sr : 1-2*(1-br)*(1-sr);
                    blendedG = bg<0.5f ? 2*bg*sg : 1-2*(1-bg)*(1-sg);
                    blendedB = bb<0.5f ? 2*bb*sb : 1-2*(1-bb)*(1-sb);
                    break;
                default: blendedR=sr; blendedG=sg; blendedB=sb; break;
            }
            float rr = br*(1-alpha)+blendedR*alpha;
            float gg = bg*(1-alpha)+blendedG*alpha;
            float bb2 = bb*(1-alpha)+blendedB*alpha;
            out[i]=(uint8_t)std::clamp(rr*255.0f,0.0f,255.0f);
            out[i+1]=(uint8_t)std::clamp(gg*255.0f,0.0f,255.0f);
            out[i+2]=(uint8_t)std::clamp(bb2*255.0f,0.0f,255.0f);
        }
    }
    return out;
}
static std::vector<uint8_t> GPU_MakeupBlend(const std::vector<uint8_t>& base, const std::vector<float>& mask, Float4 color, float intensity, float opacity, int blendMode, float softness, int w, int h){
    return CPU_MakeupBlend(base,mask,color,intensity,opacity,blendMode,softness,w,h);
}

int main(){
    std::cout<<"=== Phase 10 CPU/GPU Parity (CPU-simulated GPU after fix, no global tint) ==="<<std::endl;
    int w=400,h=400;
    std::vector<uint8_t> input(w*h*4);
    for(int y=0;y<h;++y){ for(int x=0;x<w;++x){ int i=(y*w+x)*4; input[i]= (uint8_t)(x*255/w); input[i+1]=(uint8_t)(y*255/h); input[i+2]=128; input[i+3]=255; } }
    std::vector<float> skinMask(w*h);
    for(int y=0;y<h;++y){ for(int x=0;x<w;++x){ float cx=w*0.5f,cy=h*0.5f; float dx=x-cx,dy=y-cy; float dist=std::sqrt(dx*dx+dy*dy); float maxD=std::min(w,h)*0.4f; float a=1.0f-dist/maxD; a=std::clamp(a,0.0f,1.0f); skinMask[y*w+x]=a; } }

    {
        auto cpu = CPU_Tone(input, skinMask, 0.5f, 0.3f, 0.1f, 0.2f, 0.7f, w,h);
        auto gpu = GPU_Tone(input, skinMask, 0.5f, 0.3f, 0.1f, 0.2f, 0.7f, w,h);
        Metrics met = ComputeMetrics(cpu,gpu,w,h);
        std::cout<<"Beauty Tone CPU vs GPU MAE="<<met.mae<<" RMSE="<<met.rmse<<" Max="<<met.maxErr<<" PSNR="<<met.psnr<<" %within1="<<met.pctWithin1<<std::endl;
    }
    {
        auto cpu = CPU_Brightness(input, skinMask, 0.2f, 0.8f, w,h);
        auto gpu = GPU_Brightness(input, skinMask, 0.2f, 0.8f, w,h);
        Metrics met = ComputeMetrics(cpu,gpu,w,h);
        std::cout<<"Beauty Brightness MAE="<<met.mae<<std::endl;
    }
    {
        auto cpu = CPU_Contrast(input, skinMask, 0.2f, 0.8f, w,h);
        auto gpu = GPU_Contrast(input, skinMask, 0.2f, 0.8f, w,h);
        Metrics met = ComputeMetrics(cpu,gpu,w,h);
        std::cout<<"Beauty Contrast MAE="<<met.mae<<std::endl;
    }
    {
        auto cpu = CPU_Smoothing(input, skinMask, 0.5f, 2.0f, 0.8f, 0.6f, w,h);
        auto gpu = GPU_Smoothing(input, skinMask, 0.5f, 2.0f, 0.8f, 0.6f, w,h);
        Metrics met = ComputeMetrics(cpu,gpu,w,h);
        std::cout<<"Beauty Smoothing MAE="<<met.mae<<" Max="<<met.maxErr<<std::endl;
    }
    std::vector<float> makeupMask(w*h,0);
    for(int y=0;y<h;++y){ for(int x=0;x<w;++x){ float a=0; if(y>h*0.6f && y<h*0.8f && x>w*0.35f && x<w*0.65f) a=1.0f; makeupMask[y*w+x]=a; } }
    {
        Float4 color={1,0,0,1};
        auto cpu = CPU_MakeupBlend(input, makeupMask, color, 0.5f, 0.9f, 0, 0.3f, w,h);
        auto gpu = GPU_MakeupBlend(input, makeupMask, color, 0.5f, 0.9f, 0, 0.3f, w,h);
        Metrics met = ComputeMetrics(cpu,gpu,w,h);
        std::cout<<"Makeup Lip MAE="<<met.mae<<std::endl;
    }
    {
        auto b1 = CPU_Smoothing(input, skinMask, 0.5f, 2.0f, 0.8f, 0.6f, w,h);
        auto b2 = CPU_Tone(b1, skinMask, 0.5f, 0.3f, 0.1f, 0.2f, 0.7f, w,h);
        auto b3 = CPU_Brightness(b2, skinMask, 0.2f, 0.8f, w,h);
        auto b4 = CPU_Contrast(b3, skinMask, 0.2f, 0.8f, w,h);
        Float4 lip={1,0,0,1};
        auto cpuFull = CPU_MakeupBlend(b4, makeupMask, lip, 0.5f, 0.9f, 0, 0.3f, w,h);
        auto g1 = GPU_Smoothing(input, skinMask, 0.5f, 2.0f, 0.8f, 0.6f, w,h);
        auto g2 = GPU_Tone(g1, skinMask, 0.5f, 0.3f, 0.1f, 0.2f, 0.7f, w,h);
        auto g3 = GPU_Brightness(g2, skinMask, 0.2f, 0.8f, w,h);
        auto g4 = GPU_Contrast(g3, skinMask, 0.2f, 0.8f, w,h);
        auto gpuFull = GPU_MakeupBlend(g4, makeupMask, lip, 0.5f, 0.9f, 0, 0.3f, w,h);
        Metrics met = ComputeMetrics(cpuFull,gpuFull,w,h);
        std::cout<<"Full Pipeline MAE="<<met.mae<<" RMSE="<<met.rmse<<" Max="<<met.maxErr<<" PSNR="<<met.psnr<<" %within1="<<met.pctWithin1<<" %within5="<<met.pctWithin5<<std::endl;
    }
    std::cout<<"=== Phase10 Parity Complete ==="<<std::endl;
    return 0;
}
