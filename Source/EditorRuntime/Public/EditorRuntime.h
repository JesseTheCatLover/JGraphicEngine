//  Copyright 2025 JesseTheCatLover. All Rights Reserved.

#pragma once

#include "Scene/SceneAPI.h"
#include "Surface/SurfaceAPI.h"
#include "Tools/TEditorTools.h"
#include "Tools/FEditorToolFrameState.h"
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

    static constexpr uint32_t kEditorPostProfile = 1;

public:
    ~EditorRuntime();

    // Mutating operations – ONLY EditorCore may call them
    [[nodiscard]] EditorSceneAPI& GetSceneAPI() { return m_SceneAPI; }
    [[nodiscard]] EditorViewportAPI& GetViewportAPI() { return m_ViewportAPI; }
    [[nodiscard]] EditorSurfaceAPI& GetSurfaceAPI() { return m_SurfaceAPI; }

    void SubmitEditorViewSources(const FCameraToolState& state);

private:
    // Helpers:
    void TickCameraTools(float deltaTime, const FCameraToolState& state);
};
