// Copyright 2025 JesseTheCatLover. All Rights Reserved.

#pragma once
#include <unordered_map>
#include <vector>

#include "FSurfaceDesc.h"
#include "IRenderDevice.h"
#include "RHandles.h"
#include "RCommandBuffer.h"

struct FBackendCoordDesc;

struct FCoordAdapter
{
    FMatrix4 EngineToBackend;
    FMatrix4 BackendToEngine;
};

struct FPassParam;
class PostProcessManager;
struct RRenderProxy;
class IRenderBackend;

class RendererSubsystem : public IRenderDevice
{
    friend class JEngine;

private:
    explicit RendererSubsystem(IRenderBackend* backend);

    void BeginScene();
    void EndScene();
    void Shutdown();

    IRenderBackend* m_Backend = nullptr;
    PostProcessManager* m_PPM = nullptr;

    RCommandBuffer m_CommandBuffer;
    void FlushCommandBuffer();

    FGPUStateCache m_GPUStateCache;

    RShaderHandle m_DefaultShader{};
    void BuildDefaultShader();

    FCoordAdapter m_CoordAdaptor;
    FMatrix4 m_ViewMat;
    FMatrix4 m_ProjMat;

private:
    struct FMaterialEntry
    {
        FSurfaceDesc surface;
    };

    std::unordered_map<Rint, FMaterialEntry> m_Materials{};
    Rint m_NextMaterialId = 1;

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

    FCoordAdapter BuildCoordAdapter(const FBackendCoordDesc& d);

    void ApplyCamera(const RShaderHandle& shaderToUse, const FMatrix4& viewEngine, const FMatrix4& projEngine);
    void DrawMesh(const RMeshHandle& meshHandle, const RShaderHandle& shaderToUse, const FMatrix4& modelEngine);

public:
    ~RendererSubsystem() = default;

    void SetPostProcessManager(PostProcessManager* ppm) { m_PPM = ppm;}

    IRenderSubmission& GetSubmission() override { return m_CommandBuffer; }

    RMeshHandle CreateMesh(const RMesh &data) override;
    void DestroyMesh(RMeshHandle h) override;

    RTextureHandle CreateTexture(const RTexture &data) override;
    void DestroyTexture(RTextureHandle h) override;

    RShaderHandle CreateShader(const RShader &data) override;
    void DestroyShader(RShaderHandle h) override;

    RMaterialHandle CreateMaterial(const FSurfaceDesc &surface) override;
    void DestroyMaterial(RMaterialHandle h) override;

    void SetDefaultShader(RShaderHandle h) { m_DefaultShader = h; }
    [[nodiscard]] RShaderHandle GetDefaultShader() const { return m_DefaultShader; }

    // Allow draw path to query surface by handle
    const FSurfaceDesc* GetMaterialSurface(RMaterialHandle h) const;

    void EnqueueRenderTask(std::function<void()> fn) override;
};
