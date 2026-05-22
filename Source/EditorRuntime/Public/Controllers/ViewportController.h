//  Copyright 2025-2026 JesseTheCatLover. All Rights Reserved.

#pragma once
#include <vector>
#include <cstdint>

#include "PanelRegistry.h"
#include "Rendering/FViewportRT.h"
#include "Tools/Controllers/GizmoEditorController.h"
#include "Utilities/UDynamicID.h"

class ViewportSubsystem;
class HierarchyService;
class SelectionService;
class ScenePickingService;

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
    ScenePickingService&   m_Picker;
    HierarchyService&  m_Hierarchy;
    ViewportSubsystem& m_ViewportSubsystem;

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

    struct FGizmoEditSession
    {
        bool bActive = false;

        GizmoEditorTool::EMode  mode  = GizmoEditorTool::EMode::Translate;
        GizmoEditorTool::ESpace space = GizmoEditorTool::ESpace::World;
        GizmoEditorTool::EHandle handle = GizmoEditorTool::EHandle::None;

        // Pivot and basis locked at capture begin
        FTransform gizmoStartXf{};
        FVector3  pivotWS{0,0,0};
        FVector3  basisX{1,0,0}, basisY{0,1,0}, basisZ{0,0,1};

        // Selection snapshot at capture begin
        std::vector<uint64_t> actors;
        std::vector<FTransform> startXfs; // same order as actors

        void Reset() { *this = FGizmoEditSession{}; }
    };

    FGizmoEditSession m_GizmoSession;

private:
    void EnsureCameraTool();
    void DestroyCameraTool();

    void EnsureRenderTarget();
    void DestroyRenderTarget();

    void UpdateInputPolicy(const FViewportPanelInput& input);

    void TickCamera(float deltaTime);
    bool BuildRenderView(FRenderView& outView) const;
    void SubmitView(const FRenderView& view);

    void HandleActorPicking(const FViewportPanelInput& input, CameraEditorTool* cam, const ScenePickingService& picker,
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

    // Gizmo edit session

    void BeginGizmoEditSession(const SelectionService& selection, const FTransform& gizmoXf);
    void UpdateGizmoEditSession(const GizmoEditorController::FGizmoTransformDelta& delta);
    void EndGizmoEditSession(bool bCommit);

    static FTransform ApplyDeltaToTransformWS(const FTransform& start,
                                             const FGizmoEditSession& session,
                                             const GizmoEditorController::FGizmoTransformDelta& delta);

public:
    ViewportController(PanelID id, EditorHost& host, EditorRuntime& runtime, ToolService& tools);
    ~ViewportController();

    void Update(float deltaTime, const FViewportPanelInput& input, FViewportOutput& out);

    void OnPanelDestroyed();
};
