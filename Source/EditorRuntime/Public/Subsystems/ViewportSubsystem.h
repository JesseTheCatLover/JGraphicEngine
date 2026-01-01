//  Copyright 2025 JesseTheCatLover. All Rights Reserved.

#pragma once

#include "IPanelSubsystem.h"

#include "TPanelSubsystem.h"
#include "ToolService.h"
#include "Controllers/ViewportController.h"
#include "Controllers/Inputs/FViewportPanelInput.h"
#include "Controllers/Outputs/FViewportOutput.h"

class EditorHost;
class EditorRuntime;
class ToolService;

class ViewportSubsystem : public IPanelSubsystem
{
private:
    EditorHost& m_Host;
    EditorRuntime& m_Runtime;
    ToolService& m_Tools;

    using Channel = TPanelSubsystem<FViewportPanelInput, FViewportOutput, ViewportController>;
    Channel m_Channel;

    PanelID m_CaptureOwner = 0;
    EMouseCaptureKind m_SharedCaptureKind = EMouseCaptureKind::None;
    bool m_bCursorCaptured = false;

    GizmoEditorTool::EMode m_SharedGizmoMode = GizmoEditorTool::EMode::Translate;
    GizmoEditorTool::ESpace m_SharedGizmoSpace = GizmoEditorTool::ESpace::World;

    void ApplySurfaceCursorCapture(bool bShouldCapture);

public:
    ViewportSubsystem(EditorHost &host, EditorRuntime &runtime, ToolService &tools);

    void Tick(float deltaTime) override { m_Channel.Tick(deltaTime); }

    void SubmitInput(const FViewportPanelInput& input) { m_Channel.SubmitInput(input); }

    const FViewportOutput* GetOutput(const char* panelKey) const
    {
        return m_Channel.GetOutput(panelKey);
    }

    void Destroy(const char* panelKey)
    {
        m_Channel.Destroy(panelKey);
    }

    bool IsCaptureOwner(PanelID id) const;
    bool HasCapture() const { return m_SharedCaptureKind != EMouseCaptureKind::None && m_CaptureOwner != 0; }
    EMouseCaptureKind GetCaptureKind() const { return m_SharedCaptureKind; }

    bool TryBeginCapture(PanelID id, EMouseCaptureKind kind);
    void EndCapture(PanelID id);

    bool HasMouseCapture() const
    {
        return m_bCursorCaptured;
    }

    // Shared gizmo state
    GizmoEditorTool::EMode  GetGizmoMode() const  { return m_SharedGizmoMode; }
    GizmoEditorTool::ESpace GetGizmoSpace() const { return m_SharedGizmoSpace; }

    void SetGizmoMode(GizmoEditorTool::EMode mode)   { m_SharedGizmoMode = mode; }
    void SetGizmoSpace(GizmoEditorTool::ESpace space) { m_SharedGizmoSpace = space; }
};
