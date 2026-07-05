//  Copyright 2025-2026 JesseTheCatLover. All Rights Reserved.

#pragma once

#include <vector>

#include "EditorRuntime.h"
#include "Core/IEditorBridge.h"
#include "Core/Memory/SmartPointers.h"

class ProjectContext;
class ShellCommandService;
class IEditorRenderer;
class IEditorUIBackend;
class EditorPanelTracker;
class EditorLayoutModel;
class IEditorPanel;
class EditorHost;
class IPlatformWindow;

class InkBlueEditorApp : public IEditorBridge
{
private:
    IPlatformWindow* m_Window;

    // Core components
    TUniquePtr<EditorRuntime> m_EditorRuntime;
    TUniquePtr<EditorHost> m_EditorHost;

    // Backend
    TUniquePtr<IEditorUIBackend> m_EditorUIBackend;

    TUniquePtr<IEditorRenderer> m_Renderer;
    TUniquePtr<EditorPanelTracker> m_PanelTracker;
    TUniquePtr<EditorLayoutModel> m_LayoutModel;

public:
    InkBlueEditorApp();
    ~InkBlueEditorApp();

    void BeginFrame();
    void RenderPanels();
    void EndFrame();

    void Shutdown();

    void OnProjectInitialized(IPlatformWindow* window, ProjectContext& projectCtx) override;
    void OnSceneLoaded(const std::string &sceneName) override;
    void OnRenderOverlay(float deltaTime) override;
    void OnTick(float deltaTime) override;

private:
    void EnsureDefaultEditorLayoutExists(ProjectContext& projectCtx);

    void RegisterEditorShellCommands(EditorHost& host, EditorLayoutModel& layout);
    static void RegisterViewCommands(ShellCommandService& shell, EditorLayoutModel& layout);
    static void RegisterViewportCommands(ShellCommandService& shell, EditorLayoutModel& layout);
    void RegisterServicesShellCommands(EditorHost& host);
};
