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
#include "Core/Services/SceneQueryService.h"
#include "Core/Services/SelectionService.h"
#include "Rendering/EViewType.h"
#include "Rendering/FRenderView.h"
#include "Scene/FSelectionModifiers.h"

class SceneQueryService;

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

void ViewportController::TickCamera(float deltaTime)
{
    CameraEditorTool* cam = m_Tools.GetCameraTool(m_CameraToolID);
    if (!cam) return;

    const bool bActive = m_bHasMouseCapture && !m_Gizmo.IsCapturing();

    float aspect = (m_Height > 0.f) ? (m_Width / m_Height) : (16.f / 9.f);

    cam->Tick(deltaTime, bActive, aspect);
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
    outView.viewIndex = 0;

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

FViewportPolicy ViewportController::UpdateCapturePolicy(const FViewportPanelInput& input,
                                                       const CameraEditorTool* cam,
                                                       const FRenderView* view)
{
    FViewportPolicy policy{};

    // ----------------------------
    // 1) RMB camera capture policy
    // ----------------------------
    if (input.bOverViewport && input.bRightClicked)
    {
        m_bHasMouseCapture = true;
        m_Runtime.GetSurface().SetCursorDisabled();
    }

    if (m_bHasMouseCapture && input.bRightReleased)
    {
        m_bHasMouseCapture = false;
        m_Runtime.GetSurface().SetCursorVisible();
    }

    // ---------------------------------
    // 2) Gizmo policy
    // ---------------------------------
    bool bGizmoSuppressPick = false;

    if (cam && view)
    {
        bGizmoSuppressPick = HandleGizmo(input, *cam, *view);
        // HandleGizmo already cancels capture + clears hover/active when no selection pivot.
    }
    else
    {
        // No view -> no hit-test -> ensure we aren't "stuck" capturing gizmo
        m_Gizmo.CancelCapture();
        m_GizmoHovered = GizmoEditorTool::EHandle::None;
        m_GizmoActive  = GizmoEditorTool::EHandle::None;
    }

    // ---------------------------------
    // 3) Combine into frame-level policy
    // ---------------------------------
    policy.bSuppressActorPick = bGizmoSuppressPick;

    // Camera is active only when RMB capture AND gizmo not capturing
    policy.bCameraActive = (m_bHasMouseCapture && !m_Gizmo.IsCapturing());

    return policy;
}


void ViewportController::HandleActorPicking(const FViewportPanelInput& input)
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

void ViewportController::EnsureGizmoIDs()
{
    if (m_GizmoBaseHitID != 0) return;

    // simple deterministic base per panel
    m_GizmoBaseHitID = 100000u + uint32_t(m_PanelID) * 100u;
    m_Gizmo.SetBaseHitID(m_GizmoBaseHitID);
}

bool ViewportController::HandleGizmo(const FViewportPanelInput& input,
                                     const CameraEditorTool& cam,
                                     const FRenderView& view)
{
    EnsureGizmoIDs();

    // Build pivot from selection
    FTransform gizmoXf{};
    const bool bHasPivot = TryBuildGizmoTransform(gizmoXf);
    if (!bHasPivot)
    {
        m_Gizmo.CancelCapture();
        m_GizmoHovered = GizmoEditorTool::EHandle::None;
        m_GizmoActive  = GizmoEditorTool::EHandle::None;
        return false;
    }

    const FMatrix4 viewMat = cam.GetViewMatrix();
    const FMatrix4 projMat = cam.GetProjectionMatrix(float(view.viewportW) / float(view.viewportH));

    DebugDraw& debugDraw = m_Runtime.GetScene().GetDebugDraw();

    const FVector3 camPos = cam.GetPosition();
    const FVector3 camFwd = cam.GetRotation().RotateVector(FVector3::Forward()).Normalized();

    const bool bDrawGizmo = true; // later: only when something selected

    const auto gizmoResult = m_Gizmo.UpdateAndDraw(debugDraw, view, viewMat, projMat,
                                          camPos, camFwd, gizmoXf, bDrawGizmo, input);

    m_GizmoHovered = gizmoResult.hovered;
    m_GizmoActive = gizmoResult.active;

    // IMPORTANT: do NOT set m_bHasMouseCapture here.
    // Return whether actor picking should be suppressed this frame.
    return gizmoResult.bConsumesClick || m_Gizmo.IsCapturing();
}

bool ViewportController::TryBuildGizmoTransform(FTransform& outXf) const
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
    if (m_Width > 0.f && m_Height > 0.f)
        EnsureRenderTarget();
    else
        DestroyRenderTarget();

    // Build view once
    FRenderView view{};
    const bool bHasView = BuildRenderView(view);

    CameraEditorTool* cam = m_Tools.GetCameraTool(m_CameraToolID);
    if (!cam) return;

    // Policy
    const FViewportPolicy policy = UpdateCapturePolicy(input, cam, bHasView ? &view : nullptr);

    // Picking (only if allowed)
    if (!policy.bSuppressActorPick)
        HandleActorPicking(input);

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
