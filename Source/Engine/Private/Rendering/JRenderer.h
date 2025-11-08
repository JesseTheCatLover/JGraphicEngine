// Copyright 2025 JesseTheCatLover. All Rights Reserved.

#pragma once
#include <unordered_map>
#include <vector>

#include "IRenderDevice.h"
#include "RHandles.h"
#include "RRenderRoute.h"

struct FPassParam;
class PostProcessManager;
struct RRenderProxy;
class IRenderBackend;

class JRenderer : public IRenderDevice
{
    friend class JEngine;

private:
    explicit JRenderer(IRenderBackend* backend) { m_Backend = backend; }

    void BeginScene();
    void EndScene();
    void Shutdown();

    IRenderBackend* m_Backend = nullptr;
    PostProcessManager* m_PPM = nullptr;

    std::vector<RRenderProxy*> m_Proxies; // Gathered each frame
    RRenderRoute m_Route;
    FGPUStateCache m_GPUStateCache;
    void DrawRenderQueues();

private:
    struct FTarget
    {
        RFramebufferHandle fbo{};
        RTextureHandle color{};
        RTextureHandle depth{};
        int w = 0, h = 0, samples = 1;
    };

    // Scene targets
    FTarget m_SceneMSAA{};
    FTarget m_Scene{};
    // Ping-Pong for post
    FTarget m_Ping{};
    FTarget m_Pong{};

    // Fullscreen quad/tri
    RMeshHandle   m_FSQuad{};
    RShaderHandle m_CopyShader{}; // Simple “texture copy” (or tonemap)

    // Pass kernels
    struct FPassKernel
    {
        RShaderHandle shader{};
        // function that sets uniforms from FPassParam
        void (*BindParams)(IRenderBackend*, RShaderHandle, const FPassParam&) = nullptr;
    };

    std::unordered_map<std::string, FPassKernel> m_Kernels;

    void EnsureTargets(int w, int h, int samples);
    void DestroyTarget(FTarget& t);
    void BuildTarget(FTarget& t, int w, int h, int samples, bool withDepth = false, bool hdr = false, bool srgb = false);
    void RunPostProcessChain(RTextureHandle sceneColor, int w, int h);
    void EnsureFullscreenQuad();
    void BlitFullscreen(RShaderHandle sh, RTextureHandle inputTex, int w, int h);
    void RebuildKernelsIfDirty();
    
public:
    ~JRenderer() = default;

    void SetPostProcessManager(PostProcessManager* ppm) { m_PPM = ppm;}

    RMeshHandle CreateMesh(const RMesh &data) override;
    void DestroyMesh(RMeshHandle h) override;

    RTextureHandle CreateTexture(const RTexture &data) override;
    void DestroyTexture(RTextureHandle h) override;

    RShaderHandle CreateShader(const RShader &data) override;
    void DestroyShader(RShaderHandle h) override;

    void EnqueueRenderTask(std::function<void()> fn) override;

    void SubmitProxy(RRenderProxy* proxy);
};
