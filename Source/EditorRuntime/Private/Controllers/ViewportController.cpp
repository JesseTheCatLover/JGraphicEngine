//  Copyright 2025-2026 JesseTheCatLover. All Rights Reserved.

#pragma once

#include "Controllers/ViewportController.h"
#include <cstdint>
#include <iostream>

#include "Core/EditorHost.h"
#include "EditorRuntime.h"
#include "Controllers/Inputs/FViewportPanelInput.h"
#include "Controllers/Outputs/FViewportOutput.h"
#include "Tools/CameraEditorTool.h"
#include "ToolService.h"
#include "Core/Services/EditTimelineService.h"
#include "Core/Services/HierarchyService.h"
#include "Core/Services/ScenePickingService.h"
#include "Core/Services/SceneQueryService.h"
#include "Core/Services/SelectionService.h"
#include "Framework/InputManager.h"
#include "Rendering/EViewType.h"
#include "Rendering/FRenderView.h"
#include "Scene/FSelectionModifiers.h"
#include "Edits/UndoableActions/SetActorsTransformAction.h"

ViewportController::ViewportController(PanelID id, EditorHost& host, EditorRuntime& runtime, ToolService& tools)
    : m_PanelID(id)
    , m_Host(host)
    , m_Runtime(runtime)
    , m_Tools(tools)
    , m_Selection(m_Host.GetService<SelectionService>())
    , m_Picker(m_Host.GetService<ScenePickingService>())
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
    EndGizmoEditSession(false);

    if (IsMyCapture(EMouseCaptureKind::GizmoTransform))
        m_ViewportSubsystem.EndCapture(m_PanelID);

    m_Gizmo.CancelCapture();
}

void ViewportController::BeginGizmoEditSession(const SelectionService &selection, const FTransform &gizmoXf)
{
    m_GizmoSession.Reset();
    m_GizmoSession.bActive = true;

    m_GizmoSession.mode   = m_ViewportSubsystem.GetGizmoMode();
    m_GizmoSession.space  = m_ViewportSubsystem.GetGizmoSpace();
    m_GizmoSession.handle = m_Gizmo.IsCapturing() ? GizmoEditorTool::EHandle::None : GizmoEditorTool::EHandle::None; // optional

    m_GizmoSession.gizmoStartXf = gizmoXf;
    m_GizmoSession.pivotWS = gizmoXf.GetPosition();

    GizmoEditorController::BuildBasisWS(gizmoXf, m_GizmoSession.space,
                 m_GizmoSession.basisX, m_GizmoSession.basisY, m_GizmoSession.basisZ);

    // Snapshot selection + start transforms
    auto& queries = m_Host.GetService<SceneQueryService>();
    const auto& sel = selection.GetSelection();

    m_GizmoSession.actors.reserve(sel.size());
    m_GizmoSession.startXfs.reserve(sel.size());

    for (ActorID id : sel)
    {
        FTransform xf{};
        if (!queries.TryGetActorWorldTransform(id, xf))
            continue;

        m_GizmoSession.actors.push_back(id);
        m_GizmoSession.startXfs.push_back(xf);
    }

    if (m_GizmoSession.actors.empty())
        m_GizmoSession.Reset();
}

void ViewportController::UpdateGizmoEditSession(const GizmoEditorController::FGizmoTransformDelta &delta)
{
    if (!m_GizmoSession.bActive)
        return;

    if (!delta.bHasDelta)
        return;

    auto& queries = m_Host.GetService<SceneQueryService>();

    // Apply preview from start transforms every frame (stable, no drift)
    for (size_t i = 0; i < m_GizmoSession.actors.size(); ++i)
    {
        const ActorID id = m_GizmoSession.actors[i];
        const FTransform& startXf = m_GizmoSession.startXfs[i];

        const FTransform newXf = ApplyDeltaToTransformWS(startXf, m_GizmoSession, delta);

        queries.TrySetActorWorldTransform(id, newXf);
    }
}

void ViewportController::EndGizmoEditSession(bool bCommit)
{
    if (!m_GizmoSession.bActive)
        return;

    auto& queries = m_Host.GetService<SceneQueryService>();

    if (!bCommit)
    {
        // revert preview
        for (size_t i = 0; i < m_GizmoSession.actors.size(); ++i)
            queries.TrySetActorWorldTransform(m_GizmoSession.actors[i], m_GizmoSession.startXfs[i]);

        m_GizmoSession.Reset();
        return;
    }

    // 1) Capture AFTER transforms at commit time
    std::vector<FTransform> endXfs;
    endXfs.reserve(m_GizmoSession.actors.size());

    for (ActorID id : m_GizmoSession.actors)
    {
        FTransform xf{};
        if (!queries.TryGetActorWorldTransform(id, xf))
            xf = FTransform{}; // fallback
        endXfs.push_back(xf);
    }

    // 2) If nothing actually changed, don't push history
    bool anyChanged = false;
    const size_t n = std::min(endXfs.size(), m_GizmoSession.startXfs.size());
    for (size_t i = 0; i < n; ++i)
    {
        if (!(endXfs[i] == m_GizmoSession.startXfs[i]))
        {
            anyChanged = true;
            break;
        }
    }

    if (anyChanged)
    {
        auto& timeline = m_Host.GetService<EditTimelineService>();

        // Optional nicer title
        std::string title = "Transform Actors";
        switch (m_GizmoSession.mode)
        {
            case GizmoEditorTool::EMode::Translate: title = "Move Actors"; break;
            case GizmoEditorTool::EMode::Rotate:    title = "Rotate Actors"; break;
            case GizmoEditorTool::EMode::Scale:     title = "Scale Actors"; break;
            default: break;
        }

        timeline.Execute(MakeUnique<SetActorsTransformAction>(
            m_Runtime,
            m_GizmoSession.actors,
            m_GizmoSession.startXfs,
            endXfs,
            title));
    }

    m_GizmoSession.Reset();
}

FTransform ViewportController::ApplyDeltaToTransformWS(const FTransform &start, const FGizmoEditSession &session,
    const GizmoEditorController::FGizmoTransformDelta &delta)
{
    FTransform out = start;

    const FVector3 pivot = session.pivotWS;

    // --- TRANSLATE ---
    if (delta.mode == GizmoEditorTool::EMode::Translate)
    {
        out.SetPosition(start.GetPosition() + delta.deltaTranslationWS);
        return out;
    }

    // --- ROTATE (world space around pivot) ---
    if (delta.mode == GizmoEditorTool::EMode::Rotate)
    {
        const FVector3 axis = delta.rotationAxisWS.Normalized();
        const float angle = delta.rotationAngleRad;

        if (std::fabs(angle) < 1e-8f)
            return out;

        const FQuat qDelta(axis, angle);

        // rotate position around pivot
        const FVector3 p0 = start.GetPosition();
        const FVector3 v  = p0 - pivot;
        const FVector3 v2 = qDelta.RotateVector(v);
        out.SetPosition(pivot + v2);

        // rotate orientation in world space (pre-multiply is typical for world delta)
        // If your convention is opposite, swap order: startRot * qDelta
        const FQuat startRot = start.GetRotation();
        out.SetRotation(qDelta * startRot);

        return out;
    }

    // --- SCALE (around pivot, in gizmo basis) ---
    if (delta.mode == GizmoEditorTool::EMode::Scale)
    {
        const FVector3 mul = delta.deltaScaleMul;

        // 1) Scale position around pivot in gizmo basis (works for any selection count)
        const FVector3 p0 = start.GetPosition();
        const FVector3 off = p0 - pivot;

        const float x = off.Dot(session.basisX);
        const float y = off.Dot(session.basisY);
        const float z = off.Dot(session.basisZ);

        const FVector3 offScaled =
            session.basisX * (x * mul.x) +
            session.basisY * (y * mul.y) +
            session.basisZ * (z * mul.z);

        out.SetPosition(pivot + offScaled);

        // 2) Scale the actor itself.
        // “Pro” path is matrix decomposition; for now we keep it predictable:
        // - Local gizmo: scale local components directly (matches most editors)
        // - World gizmo: apply uniform mul to avoid weird rotation-coupling
        const FVector3 s0 = start.GetScale();

        if (session.space == GizmoEditorTool::ESpace::Local)
        {
            out.SetScale(FVector3(s0.x * mul.x, s0.y * mul.y, s0.z * mul.z));
        }
        else
        {
            out.SetScale(FVector3(s0.x * mul.x, s0.y * mul.y, s0.z * mul.z));
        }

        return out;
    }

    return out;
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

    if (inputManager.GetActionDown("Editor.Tools.Translate"))
        m_ViewportSubsystem.SetGizmoMode(GizmoEditorTool::EMode::Translate);

    if (inputManager.GetActionDown("Editor.Tools.Rotate"))
        m_ViewportSubsystem.SetGizmoMode(GizmoEditorTool::EMode::Rotate);

    if (inputManager.GetActionDown("Editor.Tools.Scale"))
        m_ViewportSubsystem.SetGizmoMode(GizmoEditorTool::EMode::Scale);
}

void ViewportController::HandleActorPicking(const FViewportPanelInput& input,
                                           CameraEditorTool* cam,
                                           const ScenePickingService& picker,
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

    // Claim global capture if local capture started
    if (m_Gizmo.IsCapturing() && !IsMyCapture(EMouseCaptureKind::GizmoTransform))
    {
        // If this fails, immediately cancel local capture so we don't desync
        if (!m_ViewportSubsystem.TryBeginCapture(m_PanelID, EMouseCaptureKind::GizmoTransform))
        {
            EndGizmoEditSession(false);
            m_Gizmo.CancelCapture();
        }
    }

    // Session begin
    if (gizmoResult.bBeganCapture && m_Gizmo.IsCapturing())
        BeginGizmoEditSession(selection, gizmoXf);

    // Session update (preview apply)
    if (m_Gizmo.IsCapturing() && gizmoResult.manipulation.bHasDelta)
        UpdateGizmoEditSession(gizmoResult.manipulation);

    // Session end
    if (gizmoResult.bEndedCapture)
        EndGizmoEditSession(true);

    // If local capture stopped, release global capture
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

    bool bBlockPick = false;
    if (bHasView)
        bBlockPick = HandleGizmo(input, *cam, view, m_Selection);

    if (!bBlockPick)
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
