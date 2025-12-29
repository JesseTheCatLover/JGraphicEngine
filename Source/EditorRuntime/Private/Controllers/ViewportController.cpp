#include "Controllers/ViewportController.h"
#include <cstdint>

#include "Core/EditorHost.h"
#include "EditorRuntime.h"
#include "Controllers/Inputs/FViewportPanelInput.h"
#include "Controllers/Outputs/FViewportOutput.h"
#include "Tools/CameraEditorTool.h"
#include "ToolService.h"
#include "Core/Services/HierarchyService.h"
#include "Core/Services/PickingService.h"
#include "Core/Services/SelectionService.h"
#include "Rendering/EViewType.h"
#include "Rendering/FRenderView.h"
#include "Scene/FSelectionModifiers.h"

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

    const bool bActive = m_bHasMouseCapture;

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
    view.bApplyPostGamma    = false; // TODO: Make this configurable for future
    view.bEnablePostProcess = true;
    view.postProfileId = EditorRuntime::kEditorPostProfile;

    m_Runtime.GetViewport().SubmitRenderView(view);
}

void ViewportController::UpdateCapturePolicy(const FViewportPanelInput &input)
{
    // Start capture
    if (input.bOverViewport && input.bRightClicked)
    {
        m_bHasMouseCapture = true;
        m_Runtime.GetSurface().SetCursorDisabled();
    }

    // End capture
    if (m_bHasMouseCapture && input.bRightReleased)
    {
        m_bHasMouseCapture = false;
        m_Runtime.GetSurface().SetCursorVisible();
    }
}

void ViewportController::HandlePicking(const FViewportPanelInput& input)
{
    if (!(input.bOverViewport && input.bLeftClicked))
        return;

    const auto cam = m_Tools.GetCameraTool(m_CameraToolID);
    if (!cam) return;

    FSelectionModifiers mods{};
    mods.bRange  = false;
    mods.bToggle = input.bCtrl || input.bShift || input.bSuper;

    auto& picker    = m_Host.GetService<PickingService>();
    auto& selection = m_Host.GetService<SelectionService>();
    auto& hierarchy = m_Host.GetService<HierarchyService>();

    const ActorID id = picker.PickActorAtViewportPos(*cam, input.width, input.height, input.mouseX, input.mouseY);

    selection.ApplyClick(id, mods, mods.bRange ? &hierarchy.GetVisibleOrder() : nullptr);
}

void ViewportController::Update(float deltaTime, const FViewportPanelInput& input, FViewportOutput& out)
{
    EnsureCameraTool();

    // Read snapshot input
    m_Width   = input.width;
    m_Height  = input.height;
    m_Focused = input.bFocused;
    m_Hovered = input.bHovered && input.bOverViewport;

    // Policy
    UpdateCapturePolicy(input);
    HandlePicking(input);

    // Tick tools
    TickCamera(deltaTime);

    // Viewport Render
    if (m_Width > 0.f && m_Height > 0.f)
    {
        EnsureRenderTarget();
        SubmitView();
    }
    else
    {
        DestroyRenderTarget();
    }

    // Write snapshot output
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
