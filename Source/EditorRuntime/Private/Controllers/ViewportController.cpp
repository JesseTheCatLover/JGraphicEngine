#include "Controllers/ViewportController.h"

#include "Core/EditorHost.h"
#include "EditorRuntime.h"
#include "Controllers/Inputs/FViewportPanelInput.h"
#include "Controllers/Outputs/FViewportOutput.h"
#include "Tools/CameraEditorTool.h"
#include "ToolService.h"
#include "Rendering/EViewType.h"
#include "Rendering/FRenderView.h"

ViewportController::ViewportController(PanelID id, EditorHost& host, EditorRuntime& runtime, ToolService& tools)
    : m_PanelID(id)
    , m_Host(host)
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
    DestroyCameraTool();
    DestroyRenderTarget();
}

void ViewportController::EnsureCameraTool()
{
    if (m_CameraToolID == 0)
        m_CameraToolID = m_Tools.CreateCameraTool();
}

void ViewportController::DestroyCameraTool()
{
    if (m_CameraToolID != 0)
    {
        m_Tools.DestroyCameraTool(m_CameraToolID);
        m_CameraToolID = 0;
    }
}

void ViewportController::EnsureRenderTarget()
{
    if (m_Width <= 0.f || m_Height <= 0.f)
        return;

    const int vpW = (int)m_Width;
    const int vpH = (int)m_Height;

    if (!m_RT.fbo.IsValid() || m_RT.width != vpW || m_RT.height != vpH)
    {
        DestroyRenderTarget();

        // Create RT owned by the controller
        m_Runtime.GetViewport().CreateViewportTarget(vpW, vpH, m_RT);
    }
}

void ViewportController::DestroyRenderTarget()
{
    if (m_RT.fbo.IsValid())
    {
        m_Runtime.GetViewport().DestroyViewportTarget(m_RT);
        m_RT = {};
    }
}

void ViewportController::TickCamera(float dt)
{
    CameraEditorTool* cam = m_Tools.GetCameraTool(m_CameraToolID);
    if (!cam) return;

    const bool bActive = m_Focused;

    float aspect = (m_Height > 0.f) ? (m_Width / m_Height) : (16.f / 9.f);

    cam->Tick(dt, bActive, aspect);
}

void ViewportController::SubmitView()
{
    if (!m_RT.fbo.IsValid())
        return;

    CameraEditorTool* cam = m_Tools.GetCameraTool(m_CameraToolID);
    if (!cam) return;

    auto* scene = m_Runtime.GetScene().GetActiveScene();
    if (!scene) return;

    FRenderView view{};
    view.scene     = scene;
    view.camera    = cam;
    view.viewType  = EViewType::EditorViewport;
    view.viewIndex = 0;

    view.targetFBO = m_RT.fbo;

    view.viewportX = 0;
    view.viewportY = 0;
    view.viewportW = m_RT.width;
    view.viewportH = m_RT.height;

    view.sampleCount        = m_MSAASamples;
    view.bClearColor        = true;
    view.bClearDepth        = true;
    view.clearColorValue    = {0.1f, 0.1f, 0.1f, 1.0f};
    view.renderMask         = 0xFFFFFFFFu;
    view.bEnablePostProcess = true;

    m_Runtime.GetViewport().SubmitRenderView(view);
}

void ViewportController::Update(float deltaTime, const FViewportPanelInput& input, FViewportOutput& out)
{
    EnsureCameraTool();

    // 1) Read snapshot input
    m_Width   = input.width;
    m_Height  = input.height;
    m_Focused = input.bFocused;
    m_Hovered = input.bHovered && input.bOverViewport;

    // 2) Tick tools
    TickCamera(deltaTime);

    // 3) Render
    if (m_Width > 0.f && m_Height > 0.f)
    {
        EnsureRenderTarget();
        SubmitView();
    }
    else
    {
        DestroyRenderTarget();
    }

    // 4) Write snapshot output
    if (m_RT.color.IsValid())
    {
        out.nativeTexture = m_Runtime.GetViewport().GetNativeTexture(m_RT.color);
        out.bHasTexture   = (out.nativeTexture != nullptr);
    }
    else
    {
        out.nativeTexture = nullptr;
        out.bHasTexture   = false;
    }
}
