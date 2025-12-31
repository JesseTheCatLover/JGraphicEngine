//  Copyright 2025 JesseTheCatLover. All Rights Reserved.

#pragma once

#include "PanelRegistry.h"
#include "Rendering/FViewportRT.h"
#include "Tools/Controllers/GizmoEditorController.h"
#include "Utilities/UDynamicID.h"

enum class ECaptureOwner : uint8_t
{
    None,
    CameraRMB,
    GizmoLMB,
};

struct FViewportPolicy
{
    bool bSuppressActorPick = false; // gizmo-first or gizmo capture
    bool bCameraMovementActive      = false; // whether cam tool should activative input movement this frame
};

class CameraEditorTool;
struct FViewportPanelInput;
struct FViewportOutput;

class EditorHost;
class EditorRuntime;
class ToolService;

class ViewportController
{
private:
    PanelID m_PanelID = 0;

    EditorHost&    m_Host;
    EditorRuntime& m_Runtime;
    ToolService&   m_Tools;

    // One camera tool per panel
    UDynamicID::IDType m_CameraToolID = 0;

    // Controller-owned RT
    FViewportRT m_RT;

    int m_PostProfile = 1;

    // Cached viewport facts (from input)
    float m_Width = 0.f;
    float m_Height = 0.f;
    bool m_Focused = false;
    bool m_Hovered = false;

    int m_MSAASamples = 4;

    // Per-viewport gizmo state/policy (controller-owned)
    GizmoEditorController m_Gizmo;

    // Stable base id for this panel so hitIds don’t collide
    uint32_t m_GizmoBaseHitID = 0;

    // Cache the last hovered/active for debug/UI
    GizmoEditorTool::EHandle m_GizmoHovered = GizmoEditorTool::EHandle::None;
    GizmoEditorTool::EHandle m_GizmoActive = GizmoEditorTool::EHandle::None;

    // Stable view index per viewport controller instance
    int m_ViewportIndex = -1;

    // Explicit capture ownership
    ECaptureOwner m_CaptureOwner = ECaptureOwner::None;

private:
    void EnsureCameraTool();
    void DestroyCameraTool();

    void EnsureRenderTarget();
    void DestroyRenderTarget();

    void TickCamera(float deltaTime, bool bCameraMovementActive);
    bool BuildRenderView(FRenderView& outView) const;
    void SubmitView(const FRenderView& view);

    void HandleActorPicking(const FViewportPanelInput& input);

    void EnsureGizmoIDs();
    bool HandleGizmo(const FViewportPanelInput& input, const CameraEditorTool& cam, const FRenderView& view, bool bAllowBeginCapture);
    bool TryBuildGizmoTransform(FTransform& outXf) const;
    void UpdateGizmoMode();

    void ForceReleaseCapture();
    void SetCursorCaptured(bool bCaptured);
    void UpdateCaptureFromFocus(const FViewportPanelInput& input);
    void UpdateCameraCapturePolicy(const FViewportPanelInput& input);
    bool UpdateGizmoPolicyAndDraw(const FViewportPanelInput& input,
                                     const CameraEditorTool* cam,
                                     const FRenderView* view); // returns suppressPick
    [[nodiscard]] FViewportPolicy ComputeFramePolicy(bool bGizmoSuppressPick) const;

public:
    ViewportController(PanelID id, EditorHost& host, EditorRuntime& runtime, ToolService& tools);
    ~ViewportController();

    void Update(float deltaTime, const FViewportPanelInput& input, FViewportOutput& out);

    void OnPanelDestroyed();
};
