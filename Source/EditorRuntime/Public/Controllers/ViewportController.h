//  Copyright 2025 JesseTheCatLover. All Rights Reserved.

#pragma once

#include "Tools/FEditorToolFrameState.h"
#include "Utilities/UDynamicID.h"
#include "Rendering/RHandles.h"
#include "Viewport/FViewportPanelContext.h"

class EditorCore;
class EditorRuntime;
class EditorToolManager;
class CameraEditorTool;

class ViewportController
{
private:
    PanelID m_PanelId = 0;

    EditorCore& m_Core;
    EditorRuntime& m_Runtime;
    EditorToolManager& m_Tools;

    // One camera tool per panel
    UDynamicID::IDType m_CameraToolId = 0;

    // Cached output for UI
    RTextureHandle m_Color = {};
    void* m_NativeTexture = nullptr;

    // Cached viewport facts
    float m_Width = 0.f;
    float m_Height = 0.f;
    bool m_Focused = false;
    bool m_Hovered = false;

    int m_MSAASamples = 1;

private:
    void EnsureCameraTool();
    void UpdateCameraTool(float dt);

    // View submission
    void SubmitView();

public:
    ViewportController(PanelID panelId, EditorCore& core, EditorRuntime& runtime, EditorToolManager& tools);
    ~ViewportController();

    [[nodiscard]] PanelID GetPanelId() const { return m_PanelId; }

    // Called once per frame from ToolManager
    void Update(float dt, const FViewportPanelContext& frame);

    // For panels to display
    [[nodiscard]] void* GetNativeTexture() const { return m_NativeTexture; }

    void OnPanelDestroyed();
};

