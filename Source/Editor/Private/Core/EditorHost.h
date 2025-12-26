//  Copyright 2025 JesseTheCatLover. All Rights Reserved.

#pragma once

#include "EditorRuntime.h"
#include "PanelSubsystem.h"

struct FViewportPanelInput;

class EditorHost
{
    friend class EditorApp;
private:
    explicit EditorHost(EditorRuntime& runtime);

    EditorRuntime& m_EngineEditor;
    PanelSubsystem m_PanelSubsystem;

public:
    // Called every frame from EditorApp::OnTick
    void Tick(float deltaTime);

    // --- Viewport section ---

    void SubmitViewportInput(const FViewportPanelInput& input);

    [[nodiscard]] void* GetViewportNativeTexture(const char* panelKey) const // TODO: Decide Should we remove these and create a public getter for panel substem? or this current approach is better? (I think we should forward to the getter)
    {
        return m_PanelSubsystem.GetViewportNativeTexture(panelKey);
    }

    void DestroyViewport(const char* panelKey)
    {
        m_PanelSubsystem.DestroyViewport(panelKey);
    }
};