//  Copyright 2025 JesseTheCatLover. All Rights Reserved.

#include "EditorRuntime.h"

#include <vector>
#include <unordered_map>
#include "Core/EngineContext.h"
#include "Core/EngineGlobals.h"
#include "Core/JEngine.h"
#include "Framework/PostProcessManager.h"
#include "Framework/SceneManager.h"
#include "Rendering/FRenderView.h"
#include "Rendering/IPlatformSurface.h"
#include "Rendering/RendererSubsystem.h"
#include "Tools/CameraEditorTool.h"

EditorRuntime::EditorRuntime()
    : m_Context(JEngine::Get().GetEngineContext())
    , m_SceneManager(*JEngine::Get().GetSceneManager())
    , m_Renderer(*JEngine::Get().GetRenderer())
    , m_PlatformSurface(*JEngine::Get().GetPlatformSurface())
    , m_SceneAPI(m_Context, m_SceneManager, *JEngine::Get().GetDebugDraw())
    , m_ViewportAPI(m_Context, m_Renderer)
    , m_SurfaceAPI(m_Context, m_PlatformSurface)
{
    // Editor takes over rendering, so don't render directly to platform surface
    m_Context.SetShouldRenderToPlatformSurface(false);

    auto& chain = GetPostProcessManager()->EditChain(kEditorPostProfile); // TODO: LEGACY
    chain.clear();

    FPostPassDesc outline{};
    outline.name = "Outline";
    outline.bEnabled = true;

    // Optional defaults (can be overridden per-view via frame params)
    // Edge/occlusion tuning
    outline.params.floats["u_DepthEpsilon"] = 0.0005f;
    outline.params.floats["u_Thickness"]    = 1.5f;   // try 1..3

    // Defaults
    outline.params.floats["u_Occlusion"] = 0.0f;  // 1=hide behind walls, 0=x-ray outlines
    outline.params.floats["u_FillAlpha"] = 0.08f; // subtle fill inside selection

    // Outline color (linear space)
    outline.params.floats["u_OutlineR"] = 1.0f;
    outline.params.floats["u_OutlineG"] = 0.65f;
    outline.params.floats["u_OutlineB"] = 0.10f;
    outline.params.floats["u_OutlineA"] = 1.0f;

    // outline.params.floats["u_OutlineR"] = 0.18f;
    // outline.params.floats["u_OutlineG"] = 0.28f;
    // outline.params.floats["u_OutlineB"] = 0.40f;
    // outline.params.floats["u_OutlineA"] = 1.0f;



    chain.push_back(std::move(outline));

    FPostPassDesc fxaa{};
    fxaa.name = "FXAA";
    fxaa.bEnabled = true;

    // Optional tuning (sane defaults)
    fxaa.params.floats["u_FXAA_ReduceMin"] = 1.0f / 128.0f;
    fxaa.params.floats["u_FXAA_ReduceMul"] = 1.0f / 8.0f;
    fxaa.params.floats["u_FXAA_SpanMax"]   = 4.0f;

    chain.push_back(fxaa);
}

EditorRuntime::~EditorRuntime()
{

}
//
// void EditorRuntime::TickAllTools(float deltaTime, const FEditorToolFrameState& state) // Legacy
// {
//     TickCameraTools(deltaTime, state.camera);
// }
//
// void EditorRuntime::TickCameraTools(float deltaTime, const FCameraToolState& state)
// {
//     m_CameraTools.ForEach(
//         [&](UDynamicID::IDType id, CameraEditorTool& tool)
//         {
//             const bool isActive = (id == state.activeCameraId);
//
//             float aspect = 16.f / 9.f;
//             if (auto itVS = state.viewstateMap.find(id); itVS != state.viewstateMap.end())
//             {
//                 aspect = itVS->second.aspect;
//             }
//
//             tool.Tick(deltaTime, isActive, aspect);
//         });
// }
//
// void EditorRuntime::SubmitEditorViewSources(const FCameraToolState& state)
// {
//     auto* scene = m_SceneManager.GetActiveScene();
//     if (!scene)
//         return;
//
//     // One FRenderView per camera entry in the state
//     for (const auto& [cameraId, viewState] : state.viewstateMap)
//     {
//         CameraEditorTool* tool = m_CameraTools.Get(cameraId);
//         if (!tool)
//             continue;
//
//         if (viewState.width <= 0.f || viewState.height <= 0.f)
//             continue;
//
//         const int vpW = static_cast<int>(viewState.width);
//         const int vpH = static_cast<int>(viewState.height);
//
//         // Per-camera MSAA sample count (for scene RT)
//         int samples = 1;
//         if (auto itSamples = state.cameraSampleMap.find(cameraId);
//             itSamples != state.cameraSampleMap.end())
//         {
//             samples = itSamples->second;
//         }
//
//         // Ensure this camera has a viewport RT of the right size
//         FViewportRT& rt = tool->GetRT();
//         if (!rt.fbo.IsValid() || rt.width != vpW || rt.height != vpH)
//         {
//             // Destroy old
//             if (rt.fbo.IsValid())
//             {
//                 m_Renderer.DestroyColorTarget(rt.fbo);
//                 rt.fbo   = {};
//                 rt.color = {};
//                 rt.width = 0;
//                 rt.height = 0;
//             }
//
//             // Create new
//             RTextureHandle colorTex{};
//             RFramebufferHandle fbo = m_Renderer.CreateColorTarget(vpW, vpH, colorTex);
//
//             rt.fbo   = fbo;
//             rt.color = colorTex;
//             rt.width = vpW;
//             rt.height = vpH;
//         }
//
//         if (!rt.fbo.IsValid() || !rt.color.IsValid())
//             continue;
//
//         // Build the view for this editor camera
//         FRenderView view{};
//         view.scene     = scene;
//         view.camera    = tool;
//         view.viewType  = EViewType::GameView;   // or EViewType::EditorScene if you add it
//         view.viewIndex = viewState.viewIndex;
//
//         view.targetFBO = rt.fbo;     // blit final scene into this FBO
//
//         view.viewportX = 0;
//         view.viewportY = 0;
//         view.viewportW = vpW;
//         view.viewportH = vpH;
//
//         view.sampleCount        = samples;
//         view.bClearColor        = true;
//         view.bClearDepth        = true;
//         view.clearColorValue    = {0.1f, 0.1f, 0.1f, 1.0f};
//         view.renderMask         = 0xFFFFFFFFu;
//         view.bApplyPostGamma    = false; // TODO: Make this configurable for future
//         view.bEnablePostProcess = true;
//         view.postProfileId      = kEditorPostProfile;
//
//         m_Context.SubmitViewSource(view);
//     }
// }