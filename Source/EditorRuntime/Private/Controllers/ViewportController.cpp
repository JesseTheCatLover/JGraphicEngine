#include "Controllers/ViewportController.h"
#include <cstdint>
#include <iostream>
#include <atomic>

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

// Allocate stable view indices once per controller instance
static std::atomic<int> GNextViewportIndex{1};

ViewportController::ViewportController(PanelID id, EditorHost& host, EditorRuntime& runtime, ToolService& tools)
    : m_PanelID(id)
    , m_Host(host)
    , m_Runtime(runtime)
    , m_Tools(tools)
{
    m_ViewportIndex = GNextViewportIndex.fetch_add(1);
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

void ViewportController::TickCamera(float deltaTime, bool bCameraMovementActive)
{
    CameraEditorTool* cam = m_Tools.GetCameraTool(m_CameraToolID);
    if (!cam) return;

    float aspect = (m_Height > 0.f) ? (m_Width / m_Height) : (16.f / 9.f);

    cam->Tick(deltaTime, bCameraMovementActive, aspect);
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
    outView.viewIndex = m_ViewportIndex;

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

void ViewportController::SetCursorCaptured(bool bCaptured)
{
    if (bCaptured)
        m_Runtime.GetSurface().SetCursorDisabled();
    else
        m_Runtime.GetSurface().SetCursorVisible();
}

void ViewportController::ForceReleaseCapture()
{
    if (m_CaptureOwner == ECaptureOwner::CameraRMB)
        SetCursorCaptured(false);

    m_CaptureOwner = ECaptureOwner::None;

    // Safety: ensure gizmo is not stuck
    m_Gizmo.CancelCapture();
    m_GizmoHovered = GizmoEditorTool::EHandle::None;
    m_GizmoActive  = GizmoEditorTool::EHandle::None;
}


void ViewportController::UpdateCaptureFromFocus(const FViewportPanelInput& input)
{
    // If panel/app focus is lost, never remain captured.
    if (!input.bFocused)
        ForceReleaseCapture();
}

void ViewportController::UpdateCameraCapturePolicy(const FViewportPanelInput& input)
{
    // Camera capture can only begin if nobody else owns capture.
    const bool bCanBegin = (m_CaptureOwner == ECaptureOwner::None);

    if (bCanBegin && input.bOverViewport && input.bRightClicked)
    {
        m_CaptureOwner = ECaptureOwner::CameraRMB;
        SetCursorCaptured(true);
    }

    if (m_CaptureOwner == ECaptureOwner::CameraRMB)
    {
        // Release on RMB up OR focus loss handled elsewhere
        if (input.bRightReleased)
        {
            SetCursorCaptured(false);
            m_CaptureOwner = ECaptureOwner::None;
        }
    }
}

bool ViewportController::UpdateGizmoPolicyAndDraw(const FViewportPanelInput& input,
                                                     const CameraEditorTool* cam,
                                                     const FRenderView* view)
{
    // If camera owns capture, gizmo cannot begin capture this frame.
    const bool bAllowBegin = (m_CaptureOwner == ECaptureOwner::None);

    bool bSuppressPick = false;

    if (cam && view)
        bSuppressPick = HandleGizmo(input, *cam, *view, bAllowBegin);
    else
    {
        m_Gizmo.CancelCapture();
        m_GizmoHovered = GizmoEditorTool::EHandle::None;
        m_GizmoActive  = GizmoEditorTool::EHandle::None;
        return false;
    }

    // If gizmo is capturing, it owns capture (exclusive)
    if (m_Gizmo.IsCapturing())
        m_CaptureOwner = ECaptureOwner::GizmoLMB;
    else if (m_CaptureOwner == ECaptureOwner::GizmoLMB)
        m_CaptureOwner = ECaptureOwner::None;

    return bSuppressPick;
}

FViewportPolicy ViewportController::ComputeFramePolicy(bool bGizmoSuppressPick) const
{
    FViewportPolicy policy{};

    // Suppress pick while gizmo is capturing OR gizmo wants to consume click
    policy.bSuppressActorPick = bGizmoSuppressPick || (m_CaptureOwner == ECaptureOwner::GizmoLMB);

    // Camera tool active only when it owns capture
    policy.bCameraMovementActive = (m_CaptureOwner == ECaptureOwner::CameraRMB);

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
                                     const FRenderView& view,
                                     bool bAllowBeginCapture)
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
                                          camPos, camFwd, gizmoXf, bDrawGizmo, bAllowBeginCapture, input);

    m_GizmoHovered = gizmoResult.hovered;
    m_GizmoActive = gizmoResult.active;

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

void ViewportController::UpdateGizmoMode()
{
    // Don’t change modes during camera capture
    if (m_CaptureOwner == ECaptureOwner::CameraRMB) return;

    auto& inputManager = m_Runtime.GetSurface().GetInputManager();

    if (inputManager.GetActionDown("Editor_GizmoTranslate"))
    {
        m_Gizmo.SetMode(GizmoEditorTool::EMode::Translate);
    }
    if (inputManager.GetActionDown("Editor_GizmoRotation"))
    {
        m_Gizmo.SetMode(GizmoEditorTool::EMode::Rotate);
    }
    if (inputManager.GetActionDown("Editor_GizmoScale"))
    {
        m_Gizmo.SetMode(GizmoEditorTool::EMode::Scale);
    }
}

void ViewportController::Update(float deltaTime, const FViewportPanelInput& input, FViewportOutput& out)
{
    EnsureCameraTool();

    // Read snapshot input
    m_Width   = input.width;
    m_Height  = input.height;
    m_Focused = input.bFocused;
    m_Hovered = input.bHovered && input.bOverViewport;

    // Focus safety
    UpdateCaptureFromFocus(input);

    // RT lifecycle
    if (m_Width > 0.f && m_Height > 0.f) EnsureRenderTarget();
    else DestroyRenderTarget();

    // Build view once
    FRenderView view{};
    const bool bHasView = BuildRenderView(view);

    CameraEditorTool* cam = m_Tools.GetCameraTool(m_CameraToolID);
    if (!cam) return;

    // 1) Update gizmo first (so it can claim capture on LMB)
    const bool bGizmoSuppressPick = UpdateGizmoPolicyAndDraw(input, cam, bHasView ? &view : nullptr);

    // 2) Update camera RMB capture (only if nobody owns capture)
    //    Note: if gizmo captured, camera won't capture.
    UpdateCameraCapturePolicy(input);

    // 3) Compute policy from resulting capture state
    const FViewportPolicy policy = ComputeFramePolicy(bGizmoSuppressPick);

    // Picking (only if allowed)
    if (!policy.bSuppressActorPick)
        HandleActorPicking(input);

    // Tick camera using policy
    TickCamera(deltaTime, policy.bCameraMovementActive);

    UpdateGizmoMode();

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
