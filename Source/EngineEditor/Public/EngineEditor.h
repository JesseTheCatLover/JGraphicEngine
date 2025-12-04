//  Copyright 2025 JesseTheCatLover. All Rights Reserved.

#pragma once

#include "Scene/SceneAPI.h"
#include "Surface/SurfaceAPI.h"
#include "Tools/TEditorTools.h"
#include "Tools/FEditorToolFrameState.h"
#include "Viewport/ViewportAPI.h"

class ICameraViewSource;
class CameraEditorTool;
class EngineContext;
class SceneManager;
class RendererSubsystem;

class EngineEditor
{
    friend class EditorApp;
private:
    EngineEditor();

    EngineContext& m_Context;
    SceneManager& m_SceneManager;
    RendererSubsystem& m_Renderer;
    IPlatformSurface& m_PlatformSurface;

    EditorSceneAPI m_SceneAPI;
    EditorViewportAPI m_ViewportAPI;
    EditorSurfaceAPI m_SurfaceAPI;

    TEditorTools<CameraEditorTool> m_CameraTools;

public:
    ~EngineEditor();

    // Mutating operations – ONLY EditorCore may call them
    [[nodiscard]] EditorSceneAPI& GetScene() { return m_SceneAPI; }
    [[nodiscard]] EditorViewportAPI& GetViewport() { return m_ViewportAPI; }
    [[nodiscard]] EditorSurfaceAPI& GetSurface() { return m_SurfaceAPI; }

    void TickAllTools(float deltaTime, const FEditorToolFrameState& state);
    void SubmitEditorViewSources(const FEditorToolFrameState& state);

    // Camera Tool
    UDynamicID::IDType CreateCameraEditorTool();
    bool DestroyCameraEditorTool(UDynamicID::IDType cameraID);
    CameraEditorTool* GetCameraEditorTool(UDynamicID::IDType cameraID);

private:
    // Helpers:
    [[nodiscard]] std::vector<ICameraViewSource*> CollectEditorCameraViews() const;
    void TickCameraTools(float deltaTime, const FEditorToolFrameState& state);
};
