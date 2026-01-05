//  Copyright 2025 JesseTheCatLover. All Rights Reserved.

#pragma once

#include <cstdint>

#include "Core/Math/FMatrix4.h"
#include "Core/Math/FTransform.h"
#include "Framework/DebugDrawFramework.h"
#include "Tools/GizmoEditorTool.h"

struct FViewportPanelInput;
struct FRenderView;

class GizmoEditorController
{
public:
    struct FConfig
    {
        float hitRadiusPx = 8.0f;
        bool  bGizmoFirst = true;
    };

    struct FResult
    {
        GizmoEditorTool::EHandle hovered = GizmoEditorTool::EHandle::None;
        GizmoEditorTool::EHandle active  = GizmoEditorTool::EHandle::None;

        bool bConsumesClick = false; // block actor picking this frame
        bool bWantsCapture = false; // request exclusive mouse routing

        bool bBeganCapture = false;
        bool bEndedCapture = false;

        // Interaction output
        bool bHasTranslationDelta = false;
        FVector3 translationDelta = FVector3(0,0,0);
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


    struct FTranslateCapture
    {
        bool bActive = false;

        GizmoEditorTool::EHandle handle = GizmoEditorTool::EHandle::None;

        FVector3 originWS = FVector3(0,0,0);

        // Basis at capture begin (world-space)
        FVector3 X = FVector3(1,0,0);
        FVector3 Y = FVector3(0,1,0);
        FVector3 Z = FVector3(0,0,1);

        // Drag plane (world): N·(P - P0)=0, store N and a point on plane
        FVector3 planeN = FVector3(0,0,1);
        FVector3 planeP = FVector3(0,0,0);

        // For axis/plane constraints
        FVector3 axis = FVector3(1,0,0);   // used for T_X/Y/Z
        FVector3 a    = FVector3(1,0,0);   // used for planes (basis A,B)
        FVector3 b    = FVector3(0,1,0);

        FVector3 startHitWS = FVector3(0,0,0);
        bool bHasStartHit = false;
    };

    FTranslateCapture m_TCapture;

    static float ComputeFadeMul(const FMatrix4& viewMat, const FVector3& gizmoPosWS);

public:
    GizmoEditorController() = default;

    void SetConfig(const FConfig& cfg) { m_Cfg = cfg; }

    void SetMode(GizmoEditorTool::EMode mode) { m_Mode = mode; }
    void SetSpace(GizmoEditorTool::ESpace space) { m_Space = space; }
    void SetBaseHitID(uint32_t baseHitID) { m_BaseHitID = baseHitID; }

    [[nodiscard]] bool IsCapturing() const { return m_bCapturing; }

    void CancelCapture();

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

private:
    [[nodiscard]] GizmoEditorTool::EHandle HitTest(const DebugDraw& debugDraw,
                                     const FRenderView& view,
                                     const FMatrix4& viewMat,
                                     const FMatrix4& projMat,
                                     float mouseX_px,
                                     float mouseY_px) const;
};
