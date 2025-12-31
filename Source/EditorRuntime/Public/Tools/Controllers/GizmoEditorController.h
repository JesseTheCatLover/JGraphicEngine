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
        bool bWantsCapture  = false; // request exclusive mouse routing
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
