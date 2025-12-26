//  Copyright 2025 JesseTheCatLover. All Rights Reserved.

#pragma once

#include "Scene/SceneAPI.h"
#include "Surface/SurfaceAPI.h"
#include "Tools/TEditorTools.h"
#include "Tools/GizmoEditorTool.h"
#include "Viewport/ViewportAPI.h"

class ICameraViewSource;
class CameraEditorTool;
class EngineContext;
class SceneManager;
class RendererSubsystem;

class EditorRuntime
{
    friend class EditorApp;
private:
    EditorRuntime();

    EngineContext& m_Context;
    SceneManager& m_SceneManager;
    RendererSubsystem& m_Renderer;
    IPlatformSurface& m_PlatformSurface;

    EditorSceneAPI m_SceneAPI;
    EditorViewportAPI m_ViewportAPI;
    EditorSurfaceAPI m_SurfaceAPI;

    static constexpr uint32_t kEditorPostProfile = 1; // TODO: Legacy: should be moved

public:
    ~EditorRuntime();

    // Mutating operations
    [[nodiscard]] EditorSceneAPI& GetScene() { return m_SceneAPI; }
    [[nodiscard]] EditorViewportAPI& GetViewport() { return m_ViewportAPI; }
    [[nodiscard]] EditorSurfaceAPI& GetSurface() { return m_SurfaceAPI; }

private:
};
