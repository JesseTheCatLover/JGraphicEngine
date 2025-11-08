// Copyright 2025 JesseTheCatLover. All Rights Reserved.

#include "JRenderer.h"

#include "IRenderBackend.h"
#include "RRenderProxies.h"
#include "Core/EngineGlobals.h"
#include "Framework/PostProcessManager.h"
#include <algorithm>
#include <iostream>

// Helpers to read/replace the 16-bit depth inside 'packet'
static inline uint16_t ExtractDepth(uint64_t p) { return uint16_t((p >> 40) & 0xFFFF); }
static inline uint64_t ReplaceDepth(uint64_t p, uint16_t d)
{
    const uint64_t clear = ~(0xFFFFULL << 40);
    return (p & clear) | (uint64_t(d) << 40);
}

// Map view-space Z to a 16-bit bucket (tweak sign to your view convention)
static inline uint16_t DepthToBucket(float viewZ, float nearZ, float farZ)
{
    float z = (viewZ - nearZ) / (farZ - nearZ);   // normalize 0..1
    z = std::clamp(z, 0.0f, 1.0f);
    return uint16_t(z * 65535.0f + 0.5f);
}

// Compute buckets for any queue that needs depth ordering
static void ComputeDepthBucketsFor(RRenderQueue& q, const FMatrix4& view, float nearZ, float farZ)
{
    auto& commands = q.GetDrawCommands();
    for (auto& c : commands)
    {
        if (ExtractDepth(c.packet) != 0) continue; // respect pre-filled depth

        const FVector3 worldP = c.transform.GetTranslation();
        const FVector3 viewP  = view.TransformPoint(worldP); // (View * Model) * [0,0,0,1]
        const float zVS = viewP.z; // If -Z is forward, use -viewP.z

        c.packet = ReplaceDepth(c.packet, DepthToBucket(zVS, nearZ, farZ));
    }
}

void JRenderer::BeginScene()
{
    if (!m_PPM)
    {
        std::cerr << "[JRenderer] PostProcessManager is null, cannot render" << std::endl;
        return;
    }

    m_Route.Clear();
    m_GPUStateCache = {};

    // Determine desired size from surface/window
    int fbW = JEngine::Get().GetState().GetFramebufferWidth(), fbH = JEngine::Get().GetState().GetFramebufferHeight();

    int samples = 4; // TODO: Hardcoded for now; Expose a setting for samples
    EnsureTargets(fbW, fbH, samples);

    // Bind scene target for world rendering (MSAA if >1)
    if (m_SceneMSAA.fbo.IsValid()) m_Backend->BindFramebuffer(m_SceneMSAA.fbo);
    else m_Backend->BindFramebuffer(m_Scene.fbo);

    // Match viewport to the render target
    const FTarget& rt = m_SceneMSAA.fbo.IsValid() ? m_SceneMSAA : m_Scene;
    m_Backend->SetViewport(0, 0, rt.w, rt.h);

    // Clear
    m_Backend->BeginFrame();
    m_Backend->ClearColorDepth(0.2f, 0.3f, 0.3f, 1.f, true);
}

void JRenderer::EndScene()
{
    // Record proxies to the route
    RRenderContext ctx{};
    for (RRenderProxy* proxy : m_Proxies)
        if (proxy) proxy->RecordToRoute(m_Route, ctx);
    m_Proxies.clear();

    const FMatrix4 view = /* TODO: active camera view */ FMatrix4::Identity();
    const float nearZ = 0.1f, farZ = 1000.0f;

    ComputeDepthBucketsFor(m_Route.opaque, view, nearZ, farZ);
    ComputeDepthBucketsFor(m_Route.alpha,  view, nearZ, farZ);

    m_Route.SortAllQueues();

    // DRAW recorded commands into the scene FBO (already bound in BeginScene)
    DrawRenderQueues();

    // Resolve MSAA to m_Scene if needed
    if (m_SceneMSAA.fbo.IsValid()) {
        m_Backend->ResolveFramebuffer(
            m_SceneMSAA.fbo, m_Scene.fbo,
            IRenderBackend::EResolveMask::Color,
            IRenderBackend::EResolveFilter::Nearest); // TODO: for future: when scaling during blit, use Linear for color masks; keep Nearest when depth is involved.
    }

    int fbW = JEngine::Get().GetState().GetFramebufferWidth(), fbH = JEngine::Get().GetState().GetFramebufferHeight();

    m_Backend->UnbindFramebuffer();
    m_Backend->SetViewport(0, 0, fbW, fbH);

    RunPostChain(m_Scene.color, fbW, fbH);

    // DO NOT swap buffers here; Engine loop does it after UI
    m_Backend->EndFrame();
}

void JRenderer::Shutdown()
{
    m_Backend->Shutdown();
}

void JRenderer::DrawRenderQueues()
{
    if (!m_Route.GetLights().empty())
        m_Backend->UploadLights(m_Route.GetLights().data(), static_cast<uint32_t>(m_Route.GetLights().size()));

    // Local lambda for state changes
    auto setLayerState = [&](ERenderLayer layer){
        switch (layer)
        {
            case ERenderLayer::Opaque:
                m_Backend->SetCullMode(IRenderBackend::ECullMode::Back);
                m_Backend->SetDepthState(/*test*/true, /*write*/true, IRenderBackend::ECompareFunc::LessEqual);
                m_Backend->SetBlendState(false, IRenderBackend::EBlendFactor::One, IRenderBackend::EBlendFactor::Zero);
                break;

            case ERenderLayer::Alpha:
                m_Backend->SetCullMode(IRenderBackend::ECullMode::Back);
                m_Backend->SetDepthState(true, /*write*/false, IRenderBackend::ECompareFunc::LessEqual);
                m_Backend->SetBlendState(true, IRenderBackend::EBlendFactor::SrcAlpha, IRenderBackend::EBlendFactor::OneMinusSrcAlpha);
                break;

            case ERenderLayer::Overlay:
                m_Backend->SetCullMode(IRenderBackend::ECullMode::None);
                m_Backend->SetDepthState(false, false, IRenderBackend::ECompareFunc::Always);
                m_Backend->SetBlendState(true, IRenderBackend::EBlendFactor::SrcAlpha, IRenderBackend::EBlendFactor::OneMinusSrcAlpha);
                break;
        }
    };

    auto drawList = [&](const std::vector<RDrawCommand>& cmds, ERenderLayer L)
    {
        if (cmds.empty()) return;
        setLayerState(L);

        for (const auto& c : cmds)
        {
            if (c.state.shader.id != m_GPUStateCache.shader.id)
            {
                m_GPUStateCache.shader = c.state.shader;
                m_Backend->BindShader(m_GPUStateCache.shader);

                // Set light count once per shader bind (clamp to backend max)
                const int lightCount = (int)std::min<size_t>(m_Route.GetLights().size(), IRenderBackend::kMaxLights);
                m_Backend->SetUniformInt(m_GPUStateCache.shader, "u_LightCount", lightCount);
            }

            if (c.state.material.id) m_Backend->BindMaterial(c.state.material);

            m_Backend->SubmitMesh(c.state.mesh, m_GPUStateCache.shader, c.transform);
        }
    };

    drawList(m_Route.opaque.GetDrawCommands(), ERenderLayer::Opaque);
    drawList(m_Route.alpha.GetDrawCommands(), ERenderLayer::Alpha);
    drawList(m_Route.overlay.GetDrawCommands(), ERenderLayer::Overlay);
}

void JRenderer::EnsureTargets(int w, int h, int samples)
{
    // (Re)build on first run or size/sample change
    auto need = [&](const FTarget& t){ return t.w!=w || t.h!=h || t.samples!=samples || !t.fbo.IsValid(); };

    if (need(m_Scene))
    {
        DestroyTarget(m_Scene);
        BuildTarget(m_Scene, w, h, 1, /*withDepth*/true, /*hdr*/true, /*srgb*/false); // keep post chain in HDR
    }
    if (samples > 1 && need(m_SceneMSAA))
    {
        DestroyTarget(m_SceneMSAA);
        BuildTarget(m_SceneMSAA, w, h, samples, /*withDepth*/true, /*hdr*/true, /*srgb*/false);
    }
    else if (samples == 1 && m_SceneMSAA.fbo.IsValid())
    {
        DestroyTarget(m_SceneMSAA);
    }
    if (need(m_Ping))
    {
        DestroyTarget(m_Ping); DestroyTarget(m_Pong);
        BuildTarget(m_Ping, w, h, 1, false, true, false);
        BuildTarget(m_Pong, w, h, 1, false, true, false);
    }
}

void JRenderer::DestroyTarget(FTarget &t)
{
    if (t.fbo.IsValid()) m_Backend->DestroyFramebuffer(t.fbo);
    t = {}; // backend should have invalidated the exported handles
}

void JRenderer::BuildTarget(FTarget &t, int w, int h, int samples, bool withDepth, bool hdr, bool srgb)
{
    // Describe FBO you want; backend owns attachments
    RFramebuffer fb{};
    fb.width = w;
    fb.height = h;
    fb.samples = std::max(1, samples);
    fb.colorAsTexture = true; // we want to sample color
    fb.depthAsTexture = withDepth && samples==1;  // depth as tex only if single-sample
    fb.colorMode = hdr ? EColorMode::HDR16F : EColorMode::LDR8;
    fb.depthMode = withDepth ? EDepthMode::D24S8 : EDepthMode::None;

    t.fbo = m_Backend->CreateFramebuffer(fb);

    // Query the engine handles for attached textures (valid only if !MSAA)
    t.color = m_Backend->GetFramebufferColorTexture(t.fbo);
    t.depth = m_Backend->GetFramebufferDepthTexture(t.fbo);

    t.w = w; t.h = h; t.samples = fb.samples;
}

void JRenderer::RunPostChain(RTextureHandle sceneColor, int w, int h)
{
    // Sync shaders with UI/gameplay changes
    RebuildKernelsIfDirty();

    // If no PPM or no enabled passes: copy to backbuffer w/ tonemap gamma (or straight copy if LDR)
    auto copyToBackbuffer = [&](){
        m_Backend->UnbindFramebuffer(); // Default FBO
        DrawFullscreen(m_CopyShader, sceneColor, w, h);
    };

    if (!m_PPM) { copyToBackbuffer(); return; }

    const auto& chain = m_PPM->GetChain();
    bool anyEnabled = false;
    for (auto& p : chain) if (p.enabled) { anyEnabled = true; break; }
    if (!anyEnabled) { copyToBackbuffer(); return; }

    // Ping-pong
    bool usePing = true;
    RTextureHandle current = sceneColor;

    for (const auto& pass : chain)
    {
        if (!pass.enabled) continue;

        auto it = m_Kernels.find(pass.name);
        if (it == m_Kernels.end()) continue; // unknown pass; skip

        FTarget& dst = usePing ? m_Ping : m_Pong;
        m_Backend->BindFramebuffer(dst.fbo);

        // Set viewport
        m_Backend->SetViewport(0,0,dst.w,dst.h);

        // Bind shader + params and draw
        m_Backend->BindShader(it->second.shader);
        if (it->second.BindParams) it->second.BindParams(m_Backend, it->second.shader, pass.params);
        m_Backend->BindTexture(current, 0);
        DrawFullscreen(it->second.shader, current, dst.w, dst.h);

        m_Backend->UnbindFramebuffer();
        current = dst.color;
        usePing = !usePing;
    }

    // Present last
    m_Backend->UnbindFramebuffer();
    DrawFullscreen(m_CopyShader, current, w, h);
}

void JRenderer::EnsureFullscreenResources()
{
    if (!m_FSQuad.IsValid())
    {
        // Fullscreen triangle: (x,y,z,u,v)
        std::vector<float> verts = {
            //    x     y     z     u    v
            -1.f, -1.f, 0.f,  0.f,  0.f,
             3.f, -1.f, 0.f,  2.f,  0.f,
            -1.f,  3.f, 0.f,  0.f,  2.f
        };

        RMesh m{};
        m.vertices     = std::move(verts);
        m.indices.clear();
        m.vertexStride = sizeof(float) * 5; // 3 pos + 2 uv
        m.bHasNormals  = false;
        m.bHasUVs      = true;
        m.bHasTangents = false;
        m_FSQuad = m_Backend->CreateMesh(m);
    }

    if (!m_CopyShader.IsValid()) //TODO: Future work: Tone/Gamma policty: either do it in the final shader (keep sRGB off) or enable sRGB and keep shader linear
    {
        RShader s{};
        s.vertexSource = R"(#version 330 core
            layout(location=0) in vec3 aPos;
            layout(location=2) in vec2 aUV;
            out vec2 vUV;
            void main(){ vUV=aUV; gl_Position = vec4(aPos, 1.0); }
        )";
        s.fragmentSource = R"(#version 330 core
            in vec2 vUV;
            out vec4 FragColor;
            uniform sampler2D u_Input;
            void main(){ FragColor = texture(u_Input, vUV); }
        )";
        m_CopyShader = m_Backend->CreateShader(s);
    }
}

void JRenderer::DrawFullscreen(RShaderHandle sh, RTextureHandle inputTex, int w, int h)
{
    EnsureFullscreenResources();

    RShaderHandle shaderToUse = sh.IsValid() ? sh : m_CopyShader;
    m_Backend->SetViewport(0, 0, w, h);
    m_Backend->BindShader(shaderToUse);
    m_Backend->SetUniformInt(shaderToUse, "u_Input", 0);
    m_Backend->BindTexture(inputTex, 0);
    m_Backend->SubmitMesh(m_FSQuad, shaderToUse, FMatrix4::Identity());
}

void JRenderer::RebuildKernelsIfDirty()
{
    if (!m_PPM) return;
    if (!m_PPM->IsDirtyAndClear()) return;

    std::unordered_map<std::string, FPassKernel> newKernels;

    for (const auto& pass : m_PPM->GetChain())
    {
        if (auto it = m_Kernels.find(pass.name); it != m_Kernels.end())
        {
            newKernels.emplace(pass.name, it->second); // Keep compiled kernel
        }
        else
        {
            // Minimal bootstrap: unknown names fall back to copy shader
            EnsureFullscreenResources();
            FPassKernel k{};
            k.shader = m_CopyShader;
            k.BindParams = nullptr;
            newKernels.emplace(pass.name, k);
        }
    }

    m_Kernels.swap(newKernels);
}

RMeshHandle JRenderer::CreateMesh(const RMesh &data)
{
    return m_Backend->CreateMesh(data);
}

void JRenderer::DestroyMesh(RMeshHandle h)
{
    m_Backend->DestroyMesh(h);
}

RTextureHandle JRenderer::CreateTexture(const RTexture &data)
{
    return m_Backend->CreateTexture(data);
}

void JRenderer::DestroyTexture(RTextureHandle h)
{
    m_Backend->DestroyTexture(h);
}

RShaderHandle JRenderer::CreateShader(const RShader &data)
{
    m_Backend->CreateShader(data);
}

void JRenderer::DestroyShader(RShaderHandle h)
{
    m_Backend->DestroyShader(h);
}

void JRenderer::Enqueue(std::function<void()> fn)
{
    // TODO: later, push into a lock-free queue drained in BeginFrame/EndFrame
    fn();
}

void JRenderer::SubmitProxy(RRenderProxy *proxy)
{
    if (proxy)
    {
        m_Proxies.push_back(proxy);
    }
}
