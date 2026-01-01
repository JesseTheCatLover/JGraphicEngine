//  Copyright 2025 JesseTheCatLover. All Rights Reserved.

#pragma once

#include "PanelRegistry.h"
#include "Rendering/FViewportRT.h"
#include "Tools/Controllers/GizmoEditorController.h"
#include "Utilities/UDynamicID.h"

class ViewportSubsystem;
class HierarchyService;
class SelectionService;
class PickingService;

enum class EMouseCaptureKind : uint8_t
{
    None,
    CameraFly,
    CameraOrbit,
    GizmoTransform,
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

    // Service cashes
    SelectionService& m_Selection;
    PickingService&   m_Picker;
    HierarchyService&  m_Hierarchy;
    ViewportSubsystem& m_ViewportSubsystem;

    // One camera tool per panel
    UDynamicID::IDType m_CameraToolID = 0;

    // Controller-owned RT
    FViewportRT m_RT;

    int m_PostProfile = 1;

    // Stable view index per viewport controller instance
    int m_ViewportIndex = -1;

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

private:
    void EnsureCameraTool();
    void DestroyCameraTool();

    void EnsureRenderTarget();
    void DestroyRenderTarget();

    void UpdateInputPolicy(const FViewportPanelInput& input);

    void TickCamera(float deltaTime);
    bool BuildRenderView(FRenderView& outView) const;
    void SubmitView(const FRenderView& view);

    void HandleActorPicking(const FViewportPanelInput& input, CameraEditorTool* cam, const PickingService& picker,
                                                    SelectionService& selection, const HierarchyService& hierarchy);

    void EnsureGizmoIDs();
    bool HandleGizmo(const FViewportPanelInput& input, const CameraEditorTool& cam, const FRenderView& view,
                                                    const SelectionService& selection);
    bool TryBuildGizmoTransform(FTransform& outXf) const;
    void UpdateSharedGizmoModePolicy(const FViewportPanelInput& input); // shared mode hotkeys

    // Capture
    void CheckPanelFocusStatus(const FViewportPanelInput& input);

    void UpdateCameraCapturePolicy(const FViewportPanelInput& input);

    bool CanConsumeInputThisFrame(const FViewportPanelInput& input) const;
    bool IsMyCapture(EMouseCaptureKind kind) const;
    bool CanDriveSharedHotkeys(const FViewportPanelInput& input) const;
    bool AllowBeginGizmoCapture(const FViewportPanelInput& input) const;

    void CancelGizmoCapture();

public:
    ViewportController(PanelID id, EditorHost& host, EditorRuntime& runtime, ToolService& tools);
    ~ViewportController();

    void Update(float deltaTime, const FViewportPanelInput& input, FViewportOutput& out);

    void OnPanelDestroyed();
};
