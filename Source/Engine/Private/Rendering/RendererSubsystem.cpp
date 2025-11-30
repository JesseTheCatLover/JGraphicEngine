// Copyright 2025 JesseTheCatLover. All Rights Reserved.

#include "RendererSubsystem.h"

#include "IRenderBackend.h"
#include "RRenderProxies.h"
#include "Core/EngineGlobals.h"
#include "Framework/PostProcessManager.h"
#include <algorithm>
#include <iostream>
#include "Core/EngineContext.h"
#include "Scene/SceneComponents/JCameraComponent.h"

RendererSubsystem::RendererSubsystem(IRenderBackend *backend, EngineContext& ctx):
m_Context(ctx),
m_Backend(backend)
{
    BuildDefaultShader();
    m_CoordAdaptor = BuildCoordAdapter(m_Backend->GetCoordConvention());
}

void RendererSubsystem::BeginScene()
{
    if (!m_PPM)
    {
        std::cerr << "[RendererSubsystem] PostProcessManager is null, cannot render" << std::endl;
        return;
    }

    m_CommandBuffer.Clear();
    m_GPUStateCache = {};

    // Determine desired size from surface/window
    int fbW = m_Context.GetFramebufferWidth(), fbH = m_Context.GetFramebufferHeight();

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

void RendererSubsystem::EndScene()
{
    auto camera = m_Context.GetCamera();
    if (!camera)
    {
        std::cerr << "[RendererSubsystem]: EndScene called with null camera" << std::endl;
        return;
    }
    m_ViewMat = camera->GetViewMatrix(m_Context.GetAspectRatio());
    m_ProjMat = camera->GetProjectionMatrix(m_Context.GetAspectRatio());
    RCommandQueue::ComputeDepthBucketsFor(m_CommandBuffer.opaque, m_ViewMat, camera->GetNearPlane(), camera->GetFarPlane());
    RCommandQueue::ComputeDepthBucketsFor(m_CommandBuffer.alpha,  m_ViewMat, camera->GetNearPlane(), camera->GetFarPlane());

    m_CommandBuffer.SortAllQueues();
    FlushCommandBuffer();

    // Resolve MSAA to m_Scene if needed
    if (m_SceneMSAA.fbo.IsValid()) {
        m_Backend->ResolveFramebuffer(
            m_SceneMSAA.fbo, m_Scene.fbo,
            IRenderBackend::EResolveMask::Color,
            IRenderBackend::EResolveFilter::Nearest); // TODO: for future: when scaling during blit, use Linear for color masks; keep Nearest when depth is involved.
    }

    int fbW = m_Context.GetFramebufferWidth(), fbH = m_Context.GetFramebufferHeight();

    m_Backend->UnbindFramebuffer();
    m_Backend->SetViewport(0, 0, fbW, fbH);

    RunPostProcessChain(m_Scene.color, fbW, fbH);

    // DO NOT swap buffers here; Engine loop does it after UI
    m_Backend->EndFrame();
}

void RendererSubsystem::Shutdown()
{
    m_Backend->Shutdown();
}

void RendererSubsystem::FlushCommandBuffer()
{
    if (!m_CommandBuffer.GetLights().empty())
        m_Backend->UploadLights(m_CommandBuffer.GetLights().data(),
            static_cast<uint32_t>(m_CommandBuffer.GetLights().size()));

    // Local lambda for state changes
    auto setLayerState = [&](ERenderLayer layer) {
        switch (layer)
        {
            case ERenderLayer::Opaque:
                m_Backend->SetCullMode(IRenderBackend::ECullMode::Back);
                m_Backend->SetDepthState(true, true, IRenderBackend::ECompareFunc::LessEqual);
                m_Backend->SetBlendState(false, IRenderBackend::EBlendFactor::One, IRenderBackend::EBlendFactor::Zero);
                break;

            case ERenderLayer::Alpha:
                m_Backend->SetCullMode(IRenderBackend::ECullMode::Back);
                m_Backend->SetDepthState(true, false, IRenderBackend::ECompareFunc::LessEqual);
                m_Backend->SetBlendState(true, IRenderBackend::EBlendFactor::SrcAlpha,
                                         IRenderBackend::EBlendFactor::OneMinusSrcAlpha);
                break;

            case ERenderLayer::Overlay:
                m_Backend->SetCullMode(IRenderBackend::ECullMode::None);
                m_Backend->SetDepthState(false, false, IRenderBackend::ECompareFunc::Always);
                m_Backend->SetBlendState(true, IRenderBackend::EBlendFactor::SrcAlpha,
                                         IRenderBackend::EBlendFactor::OneMinusSrcAlpha);
                break;
        }
    };

    auto drawList = [&](const std::vector<RDrawCommand> &cmds, ERenderLayer L) {
        if (cmds.empty()) return;
        setLayerState(L);

        for (const auto &c: cmds)
        {
            // choose shader handle (use model shader if valid, otherwise fallback)
            RShaderHandle shaderToUse = c.state.shader.IsValid() ? c.state.shader : m_DefaultShader;
            if (!shaderToUse.IsValid())
            {
                BuildDefaultShader();
                shaderToUse = m_DefaultShader;
            }

            // only re-bind when shader actually changed
            if (shaderToUse.id != m_GPUStateCache.shader.id)
            {
                m_GPUStateCache.shader = shaderToUse; // cache the actual shader handle
                m_Backend->BindShader(shaderToUse);

                // upload camera matrices to the shader we just bound
                ApplyCamera(shaderToUse, m_ViewMat, m_ProjMat);

                // Set light count once per shader bind
                const int lightCount = (int) std::min<size_t>(m_CommandBuffer.GetLights().size(),
                                                              IRenderBackend::kMaxLights);
                m_Backend->SetUniformInt(shaderToUse, "u_LightCount", lightCount);
            }

            // material cache update (use shaderToUse for uniform calls)
            if (c.state.material != m_GPUStateCache.material)
            {
                m_GPUStateCache.material = c.state.material;

                if (const FSurfaceDesc *surf = GetMaterialSurface(c.state.material))
                {
                    int unit = 0;

                    if (surf->baseColor.IsValid())
                    {
                        m_Backend->BindTexture(surf->baseColor, unit);
                        m_Backend->SetUniformInt(shaderToUse, "u_BaseColor", unit);
                        m_Backend->SetUniformInt(shaderToUse, "u_UseBaseColorMap", 1);
                        ++unit;
                    }
                    else
                    {
                        m_Backend->SetUniformVec4(shaderToUse, "u_BaseColorFactor", surf->params.baseColorFactor);
                        m_Backend->SetUniformInt(shaderToUse, "u_UseBaseColorMap", 0);
                    }

                    if (surf->normal.IsValid())
                    {
                        m_Backend->BindTexture(surf->normal, unit);
                        m_Backend->SetUniformInt(shaderToUse, "u_NormalMap", unit);
                        ++unit;
                    }

                    if (surf->metallicRoughness.IsValid())
                    {
                        m_Backend->BindTexture(surf->metallicRoughness, unit);
                        m_Backend->SetUniformInt(shaderToUse, "u_MetalRoughMap", unit);
                        ++unit;
                        m_Backend->SetUniformFloat(shaderToUse, "u_MetallicFactor", surf->params.metallicFactor);
                        m_Backend->SetUniformFloat(shaderToUse, "u_RoughnessFactor", surf->params.roughnessFactor);
                    }

                    if (surf->emissive.IsValid())
                    {
                        m_Backend->BindTexture(surf->emissive, unit);
                        m_Backend->SetUniformInt(shaderToUse, "u_EmissiveMap", unit);
                        ++unit;
                    }

                    m_Backend->SetUniformVec2(shaderToUse, "u_UVTiling", surf->params.uvTiling);
                }
            }

            DrawMesh(c.state.mesh, shaderToUse, c.transform);
        }
    };

    drawList(m_CommandBuffer.opaque.GetDrawCommands(), ERenderLayer::Opaque);
    drawList(m_CommandBuffer.alpha.GetDrawCommands(), ERenderLayer::Alpha);
    drawList(m_CommandBuffer.overlay.GetDrawCommands(), ERenderLayer::Overlay);
}

void RendererSubsystem::BuildDefaultShader()
{
    if (m_DefaultShader.IsValid())
        return;

    // Attributes:
    //   location=0: position (vec3)
    //   location=1: normal   (vec3)
    //   location=2: uv       (vec2)
    //
    // Uniforms expected from engine:
    //   u_Model, u_View, u_Proj        : mat4
    //   u_BaseColor                    : sampler2D (optional)
    //   u_BaseColorFactor              : vec4      (used if no texture)
    //   u_UseBaseColorMap              : int       (0/1)
    //
    // Notes:
    //  - Hemisphere lighting avoids needing a light buffer/UBO for the "first pixels".
    //  - Normal transform uses a standard normal matrix from u_Model.
    const char* kVertex = R"(#version 330 core
    layout (location=0) in vec3 aPos;
    layout (location=1) in vec3 aNormal;
    layout (location=2) in vec2 aUV;

    uniform mat4 u_Model;
    uniform mat4 u_View;
    uniform mat4 u_Proj;

    out vec3 vWorldNormal;
    out vec2 vUV;

    void main()
    {
        // normal matrix = transpose(inverse(mat3(u_Model)))
        mat3 N = transpose(inverse(mat3(u_Model)));
        vWorldNormal = normalize(N * aNormal);
        vUV = aUV;

        gl_Position = u_Proj * u_View * u_Model * vec4(aPos, 1.0);
    }
)";

    const char* kFragment = R"(#version 330 core
    in vec3 vWorldNormal;
    in vec2 vUV;
    out vec4 FragColor;

    uniform sampler2D u_BaseColor;        // only valid if u_UseBaseColorMap == 1
    uniform vec4      u_BaseColorFactor;  // used when no texture
    uniform int       u_UseBaseColorMap;  // 0 or 1

    // Simple hemisphere lighting so there's always something visible.
    // You can drive these as uniforms later if you want.
    const vec3 SKY_COLOR    = vec3(0.6, 0.7, 0.9);
    const vec3 GROUND_COLOR = vec3(0.3, 0.25, 0.2);
    const float AMBIENT     = 0.15;

    void main()
    {
        vec4 baseColorTex = texture(u_BaseColor, vUV);
        vec4 baseColor    = (u_UseBaseColorMap == 1) ? baseColorTex : u_BaseColorFactor;

        // Hemisphere term (y-up)
        float hemiT = clamp(vWorldNormal.y * 0.5 + 0.5, 0.0, 1.0);
        vec3 hemi   = mix(GROUND_COLOR, SKY_COLOR, hemiT);

        vec3 lit = baseColor.rgb * (AMBIENT + hemi);
        FragColor = vec4(lit, baseColor.a);
    }
)";

    RShader sh{};
    sh.vertexSource = kVertex;
    sh.fragmentSource = kFragment;
    m_DefaultShader = m_Backend->CreateShader(sh);
}

void RendererSubsystem::EnsureTargets(int w, int h, int samples)
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

void RendererSubsystem::DestroyTarget(FTarget &t)
{
    if (t.fbo.IsValid()) m_Backend->DestroyFramebuffer(t.fbo);
    t = {}; // backend should have invalidated the exported handles
}

void RendererSubsystem::BuildTarget(FTarget &t, int w, int h, int samples, bool withDepth, bool hdr, bool srgb)
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

void RendererSubsystem::RunPostProcessChain(RTextureHandle sceneColor, int w, int h)
{
    // Sync shaders with UI/gameplay changes
    RebuildKernelsIfDirty();

    // If no PPM or no enabled passes: copy to backbuffer w/ tonemap gamma (or straight copy if LDR)
    auto copyToBackbuffer = [&](){
        m_Backend->UnbindFramebuffer(); // Default FBO
        BlitFullscreen(m_CopyShader, sceneColor, w, h);
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
        BlitFullscreen(it->second.shader, current, dst.w, dst.h);

        m_Backend->UnbindFramebuffer();
        current = dst.color;
        usePing = !usePing;
    }

    // Present last
    m_Backend->UnbindFramebuffer();
    BlitFullscreen(m_CopyShader, current, w, h);
}

void RendererSubsystem::EnsureFullscreenQuad()
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

void RendererSubsystem::BlitFullscreen(RShaderHandle sh, RTextureHandle inputTex, int w, int h)
{
    EnsureFullscreenQuad();

    RShaderHandle shaderToUse = sh.IsValid() ? sh : m_CopyShader;
    m_Backend->SetViewport(0, 0, w, h);
    m_Backend->BindShader(shaderToUse);
    m_Backend->SetUniformInt(shaderToUse, "u_Input", 0);
    m_Backend->BindTexture(inputTex, 0);
    m_Backend->SubmitMesh(m_FSQuad, shaderToUse, FMatrix4::Identity());
}

void RendererSubsystem::RebuildKernelsIfDirty()
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
            EnsureFullscreenQuad();
            FPassKernel k{};
            k.shader = m_CopyShader;
            k.BindParams = nullptr;
            newKernels.emplace(pass.name, k);
        }
    }

    m_Kernels.swap(newKernels);
}

FCoordAdapter RendererSubsystem::BuildCoordAdapter(const FBackendCoordDesc& d)
{
    // d.X, d.Y, d.Z are backend basis expressed in ENGINE coordinates.

    // Build the 4x4 basis transform T: Engine -> Backend
    // Columns are images of backend's basis vectors in engine space,
    // but we want a matrix that transforms engine-space vectors into backend-space.
    //
    // Since d.X, d.Y, d.Z are backend axes in engine coordinates, we actually want
    // the matrix that maps engine basis -> backend basis.
    // That matrix is the inverse of the one with columns d.X, d.Y, d.Z.
    //
    // Let B = [d.X d.Y d.Z]. A vector in backend coords v_b = B * v_e (engine coords).
    // We want v_b = T * v_e, so T = B.

    FMatrix4 T = FMatrix4::Identity();

    T.SetBasisX(d.X); // backend X axis in engine coords
    T.SetBasisY(d.Y);
    T.SetBasisZ(d.Z);

    FCoordAdapter adapter;
    adapter.EngineToBackend = T;
    adapter.BackendToEngine = T.Inverse();
    return adapter;
}

void RendererSubsystem::ApplyCamera(const RShaderHandle& shaderToUse, const FMatrix4& viewEngine, const FMatrix4& projEngine)
{
    // Convert from engine space to backend space
    FMatrix4 viewBackend = m_CoordAdaptor.EngineToBackend * viewEngine * m_CoordAdaptor.BackendToEngine;
    FMatrix4 projBackend = projEngine;

    float viewRaw[16];
    float projRaw[16];

    viewBackend.ToFloatArray(viewRaw);
    projBackend.ToFloatArray(projRaw);

    m_Backend->SetUniformMat4(shaderToUse, "u_View", viewRaw);
    m_Backend->SetUniformMat4(shaderToUse, "u_Proj", projRaw);
}

void RendererSubsystem::DrawMesh(const RMeshHandle& meshHandle, const RShaderHandle &shaderToUse, const FMatrix4 &modelEngine)
{
    FMatrix4 modelBackend = m_CoordAdaptor.EngineToBackend * modelEngine * m_CoordAdaptor.BackendToEngine;
    float modelRaw[16];
    modelBackend.ToFloatArray(modelRaw);
    m_Backend->SetUniformMat4(shaderToUse, "u_Model", modelRaw);

    m_Backend->SubmitMesh(meshHandle, shaderToUse, modelBackend);
}

RMeshHandle RendererSubsystem::CreateMesh(const RMesh &data)
{
    return m_Backend->CreateMesh(data);
}

void RendererSubsystem::DestroyMesh(RMeshHandle h)
{
    m_Backend->DestroyMesh(h);
}

RTextureHandle RendererSubsystem::CreateTexture(const RTexture &data)
{
    return m_Backend->CreateTexture(data);
}

void RendererSubsystem::DestroyTexture(RTextureHandle h)
{
    m_Backend->DestroyTexture(h);
}

RShaderHandle RendererSubsystem::CreateShader(const RShader &data)
{
    return m_Backend->CreateShader(data);
}

void RendererSubsystem::DestroyShader(RShaderHandle h)
{
    m_Backend->DestroyShader(h);
}

RMaterialHandle RendererSubsystem::CreateMaterial(const FSurfaceDesc &surface)
{
    // Generate a new handle id
    const Rint id = m_NextMaterialId++;
    m_Materials.emplace(id, FMaterialEntry{ surface });
    return RMaterialHandle{ id };
}

void RendererSubsystem::DestroyMaterial(RMaterialHandle h)
{
    if (!h.IsValid()) return;
    m_Materials.erase(h.id);
    if (m_GPUStateCache.material == h)
        {
        m_GPUStateCache.material = RMaterialHandle::Invalid();
    }
}

const FSurfaceDesc* RendererSubsystem::GetMaterialSurface(RMaterialHandle h) const
{
    if (!h.IsValid()) return nullptr;
    auto it = m_Materials.find(h.id);
    return (it == m_Materials.end()) ? nullptr : &it->second.surface;
}

void RendererSubsystem::EnqueueRenderTask(std::function<void()> fn)
{
    // TODO: later, push into a lock-free queue drained in BeginFrame/EndFrame
    fn();
}