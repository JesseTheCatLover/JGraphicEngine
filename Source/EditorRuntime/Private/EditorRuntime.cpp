//  Copyright 2025-2026 JesseTheCatLover. All Rights Reserved.

#include "EditorRuntime.h"

#include "Core/EngineContext.h"
#include "Core/EngineGlobals.h"
#include "Core/JEngine.h"
#include "Framework/PostProcessManager.h"
#include "Framework/SceneManager.h"
#include "Rendering/FRenderView.h"
#include "Rendering/IPlatformSurface.h"
#include "Rendering/RendererSubsystem.h"
#include "Resources/ResourceSubsystem.h"

EditorRuntime::EditorRuntime()
    : m_Context(JEngine::Get().GetEngineContext())
    , m_SceneManager(*JEngine::Get().GetSceneManager())
    , m_Renderer(*JEngine::Get().GetRenderer())
    , m_PlatformSurface(*JEngine::Get().GetPlatformSurface())
    , m_Resource(*JEngine::Get().GetResourceSubsystem())
    , m_SceneAPI(m_Context, m_SceneManager, *JEngine::Get().GetDebugDraw())
    , m_ViewportAPI(m_Context, m_Renderer)
    , m_SurfaceAPI(m_Context, m_PlatformSurface, *JEngine::Get().GetInputManager())
    , m_FileAPI(m_Context, m_Resource)
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