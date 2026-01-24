//  Copyright 2025-2026 JesseTheCatLover. All Rights Reserved.

#pragma once

#include <vector>

#include "EditorRuntime.h"
#include "Core/IEditorBridge.h"
#include "Core/Memory/SmartPointers.h"

class IEditorUIBackend;
class EditorPanelTracker;
class EditorAssetCache;
class EditorLayoutModel;
class ImGuiRenderer;
class IEditorPanel;
class EditorHost;
struct GLFWwindow;

class EditorApp : public IEditorBridge
{
public:
    EditorApp();
    ~EditorApp();

    void BeginFrame();
    void RenderPanels();
    void EndFrame();

    void Shutdown();

    void OnEngineInitialized(IPlatformSurface* surface) override;
    void OnSceneLoaded(const std::string &sceneName) override;
    void OnRenderOverlay(float deltaTime) override;
    void OnTick(float deltaTime) override;

private:
    GLFWwindow* m_Window;

    // Core components
    TUniquePtr<EditorRuntime> m_EditorRuntime;
    TUniquePtr<EditorHost> m_EditorHost;

    // Backend
    TUniquePtr<IEditorUIBackend> m_EditorUIBackend;

    TUniquePtr<ImGuiRenderer> m_Renderer;
    TUniquePtr<EditorPanelTracker> m_PanelTracker;
    TUniquePtr<EditorLayoutModel> m_LayoutModel;
    TUniquePtr<EditorAssetCache> m_EditorCache;
};
