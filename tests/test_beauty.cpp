/**
 * Test Beauty — Phase 7 Full Beauty & Face Retouching Engine
 * Covers BeautyMaskGeneration, MaskBounds, MaskExclusion, MaskFeather,
 * BeautyParameter, ParameterSensitivity, SkinSmoothing, TextureRefinement,
 * BlemishReduction, SkinTone, Brightness, Contrast, FaceRetouch,
 * CPUBeautyRender, D3D11BeautyRender, MultiFace, Temporal, Mirror, Rotation,
 * BeautyMakeupPipeline, Regression, RAII
 */

#include "../sdk/src/beauty/beauty_mask.h"
#include "../sdk/src/beauty/beauty_params.h"
#include "../sdk/src/beauty/beauty_renderer.h"
#include "../sdk/src/makeup/makeup_renderer.h"
#include "../sdk/src/face/production_face_tracker.h"
#include "../sdk/src/face/coordinate_transform.h"
#include "../sdk/include/huanface_c_api.h"
#include <iostream>
#include <cmath>
#include <vector>
#include <chrono>

using namespace huanface;

static int checks=0, passed=0;
static void CHECK(bool cond, const char* msg){
    checks++;
    if(cond) passed++;
    else std::cout<<"  FAIL: "<<msg<<std::endl;
}

static HFFaceData CreateTestFace(int w=200, int h=200) {
    HFFaceData face;
    face.bboxX = w*0.2f;
    face.bboxY = h*0.2f;
    face.bboxW = w*0.6f;
    face.bboxH = h*0.6f;
    face.confidence = 0.8f;
    face.detectionConfidence = 0.6f;
    float fx=face.bboxX, fy=face.bboxY, fw=face.bboxW, fh=face.bboxH;
    for(int i=0;i<17;++i){
        float t = i/16.0f;
        float x = fx + fw * (0.1f + 0.8f*t);
        float y = fy + fh * (0.8f + 0.15f*std::sin(t*3.14159f));
        face.landmarks.emplace_back(x,y);
        face.landmarks3D.emplace_back(x,y,0.0f);
    }
    for(int i=0;i<5;++i){
        float x = fx + fw*(0.2f + 0.15f*i/4.0f);
        float y = fy + fh*0.35f;
        face.landmarks.emplace_back(x,y);
        face.landmarks3D.emplace_back(x,y,0.0f);
    }
    for(int i=0;i<5;++i){
        float x = fx + fw*(0.6f + 0.15f*i/4.0f);
        float y = fy + fh*0.35f;
        face.landmarks.emplace_back(x,y);
        face.landmarks3D.emplace_back(x,y,0.0f);
    }
    for(int i=0;i<4;++i){
        float x = fx + fw*0.5f;
        float y = fy + fh*(0.45f + 0.1f*i/3.0f);
        face.landmarks.emplace_back(x,y);
        face.landmarks3D.emplace_back(x,y,0.0f);
    }
    for(int i=0;i<5;++i){
        float x = fx + fw*(0.45f + 0.1f*i/4.0f);
        float y = fy + fh*0.6f;
        face.landmarks.emplace_back(x,y);
        face.landmarks3D.emplace_back(x,y,0.0f);
    }
    float re_cx = fx + fw*0.35f, re_cy = fy + fh*0.45f;
    for(int i=0;i<6;++i){
        float ang = i*3.14159f*2.0f/6.0f;
        float x = re_cx + 10.0f*std::cos(ang);
        float y = re_cy + 6.0f*std::sin(ang);
        face.landmarks.emplace_back(x,y);
        face.landmarks3D.emplace_back(x,y,0.0f);
    }
    float le_cx = fx + fw*0.65f, le_cy = fy + fh*0.45f;
    for(int i=0;i<6;++i){
        float ang = i*3.14159f*2.0f/6.0f;
        float x = le_cx + 10.0f*std::cos(ang);
        float y = le_cy + 6.0f*std::sin(ang);
        face.landmarks.emplace_back(x,y);
        face.landmarks3D.emplace_back(x,y,0.0f);
    }
    float lip_cx = fx + fw*0.5f, lip_cy = fy + fh*0.75f;
    for(int i=0;i<12;++i){
        float ang = i*3.14159f*2.0f/12.0f;
        float x = lip_cx + 20.0f*std::cos(ang);
        float y = lip_cy + 10.0f*std::sin(ang);
        face.landmarks.emplace_back(x,y);
        face.landmarks3D.emplace_back(x,y,0.0f);
    }
    for(int i=0;i<8;++i){
        float ang = i*3.14159f*2.0f/8.0f;
        float x = lip_cx + 10.0f*std::cos(ang);
        float y = lip_cy + 5.0f*std::sin(ang);
        face.landmarks.emplace_back(x,y);
        face.landmarks3D.emplace_back(x,y,0.0f);
    }
    // Mesh 77v dummy from landmarks
    for(int i=0;i<77;++i){
        float x = fx + fw*0.5f + (i%11-5)*5.0f;
        float y = fy + fh*0.5f + (i/11-3)*5.0f;
        face.mesh.vertices.emplace_back(x,y,0);
        face.mesh.uv.emplace_back(x/200.0f, y/200.0f);
        face.mesh.vertexRegions.push_back(FaceRegion::FACE);
    }
    for(int i=0;i<111;++i){
        int a=i%77, b=(i+1)%77, c=(i+2)%77;
        face.mesh.indices.push_back(a);
        face.mesh.indices.push_back(b);
        face.mesh.indices.push_back(c);
    }
    return face;
}

static HFImage CreateTestImage(int w=200, int h=200) {
    HFImage img;
    img.width=w; img.height=h; img.channels=4;
    img.data.resize(w*h*4);
    for(int y=0;y<h;++y){
        for(int x=0;x<w;++x){
            uint8_t* px=&img.data[(y*w+x)*4];
            // Skin-like base with some noise to test smoothing
            px[0]=(uint8_t)(180 + (x%10) + (y%7));
            px[1]=(uint8_t)(150 + (x%8) + (y%5));
            px[2]=(uint8_t)(130 + (x%6) + (y%9));
            px[3]=255;
            // Add blemish-like spots for blemish test
            if((x-100)*(x-100)+(y-100)*(y-100)<25) {
                px[0]=50; px[1]=30; px[2]=20;
            }
        }
    }
    return img;
}

static float ComputeMeanDiff(const HFImage& a, const HFImage& b) {
    if(a.width!=b.width||a.height!=b.height||a.channels!=b.channels) return 0.0f;
    double sum=0;
    size_t total = a.data.size();
    for(size_t i=0;i<total;++i){
        sum += std::abs((int)a.data[i]-(int)b.data[i]);
    }
    return (float)(sum/total);
}

int RunBeautyTests() {
    std::cout<<"=== Test Beauty Phase 7 ==="<<std::endl;
    checks=0; passed=0;

    // ========================================================================
    // BeautyMaskGenerationTests
    // ========================================================================
    {
        HFBeautyMaskGenerator gen;
        HFFaceData face = CreateTestFace(200,200);
        std::map<BeautyMaskType, HFBeautyMask> masks;
        std::string err;
        bool ok = gen.GenerateAllMasks(face,200,200,masks,err);
        CHECK(ok, "GenerateAllMasks should succeed");
        CHECK(masks.size()>=8, "Should have at least 8 masks (minimal regions)");
        // Check required types
        CHECK(masks.find(BeautyMaskType::Face)!=masks.end(), "Face mask exists");
        CHECK(masks.find(BeautyMaskType::Forehead)!=masks.end(), "Forehead mask exists");
        CHECK(masks.find(BeautyMaskType::LeftCheek)!=masks.end(), "LeftCheek exists");
        CHECK(masks.find(BeautyMaskType::RightCheek)!=masks.end(), "RightCheek exists");
        CHECK(masks.find(BeautyMaskType::Nose)!=masks.end(), "Nose exists");
        CHECK(masks.find(BeautyMaskType::Chin)!=masks.end(), "Chin exists");
        CHECK(masks.find(BeautyMaskType::UnderEyeLeft)!=masks.end(), "UnderEyeLeft exists");
        CHECK(masks.find(BeautyMaskType::UnderEyeRight)!=masks.end(), "UnderEyeRight exists");
        CHECK(masks.find(BeautyMaskType::Skin)!=masks.end(), "Skin mask exists");

        // Validate each mask
        for(auto& kv: masks){
            CHECK(kv.second.IsValid(), ("Mask valid "+BeautyMaskTypeToString(kv.first)).c_str());
            CHECK(kv.second.HasFinite(), ("Mask finite "+BeautyMaskTypeToString(kv.first)).c_str());
            CHECK(kv.second.MinAlpha()>=0.0f, ("Mask min>=0 "+BeautyMaskTypeToString(kv.first)).c_str());
            CHECK(kv.second.MaxAlpha()<=1.001f, ("Mask max<=1 "+BeautyMaskTypeToString(kv.first)).c_str());
            CHECK(kv.second.HasNonZero(), ("Mask non-zero "+BeautyMaskTypeToString(kv.first)).c_str());
        }

        // Skin mask should exclude eyes/lips/brows
        auto itSkin = masks.find(BeautyMaskType::Skin);
        auto itEye = masks.find(BeautyMaskType::EyeExclusion);
        auto itLip = masks.find(BeautyMaskType::LipExclusion);
        if(itSkin!=masks.end() && itEye!=masks.end() && itLip!=masks.end()){
            std::string exclusionErr;
            bool exclOk = HFBeautyMaskGenerator::ValidateSkinExclusion(itSkin->second, itEye->second, itLip->second, exclusionErr);
            CHECK(exclOk, ("Skin exclusion valid: "+exclusionErr).c_str());
        }

        // Mask follows landmarks
        HFFaceData face2 = CreateTestFace(200,200);
        for(auto& lm: face2.landmarks){ lm.x+=20; lm.y+=10; }
        bool follows = HFBeautyMaskGenerator::ValidateMaskFollowsLandmarks(face, face2, BeautyMaskType::Face, 200,200);
        CHECK(follows, "Mask follows landmark movement");
    }

    // ========================================================================
    // BeautyMaskBoundsTests
    // ========================================================================
    {
        HFBeautyMaskGenerator gen;
        HFFaceData face = CreateTestFace(200,200);
        HFBeautyMask mask;
        std::string err;
        bool ok = gen.GenerateFaceMask(face,200,200,mask,err);
        CHECK(ok, "Face mask generation for bounds");
        if(ok){
            CHECK(mask.width==200 && mask.height==200, "Mask dimensions match image");
            // Bounds inside image not full coverage
            float coverage = mask.Coverage();
            CHECK(coverage>0.01f && coverage<0.8f, "Face coverage reasonable 0.01-0.8");
            // Check not covering entire image (background excluded)
            int cornerAlphaCount=0;
            if(mask.GetAlpha(0,0)>0.5f) cornerAlphaCount++;
            if(mask.GetAlpha(199,0)>0.5f) cornerAlphaCount++;
            if(mask.GetAlpha(0,199)>0.5f) cornerAlphaCount++;
            if(mask.GetAlpha(199,199)>0.5f) cornerAlphaCount++;
            CHECK(cornerAlphaCount==0, "Face mask should not cover far corners (background)");
        }
    }

    // ========================================================================
    // BeautyMaskExclusionTests
    // ========================================================================
    {
        HFBeautyMaskGenerator gen;
        HFFaceData face = CreateTestFace(200,200);
        HFBeautyMask skinMask, eyeMask, lipMask, browMask;
        std::string err;
        bool ok1 = gen.GenerateSkinMask(face,200,200,skinMask,err);
        bool ok2 = gen.GenerateEyeExclusionMask(face,200,200,eyeMask,err);
        bool ok3 = gen.GenerateLipExclusionMask(face,200,200,lipMask,err);
        bool ok4 = gen.GenerateBrowExclusionMask(face,200,200,browMask,err);
        CHECK(ok1&&ok2&&ok3&&ok4, "All exclusion masks generated");
        if(ok1&&ok2&&ok3){
            std::string e;
            bool valid = HFBeautyMaskGenerator::ValidateSkinExclusion(skinMask, eyeMask, lipMask, e);
            CHECK(valid, ("Skin excludes eyes/lips: "+e).c_str());
        }
        // Check eye exclusion actually covers eye area
        if(ok2){
            CHECK(eyeMask.Coverage()>0.001f, "Eye exclusion non-zero");
            CHECK(eyeMask.Coverage()<0.1f, "Eye exclusion not too large");
        }
        if(ok3){
            CHECK(lipMask.Coverage()>0.001f, "Lip exclusion non-zero");
        }
        if(ok4){
            CHECK(browMask.Coverage()>0.001f, "Brow exclusion non-zero");
        }
    }

    // ========================================================================
    // BeautyMaskFeatherTests
    // ========================================================================
    {
        HFBeautyMaskGenerator gen;
        HFFaceData face = CreateTestFace(200,200);
        HFBeautyMask mask;
        std::string err;
        gen.GenerateFaceMask(face,200,200,mask,err);
        float beforeMax = mask.MaxAlpha();
        HFBeautyMaskGenerator::ApplyFeather(mask, 3.0f);
        CHECK(mask.HasFinite(), "Feathered mask finite");
        CHECK(mask.MinAlpha()>=0.0f && mask.MaxAlpha()<=1.001f, "Feathered mask alpha 0-1");
        // Feather should reduce hard edges, but not destroy mask
        CHECK(mask.HasNonZero(), "Feathered mask still non-zero");
    }

    // ========================================================================
    // BeautyParameterTests
    // ========================================================================
    {
        HFBeautyParameters params;
        CHECK(params.IsValid(), "Default beauty params valid");
        CHECK(params.enabled, "Beauty enabled by default");
        CHECK(params.smoothing.intensity>=0 && params.smoothing.intensity<=1, "Smoothing intensity 0-1");
        CHECK(params.texture.intensity>=0 && params.texture.intensity<=1, "Texture intensity 0-1");
        CHECK(params.blemish.intensity>=0 && params.blemish.intensity<=1, "Blemish intensity 0-1");
        CHECK(params.tone.temperature>=-1 && params.tone.temperature<=1, "Tone temperature -1 to 1");
        CHECK(params.brightness.intensity>=-1 && params.brightness.intensity<=1, "Brightness -1 to 1");
        CHECK(params.contrast.intensity>=-1 && params.contrast.intensity<=1, "Contrast -1 to 1 neutral 0");

        auto floatMap = params.ToFloatMap();
        CHECK(floatMap.size()>10, "ToFloatMap has many entries");
        CHECK(floatMap.find("beauty.smoothing.intensity")!=floatMap.end(), "FloatMap has smoothing");
        CHECK(floatMap.find("beauty.brightness.intensity")!=floatMap.end(), "FloatMap has brightness");
    }

    // ========================================================================
    // BeautyParameterSensitivityTests — intensity 0 vs 0.5 vs 1 must differ
    // ========================================================================
    {
        CPUBeautyRenderer renderer;
        renderer.Init();
        HFFaceData face = CreateTestFace(200,200);
        HFImage input = CreateTestImage(200,200);
        HFBeautyMaskGenerator maskGen;
        std::map<BeautyMaskType, HFBeautyMask> masks;
        std::string err;
        maskGen.GenerateAllMasks(face,200,200,masks,err);
        auto itSkin = masks.find(BeautyMaskType::Skin);
        CHECK(itSkin!=masks.end(), "Skin mask for sensitivity test");

        if(itSkin!=masks.end()){
            // Smoothing sensitivity
            HFImage out0, out05, out1;
            HFSkinSmoothingParams p0, p05, p1;
            p0.enabled=true; p0.intensity=0.0f; p0.radius=2.0f; p0.opacity=0.8f; p0.edgePreservation=0.6f;
            p05.enabled=true; p05.intensity=0.5f; p05.radius=2.0f; p05.opacity=0.8f; p05.edgePreservation=0.6f;
            p1.enabled=true; p1.intensity=1.0f; p1.radius=2.0f; p1.opacity=0.8f; p1.edgePreservation=0.6f;

            renderer.RenderSmoothing(input, face, itSkin->second, p0, out0, err);
            renderer.RenderSmoothing(input, face, itSkin->second, p05, out05, err);
            renderer.RenderSmoothing(input, face, itSkin->second, p1, out1, err);

            float diff0_05 = ComputeMeanDiff(out0, out05);
            float diff05_1 = ComputeMeanDiff(out05, out1);
            float diff0_1 = ComputeMeanDiff(out0, out1);
            CHECK(diff0_05>0.1f, ("Smoothing 0 vs 0.5 diff>epsilon: "+std::to_string(diff0_05)).c_str());
            CHECK(diff05_1>0.1f, ("Smoothing 0.5 vs 1 diff>epsilon: "+std::to_string(diff05_1)).c_str());
            CHECK(diff0_1>0.2f, ("Smoothing 0 vs 1 diff>0.2: "+std::to_string(diff0_1)).c_str());

            // Texture sensitivity
            HFSkinTextureParams tp0, tp1;
            tp0.enabled=true; tp0.intensity=0.0f; tp0.preservation=0.7f; tp0.opacity=0.7f;
            tp1.enabled=true; tp1.intensity=1.0f; tp1.preservation=0.7f; tp1.opacity=0.7f;
            HFImage tout0, tout1;
            renderer.RenderTextureRefinement(input, face, itSkin->second, tp0, tout0, err);
            renderer.RenderTextureRefinement(input, face, itSkin->second, tp1, tout1, err);
            float diffTex = ComputeMeanDiff(tout0, tout1);
            CHECK(diffTex>0.1f, ("Texture 0 vs 1 diff>epsilon: "+std::to_string(diffTex)).c_str());

            // Blemish sensitivity
            HFBlemishReductionParams bp0, bp1;
            bp0.enabled=true; bp0.intensity=0.0f; bp0.radius=2.5f; bp0.opacity=0.8f;
            bp1.enabled=true; bp1.intensity=1.0f; bp1.radius=2.5f; bp1.opacity=0.8f;
            HFImage bout0, bout1;
            renderer.RenderBlemishReduction(input, face, itSkin->second, bp0, bout0, err);
            renderer.RenderBlemishReduction(input, face, itSkin->second, bp1, bout1, err);
            float diffBlem = ComputeMeanDiff(bout0, bout1);
            CHECK(diffBlem>0.1f, ("Blemish 0 vs 1 diff>epsilon: "+std::to_string(diffBlem)).c_str());

            // Tone sensitivity
            HFSkinToneParams tone0, tone1;
            tone0.enabled=true; tone0.intensity=0.0f; tone0.temperature=0; tone0.tint=0; tone0.saturation=0;
            tone1.enabled=true; tone1.intensity=1.0f; tone1.temperature=0.5f; tone1.tint=0.2f; tone1.saturation=0.3f;
            HFImage toneOut0, toneOut1;
            renderer.RenderSkinTone(input, face, itSkin->second, tone0, toneOut0, err);
            renderer.RenderSkinTone(input, face, itSkin->second, tone1, toneOut1, err);
            float diffTone = ComputeMeanDiff(toneOut0, toneOut1);
            CHECK(diffTone>0.1f, ("Tone 0 vs 1 diff>epsilon: "+std::to_string(diffTone)).c_str());

            // Brightness sensitivity
            HFBrightnessParams bright0, bright1;
            bright0.enabled=true; bright0.intensity=0.0f; bright0.opacity=0.8f; bright0.skinOnly=true;
            bright1.enabled=true; bright1.intensity=0.8f; bright1.opacity=0.8f; bright1.skinOnly=true;
            HFImage brightOut0, brightOut1;
            renderer.RenderBrightness(input, face, itSkin->second, bright0, brightOut0, err);
            renderer.RenderBrightness(input, face, itSkin->second, bright1, brightOut1, err);
            float diffBright = ComputeMeanDiff(brightOut0, brightOut1);
            CHECK(diffBright>0.1f, ("Brightness 0 vs 0.8 diff>epsilon: "+std::to_string(diffBright)).c_str());

            // Contrast sensitivity
            HFContrastParams cont0, cont1, contNeg;
            cont0.enabled=true; cont0.intensity=0.0f; cont0.opacity=0.8f; cont0.skinOnly=true;
            cont1.enabled=true; cont1.intensity=0.8f; cont1.opacity=0.8f; cont1.skinOnly=true;
            contNeg.enabled=true; contNeg.intensity=-0.8f; contNeg.opacity=0.8f; contNeg.skinOnly=true;
            HFImage contOut0, contOut1, contOutNeg;
            renderer.RenderContrast(input, face, itSkin->second, cont0, contOut0, err);
            renderer.RenderContrast(input, face, itSkin->second, cont1, contOut1, err);
            renderer.RenderContrast(input, face, itSkin->second, contNeg, contOutNeg, err);
            float diffCont0_1 = ComputeMeanDiff(contOut0, contOut1);
            float diffContNeg_1 = ComputeMeanDiff(contOutNeg, contOut1);
            CHECK(diffCont0_1>0.1f, ("Contrast 0 vs 0.8 diff>epsilon: "+std::to_string(diffCont0_1)).c_str());
            CHECK(diffContNeg_1>0.1f, ("Contrast -0.8 vs 0.8 diff>epsilon: "+std::to_string(diffContNeg_1)).c_str());
        }
        renderer.Shutdown();
    }

    // ========================================================================
    // SkinSmoothingTests
    // ========================================================================
    {
        CPUBeautyRenderer renderer;
        renderer.Init();
        HFFaceData face = CreateTestFace(200,200);
        HFImage input = CreateTestImage(200,200);
        HFBeautyMaskGenerator maskGen;
        std::map<BeautyMaskType, HFBeautyMask> masks;
        std::string err;
        maskGen.GenerateAllMasks(face,200,200,masks,err);
        auto itSkin = masks.find(BeautyMaskType::Skin);
        if(itSkin!=masks.end()){
            HFSkinSmoothingParams params;
            params.enabled=true; params.intensity=0.8f; params.radius=2.0f; params.opacity=0.8f; params.edgePreservation=0.6f;
            HFImage output;
            HFResult r = renderer.RenderSmoothing(input, face, itSkin->second, params, output, err);
            CHECK(r==HF_RESULT_OK, "Smoothing render OK");
            float diff = ComputeMeanDiff(input, output);
            CHECK(diff>0.1f, ("Smoothing changes output diff="+std::to_string(diff)).c_str());
            // Check eyes/lips protected: eye area should have less change
            // Sample eye region (around 70,90) and skin region (100,100) - eye should have smaller diff if protected
            // For simplicity, check that output is not just global blur: background corners should be unchanged (mask 0)
            int bgX=5, bgY=5;
            bool bgUnchanged = true;
            for(int c=0;c<3;++c){
                if(std::abs((int)input.data[(bgY*200+bgX)*4+c] - (int)output.data[(bgY*200+bgX)*4+c])>2) bgUnchanged=false;
            }
            CHECK(bgUnchanged, "Smoothing should not affect background (mask 0)");
        }
        renderer.Shutdown();
    }

    // ========================================================================
    // TextureRefinementTests
    // ========================================================================
    {
        CPUBeautyRenderer renderer;
        renderer.Init();
        HFFaceData face = CreateTestFace(200,200);
        HFImage input = CreateTestImage(200,200);
        HFBeautyMaskGenerator maskGen;
        std::map<BeautyMaskType, HFBeautyMask> masks;
        std::string err;
        maskGen.GenerateAllMasks(face,200,200,masks,err);
        auto itSkin = masks.find(BeautyMaskType::Skin);
        if(itSkin!=masks.end()){
            HFSkinTextureParams params;
            params.enabled=true; params.intensity=0.8f; params.preservation=0.7f; params.opacity=0.7f;
            HFImage output;
            HFResult r = renderer.RenderTextureRefinement(input, face, itSkin->second, params, output, err);
            CHECK(r==HF_RESULT_OK, "Texture refinement OK");
            float diff = ComputeMeanDiff(input, output);
            CHECK(diff>0.05f, ("Texture refinement changes output diff="+std::to_string(diff)).c_str());
        }
        renderer.Shutdown();
    }

    // ========================================================================
    // BlemishReductionTests
    // ========================================================================
    {
        CPUBeautyRenderer renderer;
        renderer.Init();
        HFFaceData face = CreateTestFace(200,200);
        HFImage input = CreateTestImage(200,200);
        HFBeautyMaskGenerator maskGen;
        std::map<BeautyMaskType, HFBeautyMask> masks;
        std::string err;
        maskGen.GenerateAllMasks(face,200,200,masks,err);
        auto itSkin = masks.find(BeautyMaskType::Skin);
        if(itSkin!=masks.end()){
            HFBlemishReductionParams params;
            params.enabled=true; params.intensity=0.8f; params.radius=2.5f; params.opacity=0.8f;
            HFImage output;
            HFResult r = renderer.RenderBlemishReduction(input, face, itSkin->second, params, output, err);
            CHECK(r==HF_RESULT_OK, "Blemish reduction OK");
            float diff = ComputeMeanDiff(input, output);
            CHECK(diff>0.05f, ("Blemish reduction changes output diff="+std::to_string(diff)).c_str());
            // Should not be global blur — background unchanged
            int bgX=5, bgY=5;
            bool bgUnchanged = true;
            for(int c=0;c<3;++c){
                if(std::abs((int)input.data[(bgY*200+bgX)*4+c] - (int)output.data[(bgY*200+bgX)*4+c])>2) bgUnchanged=false;
            }
            CHECK(bgUnchanged, "Blemish should not affect background");
        }
        renderer.Shutdown();
    }

    // ========================================================================
    // SkinToneTests
    // ========================================================================
    {
        CPUBeautyRenderer renderer;
        renderer.Init();
        HFFaceData face = CreateTestFace(200,200);
        HFImage input = CreateTestImage(200,200);
        HFBeautyMaskGenerator maskGen;
        std::map<BeautyMaskType, HFBeautyMask> masks;
        std::string err;
        maskGen.GenerateAllMasks(face,200,200,masks,err);
        auto itSkin = masks.find(BeautyMaskType::Skin);
        if(itSkin!=masks.end()){
            HFSkinToneParams params;
            params.enabled=true; params.intensity=0.8f; params.temperature=0.5f; params.tint=0.2f; params.saturation=0.3f; params.opacity=0.7f;
            HFImage output;
            HFResult r = renderer.RenderSkinTone(input, face, itSkin->second, params, output, err);
            CHECK(r==HF_RESULT_OK, "Skin tone OK");
            float diff = ComputeMeanDiff(input, output);
            CHECK(diff>0.05f, ("Tone changes output diff="+std::to_string(diff)).c_str());
            // Background should be unchanged (skin mask)
            int bgX=5, bgY=5;
            bool bgUnchanged = true;
            for(int c=0;c<3;++c){
                if(std::abs((int)input.data[(bgY*200+bgX)*4+c] - (int)output.data[(bgY*200+bgX)*4+c])>2) bgUnchanged=false;
            }
            CHECK(bgUnchanged, "Tone should not affect background");
        }
        renderer.Shutdown();
    }

    // ========================================================================
    // BrightnessTests
    // ========================================================================
    {
        CPUBeautyRenderer renderer;
        renderer.Init();
        HFFaceData face = CreateTestFace(200,200);
        HFImage input = CreateTestImage(200,200);
        HFBeautyMaskGenerator maskGen;
        std::map<BeautyMaskType, HFBeautyMask> masks;
        std::string err;
        maskGen.GenerateAllMasks(face,200,200,masks,err);
        auto itSkin = masks.find(BeautyMaskType::Skin);
        if(itSkin!=masks.end()){
            HFBrightnessParams params;
            params.enabled=true; params.intensity=0.5f; params.opacity=0.8f; params.skinOnly=true;
            HFImage output;
            HFResult r = renderer.RenderBrightness(input, face, itSkin->second, params, output, err);
            CHECK(r==HF_RESULT_OK, "Brightness OK");
            float diff = ComputeMeanDiff(input, output);
            CHECK(diff>0.05f, ("Brightness changes output diff="+std::to_string(diff)).c_str());
            // Skin-only: background unchanged
            int bgX=5, bgY=5;
            bool bgUnchanged = true;
            for(int c=0;c<3;++c){
                if(std::abs((int)input.data[(bgY*200+bgX)*4+c] - (int)output.data[(bgY*200+bgX)*4+c])>2) bgUnchanged=false;
            }
            CHECK(bgUnchanged, "Brightness skin-only should not affect background");
        }
        renderer.Shutdown();
    }

    // ========================================================================
    // ContrastTests
    // ========================================================================
    {
        CPUBeautyRenderer renderer;
        renderer.Init();
        HFFaceData face = CreateTestFace(200,200);
        HFImage input = CreateTestImage(200,200);
        HFBeautyMaskGenerator maskGen;
        std::map<BeautyMaskType, HFBeautyMask> masks;
        std::string err;
        maskGen.GenerateAllMasks(face,200,200,masks,err);
        auto itSkin = masks.find(BeautyMaskType::Skin);
        if(itSkin!=masks.end()){
            HFContrastParams params0, paramsNeg, paramsPos;
            params0.enabled=true; params0.intensity=0.0f; params0.opacity=0.8f; params0.skinOnly=true;
            paramsNeg.enabled=true; paramsNeg.intensity=-0.5f; paramsNeg.opacity=0.8f; paramsNeg.skinOnly=true;
            paramsPos.enabled=true; paramsPos.intensity=0.5f; paramsPos.opacity=0.8f; paramsPos.skinOnly=true;
            HFImage out0, outNeg, outPos;
            renderer.RenderContrast(input, face, itSkin->second, params0, out0, err);
            renderer.RenderContrast(input, face, itSkin->second, paramsNeg, outNeg, err);
            renderer.RenderContrast(input, face, itSkin->second, paramsPos, outPos, err);
            float diff0Neg = ComputeMeanDiff(out0, outNeg);
            float diff0Pos = ComputeMeanDiff(out0, outPos);
            float diffNegPos = ComputeMeanDiff(outNeg, outPos);
            CHECK(diff0Neg>0.05f, ("Contrast 0 vs -0.5 diff="+std::to_string(diff0Neg)).c_str());
            CHECK(diff0Pos>0.05f, ("Contrast 0 vs 0.5 diff="+std::to_string(diff0Pos)).c_str());
            CHECK(diffNegPos>0.1f, ("Contrast -0.5 vs 0.5 diff="+std::to_string(diffNegPos)).c_str());
        }
        renderer.Shutdown();
    }

    // ========================================================================
    // FaceRetouchTests
    // ========================================================================
    {
        FullBeautyEngine engine;
        HFEngineConfigC config{}; config.maxFaces=5;
        HFResult r = engine.Init(config);
        CHECK(r==HF_RESULT_OK, "FullBeautyEngine init OK");
        if(r==HF_RESULT_OK){
            HFFaceData face = CreateTestFace(200,200);
            HFImage input = CreateTestImage(200,200);
            HFTrackingData tracking;
            tracking.faces.push_back(face);
            HFBeautyParameters params;
            params.enabled=true;
            params.retouch.enabled=true;
            params.retouch.intensity=0.8f;
            params.retouch.smoothing=0.5f;
            params.retouch.texture=0.3f;
            params.retouch.blemish=0.4f;
            params.retouch.tone=0.2f;
            params.retouch.brightness=0.1f;
            params.retouch.contrast=0.1f;
            params.retouch.opacity=0.9f;
            // Also enable individual features for retouch to use
            params.smoothing.enabled=true;
            params.texture.enabled=true;
            params.blemish.enabled=true;
            params.tone.enabled=true;
            params.brightness.enabled=true;
            params.contrast.enabled=true;

            HFImage output;
            std::string err;
            HFResult r2 = engine.ProcessCPU(input, tracking, params, output, err);
            CHECK(r2==HF_RESULT_OK, ("Retouch CPU process OK: "+err).c_str());
            if(r2==HF_RESULT_OK){
                float diff = ComputeMeanDiff(input, output);
                CHECK(diff>0.05f, ("Retouch changes output diff="+std::to_string(diff)).c_str());
            }
        }
        engine.Shutdown();
    }

    // ========================================================================
    // CPUBeautyRenderTests
    // ========================================================================
    {
        FullBeautyEngine engine;
        HFEngineConfigC config{}; config.maxFaces=5;
        engine.Init(config);
        HFFaceData face = CreateTestFace(200,200);
        HFImage input = CreateTestImage(200,200);
        HFTrackingData tracking;
        tracking.faces.push_back(face);
        HFBeautyParameters params;
        params.smoothing.enabled=true; params.smoothing.intensity=0.5f; params.smoothing.radius=2.0f;
        HFImage output;
        std::string err;
        HFResult r = engine.ProcessCPU(input, tracking, params, output, err);
        CHECK(r==HF_RESULT_OK, "CPUBeauty full process OK");
        CHECK(output.width==input.width && output.height==input.height, "CPU output dimensions match");
        engine.Shutdown();
    }

    // ========================================================================
    // D3D11BeautyRenderTests
    // ========================================================================
    {
        D3D11BeautyRenderer gpuRenderer;
        HFResult r = gpuRenderer.Init(nullptr, nullptr);
        CHECK(r==HF_RESULT_OK, "D3D11BeautyRenderer init OK (null device honest)");
        CHECK(gpuRenderer.IsInitialized(), "GPU renderer initialized");
        CHECK(gpuRenderer.AreShadersCompiled(), "GPU shaders compiled (validation via file content)");

        // Check shader files exist and contain real HLSL
        // We can't access file system here easily, but we can check compile log
        std::string log = gpuRenderer.GetShaderCompileLog();
        CHECK(!log.empty(), "Shader compile log not empty");

        // GPU process should return NOT_SUPPORTED in Linux CI
        HFFaceData face = CreateTestFace(200,200);
        HFImage input = CreateTestImage(200,200);
        HFBeautyMaskGenerator maskGen;
        std::map<BeautyMaskType, HFBeautyMask> masks;
        std::string err;
        maskGen.GenerateAllMasks(face,200,200,masks,err);
        HFBeautyParameters params;
        params.smoothing.enabled=true; params.smoothing.intensity=0.5f;
        HFImage output;
        HFResult r2 = gpuRenderer.ProcessFaceGPU(input, face, params, masks, output, err);
        CHECK(r2==HF_RESULT_NOT_SUPPORTED, "GPU process returns NOT_SUPPORTED in Linux CI honest");

        gpuRenderer.Shutdown();
    }

    // ========================================================================
    // MultiFaceBeautyTests
    // ========================================================================
    {
        FullBeautyEngine engine;
        HFEngineConfigC config{}; config.maxFaces=5;
        engine.Init(config);

        HFImage input = CreateTestImage(400,400);
        HFTrackingData tracking0, tracking1, tracking2;

        // 0 faces
        HFBeautyParameters params;
        params.smoothing.enabled=true; params.smoothing.intensity=0.5f;
        HFImage out0;
        std::string err;
        HFResult r0 = engine.ProcessCPU(input, tracking0, params, out0, err);
        CHECK(r0==HF_RESULT_OK, "MultiFace 0 faces OK");
        float diff0 = ComputeMeanDiff(input, out0);
        CHECK(diff0<0.01f, "0 faces should be unchanged");

        // 1 face
        HFFaceData face1 = CreateTestFace(400,400);
        tracking1.faces.push_back(face1);
        HFImage out1;
        HFResult r1 = engine.ProcessCPU(input, tracking1, params, out1, err);
        CHECK(r1==HF_RESULT_OK, "MultiFace 1 face OK");
        float diff1 = ComputeMeanDiff(input, out1);
        CHECK(diff1>0.05f, "1 face should have beauty applied");

        // 2 faces
        HFFaceData face2 = CreateTestFace(400,400);
        for(auto& lm: face2.landmarks){ lm.x+=100; }
        for(auto& v: face2.mesh.vertices){ v.x+=100; }
        tracking2.faces.push_back(face1);
        tracking2.faces.push_back(face2);
        HFImage out2;
        HFResult r2 = engine.ProcessCPU(input, tracking2, params, out2, err);
        CHECK(r2==HF_RESULT_OK, "MultiFace 2 faces OK");
        float diff2 = ComputeMeanDiff(input, out2);
        CHECK(diff2>0.05f, "2 faces should have beauty applied");
        // Both faces processed should differ from single face? At least not crash
        CHECK(out2.width==400 && out2.height==400, "2 faces output dimensions");

        engine.Shutdown();
    }

    // ========================================================================
    // TemporalBeautyTests
    // ========================================================================
    {
        HFBeautyMaskGenerator gen;
        HFFaceData faceA = CreateTestFace(200,200);
        HFFaceData faceB = CreateTestFace(200,200);
        HFFaceData faceC = CreateTestFace(200,200);
        // Slight movement
        for(auto& lm: faceB.landmarks){ lm.x+=2; lm.y+=1; }
        for(auto& lm: faceC.landmarks){ lm.x+=4; lm.y+=2; }

        HFBeautyMask maskA, maskB, maskC;
        std::string err;
        gen.GenerateSkinMask(faceA,200,200,maskA,err);
        gen.GenerateSkinMask(faceB,200,200,maskB,err);
        gen.GenerateSkinMask(faceC,200,200,maskC,err);

        // Check masks don't flicker/jump randomly — centroid movement should be small and consistent
        auto centroid = [](const HFBeautyMask& m){
            float cx=0,cy=0,sum=0;
            for(int y=0;y<m.height;++y) for(int x=0;x<m.width;++x){
                float a=m.GetAlpha(x,y);
                cx+=x*a; cy+=y*a; sum+=a;
            }
            if(sum>1e-6f){ cx/=sum; cy/=sum; }
            return std::pair<float,float>(cx,cy);
        };
        auto ca=centroid(maskA);
        auto cb=centroid(maskB);
        auto cc=centroid(maskC);
        float distAB = std::sqrt((ca.first-cb.first)*(ca.first-cb.first)+(ca.second-cb.second)*(ca.second-cb.second));
        float distBC = std::sqrt((cb.first-cc.first)*(cb.first-cc.first)+(cb.second-cc.second)*(cb.second-cc.second));
        float distAC = std::sqrt((ca.first-cc.first)*(ca.first-cc.first)+(ca.second-cc.second)*(ca.second-cc.second));
        // Small movement should result in small centroid movement, not random jump
        CHECK(distAB<10.0f, ("Temporal A->B centroid dist small: "+std::to_string(distAB)).c_str());
        CHECK(distBC<10.0f, ("Temporal B->C centroid dist small: "+std::to_string(distBC)).c_str());
        CHECK(distAC>distAB*0.5f, "Temporal movement consistent");
        // Coverage should be stable
        float covA=maskA.Coverage(), covB=maskB.Coverage(), covC=maskC.Coverage();
        CHECK(std::abs(covA-covB)<0.02f, "Temporal coverage stable A-B");
        CHECK(std::abs(covB-covC)<0.02f, "Temporal coverage stable B-C");
    }

    // ========================================================================
    // MirrorBeautyTests
    // ========================================================================
    {
        HFBeautyMaskGenerator gen;
        HFFaceData face = CreateTestFace(200,200);
        HFBeautyMask maskNormal, maskMirror;
        std::string err;
        gen.GenerateSkinMask(face,200,200,maskNormal,err);

        // Simulate mirror by flipping landmarks X
        HFFaceData faceMirror = face;
        for(auto& lm: faceMirror.landmarks){ lm.x = 200 - lm.x; }
        for(auto& v: faceMirror.mesh.vertices){ v.x = 200 - v.x; }
        gen.GenerateSkinMask(faceMirror,200,200,maskMirror,err);

        CHECK(maskMirror.IsValid(), "Mirror mask valid");
        CHECK(maskMirror.HasNonZero(), "Mirror mask non-zero");
        // Coverage should be similar
        CHECK(std::abs(maskNormal.Coverage()-maskMirror.Coverage())<0.05f, "Mirror coverage similar");
    }

    // ========================================================================
    // RotationBeautyTests
    // ========================================================================
    {
        HFBeautyMaskGenerator gen;
        HFFaceData face = CreateTestFace(200,200);
        // Test rotation 0/90/180/270 via CoordinateTransform if available
        // For now, just ensure mask generation works for rotated face positions
        HFFaceData face90 = face;
        // Simulate 90 degree rotation around center 100,100
        for(auto& lm: face90.landmarks){
            float x=lm.x-100, y=lm.y-100;
            float nx = -y + 100;
            float ny = x + 100;
            lm.x=nx; lm.y=ny;
        }
        for(auto& v: face90.mesh.vertices){
            float x=v.x-100, y=v.y-100;
            float nx = -y + 100;
            float ny = x + 100;
            v.x=nx; v.y=ny;
        }
        HFBeautyMask mask0, mask90;
        std::string err;
        bool ok0 = gen.GenerateSkinMask(face,200,200,mask0,err);
        bool ok90 = gen.GenerateSkinMask(face90,200,200,mask90,err);
        CHECK(ok0&&ok90, "Rotation masks generated");
        CHECK(mask90.HasNonZero(), "Rotated mask non-zero");
    }

    // ========================================================================
    // BeautyMakeupPipelineTests — Beauty before Makeup
    // ========================================================================
    {
        FullBeautyEngine beautyEngine;
        FullMakeupEngine makeupEngine;
        HFEngineConfigC config{}; config.maxFaces=5;
        beautyEngine.Init(config);
        makeupEngine.Init(config);

        HFFaceData face = CreateTestFace(200,200);
        HFImage input = CreateTestImage(200,200);
        HFTrackingData tracking;
        tracking.faces.push_back(face);

        HFBeautyParameters beautyParams;
        beautyParams.enabled=true;
        beautyParams.smoothing.enabled=true;
        beautyParams.smoothing.intensity=0.5f;
        beautyParams.smoothing.radius=2.0f;

        HFMakeupParameters makeupParams;
        makeupParams.lip.enabled=true;
        makeupParams.lip.intensity=0.8f;

        HFImage beautyOut, finalOut;
        std::string err;
        HFResult r1 = beautyEngine.ProcessCPU(input, tracking, beautyParams, beautyOut, err);
        CHECK(r1==HF_RESULT_OK, "Beauty->Makeup pipeline beauty step OK");

        HFResult r2 = makeupEngine.ProcessCPU(beautyOut, tracking, makeupParams, finalOut, err);
        CHECK(r2==HF_RESULT_OK, "Beauty->Makeup pipeline makeup step OK");

        float diffBeauty = ComputeMeanDiff(input, beautyOut);
        float diffFinal = ComputeMeanDiff(input, finalOut);
        CHECK(diffBeauty>0.05f, "Beauty changes output");
        CHECK(diffFinal>0.05f, "Beauty+Makeup changes output");
        CHECK(diffFinal>=diffBeauty*0.5f, "Final output includes beauty + makeup");

        beautyEngine.Shutdown();
        makeupEngine.Shutdown();
    }

    // ========================================================================
    // BeautyRegressionTests
    // ========================================================================
    {
        // Ensure beauty doesn't crash on various image sizes
        FullBeautyEngine engine;
        HFEngineConfigC config{}; config.maxFaces=5;
        engine.Init(config);
        std::vector<std::pair<int,int>> sizes = {{100,100},{400,400},{640,480}};
        for(auto& sz: sizes){
            HFFaceData face = CreateTestFace(sz.first, sz.second);
            HFImage input = CreateTestImage(sz.first, sz.second);
            HFTrackingData tracking;
            tracking.faces.push_back(face);
            HFBeautyParameters params;
            params.smoothing.enabled=true; params.smoothing.intensity=0.5f;
            HFImage output;
            std::string err;
            HFResult r = engine.ProcessCPU(input, tracking, params, output, err);
            CHECK(r==HF_RESULT_OK, ("Regression size "+std::to_string(sz.first)+"x"+std::to_string(sz.second)+" OK").c_str());
        }
        engine.Shutdown();
    }

    // ========================================================================
    // BeautyRAIITests
    // ========================================================================
    {
        // Test RAII: multiple init/shutdown cycles, no leak/crash
        for(int i=0;i<3;++i){
            FullBeautyEngine engine;
            HFEngineConfigC config{}; config.maxFaces=5;
            HFResult r = engine.Init(config);
            CHECK(r==HF_RESULT_OK, ("RAII init cycle "+std::to_string(i)+" OK").c_str());
            CHECK(engine.AreResourcesValid(), ("RAII resources valid cycle "+std::to_string(i)).c_str());
            engine.Shutdown();
        }
        // Test mask generator RAII
        {
            HFBeautyMaskGenerator gen;
            HFFaceData face = CreateTestFace(200,200);
            std::map<BeautyMaskType, HFBeautyMask> masks;
            std::string err;
            bool ok = gen.GenerateAllMasks(face,200,200,masks,err);
            CHECK(ok, "RAII mask generation OK");
        }
        CHECK(true, "RAII no crash after scope exit");
    }

    std::cout<<"  Checks: "<<passed<<"/"<<checks<<std::endl;
    return (passed==checks)?0:1;
}
