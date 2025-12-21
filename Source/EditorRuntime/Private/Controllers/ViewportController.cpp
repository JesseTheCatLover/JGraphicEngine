//  Copyright 2025 JesseTheCatLover. All Rights Reserved.

#include "Controllers/ViewportController.h"
#include "Core/EditorCore.h"
#include "EditorRuntime.h"
#include "Tools/EditorToolManager.h"

#include "Tools/CameraEditorTool.h"

ViewportController::ViewportController(PanelID panelId, EditorCore& core, EditorRuntime& runtime, EditorToolManager& tools)
    : m_PanelId(panelId)
    , m_Core(core)
    , m_Runtime(runtime)
    , m_Tools(tools)
{
}

ViewportController::~ViewportController()
{
    OnPanelDestroyed();
}

void ViewportController::OnPanelDestroyed()
{
    // Destroy camera tool instance (and its RT, if camera tool owns it)
    if (m_CameraToolId != 0)
    {
        m_Tools.DestroyCameraTool(m_CameraToolId);
        m_CameraToolId = 0;
    }

    m_Color = {};
    m_NativeTexture = nullptr;
}

void ViewportController::EnsureCameraTool()
{
    if (m_CameraToolId == 0)
        m_CameraToolId = m_Tools.CreateCameraTool();
}

void ViewportController::Update(float dt, const FViewportPanelContext& frame)
{
    EnsureCameraTool();

    m_Width = frame.width;
    m_Height = frame.height;
    m_Focused = frame.focused;
    m_Hovered = frame.hovered;

    // 1) Tick tools (camera uses your input system internally)
    UpdateCameraTool(dt);

    // 2) Submit view and cache output texture handle
    SubmitView();
}

void ViewportController::UpdateCameraTool(float dt)
{
    CameraEditorTool* cam = m_Tools.GetCameraTool(m_CameraToolId);
    if (!cam) return;

    // Only allow camera input when focused (policy point)
    const bool bActive = m_Focused;

    float aspect = 16.f / 9.f;
    if (m_Width > 0.f && m_Height > 0.f)
        aspect = m_Width / m_Height;

    cam->Tick(dt, bActive, aspect);
}

void ViewportController::SubmitView()
{
    if (m_Width <= 0.f || m_Height <= 0.f)
        return;

    // For now, keep using your existing EditorRuntime::SubmitEditorViewSources flow.
    // We'll build a tiny camera state for just this one camera tool.

    FCameraToolState cameraState{};
    cameraState.activeCameraId = (m_Focused ? m_CameraToolId : 0);

    // view state for this camera
    FCameraToolViewState vs{};
    vs.width = m_Width;
    vs.height = m_Height;
    vs.aspect = (m_Height > 0.f) ? (m_Width / m_Height) : (16.f/9.f);
    vs.viewIndex = 0; // you can set per panel index if needed

    cameraState.viewstateMap[m_CameraToolId] = vs;
    cameraState.cameraSampleMap[m_CameraToolId] = m_MSAASamples;

    // Submits a render view into EngineContext and ensures RT in camera tool (current design)
    m_Runtime.SubmitEditorViewSources(cameraState);

    // Get color texture for panel and cache native handle for UI
    m_Color = m_Runtime.GetViewportColorHandle(m_CameraToolId);
    m_NativeTexture = m_Runtime.GetNativeTextureHandle(m_Color);
}
