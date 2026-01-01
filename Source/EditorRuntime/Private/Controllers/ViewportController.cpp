#include "Controllers/ViewportController.h"
#include <cstdint>
#include <iostream>

#include "Core/EditorHost.h"
#include "EditorRuntime.h"
#include "Controllers/Inputs/FViewportPanelInput.h"
#include "Controllers/Outputs/FViewportOutput.h"
#include "Tools/CameraEditorTool.h"
#include "ToolService.h"
#include "Core/Services/HierarchyService.h"
#include "Core/Services/PickingService.h"
#include "Core/Services/SceneQueryService.h"
#include "Core/Services/SelectionService.h"
#include "Framework/InputManager.h"
#include "Rendering/EViewType.h"
#include "Rendering/FRenderView.h"
#include "Scene/FSelectionModifiers.h"

ViewportController::ViewportController(PanelID id, EditorHost& host, EditorRuntime& runtime, ToolService& tools)
    : m_PanelID(id)
    , m_Host(host)
    , m_Runtime(runtime)
    , m_Tools(tools)
    , m_Selection(m_Host.GetService<SelectionService>())
    , m_Picker(m_Host.GetService<PickingService>())
    , m_Hierarchy(m_Host.GetService<HierarchyService>())
    , m_ViewportSubsystem(m_Host.GetSubsystem<ViewportSubsystem>())
{
}

ViewportController::~ViewportController()
{
    OnPanelDestroyed();
}

void ViewportController::OnPanelDestroyed()
{
    if (m_ViewportSubsystem.IsCaptureOwner(m_PanelID))
        m_ViewportSubsystem.EndCapture(m_PanelID);

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

bool ViewportController::IsMyCapture(EMouseCaptureKind kind) const
{
    return m_ViewportSubsystem.IsCaptureOwner(m_PanelID) &&
           (m_ViewportSubsystem.GetCaptureKind() == kind);
}

bool ViewportController::CanConsumeInputThisFrame(const FViewportPanelInput& input) const
{
    if (!input.bAppFocused || input.bHidden)
        return false;

    // If someone is capturing, only the owner should react to inputs.
    if (m_ViewportSubsystem.HasMouseCapture())
        return m_ViewportSubsystem.IsCaptureOwner(m_PanelID);

    // If no capture, any controller can *exist*, but only the focused+hovered
    // viewport should drive “shared” hotkeys.
    return true;
}

bool ViewportController::CanDriveSharedHotkeys(const FViewportPanelInput &input) const
{
    if (!CanConsumeInputThisFrame(input)) return false;

    return true;
}

bool ViewportController::AllowBeginGizmoCapture(const FViewportPanelInput& input) const
{
    // Can only *start* gizmo capture if allowed to consume input.
    // (Once you already own gizmo capture, you can keep updating even if mouse leaves.)
    if (IsMyCapture(EMouseCaptureKind::GizmoTransform))
        return true;

    // starting capture needs ownership permission
    return CanConsumeInputThisFrame(input) && m_Focused && m_Hovered && input.bOverViewport;
}

void ViewportController::CancelGizmoCapture()
{
    if (IsMyCapture(EMouseCaptureKind::GizmoTransform))
        m_ViewportSubsystem.EndCapture(m_PanelID);

    m_Gizmo.CancelCapture();
}

void ViewportController::UpdateInputPolicy(const FViewportPanelInput& input)
{
    // Focus safety
    CheckPanelFocusStatus(input);

    // Shared gizmo hotkeys first (so mode is ready for gizmo update)
    UpdateSharedGizmoModePolicy(input);

    // Then capture decisions (camera capture can block gizmo capture start)
    UpdateCameraCapturePolicy(input);
}

void ViewportController::TickCamera(float deltaTime)
{
    CameraEditorTool* cam = m_Tools.GetCameraTool(m_CameraToolID);
    if (!cam) return;

    float aspect = (m_Height > 0.f) ? (m_Width / m_Height) : (16.f / 9.f);

    const bool bFly = IsMyCapture(EMouseCaptureKind::CameraFly);

    cam->Tick(deltaTime, bFly , aspect);
}

bool ViewportController::BuildRenderView(FRenderView &outView) const
{
    if (!m_RT.fbo.IsValid())
        return false;

    CameraEditorTool* cam = m_Tools.GetCameraTool(m_CameraToolID);
    if (!cam) return false;

    auto* scene = m_Runtime.GetScene().GetActiveScene();
    if (!scene) return false;

    outView = {};
    outView.scene     = scene;
    outView.camera    = cam;
    outView.viewType  = EViewType::EditorViewport;
    outView.viewIndex = m_PanelID;

    outView.targetFBO = m_RT.fbo;

    outView.viewportX = 0;
    outView.viewportY = 0;
    outView.viewportW = m_RT.width;
    outView.viewportH = m_RT.height;

    outView.sampleCount        = m_MSAASamples;
    outView.bClearColor        = true;
    outView.bClearDepth        = true;
    outView.clearColorValue    = {0.1f, 0.1f, 0.1f, 1.0f};
    outView.renderMask         = 0xFFFFFFFFu;
    outView.bApplyPostGamma    = false; // TODO: Make this configurable for future
    outView.bEnablePostProcess = true;
    outView.postProfileId = EditorRuntime::kEditorPostProfile;

    return true;
}

void ViewportController::SubmitView(const FRenderView& view)
{
    m_Runtime.GetViewport().SubmitRenderView(view);
}

void ViewportController::CheckPanelFocusStatus(const FViewportPanelInput& input)
{
    // If panel/app focus is lost, never remain captured.
    if (input.bAppFocused && !input.bHidden)
        return;

    // If I was the capture owner, release globally.
    if (m_ViewportSubsystem.IsCaptureOwner(m_PanelID))
        m_ViewportSubsystem.EndCapture(m_PanelID);

    // Always cancel local gizmo capture so we don't get stuck.
    m_Gizmo.CancelCapture();
}

void ViewportController::UpdateCameraCapturePolicy(const FViewportPanelInput& input)
{
    // Begin capture if nobody has capture
    if (!m_ViewportSubsystem.HasMouseCapture())
    {
        if (input.bOverViewport && input.bRightClicked)
            m_ViewportSubsystem.TryBeginCapture(m_PanelID, EMouseCaptureKind::CameraFly);
        return;
    }

    // If I'm the owner, and I'm in camera fly, release on RMB up
    if (IsMyCapture(EMouseCaptureKind::CameraFly))
    {
        if (input.bRightReleased)
            m_ViewportSubsystem.EndCapture(m_PanelID);
    }
}

void ViewportController::UpdateSharedGizmoModePolicy(const FViewportPanelInput& input)
{
    if (!CanDriveSharedHotkeys(input))
        return;

    if (m_ViewportSubsystem.HasCapture())
        return;

    auto& inputManager = m_Runtime.GetSurface().GetInputManager();

    if (inputManager.GetActionDown("Editor_GizmoTranslate"))
        m_ViewportSubsystem.SetGizmoMode(GizmoEditorTool::EMode::Translate);

    if (inputManager.GetActionDown("Editor_GizmoRotation"))
        m_ViewportSubsystem.SetGizmoMode(GizmoEditorTool::EMode::Rotate);

    if (inputManager.GetActionDown("Editor_GizmoScale"))
        m_ViewportSubsystem.SetGizmoMode(GizmoEditorTool::EMode::Scale);
}

void ViewportController::HandleActorPicking(const FViewportPanelInput& input,
                                           CameraEditorTool* cam,
                                           const PickingService& picker,
                                           SelectionService& selection,
                                           const HierarchyService& hierarchy)
{
    if (!CanConsumeInputThisFrame(input)) return;
    if (!cam) return;

    // Don’t pick while camera fly or gizmo transform capture exists
    if (m_ViewportSubsystem.HasMouseCapture())
        return;

    if (!(input.bOverViewport && input.bLeftClicked))
        return;

    FSelectionModifiers mods{};
    mods.bRange  = false;
    mods.bToggle = input.bCtrl || input.bShift || input.bSuper;

    const ActorID id = picker.PickActorAtViewportPos(*cam, input.width, input.height, input.mouseX, input.mouseY);
    selection.ApplyClick(id, mods, mods.bRange ? &hierarchy.GetVisibleOrder() : nullptr);
}

void ViewportController::EnsureGizmoIDs()
{
    if (m_GizmoBaseHitID != 0) return;

    // simple deterministic base per panel
    m_GizmoBaseHitID = 100000u + uint32_t(m_PanelID) * 100u;
    m_Gizmo.SetBaseHitID(m_GizmoBaseHitID);
}

bool ViewportController::HandleGizmo(const FViewportPanelInput& input,
                                     const CameraEditorTool& cam,
                                     const FRenderView& view,
                                     const SelectionService& selection)
{
    // Always apply shared mode/space to local gizmo (read-only)
    m_Gizmo.SetMode(m_ViewportSubsystem.GetGizmoMode());
    m_Gizmo.SetSpace(m_ViewportSubsystem.GetGizmoSpace());

    const bool bDrawGizmo = !selection.IsSelectionEmpty();
    if (!bDrawGizmo)
    {
        // If selection disappeared and I was gizmo-capturing, release global capture
        CancelGizmoCapture();
        return false;
    }

    EnsureGizmoIDs();

    // Build pivot from selection
    FTransform gizmoXf{};
    const bool bHasPivot = TryBuildGizmoTransform(gizmoXf);
    if (!bHasPivot)
    {
        CancelGizmoCapture();
        return false;
    }

    const FMatrix4 viewMat = cam.GetViewMatrix();
    const FMatrix4 projMat = cam.GetProjectionMatrix(float(view.viewportW) / float(view.viewportH));

    DebugDraw& debugDraw = m_Runtime.GetScene().GetDebugDraw();

    const FVector3 camPos = cam.GetPosition();
    const FVector3 camFwd = cam.GetRotation().RotateVector(FVector3::Forward()).Normalized();

    const bool bAllowBeginCapture = AllowBeginGizmoCapture(input);

    const auto gizmoResult = m_Gizmo.UpdateAndDraw(debugDraw, view, viewMat, projMat,
                                          camPos, camFwd, gizmoXf, bDrawGizmo, bAllowBeginCapture, input);

    // If gizmo started capturing this frame, claim global capture
    if (m_Gizmo.IsCapturing() && !IsMyCapture(EMouseCaptureKind::GizmoTransform))
    {
        // If this fails, immediately cancel local capture so we don't desync
        if (!m_ViewportSubsystem.TryBeginCapture(m_PanelID, EMouseCaptureKind::GizmoTransform))
            m_Gizmo.CancelCapture();
    }

    // If gizmo stopped capturing and I own global gizmo capture, release
    if (!m_Gizmo.IsCapturing() && IsMyCapture(EMouseCaptureKind::GizmoTransform))
        m_ViewportSubsystem.EndCapture(m_PanelID);

    // Return whether capturing was successful this frame.
    return gizmoResult.bConsumesClick;
}

bool ViewportController::TryBuildGizmoTransform(FTransform& outXf) const // TODO: Add a cached gizmo pivot to SelectionService In Future (IMPORTANT FOR PERFORMANCE)
{
    auto& sel = m_Host.GetService<SelectionService>().GetSelection();
    if (sel.empty())
        return false;

    auto& queries = m_Host.GetService<SceneQueryService>();

    // Single selection: exact actor transform
    if (sel.size() == 1)
        return queries.TryGetActorWorldTransform(sel[0], outXf);

    // Multi-selection: position = average of actor positions, rotation = world
    FVector3 sum(0,0,0);
    int count = 0;

    for (ActorID id : sel)
    {
        FTransform xf{};
        if (!queries.TryGetActorWorldTransform(id, xf))
            continue;

        sum += xf.GetPosition();
        ++count;
    }

    if (count == 0)
        return false;

    const FVector3 pivot = sum * (1.0f / float(count));

    // Build a world-space pivot transform (rotation identity)
    outXf = FTransform{};
    outXf.SetPosition(pivot);
    outXf.SetRotation(FQuat{}); // identity
    outXf.SetScale(FVector3(1.f));
    return true;
}

void ViewportController::Update(float deltaTime, const FViewportPanelInput& input, FViewportOutput& out)
{
    EnsureCameraTool();

    // Read snapshot input
    m_Width   = input.width;
    m_Height  = input.height;
    m_Focused = input.bFocused;
    m_Hovered = input.bHovered && input.bOverViewport;

    // RT lifecycle
    if (m_Width > 0.f && m_Height > 0.f) EnsureRenderTarget();
    else DestroyRenderTarget();

    // View
    FRenderView view{};
    bool bHasView = false;
    if (!input.bHidden)
        bHasView = BuildRenderView(view);

    // Policy (focus/capture/hotkeys)
    UpdateInputPolicy(input);

    CameraEditorTool* cam = m_Tools.GetCameraTool(m_CameraToolID);
    if (!cam) return;

    if (bHasView)
        HandleGizmo(input, *cam, view, m_Selection);

    HandleActorPicking(input, cam, m_Picker, m_Selection, m_Hierarchy);

    // Tick camera
    TickCamera(deltaTime);

    // Submit render
    if (bHasView)
        SubmitView(view);

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
