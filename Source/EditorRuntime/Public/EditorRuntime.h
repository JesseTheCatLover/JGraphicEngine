//  Copyright 2025-2026 JesseTheCatLover. All Rights Reserved.

#pragma once

#include "File/FileAPI.h"
#include "Scene/SceneAPI.h"
#include "Surface/SurfaceAPI.h"
#include "Viewport/ViewportAPI.h"

class InputSubsystem;
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
    ResourceSubsystem& m_Resource;
    InputSubsystem& m_InputSubsystem;

    EditorSceneAPI m_SceneAPI;
    EditorViewportAPI m_ViewportAPI;
    EditorSurfaceAPI m_SurfaceAPI;
    EditorFileAPI m_FileAPI;

public:
    ~EditorRuntime();

    // Mutating operations
    [[nodiscard]] EditorSceneAPI& GetScene() { return m_SceneAPI; }
    [[nodiscard]] EditorViewportAPI& GetViewport() { return m_ViewportAPI; }
    [[nodiscard]] EditorSurfaceAPI& GetSurface() { return m_SurfaceAPI; }
    [[nodiscard]] EditorFileAPI& GetFile() { return m_FileAPI; }

    static constexpr uint32_t kEditorPostProfile = 1; // TODO: Legacy: should be moved
private:
    bool InstallEditorInputMappings();
};
