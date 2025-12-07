// Copyright 2025 JesseTheCatLover. All Rights Reserved.

#include "RendererSubsystem.h"

#include "IRenderBackend.h"
#include "RRenderProxies.h"
#include "Core/EngineGlobals.h"
#include "Framework/PostProcessManager.h"
#include <algorithm>
#include <iostream>
#include "Core/EngineContext.h"
#include "Rendering/FRenderView.h"
#include "Scene/JScene.h"
#include "Scene/SceneComponents/JCameraComponent.h"

RendererSubsystem::RendererSubsystem(IRenderBackend *backend, EngineContext& ctx):
m_Context(ctx),
m_Backend(backend)
{
    BuildDefaultShader();x
    m_CoordAdaptor = BuildCoordAdapter(m_Backend->GetCoordConvention());
}

void RendererSubsystem::RenderFrame(const std::vector<FRenderView> &views) // TODO: Maybe for future make the renderer do shared objects supporting between views for optimization
{
    m_Backend->BeginFrame();

    // Even if there are no views, it's nice to at least clear the platform surface
    if (views.empty())
    {
        // Fallback: clear default framebuffer to something neutral
        m_Backend->BindFramebuffer(RFramebufferHandle{});
        m_Backend->SetViewport(0, 0,
                               m_Context.GetFramebufferWidth(),
                               m_Context.GetFramebufferHeight());
        m_Backend->ClearColorDepth(0.2f, 0.3f, 0.3f, 1.f, true);
        m_Backend->EndFrame();
        return;
    }

    for (const FRenderView& view : views)
    {
        if (!view.camera || view.viewportW <= 0 || view.viewportH <= 0)
            continue;

        // Reset GPU state cache per view so state doesn't leak between views
        m_GPUStateCache = {};

        // 1) Build per-view targets ( Size + Samples per view)
        EnsureTargets(view.viewportW, view.viewportH, view.sampleCount);

        FTarget& sceneRT = (view.sampleCount > 1 && m_SceneMSAA.fbo.IsValid()) ? m_SceneMSAA : m_Scene;

        m_Backend->BindFramebuffer(sceneRT.fbo);
        m_Backend->SetViewport(0, 0, sceneRT.w, sceneRT.h);

        // 2) Clear Color/Depth if requested
        if (view.bClearColor)
        {
            m_Backend->ClearColorDepth(
                view.clearColorValue.x,
                view.clearColorValue.y,
                view.clearColorValue.z,
                view.clearColorValue.w,
                view.bClearDepth);
        }
        else
        {
            m_Backend->ClearDepthOnly(view.bClearDepth);
        }

        // 3) Build camera matrices
        const float aspect = (sceneRT.h > 0) ? static_cast<float>(sceneRT.w) / static_cast<float>(sceneRT.h)
        : 1.f;

        FMatrix4 viewMat = view.camera->GetViewMatrix();
        FMatrix4 projMat = view.camera->GetProjectionMatrix(aspect);

        // 4) Gather renderables for this view into a local command buffer
        RCommandBuffer cmd;

        if (view.scene)
        {
            FRenderContext ctx{};

            FViewParams viewParams{};
            viewParams.viewType = view.viewType;
            viewParams.viewIndex = view.viewIndex;
            viewParams.camera = view.camera;
            viewParams.viewMatrix = viewMat;
            viewParams.projMatrix = projMat;
            viewParams.nearPlane = view.camera->GetNearPlane();
            viewParams.farPlane = view.camera->GetFarPlane();
            viewParams.renderMask = view.renderMask;

            ctx.view = &viewParams;

            // Scene populates proxies into cmd buffer
            view.scene->GatherRenderables(cmd, ctx);
        }

        // 5) Depth buckets + sorting
        if (view.scene)
        {
            const float nearPlane = view.camera->GetNearPlane();
            const float farPlane = view.camera->GetFarPlane();

            RCommandQueue::ComputeDepthBucketsFor(
                cmd.opaque, viewMat, nearPlane, farPlane);
            RCommandQueue::ComputeDepthBucketsFor(
                cmd.alpha,  viewMat, nearPlane, farPlane);

            cmd.SortAllQueues(); // We do this after depth computation
        }

        // 6) Draw this view into sceneRenderTarget
        DrawCommandBuffer(cmd, viewMat, projMat);

        // 7) Resolve MSAA if needed
        if (view.sampleCount > 1 && m_SceneMSAA.fbo.IsValid())
        {
            m_Backend->ResolveFramebuffer(
                m_SceneMSAA.fbo, m_Scene.fbo,
                IRenderBackend::EResolveMask::Color,
                IRenderBackend::EResolveFilter::Nearest); // TODO: for future: when scaling during blit, use Linear for color masks; keep Nearest when depth is involved.
        }

        RTextureHandle finalColor = m_Scene.color;

        // 8) Apply post process chain if enabled
        if (view.bEnablePostProcess)
        {
            finalColor = RunPostProcessChain(finalColor, sceneRT.w, sceneRT.h, view.postProfileId);
        }

        // 9) Present this view to its target
        // Editor vs game is just: does targetFBO exist or not?
        if (view.targetFBO.IsValid()) // Editor
        {
            m_Backend->BindFramebuffer(view.targetFBO);
        }
        else // Game
        {
            m_Backend->UnbindFramebuffer();  // If it doesn't exist, it's a back-buffer
        }

        m_Backend->SetViewport(view.viewportX, view.viewportY, view.viewportW, view.viewportH);

        // Uses PresentShader by default (tone+gamma)
        BlitFullscreen(m_PresentShader, finalColor, view.viewportW, view.viewportH);
    }

    // Restore default framebuffer + viewport for UI/editor
    m_Backend->BindFramebuffer(RFramebufferHandle{});
    m_Backend->SetViewport(0, 0,
                           m_Context.GetFramebufferWidth(),
                           m_Context.GetFramebufferHeight());

    m_Backend->EndFrame();
}

void RendererSubsystem::Shutdown()
{
    m_Backend->Shutdown();
}

void RendererSubsystem::DrawCommandBuffer(RCommandBuffer& buffer, const FMatrix4& viewMat, const FMatrix4& projMat)
{
    // Upload lights for this buffer
    if (!buffer.GetLights().empty())
    {
        m_Backend->UploadLights(
            buffer.GetLights().data(),
            static_cast<uint32_t>(buffer.GetLights().size())
        );
    }

    // Local lambda for state changes
    auto setLayerState = [&](ERenderLayer layer)
    {
        switch (layer)
        {
            case ERenderLayer::Opaque:
                m_Backend->SetCullMode(IRenderBackend::ECullMode::Back);
                m_Backend->SetDepthState(true, true, IRenderBackend::ECompareFunc::LessEqual);
                m_Backend->SetBlendState(false,
                    IRenderBackend::EBlendFactor::One,
                    IRenderBackend::EBlendFactor::Zero);
                break;

            case ERenderLayer::Alpha:
                m_Backend->SetCullMode(IRenderBackend::ECullMode::Back);
                m_Backend->SetDepthState(true, false, IRenderBackend::ECompareFunc::LessEqual);
                m_Backend->SetBlendState(true,
                    IRenderBackend::EBlendFactor::SrcAlpha,
                    IRenderBackend::EBlendFactor::OneMinusSrcAlpha);
                break;

            case ERenderLayer::Overlay:
                m_Backend->SetCullMode(IRenderBackend::ECullMode::None);
                m_Backend->SetDepthState(false, false, IRenderBackend::ECompareFunc::Always);
                m_Backend->SetBlendState(true,
                    IRenderBackend::EBlendFactor::SrcAlpha,
                    IRenderBackend::EBlendFactor::OneMinusSrcAlpha);
                break;
        }
    };

    auto drawList = [&](const std::vector<RDrawCommand>& cmds, ERenderLayer L)
    {
        if (cmds.empty()) return;
        setLayerState(L);

        for (const auto& c : cmds)
        {
            // Choose shader handle (use model shader if valid, otherwise fallback)
            RShaderHandle shaderToUse = c.state.shader.IsValid()
                ? c.state.shader
                : m_DefaultShader;

            if (!shaderToUse.IsValid())
            {
                BuildDefaultShader();
                shaderToUse = m_DefaultShader;
            }

            // Only re-bind when shader actually changed
            if (shaderToUse.id != m_GPUStateCache.shader.id)
            {
                m_GPUStateCache.shader = shaderToUse;
                m_Backend->BindShader(shaderToUse);

                // Upload camera matrices to the shader we just bound
                ApplyCamera(shaderToUse, viewMat, projMat);

                // Set light count once per shader bind
                const int lightCount =
                    (int)std::min<size_t>(buffer.GetLights().size(),
                                          IRenderBackend::kMaxLights);
                m_Backend->SetUniformInt(shaderToUse, "u_LightCount", lightCount);
            }

            // Material cache update
            if (c.state.material != m_GPUStateCache.material)
            {
                m_GPUStateCache.material = c.state.material;

                if (const FSurfaceDesc* surf = GetMaterialSurface(c.state.material))
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
                        m_Backend->SetUniformVec4(shaderToUse,
                            "u_BaseColorFactor", surf->params.baseColorFactor);
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

                        m_Backend->SetUniformFloat(shaderToUse,
                            "u_MetallicFactor",  surf->params.metallicFactor);
                        m_Backend->SetUniformFloat(shaderToUse,
                            "u_RoughnessFactor", surf->params.roughnessFactor);
                    }

                    if (surf->emissive.IsValid())
                    {
                        m_Backend->BindTexture(surf->emissive, unit);
                        m_Backend->SetUniformInt(shaderToUse, "u_EmissiveMap", unit);
                        ++unit;
                    }

                    m_Backend->SetUniformVec2(shaderToUse,
                        "u_UVTiling", surf->params.uvTiling);
                }
            }

            DrawMesh(c.state.mesh, shaderToUse, c.transform);
        }
    };

    // Now draw for each layer
    drawList(buffer.opaque.GetDrawCommands(),   ERenderLayer::Opaque);
    drawList(buffer.alpha.GetDrawCommands(),    ERenderLayer::Alpha);
    drawList(buffer.overlay.GetDrawCommands(),  ERenderLayer::Overlay);
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
    const vec3 SKY_COLOR    = vec3(0.6, 0.7, 0.9);
    const vec3 GROUND_COLOR = vec3(0.3, 0.25, 0.2);
    const float AMBIENT     = 0.15;

    void main()
    {
        vec4 baseColorTex = texture(u_BaseColor, vUV);
        vec4 baseColor    = (u_UseBaseColorMap == 1) ? baseColorTex : u_BaseColorFactor;

        // Hemisphere term (y-up)
        // float hemiT = clamp(vWorldNormal.y * 0.5 + 0.5, 0.0, 1.0);
        // vec3 hemi   = mix(GROUND_COLOR, SKY_COLOR, hemiT);

        // vec3 lit = baseColor.rgb * (AMBIENT + hemi);
        // FragColor = vec4(lit, baseColor.a);

        // DEBUG: UNLIT, NO LIGHTING
        FragColor = baseColor;
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

RTextureHandle RendererSubsystem::RunPostProcessChain(RTextureHandle sceneColor, int w, int h, uint32_t profileId)
{
    // If there's no PPM or no active passes, just return the input scene color
    if (!m_PPM)
    {
        return sceneColor;
    }

    // Sync shaders with UI/gameplay changes
    RebuildKernelsIfDirty(profileId);

    const auto& chain = m_PPM->GetChain(profileId);

    bool bAnyEnabled = false;
    for (auto& p : chain)
        if (p.bEnabled)
            { bAnyEnabled = true; break; }

    if (!bAnyEnabled)
    {
        return sceneColor;
    }

    // Ping-pong
    bool bUsePing = true;
    RTextureHandle current = sceneColor;

    for (const auto& pass : chain)
    {
        if (!pass.bEnabled) continue;

        auto it = m_Kernels.find(pass.name);
        if (it == m_Kernels.end()) continue; // unknown pass; skip

        FTarget& dst = bUsePing ? m_Ping : m_Pong;
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
        bUsePing = !bUsePing;
    }

    m_Backend->UnbindFramebuffer();

    // Final texture
    return current;
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

    // Linear copy shader (no tone/gamma)
    if (!m_LinearCopyShader.IsValid())
    {
        RShader s{};
        s.vertexSource = R"(#version 330 core
            layout(location=0) in vec3 aPos;
            layout(location=2) in vec2 aUV;
            out vec2 vUV;
            void main()
            {
                vUV = aUV;
                gl_Position = vec4(aPos, 1.0);
            }
        )";

        s.fragmentSource = R"(#version 330 core
            in vec2 vUV;
            out vec4 FragColor;
            uniform sampler2D u_Input;

            void main()
            {
                // Pure linear copy, no tone/gamma
                FragColor = texture(u_Input, vUV);
            }
        )";

        m_LinearCopyShader = m_Backend->CreateShader(s);
    }

    // Present shader: tone map + gamma
    if (!m_PresentShader.IsValid())
    {
        RShader s{};
        s.vertexSource = R"(#version 330 core
            layout(location=0) in vec3 aPos;
            layout(location=2) in vec2 aUV;
            out vec2 vUV;
            void main()
            {
                vUV = aUV;
                gl_Position = vec4(aPos, 1.0);
            }
        )";

        s.fragmentSource = R"(
            #version 330 core
            in vec2 vUV;
            out vec4 FragColor;

            uniform sampler2D u_Input;

            void main()
            {
                vec3 color = texture(u_Input, vUV).rgb;

                // No tone map, just clamp for safety
                color = clamp(color, 0.0, 1.0);

                // Single gamma encode to sRGB
                color = pow(color, vec3(1.0 / 2.2));

                FragColor = vec4(color, 1.0);
            }
        )";

        m_PresentShader = m_Backend->CreateShader(s);
    }
}

void RendererSubsystem::BlitFullscreen(RShaderHandle sh, RTextureHandle inputTex, int w, int h)
{
    EnsureFullscreenQuad();

    RShaderHandle shaderToUse = sh; // must be valid
    if (!shaderToUse.IsValid())
        shaderToUse = m_LinearCopyShader; // safe fallback

    m_Backend->SetViewport(0, 0, w, h);
    m_Backend->BindShader(shaderToUse);
    m_Backend->SetUniformInt(shaderToUse, "u_Input", 0);
    m_Backend->BindTexture(inputTex, 0);
    m_Backend->SubmitMesh(m_FSQuad, shaderToUse, FMatrix4::Identity());
}

void RendererSubsystem::RebuildKernelsIfDirty(uint32_t profileId)
{
    if (!m_PPM) return;
    if (!m_PPM->IsDirtyAndClear(profileId)) return;

    std::unordered_map<std::string, FPassKernel> newKernels;

    for (const auto& pass : m_PPM->GetChain(profileId))
    {
        if (auto it = m_Kernels.find(pass.name); it != m_Kernels.end())
        {
            newKernels.emplace(pass.name, it->second); // Keep compiled kernel
        }
        else
        {
            EnsureFullscreenQuad();

            FPassKernel k{};
            k.shader = m_LinearCopyShader; // linear copy, NO gamma
            k.BindParams = nullptr; // can be specialized later

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

void* RendererSubsystem::GetNativeTextureHandle(RTextureHandle handle) const
{
    return m_Backend->GetNativeTextureHandle(handle);
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
    // TODO: later, push into a lock-free queue
    fn();
}