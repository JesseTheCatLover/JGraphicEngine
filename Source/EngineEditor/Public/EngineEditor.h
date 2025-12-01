//  Copyright 2025 JesseTheCatLover. All Rights Reserved.

#pragma once
#include "Scene/SceneAPI.h"
#include "Viewport/ViewportAPI.h"

class EngineContext;
class SceneManager;
class RendererSubsystem;

class EngineEditor
{
    friend class EditorApp;
    friend class EditorCore;
private:
    EngineEditor();

    EngineContext& m_Context;
    SceneManager& m_SceneManager;
    RendererSubsystem& m_Renderer;

    EditorSceneAPI m_SceneAPI;
    EditorViewportAPI m_ViewportAPI;

public:
    // Read-only engine view for external tools
    // Users get *const* access to the sub-APIs
    [[nodiscard]] const EditorSceneAPI& GetScene() const { return m_SceneAPI; }
    [[nodiscard]] const EditorViewportAPI& GetViewport() const { return m_ViewportAPI; }

private:
    // Mutating operations – ONLY EditorCore may call the
    [[nodiscard]] EditorSceneAPI& GetScene() { return m_SceneAPI; }
    [[nodiscard]] EditorViewportAPI& GetViewport() { return m_ViewportAPI; }
};
