//  Copyright 2025 JesseTheCatLover. All Rights Reserved.

#pragma once

#include "Utilities/UDynamicID.h"
#include "Rendering/RHandles.h"
#include "PanelContainer.h"
#include "../../../Editor/Private/UI/Inputs/FViewportPanelInput.h"

class EditorHost;
class EditorRuntime;
class PanelContainer;
class CameraEditorTool;

class ViewportController
{
private:
    PanelID m_PanelID = 0;

    EditorHost& m_Core;
    EditorRuntime& m_Runtime;
    PanelContainer& m_Tools;

    // One camera tool per panel
    UDynamicID::IDType m_CameraToolID = 0;

    // Cached output for UI
    RTextureHandle m_Color = {};
    void* m_NativeTexture = nullptr;

    // Cached viewport facts
    float m_Width = 0.f;
    float m_Height = 0.f;
    bool m_Focused = false;
    bool m_Hovered = false;

    int m_MSAASamples = 4;

private:
    void EnsureCameraTool();
    void UpdateCameraTool(float dt);

    // View submission
    void SubmitView();

public:
    ViewportController(PanelID panelId, EditorHost& core, EditorRuntime& runtime, PanelContainer& tools);
    ~ViewportController();

    [[nodiscard]] PanelID GetPanelID() const { return m_PanelID; }

    // Called once per frame from PanelManager
    void Update(float deltaTime, const FViewportPanelInput& ctx);

    // For panels to display
    [[nodiscard]] void* GetNativeTexture() const { return m_NativeTexture; }

    void OnPanelDestroyed();
};

