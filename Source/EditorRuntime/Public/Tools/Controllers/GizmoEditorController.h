//  Copyright 2025-2026 JesseTheCatLover. All Rights Reserved.

#pragma once

#include <cstdint>

#include "Core/Math/FMatrix4.h"
#include "Core/Math/FTransform.h"
#include "Framework/DebugDrawFramework.h"
#include "Tools/GizmoEditorTool.h"

struct FRay;
struct FViewportPanelInput;
struct FRenderView;

class GizmoEditorController
{

public:
    struct FConfig
    {
        float hitRadiusPx = 15.0f;
        bool  bGizmoFirst = true;

        // Scaling feel: 0.25 = gentle, 0.6 = aggressive
        float scaleSensitivity = 0.25f;

        // Prevent negative/zero scale flips while dragging
        float minAxisScaleMul = 0.01f;
    };

    // What the controller produced this frame (apply to selection/pivot).
    // All deltas are expressed in WORLD SPACE, but “axis selection” is based on current gizmo basis.
    struct FGizmoTransformDelta
    {
        bool bHasDelta = false;

        GizmoEditorTool::EMode  mode   = GizmoEditorTool::EMode::Translate;
        GizmoEditorTool::ESpace space  = GizmoEditorTool::ESpace::World;
        GizmoEditorTool::EHandle handle = GizmoEditorTool::EHandle::None;

        // Pivot used for the computation (usually gizmo position)
        FVector3 pivotWS{0,0,0};

        // Translate: add this to actor world position
        FVector3 deltaTranslationWS{0,0,0};

        // Rotate: rotate around (pivotWS, rotationAxisWS) by rotationAngleRad
        FVector3 rotationAxisWS{1,0,0};
        float    rotationAngleRad = 0.0f;

        // Scale: multiplicative axis scale in gizmo basis (X/Y/Z correspond to basis axes)
        // Example: (1.2,1,1) means scale along basis X only
        FVector3 deltaScaleMul{1,1,1};
    };

    struct FResult
    {
        GizmoEditorTool::EHandle hovered = GizmoEditorTool::EHandle::None;
        GizmoEditorTool::EHandle active  = GizmoEditorTool::EHandle::None;

        bool bConsumesClick = false; // block actor picking this frame
        bool bWantsCapture  = false; // request exclusive mouse routing
        bool bBeganCapture  = false;
        bool bEndedCapture  = false;

        FGizmoTransformDelta manipulation;
    };

private:
    GizmoEditorTool m_GizmoTool;

    FConfig m_Cfg{};
    uint32_t m_BaseHitID = 0;

    GizmoEditorTool::EMode  m_Mode  = GizmoEditorTool::EMode::Translate;
    GizmoEditorTool::ESpace m_Space = GizmoEditorTool::ESpace::World;

    bool m_bCapturing = false;
    GizmoEditorTool::EHandle m_Hovered = GizmoEditorTool::EHandle::None;
    GizmoEditorTool::EHandle m_Active  = GizmoEditorTool::EHandle::None;

    // Drag state for the current capture
    struct FGizmoDragState
    {
        bool bActive = false;

        GizmoEditorTool::EMode mode = GizmoEditorTool::EMode::Translate;
        GizmoEditorTool::ESpace space = GizmoEditorTool::ESpace::World;
        GizmoEditorTool::EHandle handle = GizmoEditorTool::EHandle::None;

        FTransform gizmoStartXf{};
        FVector3 pivotWS{0,0,0};

        // Basis at capture start (depends on World/Local)
        FVector3 X{1,0,0}, Y{0,1,0}, Z{0,0,1};

        // Constraints
        FVector3 axisWS{1,0,0};         // for axis translate/rotate/scale
        FVector3 planeNormalWS{0,0,1};  // for plane translate / rotate plane

        // “Start” values for stable deltas
        bool bHasStartHit = false;
        FVector3 startHitWS{0,0,0};

        bool bHasAxisSStart = false;
        float axisSStart = 0.0f;

        // Rotation accumulation for smoothness
        FVector3 prevVecWS{1,0,0};
        float angleAccumRad = 0.0f;

        bool bHasUniformDir = false;
        FVector3 uniformDirWS{1,0,0};

        // Plane-scale start (in gizmo basis)
        bool  bHasPlaneScaleStart = false;
        float planeU0 = 0.0f;
        float planeV0 = 0.0f;
        FVector3 planeAxisU{1,0,0};
        FVector3 planeAxisV{0,1,0};

    };

    FGizmoDragState m_Drag{};

private:
    [[nodiscard]] GizmoEditorTool::EHandle HitTest(const DebugDraw& debugDraw, const FRenderView& view,
                                     const FMatrix4& viewMat, const FMatrix4& projMat,
                                     float mouseX_px, float mouseY_px) const;

    static float ComputeFadeMul(const FMatrix4& viewMat, const FVector3& gizmoPosWS);

    static FRay MakeMouseRayWS(const FRenderView& view,
                           const FMatrix4& viewMat,
                           const FMatrix4& projMat,
                           float mouseX_px,
                           float mouseY_px);

    static bool RayPlaneThroughPoint(const FRay& ray,
                                 const FVector3& P0,
                                 const FVector3& N, // should be unit-ish
                                 FVector3& outHit);

    // Closest points between ray and an infinite axis line: L(s)=P0 + A*s
    static bool RayAxisClosestS(const FRay& ray,
                            const FVector3& P0,
                            const FVector3& A_unit,
                            float& outS);

    static FVector3 AxisDragPlaneNormal(const FVector3& axisUnit, const FVector3& camFwdUnit);

public:
    GizmoEditorController() = default;

    void SetConfig(const FConfig& cfg) { m_Cfg = cfg; }

    void SetMode(GizmoEditorTool::EMode mode) { m_Mode = mode; }
    void SetSpace(GizmoEditorTool::ESpace space) { m_Space = space; }
    void SetBaseHitID(uint32_t baseHitID) { m_BaseHitID = baseHitID; }

    [[nodiscard]] bool IsCapturing() const { return m_bCapturing; }

    void CancelCapture();

    bool BeginDrag(const FRay& rayWS, const FVector3& camFwd, const FTransform& gizmoXf);
    bool UpdateDrag(const FRay& rayWS, const FVector3& camFwd, FGizmoTransformDelta& outDelta);
    void EndDrag();

    FResult UpdateAndDraw(DebugDraw& debugDraw,
                          const FRenderView& view,
                          const FMatrix4& viewMat,
                          const FMatrix4& projMat,
                          const FVector3& camPos,
                          const FVector3& camFwd,
                          const FTransform& gizmoXf,
                          bool bDraw,
                          bool bAllowBeginCapture,
                          const FViewportPanelInput& input);

    static void BuildBasisWS(const FTransform& xf, GizmoEditorTool::ESpace space,
                       FVector3& outX, FVector3& outY, FVector3& outZ);
};
